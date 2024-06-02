#pragma once
#include <utility>
#include <tuple>
#include <vector>
#include <random>
#include <algorithm>
#include "_main.hxx"
#include "Graph.hxx"
#include "properties.hxx"
#include "csr.hxx"
#ifdef OPENMP
#include <omp.h>
#endif

using std::tuple;
using std::vector;
using std::uniform_int_distribution;
using std::make_pair;
using std::move;
using std::swap;
using std::get;
using std::min;
using std::max;




#pragma region TYPES
/**
 * Options for Leiden algorithm.
 */
struct LeidenOptions {
  #pragma region DATA
  /** Number of times to repeat the algorithm [1]. */
  int repeat;
  /** Resolution parameter for modularity [1]. */
  double resolution;
  /** Tolerance for convergence [1e-2]. */
  double tolerance;
  /** Tolerance for aggregation [0.8]. */
  double aggregationTolerance;
  /** Tolerance drop factor after each pass [10]. */
  double toleranceDrop;
  /** Maximum number of iterations per pass [20]. */
  int maxIterations;
  /** Maximum number of passes [10]. */
  int maxPasses;
  #pragma endregion


  #pragma region CONSTRUCTORS
  /**
   * Define options for Leiden algorithm.
   * @param repeat number of times to repeat the algorithm [1]
   * @param resolution resolution parameter for modularity [1]
   * @param tolerance tolerance for convergence [1e-2]
   * @param aggregationTolerance tolerance for aggregation [0.8]
   * @param toleranceDrop tolerance drop factor after each pass [10]
   * @param maxIterations maximum number of iterations per pass [20]
   * @param maxPasses maximum number of passes [10]
   */
  LeidenOptions(int repeat=1, double resolution=1, double tolerance=1e-2, double aggregationTolerance=1.0, double toleranceDrop=10, int maxIterations=20, int maxPasses=10) :
  repeat(repeat), resolution(resolution), tolerance(tolerance), aggregationTolerance(aggregationTolerance), toleranceDrop(toleranceDrop), maxIterations(maxIterations), maxPasses(maxPasses) {}
  #pragma endregion
};


/** Weight to be used in hashtable. */
#define LEIDEN_WEIGHT_TYPE double




/**
 * Result of Leiden algorithm.
 * @tparam K key type (vertex-id)
 * @tparam W weight type
 */
template <class K, class W=LEIDEN_WEIGHT_TYPE>
struct LeidenResult {
  #pragma region DATA
  /** Community membership each vertex belongs to. */
  vector<K> membership;
  /** Total edge weight of each vertex. */
  vector<W> vertexWeight;
  /** Total edge weight of each community. */
  vector<W> communityWeight;
  /** Number of iterations performed. */
  int iterations;
  /** Number of passes performed. */
  int passes;
  /** Time spent in milliseconds. */
  float time;
  /** Time spent in milliseconds for initial marking of affected vertices. */
  float markingTime;
  /** Time spent in initializing community memberships and total vertex/community weights. */
  float initializationTime;
  /** Time spent in milliseconds in first pass. */
  float firstPassTime;
  /** Time spent in milliseconds in local-moving phase. */
  float localMoveTime;
  /** Time spent in milliseconds in refinement phase. */
  float refinementTime;
  /** Time spent in milliseconds in aggregation phase. */
  float aggregationTime;
  /** Number of vertices initially marked as affected. */
  size_t affectedVertices;
  #pragma endregion


  #pragma region CONSTRUCTORS
  /**
   * Result of Leiden algorithm.
   * @param membership community membership each vertex belongs to
   * @param vertexWeight total edge weight of each vertex
   * @param communityWeight total edge weight of each community
   * @param iterations number of iterations performed
   * @param passes number of passes performed
   * @param time time spent in milliseconds
   * @param markingTime time spent in milliseconds for initial marking of affected vertices
   * @param initializationTime time spent in initializing community memberships and total vertex/community weights
   * @param firstPassTime time spent in milliseconds in first pass
   * @param localMoveTime time spent in milliseconds in local-moving phase
   * @param refinementTime time spent in milliseconds in refinement phase
   * @param aggregationTime time spent in milliseconds in aggregation phase
   * @param affectedVertices number of vertices initially marked as affected
   */
  LeidenResult(vector<K>&& membership, vector<W>&& vertexWeight, vector<W>&& communityWeight, int iterations=0, int passes=0, float time=0, float markingTime=0, float initializationTime=0, float firstPassTime=0, float localMoveTime=0, float refinementTime=0, float aggregationTime=0, size_t affectedVertices=0) :
  membership(membership), vertexWeight(vertexWeight), communityWeight(communityWeight), iterations(iterations), passes(passes), time(time), markingTime(markingTime), initializationTime(initializationTime), firstPassTime(firstPassTime), localMoveTime(localMoveTime), refinementTime(refinementTime), aggregationTime(aggregationTime), affectedVertices(affectedVertices) {}


  /**
   * Result of Leiden algorithm.
   * @param membership community membership each vertex belongs to (moved)
   * @param vertexWeight total edge weight of each vertex (moved)
   * @param communityWeight total edge weight of each community (moved)
   * @param iterations number of iterations performed
   * @param passes number of passes performed
   * @param time time spent in milliseconds
   * @param markingTime time spent in milliseconds for initial marking of affected vertices
   * @param initializationTime time spent in initializing community memberships and total vertex/community weights
   * @param firstPassTime time spent in milliseconds in first pass
   * @param localMoveTime time spent in milliseconds in local-moving phase
   * @param refinementTime time spent in milliseconds in refinement phase
   * @param aggregationTime time spent in milliseconds in aggregation phase
   * @param affectedVertices number of vertices initially marked as affected
   */
  LeidenResult(vector<K>& membership, vector<W>& vertexWeight, vector<W>& communityWeight, int iterations=0, int passes=0, float time=0, float markingTime=0, float initializationTime=0, float firstPassTime=0, float localMoveTime=0, float refinementTime=0, float aggregationTime=0, size_t affectedVertices=0) :
  membership(move(membership)), vertexWeight(move(vertexWeight)), communityWeight(move(communityWeight)), iterations(iterations), passes(passes), time(time), markingTime(markingTime), initializationTime(initializationTime), firstPassTime(firstPassTime), localMoveTime(localMoveTime), refinementTime(refinementTime), aggregationTime(aggregationTime), affectedVertices(affectedVertices) {}
  #pragma endregion
};
#pragma endregion




#pragma region METHODS
#pragma region HASHTABLES
/**
 * Allocate a number of hashtables.
 * @param vcs communities vertex u is linked to (updated)
 * @param vcout total edge weight from vertex u to community C (updated)
 * @param S size of each hashtable
 */
template <class K, class W>
inline void leidenAllocateHashtablesW(vector<vector<K>*>& vcs, vector<vector<W>*>& vcout, size_t S) {
  size_t N = vcs.size();
  for (size_t i=0; i<N; ++i) {
    vcs[i]   = new vector<K>();
    vcout[i] = new vector<W>(S);
  }
}


/**
 * Free a number of hashtables.
 * @param vcs communities vertex u is linked to (updated)
 * @param vcout total edge weight from vertex u to community C (updated)
 */
template <class K, class W>
inline void leidenFreeHashtablesW(vector<vector<K>*>& vcs, vector<vector<W>*>& vcout) {
  size_t N = vcs.size();
  for (size_t i=0; i<N; ++i) {
    delete vcs[i];
    delete vcout[i];
  }
}
#pragma endregion




#pragma region RANDOM NUMBER GENERATORS
/**
 * Allocate a number of random number generators.
 * @param rng per-thread random number generators (updated)
 * @param rnd random number generator for seeding
 */
template <class RND>
inline void leidenAllocateRngsW(vector<xorshift32_engine*>& rng, RND& rnd) {
  uniform_int_distribution<uint32_t> dis(0, UINT32_MAX);
  size_t N = rng.size();
  for (size_t i=0; i<N; ++i)
    rng[i] = new xorshift32_engine(dis(rnd));
}


/**
 * Free a number of random number generators.
 * @param rng per-thread random number generators (updated)
 */
inline void leidenFreeRngsW(vector<xorshift32_engine*>& rng) {
  size_t N = rng.size();
  for (size_t i=0; i<N; ++i)
    delete rng[i];
}
#pragma endregion




#pragma region INITIALIZE
/**
 * Find the total edge weight of each vertex.
 * @param vtot total edge weight of each vertex (updated, must be initialized)
 * @param x original graph
 */
template <class G, class W>
inline void leidenVertexWeightsW(vector<W>& vtot, const G& x) {
  x.forEachVertexKey([&](auto u) {
    x.forEachEdge(u, [&](auto v, auto w) {
      vtot[u] += w;
    });
  });
}


#ifdef OPENMP
/**
 * Find the total edge weight of each vertex.
 * @param vtot total edge weight of each vertex (updated, must be initialized)
 * @param x original graph
 */
template <class G, class W>
inline void leidenVertexWeightsOmpW(vector<W>& vtot, const G& x) {
  using  K = typename G::key_type;
  size_t S = x.span();
  #pragma omp parallel for schedule(dynamic, 2048)
  for (K u=0; u<S; ++u) {
    if (!x.hasVertex(u)) continue;
    x.forEachEdge(u, [&](auto v, auto w) { vtot[u] += w; });
  }
}
#endif


/**
 * Find the total edge weight of each community.
 * @param ctot total edge weight of each community (updated, must be initialized)
 * @param x original graph
 * @param vcom community each vertex belongs to
 * @param vtot total edge weight of each vertex
 */
template <class G, class K, class W>
inline void leidenCommunityWeightsW(vector<W>& ctot, const G& x, const vector<K>& vcom, const vector<W>& vtot) {
  x.forEachVertexKey([&](auto u) {
    K c = vcom[u];
    ctot[c] += vtot[u];
  });
}


#ifdef OPENMP
/**
 * Find the total edge weight of each community.
 * @param ctot total edge weight of each community (updated, must be initialized)
 * @param x original graph
 * @param vcom community each vertex belongs to
 * @param vtot total edge weight of each vertex
 */
template <class G, class K, class W>
inline void leidenCommunityWeightsOmpW(vector<W>& ctot, const G& x, const vector<K>& vcom, const vector<W>& vtot) {
  size_t S = x.span();
  #pragma omp parallel for schedule(static, 2048)
  for (K u=0; u<S; ++u) {
    if (!x.hasVertex(u)) continue;
    K c = vcom[u];
    #pragma omp atomic
    ctot[c] += vtot[u];
  }
}
#endif


/**
 * Initialize communities such that each vertex is its own community.
 * @param vcom community each vertex belongs to (updated, must be initialized)
 * @param ctot total edge weight of each community (updated, must be initialized)
 * @param x original graph
 * @param vtot total edge weight of each vertex
 * @param fr does vertex need to be reset?
 */
template <class G, class K, class W, class FR>
inline void leidenInitializeW(vector<K>& vcom, vector<W>& ctot, const G& x, const vector<W>& vtot, FR fr) {
  x.forEachVertexKey([&](auto u) {
    if (!fr(u)) return;
    vcom[u] = u;
    ctot[u] = vtot[u];
  });
}


/**
 * Initialize communities such that each vertex is its own community.
 * @param vcom community each vertex belongs to (updated, must be initialized)
 * @param ctot total edge weight of each community (updated, must be initialized)
 * @param x original graph
 * @param vtot total edge weight of each vertex
 */
