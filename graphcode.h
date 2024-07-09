/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Graphcode

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef GRAPHCODE_H
#define GRAPHCODE_H

#ifdef MPI_SUPPORT
#include <classdescMP.h>

#ifdef PARMETIS
/* Metis stuff */
extern "C" void METIS_PartGraphRecursive
(unsigned* n,unsigned* xadj,unsigned* adjncy,unsigned* vwgt,unsigned* adjwgt,
 unsigned* wgtflag,unsigned* numflag,unsigned* nparts,unsigned* options, 
 unsigned* edgecut, unsigned* part);
extern "C" void METIS_PartGraphKway
(unsigned* n,unsigned* xadj,unsigned* adjncy,unsigned* vwgt,unsigned* adjwgt,
 unsigned* wgtflag,unsigned* numflag,unsigned* nparts,unsigned* options, 
 unsigned* edgecut, unsigned* part);

#include <parmetis.h>
#else
typedef int idxtype;  /* just for defining dummy weight functions */
#endif

#else
typedef int idxtype;  /* just for defining dummy weight functions */
#endif  /* MPI_SUPPORT */

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>

#ifdef ECOLAB_LIB
#include "TCL_obj_base.h"
#endif

#include <vector>
#include <set>
#include <algorithm>
#include <iostream>

#include <classdesc_access.h>
#include <object.h>
#include <pack_base.h>
#include <pack_stl.h>

namespace classdesc
{
  class RESTProcess_t;
}

namespace graphcode
{
  /// Type used for Graph object identifier
  typedef unsigned long GraphID_t;
  /** a pin with ID==bad_ID cannot be inserted into a map or wire 
   - can be used for handling boundary conditions during graph construction */
  const GraphID_t bad_ID=~0UL;

  using std::vector;
  using std::map;
  using std::set;
  using std::find_if;
  using std::find;
  using std::cout;
  using std::endl;
  using namespace classdesc;
  using classdesc::string;

#ifdef MPI_SUPPORT
  /* MPI_Finalized only available in MPI-2 standard */
  inline bool MPI_running()
    {
      int fi, ff=0; MPI_Initialized(&fi); 
#if (defined(MPI_VERSION) && MPI_VERSION>1 || defined(MPICH_NAME))
      MPI_Finalized(&ff); 
#endif  /* MPI_VERSION>1 */
      return fi&&!ff;
    }
#endif  /* MPI_SUPPORT */

  /// return my processor no.
  inline  unsigned myid() 
    {
      int m=0;
#ifdef MPI_SUPPORT
      if (MPI_running()) MPI_Comm_rank(MPI_COMM_WORLD,&m);
#endif
      return m;
  }

  /// return number of processors
  inline unsigned nprocs() 
  {
    int m=1;
#ifdef MPI_SUPPORT
    if (MPI_running()) MPI_Comm_size(MPI_COMM_WORLD,&m);
#endif
    return m;
  }

  /** Utility for for returning \a val % \a limit 
      - \a val \f$\in[-l,2l)\f$ where \f$l\f$=\a limit
  */
  template <typename TYPE>
  inline TYPE Wrap(TYPE val, TYPE limit)
  {
    if( val >= limit )
      return val-limit;
    else if( val < 0 )
      return val+limit;
    else
      return val;
  }


  class object;
  using ObjectPtr=std::shared_ptr<object>;
  using OMap=std::unordered_map<GraphID_t, ObjectPtr>;
  class Graph;

  /* base object of graphcode - can be a pin or a wire - whatever */
  /** Reference to an object type */
  class ObjRef
  {
    const OMap::value_type *payload; /* referenced data */
  public:
    
//    struct id_eq /*predicate function testing whether x's ID is a given value*/
//    {
//      GraphID_t id; 
//      id_eq(GraphID_t i): id(i) {} 
//      bool operator()(const objref& x) {return x.ID==id;}
//    };
    
