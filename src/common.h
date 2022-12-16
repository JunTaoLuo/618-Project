#ifndef COMMON_H_
#define COMMON_H_

#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct StartupOptions {
  std::string outputFile;
  std::string inputFile;
};

inline StartupOptions parseOptions(int argc, char *argv[]) {
  StartupOptions rs;
  for (int i = 1; i < argc; i++) {
    if (i < argc - 1) {
      if (strcmp(argv[i], "-in") == 0)
        rs.inputFile = argv[i + 1];
      else if (strcmp(argv[i], "-o") == 0)
        rs.outputFile = argv[i + 1];
    }
  }
  return rs;
}

struct MicroTestOptions {
  int D;
  long N;
  int R;
  int IDBits;
};

inline MicroTestOptions parseTestOptions(int argc, char *argv[]) {
  MicroTestOptions rs;
  for (int i = 1; i < argc; i++) {
    if (i < argc - 1) {
      if (strcmp(argv[i], "-D") == 0)
        rs.D = atoi(argv[i + 1]);
      else if (strcmp(argv[i], "-N") == 0)
        rs.N = atoi(argv[i + 1]);
      else if (strcmp(argv[i], "-R") == 0)
        rs.R = atoi(argv[i + 1]);
      else if (strcmp(argv[i], "-IDBits") == 0)
        rs.IDBits = atoi(argv[i + 1]);
    }
  }
  return rs;
}

// These functions are marked inline only because we want to define them in the
// header file.
inline bool loadGraphFromFile(std::string fileName,
                         std::vector<std::vector<uint>> &edges) {
  std::ifstream f(fileName);
  assert((bool)f && "Cannot open input file");

  std::string line;
  std::getline(f, line);
  int nodes = (int)atoi(line.c_str());

  for (int i = 0; i < nodes; i++)
  {
    std::vector<uint> row(nodes, 0);
    edges.push_back(row);
  }

  while (std::getline(f, line)) {
    std::stringstream sstream(line);
    std::string i_str;
    std::string j_str;
    std::string w_str;
    std::getline(sstream, i_str, ' ');
    std::getline(sstream, j_str, ' ');
    std::getline(sstream, w_str, ' ');
    uint i = (uint)atoi(i_str.c_str());
    uint j = (uint)atoi(j_str.c_str());
    uint w = (uint)atoi(w_str.c_str());
    edges[i][j] = w;
    edges[j][i] = w;
  }
  return true;
}

inline bool loadDistancesFromFile(std::string fileName,
                         std::vector<uint> &distances) {
  std::ifstream f(fileName);
  assert((bool)f && "Cannot open input file");

  std::string line;
  while (std::getline(f, line)) {
    distances.push_back((uint)atol(line.c_str()));
  }
  return true;
}

inline void saveDistancesToFile(std::string fileName,
                       const std::vector<uint> &distances) {
  std::ofstream f(fileName);
  assert((bool)f && "Cannot open output file");

  for (size_t i = 0; i < distances.size(); i++)
  {
    f << distances[i] << std::endl;
  }

  assert((bool)f && "Failed to write to output file");
}

#endif