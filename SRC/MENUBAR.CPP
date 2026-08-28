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
#define _CRT_SECURE_NO_WARNINGS
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
#include "STL/IMAGE.HPP"
#include "GALLERY.HPP"
#include "LAYERV.HPP"
#include <math.h>

int menuBar_height = 17;
unsigned int undoBlink = 0;
unsigned int selectionBlink = 0;
unsigned int picturesBlink = 0;

float mouseSensitivity = 3;

String hex2_(int v) {
  const char *h="0123456789ABCDEF";
  String r;
  r.resize(2);
  r[0] = h[v / 16];
  r[1] = h[v & 15];
  return r;
}

void dosShell();

String timeDescString() {
  //int rtc_seconds = rtcRead(0);
  int rtc_minutes = rtcRead(2);
  int rtc_hours   = rtcRead(4);
  int rtc_weekday = rtcRead(6);
  int rtc_day     = rtcRead(7);
  int rtc_month   = rtcRead(8);
  int rtc_year    = rtcRead(9);
  const char *day[8] ={"Mon","Tue","Wed","Thu","Fri","Sat","Sun","Sun"};
  return hex2_(rtc_day)+"."+hex2_(rtc_month)+"."+hex2_(rtc_year)+" "+String(day[(rtc_weekday-2)&7])+" "+hex2_(rtc_hours)+":"+hex2_(rtc_minutes);
}

String extensiveTimeDescString() {
  int rtc_seconds = rtcRead(0);
  int rtc_minutes = rtcRead(2);
  int rtc_hours   = rtcRead(4);
  int rtc_weekday = rtcRead(6);
  int rtc_day     = rtcRead(7);
  int rtc_month   = rtcRead(8);
  int rtc_year    = rtcRead(9);
  const char *day[8] ={"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday","Sunday"};
  const char *months[12] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
  int k = ((rtc_month & 15)+(rtc_month/16*10)); if (k > 0) k--;
  return String(day[(rtc_weekday-2)&7]) + "," + hex2_(rtc_day)+"."+months[k % 12]+" 20"+hex2_(rtc_year)+","+hex2_(rtc_hours)+":"+hex2_(rtc_minutes) + ":" + hex2_(rtc_seconds);
}

void setMouseSensitivity() {
    mouseSpeed = pow(1.1,mouseSensitivity)-0.1-0.75;
#ifdef _WIN32
    mouseSpeed *= 8;
#endif // _WIN32
}

void initMenuBar() {
  setMouseSensitivity();
}

void freeMenuBar() {
}

void takeScreenShot() {
  RGBAImage img;
  img.data = glFrameBuffer;
  img.width = glFrameBufferWidth;
  img.height = glFrameBufferHeight;
  int newWidth = (int)(img.width * aspectRatio * main_window_yres/main_window_xres);
  img = img.getResized(newWidth,img.height);
  memset(glFrameBuffer,0xff,glFrameBufferWidth*glFrameBufferHeight*sizeof(unsigned int));
  if (refreshMasked) glRefreshMasked(); else glRefresh();
  static int c = -1; c++;
  img.savePNG(combined(String("SCN")+String::fromInt(c)+String(".PNG")).c_str());
  img.free();
}


void horzSep() {
  ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1.f,1.f,1.f,0.625f));
  ImGui::Text("|");
  ImGui::PopStyleColor();
}