    ObjRef()=default;
    ObjRef(const OMap::value_type& x): payload(&x) {}
    GraphID_t id() const {return payload? payload->first: 0;}
    int proc() const;
    object& operator*();
    object* operator->();
    const object& operator*() const;
    const object* operator->() const;
    operator void*() const;
    //    void nullify() {payload=nullptr;}  /// remove reference and delete target if managed
  };

  //#include xstr(MAP)

//  /**
//     Distributed objects database
//  */
//  class omap: public std::unordered_map<GraphID_t,std::shared_ptr<object>>
//  {
//    objref bad_thing;
//  public:
//    omap() {bad_thing.ID=bad_ID;}
//    inline objref& operator[](GraphID_t i);
//    inline omap& operator=(const omap& x);
//  };

    
  /**
     Vector of references to objects:
     - serialisable
     - semantically like vector<objref>, but with less storage
     - objects are not owned by this class
  */
//  class Ptrlist 
//  {
//    vector<objref*> list;
//    friend class omap;
//    friend class Graph;
//    friend void unpack(pack_t*,const string&,objref&);
//  public:
//    typedef vector<objref*>::size_type size_type;
//
//#if defined(__GNUC__) && !defined(__ICC)
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
//#endif
//
//    class iterator: public std::iterator<std::random_access_iterator_tag,objref>
//    {
//      typedef vector<objref*>::const_iterator vec_it;
//      vec_it iter;
//    public:
//      iterator& operator=(const vec_it& x) {iter=x; return *this;}
//      iterator() {}
//      iterator(const iterator& x) {iter=x.iter;}
//      iterator(vector<GRAPHCODE_NS::objref*>::const_iterator x)  {iter=x;}
//      objref& operator*() {return **iter;}
//      objref* operator->() {return *iter;}
//      iterator operator++() {return iterator(++iter);}
//      iterator operator--() {return iterator(--iter);}
//      iterator operator++(int) {return iterator(iter++);}
//      iterator operator--(int) {return iterator(iter--);}
//      bool operator==(const iterator& x) const {return x.iter==iter;}
//      bool operator!=(const iterator& x) const {return x.iter!=iter;}
//      size_t operator-(const iterator& x) const {return iter-x.iter;}
//      iterator operator+(size_t x) const {return iterator(iter+x);}
//    };
//    
//    iterator begin() const {return iterator(list.begin());}
//    iterator end() const {return iterator(list.end());}
//    objref& front()  {return *list.front();}
//    objref& back()  {return *list.back();}
//    const objref& front() const {return *list.front();}
//    const objref& back() const {return *list.back();}
//    objref& operator[](unsigned i) const 
//    {
//      assert(i<list.size());
//      return *list[i];
//    }
//    void push_back(objref* x) 
//    {
//      if (x->ID!=bad_ID)
//        list.push_back(&objectMap()[x->ID]); 
//    }
//    void push_back(objref& x) {push_back(&x);}     
//    void erase(GraphID_t i) 
//    {
//      vector<objref*>::iterator it;
//      for (it=list.begin(); it!=list.end(); it++) if ((*it)->ID==i) break;
//      if ((*it)->ID==i) list.erase(it);
//    }
//    void clear() {list.clear();}
//    size_type size() const {return list.size();}
//    Ptrlist& operator=(const Ptrlist &x)
//    {
//      /* assignment of these refs must also fix up pointers to be consistent 
//	 with the map */
//      list.resize(x.size());
//      for (size_type i=0; i<size(); i++)
//        list[i]=&objectMap()[x[i].ID];
//      return *this;
//    }
//    void lpack(pack_t& targ) const
//    {
//      targ<<size();
//      for (iterator i=begin(); i!=end(); i++) targ << i->ID;
//    }
//    void lunpack(pack_t& targ) 
//    {
//      clear();
//      size_type size; targ>>size;
//      for (unsigned i=0; i<size; i++)
//	{
//	  GraphID_t id; targ>>id; 
//	  push_back(&objectMap()[id]);
//	}
//    }
//  };

