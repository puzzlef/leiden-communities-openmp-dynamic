#pragma once
#include <memory>
#include <tuple>
#include <vector>
#include <omp.h>
#include "update.hxx"

using std::unique_ptr;
using std::tuple;
using std::vector;




#pragma region METHODS
/**
 * Ensure that the graph is symmetric.
 * @param a graph (updated)
 */
template <class G>
inline void symmetrizeU(G& a) {
  using K = typename G::key_type;
  using V = typename G::edge_value_type;
  // Obtain the list of missing edges.
  vector<tuple<K, K, V>> insertions;
  a.forEachVertexKey([&](auto u) {
    a.forEachEdgeKey(u, [&](auto v, auto w) {
      if (a.hasEdge(v, u)) return;
      insertions.emplace_back(v, u, w);
    });
  });
  // Insert the missing edges.
  for (const auto& [u, v, w] : insertions)
    a.addEdge(u, v, w);
  a.update();
}

#ifdef OPENMP
/**
 * Ensure that the graph is symmetric.
 * @param a graph (updated)
 */
template <class G>
inline void symmetrizeOmpU(G& a) {
  using K = typename G::key_type;
  using V = typename G::edge_value_type;
  // Obtain the list of missing edges.
  size_t S = a.span();
  int    T = omp_get_max_threads();
  vector<unique_ptr<vector<tuple<K, K, V>>>> insertions(T);
  for (int t=0; t<T; ++t)
    insertions[t] = std::make_unique<vector<tuple<K, K, V>>>();
  #pragma omp parallel for schedule(dynamic, 2048)
  for (size_t u=0; u<S; ++u) {
    int t = omp_get_thread_num();
    a.forEachEdge(u, [&](auto v, auto w) {
      if (a.hasEdge(v, u)) return;
      insertions[t]->emplace_back(v, u, w);
    });
  }
  #pragma omp parallel
  {
    for (int t=0; t<T; ++t) {
      for (const auto& [u, v, w] : *insertions[t])
        addEdgeOmpU(a, u, v, w);
    }
  }
  updateOmpU(a);
}
#endif
#pragma endregion
