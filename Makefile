MPI=
DEBUGGING=
OPT=
# specify any additional map names here
MAP=hmap
PREFIX=$(HOME)/usr
INCLUDES=-I../classdesc -I$(HOME)/usr/include -I/usr/local/include
OBJS=gather.o prepare_neighbours.o partition.o
ALLOBJS=$(OBJS:%.o=%.$(MAP)) $(OBJS:%.o=%.vmap)

.SUFFIXES: .cc .o .d .cd .h .$(MAP) .vmap

FLAGS+=$(INCLUDES) -DTR1
LIBS+=-L$(HOME)/usr/lib -L/usr/local/lib -L/usr/lib -L. -lgraphcode

ifdef DEBUGGING
FLAGS+=-g
else
OPT=-O3
endif
FLAGS+=$(OPT)

# use mpicc etc by default if MPI set
ifdef MPI
HAVE_mpiCC=$(shell if which mpiCC>&/dev/null; then echo 1; fi)
CC=mpicc
ifeq ($(HAVE_mpiCC),1)
CPLUSPLUS=mpiCC
else
# newer versions of mpich use this name!!
CPLUSPLUS=mpicxx
endif
LIBS+=-lparmetis -lmetis
FLAGS+=-DPARMETIS
LINK=$(CPLUSPLUS)
CPP=$(CPLUSPLUS) -E

# disable inclusion of mpi++.h in the MPICH case (don't know what the problem is here)
FLAGS+=-DMPI_SUPPORT -UHAVE_MPI_CPP
else
CC=gcc
CPLUSPLUS=g++
LINK=g++
CPP=g++ -E
endif

ifdef AEGIS
FLAGS+=-DSILENT
aegis-all: all test/testvmap
endif

all: libgraphcode.a poisson_demo

libgraphcode.a: $(ALLOBJS)
	ar r $@ $(ALLOBJS)

poisson_demo: poisson_demo.o mt19937b-int.o libgraphcode.a
	$(LINK) $(FLAGS) $^ $(LIBS) -o $@

test/testvmap: test/testvmap.o libgraphcode.a 
	$(LINK) $(FLAGS) $^ $(LIBS) -o $@

.SUFFIXES: .cc .c .h .d .o .$(MAP)

.cc.$(MAP): 
	rm -f $@
	$(CPLUSPLUS) -c $(FLAGS) -DMAP=$(MAP) -o $@ $<

.cc.vmap: 
	rm -f $@

	$(CPLUSPLUS) -c $(FLAGS) -DMAP=vmap -o $@ $<

.cc.o:
	$(CPLUSPLUS) -c $(FLAGS) -o $@ $<

.cc.d: 
	gcc $(FLAGS) -w -MM -MG $< >$@

.h.cd:
	classdesc -nodef pack unpack <$< >$@

ifneq ($(MAKECMDGOALS),clean)
include $(OBJS:.o=.d) poisson_demo.d
endif

clean:
	rm -f *.a *.o *~ *.d *.cd *.vmap *.hmap \#* poisson_demo 
	cd doc; rm -f *~ *.aux *.dvi *.log *.blg *.toc *.lof
	cd test; rm -f testvmap *.a *.o *~ *.d *.cd *.vmap *.hmap \#* 

install: libgraphcode.a
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/include
	-cp -f libgraphcode.a $(PREFIX)/lib
	-cp -f *.h vmap hmap $(PREFIX)/include