template <class G, class K, class W>
inline void leidenInitializeW(vector<K>& vcom, vector<W>& ctot, const G& x, const vector<W>& vtot) {
  auto fr = [](auto u) { return true; };
  leidenInitializeW(vcom, ctot, x, vtot, fr);
}


#ifdef OPENMP
/**
 * Initialize communities such that each vertex is its own community.
 * @param vcom community each vertex belongs to (updated, must be initialized)
 * @param ctot total edge weight of each community (updated, must be initialized)
 * @param x original graph
 * @param vtot total edge weight of each vertex
 * @param fr is vertex in the subset of vertices to process?
 */
template <class G, class K, class W, class FR>
inline void leidenInitializeOmpW(vector<K>& vcom, vector<W>& ctot, const G& x, const vector<W>& vtot, FR fr) {
  size_t S = x.span();
  #pragma omp parallel for schedule(static, 2048)
  for (K u=0; u<S; ++u) {
    if (!x.hasVertex(u)) continue;
    if (!fr(u)) continue;
    vcom[u] = u;
    ctot[u] = vtot[u];
  }
}


/**
 * Initialize communities such that each vertex is its own community.
 * @param vcom community each vertex belongs to (updated, must be initialized)
 * @param ctot total edge weight of each community (updated, must be initialized)
 * @param x original graph
 * @param vtot total edge weight of each vertex
 */
template <class G, class K, class W>
inline void leidenInitializeOmpW(vector<K>& vcom, vector<W>& ctot, const G& x, const vector<W>& vtot) {
  auto fr = [](auto u) { return true; };
  leidenInitializeOmpW(vcom, ctot, x, vtot, fr);
}
#endif


/**
 * Initialize communities from given initial communities.
 * @param vcom community each vertex belongs to (updated, must be initialized)
 * @param ctot total edge weight of each community (updated, must be initialized)
 * @param y updated graph
 * @param vtot total edge weight of each vertex
 * @param q initial community each vertex belongs to
 */
template <class G, class K, class W>
inline void leidenInitializeFromW(vector<K>& vcom, vector<W>& ctot, const G& y, const vector<W>& vtot, const vector<K>& q) {
  y.forEachVertexKey([&](auto u) {
    K c = q[u];
    vcom[u]  = c;
    ctot[c] += vtot[u];
  });
}


#ifdef OPENMP
/**
 * Initialize communities from given initial communities.
 * @param vcom community each vertex belongs to (updated, must be initialized)
 * @param ctot total edge weight of each community (updated, must be initialized)
 * @param y updated graph
 * @param vtot total edge weight of each vertex
 * @param q initial community each vertex belongs to
 */
template <class G, class K, class W>
inline void leidenInitializeFromOmpW(vector<K>& vcom, vector<W>& ctot, const G& y, const vector<W>& vtot, const vector<K>& q) {
  size_t S = y.span();
  #pragma omp parallel for schedule(static, 2048)
  for (K u=0; u<S; ++u) {
    if (!y.hasVertex(u)) continue;
    K c = q[u];
    vcom[u]  = c;
    #pragma omp atomic
    ctot[c] += vtot[u];
  }
}
#endif


/**
 * Update weights using given edge deletions and insertions.
 * @param vtot total edge weight of each vertex (updated)
 * @param ctot total edge weight of each community (updated)
 * @param y updated graph
 * @param deletions edge deletions in batch update
 * @param insertions edge insertions in batch update
 * @param vcom community each vertex belongs to
 */
template <class G, class K, class V, class W>
inline void leidenUpdateWeightsFromU(vector<W>& vtot, vector<W>& ctot, const G& y, const vector<tuple<K, K, V>>& deletions, const vector<tuple<K, K, V>>& insertions, const vector<K>& vcom) {
  for (auto [u, v, w] : deletions) {
    K c = vcom[u];
    vtot[u] -= w;
    ctot[c] -= w;
  }
  for (auto [u, v, w] : insertions) {
    K c = vcom[u];
    vtot[u] += w;
    ctot[c] += w;
  }
}


#ifdef OPENMP
/**
 * Update weights using given edge deletions and insertions.
 * @param vtot total edge weight of each vertex (updated)
 * @param ctot total edge weight of each community (updated)
 * @param y updated graph
 * @param deletions edge deletions in batch update
 * @param insertions edge insertions in batch update
 * @param vcom community each vertex belongs to
 */
template <class G, class K, class V, class W>
inline void leidenUpdateWeightsFromOmpU(vector<W>& vtot, vector<W>& ctot, const G& y, const vector<tuple<K, K, V>>& deletions, const vector<tuple<K, K, V>>& insertions, const vector<K>& vcom) {
  #pragma omp parallel
  {
    for (auto [u, v, w] : deletions) {
      K c = vcom[u];
      if (belongsOmp(u)) vtot[u] -= w;
      if (belongsOmp(c)) ctot[c] -= w;
    }
    for (auto [u, v, w] : insertions) {
      K c = vcom[u];
      if (belongsOmp(u)) vtot[u] += w;
      if (belongsOmp(c)) ctot[c] += w;
    }
  }
}
#endif
#pragma endregion




#pragma region CHANGE COMMUNITY
/**
 * Scan an edge community connected to a vertex.
 * @param vcs communities vertex u is linked to (updated)
 * @param vcout total edge weight from vertex u to community C (updated)
 * @param u given vertex
 * @param v outgoing edge vertex
 * @param w outgoing edge weight
 * @param vcom community each vertex belongs to
 * @param vcob community bound each vertex belongs to
 */
template <bool SELF=false, bool REFINE=false, class K, class V, class W>
inline void leidenScanCommunityW(vector<K>& vcs, vector<W>& vcout, K u, K v, V w, const vector<K>& vcom, const vector<K>& vcob) {
  if (!SELF && u==v) return;
  if (REFINE && vcob[u]!=vcob[v]) return;
  K c = vcom[v];
  if (!vcout[c]) vcs.push_back(c);
  vcout[c] += w;
}


/**
 * Scan an edge community connected to a vertex.
 * @param vcs communities vertex u is linked to (updated)
 * @param vcout total edge weight from vertex u to community C (updated)
 * @param u given vertex
 * @param v outgoing edge vertex
 * @param w outgoing edge weight
 * @param vcom community each vertex belongs to
 */
template <bool SELF=false, class K, class V, class W>
inline void leidenScanCommunityW(vector<K>& vcs, vector<W>& vcout, K u, K v, V w, const vector<K>& vcom) {
  leidenScanCommunityW<SELF, false>(vcs, vcout, u, v, w, vcom, vcom);
}


/**
 * Scan communities connected to a vertex.
 * @param vcs communities vertex u is linked to (updated)
 * @param vcout total edge weight from vertex u to community C (updated)
 * @param x original graph
 * @param u given vertex
 * @param vcom community each vertex belongs to
 * @param vcob community bound each vertex belongs to
 */
template <bool SELF=false, bool REFINE=false, class G, class K, class W>
inline void leidenScanCommunitiesW(vector<K>& vcs, vector<W>& vcout, const G& x, K u, const vector<K>& vcom, const vector<K>& vcob) {
  x.forEachEdge(u, [&](auto v, auto w) { leidenScanCommunityW<SELF, REFINE>(vcs, vcout, u, v, w, vcom, vcob); });
}


/**
 * Scan communities connected to a vertex.
 * @param vcs communities vertex u is linked to (updated)
 * @param vcout total edge weight from vertex u to community C (updated)
 * @param x original graph
 * @param u given vertex
 * @param vcom community each vertex belongs to
 */
template <bool SELF=false, class G, class K, class W>
inline void leidenScanCommunitiesW(vector<K>& vcs, vector<W>& vcout, const G& x, K u, const vector<K>& vcom) {
  leidenScanCommunitiesW<SELF, false>(vcs, vcout, x, u, vcom, vcom);
}


/**
 * Clear communities scan data.
 * @param vcs total edge weight from vertex u to community C (updated)
 * @param vcout communities vertex u is linked to (updated)
 */
template <class K, class W>
inline void leidenClearScanW(vector<K>& vcs, vector<W>& vcout) {
  for (K c : vcs)
    vcout[c] = W();
  vcs.clear();
}


/**
 * Choose connected community with best delta modularity.
 * @param x original graph
 * @param u given vertex
 * @param vcom community each vertex belongs to
 * @param vtot total edge weight of each vertex
 * @param ctot total edge weight of each community
 * @param vcs communities vertex u is linked to
 * @param vcout total edge weight from vertex u to community C
 * @param M total weight of "undirected" graph (1/2 of directed graph)
 * @param R resolution (0, 1]
 * @returns [best community, delta modularity]
 */
template <bool SELF=false, class G, class K, class W>
inline auto leidenChooseCommunityGreedy(const G& x, K u, const vector<K>& vcom, const vector<W>& vtot, const vector<W>& ctot, const vector<K>& vcs, const vector<W>& vcout, double M, double R) {
  K cmax = K(), d = vcom[u];
  W emax = W();
  for (K c : vcs) {
    if (!SELF && c==d) continue;
    W e = deltaModularity(vcout[c], vcout[d], vtot[u], ctot[c], ctot[d], M, R);
    if (e>emax) { emax = e; cmax = c; }
  }
  return make_pair(cmax, emax);
}


/**
 * Choose connected community with best delta modularity.
 * @param rng random number generator
 * @param x original graph
 * @param u given vertex
 * @param vcom community each vertex belongs to
 * @param vtot total edge weight of each vertex
 * @param ctot total edge weight of each community
 * @param vcs communities vertex u is linked to
 * @param vcout total edge weight from vertex u to community C
 * @param M total weight of "undirected" graph (1/2 of directed graph)
 * @param R resolution (0, 1]
 * @returns [best community, delta modularity]
 */
template <bool SELF=false, class G, class K, class W>
inline auto leidenChooseCommunityRandom(xorshift32_engine& rng, const G& x, K u, const vector<K>& vcom, const vector<W>& vtot, const vector<W>& ctot, const vector<K>& vcs, const vector<W>& vcout, double M, double R) {
  K cmax = K(), d = vcom[u];
  W emax = W(), esum = W(), etil = W();
  for (K c : vcs) {
    if (!SELF && c==d) continue;
    W e = deltaModularity(vcout[c], vcout[d], vtot[u], ctot[c], ctot[d], M, R);
    if (e>0) esum += e;
  }
  W esel = ((rng() & 0xFFFF) / W(65536.0)) * esum;
  for (K c : vcs) {
    if (!SELF && c==d) continue;
    W e = deltaModularity(vcout[c], vcout[d], vtot[u], ctot[c], ctot[d], M, R);
    if (e>0) { etil += e; cmax = c; emax = e; }
    if (esel > etil) break;
  }
  return make_pair(cmax, emax);
}


/**
 * Choose connected community with best delta modularity.
 * @param rng random number generator
 * @param x original graph
 * @param u given vertex
 * @param vcom community each vertex belongs to
 * @param vtot total edge weight of each vertex
 * @param ctot total edge weight of each community
 * @param vcs communities vertex u is linked to
 * @param vcout total edge weight from vertex u to community C
 * @param M total weight of "undirected" graph (1/2 of directed graph)
 * @param R resolution (0, 1]
 * @returns [best community, delta modularity]
 */
