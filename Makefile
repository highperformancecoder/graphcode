MPI=
DEBUGGING=
OPT=
PREFIX=$(HOME)/usr
INCLUDES=-I. -I../classdesc -I../classdesc/json5_parser/json5_parser -I$(HOME)/usr/include -I/usr/local/include
VPATH+=../classdesc ../classdesc/json5_parser/json5_parser $(HOME)/usr/include /usr/local/include
OBJS=gather.o prepare_neighbours.o partition.o
PATH:=../classdesc:$(PATH)

.SUFFIXES: .cc .o .d .cd .h 

FLAGS+=$(INCLUDES) -DTR1
LIBS+=-L$(HOME)/usr/lib -L/usr/local/lib -L/usr/lib -L. -lgraphcode

# insert a pause just after MPI_Init to attach debuggers (eg gdb) to processes.
ifdef MPI_DEBUG
FLAGS+=-DMPI_DEBUG
DEBUG=1
MPI=1
endif

ifdef DEBUGGING
DEBUG=1
endif

ifdef DEBUG
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

libgraphcode.a: $(OBJS)
	ar r $@ $(OBJS)

poisson_demo: poisson_demo.o mt19937b-int.o libgraphcode.a
	$(LINK) $(FLAGS) $^ $(LIBS) -o $@

test/testvmap: test/testvmap.o libgraphcode.a 
	$(LINK) $(FLAGS) $^ $(LIBS) -o $@

.cc.o:
	$(CPLUSPLUS) -c $(FLAGS) -o $@ $<

.cc.d: 
	$(CPLUSPLUS) $(FLAGS) -w -MM -MG $< >$@

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
