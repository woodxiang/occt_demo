#include <iostream>
#include <emscripten.h>
#include <emscripten/html5.h>

#include <TCollection_AsciiString.hxx>
#include <gp_Pnt.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GC_MakeSegment.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopExp_Explorer.hxx>
#include <Geom_Plane.hxx>
#include <AIS_ViewController.hxx>

#include <Message_PrinterSystemLog.hxx>
#include <OSD_Parallel.hxx>
#include <Standard_ArrayStreamBuffer.hxx>
#include <BRepTools.hxx>
#include <AIS_Shape.hxx>

// #include "views/plot_view.h"
#include "views/occt_view.h"

static OcctView canvas_view;

void onMainLoop()
{
    emscripten_cancel_main_loop();
}

extern "C" void onFileDataRead(void *theOpaque, void *theBuffer, int theDataLen)
{
    const char *aName = theOpaque != NULL ? (const char *)theOpaque : "";
    {
        AIS_ListOfInteractive aShapes;
        canvas_view.Context()->DisplayedObjects(AIS_KOI_Shape, -1, aShapes);
        for (AIS_ListOfInteractive::Iterator aShapeIter(aShapes); aShapeIter.More(); aShapeIter.Next())
        {
            canvas_view.Context()->Remove(aShapeIter.Value(), false);
        }
    }

    Standard_ArrayStreamBuffer aStreamBuffer((const char *)theBuffer, theDataLen);
    std::istream aStream(&aStreamBuffer);
    TopoDS_Shape aShape;
    BRep_Builder aBuilder;
    BRepTools::Read(aShape, aStream, aBuilder);

    Handle(AIS_Shape) aShapePrs = new AIS_Shape(aShape);
    aShapePrs->SetMaterial(Graphic3d_NameOfMaterial_Silver);
    canvas_view.Context()->Display(aShapePrs, AIS_Shaded, 0, false);
    canvas_view.View()->FitAll(0.01, false);
    canvas_view.View()->Redraw();
    Message::DefaultMessenger()->Send(TCollection_AsciiString("Loaded file ") + aName, Message_Info);
    Message::DefaultMessenger()->Send(OSD_MemInfo::PrintInfo(), Message_Trace);
}

//! File read error event.
static void onFileReadFailed(void *theOpaque)
{
    const char *aName = (const char *)theOpaque;
    Message::DefaultMessenger()->Send(TCollection_AsciiString("Error: unable to load file ") + aName, Message_Fail);
}

