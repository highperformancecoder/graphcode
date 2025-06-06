/* 
 @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Graphcode

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef GRAPHCODE_H
#define GRAPHCODE_H

#include <classdesc_access.h>
#include <object.h>
#include <pack_base.h>
#include <pack_stl.h>
#include <RESTProcess_base.h>
#include <polyRESTProcessBase.h>

#ifdef MPI_SUPPORT
#include <classdescMP.h>

#ifdef PARMETIS
#include <parmetis.h>
#else
typedef int idx_t;  /* just for defining dummy weight functions */
#endif

#else
typedef int idx_t;  /* just for defining dummy weight functions */
#endif  /* MPI_SUPPORT */

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>

#include <vector>
#include <set>
#include <algorithm>
#include <iostream>

namespace classdesc
{
  struct RESTProcess_t;
}

namespace graphcode
{
  /// Type used for Graph object identifier
  typedef unsigned long GraphId;
  /** a pin with ID==badId cannot be inserted into a map or wire 
   - can be used for handling boundary conditions during graph construction */
  const GraphId badId=~0UL;

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

  class ObjectPtrBase: public std::shared_ptr<graphcode::object>
  {
    GraphId m_id;
  public:
    GraphId id() const {return m_id;}
    unsigned proc=0;
    ObjectPtrBase(GraphId id=badId, const std::shared_ptr<graphcode::object>& x=nullptr):
      std::shared_ptr<object>(x), m_id(id) {}
    ObjectPtrBase(GraphId id, std::shared_ptr<graphcode::object>&& x): std::shared_ptr<object>(x), m_id(id) {}
    ObjectPtrBase& operator=(const ObjectPtrBase& x) {
      // specialisation to ensure m_id is not overwritten
      std::shared_ptr<graphcode::object>::operator=(x);
      proc=x.proc;
      return *this;
    }
  };

  template <class T> class ObjectPtr: public ObjectPtrBase
  {
  public:
    ObjectPtr(GraphId id=badId, const std::shared_ptr<T>& x=nullptr):
      ObjectPtrBase(id, x) {}
    ObjectPtr(GraphId id, std::shared_ptr<T>&& x): ObjectPtrBase(id, x) {}
    T& operator*() const {return static_cast<T&>(ObjectPtrBase::operator*());}
    T* operator->() const {return static_cast<T*>(ObjectPtrBase::operator->());}
  };
  
  /* base object of graphcode - can be a pin or a wire - whatever */
  /** Reference to an object type */
  class ObjRef
  {
  public:
    ObjectPtrBase *payload=nullptr; /* referenced data */
    CLASSDESC_ACCESS(ObjRef);
  public:
    ObjRef()=default;
    ObjRef(const ObjectPtrBase& x): payload(const_cast<ObjectPtrBase*>(&x)) {}
    GraphId id() const {return payload? payload->id(): badId;}
    unsigned proc() const {return payload? payload->proc: 0;}
    unsigned proc(unsigned p) {if (payload) payload->proc=p; return proc();}
    object& operator*() const {return **payload;}
    object* operator->() const {return payload->get();}
    operator bool() const {return payload && *payload;}
    operator ObjectPtrBase() const {return *payload;}
    void nullify() {payload->reset();}
  };

  /// Allocator class - handles SYCL USM allocation, delegates to std::allocator when not needed
#ifdef SYCL_LANGUAGE_VERSION
  template <class T>
  class Allocator
  {
    sycl::context context;
    sycl::device device;
    sycl::usm::alloc type=sycl::usm::alloc::unknown;
    template <class U> friend class Allocator;
  public:
    using value_type=T;
    using pointer=T*;
    using reference=T&;
    using difference_type=std::ptrdiff_t;
    using propagate_on_container_move_assignment=std::true_type;
    
    Allocator()=default;
    Allocator(const sycl::context& context, const sycl::device& device, sycl::usm::alloc type):
      context(context), device(device), type(type) {}
    Allocator(const sycl::queue& queue, sycl::usm::alloc type): Allocator(queue.get_context(),queue.get_device(),type) {}
    template <class U> Allocator(const Allocator<U>& x):
      context(x.context), device(x.device), type(x.type) {}
    template<class U> struct rebind {using other=Allocator<U>;};
    T* allocate(size_t n) {
      if (type==sycl::usm::alloc::unknown)
        return std::allocator<T>().allocate(n);
      else
        return sycl::malloc<T>(n,device,context,type);
    }
    void deallocate(T* p,size_t) {
      if (type==sycl::usm::alloc::unknown)
        return std::free(p);
      else
        return sycl::free(p,context);
    }
    bool operator==(const Allocator& x) const {
      return type==x.type && context==x.context && device==x.device;
    }
  };
#else
  template <class T>
  class Allocator: public std::allocator<T>
  {
  public:
    Allocator()=default;
    template<class U> constexpr Allocator(const Allocator<U>& x) noexcept:
      std::allocator<T>(x) {}
    template<class U> struct rebind {using other=Allocator<U>;};
  };
#endif
  
