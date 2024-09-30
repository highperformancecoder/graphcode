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
struct MakeId
{
  int size;
  MakeId(int s): size(s) {}
  GraphId operator()(int x, int y)
  { return Wrap(x,size) + size*Wrap(y,size);}
};

void print(ObjRef& x)
{
  std::cout << x.id() << " is connected to ";
  for (auto& j: *x)
    std::cout << j.id() << " ";
  std::cout << std::endl;
};

class Von: public Graph<Cell>
{
public:
  void setup(int size);
  void update();
  void print();
};

void Von::setup(int size)
{
  unsigned xprocs=(unsigned)sqrt(double(nprocs()));
  unsigned yprocs=nprocs()/xprocs;
  int i, j;
  MakeId makeId(size);
  for(j=0; j<size; j++)
    for(i=0; i<size; i++)
      {
	ObjRef o=insertObject<>(makeId(i,j));
        o.proc((i*xprocs) / size + (j*yprocs)/size*xprocs);
        o->neighbours.push_back(makeId(i-1,j)); 
        o->neighbours.push_back(makeId(i+1,j)); 
        o->neighbours.push_back(makeId(i,j-1)); 
        o->neighbours.push_back(makeId(i,j+1)); 
      }

  rebuildPtrLists();
}

void Cell::update(const Cell& from)
{
  double sumNbr=0;
  for (auto& n: from)
    {
      auto& nbr=*n->as<Cell>();
      sumNbr += nbr.myValue;
    }
  myValue = from.myValue + 0.1*(sumNbr - from.size()*from.myValue);
}

void Von::update()
{
  prepareNeighbours(true); /* make a copy of neighbouring objects
				      onto the current thread */
  Graph from;
  from.objects=objects.deepCopy();
  from.rebuildPtrLists();
  for(auto& i: *this)
    i->as<Cell>()->update( *from.objects[i.id()]);
}
	
double localError(Graph<Cell>& pGraph, unsigned int size)
{
  double retval=0.0;
  for (auto& i: pGraph)
    retval += fabs(i->as<Cell>()->myValue-0.5);
  return retval;
}

double error(Graph<Cell>& pGraph, unsigned int size)
{
  double retval=0.0;
  for (auto& i: pGraph.objects)
    retval += fabs(i->myValue-0.5);
  return retval;
}

inline void swap(Von*& x, Von*& y)  { Von *t=x;  x=y;  y=t;}

int main(int argc, char** argv) 
{

#ifdef MPI_SUPPORT
  MPISPMD c(argc,argv);
#endif

  if (argc<3) 
    {
      printf("usage: %s gridsize niter\n",argv[0]);
      return 1;
    }
  const int testSize=atoi(argv[1]);
  const int nIter=atoi(argv[2]);
  
  /* initialise random number generator */
  initr(0xdeadbeef);
  for(int junk=0; junk<(1<<16); junk++)
    ibase();

  Von g;

  g.setup(testSize);

  // In this case, objects are created insitu, so neither of the
  // following methods are needed. They are included just to exercise
  // them for unit test purposes
  g.partitionObjects();
  g.distributeObjects();

  g.gather();
  double startError=error(g, testSize);

  for(int t=0; t<nIter; t++)
    {
#ifndef SILENT
      printf("On node %d: time is: %d, error is: %5.10f, updating...\n", 
	     myid(), t, localError(g, testSize));
#endif
      g.update();
    }

  g.gather();
  if (myid()==0)
    {
      cout << "Total error="<<error(g, testSize)<<endl;
      if (error(g, testSize)/ startError >0.2) return 1;
    }
  return 0;
}

