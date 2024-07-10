/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "graphcode.h"
#include <utility>
#include <map>
#include "classdesc_epilogue.h"
#ifdef ECOLAB_LIB
#include "ecolab_epilogue.h"
#endif

namespace graphcode
{
  using std::pair;
  using std::map;

  template <class T>
  struct array /* simple array for ParMETIS use */
  {
    T *v;
    array(unsigned n) {v=new T[n];}
    ~array() {delete [] v;}
    operator T*() {return v;}
    T& operator[](unsigned i) {return v[i];}
  };

#ifdef MPI_SUPPORT
  void check_add_reverse_edge(vector<vector<unsigned> >& nbrs, MPIbuf& b)
    {
      pair<unsigned,unsigned> edge;
      while (b.pos()<b.size())
	{
	  b>>edge;
	  vector<unsigned>::iterator begin=nbrs[edge.first].begin(),
	    end=nbrs[edge.first].end();
	  if (find(begin,end,edge.second)==end) /* edge not found, insert */
	    nbrs[edge.first].push_back(edge.second);
	}
    }
#endif

  void partitionObjects(const PtrList& global, const PtrList& local)
  {
#if defined(MPI_SUPPORT) && defined(PARMETIS)
    if (nprocs()==1) return;
    prepareNeighbours(); /* used for computing edgeweights */
    unsigned i, j, nedges, nvertices=objects.size();

    /* ParMETIS needs vertices to be labelled contiguously on each processor */
    map<GraphID_t,unsigned int> pmap; 			
    vector<idx_t> counts(nprocs()+1);
    for (i=0; i<=nprocs(); i++) 
      counts[i]=0;

    /* label each pin sequentially within processor */
    for (auto& pi: global) 
      pmap[pi->id]=counts[pi->proc+1]++;

    /* counts becomes running sum of counts of local pins */
    for (i=1; i<nprocs(); i++) counts[i+1]+=counts[i];

    /* add offset for each processor to map */
    for (auto& pi: globa)  
      pmap[pi->id]+=counts[pi->proc];

    /* construct a set of edges connected to each local vertex */
    vector<vector<unsigned> > nbrs(nvertices);
    {
      MPIbuf_array edgedist(nprocs());    
      for (auto& p: local)
	for (auto& n: *p)
	  {
	    if (n->id==p->id) continue; /* ignore self-links */
	    nbrs[pmap[p->id]].push_back(pmap[n->id]);
	    edgedist[n->proc] << make_pair(pmap[n->id],pmap[p->id]);
	  }

      /* Ensure reverse edge is in graph (Metis requires graphs to be undirected */
      tag++;
      for (i=0; i<nprocs(); i++) if (i!=myid()) edgedist[i].isend(i,tag);
      /* process local list first */
      check_add_reverse_edge(nbrs,edgedist[myid()]);
      /* now get them from remote processors */
      for (i=0; i<nprocs()-1; i++)
	check_add_reverse_edge(nbrs,MPIbuf().get(MPI_ANY_SOURCE,tag));
    }

    /* compute number of edges connected to vertices local to this processor */
    nedges=0;
    for (int i=counts[myid()]; i<counts[myid()+1]; i++) 
      nedges+=nbrs[i].size();

    vector<idx_t> offsets(size()+1);
    vector<idx_t> edges(nedges);
    vector<idx_t> partitioning(size());

    /* fill adjacency arrays suitable for call to METIS */
    offsets[0]=0; 
    nedges=0;
    for (int i=counts[myid()]; i<counts[myid()+1]; i++)
      {
	for (auto& j: nbrs[i])
          edges[nedges++]=j;
	offsets[i-counts[myid()]+1]=nedges;
      }

    int weightflag=3, numflag=0, nparts=nprocs(), edgecut, ncon=1;
    vector<float> tpwgts(nparts);
    vector<idx_t> vwgts(size()), ewgts(nedges);
    i=0;
    for (auto& p: local) vwgts[i++]=p->weight();
    /* reverse pmap */
    vector<GraphID_t> rpmap(nvertices); 			
    for (auto& pi: global) 
      rpmap[pmap[pi->id]]=pi->id;
    i=1, j=0;
    for (auto p=local.begin(); p!=local.end(); p++, i++) 
      for (; j<unsigned(offsets[i]); j++)
        {
          auto otherNode=objects.find(rpmap[(unsigned)edges[j]]);
          ewgts[j]=otherNode==objects.end()? 1: (*p)->edgeweight(*otherNode);
        }
    for (i=0; i<unsigned(nparts); i++) tpwgts[i]=1.0/nparts;
    float ubvec[]={1.05};
    int options[]={0,0,0,0,0}; /* for production */
    //int options[]={1,0xFF,15,0,0};  /* for debugging */
    MPI_Comm comm=MPI_COMM_WORLD;
    ParMETIS_V3_PartKway(counts.data(),offsets.data(),edges.data(),vwgts.data(),ewgts.data(),
			&weightflag,&numflag,&ncon,&nparts,tpwgts.data(),ubvec,options,
			&edgecut,partitioning.data(),&comm);

    rec_req.clear(); /* destroy record of previous communication patterns */

#if 0
    /* this simple minded code updates processor locations, pulls all
       data to the master, then redistributes - replaces the more
       complex code after the #if 0, which doesn't seem to work ... */
    for (auto& p: local)
      p->proc=partitioning[pmap[p->ID]-counts[myid()]];
      
    gather();
    Distribute_Objects();
    return;
#endif

    /* prepare pins to be sent to remote processors */
    for (auto& p:*this)
      p->proc=partitioning[pmap[p.id()]-counts[myid()]];

#endif
  }
}
