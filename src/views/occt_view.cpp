// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is part of the examples of the Open CASCADE Technology software library.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE

#include "occt_view.h"

#include "occt_virtual_keys.h"

#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>
#include <Aspect_Handle.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Prs3d_DatumAspect.hxx>

#include <iostream>

#define THE_CANVAS_ID "canvas"

namespace
{
  EM_JS(int, js_canvas_get_width, (), {
    return canvas.width;
  });

  EM_JS(int, js_canvas_get_height, (), {
    return canvas.height;
  });

  EM_JS(float, js_device_pixel_ratio, (),
        {
          var aDevicePixelRatio = window.devicePixelRatio || 1;
          return aDevicePixelRatio;
        });

  //! Return cavas size in pixels.
  static Graphic3d_Vec2i js_canvas_size()
  {
    return Graphic3d_Vec2i(js_canvas_get_width(), js_canvas_get_height());
  }
}

// ================================================================
// Function : WasmOcctView
// Purpose  :
// ================================================================
OcctView::OcctView()
    : device_pixel_ratio_(1.0f),
      update_requests_(0)
{
}

// ================================================================
// Function : ~WasmOcctView
// Purpose  :
// ================================================================
OcctView::~OcctView()
{
}

// ================================================================
// Function : run
// Purpose  :
// ================================================================
void OcctView::Run()
{
  initWindow();
  initViewer();
  initDemoScene();
  if (v3d_view_.IsNull())
  {
    return;
  }

  v3d_view_->MustBeResized();
  v3d_view_->Redraw();

  // There is no inifinite message loop, main() will return from here immediately.
  // Tell that our Module should be left loaded and handle events through callbacks.
  //emscripten_set_main_loop (redrawView, 60, 1);
  //emscripten_set_main_loop (redrawView, -1, 1);
  EM_ASM(Module['noExitRuntime'] = true);
}

// ================================================================
// Function : initWindow
// Purpose  :
// ================================================================
void OcctView::initWindow()
{
  device_pixel_ratio_ = js_device_pixel_ratio();
  canvas_id_ = THE_CANVAS_ID;
  const char *aTargetId = !canvas_id_.IsEmpty() ? canvas_id_.ToCString() : EMSCRIPTEN_EVENT_TARGET_WINDOW;
  const EM_BOOL toUseCapture = EM_TRUE;
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, toUseCapture, onResizeCallback);

  emscripten_set_mousedown_callback(aTargetId, this, toUseCapture, onMouseCallback);
  emscripten_set_mouseup_callback(aTargetId, this, toUseCapture, onMouseCallback);
  emscripten_set_mousemove_callback(aTargetId, this, toUseCapture, onMouseCallback);
  emscripten_set_dblclick_callback(aTargetId, this, toUseCapture, onMouseCallback);
  emscripten_set_click_callback(aTargetId, this, toUseCapture, onMouseCallback);
  emscripten_set_mouseenter_callback(aTargetId, this, toUseCapture, onMouseCallback);
  emscripten_set_mouseleave_callback(aTargetId, this, toUseCapture, onMouseCallback);
  emscripten_set_wheel_callback(aTargetId, this, toUseCapture, onWheelCallback);

  emscripten_set_touchstart_callback(aTargetId, this, toUseCapture, onTouchCallback);
  emscripten_set_touchend_callback(aTargetId, this, toUseCapture, onTouchCallback);
  emscripten_set_touchmove_callback(aTargetId, this, toUseCapture, onTouchCallback);
  emscripten_set_touchcancel_callback(aTargetId, this, toUseCapture, onTouchCallback);

  //emscripten_set_keypress_callback   (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, toUseCapture, onKeyCallback);
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, toUseCapture, onKeyDownCallback);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, toUseCapture, onKeyUpCallback);
}

