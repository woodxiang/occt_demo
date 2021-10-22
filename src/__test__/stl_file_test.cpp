#include "../stl_file.h"

#include <cstring>
#include <fstream>

#include "../help_algorithms.h"
#include "../membuf.h"

void testLoadStl(std::string fileName) {
  std::ifstream ifs;
  ifs.open(fileName, std::ios_base::binary);

  StlFile stlFile;

  stlFile.LoadFromStream(ifs);

  ifs.close();

  assert(stlFile.get_TriangleCount() > 0);

  std::vector<float> vertexes;
  std::vector<unsigned int> indexes;

  stlFile.ToIndexedData(vertexes, indexes);

  std::cout << "loaded file " << fileName << std::endl;
  std::cout << "Vertexes Number :" << vertexes.size() / 3 << std::endl;

  std::vector<std::tuple<unsigned int, unsigned int>> edges;
  std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> triangles;

  findEdges(indexes, edges, triangles);

  std::cout << "Edges Number: " << edges.size() << std::endl;

  std::cout << "Triangles Number :" << triangles.size() << std::endl;
}

int main() {
  testLoadStl("ascii.stl");
  testLoadStl("binary.stl");
}