  /**
     Vector of references to objects:
     - serialisable
     - objects are not owned by this class
  */

  // PtrList needs copy operations clobbered.
  struct PtrList: std::vector<ObjRef,graphcode::Allocator<ObjRef>>
  {
    using Allocator=graphcode::Allocator<ObjRef>;
    PtrList()=default;
    PtrList(const PtrList& x) {}
    template <class I> PtrList(I begin, I end, const Allocator& alloc={}):
      std::vector<ObjRef,Allocator>(begin,end,alloc) {}
    PtrList& operator=(const PtrList&) {return *this;}
    // set a new allocation scheme for this vector. This copies any existing elements into the new space
    void setAllocator(const PtrList::Allocator& alloc) {
      if (get_allocator()==alloc) return; // nothing to do optimisation
      PtrList tmp(begin(),end(),alloc);
      // moving the vector moves the allocator as well, because propagate_on_container_move_assignment is true
      std::vector<ObjRef,Allocator>::operator=(std::move(tmp));
    }
  };
  
  /** 
      base class for Graphcode objects.  an object, first and foremost
      is a \c Ptrlist of other objects it is connected to (maybe its
      neighbours, maybe its classes or families to which it belongs)
  */
  class object: public Exclude<PtrList>, public classdesc::object, public classdesc::PolyRESTProcessBase
  {
  public:
    std::vector<GraphId> neighbours;
    /// construct the internal pointer-based neighbour list, given the list of neighbours in \a neighbours
    template <class OMap> void updatePtrList(const OMap& o, const Allocator& alloc={}) {
      clear();
      setAllocator(alloc);
      for (auto& n: neighbours) {
        auto i=o.find(n);
        if (i!=o.end())
          {
            assert(*i);
            emplace_back(*i);
          }
      }
    }
    /// clone an object of a particular type. Note it is incorrect to
    /// call this method on an object that is not T. Runtime checks
    /// are not performed.
    template <class T> T* cloneObject() const {
#ifndef SYCL_LANGUAGE_VERSION
      assert(dynamic_cast<const T*>(this));
#endif
      return static_cast<T*>(clone());
    }
    /// return a reference to this
    template <class T> T* as() {
#ifndef SYCL_LANGUAGE_VERSION
      assert(dynamic_cast<T*>(this));
#endif
      return static_cast<T*>(this);
    }
    template <class T> const T* as() const {
#ifndef SYCL_LANGUAGE_VERSION
      assert(dynamic_cast<const T*>(this));
#endif
      return static_cast<const T*>(this);
    }
    virtual idx_t weight() const {return 1;} ///< node's weight (for partitioning)
    /// weight for edge connecting \c *this to \a x
    virtual idx_t edgeWeight(const ObjRef& x) const {return 1;} 
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
    void RESTProcess(RESTProcess_t& r,const classdesc::string& d) const override
    {
#ifdef RESTPROCESS_H
      classdesc::RESTProcess(r,d,static_cast<const T&>(*this));
#endif
    }
  };
  

  template <class T>
  inline bool operator<(const ObjectPtr<T>& x, const ObjectPtr<T>& y)
  {return x.id()<y.id();}

  template <class T> struct Hash
  {
    size_t operator()(const ObjectPtr<T>& x) const {return std::hash<GraphId>()(x.id());}
  };
  
  template <class T> struct KeyEqual
  {
    bool operator()(const ObjectPtr<T>& x, const ObjectPtr<T>& y) const {
      return x.id()==y.id();}
  };