template <bool SELF=false, bool RANDOM=false, class G, class K, class W>
inline auto leidenChooseCommunity(xorshift32_engine& rng, const G& x, K u, const vector<K>& vcom, const vector<W>& vtot, const vector<W>& ctot, const vector<K>& vcs, const vector<W>& vcout, double M, double R) {
  if (RANDOM) return leidenChooseCommunityRandom<SELF>(rng, x, u, vcom, vtot, ctot, vcs, vcout, M, R);
  else        return leidenChooseCommunityGreedy<SELF>     (x, u, vcom, vtot, ctot, vcs, vcout, M, R);
}


/**
 * Move vertex to another community C.
 * @param vcom community each vertex belongs to (updated)
 * @param ctot total edge weight of each community (updated)
 * @param x original graph
 * @param u given vertex
 * @param c community to move to
 * @param vtot total edge weight of each vertex
 */
template <class G, class K, class W>
inline void leidenChangeCommunityW(vector<K>& vcom, vector<W>& ctot, const G& x, K u, K c, const vector<W>& vtot) {
  K d = vcom[u];
  ctot[d] -= vtot[u];
  ctot[c] += vtot[u];
  vcom[u] = c;
}


#ifdef OPENMP
/**
 * Move vertex to another community C.
 * @param vcom community each vertex belongs to (updated)
 * @param ctot total edge weight of each community (updated)
 * @param x original graph
 * @param u given vertex
 * @param c community to move to
 * @param vtot total edge weight of each vertex
 */
template <bool REFINE=false, class G, class K, class W>
inline bool leidenChangeCommunityOmpW(vector<K>& vcom, vector<W>& ctot, const G& x, K u, K c, const vector<W>& vtot) {
  K d = vcom[u];
  if (REFINE) {
    W ctotd = W();
    #pragma omp atomic capture
    {
      ctotd    = ctot[d];
      ctot[d] -= vtot[u];
    }
    if (ctotd  > vtot[u]) {
      #pragma omp atomic
      ctot[d] += vtot[u];
      return false;
    }
  }
  else {
    #pragma omp atomic
    ctot[d] -= vtot[u];
  }
  #pragma omp atomic
  ctot[c] += vtot[u];
  vcom[u] = c;
  return true;
}
#endif
#pragma endregion




#pragma region LOCAL-MOVING PHASE
/**
 * Leiden algorithm's local moving phase.
 * @param vcom community each vertex belongs to (initial, updated)
 * @param ctot total edge weight of each community (precalculated, updated)
 * @param vaff is vertex affected flag (updated)
 * @param vcs communities vertex u is linked to (temporary buffer, updated)
 * @param vcout total edge weight from vertex u to community C (temporary buffer, updated)
 * @param rng random number generator
 * @param x original graph
 * @param vcob community bound each vertex belongs to
 * @param vtot total edge weight of each vertex
 * @param M total weight of "undirected" graph (1/2 of directed graph)
 * @param R resolution (0, 1]
 * @param L max iterations
 * @param fc has local moving phase converged?
 * @param fa is vertex allowed to be updated?
 * @param fb track communities that need to be broken
 * @returns iterations performed (0 if converged already)
 */
template <bool REFINE=false, bool RANDOM=false, class G, class K, class W, class B, class FC, class FA, class FB>
inline int leidenMoveW(vector<K>& vcom, vector<W>& ctot, vector<B>& vaff, vector<K>& vcs, vector<W>& vcout, xorshift32_engine& rng, const G& x, const vector<K>& vcob, const vector<W>& vtot, double M, double R, int L, FC fc, FA fa, FB fb) {
  int l = 0;
  W  el = W();
  for (; l<L;) {
    el = W();
    x.forEachVertexKey([&](auto u) {
      if (!fa(u)) return;
      if (!REFINE && !vaff[u]) return;
      if ( REFINE && ctot[vcom[u]]>vtot[u]) return;
      K d = vcom[u];
      leidenClearScanW(vcs, vcout);
      leidenScanCommunitiesW<false, REFINE>(vcs, vcout, x, u, vcom, vcob);
      auto [c, e] = leidenChooseCommunity<false, RANDOM>(rng, x, u, vcom, vtot, ctot, vcs, vcout, M, R);
      if (c) {
        leidenChangeCommunityW(vcom, ctot, x, u, c, vtot);
        if (!REFINE) { fb(d); fb(c); }
        if (!REFINE) x.forEachEdgeKey(u, [&](auto v) { vaff[v] = B(1); });
      }
      if (!REFINE) vaff[u] = B();
      el += e;  // l1-norm
    });
    if (REFINE || fc(el, l++)) break;
  }
  return l>1 || el? l : 0;
}


/**
 * Leiden algorithm's local moving phase.
 * @param vcom community each vertex belongs to (initial, updated)
 * @param ctot total edge weight of each community (precalculated, updated)
 * @param vaff is vertex affected flag (updated)
 * @param vcs communities vertex u is linked to (temporary buffer, updated)
 * @param vcout total edge weight from vertex u to community C (temporary buffer, updated)
 * @param rng random number generator
 * @param x original graph
 * @param vcob community bound each vertex belongs to
 * @param vtot total edge weight of each vertex
 * @param M total weight of "undirected" graph (1/2 of directed graph)
 * @param R resolution (0, 1]
 * @param L max iterations
 * @param fc has local moving phase converged?
 * @param fa is vertex allowed to be updated?
 * @returns iterations performed (0 if converged already)
 */
template <bool REFINE=false, bool RANDOM=false, class G, class K, class W, class B, class FC, class FA>
inline int leidenMoveW(vector<K>& vcom, vector<W>& ctot, vector<B>& vaff, vector<K>& vcs, vector<W>& vcout, xorshift32_engine& rng, const G& x, const vector<K>& vcob, const vector<W>& vtot, double M, double R, int L, FC fc, FA fa) {
  auto fb = [](auto u) {};
  return leidenMoveW<REFINE, RANDOM>(vcom, ctot, vaff, vcs, vcout, rng, x, vcob, vtot, M, R, L, fc, fa, fb);
}


/**
 * Leiden algorithm's local moving phase.
 * @param vcom community each vertex belongs to (initial, updated)
 * @param ctot total edge weight of each community (precalculated, updated)
 * @param vaff is vertex affected flag (updated)
 * @param vcs communities vertex u is linked to (temporary buffer, updated)
 * @param vcout total edge weight from vertex u to community C (temporary buffer, updated)
 * @param rng random number generator
 * @param x original graph
 * @param vcob community bound each vertex belongs to
 * @param vtot total edge weight of each vertex
 * @param M total weight of "undirected" graph (1/2 of directed graph)
 * @param R resolution (0, 1]
 * @param L max iterations
 * @param fc has local moving phase converged?
 * @returns iterations performed (0 if converged already)
 */
template <bool REFINE=false, bool RANDOM=false, class G, class K, class W, class B, class FC>
inline int leidenMoveW(vector<K>& vcom, vector<W>& ctot, vector<B>& vaff, vector<K>& vcs, vector<W>& vcout, xorshift32_engine& rng, const G& x, const vector<K>& vcob, const vector<W>& vtot, double M, double R, int L, FC fc) {
  auto fa = [](auto u) { return true; };
  auto fb = [](auto u) {};
  return leidenMoveW<REFINE, RANDOM>(vcom, ctot, vaff, vcs, vcout, rng, x, vcob, vtot, M, R, L, fc, fa, fb);
}


#ifdef OPENMP
/**
 * Leiden algorithm's local moving phase.
 * @param vcom community each vertex belongs to (initial, updated)
 * @param ctot total edge weight of each community (precalculated, updated)
 * @param vaff is vertex affected flag (updated)
 * @param vcs communities vertex u is linked to (temporary buffer, updated)
 * @param vcout total edge weight from vertex u to community C (temporary buffer, updated)
 * @param rng random number generators
 * @param x original graph
 * @param vcob community bound each vertex belongs to
 * @param vtot total edge weight of each vertex
 * @param M total weight of "undirected" graph (1/2 of directed graph)
 * @param R resolution (0, 1]
 * @param L max iterations
 * @param fc has local moving phase converged?
 * @param fa is vertex allowed to be updated?
 * @param fb track communities that need to be broken
 * @returns iterations performed (0 if converged already)
 */
template <bool REFINE=false, bool RANDOM=false, class G, class K, class W, class B, class FC, class FA, class FB>
inline int leidenMoveOmpW(vector<K>& vcom, vector<W>& ctot, vector<B>& vaff, vector<vector<K>*>& vcs, vector<vector<W>*>& vcout, vector<xorshift32_engine*>& rng, const G& x, const vector<K>& vcob, const vector<W>& vtot, double M, double R, int L, FC fc, FA fa, FB fb) {
  size_t S = x.span();
  int l = 0;
  W  el = W();
  for (; l<L;) {
    el = W();
    #pragma omp parallel for schedule(dynamic, 2048) reduction(+:el)
    for (K u=0; u<S; ++u) {
      int t = omp_get_thread_num();
      if (!x.hasVertex(u)) continue;
      if (!fa(u)) continue;
      if (!REFINE && !vaff[u]) continue;
      if ( REFINE && ctot[vcom[u]]>vtot[u]) continue;
      K d = vcom[u];
      leidenClearScanW(*vcs[t], *vcout[t]);
      leidenScanCommunitiesW<false, REFINE>(*vcs[t], *vcout[t], x, u, vcom, vcob);
      auto [c, e] = leidenChooseCommunity<false, RANDOM>(*rng[t], x, u, vcom, vtot, ctot, *vcs[t], *vcout[t], M, R);
      if (c && leidenChangeCommunityOmpW<REFINE>(vcom, ctot, x, u, c, vtot)) {
        if (!REFINE) { fb(d); fb(c); }
        if (!REFINE) x.forEachEdgeKey(u, [&](auto v) { vaff[v] = B(1); });
      }
      if (!REFINE) vaff[u] = B();
      el += e;  // l1-norm
    }
    if (REFINE || fc(el, l++)) break;
  }
  return l>1 || el? l : 0;
}


/**
 * Leiden algorithm's local moving phase.
 * @param vcom community each vertex belongs to (initial, updated)
 * @param ctot total edge weight of each community (precalculated, updated)
 * @param vaff is vertex affected flag (updated)
 * @param vcs communities vertex u is linked to (temporary buffer, updated)
 * @param vcout total edge weight from vertex u to community C (temporary buffer, updated)
 * @param rng random number generators
 * @param x original graph
 * @param vcob community bound each vertex belongs to
 * @param vtot total edge weight of each vertex
 * @param M total weight of "undirected" graph (1/2 of directed graph)
 * @param R resolution (0, 1]
 * @param L max iterations
 * @param fc has local moving phase converged?
 * @param fa is vertex allowed to be updated?
 * @returns iterations performed (0 if converged already)
 */
template <bool REFINE=false, bool RANDOM=false, class G, class K, class W, class B, class FC, class FA>
inline int leidenMoveOmpW(vector<K>& vcom, vector<W>& ctot, vector<B>& vaff, vector<vector<K>*>& vcs, vector<vector<W>*>& vcout, vector<xorshift32_engine*>& rng, const G& x, const vector<K>& vcob, const vector<W>& vtot, double M, double R, int L, FC fc, FA fa) {
  auto fb = [](auto u) {};
  return leidenMoveOmpW<REFINE, RANDOM>(vcom, ctot, vaff, vcs, vcout, rng, x, vcob, vtot, M, R, L, fc, fa, fb);
}