  // PtrList needs copy operations clobbered.
  struct PtrList: std::vector<ObjRef>
  {
    PtrList()=default;
    PtrList(const PtrList&) {}
    PtrList& operator=(const PtrList&) {return *this;}
  };
  
#if defined(__GNUC__) && !defined(__ICC)
#pragma GCC diagnostic pop
#endif

  /** 
      base class for Graphcode objects.  an object, first and foremost
      is a \c Ptrlist of other objects it is connected to (maybe its
      neighbours, maybe its classes or families to which it belongs)
  */
  class object: public Exclude<PtrList>, public classdesc::object
  {
  public:
    int proc;
    std::vector<GraphID_t> neighbours;
    /// construct the internal pointer-based neighbour list, given the list of neighbours in \a neighbours
    void updatePtrList(const OMap& o) {
      clear();
      for (auto& n: neighbours) {
        auto i=o.find(n);
        if (i!=o.end())
          emplace_back(*i);
      }
    }
    // override clone to get correct return type
    object* cloneObject() const {return static_cast<object*>(clone());}
    /// allow exposure to scripting environments
    virtual void RESTProcess(classdesc::RESTProcess_t&,const classdesc::string&) {}
    virtual idxtype weight() const {return 1;} ///< node's weight (for partitioning)
    /// weight for edge connecting \c *this to \a x
    virtual idxtype edgeweight(const ObjRef& x) const {return 1;} 
  };

  /// Curiously recursive template pattern to define classdesc'd methods
  template <class T, class Base=object>
  struct Object: public classdesc::Object<T,Base>
  {
    void RESTProcess(RESTProcess_t& r,const classdesc::string& d) override
    {
#ifdef RESTPROCESS_H
      classdesc::RESTProcess(r,d,static_cast<T&>(*this));
#endif
    }
  };
  
  inline int ObjRef::proc() const {return payload? payload->second->proc: 0;}
  inline object& ObjRef::operator*()  {assert(payload); return *payload->second;}
  inline object* ObjRef::operator->() {assert(payload); return payload->second.get();}
  inline const object& ObjRef::operator*() const {assert(payload); return *payload->second;}
  inline const object* ObjRef::operator->() const {assert(payload); return payload->second.get();}
  inline ObjRef::operator void*() const {return payload->second.get();}

  /// deep copy of \a x
  inline OMap clone(const OMap& x)
  {
    OMap r;
    for (auto& i: x)
      r.emplace(i.first, i.second->cloneObject());
    return r;
  }
  
  //  inline objref& omap::operator[](GraphID_t i)
//  {
//    if (i==bad_ID) 
//      return bad_thing;
//    auto i=find(i);
//    if (i==end()) return bad_thing;
//    return i->second;
//  } 

  /** Graph is a list of node refs stored on local processor, and has a
     map of object references (called objects) referring to the nodes.   */

  class Graph: public Exclude<PtrList>
  {
  public: //(should be) private:
    vector<vector<GraphID_t> > rec_req; 
    vector<vector<GraphID_t> > requests; 
    unsigned tag=0;  /* tag used to ensure message groups do not overlap */
    bool type_registered(const graphcode::object& x) {return x.type()>=0;}

  public:
    OMap objects;

    /**
       Rebuild the list of locally hosted objects
    */
    void rebuildPtrLists()
    {
      clear();
      for (auto& i: objects)
        {
          if (i.second->proc==myid()) emplace_back(i);
          i.second->updatePtrList(objects);
        }
    }

    /**
       remove from local memory any objects not hosted locally
    */
    void purge()
    {
      set<GraphID_t> references;
      for (auto& i: objects)
        if (i.second->proc==myid())
          {
            references.insert(i.first);
            for (auto id: i.second->neighbours)
              references.insert(id);
          }
      // now remove all unreferenced items
      for (auto i=objects.begin(); i!=objects.end();)
        if (references.count(i->first)==0)
          i=objects.erase(i);
        else ++i;
    }
    