// ================================================================
// Function : dumpGlInfo
// Purpose  :
// ================================================================
void OcctView::dumpGlInfo(bool theIsBasic)
{
  TColStd_IndexedDataMapOfStringString aGlCapsDict;
  v3d_view_->DiagnosticInformation(aGlCapsDict, theIsBasic ? Graphic3d_DiagnosticInfo_Basic : Graphic3d_DiagnosticInfo_Complete);
  if (theIsBasic)
  {
    TCollection_AsciiString aViewport;
    aGlCapsDict.FindFromKey("Viewport", aViewport);
    aGlCapsDict.Clear();
    aGlCapsDict.Add("Viewport", aViewport);
  }
  aGlCapsDict.Add("Display scale", TCollection_AsciiString(device_pixel_ratio_));

  // beautify output
  {
    TCollection_AsciiString *aGlVer = aGlCapsDict.ChangeSeek("GLversion");
    TCollection_AsciiString *aGlslVer = aGlCapsDict.ChangeSeek("GLSLversion");
    if (aGlVer != NULL && aGlslVer != NULL)
    {
      *aGlVer = *aGlVer + " [GLSL: " + *aGlslVer + "]";
      aGlslVer->Clear();
    }
  }

  TCollection_AsciiString anInfo;
  for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter(aGlCapsDict); aValueIter.More(); aValueIter.Next())
  {
    if (!aValueIter.Value().IsEmpty())
    {
      if (!anInfo.IsEmpty())
      {
        anInfo += "\n";
      }
      anInfo += aValueIter.Key() + ": " + aValueIter.Value();
    }
  }

  ::Message::DefaultMessenger()->Send(anInfo, Message_Warning);
}

// ================================================================
// Function : initPixelScaleRatio
// Purpose  :
// ================================================================
void OcctView::initPixelScaleRatio()
{
  SetTouchToleranceScale(device_pixel_ratio_);
  if (!v3d_view_.IsNull())
  {
    v3d_view_->ChangeRenderingParams().Resolution = (unsigned int)(96.0 * device_pixel_ratio_ + 0.5);
  }
  if (!interactive_context_.IsNull())
  {
    interactive_context_->SetPixelTolerance(int(device_pixel_ratio_ * 6.0));
    if (!view_cube_.IsNull())
    {
      static const double THE_CUBE_SIZE = 60.0;
      view_cube_->SetSize(device_pixel_ratio_ * THE_CUBE_SIZE, false);
      view_cube_->SetBoxFacetExtension(view_cube_->Size() * 0.15);
      view_cube_->SetAxesPadding(view_cube_->Size() * 0.10);
      view_cube_->SetFontHeight(THE_CUBE_SIZE * 0.16);
      if (view_cube_->HasInteractiveContext())
      {
        interactive_context_->Redisplay(view_cube_, false);
      }
    }
  }
}

