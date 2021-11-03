#include "model_factory.h"

#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepLib.hxx>
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <GCE2d_MakeSegment.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GC_MakeSegment.hxx>
#include <Geom2d_Ellipse.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_Plane.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <cassert>
#include <gp_Ax1.hxx>

#include "help_algorithms.h"
// #include "stl_file.h"
#include "RWStl_Stream_Reader.h"

ModelFactory::ModelFactory(/* args */) {}

ModelFactory::~ModelFactory() {}

ModelFactory *ModelFactory::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new ModelFactory();
  }

  return instance_;
}

ModelFactory *ModelFactory::instance_ = nullptr;

TopoDS_Shape ModelFactory::MakeBottle(const Standard_Real myWidth,
                                      const Standard_Real myHeight,
                                      const Standard_Real myThickness) {
  // Profile : Define Support Points
  gp_Pnt aPnt1(-myWidth / 2., 0, 0);
  gp_Pnt aPnt2(-myWidth / 2., -myThickness / 4., 0);
  gp_Pnt aPnt3(0, -myThickness / 2., 0);
  gp_Pnt aPnt4(myWidth / 2., -myThickness / 4., 0);
  gp_Pnt aPnt5(myWidth / 2., 0, 0);

  // Profile : Define the Geometry
  Handle(Geom_TrimmedCurve) anArcOfCircle =
      GC_MakeArcOfCircle(aPnt2, aPnt3, aPnt4);
  Handle(Geom_TrimmedCurve) aSegment1 = GC_MakeSegment(aPnt1, aPnt2);
  Handle(Geom_TrimmedCurve) aSegment2 = GC_MakeSegment(aPnt4, aPnt5);

  // Profile : Define the Topology
  TopoDS_Edge anEdge1 = BRepBuilderAPI_MakeEdge(aSegment1);
  TopoDS_Edge anEdge2 = BRepBuilderAPI_MakeEdge(anArcOfCircle);
  TopoDS_Edge anEdge3 = BRepBuilderAPI_MakeEdge(aSegment2);
  TopoDS_Wire aWire = BRepBuilderAPI_MakeWire(anEdge1, anEdge2, anEdge3);

  // Complete Profile
  gp_Ax1 xAxis = gp::OX();
  gp_Trsf aTrsf;

  aTrsf.SetMirror(xAxis);
  BRepBuilderAPI_Transform aBRepTrsf(aWire, aTrsf);
  TopoDS_Shape aMirroredShape = aBRepTrsf.Shape();
  TopoDS_Wire aMirroredWire = TopoDS::Wire(aMirroredShape);

  BRepBuilderAPI_MakeWire mkWire;
  mkWire.Add(aWire);
  mkWire.Add(aMirroredWire);
  TopoDS_Wire myWireProfile = mkWire.Wire();

  // Body : Prism the Profile
  TopoDS_Face myFaceProfile = BRepBuilderAPI_MakeFace(myWireProfile);
  gp_Vec aPrismVec(0, 0, myHeight);
  TopoDS_Shape myBody = BRepPrimAPI_MakePrism(myFaceProfile, aPrismVec);

  // Body : Apply Fillets
  BRepFilletAPI_MakeFillet mkFillet(myBody);
  TopExp_Explorer anEdgeExplorer(myBody, TopAbs_EDGE);
  while (anEdgeExplorer.More()) {
    TopoDS_Edge anEdge = TopoDS::Edge(anEdgeExplorer.Current());
    // Add edge to fillet algorithm
    mkFillet.Add(myThickness / 12., anEdge);
    anEdgeExplorer.Next();
  }

  myBody = mkFillet.Shape();

  // Body : Add the Neck
  gp_Pnt neckLocation(0, 0, myHeight);
  gp_Dir neckAxis = gp::DZ();
  gp_Ax2 neckAx2(neckLocation, neckAxis);

  Standard_Real myNeckRadius = myThickness / 4.;
  Standard_Real myNeckHeight = myHeight / 10.;

  BRepPrimAPI_MakeCylinder MKCylinder(neckAx2, myNeckRadius, myNeckHeight);
  TopoDS_Shape myNeck = MKCylinder.Shape();

  myBody = BRepAlgoAPI_Fuse(myBody, myNeck);

  // Body : Create a Hollowed Solid
  TopoDS_Face faceToRemove;
  Standard_Real zMax = -1;

  for (TopExp_Explorer aFaceExplorer(myBody, TopAbs_FACE); aFaceExplorer.More();
       aFaceExplorer.Next()) {
    TopoDS_Face aFace = TopoDS::Face(aFaceExplorer.Current());
    // Check if <aFace> is the top face of the bottle's neck
    Handle(Geom_Surface) aSurface = BRep_Tool::Surface(aFace);
    if (aSurface->DynamicType() == STANDARD_TYPE(Geom_Plane)) {
      Handle(Geom_Plane) aPlane = Handle(Geom_Plane)::DownCast(aSurface);
      gp_Pnt aPnt = aPlane->Location();
      Standard_Real aZ = aPnt.Z();
      if (aZ > zMax) {
        zMax = aZ;
        faceToRemove = aFace;
      }
    }
  }

  TopTools_ListOfShape facesToRemove;
  facesToRemove.Append(faceToRemove);
  BRepOffsetAPI_MakeThickSolid BodyMaker;
  BodyMaker.MakeThickSolidByJoin(myBody, facesToRemove, -myThickness / 50,
                                 1.e-3);
  myBody = BodyMaker.Shape();
  // Threading : Create Surfaces
  Handle(Geom_CylindricalSurface) aCyl1 =
      new Geom_CylindricalSurface(neckAx2, myNeckRadius * 0.99);
  Handle(Geom_CylindricalSurface) aCyl2 =
      new Geom_CylindricalSurface(neckAx2, myNeckRadius * 1.05);

  // Threading : Define 2D Curves
  gp_Pnt2d aPnt(2. * M_PI, myNeckHeight / 2.);
  gp_Dir2d aDir(2. * M_PI, myNeckHeight / 4.);
  gp_Ax2d anAx2d(aPnt, aDir);

  Standard_Real aMajor = 2. * M_PI;
  Standard_Real aMinor = myNeckHeight / 10;

  Handle(Geom2d_Ellipse) anEllipse1 =
      new Geom2d_Ellipse(anAx2d, aMajor, aMinor);
  Handle(Geom2d_Ellipse) anEllipse2 =
      new Geom2d_Ellipse(anAx2d, aMajor, aMinor / 4);
  Handle(Geom2d_TrimmedCurve) anArc1 =
      new Geom2d_TrimmedCurve(anEllipse1, 0, M_PI);
  Handle(Geom2d_TrimmedCurve) anArc2 =
      new Geom2d_TrimmedCurve(anEllipse2, 0, M_PI);
  gp_Pnt2d anEllipsePnt1 = anEllipse1->Value(0);
  gp_Pnt2d anEllipsePnt2 = anEllipse1->Value(M_PI);

  Handle(Geom2d_TrimmedCurve) aSegment =
      GCE2d_MakeSegment(anEllipsePnt1, anEllipsePnt2);
  // Threading : Build Edges and Wires
  TopoDS_Edge anEdge1OnSurf1 = BRepBuilderAPI_MakeEdge(anArc1, aCyl1);
  TopoDS_Edge anEdge2OnSurf1 = BRepBuilderAPI_MakeEdge(aSegment, aCyl1);
  TopoDS_Edge anEdge1OnSurf2 = BRepBuilderAPI_MakeEdge(anArc2, aCyl2);
  TopoDS_Edge anEdge2OnSurf2 = BRepBuilderAPI_MakeEdge(aSegment, aCyl2);
  TopoDS_Wire threadingWire1 =
      BRepBuilderAPI_MakeWire(anEdge1OnSurf1, anEdge2OnSurf1);
  TopoDS_Wire threadingWire2 =
      BRepBuilderAPI_MakeWire(anEdge1OnSurf2, anEdge2OnSurf2);
  BRepLib::BuildCurves3d(threadingWire1);
  BRepLib::BuildCurves3d(threadingWire2);

  // Create Threading
  BRepOffsetAPI_ThruSections aTool(Standard_True);
  aTool.AddWire(threadingWire1);
  aTool.AddWire(threadingWire2);
  aTool.CheckCompatibility(Standard_False);

  TopoDS_Shape myThreading = aTool.Shape();

  // Building the Resulting Compound
  TopoDS_Compound aRes;
  BRep_Builder aBuilder;
  aBuilder.MakeCompound(aRes);
  aBuilder.Add(aRes, myBody);
  aBuilder.Add(aRes, myThreading);

  return aRes;
}

