#include "pardijk.cpp"

using namespace std;

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