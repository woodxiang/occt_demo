#include "../RWStl_Stream_Reader.h"
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <cassert>
#include <fstream>
#include <string>

void testLoadStl(std::string fileName) {
  std::ifstream ifs;
  ifs.open(fileName, std::ios_base::binary);

  RWStl_Stream_Reader reader;
  reader.Read(ifs);
  ifs.close();

  auto triangulation = reader.GetTriangulation();
  auto &nodes = triangulation->InternalNodes();
  auto &triangles = triangulation->InternalTriangles();

  BRep_Builder builder;
  TopoDS_Compound compound;
  builder.MakeCompound(compound);

  for (Standard_Integer triIndex = triangles.Lower();
       triIndex < triangles.Upper(); triIndex++) {
    auto &triangle = triangles.Value(triIndex);
    Standard_Integer nodeIndexes[3];
    triangle.Get(nodeIndexes[0], nodeIndexes[1], nodeIndexes[2]);
    const auto &p1 = nodes[nodeIndexes[0]-1];
    const auto &p2 = nodes[nodeIndexes[1]-1];
    const auto &p3 = nodes[nodeIndexes[2]-1];

    if ((!p1.IsEqual(p2, 0.0)) && !p1.IsEqual(p3, 0.0)) {
      TopoDS_Vertex vertexes[] = {BRepBuilderAPI_MakeVertex(p1),
                                  BRepBuilderAPI_MakeVertex(p2),
                                  BRepBuilderAPI_MakeVertex(p3)};
      TopoDS_Wire wire = BRepBuilderAPI_MakePolygon(vertexes[0], vertexes[1],
                                                    vertexes[2], Standard_True);
      if (!wire.IsNull()) {
        TopoDS_Face face = BRepBuilderAPI_MakeFace(wire);
        if (!face.IsNull()) {
          builder.Add(compound, face);
        }
      }
    }
  }

  BRepBuilderAPI_Sewing sewing;
  sewing.Init();
  sewing.Load(compound);
  sewing.Perform();

  TopoDS_Shape shape = sewing.SewedShape();
  if (shape.IsNull()) {
    shape = compound;
  }
  return;
}

int main() {
  testLoadStl("binary.stl");
  testLoadStl("ascii.stl");
}