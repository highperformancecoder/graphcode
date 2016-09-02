# Graphcode

 Imagine a network (graph in mathematical parlance) of C++ objects wired in some arbitrary topology. Furthermore, imagine that that this graph object distributes the objects across multiple MPI processes, automatically handling object migration, performs load balancing, and migrates boundary data when nodes need to access neighbouring nodes of the network.

What you've imagined is Graphcode. Graphcode depends on [Classdesc](http://classdesc.sourceforge.net) to perform the serialisation of objects required for migration between the different address spaces of different MPI processes.

Graphcode provides an abstraction of parallel programming that is as simple as data parallel programming, yet vastly more powerful in what it can represent. It represents, for example, a typical paradigm needed for mapping agent based models onto parallel computers.

Graphcode has been deployed in a combinatorial spacetime model (Madina, 2002), an artificial chemistry model (Madina, et al. 2003,2004) and integrated into the [EcoLab simulation system](http://ecolab.sf.net), where it has been deployed into a spatial Ecolab model, and a model of Jellyfish migration.

Graphcode is also downloadable as a standalone product from the SourceForge [EcoLab repository](https://sourceforge.net/projects/ecolab/files/), or is available as part of [EcoLab system](http://ecolab.sf.net).

We have not yet thoroughly evaluated graphcode's performance, relative to more traditional techniques such as data parallel programming. In part the problem is in generating meaningful benchmarks. However, the jellyfish simulation has demonstrated a 40 times speedup over 100 Intel CPUs with 1 million jellyfish, indicating that it can be very scalable.

### Publications

1.    Standish, R.K. and Madina, D. (2008). ``Classdesc and Graphcode: support for scientific programming in C++'', arXiv:cs.CE/0610120

1.    Madina, D, and Ikegami, T. (2004). ``Cellular Dynamics in a 3D Molecular Dynamics System with Chemistry'' in Artificial Life IX, Pollack et al. (eds) (MIT Press: Cambridge, MA) 461-465.

1.    Madina, D, Ono, N. and Ikegami, T. (2003). ``Cellular evolution in a 3D lattice artificial chemistry.'' In Banzhaf et al., editors, Advances in Artificial Life, volume 2801 of Lecture Notes in Computer Science, pages 59-68, Berlin, Springer.

1.    Madina, D (2002). ``Topology and Complexity: From Automata to Agents'', In Complex Systems '02, Namatame et al. (eds) Chuo University, September.