    /** 
        print IDs of objects hosted on proc 0, for debugging purposes
    */
    void print(unsigned proc) 
    {
      if (proc==myid())
	for (auto& i: *this)
	  {
	    std::cout << " i->ID="<<i.id()<<":";
            if (i)
              for (auto& j: *i)
                std::cout << j.id() <<",";
	    std::cout << std::endl;
	  }
    }


    /* these method must be called on all processors simultaneously */
    void gather(); ///< gather all data onto processor 0
    /**
       Prepare cached copies of objects linked to by locally hosted objects
       - \a cache_requests=true means recompute the communication pattern
    */
    void prepareNeighbours(bool cache_requests=false);
    void partitionObjects(); ///< partition
    /**
       distribute objects from proc 0 according to partitioning set in the 
       \c objref's \c proc field
    */
    inline void distributeObjects(); 

    /** 
        add the specified object into the Graph, if not already present
    */
    ObjRef AddNewObject(GraphID_t id, const ObjectPtr& o)
    {
      auto i=objects.emplace(id, o).first;
      i->second->type(); /* ensure type is registered */
      assert(type_registered(*i->second));
      return *i;
    }

    /** 
        add the specified object into the Graph, replacing any already present
    */

    
    ObjRef AddObject(GraphID_t id, const ObjectPtr& o)
    {
      auto i=objects.find(id);
      if (i==objects.end())
        return AddNewObject(id,o);
      return *i;
    }
  
    /**
       add a object of type T if none already present:
       - use as graph.AddObject<foo>(id);
    */
    template <class T, class... Args>
    ObjRef AddObject(GraphID_t id, Args... args) 
    {
      auto i=objects.find(id);
      if (i==objects.end())
        return AddNewObject(id, std::make_shared<T>(std::forward<Args>(args)...));
      return *i;
    }
  };		   
  

  inline void 
  Graph::distributeObjects()
  {
#ifdef MPI_SUPPORT
    rec_req.clear();
    MPIbuf() << objectMap() << bcast(0) >> objectMap();
    rebuild_local_list();
#endif
  }

};

/* export pack/unpack routines */
//using GRAPHCODE_NS::pack;
//using GRAPHCODE_NS::unpack;

#ifdef _CLASSDESC
#pragma omit pack GRAPHCODE_NS::omap
#pragma omit unpack GRAPHCODE_NS::omap
#pragma omit isa GRAPHCODE_NS::omap
#pragma omit pack GRAPHCODE_NS::omap::iterator
#pragma omit unpack GRAPHCODE_NS::omap::iterator
#pragma omit isa GRAPHCODE_NS::omap::iterator
#pragma omit pack GRAPHCODE_NS::Ptrlist
#pragma omit unpack GRAPHCODE_NS::Ptrlist
#pragma omit pack GRAPHCODE_NS::Ptrlist::iterator
#pragma omit unpack GRAPHCODE_NS::Ptrlist::iterator
#pragma omit pack GRAPHCODE_NS::object
#pragma omit unpack GRAPHCODE_NS::object
#pragma omit pack GRAPHCODE_NS::objref
#pragma omit unpack GRAPHCODE_NS::objref
#pragma omit isa GRAPHCODE_NS::objref
#endif
 