  template <class T> using OMapImpl=std::unordered_set<ObjectPtr<T>, Hash<T>, KeyEqual<T>, Allocator<ObjectPtr<T>>>;
  template <class T> struct OMap: public OMapImpl<T>
  {
    using Super=std::unordered_set<ObjectPtr<T>, Hash<T>, KeyEqual<T>, Allocator<ObjectPtr<T>>>;
    using Super::erase;
    using Super::count;
    using Super::insert;

    OMap(const Allocator<ObjectPtr<T>>& allocator={}): OMapImpl<T>(allocator) {}
    typename OMap<T>::Super::iterator find(GraphId id) {
      ObjectPtr<T> tmp(id); return Super::find(tmp);
    }
    typename OMap<T>::Super::const_iterator find(GraphId id) const {
      ObjectPtr<T> tmp(id); return Super::find(tmp);
    }
    size_t erase(GraphId id) {
      ObjectPtr<T> tmp(id); return Super::erase(tmp);
    }
    size_t count(GraphId id) const {
      ObjectPtr<T> tmp(id); return OMapImpl<T>::count(tmp);
    }
    ObjectPtr<T>& operator[](GraphId id) {
      // const_cast OK because id() is immutable
      return const_cast<ObjectPtr<T>&>(*OMapImpl<T>::emplace(id).first);
    }
    const ObjectPtr<T>& operator[](GraphId id) const {
        auto& i=find(id);
        if (i==this->end()) return badId;
        return *i;
    }
    OMap deepCopy() {
      OMap r;
      for (auto& x: *this)
        r.insert(ObjectPtr<T>
                 (x.id(), std::shared_ptr<T>(x? x->template cloneObject<T>(): nullptr)));
      return r;
    }
    bool noNulls() const {
      bool r=true;
      for (auto& i: *this) r &= bool(i);
      return r;
    }
    // true if all keys are distinct
    bool sane() const {
      set<GraphId> keys;
      for (auto& i: *this)
        if (!keys.insert(i.id()).second)
          return false;
      return true;
    }
  };

  class GraphBase: public PtrList
  {
  protected:
    vector<vector<GraphId> > rec_req; 
    vector<vector<GraphId> > requests; 
    unsigned tag=0;  /* tag used to ensure message groups do not overlap */
    /// checks that objects all have unique keys (ids).
    virtual bool sane() const=0;
    CLASSDESC_ACCESS(GraphBase);
  public:
    static bool typeRegistered(const graphcode::object& x) {return x.type()>=0;}
    PtrList objectRefs;
    virtual ObjectPtrBase& objectRef(GraphId)=0;

    /**
       Rebuild the list of locally hosted objects
    */
    virtual void rebuildPtrLists()=0;
    /**
       remove from local memory any objects not hosted locally
    */
    void purge()
    {
      std::unordered_set<GraphId> references;
      for (auto& i: objectRefs)
        if (i.proc()==myid())
          {
            references.insert(i.id());
            for (auto id: i->neighbours)
              references.insert(id);
          }
      // now remove all unreferenced items
      for (auto& i: this->objectRefs)
        if (references.count(i.id())==0)
          i.nullify();
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
  };

  /** Graph is a list of node refs stored on local processor, and has a
     map of object references (called objects) referring to the nodes.   */

  template <class T>
  class Graph: public GraphBase
  {
    ObjectPtrBase& objectRef(GraphId id) override {return objects[id];}
    bool sane() const override {return objects.sane();}
    CLASSDESC_ACCESS(Graph);
    graphcode::Allocator<T> cellAlloc;
    PtrList::Allocator ptrListAlloc;
  public:
    using Cell=T;
    using OMapAllocator=graphcode::Allocator<ObjectPtr<T>>;
    OMap<T> objects;

    Graph(const graphcode::Allocator<T>& cellAlloc={}, const PtrList::Allocator& ptrListAlloc={}, const typename Graph::OMapAllocator& mapAllocator={}):
      cellAlloc(cellAlloc), ptrListAlloc(ptrListAlloc), objects(mapAllocator) {}
    
    void rebuildPtrLists() override
    {
      clear();
      setAllocator(ptrListAlloc);
      objectRefs.clear();
      for (auto& i: objects)
        {
          objectRefs.emplace_back(i);
          if (i.proc==myid()) {
            assert(i);
            emplace_back(i);
          }
          if (i) i->updatePtrList(objects,ptrListAlloc);
        }
    }

    /**
       distribute objects from proc 0 according to partitioning set in the 
       \c objref's \c proc field
    */
    void distributeObjects()
    {
#ifdef MPI_SUPPORT
      rec_req.clear();
      MPIbuf() << objects << bcast(0) >> objects;
#endif
      rebuildPtrLists();
    }

    /** 
        add the specified object into the Graph, if not already present
    */
    ObjRef insertObject(const ObjectPtr<T>& o)
    {
      auto& i=*(objects.emplace(o).first);
      if (i)
        {
          i->type(); /* ensure type is registered */
          assert(typeRegistered(*i));
        }
      return i;
    }

