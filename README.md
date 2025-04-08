Design of OpenMP-based Dynamic [Leiden algorithm] for [community detection].

Real-world graphs often evolve over time, making community or cluster detection a crucial task. In this technical report, we extend three dynamic approaches - Naive-dynamic (ND), Delta-screening (DS), and Dynamic Frontier (DF) - to our multicore implementation of the Leiden algorithm, known for its high-quality community detection. Our experiments, conducted on a server with a 64-core AMD EPYC-7742 processor, show that ND, DS, and DF Leiden achieve average speedups of `1.37x`, `1.47x`, and `1.98x` on large graphs with random batch updates, compared to the Static Leiden algorithm - while scaling at a rate of `1.6x` for every doubling of threads. To our knowledge, this is the first attempt to apply dynamic approaches to the Leiden algorithm. We hope these early results pave the way for further development of dynamic approaches for evolving graphs.

Below we plot the time taken by Naive-dynamic (ND), Delta-screening (DS), and Dynamic Frontier (DF) Leiden, against Static Leiden on 12 large graphs with random batch updates of size `10^-7|E|` to `0.1|E|`, consisting of `80%` edge insertions and `20%` deletions. We observe that ND, DS, and DF Leiden achieve mean speedups of `1.37×`,` 1.47×`, and `1.98×`, respectively, when compared to Static Leiden. This speedup is higher on smaller batch updates, with ND, DS, and DF Leiden being on average `1.46×`, `1.74×`, and `3.72×`, respectively, when compared to Static Leiden, on batch updates of size `10^−7|E|`.

[![](https://i.imgur.com/dOY2xuE.png)][sheets-o1]

Next, we plot the modularity of communities identified by ND, DS, and DF Leiden, compared to Static Leiden. On average, the modularity of communities from Static Leiden is slightly lower. However, we notice that modularity of communities identified by our algorithms do not differ by more than `0.002` from Static Leiden, on average.

[![](https://i.imgur.com/HkvkybT.png)][sheets-o1]

Refer to our technical report for more details: \
[A Starting Point for Dynamic Community Detection with Leiden Algorithm][report].

<br>

> [!NOTE]
> You can just copy `main.sh` to your system and run it. \
> For the code, refer to `main.cxx`.

[Leiden algorithm]: https://www.nature.com/articles/s41598-019-41695-z
[community detection]: https://en.wikipedia.org/wiki/Community_search
[Prof. Dip Sankar Banerjee]: https://sites.google.com/site/dipsankarban/
[Prof. Kishore Kothapalli]: https://faculty.iiit.ac.in/~kkishore/
[SuiteSparse Matrix Collection]: https://sparse.tamu.edu
[sheets-o1]: https://docs.google.com/spreadsheets/d/1hJBGzr2Xo9PLkiH2zDctvIoD8fXbstqOr1J2Qf6zeVY/edit?usp=sharing
[report]: https://arxiv.org/abs/2405.11658

<br>
<br>


### Code structure

The code structure is as follows:

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
- inc/leiden.hxx: Leiden algorithm functions
- inc/louvian.hxx: Louvian algorithm functions
- inc/main.hxx: Main header
- inc/mtx.hxx: Graph file reading functions
- inc/properties.hxx: Graph Property functions
- inc/selfLoop.hxx: Graph Self-looping functions
- inc/snap.hxx: SNAP dataset reading functions
- inc/symmetricize.hxx: Graph Symmetricization functions
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


[![](https://i.imgur.com/Z0g3W0u.jpg)](https://www.youtube.com/watch?v=yqO7wVBTuLw&pp)<br>
[![ORG](https://img.shields.io/badge/org-puzzlef-green?logo=Org)](https://puzzlef.github.io)
![](https://ga-beacon.deno.dev/G-KD28SG54JQ:hbAybl6nQFOtmVxW4if3xw/github.com/puzzlef/leiden-communities-openmp-dynamic)