// ================================================================
// Function : initViewer
// Purpose  :
// ================================================================
bool OcctView::initViewer()
{
  // Build with "--preload-file MyFontFile.ttf" option
  // and register font in Font Manager to use custom font(s).
  /*const char* aFontPath = "MyFontFile.ttf";
  if (Handle(Font_SystemFont) aFont = Font_FontMgr::GetInstance()->CheckFont (aFontPath))
  {
    Font_FontMgr::GetInstance()->RegisterFont (aFont, true);
  }
  else
  {
    Message::DefaultMessenger()->Send (TCollection_AsciiString ("Error: font '") + aFontPath + "' is not found", Message_Fail);
  }*/

  Handle(Aspect_DisplayConnection) aDisp;
  Handle(OpenGl_GraphicDriver) aDriver = new OpenGl_GraphicDriver(aDisp, false);
  aDriver->ChangeOptions().buffersNoSwap = true; // swap has no effect in WebGL
  if (!aDriver->InitContext())
  {
    Message::DefaultMessenger()->Send(TCollection_AsciiString("Error: EGL initialization failed"), Message_Fail);
    return false;
  }

  Handle(V3d_Viewer) aViewer = new V3d_Viewer(aDriver);
  aViewer->SetComputedMode(false);
  aViewer->SetDefaultShadingModel(Graphic3d_TOSM_FRAGMENT);
  aViewer->SetDefaultLights();
  aViewer->SetLightOn();

  Handle(Aspect_NeutralWindow) aWindow = new Aspect_NeutralWindow();
  Graphic3d_Vec2i aWinSize = js_canvas_size();
  if (aWinSize.x() < 10 || aWinSize.y() < 10)
  {
    Message::DefaultMessenger()->Send(TCollection_AsciiString("Warning: invalid canvas size"), Message_Warning);
  }
  aWindow->SetSize(aWinSize.x(), aWinSize.y());

  text_style_ = new Prs3d_TextAspect();
  text_style_->SetFont(Font_NOF_ASCII_MONO);
  text_style_->SetHeight(12);
  text_style_->Aspect()->SetColor(Quantity_NOC_GRAY95);
  text_style_->Aspect()->SetColorSubTitle(Quantity_NOC_BLACK);
  text_style_->Aspect()->SetDisplayType(Aspect_TODT_SHADOW);
  text_style_->Aspect()->SetTextFontAspect(Font_FA_Bold);
  text_style_->Aspect()->SetTextZoomable(false);
  text_style_->SetHorizontalJustification(Graphic3d_HTA_LEFT);
  text_style_->SetVerticalJustification(Graphic3d_VTA_BOTTOM);

  v3d_view_ = new V3d_View(aViewer);
  v3d_view_->SetImmediateUpdate(false);
  v3d_view_->ChangeRenderingParams().Resolution = (unsigned int)(96.0 * device_pixel_ratio_ + 0.5);
  v3d_view_->ChangeRenderingParams().ToShowStats = true;
  v3d_view_->ChangeRenderingParams().StatsTextAspect = text_style_->Aspect();
  v3d_view_->ChangeRenderingParams().StatsTextHeight = (int)text_style_->Height();
  v3d_view_->SetWindow(aWindow);
  dumpGlInfo(false);

  interactive_context_ = new AIS_InteractiveContext(aViewer);
  initPixelScaleRatio();
  return true;
}

// ================================================================
// Function : initDemoScene
// Purpose  :
// ================================================================
void OcctView::initDemoScene()
{
  if (interactive_context_.IsNull())
  {
    return;
  }

  //myView->TriedronDisplay (Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GOLD, 0.08, V3d_WIREFRAME);

  view_cube_ = new AIS_ViewCube();
  // presentation parameters
  initPixelScaleRatio();
  view_cube_->SetTransformPersistence(new Graphic3d_TransformPers(Graphic3d_TMF_TriedronPers, Aspect_TOTP_RIGHT_LOWER, Graphic3d_Vec2i(100, 100)));
  view_cube_->Attributes()->SetDatumAspect(new Prs3d_DatumAspect());
  view_cube_->Attributes()->DatumAspect()->SetTextAspect(text_style_);
  // animation parameters
  view_cube_->SetViewAnimation(myViewAnimation);
  view_cube_->SetFixedAnimationLoop(false);
  view_cube_->SetAutoStartAnimation(true);
  interactive_context_->Display(view_cube_, false);

  // Build with "--preload-file MySampleFile.brep" option to load some shapes here.
}

// ================================================================
// Function : updateView
// Purpose  :
// ================================================================
void OcctView::updateView()
{
  if (!v3d_view_.IsNull())
  {
    if (++update_requests_ == 1)
    {
      emscripten_async_call(onRedrawView, this, 0);
    }
  }
}

// ================================================================
// Function : redrawView
// Purpose  :
// ================================================================
void OcctView::redrawView()
{
  if (!v3d_view_.IsNull())
  {
    FlushViewEvents(interactive_context_, v3d_view_, true);
  }
}

// ================================================================
// Function : handleViewRedraw
// Purpose  :
// ================================================================
void OcctView::handleViewRedraw(const Handle(AIS_InteractiveContext) & theCtx,
                                    const Handle(V3d_View) & theView)
{
  update_requests_ = 0;
  AIS_ViewController::handleViewRedraw(theCtx, theView);
  if (myToAskNextFrame)
  {
    // ask more frames
    ++update_requests_;
    emscripten_async_call(onRedrawView, this, 0);
  }
}

