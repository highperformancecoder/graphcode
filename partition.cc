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
  struct Array /* simple array for ParMETIS use */
  {
    T *v;
    Array(unsigned n) {v=new T[n];}
    ~Array() {delete [] v;}
    operator T*() {return v;}
    T& operator[](unsigned i) {return v[i];}
  };

#ifdef MPI_SUPPORT
  void checkAddReverseEdge(vector<vector<unsigned> >& nbrs, MPIbuf& b)
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

  void partitionObjectsImpl(const PtrList& global, PtrList& local, unsigned& tag)
  {
#if defined(MPI_SUPPORT) && defined(PARMETIS)
    unsigned i, j, nedges, nvertices=global.size();

    /* ParMETIS needs vertices to be labelled contiguously on each processor */
    map<GraphId,unsigned int> pMap; 			
    vector<idx_t> counts(nprocs()+1);
    for (i=0; i<=nprocs(); i++) 
      counts[i]=0;

    /* label each pin sequentially within processor */
    for (auto& pi: global) 
      pMap[pi.id()]=counts[pi.proc()+1]++;

    /* counts becomes running sum of counts of local pins */
    for (i=1; i<nprocs(); i++) counts[i+1]+=counts[i];

    /* add offset for each processor to map */
    for (auto& pi: global)  
      pMap[pi.id()]+=counts[pi.proc()];

    /* construct a set of edges connected to each local vertex */
    vector<vector<unsigned> > nbrs(nvertices);
    {
      MPIbuf_array edgedist(nprocs());    
      for (auto& p: local)
	for (auto& n: *p)
	  {
	    if (n.id()==p.id()) continue; /* ignore self-links */
	    nbrs[pMap[p.id()]].push_back(pMap[n.id()]);
	    edgedist[n.proc()] << std::make_pair(pMap[n.id()],pMap[p.id()]);
	  }

      /* Ensure reverse edge is in graph (Metis requires graphs to be undirected */
      tag++;
      for (i=0; i<nprocs(); i++) if (i!=myid()) edgedist[i].isend(i,tag);
      /* process local list first */
      checkAddReverseEdge(nbrs,edgedist[myid()]);
      /* now get them from remote processors */
      for (i=0; i<nprocs()-1; i++)
	checkAddReverseEdge(nbrs,MPIbuf().get(MPI_ANY_SOURCE,tag));
    }

    /* compute number of edges connected to vertices local to this processor */
    nedges=0;
    for (int i=counts[myid()]; i<counts[myid()+1]; i++) 
      nedges+=nbrs[i].size();

    vector<idx_t> offsets(global.size()+1);
    vector<idx_t> edges(nedges);
    vector<idx_t> partitioning(local.size());

    /* fill adjacency arrays suitable for call to METIS */
    offsets[0]=0; 
    nedges=0;
    for (int i=counts[myid()]; i<counts[myid()+1]; i++)
      {
	for (auto& j: nbrs[i])
          edges[nedges++]=j;
	offsets[i-counts[myid()]+1]=nedges;
      }

    int weightFlag=3, numFlag=0, nParts=nprocs(), edgeCut, nCon=1;
    vector<float> tpWgts(nParts);
    vector<idx_t> vWgts(local.size()), eWgts(nedges);
    i=0;
    for (auto& p: local) vWgts[i++]=p->weight();
    /* reverse pmap */
    vector<GraphId> rpMap(nvertices); 			
    for (auto& pi: global) 
      rpMap[pMap[pi.id()]]=pi.id();
    i=1, j=0;
    for (auto p=local.begin(); p!=local.end(); p++, i++) 
      for (auto otherNode=(*p)->begin(); j<unsigned(offsets[i]); ++j, ++otherNode)
        {
          eWgts[j]=(*p)->edgeWeight(**otherNode);
        }
    for (i=0; i<unsigned(nParts); i++) tpWgts[i]=1.0/nParts;
    float ubvec[]={1.05};
    int options[]={0,0,0,0,0}; /* for production */
    //int options[]={1,0xFF,15,0,0};  /* for debugging */
    MPI_Comm comm=MPI_COMM_WORLD;
    ParMETIS_V3_PartKway(counts.data(),offsets.data(),edges.data(),vWgts.data(),eWgts.data(),
			&weightFlag,&numFlag,&nCon,&nParts,tpWgts.data(),ubvec,options,
			&edgeCut,partitioning.data(),&comm);

    /* prepare pins to be sent to remote processors */
    for (auto& p:local)
      {
        p.proc(partitioning[pMap[p.id()]-counts[myid()]]);
        assert(p.proc()<nprocs());
      }
#endif
  }
}