/**
 * Leiden algorithm's local moving phase.
 * @param vcom community each vertex belongs to (initial, updated)
 * @param ctot total edge weight of each community (precalculated, updated)
 * @param vaff is vertex affected flag (updated)
 * @param vcs communities vertex u is linked to (temporary buffer, updated)
 * @param vcout total edge weight from vertex u to community C (temporary buffer, updated)
 * @param rng random number generators
 * @param x original graph
 * @param vcob community bound each vertex belongs to
 * @param vtot total edge weight of each vertex
 * @param M total weight of "undirected" graph (1/2 of directed graph)
 * @param R resolution (0, 1]
 * @param L max iterations
 * @param fc has local moving phase converged?
 * @returns iterations performed (0 if converged already)
 */
template <bool REFINE=false, bool RANDOM=false, class G, class K, class W, class B, class FC>
inline int leidenMoveOmpW(vector<K>& vcom, vector<W>& ctot, vector<B>& vaff, vector<vector<K>*>& vcs, vector<vector<W>*>& vcout, vector<xorshift32_engine*>& rng, const G& x, const vector<K>& vcob, const vector<W>& vtot, double M, double R, int L, FC fc) {
  auto fa = [](auto u) { return true; };
  auto fb = [](auto u) {};
  return leidenMoveOmpW<REFINE, RANDOM>(vcom, ctot, vaff, vcs, vcout, rng, x, vcob, vtot, M, R, L, fc, fa, fb);
}
#endif
#pragma endregion




#pragma region PARENT COMMUNITY
/**
 * Find the parent/bounding community of each community.
 * @param a parent community of each community (updated)
 * @param x original graph
 * @param vcom community each vertex belongs to
 * @param vcob community bound each vertex belongs to
 */
template <class G, class K>
inline void leidenParentCommunityU(vector<K>& a, const G& x, const vector<K>& vcom, const vector<K>& vcob) {
  x.forEachVertexKey([&](auto u) {
    K c = vcom[u];
    K b = vcob[u];
    a[c] = b;
  });
}


/**
 * Find the parent/bounding community of each community.
 * @param a parent community of each community (updated)
 * @param x original graph
 * @param vcom community each vertex belongs to
 * @param vcob community bound each vertex belongs to
 */
template <class G, class K>
inline void leidenParentCommunityOmpU(vector<K>& a, const G& x, const vector<K>& vcom, const vector<K>& vcob) {
  size_t S = x.span();
  #pragma omp parallel for schedule(static, 2048)
  for (K u=0; u<S; ++u) {
    if (!x.hasVertex(u)) continue;
    K c = vcom[u];
    K b = vcob[u];
    a[c] = b;
  }
}
#pragma endregion




#pragma region COMMUNITY PROPERTIES
/**
 * Examine if each community exists.
 * @param a does each community exist (updated)
 * @param x original graph
 * @param vcom community each vertex belongs to
 * @returns number of communities
 */
template <class G, class K, class A>
inline size_t leidenCommunityExistsW(vector<A>& a, const G& x, const vector<K>& vcom) {
  size_t C = 0;
  fillValueU(a, A());
  x.forEachVertexKey([&](auto u) {
    K c = vcom[u];
    if (!a[c]) ++C;
    a[c] = A(1);
  });
  return C;
}


#ifdef OPENMP
/**
 * Examine if each community exists.
 * @param a does each community exist (updated)
 * @param x original graph
 * @param vcom community each vertex belongs to
 * @returns number of communities
 */
template <class G, class K, class A>
inline size_t leidenCommunityExistsOmpW(vector<A>& a, const G& x, const vector<K>& vcom) {
  size_t S = x.span();
  size_t C = 0;
  fillValueOmpU(a, A());
  #pragma omp parallel for schedule(static, 2048) reduction(+:C)
  for (K u=0; u<S; ++u) {
    if (!x.hasVertex(u)) continue;
    K c = vcom[u];
    A m = A();
    #pragma omp atomic capture
    { m = a[c]; a[c] = A(1); }
    if (!m) ++C;
  }
  return C;
}
#endif




/**
 * Find the total degree of each community.
 * @param a total degree of each community (updated)
 * @param x original graph
 * @param vcom community each vertex belongs to
 */
template <class G, class K, class A>
inline void leidenCommunityTotalDegreeW(vector<A>& a, const G& x, const vector<K>& vcom) {
  fillValueU(a, A());
  x.forEachVertexKey([&](auto u) {
    K c   = vcom[u];
    a[c] += x.degree(u);
  });
}


#ifdef OPENMP
/**
 * Find the total degree of each community.
 * @param a total degree of each community (updated)
 * @param x original graph
 * @param vcom community each vertex belongs to
 */
template <class G, class K, class A>
inline void leidenCommunityTotalDegreeOmpW(vector<A>& a, const G& x, const vector<K>& vcom) {
  size_t S = x.span();
  fillValueOmpU(a, A());
  #pragma omp parallel for schedule(static, 2048)
  for (K u=0; u<S; ++u) {
    if (!x.hasVertex(u)) continue;
    K c = vcom[u];
    #pragma omp atomic
    a[c] += x.degree(u);
  }
}
#endif




/**
 * Find the number of vertices in each community.
 * @param a number of vertices belonging to each community (updated)
 * @param x original graph
 * @param vcom community each vertex belongs to
 */
template <class G, class K, class A>
inline void leidenCountCommunityVerticesW(vector<A>& a, const G& x, const vector<K>& vcom) {
  fillValueU(a, A());
  x.forEachVertexKey([&](auto u) {
    K c = vcom[u];
    ++a[c];
  });
}


#ifdef OPENMP
/**
 * Find the number of vertices in each community.
 * @param a number of vertices belonging to each community (updated)
 * @param x original graph
 * @param vcom community each vertex belongs to
 */
template <class G, class K, class A>
inline void leidenCountCommunityVerticesOmpW(vector<A>& a, const G& x, const vector<K>& vcom) {
  size_t S = x.span();
  fillValueOmpU(a, A());
  #pragma omp parallel for schedule(static, 2048)
  for (K u=0; u<S; ++u) {
    if (!x.hasVertex(u)) continue;
    K c = vcom[u];
    #pragma omp atomic
    ++a[c];
  }
}
#endif




/**
 * Find the vertices in each community.
 * @param coff csr offsets for vertices belonging to each community (updated)
 * @param cdeg number of vertices in each community (updated)
 * @param cedg vertices belonging to each community (updated)
 * @param x original graph
 * @param vcom community each vertex belongs to
 */
template <class G, class K>
inline void leidenCommunityVerticesW(vector<K>& coff, vector<K>& cdeg, vector<K>& cedg, const G& x, const vector<K>& vcom) {
  size_t C = coff.size() - 1;
  leidenCountCommunityVerticesW(coff, x, vcom);
  coff[C] = exclusiveScanW(coff.data(), coff.data(), C);
  fillValueU(cdeg, K());
  x.forEachVertexKey([&](auto u) {
    K c = vcom[u];
    csrAddEdgeU(cdeg, cedg, coff, c, u);
  });
}


#ifdef OPENMP
/**
 * Find the vertices in each community.
 * @param coff csr offsets for vertices belonging to each community (updated)
 * @param cdeg number of vertices in each community (updated)
 * @param cedg vertices belonging to each community (updated)
 * @param bufk buffer for exclusive scan of size |threads| (scratch)
 * @param x original graph
 * @param vcom community each vertex belongs to
 */
template <class G, class K>
inline void leidenCommunityVerticesOmpW(vector<K>& coff, vector<K>& cdeg, vector<K>& cedg, vector<K>& bufk, const G& x, const vector<K>& vcom) {
  size_t S = x.span();
  size_t C = coff.size() - 1;
  leidenCountCommunityVerticesOmpW(coff, x, vcom);
  coff[C] = exclusiveScanOmpW(coff.data(), bufk.data(), coff.data(), C);
  fillValueOmpU(cdeg, K());
  #pragma omp parallel for schedule(static, 2048)
  for (K u=0; u<S; ++u) {
    if (!x.hasVertex(u)) continue;
    K c = vcom[u];
    csrAddEdgeOmpU(cdeg, cedg, coff, c, u);
  }
}
#endif
#pragma endregion




#pragma region LOOKUP COMMUNITIES
/**
 * Update community membership in a tree-like fashion (to handle aggregation).
 * @param a output community each vertex belongs to (updated)
 * @param vcom community each vertex belongs to (at this aggregation level)
 */
template <class K>
inline void leidenLookupCommunitiesU(vector<K>& a, const vector<K>& vcom) {
  for (auto& v : a)
    v = vcom[v];
}


#ifdef OPENMP
/**
 * Update community membership in a tree-like fashion (to handle aggregation).
 * @param a output community each vertex belongs to (updated)
 * @param vcom community each vertex belongs to (at this aggregation level)
 */
template <class K>
inline void leidenLookupCommunitiesOmpU(vector<K>& a, const vector<K>& vcom) {
  size_t S = a.size();
  #pragma omp parallel for schedule(static, 2048)
  for (size_t u=0; u<S; ++u)
    a[u] = vcom[a[u]];
}
#endif
#pragma endregion




#pragma region AGGREGATION PHASE
/**
 * Aggregate outgoing edges of each community.
 * @param ydeg degree of each community (updated)
 * @param yedg vertex ids of outgoing edges of each community (updated)
 * @param ywei weights of outgoing edges of each community (updated)
 * @param vcs communities vertex u is linked to (temporary buffer, updated)
 * @param vcout total edge weight from vertex u to community C (temporary buffer, updated)
 * @param x original graph
 * @param vcom community each vertex belongs to
 * @param coff offsets for vertices belonging to each community
 * @param cedg vertices belonging to each community
 * @param yoff offsets for vertices belonging to each community
 */
template <class G, class K, class W>
inline void leidenAggregateEdgesW(vector<K>& ydeg, vector<K>& yedg, vector<W>& ywei, vector<K>& vcs, vector<W>& vcout, const G& x, const vector<K>& vcom, const vector<K>& coff, const vector<K>& cedg, const vector<size_t>& yoff) {
  size_t C = coff.size() - 1;
  fillValueU(ydeg, K());
  for (K c=0; c<C; ++c) {
    K n = csrDegree(coff, c);
    if (n==0) continue;
    leidenClearScanW(vcs, vcout);
    csrForEachEdgeKey(coff, cedg, c, [&](auto u) {
      leidenScanCommunitiesW<true>(vcs, vcout, x, u, vcom);
    });
    for (auto d : vcs)
      csrAddEdgeU(ydeg, yedg, ywei, yoff, c, d, vcout[d]);
  }
}


#ifdef OPENMP
/**
 * Aggregate outgoing edges of each community.
 * @param ydeg degree of each community (updated)
 * @param yedg vertex ids of outgoing edges of each community (updated)
 * @param ywei weights of outgoing edges of each community (updated)
 * @param vcs communities vertex u is linked to (temporary buffer, updated)
 * @param vcout total edge weight from vertex u to community C (temporary buffer, updated)
 * @param x original graph
 * @param vcom community each vertex belongs to
 * @param coff offsets for vertices belonging to each community
 * @param cedg vertices belonging to each community
 * @param yoff offsets for vertices belonging to each community
 */
