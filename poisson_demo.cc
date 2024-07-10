/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/* This is a simple demo of graphcode.  */

extern void initr(unsigned long seed);
extern unsigned long ibase(void);
extern double drand(void);

// we need to include mpi.h before iostream
#ifdef MPI_SUPPORT
#include <mpi.h>
#endif

#include <math.h>

#include <iostream>
#define MAP vmap
#include "graphcode.h"
#include "graphcode.cd"
using namespace graphcode;
using namespace std;

#include "poisson_demo.h"
#include "poisson_demo.cd"
#include <classdesc_epilogue.h>

/* coordinate to ID function */
struct makeID_t
{
  int size;
  makeID_t(int s): size(s) {}
  GraphID_t operator()(int x, int y)
  { return Wrap(x,size) + size*Wrap(y,size);}
};

void print(ObjRef& x)
{
  std::cout << x->id << " is connected to ";
  for (auto& j: *x)
    std::cout << j->id << " ";
  std::cout << std::endl;
};

class von: public Graph<cell>
{
public:
  void setup(int size);
  void update();
  void print();
};

void von::setup(int size)
{
  unsigned xprocs=(unsigned)sqrt(double(nprocs()));
  unsigned yprocs=nprocs()/xprocs;
  int i, j;
  makeID_t makeID(size);
  for(j=0; j<size; j++)
    for(i=0; i<size; i++)
      {
	ObjRef o=AddObject<cell>(makeID(i,j));
        o->proc=(i*xprocs) / size + (j*yprocs)/size*xprocs;
        o->neighbours.push_back(makeID(i-1,j)); 
        o->neighbours.push_back(makeID(i+1,j)); 
        o->neighbours.push_back(makeID(i,j-1)); 
        o->neighbours.push_back(makeID(i,j+1)); 
      }                                         
  rebuildPtrLists();
//  partitionObjects();
}

void cell::update(const cell& from)
{
  double sum_nbr=0;
  for (auto& n: from)
    {
      auto& nbr=dynamic_cast<const cell&>(*n);
      sum_nbr += nbr.my_value;
    }
  my_value = from.my_value + 0.1*(sum_nbr - from.size()*from.my_value);
}

void von::update()
{
  prepareNeighbours(true); /* make a copy of neighbouring objects
				      onto the current thread */
  Graph from;
  from.objects=objects.deepCopy();
  from.rebuildPtrLists();
  for(auto& i: *this)
    dynamic_cast<cell&>(*i).update( dynamic_cast<cell&>(*from.objects[i->id]) );
}
	
double error(Graph<cell>& pGraph, unsigned int size)
{
  double retval=0.0;
  for (auto& i: pGraph)
    retval += fabs(i->as<cell>()->my_value-0.5);
  return retval;
}

inline void swap(von*& x, von*& y)  { von *t=x;  x=y;  y=t;}

int main(int argc, char** argv) 
{

#ifdef MPI_SUPPORT
  MPISPMD c(argc,argv);
//  if (myid()==0) getchar();
//  MPI_Barrier(MPI_COMM_WORLD);
#endif

  if (argc<3) 
    {
      printf("usage: %s gridsize niter\n",argv[0]);
      return 1;
    }
  const int testsize=atoi(argv[1]);
  const int niter=atoi(argv[2]);
  
  /* initialise random number generator */
  initr(0xdeadbeef);
  for(int junk=0; junk<(1<<16); junk++)
    ibase();

  von g;

  g.setup(testsize);

  g.gather();
  double starterror=error(g, testsize);

  for(int t=0; t<niter; t++)
    {
#ifndef SILENT
      g.gather();
      printf("On node %d: time is: %d, error is: %5.10f, updating...\n", 
	     myid(), t, error(g, testsize));
#endif
      g.update();
    }

  g.gather();
  if (myid()==0 && error(g, testsize)/ starterror >0.2) return 1; 
  return 0;
}

