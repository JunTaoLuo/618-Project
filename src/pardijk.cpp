#include "common.h"
#include "timing.h"
#include <climits>
#include <queue>
#include <omp.h>
#include <string>
#include <iostream>
#include <sstream>

using namespace std;

struct Offer {
  int vertex, distance;
  Offer(uint _vertex, uint _distance) : vertex(_vertex), distance(_distance) {}

  bool operator<(const Offer &o) const {
    // Reverse direction to prioritize lowest values
    return distance > o.distance;
  }
};

struct OfferOrNull {
  Offer offer;
  bool isNull;

  OfferOrNull(Offer _offer, bool _isNull) : offer(_offer), isNull(_isNull) {}
};

struct GlobalLockPQ {
  std::priority_queue<Offer> pq;
  omp_lock_t lock;

  GlobalLockPQ() {
    omp_init_lock(&lock);
  }

  void insert(Offer offer) {
    omp_set_lock(&lock);
    pq.push(offer);
    omp_unset_lock(&lock);
  }

  OfferOrNull deleteMin() {
    omp_set_lock(&lock);

    if (pq.size() == 0) {
      omp_unset_lock(&lock);
      return OfferOrNull(Offer(0, 0), true);
    }

    auto minOffer = pq.top();
    pq.pop();

    omp_unset_lock(&lock);
    return OfferOrNull(minOffer, false);
  }
};

struct Graph {
  std::vector<bool> done;
  std::vector<std::vector<uint>> edges;
  std::vector<uint> distances;
  std::vector<uint> offers;
  std::vector<omp_lock_t> distanceLocks;
  std::vector<omp_lock_t> offerLocks;
  GlobalLockPQ pq;

  Graph(uint numVertices, const std::vector<std::vector<uint>> &_edges, std::vector<uint> &_distances, const GlobalLockPQ &_pq) {
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
  bool processOffer(Offer offer) {
    bool updated = false;
    omp_set_lock(&(distanceLocks[offer.vertex]));
    if (offer.distance < distances[offer.vertex]) {
      distances[offer.vertex] = offer.distance;
      updated = true;
    }
    omp_unset_lock(&(distanceLocks[offer.vertex]));
    return updated;
  }

  void relax(uint vertex, uint distance) {
    omp_set_lock(&(offerLocks[vertex]));
    if (distance < distances[vertex]) {
      uint offerDistance = offers[vertex];
      if (offerDistance == 0 || distance < offerDistance) {
        offers[vertex] = distance;
        // printf("Worker %d inserting new order (%u -> %u)\n", omp_get_thread_num(), vertex, distance);
        pq.insert(Offer(vertex, distance));
      }
    }
    omp_unset_lock(&(offerLocks[vertex]));
  }

  void resetDone() {
    for (size_t i = 0; i < omp_get_num_threads(); i++)
    {
      done[i] = false;
    }
  }
};

void sssp_worker(Graph &graph) {
  int threadIndex = omp_get_thread_num();
  int numThreads = omp_get_num_threads();

  printf("Worker %d started\n", threadIndex);

  while (!graph.done[threadIndex]) {
    auto result = graph.pq.deleteMin();
    if (!result.isNull) {
      // printf("Worker %d retrieved offer (%u -> %u)\n", threadIndex, result.offer.vertex, result.offer.distance);
      if (graph.processOffer(result.offer)) {
        uint offerVertex = result.offer.vertex;
        auto neighbors = graph.edges[offerVertex];
        for (uint i = 0; i < neighbors.size(); i++) {
          if (i != offerVertex && neighbors[i] != 0 ) {
            uint newDistance = graph.distances[offerVertex] + neighbors[i];
            if (newDistance < graph.distances[i]) {
              graph.relax(i, newDistance);
            }
          }
        }
        graph.resetDone();
      }
    }
    else {
      // printf("Worker %d did not retrieve any offers\n", threadIndex);
      graph.done[threadIndex] = true;
      while (graph.done[threadIndex]) {
        int i;
        stringstream buf;
        buf << "Worker " << threadIndex << " checking done ";
        for (i = 0; i < numThreads && graph.done[i]; i++) { buf << graph.done[i]; }
        buf << endl;
        cout << buf.str();
        if (i == numThreads) {
          printf("Worker %d done\n", threadIndex);
          return;
        }
      }
    }
  }
}

// Single Source Shortest Path: Dijkstra's Algorithm
// Assume we want distance from node 0
void sssp(const std::vector<std::vector<uint>> &edges,
          std::vector<uint> &distances) {
  uint numVertices = distances.size();
  GlobalLockPQ pq = GlobalLockPQ();
  pq.insert(Offer(0, 0));
  Graph graph = Graph(numVertices, edges, distances, pq);

  #pragma omp parallel shared(graph)
  {
    // printf("Hello world from omp thread %d/%d\n", omp_get_thread_num(), omp_get_num_threads());
    sssp_worker(graph);
  }

  distances.swap(graph.distances);
}
