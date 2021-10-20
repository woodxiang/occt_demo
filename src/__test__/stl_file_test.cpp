#include "../stl_file.h"

#include <cstring>
#include <fstream>

#include "../membuf.h"

int main() {
  {
    std::ifstream ifs;
    ifs.open("ascii.stl");

    StlFile stlFile;

    stlFile.LoadFromStream(ifs);

    assert(stlFile.get_TriangleCount() > 0);

    ifs.close();
  }

  {
    std::ifstream ifs;
    ifs.open("binary.stl");

    StlFile stlFile;

    stlFile.LoadFromStream(ifs);

    assert(stlFile.get_TriangleCount() > 0);

    ifs.close();
  }
}