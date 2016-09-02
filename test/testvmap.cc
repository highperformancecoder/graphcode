/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/*
  test that adding new vmap entries does not stuff up ptrlists
*/


#define MAP vmap
#include "../graphcode.h"
using namespace GRAPHCODE_NS;

class myobject: public classdesc::Object<myobject, GRAPHCODE_NS::object>
{
};

// we're not actually using serialisation here...
namespace classdesc_access
{
  template <>
  struct access_pack<myobject>:
    public classdesc::NullDescriptor<classdesc::pack_t> {};
  template <>
  struct access_unpack<myobject>:
    public classdesc::NullDescriptor<classdesc::pack_t> {};
}

#include <classdesc_epilogue.h>

int main()
{
  Graph g;
  g.AddObject<myobject>(0);
  g.objects[0]->push_back(g.objects[0]);
  g.AddObject<myobject>(1);
  g.objects[1]->push_back(g.objects[1]);
  return &(*g.objects[0])[0] == &g.objects[0];
}
