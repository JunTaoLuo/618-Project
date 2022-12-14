#include "common.h"
#include "timing.h"
#include <climits>
#include <omp.h>
#include "PriorityQueue.cpp"

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
struct Graph {
  std::vector<bool> done;
  std::vector<std::vector<uint>> edges;
  std::vector<uint> distances;
  std::vector<uint> offers;
  std::vector<omp_lock_t> distanceLocks;
  std::vector<omp_lock_t> offerLocks;
  PriorityQueue<D, N, R, IDBits, TKey, TVal>* pq;

  Graph(uint numVertices, const std::vector<std::vector<uint>> &_edges, std::vector<uint> &_distances, PriorityQueue<D, N, R, IDBits, TKey, TVal>* _pq) {
    done = std::vector<bool>(omp_get_num_threads(), false);
    offers = std::vector<uint>(numVertices, 0); // 0 indicates no current offer
    distances = _distances;
    edges = _edges;
    pq = _pq;
    distanceLocks = std::vector<omp_lock_t>(numVertices);
    offerLocks = std::vector<omp_lock_t>(numVertices);

    for (size_t i = 0; i < numVertices; i++)
    {
      omp_init_lock(&(distanceLocks[i]));
      omp_init_lock(&(offerLocks[i]));
    }
  }

  // returns whether distances were updated
  bool processOffer(uint distance, uint vertex) {
    bool updated = false;
    omp_set_lock(&(distanceLocks[vertex]));
    if (distance < distances[vertex]) {
      distances[vertex] = distance;
      updated = true;
    }
    omp_unset_lock(&(distanceLocks[vertex]));
    return updated;
  }

  void relax(uint vertex, uint distance) {
    omp_set_lock(&(offerLocks[vertex]));
    if (distance < distances[vertex]) {
      uint offerDistance = offers[vertex];
      if (offerDistance == 0 || distance < offerDistance) {
        offers[vertex] = distance;
        printf("Worker %d inserting new order (%u -> %u)\n", omp_get_thread_num(), vertex, distance);
        pq->insert(distance, vertex);
        printf("Worker %d inserted new order (%u -> %u)\n", omp_get_thread_num(), vertex, distance);
      }
    }
    omp_unset_lock(&(offerLocks[vertex]));
  }

  void resetDone() {
    int threadIndex = omp_get_thread_num();
    // printf("Worker %d resetting done\n", threadIndex);
    for (size_t i = 0; i < omp_get_num_threads(); i++)
    {
      done[i] = false;
    }
  }
};

template <int D, long N, int R, int IDBits, typename TKey, typename TVal>
void sssp_worker(Graph<D, N, R, IDBits, TKey, TVal> &graph) {
  int threadIndex = omp_get_thread_num();
  int numThreads = omp_get_num_threads();

  printf("Worker %d started\n", threadIndex);

  while (!graph.done[threadIndex]) {
    TKey distance;
    TVal vertex;
    bool valid;

    printf("Worker %d retrieving offer\n", threadIndex);
    tie(distance, vertex, valid) = graph.pq->deleteMin();
    if (valid) {
      printf("Worker %d retrieved offer (%u -> %u)\n", threadIndex, (uint)vertex, (uint)distance);
      if (graph.processOffer((uint)distance, (uint)vertex)) {
        uint currentVertex = (uint)vertex;
        auto neighbors = graph.edges[currentVertex];
        for (uint i = 0; i < neighbors.size(); i++) {
          if (i != currentVertex && neighbors[i] != 0 ) {
            uint newDistance = graph.distances[currentVertex] + neighbors[i];
            if (newDistance < graph.distances[i]) {
              graph.relax(i, newDistance);
            }
          }
        }
        graph.resetDone();
      }
    }
    else {
      printf("Worker %d did not retrieve any offers\n", threadIndex);
      graph.done[threadIndex] = true;
      while (graph.done[threadIndex]) {
        int i;
        for (i = 0; i < numThreads && graph.done[i]; i++) { }
        if (i == numThreads) {
          // printf("Worker %d received concensus\n", threadIndex);
          return;
        }
        // printf("Worker %d noticed worker %d is not done\n", threadIndex, i);
      }
      printf("Worker %d restarted\n", threadIndex);
    }
  }
}

// Single Source Shortest Path: Dijkstra's Algorithm
// Assume we want distance from node 0
void sssp(const std::vector<std::vector<uint>> &edges,
          std::vector<uint> &distances) {
  uint numVertices = distances.size();

  auto pq = new PriorityQueue<8, 4294967295, 100, 13, long, long>();
  pq->insert(0, 0);
  auto graph = Graph<8, 4294967295, 100, 13, long, long>(numVertices, edges, distances, pq);

  #pragma omp parallel shared(graph)
  {
    // printf("Hello world from omp thread %d/%d\n", omp_get_thread_num(), omp_get_num_threads());
    sssp_worker(graph);
  }

  graph.pq->printPQ();
  graph.pq->printStack();

  distances.swap(graph.distances);
}

int main(int argc, char *argv[]) {
  StartupOptions options = parseOptions(argc, argv);

  std::vector<std::vector<uint>> edges;
  loadGraphFromFile(options.inputFile, edges);
  std::vector<uint> distances(edges.size(), UINT_MAX);

  Timer totalTimer;

  sssp(edges, distances);

  double totalTime = totalTimer.elapsed();

  // Profiling results ==================
  printf("total time: %.6fs\n", totalTime);
  saveDistancesToFile(options.outputFile, distances);
}