template <int CHUNK_SIZE=2048, class G, class K, class W>
inline void leidenAggregateEdgesOmpW(vector<K>& ydeg, vector<K>& yedg, vector<W>& ywei, vector<vector<K>*>& vcs, vector<vector<W>*>& vcout, const G& x, const vector<K>& vcom, const vector<K>& coff, const vector<K>& cedg, const vector<size_t>& yoff) {
  size_t C = coff.size() - 1;
  fillValueOmpU(ydeg, K());
  #pragma omp parallel for schedule(dynamic, CHUNK_SIZE)
  for (K c=0; c<C; ++c) {
    int t = omp_get_thread_num();
    K   n = csrDegree(coff, c);
    if (n==0) continue;
    leidenClearScanW(*vcs[t], *vcout[t]);
    csrForEachEdgeKey(coff, cedg, c, [&](auto u) {
      leidenScanCommunitiesW<true>(*vcs[t], *vcout[t], x, u, vcom);
    });
    for (auto d : *vcs[t])
      csrAddEdgeU(ydeg, yedg, ywei, yoff, c, d, (*vcout[t])[d]);
  }
}
#endif


/**
 * Re-number communities such that they are numbered 0, 1, 2, ...
 * @param vcom community each vertex belongs to (updated)
 * @param cext does each community exist (updated)
 * @param x original graph
 * @returns number of communities
 */
template <class G, class K>
inline size_t leidenRenumberCommunitiesW(vector<K>& vcom, vector<K>& cext, const G& x) {
  size_t C = exclusiveScanW(cext, cext);
  leidenLookupCommunitiesU(vcom, cext);
  return C;
}


#ifdef OPENMP
/**
 * Re-number communities such that they are numbered 0, 1, 2, ...
 * @param vcom community each vertex belongs to (updated)
 * @param cext does each community exist (updated)
 * @param bufk buffer for exclusive scan of size |threads| (scratch)
 * @param x original graph
 * @returns number of communities
 */
template <class G, class K>
inline size_t leidenRenumberCommunitiesOmpW(vector<K>& vcom, vector<K>& cext, vector<K>& bufk, const G& x) {
  size_t C = exclusiveScanOmpW(cext, bufk, cext);
  leidenLookupCommunitiesOmpU(vcom, cext);
  return C;
}
#endif


/**
 * Leiden algorithm's community aggregation phase.
 * @param yoff offsets for vertices belonging to each community (updated)
 * @param ydeg degree of each community (updated)
 * @param yedg vertex ids of outgoing edges of each community (updated)
 * @param ywei weights of outgoing edges of each community (updated)
 * @param vcs communities vertex u is linked to (temporary buffer, updated)
 * @param vcout total edge weight from vertex u to community C (temporary buffer, updated)
 * @param x original graph
 * @param vcom community each vertex belongs to
 * @param coff offsets for vertices belonging to each community
 * @param cedg vertices belonging to each community
 */
template <class G, class K, class W>
inline void leidenAggregateW(vector<size_t>& yoff, vector<K>& ydeg, vector<K>& yedg, vector<W>& ywei, vector<K>& vcs, vector<W>& vcout, const G& x, const vector<K>& vcom, vector<K>& coff, vector<K>& cedg) {
  size_t C = coff.size() - 1;
  leidenCommunityTotalDegreeW(yoff, x, vcom);
  yoff[C] = exclusiveScanW(yoff.data(), yoff.data(), C);
  leidenAggregateEdgesW(ydeg, yedg, ywei, vcs, vcout, x, vcom, coff, cedg, yoff);
}


#ifdef OPENMP
/**
 * Leiden algorithm's community aggregation phase.
 * @param yoff offsets for vertices belonging to each community (updated)
 * @param ydeg degree of each community (updated)
 * @param yedg vertex ids of outgoing edges of each community (updated)
 * @param ywei weights of outgoing edges of each community (updated)
 * @param bufs buffer for exclusive scan of size |threads| (scratch)
 * @param vcs communities vertex u is linked to (temporary buffer, updated)
 * @param vcout total edge weight from vertex u to community C (temporary buffer, updated)
 * @param x original graph
 * @param vcom community each vertex belongs to
 * @param coff offsets for vertices belonging to each community
 * @param cedg vertices belonging to each community
 */
template <int CHUNK_SIZE=2048, class G, class K, class W>
inline void leidenAggregateOmpW(vector<size_t>& yoff, vector<K>& ydeg, vector<K>& yedg, vector<W>& ywei, vector<size_t>& bufs, vector<vector<K>*>& vcs, vector<vector<W>*>& vcout, const G& x, const vector<K>& vcom, vector<K>& coff, vector<K>& cedg) {
  size_t C = coff.size() - 1;
  leidenCommunityTotalDegreeOmpW(yoff, x, vcom);
  yoff[C] = exclusiveScanOmpW(yoff.data(), bufs.data(), yoff.data(), C);
  leidenAggregateEdgesOmpW<CHUNK_SIZE>(ydeg, yedg, ywei, vcs, vcout, x, vcom, coff, cedg, yoff);
}
#endif
#pragma endregion




#pragma region ENVIRONMENT SETUP
/**
 * Setup and perform the Leiden algorithm.
 * @param rnd random number generator
 * @param x original graph
 * @param o leiden options
 * @param fi initializing community membership and total vertex/community weights (vcom, vtot, ctot)
 * @param fm marking affected vertices (vaff, vcs, vcout, vcom, vtot, ctot)
 * @param fa is vertex allowed to be updated? (u)
 * @param ft track communities that need to be broken (cchg, c)
 * @param fs is vertex in the subset to be refined? (cchg, vcob, u)
 * @returns leiden result
 */
template <bool DYNAMIC=false, bool RANDOM=false, bool USEPARENT=false, class FLAG=char, class RND, class G, class FI, class FM, class FA, class FT, class FS>
inline auto leidenInvoke(RND& rnd, const G& x, const LeidenOptions& o, FI fi, FM fm, FA fa, FT ft, FS fs) {
  using  K = typename G::key_type;
  using  W = LEIDEN_WEIGHT_TYPE;
  using  B = FLAG;
  // Options.
  double R = o.resolution;
  int    L = o.maxIterations, l = 0;
  int    P = o.maxPasses, p = 0;
  // Get graph properties.
  size_t X = x.size();
  size_t S = x.span();
  double M = edgeWeight(x)/2;
  // Allocate buffers.
  vector<B> vaff(S);        // Affected vertex flag (any pass)
  vector<B> cchg(S);        // Community changed flag (first pass)
  vector<K> ucom, vcom(S);  // Community membership (first pass, current pass)
  vector<K> vcob(S);        // Community bound (any pass)
  vector<K> cpar;           // Parent community (any pass)
  vector<W> utot, vtot(S);  // Total vertex weights (first pass, current pass)
  vector<W> ctot;           // Total community weights (any pass)
  vector<K> vcs;       // Hashtable keys
  vector<W> vcout(S);  // Hashtable values
  vector<xorshift32_engine*> rng(1);
  if (!DYNAMIC) ucom.resize(S);
  if (!DYNAMIC) utot.resize(S);
  if (!DYNAMIC) ctot.resize(S);
  if (USEPARENT) cpar.resize(S);
  leidenAllocateRngsW(rng, rnd);
  size_t Z = max(size_t(o.aggregationTolerance * X), X);
  size_t Y = max(size_t(o.aggregationTolerance * Z), Z);
  DiGraphCsr<K, None, None, K> cv(S, S);  // CSR for community vertices
  DiGraphCsr<K, None, W> y(S, Y);         // CSR for aggregated graph (input);  y(S, X)
  DiGraphCsr<K, None, W> z(S, Z);         // CSR for aggregated graph (output); z(S, X)
  // Perform Leiden algorithm.
  float tm = 0, ti = 0, tp = 0, tl = 0, tr = 0, ta = 0;  // Time spent in different phases
  float t  = measureDurationMarked([&](auto mark) {
    double E  = o.tolerance;
    auto   fc = [&](double el, int l) { return el<=E; };
    // Reset buffers, in case of multiple runs.
    fillValueU(vaff, B());
    fillValueU(cchg, B());
    fillValueU(ucom, K());
    fillValueU(vcom, K());
    fillValueU(vcob, K());
    fillValueU(cpar, K());
    fillValueU(utot, W());
    fillValueU(vtot, W());
    fillValueU(ctot, W());
    cv.respan(S);
    y .respan(S);
    z .respan(S);
    // Time the algorithm.
    mark([&]() {
      // Initialize community membership and total vertex/community weights.
      ti += measureDuration([&]() { fi(ucom, utot, ctot); });
      // Mark affected vertices.
      tm += measureDuration([&]() { fm(vaff, vcs, vcout, ucom, utot, ctot); });
      // Start timing first pass.
      auto t0 = timeNow(), t1 = t0;
      // Start local-moving, refinement, aggregation phases.
      // NOTE: In first pass, the input graph is a DiGraph.
      // NOTE: For subsequent passes, the input graph is a DiGraphCsr (optimization).
      for (l=0, p=0; M>0 && P>0;) {
        if (p==1) t1 = timeNow();
        bool isFirst = p==0;
        int m = 0;
        tl += measureDuration([&]() {
          auto fb = [&](auto c) { return ft(cchg, c); };
          if (isFirst) m += leidenMoveW<false, RANDOM>(ucom, ctot, vaff, vcs, vcout, rng, x, vcob, utot, M, R, L, fc, fa, fb);
          else         m += leidenMoveW<false, RANDOM>(vcom, ctot, vaff, vcs, vcout, rng, y, vcob, vtot, M, R, L, fc);
        });
        tr += measureDuration([&]() {
          auto fr = [&](auto u) { return fs(cchg, vcob, u); };
          if (isFirst) copyValuesW(vcob.data(), ucom.data(), x.span());
          else         copyValuesW(vcob.data(), vcom.data(), y.span());
          if (isFirst) leidenInitializeW(ucom, ctot, x, utot, fr);
          else         leidenInitializeW(vcom, ctot, y, vtot);
          // if (isFirst) fillValueU(vaff.data(), x.order(), B(1));
          // else         fillValueU(vaff.data(), y.order(), B(1));
          if (isFirst) m += leidenMoveW<true, RANDOM>(ucom, ctot, vaff, vcs, vcout, rng, x, vcob, utot, M, R, L, fc, fr);
          else         m += leidenMoveW<true, RANDOM>(vcom, ctot, vaff, vcs, vcout, rng, y, vcob, vtot, M, R, L, fc);
        });
        l += max(m, 1); ++p;
        if (m<=1 || p>=P) break;
        size_t GN = isFirst? x.order() : y.order();
        size_t GS = isFirst? x.span()  : y.span();
        size_t CN = 0;
        if (isFirst) CN = leidenCommunityExistsW(cv.degrees, x, ucom);
        else         CN = leidenCommunityExistsW(cv.degrees, y, vcom);
        if (double(CN)/GN >= o.aggregationTolerance) break;
        if (isFirst) leidenRenumberCommunitiesW(ucom, cv.degrees, x);
        else         leidenRenumberCommunitiesW(vcom, cv.degrees, y);
        if (USEPARENT) {
          if (isFirst) leidenParentCommunityU(cpar, x, ucom, vcob);
          else         leidenParentCommunityU(cpar, y, vcom, vcob);
        }
        if (isFirst) {}
        else         leidenLookupCommunitiesU(ucom, vcom);
        ta += measureDuration([&]() {
          cv.respan(CN); z.respan(CN);
          if (isFirst) leidenCommunityVerticesW(cv.offsets, cv.degrees, cv.edgeKeys, x, ucom);
          else         leidenCommunityVerticesW(cv.offsets, cv.degrees, cv.edgeKeys, y, vcom);
          if (isFirst) leidenAggregateW(z.offsets, z.degrees, z.edgeKeys, z.edgeValues, vcs, vcout, x, ucom, cv.offsets, cv.edgeKeys);
          else         leidenAggregateW(z.offsets, z.degrees, z.edgeKeys, z.edgeValues, vcs, vcout, y, vcom, cv.offsets, cv.edgeKeys);
        });
        swap(y, z);
        // fillValueU(vcob.data(), CN, K());
        // fillValueU(vcom.data(), CN, K());
        // fillValueU(ctot.data(), CN, W());
        fillValueU(vtot.data(), CN, W());
        fillValueU(vaff.data(), CN, B(1));
        leidenVertexWeightsW(vtot, y);
        if (USEPARENT) leidenInitializeFromW(vcom, ctot, y, vtot, cpar);
        else           leidenInitializeW(vcom, ctot, y, vtot);
        E /= o.toleranceDrop;
      }
      if (p<=1) {}
      else      leidenLookupCommunitiesU(ucom, vcom);
      if (p<=1) t1 = timeNow();
      tp += duration(t0, t1);
    });
  }, o.repeat);
  leidenFreeRngsW(rng);
  return LeidenResult<K>(ucom, utot, ctot, l, p, t, tm/o.repeat, ti/o.repeat, tp/o.repeat, tl/o.repeat, tr/o.repeat, ta/o.repeat, countValue(vaff, B(1)));
}