// ================================================================
// Function : onResizeEvent
// Purpose  :
// ================================================================
EM_BOOL OcctView::onResizeEvent(int theEventType, const EmscriptenUiEvent *theEvent)
{
  (void)theEventType; // EMSCRIPTEN_EVENT_RESIZE or EMSCRIPTEN_EVENT_CANVASRESIZED
  (void)theEvent;
  if (v3d_view_.IsNull())
  {
    return EM_FALSE;
  }

  Handle(Aspect_NeutralWindow) aWindow = Handle(Aspect_NeutralWindow)::DownCast(v3d_view_->Window());
  Graphic3d_Vec2i aWinSizeOld, aWinSizeNew(js_canvas_size());
  if (aWinSizeNew.x() < 10 || aWinSizeNew.y() < 10)
  {
    Message::DefaultMessenger()->Send(TCollection_AsciiString("Warning: invalid canvas size"), Message_Warning);
  }
  aWindow->Size(aWinSizeOld.x(), aWinSizeOld.y());
  const float aPixelRatio = js_device_pixel_ratio();
  if (aWinSizeNew != aWinSizeOld || aPixelRatio != device_pixel_ratio_)
  {
    if (device_pixel_ratio_ != aPixelRatio)
    {
      device_pixel_ratio_ = aPixelRatio;
      initPixelScaleRatio();
    }
    aWindow->SetSize(aWinSizeNew.x(), aWinSizeNew.y());
    v3d_view_->MustBeResized();
    v3d_view_->Invalidate();
    v3d_view_->Redraw();
    dumpGlInfo(true);
  }
  return EM_TRUE;
}

// ================================================================
// Function : onMouseEvent
// Purpose  :
// ================================================================
EM_BOOL OcctView::onMouseEvent(int theEventType, const EmscriptenMouseEvent *theEvent)
{
  if (v3d_view_.IsNull())
  {
    return EM_FALSE;
  }

  Graphic3d_Vec2i aWinSize;
  v3d_view_->Window()->Size(aWinSize.x(), aWinSize.y());
  const Graphic3d_Vec2i aNewPos = convertPointToBacking(Graphic3d_Vec2i(theEvent->targetX, theEvent->targetY));
  Aspect_VKeyFlags aFlags = 0;
  if (theEvent->ctrlKey == EM_TRUE)
  {
    aFlags |= Aspect_VKeyFlags_CTRL;
  }
  if (theEvent->shiftKey == EM_TRUE)
  {
    aFlags |= Aspect_VKeyFlags_SHIFT;
  }
  if (theEvent->altKey == EM_TRUE)
  {
    aFlags |= Aspect_VKeyFlags_ALT;
  }
  if (theEvent->metaKey == EM_TRUE)
  {
    aFlags |= Aspect_VKeyFlags_META;
  }

  const bool isEmulated = false;
  const Aspect_VKeyMouse aButtons = virtual_keys_mouse_buttons_from_native(theEvent->buttons);
  switch (theEventType)
  {
  case EMSCRIPTEN_EVENT_MOUSEMOVE:
  {
    if ((aNewPos.x() < 0 || aNewPos.x() > aWinSize.x() || aNewPos.y() < 0 || aNewPos.y() > aWinSize.y()) && PressedMouseButtons() == Aspect_VKeyMouse_NONE)
    {
      return EM_FALSE;
    }
    if (UpdateMousePosition(aNewPos, aButtons, aFlags, isEmulated))
    {
      updateView();
    }
    break;
  }
  case EMSCRIPTEN_EVENT_MOUSEDOWN:
  case EMSCRIPTEN_EVENT_MOUSEUP:
  {
    if (aNewPos.x() < 0 || aNewPos.x() > aWinSize.x() || aNewPos.y() < 0 || aNewPos.y() > aWinSize.y())
    {
      return EM_FALSE;
    }
    if (UpdateMouseButtons(aNewPos, aButtons, aFlags, isEmulated))
    {
      updateView();
    }
    break;
  }
  case EMSCRIPTEN_EVENT_CLICK:
  case EMSCRIPTEN_EVENT_DBLCLICK:
  {
    if (aNewPos.x() < 0 || aNewPos.x() > aWinSize.x() || aNewPos.y() < 0 || aNewPos.y() > aWinSize.y())
    {
      return EM_FALSE;
    }
    break;
  }
  case EMSCRIPTEN_EVENT_MOUSEENTER:
  {
    break;
  }
  case EMSCRIPTEN_EVENT_MOUSELEAVE:
  {
    // there is no SetCapture() support, so that mouse unclick events outside canvas will not arrive,
    // so we have to forget current state...
    if (UpdateMouseButtons(aNewPos, Aspect_VKeyMouse_NONE, aFlags, isEmulated))
    {
      updateView();
    }
    break;
  }
  }
  return EM_TRUE;
}