//namespace classdesc_access
//{
//  namespace cd=classdesc;
//
//  template <>
//  struct access_pack<graphcode::object>
//  {
//    template <class U>
//    void operator()(cd::pack_t& t,const cd::string& d, U& a)
//    {pack(t,d,static_cast<const graphcode::Ptrlist&>(a));}
//  };
//
//  template <>
//  struct access_unpack<graphcode::object>
//  {
//    template <class U>
//    void operator()(cd::pack_t& t,const cd::string& d, U& a)
//    {unpack(t,d,cd::base_cast<graphcode::Ptrlist>::cast(a));}
//  };
//
//  template <>
//  struct access_pack<GRAPHCODE_NS::omap>
//  {
//    template <class U>
//    void operator()(cd::pack_t& buf, const cd::string& desc, U& arg)
//    {
//      buf << arg.size();
//      for (typename U::iterator i=arg.begin(); i!=arg.end(); i++)
//        buf << i->ID << *i;
//    }
//  };
//
//  template <>
//  struct access_unpack<GRAPHCODE_NS::omap>
//  {
//    template <class U>
//    void operator()(cd::pack_t& buf, const cd::string& desc, U& arg)
//    {
//      typename U::size_type sz; buf>>sz;
//      GRAPHCODE_NS::GraphID_t ID;
//      for (; sz>0; sz--)
//        {
//          buf>>ID;
//          buf>>arg[ID];
//        }
//    }
//  };
//
//  template <>
//  struct access_pack<GRAPHCODE_NS::Ptrlist>
//  {
//    template <class U>
//    void operator()(cd::pack_t& targ, const cd::string& desc, U& arg) 
//    {arg.lpack(targ);}
//  };
//
//  template <>
//  struct access_unpack<GRAPHCODE_NS::Ptrlist>
//  {
//    template <class U>
//    void operator()(cd::pack_t& targ, const cd::string& desc, U& arg) 
//    {arg.lunpack(targ);}
//  };
//
//#ifdef TCL_OBJ_BASE_H
//#include graphcode_TCL_obj.h
////  /* support for EcoLab TCL_obj method */
////#ifdef _CLASSDESC
////#pragma omit TCL_obj GRAPHCODE_NS::object
////#pragma omit pack classdesc_access::access_TCL_obj
////#pragma omit unpack classdesc_access::access_TCL_obj
////#endif
////  template <>
////  struct access_TCL_obj<GRAPHCODE_NS::object>
////  {
////    template <class U>
////    void operator()(cd::TCL_obj_t& targ, const cd::string& desc,U& arg)
////    {
////      static bool not_in_virt=true; // possible thread safety problem
////      if (not_in_virt)
////        {
////           TCL_obj(targ,desc+"",cd::base_cast<GRAPHCODE_NS::Ptrlist>::cast(arg));
////           TCL_obj(targ,desc+".type",arg,&GRAPHCODE_NS::object::type);
////           TCL_obj(targ,desc+".weight",arg,&GRAPHCODE_NS::object::weight);
////           not_in_virt=false;
////           arg.TCL_obj(desc); //This will probably recurse
////           not_in_virt=true;
////        }
////    }
////  }; 
//#endif
//
//  template <>
//  struct access_pack<GRAPHCODE_NS::objref>
//  {
//    template <class U>
//    void operator()(cd::pack_t& targ, const cd::string& desc, U& arg) 
//    { 
//      ::pack(targ,desc,arg.ID);
//      ::pack(targ,desc,arg.proc);
//      if (arg.nullref()) 
//        targ<<-1;
//      else 
//       {
// 	::pack(targ,desc,arg->type());
//	arg->pack(targ);
//       }
//    }
//  };
//
//  template <>
//  struct access_unpack<GRAPHCODE_NS::objref>
//  {
//    template <class U>
//    void operator()(cd::pack_t& targ, const cd::string& desc, U& arg) 
//    {
//      ::unpack(targ,desc,arg.ID);
//      ::unpack(targ,desc,arg.proc);
//      int t; targ>>t;
//      if (t<0) 
//        {
//	  arg.nullify();
//	  return;
//        }
//      else if (arg.nullref() || arg->type()!=t)
//        {
//	  arg.nullify();
//          cd::object* newobj=cd::object::create(t);
//          GRAPHCODE_NS::object* obj=dynamic_cast<GRAPHCODE_NS::object*>(newobj);
//          if (obj)
//            arg.addref(obj,true);
//          else
//            {
//              delete newobj; //invalid object heirarchy
//              return;
//            } 
//        }
//      arg->unpack(targ);
//    }
//  };
//}
  
#undef str
#undef xstr

#endif  /* GRAPHCODE_H */
