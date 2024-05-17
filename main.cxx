#include <cstdint>
#include <cstdio>
#include <utility>
#include <random>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include "inc/main.hxx"

using namespace std;




#pragma region CONFIGURATION
#ifndef TYPE
/** Type of edge weights. */
#define TYPE float
#endif
#ifndef MAX_THREADS
/** Maximum number of threads to use. */
#define MAX_THREADS 64
#endif
#ifndef REPEAT_BATCH
/** Number of times to repeat each batch. */
#define REPEAT_BATCH 5
#endif
#ifndef REPEAT_METHOD
/** Number of times to repeat each method. */
#define REPEAT_METHOD 5
#endif
#pragma endregion




#pragma region METHODS
#pragma region HELPERS
/**
 * Obtain the modularity of community structure on a graph.
 * @param x original graph
 * @param a louvain result
 * @param M sum of edge weights
 * @returns modularity
 */
template <class G, class K>
inline double getModularity(const G& x, const LouvainResult<K>& a, double M) {
  auto fc = [&](auto u) { return a.membership[u]; };
  return modularityBy(x, fc, M, 1.0);
}


/**
 * Obtain the modularity of community structure on a graph.
 * @param x original graph
 * @param a leiden result
 * @param M sum of edge weights
 * @returns modularity
 */
template <class G, class K>
inline double getModularity(const G& x, const LeidenResult<K>& a, double M) {
  auto fc = [&](auto u) { return a.membership[u]; };
  return modularityBy(x, fc, M, 1.0);
}


/**
 * Obtain the refinement time of the result.
 * @param a louvain result
 * @returns refinement time
 */
template <class K, class W>
inline float refinementTime(const LouvainResult<K, W>& a) {
  return 0;
}


/**
 * Obtain the refinement time of the result.
 * @param a leiden result
 * @returns refinement time
 */
template <class K, class W>
inline float refinementTime(const LeidenResult<K, W>& a) {
  return a.refinementTime;
}
#pragma endregion




#pragma region PERFORM EXPERIMENT
/**
 * Perform the experiment.
 * @param x input graph
 * @param fstream input file stream
 * @param rows number of rows/vetices in the graph
 * @param size number of lines/edges (temporal) in the graph
 * @param batchFraction fraction of edges to use in each batch
 */
