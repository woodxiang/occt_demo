#include <iostream>

#include "./views/occt_view.h"

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Message_PrinterSystemLog.hxx>
#include <OSD_MemInfo.hxx>
#include <OSD_Parallel.hxx>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>

//! Dummy main loop callback for a single shot.
extern "C" void onMainLoop() {
  // do nothing here - viewer updates are handled on demand
  emscripten_cancel_main_loop();
}

void initialize(std::string canvasId) {
  OcctView &aViewer = OcctView::Instance();
  aViewer.run(canvasId);
  Message::DefaultMessenger()->Send(OSD_MemInfo::PrintInfo(), Message_Trace);
}

EMSCRIPTEN_BINDINGS(mainModule){
  emscripten::function("initialize", &initialize);
}

EMSCRIPTEN_KEEPALIVE int main() {
  Message::DefaultMessenger()->Printers().First()->SetTraceLevel(Message_Trace);
  Handle(Message_PrinterSystemLog) aJSConsolePrinter =
      new Message_PrinterSystemLog("webgl-sample", Message_Trace);
  Message::DefaultMessenger()->AddPrinter(
      aJSConsolePrinter); // open JavaScript console within the Browser to see
                          // this output
  Message::SendTrace() << "Emscripten SDK " << __EMSCRIPTEN_major__ << "."
                       << __EMSCRIPTEN_minor__ << "." << __EMSCRIPTEN_tiny__;
#if defined(__LP64__)
  Message::SendTrace() << "Architecture: WASM 64-bit";
#else
  Message::SendTrace() << "Architecture: WASM 32-bit";
#endif
  Message::SendTrace() << "NbLogicalProcessors: "
                       << OSD_Parallel::NbLogicalProcessors()
#ifdef __EMSCRIPTEN_PTHREADS__
                       << " (pthreads ON)"
#else
                       << " (pthreads OFF)"
#endif
      ;

  // setup a dummy single-shot main loop callback just to shut up a useless
  // Emscripten error message on calling eglSwapInterval()
  emscripten_set_main_loop(onMainLoop, -1, 0);
  return 0;
}