void drawMenuBar() {
  static int screenShot = 0;
  if (screenShot > 0) {
    screenShot--;
    if (screenShot == 0) takeScreenShot();
  }
  bool on = false;
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10,1));
  ImGui::PushStyleColor(ImGuiCol_MenuBarBg,ImVec4(0.f,0.f,0.f,1.f));
  ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1.f,1.f,1.f,1.f));
  ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0.f,0.f,0.f,1.f));
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered,ImVec4(0.1f,0.1f,0.1f,1));
  double canvasPixelCenterX = currentCanvas->width/2 + currentCanvas->screenPosX;
  double canvasPixelCenterY = currentCanvas->height/2 + currentCanvas->screenPosY;
  if (ImGui::BeginMainMenuBar()) {
    SVG *svg = &svgs["appicon"];
    ImVec2 cursor = ImGui::GetCursorScreenPos(); 
    cursor.x -= 7-2+4;
    ImGui::SetCursorScreenPos(cursor); 
    ImVec2 cursor2 = cursor;
    ImGui::Image((ImTextureID)0,ImVec2((float)(svg->w+7*5),(float)svg->h),ImVec2(0,0),ImVec2(0,0),ImVec4(0,0,0,0));
    if (ImGui::IsItemHovered()&&ImGui::IsMouseClicked(0)) {dialogAbout(true);}
    cursor.y += 1;
    ImGui::GetWindowDrawList()->AddImage((ImTextureID)svg->glHandle,ImVec2(cursor.x+0,cursor.y+0),ImVec2(cursor.x+svg->w,cursor.y+svg->h),ImVec2(0,0),ImVec2(1,1));
    toolTip("Info: Wypa version %d.%02d", version / 100, version % 100);
    ImGui::SameLine();
    cursor2.x += svg->w+3;

    cursor2.x += 1-2;
    ImGui::SetCursorScreenPos(cursor2);
    ImGui::Text("WYPA"); ImGui::SameLine();
    cursor2.x += 7+2 + 7*4-7-1;
    ImGui::SameLine();

    ImGui::SetCursorScreenPos(cursor2);
    horzSep();
    ImGui::SameLine();
    cursor2.x += 4+2+2+1+2;
    ImGui::SetCursorScreenPos(cursor2);

    static bool loadedAnything = false; if (!loadedAnything) {for (int i1 = 0; i1 < (int)canvas.size(); i1++) {if (canvas[i1]->layers.size() > 0) loadedAnything = true;}}
    static double k = 0; k += timeDelta; k = fmod(k,2*100); int k2 = (int)(k*4);

    if (!loadedAnything) {
      if (ImGui::BeginMenu("File")) {
        on = true;
        if (ImGui::MenuItem("New image")) {dialogNewImage(true);}
        ImGui::Separator();
        if (ImGui::MenuItem("Load wyp image")) {dialogLoadSave(true,DIALOG_LOADIMAGE);}
        if (ImGui::MenuItem("Load any image as a single layer")) {dialogLoadSave(true,DIALOG_IMPORTIMAGE);}
        ImGui::Separator();
        if (ImGui::MenuItem("Import folder into single layer")) {dialogImportFolder(true);}
        ImGui::Separator();
        if (ImGui::MenuItem("Insert image layer(s)")) {dialogLoadSave(true,DIALOG_INSERTIMAGELAYERS);}
        if (ImGui::MenuItem("Insert psd layers (experimental)")) {dialogLoadSave(true,DIALOG_INSERTPSD);}
        if (ImGui::MenuItem("Insert svg layer (experimental)")) {dialogLoadSave(true,DIALOG_INSERTSVG);}
        ImGui::Separator();
        if (ImGui::MenuItem("Palette from (paletted) image")) {dialogLoadSave(true,DIALOG_PALETTEFROMPICTURE);}
        ImGui::Separator();
        if (ImGui::MenuItem("Captured mouse", "<F1>", captureMouse)) { captureMouse = !captureMouse; }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit Wypa")) {dialogExit(true);}
        ImGui::EndMenu();
      }
    } else {
      if (ImGui::BeginMenu("File")) {
        on = true;
        if (ImGui::MenuItem("New image")) {dialogNewImage(true);}
        ImGui::Separator();
        if (ImGui::MenuItem("Load wyp image")) {dialogLoadSave(true,DIALOG_LOADIMAGE);}
        if (ImGui::MenuItem("Save wyp image")) {dialogLoadSave(true,DIALOG_SAVEIMAGE);}
        if (ImGui::MenuItem("Load any image as a single layer")) {dialogLoadSave(true,DIALOG_IMPORTIMAGE);}
        ImGui::Separator();
        if (ImGui::MenuItem("Import folder into single layer")) {dialogImportFolder(true);}
        ImGui::Separator();
        if (ImGui::MenuItem("Insert image layer(s)")) {dialogLoadSave(true,DIALOG_INSERTIMAGELAYERS);}
        if (ImGui::MenuItem("Insert psd layers (experimental)")) {dialogLoadSave(true,DIALOG_INSERTPSD);}
        if (ImGui::MenuItem("Insert svg layer (experimental)")) {dialogLoadSave(true,DIALOG_INSERTSVG);}
        ImGui::Separator();
        if (ImGui::MenuItem("Export png")) {dialogLoadSave(true,DIALOG_EXPORTPNG);}
        if (ImGui::MenuItem("Save selected layer(s) as single png")) {dialogLoadSave(true,DIALOG_EXPORTLAYERS);}
        if (ImGui::MenuItem("Save selected layer(s) as multiple pngs")) {dialogLoadSave(true,DIALOG_EXPORTLAYERSRAW);}
        ImGui::Separator();
        if (palette.size()>0 && palette.size()<=256) {
          if (ImGui::MenuItem("Export tga (paletted)")) {dialogLoadSave(true,DIALOG_EXPORTTGA);}
        }
        if (ImGui::MenuItem("Palette from (paletted) image")) {dialogLoadSave(true,DIALOG_PALETTEFROMPICTURE);}
        ImGui::Separator();
        bool hasShell = false;
#ifdef __DJGPP__
        hasShell = true;
#endif // __DJGPP__
#ifdef __WATCOMC__
#if __WATCOMC__ >= 1200
        hasShell = true;
#endif
#endif // __DJGPP__
        if (hasShell && ImGui::MenuItem("Open a shell")) {dosShell();}
        if (ImGui::MenuItem("Save user interface screenshot")) {screenShot = 2;}
        if (ImGui::MenuItem("Captured mouse", "<F1>", captureMouse)) { captureMouse = !captureMouse; }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit Wypa")) {dialogExit(true);}
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Edit")) {
        bool hasSelection = currentCanvas->hasSelection();
        on = true;
        bool one = false;
        UNUSED(one);
        int count = 0;
        for (int i = 0; i < (int)currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected || currentCanvas->layers[i]->state.active) {one = true; count++;}
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
        if (ImGui::MenuItem("Normalize",NULL,false,count == 1)) {beforeUndoPoint();currentCanvas->normalize();afterUndoPoint();}
        if (ImGui::MenuItem("Greyscale",NULL,false,count == 1)) {beforeUndoPoint();currentCanvas->greyScale();afterUndoPoint();}
        if (ImGui::MenuItem("Color matrix",NULL,false,count == 1)) {dialogColorMatrixDirect(true);}
        if (ImGui::MenuItem("Gradiation curves",NULL,false,count == 1)) {dialogGradiationCurves(true);}
        if (ImGui::MenuItem("Color balance",NULL,false,count == 1)) {dialogColorBalance(true);}
        if (ImGui::MenuItem("Invert color",NULL,false,count>0)) {beforeUndoPoint();currentCanvas->invertColor();afterUndoPoint();}
        if (ImGui::MenuItem("Invert alpha",NULL,false,count>0)) {beforeUndoPoint();currentCanvas->invertAlpha();afterUndoPoint();}
        if (ImGui::MenuItem("Simple white balance",NULL,false,count == 1)) {beforeUndoPoint();currentCanvas->whiteBalance();afterUndoPoint();}
        ImGui::Separator();
        if (ImGui::MenuItem("Undo enabled",NULL,useUndo,true)) {useUndo = !useUndo;}
        ImGui::EndMenu();
      } else {
        ImVec2 minB = ImGui::GetItemRectMin();
        ImVec2 maxB = ImGui::GetItemRectMax();
        maxB.x--;
        maxB.y--;
        static unsigned int lastMenuUndoBlink = 0;
        if (lastMenuUndoBlink != undoBlink) {
          ImGui::GetWindowDrawList()->AddRectFilled(minB,maxB,0x08ffffff);
          lastMenuUndoBlink = undoBlink;
        }
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
        bool oneLocked = false;
        for (int i = 0; i < (int)currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected || currentCanvas->layers[i]->state.active) {one = true; count++; if (currentCanvas->layers[i]->state.lockBits & LOCK_USER) oneLocked = true;}
        if (ImGui::MenuItem("Add layer")) {dialogAddLayer(true);}
        if (ImGui::MenuItem("Insert layer like this layer",NULL,false,count == 1)) {currentCanvas->insertLayerLikeThisLayer();}
        if (ImGui::MenuItem("Delete layer(s)",NULL,false,one)) {dialogDeleteLayers(true);}
        ImGui::Separator();
        if (ImGui::MenuItem("Insert text layer")) {currentCanvas->insertTextLayer(); switchTool(TOOL_TEXT);}
        if (ImGui::MenuItem("Insert adjustment layer")) {currentCanvas->insertAdjustmentLayer(); }
        bool tileMapPossible = count == 1 && (currentCanvas->getActiveLayer() != NULL && currentCanvas->getActiveLayer()->type == LAYERTYPE_BASIC);
        if (ImGui::MenuItem("Insert tilemap layer with this layers tiles",NULL,false,tileMapPossible)) {dialogInsertTileMapLayer(true); }
        if (ImGui::MenuItem("Duplicate layer(s)",NULL,false,one)) {dialogDuplicate(true);}
        if (ImGui::MenuItem("Duplicate layer as reference",NULL,false,currentCanvas->getActiveLayer()!=NULL)) {currentCanvas->duplicateLayerAsReference(currentCanvas->getActiveLayer());}
        ImGui::Separator();
        if (ImGui::MenuItem("Hide all layers")) {currentCanvas->showAllLayers(false);}
        if (ImGui::MenuItem("Show all layers")) {currentCanvas->showAllLayers(true);}
        if (ImGui::MenuItem("Select all layers")) {currentCanvas->selectAllLayers();}
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
        if (ImGui::MenuItem("Convert layer(s) from alpha to premul",NULL,false,one)) {beforeUndoPoint();currentCanvas->convertFromAlphaToPremul();afterUndoPoint();}
        if (ImGui::MenuItem("Convert layer(s) from tilemap to bitmap",NULL,false,one)) {beforeUndoPoint();currentCanvas->convertFromTilemapToBitmap();afterUndoPoint();}
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
        if (ImGui::MenuItem(oneLocked ? "*Unlock layer(s)##DRAWMENUBARUNLOCK" : "Unlock layer(s)##DRAWMENUBARUNLOCK",NULL,false,one)) {currentCanvas->lockLayers(false);}
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Selection")) {
        on = true;
        bool one = false;
        UNUSED(one);
        int count = 0;
        for (int i = 0; i < (int)currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected || currentCanvas->layers[i]->state.active) {one = true; count++;}
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
      } else {
        ImVec2 minB = ImGui::GetItemRectMin();
        ImVec2 maxB = ImGui::GetItemRectMax();
        maxB.x--;
        maxB.y--;
        static unsigned int lastMenuSelectionBlink = 0;
        if (lastMenuSelectionBlink != selectionBlink) {
          ImGui::GetWindowDrawList()->AddRectFilled(minB,maxB,0xffffffff);
          lastMenuSelectionBlink = selectionBlink;
        }
      }
      if (ImGui::BeginMenu("View")) {
        on = true;
        bool selected;
        int count = 0;
        for (int i = 0; i < (int)currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected || currentCanvas->layers[i]->state.active) {count++;}
        if (ImGui::MenuItem("Gallery",NULL,galleryVisible)) {galleryVisible = !galleryVisible;}
        ImGui::Separator();
        if (ImGui::MenuItem("Monitor calibration",NULL,hasMonitorCalibration)) {dialogMonitorGamma(true);}
        if (ImGui::MenuItem("Dither calibrated monitor colors",NULL,monitorColorsDithered&&hasMonitorCalibration,hasMonitorCalibration)) {monitorColorsDithered = !monitorColorsDithered; shouldUpdateAll = true; reGenMonitorGamma();}
        selected = aspectRatio == 16.0/9.0; if (ImGui::MenuItem("Aspect ratio 16:9",NULL,&selected)) {aspectRatio = selected?16.0/9.0:(double)glFrameBufferWidth/glFrameBufferHeight; reloadIcons(); reScaleTexts(); shouldUpdateAll = true;}
        ImGui::Separator();
        if (ImGui::MenuItem("Zoom 100%")) {currentCanvas->control.zoomTo(currentCanvas,canvasPixelCenterX,canvasPixelCenterY,100.f);}
        if (ImGui::MenuItem("Zoom 200%")) {currentCanvas->control.zoomTo(currentCanvas,canvasPixelCenterX,canvasPixelCenterY,200.f);}
        ImGui::Separator();
        if (ImGui::MenuItem("Zoom in (+)")) {currentCanvas->control.zoomBy(currentCanvas,canvasPixelCenterX,canvasPixelCenterY,zoomStep);}
        if (ImGui::MenuItem("Zoom out (-)")) {currentCanvas->control.zoomBy(currentCanvas,canvasPixelCenterX,canvasPixelCenterY,1.f/zoomStep);}
        ImGui::Separator();
        if (ImGui::MenuItem("Show layer shapes",NULL,currentCanvas->layerShapesOn)) {currentCanvas->layerShapesOn = !currentCanvas->layerShapesOn; shouldUpdateAll = true;}
        if (ImGui::MenuItem("Show canvas darkened",NULL,showCanvasDarkened)) {showCanvasDarkened = !showCanvasDarkened; shouldUpdateAll = true;}
        ImGui::Separator();
        if (ImGui::MenuItem("Vintage color wheel",NULL,alternativeColorWheel)) {alternativeColorWheel = !alternativeColorWheel;reGenColorWheel();}
        if (ImGui::MenuItem("Show skin tone range in hue",NULL,showSkinTone)) {showSkinTone = !showSkinTone;reGenColorWheel();}
        ImGui::Separator();
        if (ImGui::MenuItem("Show grid",NULL,currentCanvas->gridOn)) {currentCanvas->gridOn = !currentCanvas->gridOn; shouldUpdateAll = true;}
        if (ImGui::MenuItem("Show rulers",NULL,rulersOn)) {rulersOn = !rulersOn; shouldUpdateAll = true;}
        if (ImGui::MenuItem("Visible undo/redo",NULL,visibleUndo)) {visibleUndo = !visibleUndo; shouldUpdateAll = true;}
        if (ImGui::MenuItem("Visible layer switch",NULL,visibleLayerSwitch)) {visibleLayerSwitch = !visibleLayerSwitch; shouldUpdateAll = true;}
        ImGui::Separator();
        static bool wasStored = false;
        if (ImGui::MenuItem("Store scroll")) {currentCanvas->storeScroll(); wasStored = true;}
        if (ImGui::MenuItem("Restore scroll",NULL,false,wasStored)) {currentCanvas->restoreScroll(); shouldUpdateAll = true;}
        if (ImGui::MenuItem("Center view")) {currentCanvas->centerCanvas(); shouldUpdateAll = true;}
        if (ImGui::MenuItem("Fit to view")) {currentCanvas->fitCanvas(); shouldUpdateAll = true;}
        if (ImGui::MenuItem("Scroll to layer origin",NULL, false, count == 1)) {currentCanvas->scrollToLayerOrigin(); shouldUpdateAll = true;}
        ImGui::Separator();
        bool lockedMouse = lockedMouseX||lockedMouseY||tilingOn;
        if (lockedMouse) ImGui::PushStyleColor(ImGuiCol_Text,(k2 & 1) ? ImVec4(1,0,0,1) : ImVec4(0.5,0,0,1));
        if (ImGui::MenuItem("Clear mouse lock and tiling",NULL,false,lockedMouse)) {lockedMouseX = false; lockedMouseY = false; tilingOn = false; if (tool == TOOL_TILING) switchTool(TOOL_NONE);}
        if (lockedMouse) ImGui::PopStyleColor();
        if (ImGui::MenuItem("Show only selected layer(s) selection",NULL,showOnlyActiveLayersSelection)) {showOnlyActiveLayersSelection = !showOnlyActiveLayersSelection; shouldUpdateAll = true;}
        ImGui::Separator();
        if (ImGui::MenuItem("Sort palette by luminance",NULL)) {sortPalette(palette); updatePalette(); shouldUpdateAll = true; showPalette = true;}
        if (ImGui::MenuItem("Quantize with luminance",NULL,fastQuantWithLuma)) {fastQuantWithLuma = !fastQuantWithLuma; shouldUpdateAll = true; updatePalette();}
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Filter")) {
        on = true;
        bool one = false;
        UNUSED(one);
        int count = 0;
        for (int i = 0; i < (int)currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected || currentCanvas->layers[i]->state.active) {one = true; count++;}
        if (ImGui::MenuItem("Difference clouds",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterDifferenceClouds();}
        if (ImGui::MenuItem("Noise",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterNoise();}
        if (ImGui::MenuItem("Checkerboard",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogCheckerboard(true);}
        if (ImGui::MenuItem("Edge detect",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterMorphologicalEdgeDetect();}
        if (ImGui::MenuItem("Blur",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogBlur(true);}
        if (ImGui::MenuItem("Alpha soften",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterAlphaSoften();}
        if (ImGui::MenuItem("Turn to glow",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogGlow(true);}
        if (ImGui::MenuItem("Strong soften",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterSoften();}
        if (ImGui::MenuItem("Emboss",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogEmboss(true);}
        if (ImGui::MenuItem("Keying/matte",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogKeying(true);}
        if (ImGui::MenuItem("Denoise",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterDenoise();}
        if (ImGui::MenuItem("Sharpen and grain",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterSharpen();}
        if (ImGui::MenuItem("Pixelart inflate",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogPixelArtInflate(true);}
        if (ImGui::MenuItem("Histogram matching (brightness)",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogLoadSave(true,DIALOG_HISTOGRAMMATCH);}
        if (ImGui::MenuItem("Histogram light (colored)",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogLoadSave(true,DIALOG_HISTOGRAMMATCHINGCOLORED);}
        if (ImGui::MenuItem("Sepia",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterSepia();}
        if (ImGui::MenuItem("Color transfer (monochrome)",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogLoadSave(true,DIALOG_COLORTRANSFER);}
        if (ImGui::MenuItem("Clahe lighting",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogClahe(true);}
        if (ImGui::MenuItem("Displacement mapping",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {dialogDisplacementMap(true);}
        if (ImGui::MenuItem("OCR black & white",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterBlackWhiteOCR();}
        if (ImGui::MenuItem("Selection distance field",NULL,false,count==1&&(currentCanvas->getActiveLayer()!=NULL))) {filterSelectionDistanceField(currentCanvas->getActiveLayer());}
        ImGui::EndMenu();
      }
  
      bool hasUpdates = false;
      for (int i = 0; i < (int)canvas.size(); i++) {
        if (canvas[i]->hasUpdates()) {hasUpdates = true; break;}
      }
  
      if (ImGui::BeginMenu(combined(String(hasUpdates?".":"")+String("Pictures[")+String::fromInt((int)canvas.size()) + "]##MENUBARPICTURES").c_str())) {
        on = true;
        for (int i = 0; i < (int)canvas.size(); i++) {
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
      } else {
        ImVec2 minB = ImGui::GetItemRectMin();
        ImVec2 maxB = ImGui::GetItemRectMax();
        maxB.x--;
        maxB.y--;
        static unsigned int lastMenuPicturesBlink = 0;
        if (lastMenuPicturesBlink != picturesBlink) {
          ImGui::GetWindowDrawList()->AddRectFilled(minB,maxB,0xffffffff);
          lastMenuPicturesBlink = picturesBlink;
        }
      }
    }

    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("About")) {dialogAbout(true);}
      if (ImGui::MenuItem("Hotkeys")) {dialogHotkeys(true);}
      ImGui::Separator();
      if (ImGui::MenuItem("If you are new")) {dialogNewHere(true);}
      if (ImGui::MenuItem("Puls4r game creation suite")) {dialogSuite(true);}
      if (ImGui::MenuItem("Tiny documentation")) {dialogDocumentation(true);}
      if (ImGui::MenuItem("What's Wypa for?")) {dialogWhatFor(true);}
      ImGui::Separator();
      if (ImGui::MenuItem("Show tooltips",NULL,showToolTips,true)) {showToolTips = !showToolTips;}
      if (ImGui::MenuItem("Status window",NULL,debugWindowOn,true)) {debugWindowOn = !debugWindowOn;}
      on = true;
      ImGui::EndMenu();
    }
    ImGui::SameLine();
    horzSep();
    ImGui::SameLine();
    String timeDesc = timeDescString();
    if (((k2%10) < 1) && timeDesc.length()>3) {timeDesc[timeDesc.length()-3] = ' ';}
    ImVec2 cursorScreen = ImGui::GetCursorScreenPos();
    cursorScreen.x = (float)(main_window_xres - timeDesc.length()*7);
    ImGui::SetCursorScreenPos(cursorScreen);
    ImGui::Text("%s",timeDesc.c_str()); toolTip("Info: %s", extensiveTimeDescString().c_str());
    cursorScreen.x -= 7 + 3;
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(cursorScreen);
    horzSep();

    cursorScreen.x = (float)(main_window_xres - timeDesc.length()*7 - 150 + 5);
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
    } else toolTip("Mouse velocity");
    ImGui::PopStyleColor();
    ImGui::PopItemWidth();


    cursorScreen.x = (float)(main_window_xres - timeDesc.length()*7 - 150 - 110 - 20-3);
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(cursorScreen);
    horzSep();
    ImGui::SameLine();
    cursorScreen.x += 3+3+2+2+1;
    ImGui::SetCursorScreenPos(cursorScreen);
    ImGui::Text("Zoom");
    ImGui::SameLine();
    ImGui::PushItemWidth(50);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0.125,0.125,0.125,1));
    float zoom = (float)(currentCanvas->control.zoom);
    if (ImGui::InputFloat("##DRAWMENUBARZOOM",&zoom,0,0,2)) {
      if (zoom < 0.01f) zoom = 0.01f;
      currentCanvas->control.zoomTo(currentCanvas,canvasPixelCenterX,canvasPixelCenterY,zoom);
    } else toolTip("Zoom of the complete picture");
    ImGui::PopStyleColor();
    ImGui::PopItemWidth();
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0.125,0.125,0.125,1));
    ImGui::SameLine(); if (ImGui::Checkbox("##DRAWMENUBARFULLSCREEN",&fullScreen)) {} else toolTip("Black env.");
    ImGui::PopStyleColor();
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