TopoDS_Shape ModelFactory::LoadFromStl(std::istream &is) {
  RWStl_Stream_Reader reader;
  reader.Read(is);

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
    const auto &p1 = nodes[nodeIndexes[0] - 1];
    const auto &p2 = nodes[nodeIndexes[1] - 1];
    const auto &p3 = nodes[nodeIndexes[2] - 1];

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

  // BRepBuilderAPI_Sewing sewing;
  // sewing.Init();
  // sewing.Load(compound);
  // sewing.Perform();

  // TopoDS_Shape shape = sewing.SewedShape();
  // if (shape.IsNull()) {
  //   shape = compound;
  // }
  return compound;
}

/*
TopoDS_Shape ModelFactory::LoadFromStl(std::istream &is) {
  StlFile stlFile;

  stlFile.LoadFromStream(is);

  std::vector<float> vertexes;
  std::vector<unsigned int> indexes;
  stlFile.ToIndexedData(vertexes, indexes);

  std::vector<std::tuple<unsigned int, unsigned int>> edges;
  std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> triangles;

  file_edges(indexes, edges, triangles);

  std::vector<gp_Pnt> points;
  points.reserve(vertexes.size() / 3);
  for (size_t i = 0; i < vertexes.size() / 3; i++) {
    points.push_back(
        gp_Pnt(vertexes[i * 3], vertexes[i * 3 + 1], vertexes[i * 3 + 2]));
  }

  std::vector<TopoDS_Edge> topoEdges;
  topoEdges.reserve(edges.size());
  for (auto edge : edges) {
    topoEdges.push_back(BRepBuilderAPI_MakeEdge(points[std::get<0>(edge)],
points[std::get<1>(edge)]));
  }

  std::vector<TopoDS_Wire> topoWires;
  topoWires.reserve(triangles.size());
  for (auto triangle : triangles) {
    auto topoWire = BRepBuilderAPI_MakeWire(
        topoEdges[std::get<0>(triangle)], topoEdges[std::get<1>(triangle)],
        topoEdges[std::get<2>(triangle)]);
    topoWires.push_back(topoWire);

    BRepBuilderAPI_MakeFace(topoWires);
  }
}
*/