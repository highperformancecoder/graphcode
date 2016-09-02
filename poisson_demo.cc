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
using namespace GRAPHCODE_NS;
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

void print(objref& x)
{
  std::cout << x.ID << " is connected to ";
  for (Ptrlist::iterator p=x->begin(); p!=x->end(); p++)
    std::cout << p->ID << " ";
  std::cout << std::endl;
};

class von: public Graph
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
  objects[makeID(size-1,size-1)]; //optimised for vmaps
  for(j=0; j<size; j++)
    for(i=0; i<size; i++)
      {
	objref& o=objects[makeID(i,j)];
	o.proc=(i*xprocs) / size + (j*yprocs)/size*xprocs;
	if (o.proc==myid()) 
	  {
	    AddObject(cell(),o.ID);
	    o->push_back(objects[makeID(i-1,j)]);
	    o->push_back(objects[makeID(i+1,j)]);
	    o->push_back(objects[makeID(i,j-1)]);
	    o->push_back(objects[makeID(i,j+1)]);
	  }
      }
  rebuild_local_list();
  Partition_Objects();
}

void cell::update(const cell& from)
{
  double sum_nbr=0;
  for (Ptrlist::iterator n=from.begin(); n!=from.end(); n++)
    {
      cell& nbr=dynamic_cast<cell&>(**n);
      sum_nbr += nbr.my_value;
    }
  my_value = from.my_value + 0.1*(sum_nbr - from.size()*from.my_value);
}

void von::update()
{
  Prepare_Neighbours(true); /* make a copy of neighbouring objects
				      onto the current thread */
  omap back=objects;        // backing map
  for(iterator i=begin(); i!=end(); i++)
    dynamic_cast<cell&>(**i).update( dynamic_cast<cell&>(*back[i->ID]) );
}
	
double error(Graph& pGraph, unsigned int size)
{
  double retval=0.0;
  for (Ptrlist::iterator i=pGraph.begin(); i!=pGraph.end(); i++)
    retval += fabs(dynamic_cast<cell&>(**i).my_value-0.5);
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