// ================================================================
// Function : onWheelEvent
// Purpose  :
// ================================================================
EM_BOOL OcctView::onWheelEvent(int theEventType, const EmscriptenWheelEvent *theEvent)
{
  if (v3d_view_.IsNull() || theEventType != EMSCRIPTEN_EVENT_WHEEL)
  {
    return EM_FALSE;
  }

  Graphic3d_Vec2i aWinSize;
  v3d_view_->Window()->Size(aWinSize.x(), aWinSize.y());
  const Graphic3d_Vec2i aNewPos = convertPointToBacking(Graphic3d_Vec2i(theEvent->mouse.targetX, theEvent->mouse.targetY));
  if (aNewPos.x() < 0 || aNewPos.x() > aWinSize.x() || aNewPos.y() < 0 || aNewPos.y() > aWinSize.y())
  {
    return EM_FALSE;
  }

  double aDelta = 0.0;
  switch (theEvent->deltaMode)
  {
  case DOM_DELTA_PIXEL:
  {
    aDelta = theEvent->deltaY / (5.0 * device_pixel_ratio_);
    break;
  }
  case DOM_DELTA_LINE:
  {
    aDelta = theEvent->deltaY * 8.0;
    break;
  }
  case DOM_DELTA_PAGE:
  {
    aDelta = theEvent->deltaY >= 0.0 ? 24.0 : -24.0;
    break;
  }
  }

  if (UpdateZoom(Aspect_ScrollDelta(aNewPos, -aDelta)))
  {
    updateView();
  }
  return EM_TRUE;
}

