/*
  MIT License

  Copyright (c) 2026 Stefan Mader

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#include "MENUBAR.HPP"
#include "CONFIG.HPP"
#include "GL.H"
#include "IMGUIGL.HPP"
#include "SVG.HPP"
#include "LAYERS.HPP"
#include "COLORFLD.HPP"
#include "STL/DOS.HPP"
#include "DIALOGS.HPP"
#include <math.h>

int menuBar_height = 17;

float mouseSensitivity = 3;
void disableRenderMouseClicks();

String hex2_(int v) {
  const char *h="0123456789ABCDEF";
  String r;
  r.resize(2);
  r[0] = h[v / 16];
  r[1] = h[v & 15];
  return r;
}

String timeDescString() {
  int rtc_seconds = rtcRead(0);
  int rtc_minutes = rtcRead(2);
  int rtc_hours   = rtcRead(4);
  int rtc_weekday = rtcRead(6);
  int rtc_day     = rtcRead(7);
  int rtc_month   = rtcRead(8);
  int rtc_year    = rtcRead(9);
  const char *day[8] ={"Mon","Tue","Wed","Thu","Fri","Sat","Sun","Mon"};
  return hex2_(rtc_day)+"."+hex2_(rtc_month)+"."+hex2_(rtc_year)+" "+String(day[(rtc_weekday-2)&7])+" "+hex2_(rtc_hours)+":"+hex2_(rtc_minutes);
}

void setMouseSensitivity() {
  mouseSpeed = pow(1.1,mouseSensitivity)-0.1-0.75;
}

void initMenuBar() {
  setMouseSensitivity();
}

void drawMenuBar() {
  bool on = false;
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10,1));
  ImGui::PushStyleColor(ImGuiCol_MenuBarBg,ImVec4(0.f,0.f,0.f,1.f));
  ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1.f,1.f,1.f,1.f));
  ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0.f,0.f,0.f,1.f));
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered,ImVec4(0.1,0.1,0.1,1));
  double canvasPixelCenterX = currentCanvas->width/2 + currentCanvas->screenPosX;
  double canvasPixelCenterY = currentCanvas->height/2 + currentCanvas->screenPosY;
  if (ImGui::BeginMainMenuBar()) {
    SVG *svg = &svgs["appicon"];
    ImGui::Image((ImTextureID)svg->glHandle,ImVec2(svg->w,svg->h));
    ImGui::SameLine();
    if (ImGui::BeginMenu("File")) {
      on = true;
      if (ImGui::MenuItem("New image")) {dialogNewImage(true);}
      ImGui::Separator();
      if (ImGui::MenuItem("Load image")) {dialogLoadSave(true,DIALOG_LOADIMAGE);}
      if (ImGui::MenuItem("Insert image layer(s)")) {dialogLoadSave(true,DIALOG_INSERTIMAGELAYERS);}
      if (ImGui::MenuItem("Insert psd layers (experimental)")) {dialogLoadSave(true,DIALOG_INSERTPSD);}
      ImGui::Separator();
      if (ImGui::MenuItem("Export png")) {dialogLoadSave(true,DIALOG_EXPORTPNG);}
      if (ImGui::MenuItem("Save selected layer(s) as single png")) {dialogLoadSave(true,DIALOG_EXPORTLAYERS);}
      if (ImGui::MenuItem("Save selected layer(s) as multiple pngs")) {dialogLoadSave(true,DIALOG_EXPORTLAYERSRAW);}
      ImGui::Separator();
      if (ImGui::MenuItem("Exit")) {}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      on = true;
      if (ImGui::MenuItem("Fill with foreground color",NULL,false,currentCanvas->hasSelection())) {currentCanvas->fillSelectionWithColor(getUIntColor(foregroundColor));}
      if (ImGui::MenuItem("Erase selection",NULL,false,currentCanvas->hasSelection())) {currentCanvas->fillSelectionWithColor(0x00000000);}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Image")) {
      on = true;
      if (ImGui::MenuItem("Mirror horizontally")) {currentCanvas->mirrorHorizontally();}
      if (ImGui::MenuItem("Mirror vertically")) {currentCanvas->mirrorVertically();}
      if (ImGui::MenuItem("Rotate +90 degrees")) {currentCanvas->rotate90DegreesForward();}
      if (ImGui::MenuItem("Rotate -90 degrees")) {currentCanvas->rotate90DegreesBackward();}
      ImGui::Separator();
      if (ImGui::MenuItem("Configure canvas")) {dialogCanvas(true);}
      if (ImGui::MenuItem("Resize image")) {dialogImageResize(true);}
      ImGui::Separator();
      if (ImGui::MenuItem("Fit canvas to layers")) {currentCanvas->fitCanvasToLayers();}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Layer")) {
      on = true;
      bool one = false;
      for (int i = 0; i < currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected || currentCanvas->layers[i]->state.active) one = true;
      if (ImGui::MenuItem("Add layer")) {dialogAddLayer(true);}
      if (ImGui::MenuItem("Delete layer(s)")) {dialogDeleteLayers(true);}
      ImGui::Separator();
      if (ImGui::MenuItem("Hide all layers")) {currentCanvas->showAllLayers(false);}
      if (ImGui::MenuItem("Show all layers")) {currentCanvas->showAllLayers(true);}
      if (ImGui::MenuItem("Select all layers")) {currentCanvas->selectAllLayers();}
      if (ImGui::MenuItem("Duplicate layer(s)",NULL,false,one)) {currentCanvas->duplicateLayers();}
      ImGui::Separator();
      if (ImGui::MenuItem("Copy layer(s)",NULL,false,one)) {currentCanvas->copy();}
      if (ImGui::MenuItem("Paste",NULL,false,copyLayer != NULL)) {currentCanvas->paste();}
      if (ImGui::MenuItem("Bake layer(s)")) {dialogBakeLayers(true);}
      ImGui::Separator();
      if (ImGui::MenuItem("Mirror horizontally",NULL,false,one)) {currentCanvas->mirrorHorizontallyLayers();}
      if (ImGui::MenuItem("Mirror vertically",NULL,false,one)) {currentCanvas->mirrorVerticallyLayers();}
      if (ImGui::MenuItem("Rotate +90 degrees",NULL,false,one)) {currentCanvas->rotate90DegreesForwardLayers();}
      if (ImGui::MenuItem("Rotate -90 degrees",NULL,false,one)) {currentCanvas->rotate90DegreesBackwardLayers();}
      ImGui::Separator();
      if (ImGui::MenuItem("Update layer",NULL,false,currentCanvas->getActiveLayer()!=NULL)) {dialogUpdateLayer(true);}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Selection")) {
      on = true;
      if (ImGui::MenuItem("Select all")) {currentCanvas->selectAll();}
      if (ImGui::MenuItem("Select none")) {currentCanvas->selectNone(true);}
      if (ImGui::MenuItem("Invert selection")) {currentCanvas->invertSelection();}
      ImGui::Separator();
      if (ImGui::MenuItem("Expand/shrink selection")) {dialogExpandShrinkSelection(true);}
      if (ImGui::MenuItem("Select by alpha")) {dialogSelectByAlpha(true);}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      on = true;
      bool selected;
      selected = aspectRatio == 16.0/9.0; if (ImGui::MenuItem("Aspect ratio 16:9","",&selected)) {aspectRatio = selected?16.0/9.0:(double)glFrameBufferWidth/glFrameBufferHeight; reloadIcons(); shouldUpdateAll = true;}
      if (ImGui::MenuItem("Zoom in (+)")) {currentCanvas->control.zoomBy(currentCanvas,canvasPixelCenterX,canvasPixelCenterY,zoomStep);}
      if (ImGui::MenuItem("Zoom out (-)")) {currentCanvas->control.zoomBy(currentCanvas,canvasPixelCenterX,canvasPixelCenterY,1.f/zoomStep);}
      ImGui::Separator();
      if (ImGui::MenuItem("Show grid",NULL,currentCanvas->gridOn)) {currentCanvas->gridOn = !currentCanvas->gridOn; shouldUpdateAll = true;}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Filter")) {
      on = true;
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Pictures")) {
      on = true;
      for (int i = 0; i < canvas.size(); i++) {
        bool isIt = canvas[i] == currentCanvas;
        if (ImGui::MenuItem((String(canvas[i]->pictureName)+String("##MENUBARPICTURES")+String::fromInt(i)).c_str(),NULL,isIt)) {
          currentCanvas = canvas[i];
          shouldUpdateAll = true;
        }
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Delete picture")) {dialogDeletePicture(true);}
      if (ImGui::MenuItem("Insert layers from other picture")) {dialogGetLayersFromPicture(true);}
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("About")) {dialogAbout(true);}
      on = true;
      ImGui::EndMenu();
    }

    ImGui::SameLine();
    String timeDesc = timeDescString();
    ImVec2 cursorScreen = ImGui::GetCursorScreenPos();
    cursorScreen.x = main_window_xres - timeDesc.length()*7;
    ImGui::SetCursorScreenPos(cursorScreen);
    ImGui::Text("%s",timeDesc.c_str());


    cursorScreen.x = main_window_xres - timeDesc.length()*7 - 150;
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(cursorScreen);
    ImGui::Text("Mouse rate");
    ImGui::SameLine();
    ImGui::PushItemWidth(50);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0.125,0.125,0.125,1));
    if (ImGui::SliderFloat("##DRAWMENUBARSLIDERMOUSE",&mouseSensitivity,1,5)) {
      if (mouseSensitivity < 1) mouseSensitivity = 1;
      if (mouseSensitivity > 5) mouseSensitivity = 5;
      setMouseSensitivity();
    }
    ImGui::PopStyleColor();
    ImGui::PopItemWidth();


    cursorScreen.x = main_window_xres - timeDesc.length()*7 - 150 - 110;
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(cursorScreen);
    ImGui::Text("Zoom");
    ImGui::SameLine();
    ImGui::PushItemWidth(50);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0.125,0.125,0.125,1));
    float zoom = currentCanvas->control.zoom;
    if (ImGui::InputFloat("##DRAWMENUBARZOOM",&zoom,0,0,2)) {
      if (zoom < 0.01) zoom = 0.01;
      currentCanvas->control.zoomTo(currentCanvas,canvasPixelCenterX,canvasPixelCenterY,zoom);
    }
    ImGui::PopStyleColor();
    ImGui::PopItemWidth();

    ImGui::EndMainMenuBar();
  }
  ImGui::PopStyleColor();
  ImGui::PopStyleColor();
  ImGui::PopStyleColor();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
  if (on) {
    disableRenderMouseClicks();
  }
}
