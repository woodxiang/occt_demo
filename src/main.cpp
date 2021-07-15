#include <iostream>
#include <emscripten.h>
#include <emscripten/html5.h>

#include "views/occt_view.h"
#include "model_factory.h"

#include <Standard_ArrayStreamBuffer.hxx>
#include <TopoDS_Shape.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <AIS_Shape.hxx>

#include <Message.hxx>
#include <Message_PrinterSystemLog.hxx>
#include <OSD_Parallel.hxx>

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

void AddBottle()
{
    {
        AIS_ListOfInteractive aShapes;
        canvas_view.Context()->DisplayedObjects(AIS_KOI_Shape, -1, aShapes);
        for (AIS_ListOfInteractive::Iterator aShapeIter(aShapes); aShapeIter.More(); aShapeIter.Next())
        {
            canvas_view.Context()->Remove(aShapeIter.Value(), false);
        }
    }

    // define variables.
    Standard_Real myWidth = 100;
    Standard_Real myThickness = 20;
    Standard_Real myHeight = 120;

    TopoDS_Shape bottle = ModelFactory::GetInstance()->MakeBottle(myWidth, myHeight, myThickness);

    Handle(AIS_Shape) aShapePrs = new AIS_Shape(bottle);
    aShapePrs->SetMaterial(Graphic3d_NameOfMaterial_Silver);
    canvas_view.Context()->Display(aShapePrs, AIS_Shaded, 0, false);
    canvas_view.View()->FitAll(0.01, false);
    canvas_view.View()->Redraw();
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
    // emscripten_async_wget_data("samples/Ball.brep", (void *)"samples/Ball.brep", onFileDataRead, onFileReadFailed);
    AddBottle();
}