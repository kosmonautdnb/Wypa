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
#include "STL/GL.H"
#include "IMGUI/IMGUIGL.HPP"
#include "SVG.HPP"
#include "LAYERS.HPP"
#include "COLORFLD.HPP"
#include "STL/DOS.HPP"
#include "DIALOGS.HPP"
#include "UNDO.HPP"
#include "TOOLS.HPP"
#include "TEXT.HPP"
#include "TOOLBAR.HPP"
#include "FILTER.HPP"
#include "HOTKEYS.HPP"
#include <math.h>

int menuBar_height = 17;
bool undoBlink = false;

float mouseSensitivity = 3;

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
  const char *day[8] ={"Mon","Tue","Wed","Thu","Fri","Sat","Sun","Sun"};
  return hex2_(rtc_day)+"."+hex2_(rtc_month)+"."+hex2_(rtc_year)+" "+String(day[(rtc_weekday-2)&7])+" "+hex2_(rtc_hours)+":"+hex2_(rtc_minutes);
}

void setMouseSensitivity() {
  mouseSpeed = pow(1.1,mouseSensitivity)-0.1-0.75;
}

void initMenuBar() {
  setMouseSensitivity();
}

void freeMenuBar() {
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
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImGui::Image((ImTextureID)0,ImVec2(svg->w,svg->h));
    cursor.y -= 4;
    ImGui::GetWindowDrawList()->AddImage((ImTextureID)svg->glHandle,ImVec2(cursor.x+0,cursor.y+0),ImVec2(cursor.x+svg->w,cursor.y+svg->h),ImVec2(0,0),ImVec2(1,1));
    ImGui::SameLine();
    if (ImGui::BeginMenu("File")) {
      on = true;
      if (ImGui::MenuItem("New image")) {dialogNewImage(true);}
      ImGui::Separator();
      if (ImGui::MenuItem("Load wyp image")) {dialogLoadSave(true,DIALOG_LOADIMAGE);}
      if (ImGui::MenuItem("Save wyp image")) {dialogLoadSave(true,DIALOG_SAVEIMAGE);}
      ImGui::Separator();
      if (ImGui::MenuItem("Import image")) {dialogLoadSave(true,DIALOG_IMPORTIMAGE);}
      ImGui::Separator();
      if (ImGui::MenuItem("Insert image layer(s)")) {dialogLoadSave(true,DIALOG_INSERTIMAGELAYERS);}
      if (ImGui::MenuItem("Insert psd layers (experimental)")) {dialogLoadSave(true,DIALOG_INSERTPSD);}
      if (ImGui::MenuItem("Insert svg layer (experimental)")) {dialogLoadSave(true,DIALOG_INSERTSVG);}
      ImGui::Separator();
      if (ImGui::MenuItem("Export png")) {dialogLoadSave(true,DIALOG_EXPORTPNG);}
      if (ImGui::MenuItem("Save selected layer(s) as single png")) {dialogLoadSave(true,DIALOG_EXPORTLAYERS);}
      if (ImGui::MenuItem("Save selected layer(s) as multiple pngs")) {dialogLoadSave(true,DIALOG_EXPORTLAYERSRAW);}
      ImGui::Separator();
      if (ImGui::MenuItem("Exit")) {dialogExit(true);}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      bool hasSelection = currentCanvas->hasSelection();
      on = true;
      bool one = false;
      int count = 0;
      for (int i = 0; i < currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected || currentCanvas->layers[i]->state.active) {one = true; count++;}
      if (useUndo) {
        if (ImGui::MenuItem(combined(String("Undo[") + String::fromInt(availableUndos) + String("]##UNDO")).c_str(),NULL,false,availableUndos>0)) {undo();}
        if (ImGui::MenuItem("Redo",NULL,false,availableRedos>0)) {redo();}
        ImGui::Separator();
      }
      if (ImGui::MenuItem("Fill selection with color",NULL,false,hasSelection)) {beforeUndoPoint();currentCanvas->fillSelectionWithColor(getUIntColor(foregroundColor));afterUndoPoint();}
      if (ImGui::MenuItem("Fill selection with alpha",NULL,false,hasSelection)) {beforeUndoPoint();currentCanvas->fillSelectionWithAlpha((unsigned char)(getUIntColor(foregroundColor)>>24));beforeUndoPoint();}
      if (ImGui::MenuItem("Erase selection",NULL,false,hasSelection)) {beforeUndoPoint();currentCanvas->fillSelectionWithColor(0x00000000);afterUndoPoint();}
      ImGui::Separator();
      if (ImGui::MenuItem("Antialias selected",NULL,false,hasSelection)) {beforeUndoPoint();currentCanvas->antialiasSelected();afterUndoPoint();}
      ImGui::Separator();
      if (ImGui::MenuItem("Greyscale",NULL,false,count == 1)) {beforeUndoPoint();currentCanvas->greyScale();afterUndoPoint();}
      if (ImGui::MenuItem("Gradiation curves",NULL,false,count == 1)) {dialogGradiationCurves(true);}
      if (ImGui::MenuItem("Color balance",NULL,false,count == 1)) {dialogColorBalance(true);}
      if (ImGui::MenuItem("Invert color",NULL,false,count>0)) {beforeUndoPoint();currentCanvas->invertColor();afterUndoPoint();}
      if (ImGui::MenuItem("Invert alpha",NULL,false,count>0)) {beforeUndoPoint();currentCanvas->invertAlpha();afterUndoPoint();}
      ImGui::Separator();
      if (ImGui::MenuItem("Undo enabled",NULL,useUndo,true)) {useUndo = !useUndo;}
      ImGui::EndMenu();
    } else {
      ImVec2 minB = ImGui::GetItemRectMin();
      ImVec2 maxB = ImGui::GetItemRectMax();
      maxB.x--;
      maxB.y--;
      if (undoBlink) {
        ImGui::GetWindowDrawList()->AddRectFilled(minB,maxB,0x08ffffff);
      }
      undoBlink = false;
    }
    if (ImGui::BeginMenu("Image")) {
      on = true;
      if (ImGui::MenuItem("Mirror horizontally")) {currentCanvas->mirrorHorizontally();}
      if (ImGui::MenuItem("Mirror vertically")) {currentCanvas->mirrorVertically();}
      if (ImGui::MenuItem("Rotate +90 degrees")) {currentCanvas->rotate90DegreesForward();}
      if (ImGui::MenuItem("Rotate -90 degrees")) {currentCanvas->rotate90DegreesBackward();}
      ImGui::Separator();
      if (ImGui::MenuItem("Image properties")) {dialogCanvas(true);}
      if (ImGui::MenuItem("Resize image")) {dialogImageResize(true);}
      ImGui::Separator();
      if (ImGui::MenuItem("Fit canvas to layers")) {currentCanvas->fitCanvasToLayers();}
      if (ImGui::MenuItem("Crop canvas by alpha")) {currentCanvas->cropCanvasByAlpha(1);}
      if (ImGui::MenuItem("Fit canvas to selection")) {currentCanvas->fitCanvasToSelection();}
      ImGui::Separator();
      if (ImGui::MenuItem("Duplicate picture")) {duplicatePicture(false);}
      if (ImGui::MenuItem("Duplicate picture (merged)")) {duplicatePicture(true);}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Layer")) {
      on = true;
      bool one = false;
      int count = 0;
      for (int i = 0; i < currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected || currentCanvas->layers[i]->state.active) {one = true; count++;}
      if (ImGui::MenuItem("Add layer")) {dialogAddLayer(true);}
      if (ImGui::MenuItem("Insert layer like this layer",NULL,false,count == 1)) {currentCanvas->insertLayerLikeThisLayer();}
      if (ImGui::MenuItem("Delete layer(s)",NULL,false,one)) {dialogDeleteLayers(true);}
      ImGui::Separator();
      if (ImGui::MenuItem("Insert text layer")) {currentCanvas->insertTextLayer(); switchTool(TOOL_TEXT);}
      if (ImGui::MenuItem("Insert adjustment layer")) {currentCanvas->insertAdjustmentLayer(); }
      ImGui::Separator();
      if (ImGui::MenuItem("Hide all layers")) {currentCanvas->showAllLayers(false);}
      if (ImGui::MenuItem("Show all layers")) {currentCanvas->showAllLayers(true);}
      if (ImGui::MenuItem("Select all layers")) {currentCanvas->selectAllLayers();}
      if (ImGui::MenuItem("Duplicate layer(s)",NULL,false,one)) {dialogDuplicate(true);}
      ImGui::Separator();
      if (ImGui::MenuItem("Copy layer(s)",NULL,false,one)) {currentCanvas->copy();}
      if (ImGui::MenuItem("Paste",NULL,false,copyLayer != NULL)) {currentCanvas->paste();}
      if (ImGui::MenuItem("Bake layer(s)",NULL,false,one)) {dialogBakeLayers(true);}
      ImGui::Separator();
      if (ImGui::MenuItem("Merge down layer(s)",NULL,false,count > 1)) {dialogMergeDown(true);}
      if (ImGui::MenuItem("Insert merge of layer(s)",NULL,false,count > 0)) {currentCanvas->insertMergeOfLayers();}
      if (ImGui::MenuItem("Insert alpha mask above layer(s)",NULL,false,count > 0)) {currentCanvas->insertAlphaMasks();}
      if (ImGui::MenuItem("Apply alpha mask to below",NULL,false,count==2)) {currentCanvas->applyAlphaMaskToBelow();}
      ImGui::Separator();
      if (ImGui::MenuItem("Quantized palette from layer",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogQuantizePalette(true);}
      if (ImGui::MenuItem("Quantize layer(s) to palette",NULL,false,one)) {beforeUndoPoint();currentCanvas->quantizeLayers();afterUndoPoint();}
      ImGui::Separator();
      if (ImGui::MenuItem("Mirror horizontally",NULL,false,one)) {currentCanvas->mirrorHorizontallyLayers();}
      if (ImGui::MenuItem("Mirror vertically",NULL,false,one)) {currentCanvas->mirrorVerticallyLayers();}
      if (ImGui::MenuItem("Rotate +90 degrees",NULL,false,one)) {currentCanvas->rotate90DegreesForwardLayers();}
      if (ImGui::MenuItem("Rotate -90 degrees",NULL,false,one)) {currentCanvas->rotate90DegreesBackwardLayers();}
      ImGui::Separator();
      if (ImGui::MenuItem("Layer properties",NULL,false,currentCanvas->getActiveLayer()!=NULL)) {dialogUpdateLayer(true);}
      if (ImGui::MenuItem("Crop layer(s) by alpha",NULL,false,one)) {currentCanvas->cropLayersByAlpha(1);}
      ImGui::Separator();
      if (ImGui::MenuItem("Lock layer(s)",NULL,false,one)) {currentCanvas->lockLayers(true);}
      if (ImGui::MenuItem("Unlock layer(s)",NULL,false,one)) {currentCanvas->lockLayers(false);}
      if (ImGui::MenuItem("Duplicate layer as reference",NULL,false,currentCanvas->getActiveLayer()!=NULL)) {currentCanvas->duplicateLayerAsReference(currentCanvas->getActiveLayer());}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Selection")) {
      on = true;
      bool one = false;
      int count = 0;
      for (int i = 0; i < currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected || currentCanvas->layers[i]->state.active) {one = true; count++;}
      if (ImGui::MenuItem("Select all")) {beforeUndoPoint();currentCanvas->selectAll();afterUndoPoint();}
      if (ImGui::MenuItem("Select none")) {beforeUndoPoint();currentCanvas->selectNone(true);afterUndoPoint();}
      if (ImGui::MenuItem("Invert selection")) {beforeUndoPoint();currentCanvas->invertSelection();afterUndoPoint();}
      ImGui::Separator();
      if (ImGui::MenuItem("Expand/shrink selection")) {dialogExpandShrinkSelection(true);}
      if (ImGui::MenuItem("Select by alpha")) {dialogSelectByAlpha(true);}
      if (ImGui::MenuItem("Selection as outline")) {beforeUndoPoint(); currentCanvas->selectionAsOutline(); afterUndoPoint();}
      if (ImGui::MenuItem("Extrude selection")) {dialogExtrudeSelection(true);}
      ImGui::Separator();
      if (ImGui::MenuItem("Apply selections to layer",NULL,false,count>1&&(currentCanvas->getActiveLayer()!=NULL))) {currentCanvas->getLayerSelectionFromLayers(currentCanvas->getActiveLayer());}
      if (ImGui::MenuItem("Clear all other layer selections",NULL,false,(currentCanvas->getActiveLayer()!=NULL))) {currentCanvas->clearAllOtherLayerSelections(currentCanvas->getActiveLayer());}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      on = true;
      bool selected;
      selected = aspectRatio == 16.0/9.0; if (ImGui::MenuItem("Aspect ratio 16:9","",&selected)) {aspectRatio = selected?16.0/9.0:(double)glFrameBufferWidth/glFrameBufferHeight; reloadIcons(); reScaleTexts(); shouldUpdateAll = true;}
      if (ImGui::MenuItem("Zoom in (+)")) {currentCanvas->control.zoomBy(currentCanvas,canvasPixelCenterX,canvasPixelCenterY,zoomStep);}
      if (ImGui::MenuItem("Zoom out (-)")) {currentCanvas->control.zoomBy(currentCanvas,canvasPixelCenterX,canvasPixelCenterY,1.f/zoomStep);}
      ImGui::Separator();
      if (ImGui::MenuItem("Show layer shapes",NULL,currentCanvas->layerShapesOn)) {currentCanvas->layerShapesOn = !currentCanvas->layerShapesOn; shouldUpdateAll = true;}
      ImGui::Separator();
      if (ImGui::MenuItem("Show grid",NULL,currentCanvas->gridOn)) {currentCanvas->gridOn = !currentCanvas->gridOn; shouldUpdateAll = true;}
      if (ImGui::MenuItem("Center view")) {currentCanvas->centerCanvas(); shouldUpdateAll = true;}
      if (ImGui::MenuItem("Fit to view")) {currentCanvas->fitCanvas(); shouldUpdateAll = true;}
      ImGui::Separator();
      if (ImGui::MenuItem("Clear mouse lock",NULL,false,lockedMouseX||lockedMouseY)) {lockedMouseX = false; lockedMouseY = false;}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Filter")) {
      on = true;
      on = true;
      bool one = false;
      int count = 0;
      for (int i = 0; i < currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected || currentCanvas->layers[i]->state.active) {one = true; count++;}
      if (ImGui::MenuItem("Difference clouds",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterDifferenceClouds();}
      if (ImGui::MenuItem("Edge detect",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterMorphologicalEdgeDetect();}
      if (ImGui::MenuItem("Blur",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogBlur(true);}
      if (ImGui::MenuItem("Edge soften",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterAlphaSoften();}
      if (ImGui::MenuItem("Turn to glow",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogGlow(true);}
      if (ImGui::MenuItem("Emboss",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogEmboss(true);}
      if (ImGui::MenuItem("Keying",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogKeying(true);}
      if (ImGui::MenuItem("Denoise",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterDenoise();}
      if (ImGui::MenuItem("Sharpen",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterSharpen();}
      ImGui::EndMenu();
    }

    bool hasUpdates = false;
    for (int i = 0; i < canvas.size(); i++) {
      if (canvas[i]->hasUpdates()) {hasUpdates = true; break;}
    }

    if (ImGui::BeginMenu(combined(String(hasUpdates?".":"")+String("Pictures[")+String::fromInt(canvas.size()) + "]##MENUBARPICTURES").c_str())) {
      on = true;
      for (int i = 0; i < canvas.size(); i++) {
        bool isIt = canvas[i] == currentCanvas;
        if (ImGui::MenuItem(combined(String(canvas[i]->hasUpdates()?".":" ")+String(canvas[i]->pictureName)+String("##MENUBARPICTURES")+String::fromInt(i)).c_str(),NULL,isIt)) {
          currentCanvas = canvas[i];
          shouldUpdateAll = true;
        }
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Delete picture")) {dialogDeletePicture(true);}
      if (ImGui::MenuItem("Picture info")) {dialogPictureInfo(true);}
      if (ImGui::MenuItem("Insert layers from other picture")) {dialogGetLayersFromPicture(true);}
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("About")) {dialogAbout(true);}
      if (ImGui::MenuItem("Hotkeys")) {dialogHotkeys(true);}
      ImGui::Separator();
      if (ImGui::MenuItem("Show debug window",NULL,debugWindowOn,true)) {debugWindowOn = !debugWindowOn;}
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
    disableNativeInput();
  }
}