template <class G>
void runExperiment(G& x, istream& fstream, size_t rows, size_t size, double batchFraction) {
  using K = typename G::key_type;
  using V = typename G::edge_value_type;
  using W = LOUVAIN_WEIGHT_TYPE;
  random_device dev;
  default_random_engine rnd(dev());
  int repeat     = REPEAT_METHOD;
  int numThreads = MAX_THREADS;
  double M = edgeWeightOmp(x)/2;
  // Follow a specific result logging format, which can be easily parsed later.
  auto glog = [&](const auto& ans, const char *technique, int numThreads, const auto& y, auto M, auto deletionsf, auto insertionsf) {
    printf(
      "{-%.3e/+%.3e batchf, %03d threads} -> "
      "{%09.1fms, %09.1fms mark, %09.1fms init, %09.1fms firstpass, %09.1fms locmove, %09.1fms refine, %09.1fms aggr, %.3e aff, %04d iters, %03d passes, %01.9f modularity} %s\n",
      double(deletionsf), double(insertionsf), numThreads,
      ans.time, ans.markingTime, ans.initializationTime, ans.firstPassTime, ans.localMoveTime, refinementTime(ans), ans.aggregationTime,
      double(ans.affectedVertices), ans.iterations, ans.passes, getModularity(y, ans, M), technique
    );
  };
  vector<tuple<K, K, V>> deletions;
  vector<tuple<K, K, V>> insertions;
  // Get community memberships on original graph (static).
  auto c0 = louvainStaticOmp(x, {5});
  glog(c0, "louvainStaticOmpOriginal", MAX_THREADS, x, M, 0.0, 0.0);
  auto b0 = leidenStaticOmp(rnd, x, {5});
  glog(b0, "leidenStaticOmpOriginal", MAX_THREADS, x, M, 0.0, 0.0);
  auto BM2 = b0.membership;
  auto BV2 = b0.vertexWeight;
  auto BC2 = b0.communityWeight;
  auto BM3 = b0.membership;
  auto BV3 = b0.vertexWeight;
  auto BC3 = b0.communityWeight;
  auto BM4 = b0.membership;
  auto BV4 = b0.vertexWeight;
  auto BC4 = b0.communityWeight;
  // Get community memberships on updated graph (dynamic).
  for (int batchIndex=0; batchIndex<BATCH_LENGTH; ++batchIndex) {
    auto y = duplicate(x);
    insertions.clear();
    auto fb = [&](auto u, auto v, auto w) {
      insertions.push_back({u, v, w});
    };
    readTemporalDo(fstream, false, true, rows, size_t(batchFraction * size), fb);
    tidyBatchUpdateU(deletions, insertions, y);
    applyBatchUpdateOmpU(y, deletions, insertions);
    LOG(""); print(y); printf(" (insertions=%zu)\n", insertions.size());
    double  M = edgeWeightOmp(y)/2;
    auto flog = [&](const auto& ans, const char *technique) {
      glog(ans, technique, numThreads, y, M, 0.0, batchFraction);
    };
    // Find static Leiden.
    auto b1 = leidenStaticOmp(rnd, y, {repeat});
    flog(b1, "leidenStaticOmp");
    // Find naive-dynamic Leiden.
    auto b2 = leidenNaiveDynamicOmp(rnd, y, deletions, insertions, BM2, BV2, BC2, {repeat});
    flog(b2, "leidenNaiveDynamicOmp");
    // Find delta-screening based dynamic Leiden.
    auto b3 = leidenDynamicDeltaScreeningOmp(rnd, y, deletions, insertions, BM3, BV3, BC3, {repeat});
    flog(b3, "leidenDynamicDeltaScreeningOmp");
    // Find frontier based dynamic Leiden.
    auto b4 = leidenDynamicFrontierOmp(rnd, y, deletions, insertions, BM4, BV4, BC4, {repeat});
    flog(b4, "leidenDynamicFrontierOmp");
    copyValuesOmpW(BM2, b2.membership);
    copyValuesOmpW(BV2, b2.vertexWeight);
    copyValuesOmpW(BC2, b2.communityWeight);
    copyValuesOmpW(BM3, b3.membership);
    copyValuesOmpW(BV3, b3.vertexWeight);
    copyValuesOmpW(BC3, b3.communityWeight);
    copyValuesOmpW(BM4, b4.membership);
    copyValuesOmpW(BV4, b4.vertexWeight);
    copyValuesOmpW(BC4, b4.communityWeight);
    swap(x, y);
  }
}


/**
 * Main function.
 * @param argc argument count
 * @param argv argument values
 * @returns zero on success, non-zero on failure
 */
int main(int argc, char **argv) {
  using K = uint32_t;
  using V = TYPE;
  install_sigsegv();
  char *file     = argv[1];
  size_t rows = strtoull(argv[2], nullptr, 10);
  size_t size = strtoull(argv[3], nullptr, 10);
  double batchFraction = strtod(argv[5], nullptr);
  omp_set_num_threads(MAX_THREADS);
  LOG("OMP_NUM_THREADS=%d\n", MAX_THREADS);
  LOG("Loading graph %s ...\n", file);
  DiGraph<K, None, V> x;
  ifstream fstream(file);
  readTemporalOmpW(x, fstream, false, true, rows, size_t(0.90 * size)); LOG(""); print(x); printf(" (90%%)\n");
  x = symmetrizeOmp(x); LOG(""); print(x); printf(" (symmetrize)\n");
  runExperiment(x, fstream, rows, size, batchFraction);
  printf("\n");
  return 0;
}
#pragma endregion
#pragma endregion