#ifdef OPENMP
/**
 * Setup and perform the Leiden algorithm.
 * @param rnd random number generator
 * @param x original graph
 * @param o leiden options
 * @param fi initializing community membership and total vertex/community weights (vcom, vtot, ctot)
 * @param fm marking affected vertices (vaff, vcs, vcout, vcom, vtot, ctot)
 * @param fa is vertex allowed to be updated? (u)
 * @param ft track communities that need to be broken (cchg, c)
 * @param fs is vertex in the subset to be refined? (cchg, vcob, u)
 * @returns leiden result
 */
template <bool DYNAMIC=false, bool RANDOM=false, bool USEPARENT=false, class FLAG=char, int CHUNK_SIZE=2048, class RND, class G, class FI, class FM, class FA, class FT, class FS>
inline auto leidenInvokeOmp(RND& rnd, const G& x, const LeidenOptions& o, FI fi, FM fm, FA fa, FT ft, FS fs) {
  using  K = typename G::key_type;
  using  W = LEIDEN_WEIGHT_TYPE;
  using  B = FLAG;
  // Options.
  double R = o.resolution;
  int    L = o.maxIterations, l = 0;
  int    P = o.maxPasses, p = 0;
  // Get graph properties.
  size_t X = x.size();
  size_t S = x.span();
  double M = edgeWeightOmp(x)/2;
  // Allocate buffers.
  int    T = omp_get_max_threads();
  vector<B> vaff(S);        // Affected vertex flag (any pass)
  vector<B> cchg(S);        // Community changed flag (first pass)
  vector<K> ucom, vcom(S);  // Community membership (first pass, current pass)
  vector<K> vcob(S);        // Community bound (any pass)
  vector<K> cpar;           // Parent community (any pass)
  vector<W> utot, vtot(S);  // Total vertex weights (first pass, current pass)
  vector<W> ctot;           // Total community weights (any pass)
  vector<K> bufk(T);        // Buffer for exclusive scan
  vector<size_t> bufs(T);   // Buffer for exclusive scan
  vector<vector<K>*> vcs(T);    // Hashtable keys
  vector<vector<W>*> vcout(T);  // Hashtable values
  vector<xorshift32_engine*> rng(T);
  if (!DYNAMIC) ucom.resize(S);
  if (!DYNAMIC) utot.resize(S);
  if (!DYNAMIC) ctot.resize(S);
  if (USEPARENT) cpar.resize(S);
  leidenAllocateHashtablesW(vcs, vcout, S);
  leidenAllocateRngsW(rng, rnd);
  size_t Z = max(size_t(o.aggregationTolerance * X), X);
  size_t Y = max(size_t(o.aggregationTolerance * Z), Z);
  DiGraphCsr<K, None, None, K> cv(S, S);  // CSR for community vertices
  DiGraphCsr<K, None, W> y(S, Y);         // CSR for aggregated graph (input);  y(S, X)
  DiGraphCsr<K, None, W> z(S, Z);         // CSR for aggregated graph (output); z(S, X)
  // Perform Leiden algorithm.
  float tm = 0, ti = 0, tp = 0, tl = 0, tr = 0, ta = 0;  // Time spent in different phases
  float t  = measureDurationMarked([&](auto mark) {
    double E  = o.tolerance;
    auto   fc = [&](double el, int l) { return el<=E; };
    // Reset buffers, in case of multiple runs.
    fillValueOmpU(vaff, B());
    fillValueOmpU(cchg, B());
    fillValueOmpU(ucom, K());
    fillValueOmpU(vcom, K());
    fillValueOmpU(vcob, K());
    fillValueOmpU(cpar, K());
    fillValueOmpU(utot, W());
    fillValueOmpU(vtot, W());
    fillValueOmpU(ctot, W());
    cv.respan(S);
    y .respan(S);
    z .respan(S);
    // Time the algorithm.
    mark([&]() {
      // Initialize community membership and total vertex/community weights.
      ti += measureDuration([&]() { fi(ucom, utot, ctot); });
      // Mark affected vertices.
      tm += measureDuration([&]() { fm(vaff, vcs, vcout, ucom, utot, ctot); });
      // Start timing first pass.
      auto t0 = timeNow(), t1 = t0;
      // Start local-moving, refinement, aggregation phases.
      // NOTE: In first pass, the input graph is a DiGraph.
      // NOTE: For subsequent passes, the input graph is a DiGraphCsr (optimization).
      for (l=0, p=0; M>0 && P>0;) {
        if (p==1) t1 = timeNow();
        bool isFirst = p==0;
        int m = 0;
        tl += measureDuration([&]() {
          auto fb = [&](auto c) { return ft(cchg, c); };
          if (isFirst) m += leidenMoveOmpW<false, RANDOM>(ucom, ctot, vaff, vcs, vcout, rng, x, vcob, utot, M, R, L, fc, fa, fb);
          else         m += leidenMoveOmpW<false, RANDOM>(vcom, ctot, vaff, vcs, vcout, rng, y, vcob, vtot, M, R, L, fc);
        });
        tr += measureDuration([&]() {
          auto fr = [&](auto u) { return fs(cchg, vcob, u); };
          if (isFirst) copyValuesOmpW(vcob.data(), ucom.data(), x.span());
          else         copyValuesOmpW(vcob.data(), vcom.data(), y.span());
          if (isFirst) leidenInitializeOmpW(ucom, ctot, x, utot, fr);
          else         leidenInitializeOmpW(vcom, ctot, y, vtot);
          // if (isFirst) fillValueOmpU(vaff.data(), x.order(), B(1));
          // else         fillValueOmpU(vaff.data(), y.order(), B(1));
          if (isFirst) m += leidenMoveOmpW<true, RANDOM>(ucom, ctot, vaff, vcs, vcout, rng, x, vcob, utot, M, R, L, fc, fr);
          else         m += leidenMoveOmpW<true, RANDOM>(vcom, ctot, vaff, vcs, vcout, rng, y, vcob, vtot, M, R, L, fc);
        });
        l += max(m, 1); ++p;
        if (!isFirst && (m<=1 || p>=P)) break;
        size_t GN = isFirst? x.order() : y.order();
        size_t GS = isFirst? x.span()  : y.span();
        size_t CN = 0;
        if (isFirst) CN = leidenCommunityExistsOmpW(cv.degrees, x, ucom);
        else         CN = leidenCommunityExistsOmpW(cv.degrees, y, vcom);
        if (double(CN)/GN >= o.aggregationTolerance) break;
        if (isFirst) leidenRenumberCommunitiesOmpW(ucom, cv.degrees, bufk, x);
        else         leidenRenumberCommunitiesOmpW(vcom, cv.degrees, bufk, y);
        if (USEPARENT) {
          if (isFirst) leidenParentCommunityOmpU(cpar, x, ucom, vcob);
          else         leidenParentCommunityOmpU(cpar, y, vcom, vcob);
        }
        if (isFirst) {}
        else         leidenLookupCommunitiesOmpU(ucom, vcom);
        ta += measureDuration([&]() {
          cv.respan(CN); z.respan(CN);
          if (isFirst) leidenCommunityVerticesOmpW(cv.offsets, cv.degrees, cv.edgeKeys, bufk, x, ucom);
          else         leidenCommunityVerticesOmpW(cv.offsets, cv.degrees, cv.edgeKeys, bufk, y, vcom);
          if (isFirst) leidenAggregateOmpW<CHUNK_SIZE>(z.offsets, z.degrees, z.edgeKeys, z.edgeValues, bufs, vcs, vcout, x, ucom, cv.offsets, cv.edgeKeys);
          else         leidenAggregateOmpW<CHUNK_SIZE>(z.offsets, z.degrees, z.edgeKeys, z.edgeValues, bufs, vcs, vcout, y, vcom, cv.offsets, cv.edgeKeys);
        });
        swap(y, z);
        // fillValueOmpU(vcob.data(), CN, K());
        // fillValueOmpU(vcom.data(), CN, K());
        // fillValueOmpU(ctot.data(), CN, W());
        fillValueOmpU(vtot.data(), CN, W());
        fillValueOmpU(vaff.data(), CN, B(1));
        leidenVertexWeightsOmpW(vtot, y);
        if (USEPARENT) leidenInitializeFromOmpW(vcom, ctot, y, vtot, cpar);
        else           leidenInitializeOmpW(vcom, ctot, y, vtot);
        E /= o.toleranceDrop;
      }
      if (p<=1) {}
      else      leidenLookupCommunitiesOmpU(ucom, vcom);
      if (p<=1) t1 = timeNow();
      tp += duration(t0, t1);
    });
  }, o.repeat);
  leidenFreeHashtablesW(vcs, vcout);
  leidenFreeRngsW(rng);
  return LeidenResult<K>(ucom, utot, ctot, l, p, t, tm/o.repeat, ti/o.repeat, tp/o.repeat, tl/o.repeat, tr/o.repeat, ta/o.repeat, countValueOmp(vaff, B(1)));
}
#endif
#pragma endregion




#pragma region REPEAT SETUP (DYNAMIC)
/**
 * Setup the Dynamic Leiden algorithm for multiple runs.
 * @param qs initial community membership for each run (updated)
 * @param qvtots initial total vertex weights for each run (updated)
 * @param qctots initial total community weights for each run (updated)
 * @param q initial community membership
 * @param qvtot initial total vertex weights
 * @param qctot initial total community weights
 * @param repeat number of runs
 */
template <class K, class W>
inline void leidenSetupInitialsW(vector2d<K>& qs, vector2d<W>& qvtots, vector2d<W>& qctots, const vector<K>& q, const vector<W>& qvtot, const vector<W>& qctot, int repeat) {
  qs    .resize(repeat);
  qvtots.resize(repeat);
  qctots.resize(repeat);
  for (int r=0; r<repeat; ++r) {
    qs[r]     = q;
    qvtots[r] = qvtot;
    qctots[r] = qctot;
  }
}
#pragma endregion




#pragma region STATIC APPROACH
/**
 * Obtain the community membership of each vertex with Static Leiden.
 * @param rnd random number generator
 * @param x original graph
 * @param o leiden options
 * @returns leiden result
 */
