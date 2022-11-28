#include "common.h"
#include "timing.h"
#include <climits>
#include <queue>

struct Vertex {
  int index, distance;
  Vertex(uint _index, uint _distance) : index(_index), distance(_distance) {}

  bool operator<(const Vertex &o) const {
    // Reverse direction to prioritize lowest values
    return distance > o.distance;
  }
};

// Single Source Shortest Path: Dijkstra's Algorithm
// Assume we want distance from node 0
void sssp(const std::vector<std::vector<uint>> &edges,
          std::vector<uint> &distances) {
  uint numVertices = distances.size();
  // Keeps track of which node has been visited
  std::vector<bool> visited(numVertices, false);
  // Priority queue for tracking the frontier nodes and their distances
  std::priority_queue<Vertex> frontier;

  // Visit node 0
  distances[0] = 0;
  frontier.push(Vertex(0, 0));
  uint visitedCount = 0;

  while (frontier.size() > 0) {
  // while (visitedCount < numVertices && frontier.size() > 0) { // Possible optimization
    auto v = frontier.top();
    frontier.pop();

    if (!visited[v.index]) {
      visited[v.index] = true;
      visitedCount++;
    }

    auto neighbors = edges[v.index];
    for (uint i = 0; i < neighbors.size(); i++) {
      if (i != v.index && !visited[i] && neighbors[i] != 0 && v.distance + neighbors[i] < distances[i]) {
        distances[i] = v.distance + neighbors[i];
        frontier.push(Vertex(i, distances[i]));
      }
    }
  }
}

int main(int argc, char *argv[]) {
  StartupOptions options = parseOptions(argc, argv);

  std::vector<std::vector<uint>> edges;
  loadGraphFromFile(options.inputFile, edges);
  std::vector<uint> distances(edges.size(), UINT_MAX);

  // for (size_t i = 0; i < edges.size(); i++)
  // {
  //   auto row = edges[i];
  //   for (size_t j = 0; j < edges.size(); j++)
  //   {
  //     printf("%u ", row[j]);
  //   }
  //   printf("\n");
  // }

  Timer totalTimer;

  sssp(edges, distances);

  double totalTime = totalTimer.elapsed();

  // Profiling results ==================
  printf("total time: %.6fs\n", totalTime);
  saveDistancesToFile(options.outputFile, distances);
}
