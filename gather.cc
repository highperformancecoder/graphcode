/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "graphcode.h"
#include "classdesc_epilogue.h"
#ifdef ECOLAB_LIB
#include "ecolab_epilogue.h"
#endif

namespace graphcode
{
  void GraphBase::gather()
  {
#ifdef MPI_SUPPORT
    MPIbuf b; 
    if (myid()>0) 
      for (auto& p: *this) 
	{
	  assert(p);
	  b<<p.id()<<static_cast<ObjectPtrBase>(p);
	}
    b.gather(0);
    if (myid()==0)
      {
	while (b.pos()<b.size())
	  {
            GraphId id;
            b>>id;
            b>>objectRef(id);
	  }
      }
#endif
  }
}