template <bool RANDOM=false, bool USEPARENT=false, class FLAG=char, class RND, class G>
inline auto leidenStatic(RND& rnd, const G& x, const LeidenOptions& o={}) {
  auto fi = [&](auto& vcom, auto& vtot, auto& ctot)  {
    leidenVertexWeightsW(vtot, x);
    leidenInitializeW(vcom, ctot, x, vtot);
  };
  auto fm = [ ](auto& vaff, const auto& vcom, const auto& vtot, const auto& ctot, auto& vcs,  auto& vcout) {
    fillValueU(vaff, FLAG(1));
  };
  auto fa = [ ](auto u) { return true; };
  auto ft = [ ](auto& cchg, auto c) {};
  auto fs = [ ](const auto& cchg, const auto& vcob, auto u) { return true; };
  return leidenInvoke<false, RANDOM, USEPARENT, FLAG>(rnd, x, o, fi, fm, fa, ft, fs);
}


#ifdef OPENMP
/**
 * Obtain the community membership of each vertex with Static Leiden.
 * @param rnd random number generator
 * @param x original graph
 * @param o leiden options
 * @returns leiden result
 */
template <bool RANDOM=false, bool USEPARENT=false, class FLAG=char, class RND, class G>
inline auto leidenStaticOmp(RND& rnd, const G& x, const LeidenOptions& o={}) {
  auto fi = [&](auto& vcom, auto& vtot, auto& ctot)  {
    leidenVertexWeightsOmpW(vtot, x);
    leidenInitializeOmpW(vcom, ctot, x, vtot);
  };
  auto fm = [ ](auto& vaff, const auto& vcom, const auto& vtot, const auto& ctot, auto& vcs,  auto& vcout) {
    fillValueOmpU(vaff, FLAG(1));
  };
  auto fa = [ ](auto u) { return true; };
  auto ft = [ ](auto& cchg, auto c) {};
  auto fs = [ ](const auto& cchg, const auto& vcob, auto u) { return true; };
  return leidenInvokeOmp<false, RANDOM, USEPARENT, FLAG>(rnd, x, o, fi, fm, fa, ft, fs);
}
#endif
#pragma endregion




#pragma region NAIVE-DYNAMIC APPROACH
/**
 * Obtain the community membership of each vertex with Naive-dynamic Leiden.
 * @param rnd random number generator
 * @param y updated graph
 * @param deletions edge deletions for this batch update (undirected)
 * @param insertions edge insertions for this batch update (undirected)
 * @param q initial community each vertex belongs to
 * @param qvtot initial total edge weight of each vertex
 * @param qctot initial total edge weight of each community
 * @param o leiden options
 * @returns leiden result
 */
template <bool RANDOM=false, bool USEPARENT=false, class FLAG=char, class RND, class G, class K, class V, class W>
inline auto leidenNaiveDynamic(RND& rnd, const G& y, const vector<tuple<K, K, V>>& deletions, const vector<tuple<K, K, V>>& insertions, const vector<K>& q, const vector<W>& qvtot, const vector<W>& qctot, const LeidenOptions& o={}) {
  vector2d<K> qs;
  vector2d<W> qvtots, qctots;
  leidenSetupInitialsW(qs, qvtots, qctots, q, qvtot, qctot, o.repeat);
  int  r  = 0;
  auto fi = [&](auto& vcom, auto& vtot, auto& ctot)  {
    vcom = move(qs[r]);
    vtot = move(qvtots[r]);
    ctot = move(qctots[r]); ++r;
    leidenUpdateWeightsFromU(vtot, ctot, y, deletions, insertions, vcom);
  };
  auto fm = [ ](auto& vaff, const auto& vcom, const auto& vtot, const auto& ctot, auto& vcs,  auto& vcout) {
    fillValueU(vaff, FLAG(1));
  };
  auto fa = [ ](auto u) { return true; };
  auto ft = [ ](auto& cchg, auto c) { cchg[c] = FLAG(1); };
  auto fs = [ ](const auto& cchg, const auto& vcob, auto u) { return cchg[vcob[u]]; };
  return leidenInvoke<true, RANDOM, USEPARENT, FLAG>(rnd, y, o, fi, fm, fa, ft, fs);
}


#ifdef OPENMP
/**
 * Obtain the community membership of each vertex with Naive-dynamic Leiden.
 * @param rnd random number generator
 * @param y updated graph
 * @param deletions edge deletions for this batch update (undirected)
 * @param insertions edge insertions for this batch update (undirected)
 * @param q initial community each vertex belongs to
 * @param qvtot initial total edge weight of each vertex
 * @param qctot initial total edge weight of each community
 * @param o leiden options
 * @returns leiden result
 */
template <bool RANDOM=false, bool USEPARENT=false, class FLAG=char, class RND, class G, class K, class V, class W>
inline auto leidenNaiveDynamicOmp(RND& rnd, const G& y, const vector<tuple<K, K, V>>& deletions, const vector<tuple<K, K, V>>& insertions, const vector<K>& q, const vector<W>& qvtot, const vector<W>& qctot, const LeidenOptions& o={}) {
  vector2d<K> qs;
  vector2d<W> qvtots, qctots;
  leidenSetupInitialsW(qs, qvtots, qctots, q, qvtot, qctot, o.repeat);
  int  r  = 0;
  auto fi = [&](auto& vcom, auto& vtot, auto& ctot)  {
    vcom = move(qs[r]);
    vtot = move(qvtots[r]);
    ctot = move(qctots[r]); ++r;
    leidenUpdateWeightsFromOmpU(vtot, ctot, y, deletions, insertions, vcom);
  };
  auto fm = [ ](auto& vaff, const auto& vcom, const auto& vtot, const auto& ctot, auto& vcs,  auto& vcout) {
    fillValueOmpU(vaff, FLAG(1));
  };
  auto fa = [ ](auto u) { return true; };
  auto ft = [ ](auto& cchg, auto c) { cchg[c] = FLAG(1); };
  auto fs = [ ](const auto& cchg, const auto& vcob, auto u) { return cchg[vcob[u]]; };
  return leidenInvokeOmp<true, RANDOM, USEPARENT, FLAG>(rnd, y, o, fi, fm, fa, ft, fs);
}
#endif
#pragma endregion




#pragma region DYNAMIC DELTA-SCREENING
/**
 * Find the vertices which should be processed upon a batch of edge insertions and deletions.
 * @param vertices vertex affected flags (output)
 * @param neighbors neighbor affected flags (output)
 * @param communities community affected flags (output)
 * @param vcs communities vertex u is linked to (temporary buffer, updated)
 * @param vcout total edge weight from vertex u to community C (temporary buffer, updated)
 * @param y updated graph
 * @param deletions edge deletions for this batch update (undirected, sorted by source vertex id)
 * @param insertions edge insertions for this batch update (undirected, sorted by source vertex id)
 * @param vcom community each vertex belongs to
 * @param vtot total edge weight of each vertex
 * @param ctot total edge weight of each community
 * @param M total weight of "undirected" graph (1/2 of directed graph)
 * @param R resolution (0, 1]
 */
template <class B, class G, class K, class V, class W>
inline auto leidenAffectedVerticesDeltaScreeningW(vector<B>& vertices, vector<B>& neighbors, vector<B>& communities, vector<K>& vcs, vector<W>& vcout, const G& y, const vector<tuple<K, K, V>>& deletions, const vector<tuple<K, K, V>>& insertions, const vector<K>& vcom, const vector<W>& vtot, const vector<W>& ctot, double M, double R=1) {
  fillValueU(vertices,    B());
  fillValueU(neighbors,   B());
  fillValueU(communities, B());
  xorshift32_engine rng(0);
  for (const auto& [u, v] : deletions) {
    if (vcom[u] != vcom[v]) continue;
    vertices[u]  = 1;
    neighbors[u] = 1;
    communities[vcom[v]] = 1;
  }
  for (size_t i=0; i<insertions.size();) {
    K u = get<0>(insertions[i]);
    leidenClearScanW(vcs, vcout);
    for (; i<insertions.size() && get<0>(insertions[i])==u; ++i) {
      K v = get<1>(insertions[i]);
      V w = get<2>(insertions[i]);
      if (vcom[u] == vcom[v]) continue;
      leidenScanCommunityW(vcs, vcout, u, v, w, vcom);
    }
    auto [c, e] = leidenChooseCommunity(rng, y, u, vcom, vtot, ctot, vcs, vcout, M, R);
    if (e<=0) continue;
    vertices[u]  = 1;
    neighbors[u] = 1;
    communities[c] = 1;
  }
  y.forEachVertexKey([&](auto u) {
    if (neighbors[u]) y.forEachEdgeKey(u, [&](auto v) { vertices[v] = 1; });
    if (communities[vcom[u]]) vertices[u] = 1;
  });
}


#ifdef OPENMP
/**
 * Find the vertices which should be processed upon a batch of edge insertions and deletions.
 * @param vertices vertex affected flags (output)
 * @param neighbors neighbor affected flags (output)
 * @param communities community affected flags (output)
 * @param vcs communities vertex u is linked to (temporary buffer, updated)
 * @param vcout total edge weight from vertex u to community C (temporary buffer, updated)
 * @param y updated graph
 * @param deletions edge deletions for this batch update (undirected, sorted by source vertex id)
 * @param insertions edge insertions for this batch update (undirected, sorted by source vertex id)
 * @param vcom community each vertex belongs to
 * @param vtot total edge weight of each vertex
 * @param ctot total edge weight of each community
 * @param M total weight of "undirected" graph (1/2 of directed graph)
 * @param R resolution (0, 1]
 */
template <class B, class G, class K, class V, class W>
inline auto leidenAffectedVerticesDeltaScreeningOmpW(vector<B>& vertices, vector<B>& neighbors, vector<B>& communities, vector<vector<K>*>& vcs, vector<vector<W>*>& vcout, const G& y, const vector<tuple<K, K, V>>& deletions, const vector<tuple<K, K, V>>& insertions, const vector<K>& vcom, const vector<W>& vtot, const vector<W>& ctot, double M, double R=1) {
  size_t S = y.span();
  size_t D = deletions.size();
  size_t I = insertions.size();
  fillValueOmpU(vertices,    B());
  fillValueOmpU(neighbors,   B());
  fillValueOmpU(communities, B());
  xorshift32_engine rng(0);
  #pragma omp parallel for schedule(auto)
  for (size_t i=0; i<D; ++i) {
    K u = get<0>(deletions[i]);
    K v = get<1>(deletions[i]);
    if (vcom[u] != vcom[v]) continue;
    vertices[u]  = 1;
    neighbors[u] = 1;
    communities[vcom[v]] = 1;
  }
  #pragma omp parallel
  {
    int T = omp_get_num_threads();
    int t = omp_get_thread_num();
    K  u0 = I>0? get<0>(insertions[0]) : 0;
    for (size_t i=0, n=0; i<I;) {
      K u = get<0>(insertions[i]);
      if (u!=u0) { ++n; u0 = u; }
      if (n % T != t) { ++i; continue; }
      leidenClearScanW(*vcs[t], *vcout[t]);
      for (; i<I && get<0>(insertions[i])==u; ++i) {
        K v = get<1>(insertions[i]);
        V w = get<2>(insertions[i]);
        if (vcom[u] == vcom[v]) continue;
        leidenScanCommunityW(*vcs[t], *vcout[t], u, v, w, vcom);
      }
      auto [c, e] = leidenChooseCommunity(rng, y, u, vcom, vtot, ctot, *vcs[t], *vcout[t], M, R);
      if (e<=0) continue;
      vertices[u]  = 1;
      neighbors[u] = 1;
      communities[c] = 1;
    }
  }
  #pragma omp parallel for schedule(auto)
  for (K u=0; u<S; ++u) {
    if (!y.hasVertex(u)) continue;
    if (neighbors[u]) y.forEachEdgeKey(u, [&](auto v) { vertices[v] = 1; });
    if (communities[vcom[u]]) vertices[u] = 1;
  }
}
#endif