int main()
{
#ifdef __EMSCRIPTEN__
    std::cout << "__EMSCRIPTEN__ Defined." << std::endl;
#endif
    Message::DefaultMessenger()->Printers().First()->SetTraceLevel(Message_Trace);
    Handle(Message_PrinterSystemLog) js_console_printer = new Message_PrinterSystemLog("webgl-sample", Message_Trace);
    Message::DefaultMessenger()->AddPrinter(js_console_printer);
    Message::DefaultMessenger()->Send(TCollection_AsciiString("NbLogicalProcessors: ") + OSD_Parallel::NbLogicalProcessors(), Message_Trace);

    emscripten_set_main_loop(onMainLoop, -1, 0);

    canvas_view.Run();

    Message::DefaultMessenger()->Send(OSD_MemInfo::PrintInfo(), Message_Trace);

    // load some file
    emscripten_async_wget_data("samples/Ball.brep", (void *)"samples/Ball.brep", onFileDataRead, onFileReadFailed);

    // define variables.
    Standard_Real myWidth = 100;
    Standard_Real myThickness = 20;
    Standard_Real myHeight = 120;

    // define points.
    gp_Pnt aPnt1(-myWidth / 2., 0, 0);
    gp_Pnt aPnt2(-myWidth / 2., -myThickness / 4., 0);
    gp_Pnt aPnt3(0, -myThickness / 2., 0);
    gp_Pnt aPnt4(myWidth / 2., -myThickness / 4., 0);
    gp_Pnt aPnt5(myWidth / 2., 0, 0);

    // defined segements and curves.
    Handle(Geom_TrimmedCurve) aArcOfCircle = GC_MakeArcOfCircle(aPnt2, aPnt3, aPnt4);
    Handle(Geom_TrimmedCurve) aSegment1 = GC_MakeSegment(aPnt1, aPnt2);
    Handle(Geom_TrimmedCurve) aSegment2 = GC_MakeSegment(aPnt4, aPnt5);

    // convert to edges
    TopoDS_Edge aEdge1 = BRepBuilderAPI_MakeEdge(aSegment1);
    TopoDS_Edge aEdge2 = BRepBuilderAPI_MakeEdge(aArcOfCircle);
    TopoDS_Edge aEdge3 = BRepBuilderAPI_MakeEdge(aSegment2);

    // convert to a wire
    TopoDS_Wire aWire = BRepBuilderAPI_MakeWire(aEdge1, aEdge2, aEdge3);

    // mirror it.
    gp_Ax1 xAxis = gp::OX();

    gp_Trsf aTrsf;
    aTrsf.SetMirror(xAxis);

    BRepBuilderAPI_Transform aBRepTrsf(aWire, aTrsf);
    TopoDS_Shape aMirroredShape = aBRepTrsf.Shape();
    TopoDS_Wire aMirroredWire = TopoDS::Wire(aMirroredShape);

    // comine the wire and its mirror to a single wire
    BRepBuilderAPI_MakeWire mkWire;
    mkWire.Add(aWire);
    mkWire.Add(aMirroredWire);
    TopoDS_Wire myWireProfile = mkWire.Wire();

    // make the wire to a face.
    TopoDS_Face myFaceProfile = BRepBuilderAPI_MakeFace(myWireProfile);

    // make the face into a prism
    gp_Vec aPrismVec(0, 0, myHeight);
    TopoDS_Shape myBody = BRepPrimAPI_MakePrism(myFaceProfile, aPrismVec);

    // make fillet
    BRepFilletAPI_MakeFillet mkFillet(myBody);

    // enumerate the edges in the body make fillet.
    TopExp_Explorer anEdgeExplorer(myBody, TopAbs_EDGE);
    while (anEdgeExplorer.More())
    {
        TopoDS_Edge anEdge = TopoDS::Edge(anEdgeExplorer.Current());
        mkFillet.Add(myThickness / 12.0, anEdge);
        anEdgeExplorer.Next();
    }

    // add the neck
    gp_Pnt neckLocation(0, 0, myHeight);
    gp_Dir neckAxis = gp::DZ();
    gp_Ax2 neckAx2(neckLocation, neckAxis);

    // create cylindar
    Standard_Real myNeckRadius = myThickness / 4.;
    Standard_Real myNeckHeight = myHeight / 10;
    BRepPrimAPI_MakeCylinder MKCylinder(neckAx2, myNeckRadius, myNeckHeight);
    TopoDS_Shape myNeck = MKCylinder.Shape();

    // combin the body and neck
    myBody = BRepAlgoAPI_Fuse(myBody, myNeck);

    // Create hollowed solid

    // find the topest face as the part to remove.
    TopoDS_Face faceToRemove;
    Standard_Real zMax = -1;
    for (TopExp_Explorer aFaceExplorer(myBody, TopAbs_FACE); aFaceExplorer.More(); aFaceExplorer.Next())
    {
        TopoDS_Face aFace = TopoDS::Face(aFaceExplorer.Current());

        Handle(Geom_Surface) aSurface = BRep_Tool::Surface(aFace);
        if (aSurface->DynamicType() == STANDARD_TYPE(Geom_Plane))
        {
            Handle(Geom_Plane) aPlane = Handle(Geom_Plane)::DownCast(aSurface);
            gp_Pnt aPnt = aPlane->Location();
            Standard_Real aZ = aPnt.Z();
            if (aZ > zMax)
            {
                zMax = aZ;
                faceToRemove = aFace;
            }
        }
    }

    TopTools_ListOfShape facesToRemove;
    facesToRemove.Append(faceToRemove);

    BRepOffsetAPI_MakeThickSolid BodyMaker;
    BodyMaker.MakeThickSolidByJoin(myBody, facesToRemove, -myThickness / 50, 1.e-3);
    myBody = BodyMaker.Shape();
}