    /** 
        add the specified object into the Graph, replacing any already present
    */
  
    
    ObjRef overwriteObject(const ObjectPtr<T>& o)
    {
      auto i=objects.find(o.id());
      if (i==objects.end())
        return insertObject(o);
      // const cast OK here because id is not changed
      const_cast<ObjectPtr<T>&>(*i)=o;
      return *i;
    }
  
    /**
       add an object of type U if none already present:
       - use as graph.AddObject<foo>(id, args...);
         where args... are any arguments required by foo's constructor
       - does not create new object if one is already present
    */
    template <class U=T, class... Args>
    ObjRef insertObject(GraphId id, Args... args) 
    {
      auto i=objects.find(id);
      if (i==objects.end())
        return insertObject(ObjectPtr<T>(id, std::allocate_shared<U>(cellAlloc,std::forward<Args>(args)...)));
      return *i;
    }
  };		   
}
  
#ifdef _CLASSDESC
#pragma omit pack graphcode::ObjectPtrBase
#pragma omit unpack graphcode::ObjectPtrBase
#pragma omit RESTProcess graphcode::ObjectPtrBase
#pragma omit pack graphcode::OMap
#pragma omit unpack graphcode::OMap
#endif

namespace classdesc_access
{
  namespace cd=classdesc;

  template <>
  struct access_pack<graphcode::ObjectPtrBase> {
    template <class U>
    void operator()(cd::pack_t& p, const cd::string& d, U& a)
    {
      p<<a.proc<<static_cast<const std::shared_ptr<graphcode::object>&>(a);
    }
  };

  template <>
  struct access_unpack<graphcode::ObjectPtrBase>
  {
    void operator()(cd::unpack_t& p, const cd::string& d, graphcode::ObjectPtrBase& a)
    {
      p>>a.proc>>static_cast<std::shared_ptr<graphcode::object>&>(a);
    }
    void operator()(cd::unpack_t& p, const cd::string& d, const graphcode::ObjectPtrBase& a)
    {
      graphcode::ObjectPtrBase tmp;
      (*this)(p,d,tmp);
    }
  };
  
  template <>
  struct access_RESTProcess<graphcode::ObjectPtrBase> {
    template <class U>
    void operator()(cd::RESTProcess_t& r, const cd::string& d, U& a)
    {
      ::RESTProcess(r,d+".id",a,&::graphcode::ObjectPtrBase::id);
      ::RESTProcess(r,d+".proc",a,&::graphcode::ObjectPtrBase::proc);
      if (a) a->RESTProcess(r,d);
    }
  };

  template <class T>
  struct access_pack<graphcode::OMap<T>> {
    template <class U>
    void operator()(cd::pack_t& p, const cd::string& d, U& a)
    {
      for (auto& i: a)
        p<<i.id()<<i;
    }
  };

  template <class T>
  struct access_unpack<graphcode::OMap<T>>
  {
    void operator()(cd::unpack_t& p, const cd::string& d, graphcode::OMap<T>& a)
    {
      while (p)
        {
          graphcode::GraphId id;
          p>>id;
          p>>a[id];
        }
    }
    void operator()(cd::unpack_t& p, const cd::string& d, const graphcode::OMap<T>&)
    {
      graphcode::OMap<T> tmp;
      (*this)(p,d,tmp);
    }
  };
}
  
#define CLASSDESC_RESTProcess___graphcode__Allocator_T_
#define CLASSDESC_json_pack___graphcode__Allocator_T_
#define CLASSDESC_json_unpack___graphcode__Allocator_T_
#define CLASSDESC_pack___graphcode__Allocator_T_
#define CLASSDESC_unpack___graphcode__Allocator_T_

namespace classdesc_access
{
  template <class T>
  struct access_pack<graphcode::Allocator<T>>:
    public classdesc::NullDescriptor<classdesc::pack_t> {};
  template <class T>
  struct access_unpack<graphcode::Allocator<T>>:
    public classdesc::NullDescriptor<classdesc::pack_t> {};
  template <class T>
  struct access_json_pack<graphcode::Allocator<T>>:
    public classdesc::NullDescriptor<classdesc::json_pack_t> {};
  template <class T>
  struct access_json_unpack<graphcode::Allocator<T>>:
    public classdesc::NullDescriptor<classdesc::json_pack_t> {};
}
#endif  /* GRAPHCODE_H */