/**
 * Obtain the community membership of each vertex with Dynamic Delta-screening Leiden.
 * @param rnd random number generator
 * @param y updated graph
 * @param deletions edge deletions in batch update (undirected, sorted by source vertex id)
 * @param insertions edge insertions in batch update (undirected, sorted by source vertex id)
 * @param q initial community each vertex belongs to
 * @param qvtot initial total edge weight of each vertex
 * @param qctot initial total edge weight of each community
 * @param o leiden options
 * @returns leiden result
 */
template <bool RANDOM=false, bool USEPARENT=false, class FLAG=char, class RND, class G, class K, class V, class W>
inline auto leidenDynamicDeltaScreening(RND& rnd, const G& y, const vector<tuple<K, K, V>>& deletions, const vector<tuple<K, K, V>>& insertions, const vector<K>& q, const vector<W>& qvtot, const vector<W>& qctot, const LeidenOptions& o={}) {
  using  B = FLAG;
  size_t S = y.span();
  double R = o.resolution;
  double M = edgeWeight(y)/2;
  vector<B> vertices(S), neighbors(S), communities(S);
  vector2d<K> qs;
  vector2d<W> qvtots, qctots;
  leidenSetupInitialsW(qs, qvtots, qctots, q, qvtot, qctot, o.repeat);
  int  r  = 0;
  auto fi = [&](auto& vcom, auto& vtot, auto& ctot) {
    vcom = move(qs[r]);
    vtot = move(qvtots[r]);
    ctot = move(qctots[r]); ++r;
    leidenUpdateWeightsFromU(vtot, ctot, y, deletions, insertions, vcom);
  };
  auto fm = [&](auto& vaff, auto& vcs, auto& vcout, const auto& vcom, const auto& vtot, const auto& ctot) {
    leidenAffectedVerticesDeltaScreeningW(vertices, neighbors, communities, vcs, vcout, y, deletions, insertions, vcom, vtot, ctot, M, R);
    copyValuesW(vaff, vertices);
  };
  auto fa = [&](auto u) { return vertices[u] == B(1); };
  auto ft = [ ](auto& cchg, auto c) { cchg[c] = FLAG(1); };
  auto fs = [ ](const auto& cchg, const auto& vcob, auto u) { return cchg[vcob[u]]; };
  return leidenInvoke<true, RANDOM, USEPARENT, FLAG>(rnd, y, o, fi, fm, fa, ft, fs);
}


#ifdef OPENMP
/**
 * Obtain the community membership of each vertex with Dynamic Delta-screening Leiden.
 * @param rnd random number generator
 * @param y updated graph
 * @param deletions edge deletions in batch update
 * @param insertions edge insertions in batch update
 * @param q initial community each vertex belongs to
 * @param qvtot initial total edge weight of each vertex
 * @param qctot initial total edge weight of each community
 * @param o leiden options
 * @returns leiden result
 */
template <bool RANDOM=false, bool USEPARENT=false, class FLAG=char, class RND, class G, class K, class V, class W>
inline auto leidenDynamicDeltaScreeningOmp(RND& rnd, const G& y, const vector<tuple<K, K, V>>& deletions, const vector<tuple<K, K, V>>& insertions, const vector<K>& q, const vector<W>& qvtot, const vector<W>& qctot, const LeidenOptions& o={}) {
  using  B = FLAG;
  size_t S = y.span();
  double R = o.resolution;
  double M = edgeWeightOmp(y)/2;
  int    T = omp_get_max_threads();
  vector<B> vertices(S), neighbors(S), communities(S);
  vector2d<K> qs;
  vector2d<W> qvtots, qctots;
  leidenSetupInitialsW(qs, qvtots, qctots, q, qvtot, qctot, o.repeat);
  int  r  = 0;
  auto fi = [&](auto& vcom, auto& vtot, auto& ctot) {
    vcom = move(qs[r]);
    vtot = move(qvtots[r]);
    ctot = move(qctots[r]); ++r;
    leidenUpdateWeightsFromOmpU(vtot, ctot, y, deletions, insertions, vcom);
  };
  auto fm = [&](auto& vaff, auto& vcs, auto& vcout, const auto& vcom, const auto& vtot, const auto& ctot) {
    leidenAffectedVerticesDeltaScreeningOmpW(vertices, neighbors, communities, vcs, vcout, y, deletions, insertions, vcom, vtot, ctot, M, R);
    copyValuesOmpW(vaff, vertices);
  };
  auto fa = [&](auto u) { return vertices[u] == B(1); };
  auto ft = [ ](auto& cchg, auto c) { cchg[c] = FLAG(1); };
  auto fs = [ ](const auto& cchg, const auto& vcob, auto u) { return cchg[vcob[u]]; };
  return leidenInvokeOmp<true, RANDOM, USEPARENT, FLAG>(rnd, y, o, fi, fm, fa, ft, fs);
}
#endif
#pragma endregion




#pragma region DYNAMIC FRONTIER APPROACH
/**
 * Find the vertices which should be processed upon a batch of edge insertions and deletions.
 * @param vertices vertex affected flags (output)
 * @param y updated graph
 * @param deletions edge deletions for this batch update (undirected)
 * @param insertions edge insertions for this batch update (undirected)
 * @param vcom community each vertex belongs to
 */
template <class B, class G, class K, class V>
inline void leidenAffectedVerticesFrontierW(vector<B>& vertices, const G& y, const vector<tuple<K, K, V>>& deletions, const vector<tuple<K, K, V>>& insertions, const vector<K>& vcom) {
  fillValueU(vertices, B());
  for (const auto& [u, v] : deletions) {
    if (vcom[u] != vcom[v]) continue;
    vertices[u]  = 1;
  }
  for (const auto& [u, v, w] : insertions) {
    if (vcom[u] == vcom[v]) continue;
    vertices[u]  = 1;
  }
}


#ifdef OPENMP
/**
 * Find the vertices which should be processed upon a batch of edge insertions and deletions.
 * @param vertices vertex affected flags (output)
 * @param y updated graph
 * @param deletions edge deletions for this batch update (undirected)
 * @param insertions edge insertions for this batch update (undirected)
 * @param vcom community each vertex belongs to
 * @returns flags for each vertex marking whether it is affected
 */
template <class B, class G, class K, class V>
inline void leidenAffectedVerticesFrontierOmpW(vector<B>& vertices, const G& y, const vector<tuple<K, K, V>>& deletions, const vector<tuple<K, K, V>>& insertions, const vector<K>& vcom) {
  fillValueOmpU(vertices, B());
  size_t D = deletions.size();
  size_t I = insertions.size();
  #pragma omp parallel for schedule(auto)
  for (size_t i=0; i<D; ++i) {
    K u = get<0>(deletions[i]);
    K v = get<1>(deletions[i]);
    if (vcom[u] != vcom[v]) continue;
    vertices[u]  = 1;
  }
  #pragma omp parallel for schedule(auto)
  for (size_t i=0; i<I; ++i) {
    K u = get<0>(insertions[i]);
    K v = get<1>(insertions[i]);
    if (vcom[u] == vcom[v]) continue;
    vertices[u]  = 1;
  }
}
#endif




/**
 * Obtain the community membership of each vertex with Dynamic Frontier Leiden.
 * @param rnd random number generator
 * @param y updated graph
 * @param deletions edge deletions in batch update
 * @param insertions edge insertions in batch update
 * @param q initial community each vertex belongs to
 * @param qvtot initial total edge weight of each vertex
 * @param qctot initial total edge weight of each community
 * @param o leiden options
 * @returns leiden result
 */
template <bool RANDOM=false, bool USEPARENT=false, class FLAG=char, class RND, class G, class K, class V, class W>
inline auto leidenDynamicFrontier(RND& rnd, const G& y, const vector<tuple<K, K, V>>& deletions, const vector<tuple<K, K, V>>& insertions, const vector<K>& q, const vector<W>& qvtot, const vector<W>& qctot, const LeidenOptions& o={}) {
  vector2d<K> qs;
  vector2d<W> qvtots, qctots;
  leidenSetupInitialsW(qs, qvtots, qctots, q, qvtot, qctot, o.repeat);
  int  r  = 0;
  auto fi = [&](auto& vcom, auto& vtot, auto& ctot) {
    vcom = move(qs[r]);
    vtot = move(qvtots[r]);
    ctot = move(qctots[r]); ++r;
    leidenUpdateWeightsFromU(vtot, ctot, y, deletions, insertions, vcom);
  };
  auto fm = [&](auto& vaff, auto& vcs, auto& vcout, const auto& vcom, const auto& vtot, const auto& ctot) {
    leidenAffectedVerticesFrontierW(vaff, y, deletions, insertions, vcom);
  };
  auto fa = [ ](auto u) { return true; };
  auto ft = [ ](auto& cchg, auto c) { cchg[c] = FLAG(1); };
  auto fs = [ ](const auto& cchg, const auto& vcob, auto u) { return cchg[vcob[u]]; };
  return leidenInvoke<true, RANDOM, USEPARENT, FLAG>(rnd, y, o, fi, fm, fa, ft, fs);
}


#ifdef OPENMP
/**
 * Obtain the community membership of each vertex with Dynamic Frontier Leiden.
 * @param rnd random number generator
 * @param y updated graph
 * @param deletions edge deletions in batch update
 * @param insertions edge insertions in batch update
 * @param q initial community each vertex belongs to
 * @param qvtot initial total edge weight of each vertex
 * @param qctot initial total edge weight of each community
 * @param o leiden options
 * @returns leiden result
 */
template <bool RANDOM=false, bool USEPARENT=false, class FLAG=char, class RND, class G, class K, class V, class W>
inline auto leidenDynamicFrontierOmp(RND& rnd, const G& y, const vector<tuple<K, K, V>>& deletions, const vector<tuple<K, K, V>>& insertions, const vector<K>& q, const vector<W>& qvtot, const vector<W>& qctot, const LeidenOptions& o={}) {
  vector2d<K> qs;
  vector2d<W> qvtots, qctots;
  leidenSetupInitialsW(qs, qvtots, qctots, q, qvtot, qctot, o.repeat);
  int  r  = 0;
  auto fi = [&](auto& vcom, auto& vtot, auto& ctot) {
    vcom = move(qs[r]);
    vtot = move(qvtots[r]);
    ctot = move(qctots[r]); ++r;
    leidenUpdateWeightsFromOmpU(vtot, ctot, y, deletions, insertions, vcom);
  };
  auto fm = [&](auto& vaff, auto& vcs, auto& vcout, const auto& vcom, const auto& vtot, const auto& ctot) {
    leidenAffectedVerticesFrontierOmpW(vaff, y, deletions, insertions, vcom);
  };
  constexpr int CHUNK_SIZE = 32;
  auto fa = [ ](auto u) { return true; };
  auto ft = [ ](auto& cchg, auto c) { cchg[c] = FLAG(1); };
  auto fs = [ ](const auto& cchg, const auto& vcob, auto u) { return cchg[vcob[u]]; };
  return leidenInvokeOmp<true, RANDOM, USEPARENT, FLAG, CHUNK_SIZE>(rnd, y, o, fi, fm, fa, ft, fs);
}
#endif
#pragma endregion
#pragma endregion