// ================================================================
// Function : onTouchEvent
// Purpose  :
// ================================================================
EM_BOOL OcctView::onTouchEvent(int theEventType, const EmscriptenTouchEvent *theEvent)
{
  const double aClickTolerance = 5.0;
  if (v3d_view_.IsNull())
  {
    return EM_FALSE;
  }

  Graphic3d_Vec2i aWinSize;
  v3d_view_->Window()->Size(aWinSize.x(), aWinSize.y());
  bool hasUpdates = false;
  for (int aTouchIter = 0; aTouchIter < theEvent->numTouches; ++aTouchIter)
  {
    const EmscriptenTouchPoint &aTouch = theEvent->touches[aTouchIter];
    if (!aTouch.isChanged)
    {
      continue;
    }

    const Standard_Size aTouchId = (Standard_Size)aTouch.identifier;
    const Graphic3d_Vec2i aNewPos = convertPointToBacking(Graphic3d_Vec2i(aTouch.canvasX, aTouch.canvasY));
    switch (theEventType)
    {
    case EMSCRIPTEN_EVENT_TOUCHSTART:
    {
      if (aNewPos.x() >= 0 && aNewPos.x() < aWinSize.x() && aNewPos.y() >= 0 && aNewPos.y() < aWinSize.y())
      {
        hasUpdates = true;
        AddTouchPoint(aTouchId, Graphic3d_Vec2d(aNewPos));
        click_touch_.From.SetValues(-1.0, -1.0);
        if (myTouchPoints.Extent() == 1)
        {
          click_touch_.From = Graphic3d_Vec2d(aNewPos);
        }
      }
      break;
    }
    case EMSCRIPTEN_EVENT_TOUCHMOVE:
    {
      const int anOldIndex = myTouchPoints.FindIndex(aTouchId);
      if (anOldIndex != 0)
      {
        hasUpdates = true;
        UpdateTouchPoint(aTouchId, Graphic3d_Vec2d(aNewPos));
        if (myTouchPoints.Extent() == 1 && (click_touch_.From - Graphic3d_Vec2d(aNewPos)).cwiseAbs().maxComp() > aClickTolerance)
        {
          click_touch_.From.SetValues(-1.0, -1.0);
        }
      }
      break;
    }
    case EMSCRIPTEN_EVENT_TOUCHEND:
    case EMSCRIPTEN_EVENT_TOUCHCANCEL:
    {
      if (RemoveTouchPoint(aTouchId))
      {
        if (myTouchPoints.IsEmpty() && click_touch_.From.minComp() >= 0.0)
        {
          if (double_tap_timer_.IsStarted() && double_tap_timer_.ElapsedTime() <= myMouseDoubleClickInt)
          {
            v3d_view_->FitAll(0.01, false);
            v3d_view_->Invalidate();
          }
          else
          {
            double_tap_timer_.Stop();
            double_tap_timer_.Reset();
            double_tap_timer_.Start();
            SelectInViewer(Graphic3d_Vec2i(click_touch_.From), false);
          }
        }
        hasUpdates = true;
      }
      break;
    }
    }
  }
  if (hasUpdates)
  {
    updateView();
  }
  return hasUpdates || !myTouchPoints.IsEmpty() ? EM_TRUE : EM_FALSE;
}

// ================================================================
// Function : onKeyDownEvent
// Purpose  :
// ================================================================
EM_BOOL OcctView::onKeyDownEvent(int theEventType, const EmscriptenKeyboardEvent *theEvent)
{
  if (v3d_view_.IsNull() || theEventType != EMSCRIPTEN_EVENT_KEYDOWN) // EMSCRIPTEN_EVENT_KEYPRESS
  {
    return EM_FALSE;
  }

  const double aTimeStamp = EventTime();
  const Aspect_VKey aVKey = virtual_keys_virtual_key_from_native(theEvent->keyCode);
  if (aVKey == Aspect_VKey_UNKNOWN)
  {
    return EM_FALSE;
  }

  if (theEvent->repeat == EM_FALSE)
  {
    myKeys.KeyDown(aVKey, aTimeStamp);
  }

  if (Aspect_VKey2Modifier(aVKey) == 0)
  {
    // normal key
  }
  return EM_FALSE;
}

// ================================================================
// Function : onKeyUpEvent
// Purpose  :
// ================================================================
EM_BOOL OcctView::onKeyUpEvent(int theEventType, const EmscriptenKeyboardEvent *theEvent)
{
  if (v3d_view_.IsNull() || theEventType != EMSCRIPTEN_EVENT_KEYUP)
  {
    return EM_FALSE;
  }

  const double aTimeStamp = EventTime();
  const Aspect_VKey aVKey = virtual_keys_virtual_key_from_native(theEvent->keyCode);
  if (aVKey == Aspect_VKey_UNKNOWN)
  {
    return EM_FALSE;
  }

  if (theEvent->repeat == EM_TRUE)
  {
    return EM_FALSE;
  }

  const unsigned int aModif = myKeys.Modifiers();
  myKeys.KeyUp(aVKey, aTimeStamp);
  if (Aspect_VKey2Modifier(aVKey) == 0)
  {
    // normal key released
    switch (aVKey | aModif)
    {
    case Aspect_VKey_F:
    {
      v3d_view_->FitAll(0.01, false);
      v3d_view_->Invalidate();
      updateView();
      return EM_TRUE;
    }
    }
  }
  return EM_FALSE;
}
