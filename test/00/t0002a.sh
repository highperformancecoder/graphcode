#! /bin/sh

here=`pwd`
if test $? -ne 0; then exit 2; fi
tmp=/tmp/$$
mkdir $tmp
if test $? -ne 0; then exit 2; fi
cd $tmp
if test $? -ne 0; then exit 2; fi

fail()
{
    echo "FAILED" 1>&2
    cd $here
    chmod -R u+w $tmp
    rm -rf $tmp
    exit 1
}

pass()
{
    echo "PASSED" 1>&2
    cd $here
    chmod -R u+w $tmp
    rm -rf $tmp
    exit 0
}

trap "fail" 1 2 3 15

# execute test here. PWD is temporary, refer to classdesc home directory 
# with $here

if [ -n "$AEGIS_ARCH" ]; then
  BL=`aegis -cd -bl`
  BL1=$BL/../../baseline
else #standalone test
  BL=.
  BL1=.
fi

cat >test.cc <<EOF
#define MAP hmap
#include "graphcode.h"
#include <classdesc_epilogue.h>
using namespace GRAPHCODE_NS;
int main()
{
  omap a;
  objref &o=a[0];
  omap::iterator i=a.begin();
  int j;
  for (j=0; i!=a.end() && j<5; i++,j++);
  if (j==5) 
    return 1;
  else
    return 0;
}
EOF

g++ -DTR1 -I$here -I$HOME/usr/include test.cc
if test $? -ne 0; then fail; fi
a.out
if test $? -ne 0; then fail; fi

pass
