Design of OpenMP-based Parallel Dynamic [Leiden algorithm] for [community detection].

<br>

Many real-world graphs evolve with time. Identifying communities or clusters on such graphs is an important problem. In this technical report, we extend three dynamic approaches, namely, Naive-dynamic (ND), Delta-screening (DS), and Dynamic Frontier (DF), to our multicore implementation of the Leiden algorithm, an algorithm known for its high-quality community detection. Our experiments on a server with a 64-core AMD EPYC-7742 processor demonstrate that ND, DS, and DF Leiden achieve speedups of 1.25×, 1.24×, and 1.37× on large graphs with random batch updates, compared to Static, ND, and DS Leiden, respectively. However, on real-world dynamic graphs, ND Leiden performs the best, being on average 1.14× faster than Static Leiden. We hope our early results serve as a starting point for dynamic approaches to the Leiden algorithm on evolving graphs.


<br>

Below we illustrate the mean runtime for communities obtained with our parallel implementation of Static, Naive-dynamic (ND), Delta-screening (DS), and Dynamic Frontier (DF) Leiden on large graphs with random batch updates, on batch updates of size `10^-7|E|` to `0.1|E|` (where `|E|` is the number of edges). In (a), the speedup of each approach with respect to Static Leiden is labeled.

[![](https://i.imgur.com/7BMtsLz.png)][sheets-o1]

Next we plot the mean runtime and modularity of communities obtained with Static, ND, DS, and DF Leiden on real-world dynamic graphs on batch updates of size `10^-5|Eᴛ|` to `10^-3|Eᴛ|` (where `|Eᴛ|` is the number of temporal edges). In (a), the speedup of each approach with respect to Static Leiden is labeled.

[![](https://i.imgur.com/ZVj1f6z.png)][sheets-o2]

Refer to our technical report for more details: \
[A Starting Point for Dynamic Community Detection with Leiden Algorithm][report].

<br>

> [!NOTE]
> You can just copy `main.sh` to your system and run it. \
> For the code, refer to `main.cxx`.

[Leiden algorithm]: https://en.wikipedia.org/wiki/Leiden_algorithm
[community detection]: https://en.wikipedia.org/wiki/Community_search
[modularity]: https://en.wikipedia.org/wiki/Modularity_(networks)
[Prof. Dip Sankar Banerjee]: https://sites.google.com/site/dipsankarban/
[Prof. Kishore Kothapalli]: https://faculty.iiit.ac.in/~kkishore/
[SuiteSparse Matrix Collection]: https://sparse.tamu.edu
[sheets-o1]: https://docs.google.com/spreadsheets/d/1Wc7_D1x0qw5VhU_t6r5u4rhubUymDs0KYwHUN-qL5oo/edit?usp=sharing
[sheets-o2]: https://docs.google.com/spreadsheets/d/1MLHSIuA_0OCjmltPJikS2me3N1kfoIL4wwtD_vZF2u0/edit?usp=sharing
[report]: https://arxiv.org/abs/2405.11658

<br>
<br>


### Code structure

The code structure of our multicore implementation of Dynamic Frontier (DF) Leiden is as follows:

```bash
- inc/_algorithm.hxx: Algorithm utility functions
- inc/_bitset.hxx: Bitset manipulation functions
- inc/_cmath.hxx: Math functions
- inc/_ctypes.hxx: Data type utility functions
- inc/_cuda.hxx: CUDA utility functions
- inc/_debug.hxx: Debugging macros (LOG, ASSERT, ...)
- inc/_iostream.hxx: Input/output stream functions
- inc/_iterator.hxx: Iterator utility functions
- inc/_main.hxx: Main program header
- inc/_mpi.hxx: MPI (Message Passing Interface) utility functions
- inc/_openmp.hxx: OpenMP utility functions
- inc/_queue.hxx: Queue utility functions
- inc/_random.hxx: Random number generation functions
- inc/_string.hxx: String utility functions
- inc/_utility.hxx: Runtime measurement functions
- inc/_vector.hxx: Vector utility functions
- inc/batch.hxx: Batch update generation functions
- inc/bfs.hxx: Breadth-first search algorithms
- inc/csr.hxx: Compressed Sparse Row (CSR) data structure functions
- inc/dfs.hxx: Depth-first search algorithms
- inc/duplicate.hxx: Graph duplicating functions
- inc/Graph.hxx: Graph data structure functions
- inc/louvain.hxx: Louvain algorithm functions
- inc/leiden.hxx: Leiden algorithm functions
- inc/main.hxx: Main header
- inc/mtx.hxx: Graph file reading functions
- inc/properties.hxx: Graph Property functions
- inc/selfLoop.hxx: Graph Self-looping functions
- inc/symmetrize.hxx: Graph Symmetrization functions
- inc/transpose.hxx: Graph transpose functions
- inc/update.hxx: Update functions
- main.cxx: Experimentation code
- process.js: Node.js script for processing output logs
```

Note that each branch in this repository contains code for a specific experiment. The `main` branch contains code for the final experiment. If the intention of a branch in unclear, or if you have comments on our technical report, feel free to open an issue.

<br>
<br>


## References

- [Fast unfolding of communities in large networks; Vincent D. Blondel et al. (2008)](https://arxiv.org/abs/0803.0476)
- [Community Detection on the GPU; Md. Naim et al. (2017)](https://arxiv.org/abs/1305.2006)
- [Scalable Static and Dynamic Community Detection Using Grappolo; Mahantesh Halappanavar et al. (2017)](https://ieeexplore.ieee.org/document/8091047)
- [From Louvain to Leiden: guaranteeing well-connected communities; V.A. Traag et al. (2019)](https://www.nature.com/articles/s41598-019-41695-z)
- [CS224W: Machine Learning with Graphs | Louvain Algorithm; Jure Leskovec (2021)](https://www.youtube.com/watch?v=0zuiLBOIcsw)
- [The University of Florida Sparse Matrix Collection; Timothy A. Davis et al. (2011)](https://doi.org/10.1145/2049662.2049663)
- [Fetch-and-add using OpenMP atomic operations](https://stackoverflow.com/a/7918281/1413259)

<br>
<br>


[![](https://i.imgur.com/bJVb3DM.png)](https://www.youtube.com/watch?v=yqO7wVBTuLw&pp)<br>
[![ORG](https://img.shields.io/badge/org-puzzlef-green?logo=Org)](https://puzzlef.github.io)
