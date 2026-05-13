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
#include "STL/GL.H"
#include "IMGUI/IMGUIGL.HPP"
#include "CONFIG.HPP"
#include "DIALOGS.HPP"
#include "LAYERS.HPP"
#include "STL/DOS.HPP"
#include "SVG.HPP"
#include "SERIALZE.HPP"
#include "HOTKEYS.HPP"
#include "UNDO.HPP"
#include "FILTER.HPP"
#include "COLORFLD.HPP"
#include <math.h>

bool reOpen = false;
bool dialogNoMove = false;

#define DIALOGSTART(__name__) static int keyboardFocusNew = 0; static bool openItNextTime = false;  if (openIt) {openItNextTime = true; keyboardFocusNew = 2; return;}  if (openItNextTime) {openItNextTime = false; ImGui::OpenPopup(__name__); } ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0,0,0,1)); ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(1,1,1,1); if (ImGui::BeginPopupModal(__name__,NULL,ImGuiWindowFlags_AlwaysAutoResize|(dialogNoMove?ImGuiWindowFlags_NoMove:0))) { ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(0,0,0,1); ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.4,0.4,0.4,1));disableNativeInput();
#define DIALOGEND() ImGui::PopStyleColor(); ImGui::EndPopup();} ImGui::PopStyleColor();
#define DIALOGKEYSHERE() {if (keyboardFocusNew>0) {keyboardFocusNew--;ImGui::SetKeyboardFocusHere();}}

bool roundButton(const char *name, bool enabled);

unsigned int previewTexture = 0;
const unsigned int previewWidth = 64;
const unsigned int previewHeight = 64;
unsigned int preview[previewWidth*previewHeight];
double previewAspect = 1;

String dialogShortPath;
String dialogLongFileName;

void updatePreview() {
  glBindTexture(GL_TEXTURE_2D,previewTexture);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, previewWidth, previewHeight, 0, GL_RGBA, GL_BYTE, preview);
  glBindTexture(GL_TEXTURE_2D,0);
}

void clearPreview() {
  for (int y = 0; y < previewHeight; y++) {
    for (int x = 0; x < previewWidth; x++) {
      preview[x+y*previewWidth] = 0x40000000;
    }
  }
  previewAspect = 1;
  updatePreview();
}

void noUndoMessage() {
  ImGui::TextColored(ImVec4(1,0,0,1),"Be aware that there is no <undo> for this!!");
}

void initDialogs() {
  glGenTextures(1,&previewTexture);
  glBindTexture(GL_TEXTURE_2D,previewTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  clearPreview();
  glBindTexture(GL_TEXTURE_2D,0);
}

void freeDialogs() {
  glDeleteTextures(1, &previewTexture);
}

void drawDialogs() {
  dialogCanvas(false);
  dialogExpandShrinkSelection(false);
  dialogImageResize(false);
  dialogLoadSave(false,DIALOG_NONE);
  dialogOverwrite(false);
  dialogDeletePicture(false);
  dialogAddLayer(false);
  dialogDeleteLayers(false);
  dialogSelectByAlpha(false);
  dialogBakeLayers(false);
  dialogNewImage(false);
  dialogGetLayersFromPicture(false);
  dialogUpdateLayer(false);
  dialogAbout(false);
  dialogExit(false);
  dialogDuplicate(false);
  dialogMergeDown(false);
  dialogInsertSVG(false,0,0);
  dialogExportLayersRaw(false);
  dialogPictureInfo(false);
  dialogGradiationCurves(false);
  dialogColorBalance(false);
  dialogBlur(false);
  dialogGlow(false);
  dialogEmboss(false);
  dialogHotkeys(false);
  dialogColorMatrix(false);
  dialogQuantizePalette(false);
  dialogExtrudeSelection(false);
  dialogKeying(false);
}

void dialogAbout(bool openIt) {
  DIALOGSTART("About")
  ImGui::Text(
    "WYPA - A SIMPLE PAINT PAINT PROGRAM FOR DOS\n"
    "------------------------------------\n"
    "Coded in early 2026 by\n"
    "Stefan Mader of Puls4r\n"
    "------------------------------------\n"
    "Source is released under the most\n"
    "common: MIT License. (open source)\n"
    "------------------------------------\n"
    "Build date: " __DATE__ "\n"
    "Build time: " __TIME__ "\n"
    "Version: %d.%02d\n"
    "------------------------------------\n"
    "Wypa is not coded very \"good\".\n"
    "It's just a proof of concept, since\n"
    "there where almost no other options\n"
    "to paint in high resolutions with true color.\n"
    "------------------------------------\n"
    "This program is freeware.\n"
  ,version / 100, version % 100
  );
  ImGui::Separator();
  if (ImGui::Button("Close##DIALOGABOUTCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

#define HOTKEY(__char__,__ctrl__,__alt__,__text__) ImGui::Text("%s",__text__);

void dialogHotkeys(bool openIt) {
  DIALOGSTART("Hotkeys")
  HOTKEY_ZOOMIN
  HOTKEY_ZOOMOUT
  HOTKEY_ZOOMPAN
  HOTKEY_TOOL_SELECTION
  HOTKEY_TOOL_MOVE
  HOTKEY_TOOL_WAND
  HOTKEY_TOOL_PIPETTE
  HOTKEY_TOOL_PENCIL
  HOTKEY_TOOL_PAINTBRUSH
  HOTKEY_TOOL_ERASER
  HOTKEY_TOOL_FILL
  HOTKEY_TOOL_LINE
  HOTKEY_TOOL_GRADIENT
  HOTKEY_TOOL_TEXT
  HOTKEY_TOOL_MAGIC
  HOTKEY_TOOL_BLUR
  HOTKEY_TOOL_MEASURE
  HOTKEY_TOOL_TILING
  HOTKEY_TOOL_OVERLAYS
  HOTKEY_TOGGLE_PALETTE
  HOTKEY_LAYERUNDERMOUSE
  HOTKEY_SELECTNONE
  HOTKEY_SELECTALL
  HOTKEY_INVERTSELECTION
  HOTKEY_COPY
  HOTKEY_PASTE
  HOTKEY_FITCANVAS
  HOTKEY_MOUSELOCKX
  HOTKEY_MOUSELOCKY
  HOTKEY_FULLSCREEN
  HOTKEY_ERASE
  HOTKEY_FILLCOLOR
  ImGui::Separator();
  if (ImGui::Button("Close##DIALOGABOUTCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

extern bool quit;

void dialogExit(bool openIt) {
  DIALOGSTART("Exit")
  ImGui::Text("Do you really want to exit?");
  ImGui::Text("Did you save all your work?");
  if (ImGui::Button("Quit##DIALOGEXITEXIT")) {
    quit = true;
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGEXITCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogCanvas(bool openIt) {
  static double x0=0,y0=0,x1=0,y1=0;
  if (openIt) {
    x0 = currentCanvas->canvasX0;
    y0 = currentCanvas->canvasY0;
    x1 = currentCanvas->canvasX1;
    y1 = currentCanvas->canvasY1;
  }
  DIALOGSTART("Configure canvas")
  int k;
  ImGui::PushItemWidth(64+16*2);
  k = (int)(currentCanvas->canvasX1-currentCanvas->canvasX0);ImGui::Text("Canvas width");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASVANVASWIDTH",&k)) {currentCanvas->canvasX1 = currentCanvas->canvasX0 + k;}
  k = (int)(currentCanvas->canvasY1-currentCanvas->canvasY0);ImGui::Text("Canvas height");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASVANVASHEIGHT",&k)) {currentCanvas->canvasY1 = currentCanvas->canvasY0 + k;}
  k = (int)(currentCanvas->canvasX0);ImGui::Text("Canvas x");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASVANVASX",&k)) {int v = currentCanvas->canvasX1-currentCanvas->canvasX0;currentCanvas->canvasX0 = k;currentCanvas->canvasX1 = currentCanvas->canvasX0+v;}
  k = (int)(currentCanvas->canvasY0);ImGui::Text("Canvas y");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASVANVASY",&k)) {int v = currentCanvas->canvasY1-currentCanvas->canvasY0;currentCanvas->canvasY0 = k;currentCanvas->canvasY1 = currentCanvas->canvasY0+v;}
  ImGui::PopItemWidth();
  double w = currentCanvas->canvasX1-currentCanvas->canvasX0;
  double h = currentCanvas->canvasY1-currentCanvas->canvasY0;
  if (w < 1) w = 1;
  if (h < 1) h = 1;
  if (ImGui::Button("tl##DIALOGCANVASTL")) {currentCanvas->canvasX0 = x0;currentCanvas->canvasY0 = y0;}
  ImGui::SameLine();
  if (ImGui::Button("tc##DIALOGCANVASTC")) {currentCanvas->canvasX0 = (x0+x1)/2-w/2;currentCanvas->canvasY0 = y0;}
  ImGui::SameLine();
  if (ImGui::Button("tr##DIALOGCANVASTR")) {currentCanvas->canvasX0 = x1-w;currentCanvas->canvasY0 = y0;}

  if (ImGui::Button("cl##DIALOGCANVASCL")) {currentCanvas->canvasX0 = x0;currentCanvas->canvasY0 = (y0+y1)/2-h/2;}
  ImGui::SameLine();
  if (ImGui::Button("cc##DIALOGCANVASCC")) {currentCanvas->canvasX0 = (x0+x1)/2-w/2;currentCanvas->canvasY0 = (y0+y1)/2-h/2;}
  ImGui::SameLine();
  if (ImGui::Button("cr##DIALOGCANVASCR")) {currentCanvas->canvasX0 = x1-w;currentCanvas->canvasY0 = (y0+y1)/2-h/2;}

  if (ImGui::Button("bl##DIALOGCANVASBL")) {currentCanvas->canvasX0 = x0;currentCanvas->canvasY0 = y1-h;}
  ImGui::SameLine();
  if (ImGui::Button("bc##DIALOGCANVASBC")) {currentCanvas->canvasX0 = (x0+x1)/2-w/2;currentCanvas->canvasY0 = y1-h;}
  ImGui::SameLine();
  if (ImGui::Button("br##DIALOGCANVASBR")) {currentCanvas->canvasX0 = x1-w;currentCanvas->canvasY0 = y1-h;}

  currentCanvas->canvasX1 = currentCanvas->canvasX0+w;
  currentCanvas->canvasY1 = currentCanvas->canvasY0+h;

  ImGui::PushItemWidth(64+16*2);

  ImGui::Separator();

  ImGui::Text("");
  float k2;
  k2 = (float)currentCanvas->control.pixelSizeX; ImGui::Text("Pixel size x");ImGui::SameLine();if (ImGui::InputFloat("##DIALOGCANVASPIXELSIZEX",&k2,0,0,2)) {currentCanvas->control.pixelSizeX = k2; shouldUpdateAll = true;}
  k2 = (float)currentCanvas->control.pixelSizeY; ImGui::Text("Pixel size y");ImGui::SameLine();if (ImGui::InputFloat("##DIALOGCANVASPIXELSIZEY",&k2,0,0,2)) {currentCanvas->control.pixelSizeY = k2; shouldUpdateAll = true;}

  ImGui::Text("");
  k = currentCanvas->gridStepX; ImGui::Text("Grid step x");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASGRIDSTEPX",&k)) {currentCanvas->gridStepX = k;}
  k = currentCanvas->gridStepY; ImGui::Text("Grid step y");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASGRIDSTEPY",&k)) {currentCanvas->gridStepY = k;}
  k = currentCanvas->gridOffsetX; ImGui::Text("Grid offset x");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASGRIDOFFSETX",&k)) {currentCanvas->gridOffsetX = k;}
  k = currentCanvas->gridOffsetY; ImGui::Text("Grid offset y");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASGRIDOFFSETY",&k)) {currentCanvas->gridOffsetY = k;}
  if (currentCanvas->gridStepX < 2) currentCanvas->gridStepX = 2;
  if (currentCanvas->gridStepY < 2) currentCanvas->gridStepY = 2;
  if (currentCanvas->gridStepX > 16384) currentCanvas->gridStepX = 16384;
  if (currentCanvas->gridStepY > 16384) currentCanvas->gridStepY = 16384;
  if (currentCanvas->gridOffsetX > 16384) currentCanvas->gridOffsetX = 16384;
  if (currentCanvas->gridOffsetY > 16384) currentCanvas->gridOffsetY = 16384;
  if (currentCanvas->gridOffsetX < -16384) currentCanvas->gridOffsetX = -16384;
  if (currentCanvas->gridOffsetY < -16384) currentCanvas->gridOffsetY = -16384;

  ImGui::PopItemWidth();

  if (ImGui::Button("Close##DIALOGCANVASCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogImageResize(bool openIt) {
  static double x0=0,y0=0,x1=0,y1=0,w=0,ow=0,h=0,oh=0;
  static bool keepAspect = true;
  static bool quantize = false;
  static double aspect = 1;
  static int interpolation = INTERPOLATION_NONE;
  if (openIt) {
    x0 = currentCanvas->canvasX0;
    y0 = currentCanvas->canvasY0;
    x1 = currentCanvas->canvasX1;
    y1 = currentCanvas->canvasY1;
    ow = x1 - x0;
    w = ow;
    oh = y1 - y0;
    h = oh;
    keepAspect = true;
    quantize = true;
    if (oh != 0) aspect = ow/oh;
  }
  DIALOGSTART("Resize image")
  noUndoMessage();
  int k;
  ImGui::Text("You may lose pixel information through resizing");
  ImGui::Text("and the layers may be rotated/resized when saving the layers!");
  ImGui::Text("");
  ImGui::PushItemWidth(64+16*2);
  k = (int)w;ImGui::Text("New image width");ImGui::SameLine();if (ImGui::InputInt("##DIALOGIMAGERESIZEWIDTH",&k)) {w = k; if (keepAspect) h = w/aspect;}
  k = (int)h;ImGui::Text("New image height");ImGui::SameLine();if (ImGui::InputInt("##DIALOGIMAGERESIZEHEIGHT",&k)) {h = k; if (keepAspect) w = h*aspect;}
  if (w < 1) w = 1;
  if (h < 1) h = 1;
  ImGui::PopItemWidth();
  ImGui::Text("Preserve proportions");ImGui::SameLine();ImGui::Checkbox("##DIALOGIMAGERESIZEKEEPASPECT",&keepAspect);
  ImGui::Text("Quantize/bake all");ImGui::SameLine();ImGui::Checkbox("##DIALOGIMAGERESIZEQUANTIZE",&quantize);
  ImGui::PushItemWidth(7*12);
  ImGui::Text("Interpolation");ImGui::SameLine();ImGui::Combo("##DIALOGIMAGERESIZEINTERPOLATION",&interpolation, interpolationNames, INTERPOLATION_COUNT);
  ImGui::PopItemWidth();

  if (ImGui::Button("Resize##DIALOGIMAGERESIZEESIZE")) {
    currentCanvas->resizeCanvas(w,h,quantize,interpolation);
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGIMAGERESIZECLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogExpandShrinkSelection(bool openIt) {
  static int value = 1;
  static bool shrink = false;
  if (openIt) {
    shrink = false;
    value = 1;
  }
  DIALOGSTART("Expand or shrink selection")

  ImGui::PushItemWidth(64+16*2);
  int k;
  k = value; ImGui::Text("Expand or shrink selection by pixels");ImGui::SameLine();if (ImGui::InputInt("##DIALOGSELECTIONEXPANDORSHRINKVALUE",&k)) {value = k;}
  ImGui::Text("Shrink?");ImGui::SameLine();if (ImGui::Checkbox("##DIALOGSELECTIONEXPANDORSHRINKSHRINK",&shrink)) {}
  if (value > 100) value = 100;
  if (value < -100) value = -100;

  if (ImGui::Button("Apply##DIALOGCANVASAPPLY")) {
    beforeUndoPoint();
    currentCanvas->expandSelection(value * (shrink ? -1 : 1));
    afterUndoPoint();
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGCANVASCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}


void dialogDeletePicture(bool openIt) {
  DIALOGSTART("Delete current picture?");
  noUndoMessage();
  ImGui::Text("Do you want to delete the current picture \"%s\"?",currentCanvas->pictureName);
  if (ImGui::Button("Delete##DIALOGDELETEPICTUREDELETE")) {
    for (int i = 0; i < canvas.size(); i++) {
      if (canvas[i] == currentCanvas) {
        delete canvas[i];
        canvas.erase(i,1);
        if (i < canvas.size()) currentCanvas = canvas[i]; 
        else
        if (i-1 >= 0) currentCanvas = canvas[i-1];
      }
    }
    if (canvas.empty()) {
      addTempPicture();
      currentCanvas = canvas[0];
    }
    shouldUpdateAll = true;
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGDELETEPICTURECLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogAddLayer(bool openIt) {
  static char layerName[LAYERNAMEMAX];
  static int w = 0, h = 0;
  if (openIt) {
    static int counter = 0; counter++;
    sprintf(layerName,"Added %d",counter);
    w = currentCanvas->canvasX1-currentCanvas->canvasX0;
    h = currentCanvas->canvasY1-currentCanvas->canvasY0;
  }
  DIALOGSTART("Add layer");
  noUndoMessage();
  ImGui::Text("Layer name"); ImGui::SameLine(); ImGui::InputText("##DIALOGADDLAYERNAME",layerName,LAYERNAMEMAX);
  ImGui::PushItemWidth(64+16*2);
  ImGui::Text("Layer width"); ImGui::SameLine(); ImGui::InputInt("##DIALOGADDLAYERWIDTH",&w);
  ImGui::Text("Layer height"); ImGui::SameLine(); ImGui::InputInt("##DIALOGADDLAYERHEIGHT",&h);
  ImGui::PopItemWidth();
  if (w < 1) w = 1;
  if (h < 1) h = 1;
  if (ImGui::Button("Add layer##DIALOGADDLAYERADD")) {
    int pos = 0;
    for (int i = 0; i < currentCanvas->layers.size(); i++) {
      if (currentCanvas->layers[i]->state.active || currentCanvas->layers[i]->state.selected) {pos = i; break;}
    }
    Layer *l = new Layer();
    l->init(w,h);
    l->setLayerPos(currentCanvas->canvasX0,currentCanvas->canvasY0);
    sprintf(l->name,layerName);
    currentCanvas->insertLayer(l,pos);
    currentCanvas->setActiveLayer(pos);
    shouldUpdateAll = true;
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGADDLAYERCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogDeleteLayers(bool openIt) {
  DIALOGSTART("Delete layers");

  Array<Layer *> layers;
  bool one = false; for (int i = 0; i < currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected) {one = true; layers.push_back(currentCanvas->layers[i]);}
  if (!one) for (int i2 = 0; i2 < currentCanvas->layers.size(); i2++) if (currentCanvas->layers[i2]->state.active) {layers.push_back(currentCanvas->layers[i2]);}

  int l = layers.size();
  for (int i3 = 0; i3 < l; i3++) {
    if (layers[i3]->sourceLayerId == 0) {
      for (int i4 = 0; i4 < currentCanvas->layers.size(); i4++) {
        Layer *z = currentCanvas->layers[i4];
        if (z->sourceLayerId != 0 && z->sourceLayerId == layers[i3]->id) {
          layers.push_back(z);
        }
      }
    }
  }

  if (layers.empty()) {
    ImGui::Text("No layers selected or active");
  } else {
    noUndoMessage();
    ImGui::Text("Layers to delete");
    ImGui::BeginChild("DELETELAYERSCHILD",ImVec2(main_window_xres/2,main_window_yres/3));
    for (int i = 0; i < layers.size(); i++) {
      ImGui::Text("\"%s\" %s",layers[i]->name,layers[i]->sourceLayerId != 0 ? "it's a REFERENCE":"");
    }
    ImGui::EndChild();
  }

  if (!layers.empty()) {
    if (ImGui::Button("Delete these layers##DIALOGDELETELAYERSDELETE")) {
      for (int i = 0; i < layers.size(); i++) {
        currentCanvas->eraseLayer(layers[i]);
      }
      shouldUpdateAll = true;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGDELETELAYERSCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogMergeDown(bool openIt) {
  DIALOGSTART("Merge down layers");

  Array<Layer *> layers;
  bool one = false; for (int i = 0; i < currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected) {one = true; layers.push_back(currentCanvas->layers[i]);}
  if (!one) for (int i2 = 0; i2 < currentCanvas->layers.size(); i2++) if (currentCanvas->layers[i2]->state.active) {layers.push_back(currentCanvas->layers[i2]);}

  if (layers.empty()) {
    ImGui::Text("No layers selected or active");
  } else {
    noUndoMessage();
    ImGui::Text("Layers to merge");
    ImGui::BeginChild("MERGEDOWNLAYERSCHILD",ImVec2(main_window_xres/2,main_window_yres/3));
    for (int i = 0; i < layers.size(); i++) {
      ImGui::Text("\"%s\"",layers[i]->name);
    }
    ImGui::EndChild();
  }

  if (!layers.empty()) {
    if (ImGui::Button("Merge down these layers##DIALOGMERGEDOWNLAYERSMERGE")) {
      currentCanvas->mergeDownLayers();
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGMERGEDOWNLAYERSCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogInsertSVG(bool openIt, int width, int height) {
  static int w = 0, h = 0;
  static float scale = 1;
  if (openIt) {
    w = width;
    h = height;
    scale = 1;
  }
  DIALOGSTART("Insert SVG");
  ImGui::PushItemWidth(64+16*2);
  ImGui::Text("Layer width %d",(int)ceil(w*scale));
  ImGui::Text("Layer height %d",(int)ceil(h*scale));
  ImGui::Text("Scale"); ImGui::SameLine(); ImGui::InputFloat("##DIALOGINSERTSVGSCALE",&scale,0,0,2); if (scale < 0) scale = 0;
  ImGui::PopItemWidth();
  if ((int)ceil(w*scale) > 0 && (int)ceil(h*scale) > 0) {
    if (ImGui::Button("Insert##DIALOGINSERTSVGINSERT")) {
      insertSVGResized(w, h, scale);
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGINSERTSVGCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogExportLayersRaw(bool openIt) {
  static bool colorTransformOn = true;
  if (openIt) {
    colorTransformOn = true;
  }
  DIALOGSTART("Export layers raw");
  ImGui::Text("Apply postprocessing?"); ImGui::SameLine(); ImGui::Checkbox("##DIALOGEXPORTLAYERSRAWCOLORGRADINGON",&colorTransformOn);
  if (ImGui::Button("Save##DIALOGEXPORTLAYERSRAWEXPORT")) {
    exportLayersRaw(dialogShortPath.c_str(),dialogLongFileName.c_str(),colorTransformOn);
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGEXPORTLAYERSRAWCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

#define PICTURECOMMENTMAX 256
char tempCommentBuffer[PICTURECOMMENTMAX];

void dialogPictureInfo(bool openIt) {
  if (openIt) {
    sprintf(tempCommentBuffer,"%s",currentCanvas->comment.c_str());
  }
  DIALOGSTART("Picture info");
  int nameSize = 7*(PICTURENAMEMAX);
  ImGui::PushItemWidth(nameSize);
  ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0.9,0.9,0.9,1));
  ImGui::Text("Name"); ImGui::SameLine(); ImGui::InputText("##DIALOGPICTUREINFOPICTURENAME",currentCanvas->pictureName, PICTURENAMEMAX);
  ImGui::Text("Author"); ImGui::SameLine(); ImGui::InputText("##DIALOGPICTUREINFOAUTHOR",currentCanvas->author, PICTURENAMEMAX);
  ImGui::PopStyleColor();
  ImGui::PopItemWidth();
  ImGui::Text("Date %s",currentCanvas->timeStamp.c_str());
  ImGui::Text("Width %d", currentCanvas->canvasX1-currentCanvas->canvasX0);
  ImGui::Text("Height %d", currentCanvas->canvasY1-currentCanvas->canvasY0);
  ImGui::Text("Comment");
  ImGui::InputTextMultiline("##DIALOGPICTUREINFOCOMMENT", tempCommentBuffer, PICTURECOMMENTMAX);
  if (ImGui::Button("Close##DIALOGPICTUREINFOCLOSE")) {
    currentCanvas->comment = String(tempCommentBuffer);
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

int gradiationTextureWidth = 256;
int gradiationTextureHeight = 256;
void updateGradiationTexture(GLuint gradiationTexture, Layer *l, int type) {
  Array<unsigned int> gradiationTextureContent;
  gradiationTextureContent.resize(gradiationTextureWidth*gradiationTextureHeight);
  for (int i = 0; i < gradiationTextureContent.size(); i++) gradiationTextureContent[i] = 0xff000000;
  Array<int> counts; counts.resize(256);
  for (int i5 = 0; i5 < 256; i5++) counts[i5] = 0;
  for (int i2 = 0; i2 < l->width*l->height; i2++) {
    unsigned int col = l->pixels[i2];
    int r = col & 255;
    int g = (col>>8) & 255;
    int b = (col>>16) & 255;
    int a = (col>>24) & 255;
    int grey = (r+g+b)/3;
    int count = 0;
    switch(type) {
    case 0: {count = grey;} break;
    case 1: {count = r;} break;
    case 2: {count = g;} break;
    case 3: {count = b;} break;
    case 4: {count = a;} break;
    }
    counts[count]++;
  }
  int max = 0;
  for (int i3 = 0; i3 < 256; i3++) {
    if (counts[i3]>max) max = counts[i3];
  }
  if (max > 0) {
    for (int i4 = 0; i4 < 256; i4++) {
      counts[i4] = counts[i4]*255/max;
    }
  }
  for (int y = 0; y < gradiationTextureHeight; y++) {
    for (int x = 0; x < gradiationTextureWidth; x++) {
      int heightHere = 255-y*256/gradiationTextureHeight;
      int xk = x * 256 / gradiationTextureWidth;
      unsigned int col = 0xff000000;
      if (heightHere < counts[xk]) col = 0xff0000ff;
      gradiationTextureContent[x+y*gradiationTextureWidth] = col;
    }
  }
  glBindTexture(GL_TEXTURE_2D,gradiationTexture);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, gradiationTextureWidth, gradiationTextureHeight, 0, GL_RGBA, GL_BYTE, &gradiationTextureContent[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);
}

bool showCurveCVs = false;

class CurvePoint {
public:
  int x,y;
  float dx[2],dy[2];
  CurvePoint() {x=y=0; dx[0]=dy[0]=dx[1]=dy[1]=0;}
  CurvePoint(int x, int y) {this->x=x; this->y=y; dx[0]=dy[0]=dx[1]=dy[1]=0;}
};

double bezier(double t, double p0, double p1, double p2, double p3) {
  return (1-t)*(1-t)*(1-t)*p0 + 3*(1-t)*(1-t)*t*p1 + 3*(1-t)*t*t*p2 + t*t*t*p3;
}

class Curve {
public:
  Curve() {
    selectedPoint = -1;
    selectedCV = -1;
    dragPoint = -1;
  }
  int bezierY[256];
  Array<CurvePoint> points;
  void clear() {points.clear(); selectedPoint = -1; dragPoint = -1; selectedCV = -1;}
  int selectedPoint;
  int selectedCV;
  int dragPoint;

  int getInterpolation(int x) {
    int found = -1;
    for (int i = 0; i < points.size()-1; i++) {
      if (x >= points[i].x && x <= points[i+1].x) {
        found = i;
        break;
      }
    }
    if (found < 0) return 0;
    float deltaX = x - points[found].x;
    float deltaXF = points[found+1].x - points[found].x;
    if (deltaXF == 0) return points[found].x;
    if (points[found].dx[1]==0&&points[found].dy[1]==0&&points[found+1].dx[0]==0&&points[found+1].dy[0]==0)
      return (points[found+1].y - points[found].y)*deltaX/deltaXF + points[found].y;
    return bezierY[x];
  } 

  void insertPoint(int x) {
    int found = -1;
    for (int i = 0; i < points.size()-1; i++) {
      if (x > points[i].x && x <= points[i+1].x) {
        found = i;
        break;
      }
    }
    if (found < 0) return;
    int y = getInterpolation(x);
    points.insert(CurvePoint(x,y),found+1);
    selectedPoint = found + 1;
    dragPoint = -1;
  }

  void erasePoint(int x) {
    if (x < 1) return;
    if (x >= points.size()-1) return;
    points.erase(x,1);
    selectedPoint = x-1;
    dragPoint = -1;
  }

  void generateBezier() {
#define BEZIER_TESSEL 128
    int lxi = points[0].x;
    int lyi = points[0].y;
    for (int i = 0; i < points.size()-1; i++) {
      for (int j = 0; j < BEZIER_TESSEL+1; j++) {
        double bx = bezier((double)j/BEZIER_TESSEL,points[i].x,points[i].x+points[i].dx[1],points[i+1].x+points[i+1].dx[0],points[i+1].x);
        double by = bezier((double)j/BEZIER_TESSEL,points[i].y,points[i].y+points[i].dy[1],points[i+1].y+points[i+1].dy[0],points[i+1].y);
        int bxi = (int)floor(bx);
        int byi = (int)floor(by);
        int dx = bxi-lxi;
        int dy = byi-lyi;
        int d = abs(dx);
        float xp = lxi;
        float yp = lyi;
        float xa = 1;
        float ya = d>0?dy/d:0;
        for (int k = 0; k < d+1; k++) {
          int xpi = (int)floor(xp);
          int ypi = (int)floor(yp);
          if (xpi>=points[i].x&&xpi<=points[i+1].x&&xpi<256) {
            bezierY[xpi] = ypi;
          }
          xp += xa;
          yp += ya;
        }
        lxi = bxi;
        lyi = byi;
      }
    }
  }
};

Curve curves[5];

void drawCurve(const ImVec2 &minB,const ImVec2 &maxB, Curve &c) {
  UNUSED(maxB);
  static bool lastMouseLeft = ImGui::IsMouseDown(0);
  bool mouseLeft = ImGui::IsMouseDown(0);
  for (int i = 0; i < c.points.size()-1; i++) {
    ImVec2 p0 = ImVec2(c.points[i].x,255-c.points[i].y);
    ImVec2 p1 = ImVec2(c.points[i+1].x,255-c.points[i+1].y);
    p0.x += minB.x;
    p0.y += minB.y;
    p1.x += minB.x;
    p1.y += minB.y;
    if (c.points[i].dx[1]==0&&c.points[i].dy[1]==0&&c.points[i+1].dx[0]==0&&c.points[i+1].dy[0]==0) {
      ImGui::GetWindowDrawList()->AddLine(p0,p1,0xffffffff);
    } else {
      ImVec2 last = p0;
      for (int x = c.points[i].x; x < c.points[i+1].x; x++) {
        ImVec2 p = ImVec2(x,255-c.bezierY[x]);
        p.x += minB.x;
        p.y += minB.y;
        ImGui::GetWindowDrawList()->AddLine(last,p,0xffffffff);
        last = p;
      }
      ImGui::GetWindowDrawList()->AddLine(last,p1,0xffffffff);
    }
  }
  float circleRadius = 8;
  if (!mouseLeft) {
    c.dragPoint = -1;
  }
  for (int i2 = 0; i2 < c.points.size(); i2++) {
    ImVec2 p0 = ImVec2(c.points[i2].x,255-c.points[i2].y);
    p0.x += minB.x;
    p0.y += minB.y;
    double dx = mouseX - p0.x;
    double dy = mouseY - p0.y;
    double d = sqrt(dx*dx+dy*dy);
    if (!showCurveCVs) {
      unsigned int col = 0xff00ffff;
      if (d < circleRadius) {
        col = 0xff008080;
        if (mouseLeft) {
          if (!lastMouseLeft) {
            c.selectedPoint = i2;
            c.selectedCV = -1;
            c.dragPoint = i2;
          }
        }
      }
      if (c.selectedPoint == i2) col = 0xffffff00;
      if (c.dragPoint == i2) col = 0xffffff80;
      ImGui::GetWindowDrawList()->AddCircle(p0,circleRadius,col);
    }
    if (showCurveCVs) {
      unsigned int col;
      ImVec2 dp;
      col = 0xff00ffff;
      dp.x = p0.x + c.points[i2].dx[0];
      dp.y = p0.y - c.points[i2].dy[0];
      dx = mouseX - dp.x; dy = mouseY - dp.y; d = sqrt(dx*dx+dy*dy);
      if (d < circleRadius && i2 > 0) {
        col = 0xff008080;
        if (mouseLeft) {
          if (!lastMouseLeft) {
            c.selectedPoint = i2;
            c.dragPoint = i2;
            c.selectedCV = 0;
          }
        }
      }
      if (c.selectedPoint == i2 && c.selectedCV == 0) col = 0xffffff00;
      ImGui::GetWindowDrawList()->AddLine(p0,dp,0xff004040);
      ImGui::GetWindowDrawList()->AddCircle(dp,4,col);
      col = 0xff00ffff;
      dp.x = p0.x + c.points[i2].dx[1];
      dp.y = p0.y - c.points[i2].dy[1];
      dx = mouseX - dp.x; dy = mouseY - dp.y; d = sqrt(dx*dx+dy*dy);
      if (d < circleRadius && i2 < c.points.size() - 1 ) {
        col = 0xff008080;
        if (mouseLeft) {
          if (!lastMouseLeft) {
            c.selectedPoint = i2;
            c.dragPoint = i2;
            c.selectedCV = 1;
          }
        }
      }
      if (c.selectedPoint == i2 && c.selectedCV == 1) col = 0xffffff00;
      ImGui::GetWindowDrawList()->AddLine(p0,dp,0xff004040);
      ImGui::GetWindowDrawList()->AddCircle(dp,4,col);
    }
  }
  lastMouseLeft = mouseLeft;
}

void getGradingArrayFromCurves(unsigned char *dest) {
  for (int j = 0; j < 5; j++) {
    curves[j].generateBezier();
    for (int x = 0; x < 256; x++) {
      int v = curves[j].getInterpolation(x);
      if (v < 0) v = 0;
      if (v > 255) v = 255;
      dest[j*256+x] = (unsigned char)v;
    }
  }
}

void stringToCurves(const String &string) {
  Array<String> l = getLines(&string[0]);
  int currentCurve = -1;
  for (int i = 0; i < l.size(); i++) {
    const String &str = l[i];
    if (str == "C") {
      currentCurve++; 
      if (currentCurve >= 5) break;
      curves[currentCurve].points.clear();
      continue;
    }
    CurvePoint c; CurvePoint *p = &c;
    if (sscanf(&str[0],"P %d %d %f %f %f %f",&p->x,&p->y,&p->dx[0],&p->dy[0],&p->dx[1],&p->dy[1]) == 6) {
      if (currentCurve >= 0) curves[currentCurve].points.push_back(c);
    }
  }
}

String stringFromCurves() {
  String ret;
  for (int i = 0; i < 5; i++) {
    Curve *c = &curves[i];
    ret += "C\n";
    for (int j = 0; j < c->points.size(); j++) {
      CurvePoint *p = &c->points[j];
      sprintf(tempBuffer,"P %d %d %.02f %.02f %.02f %.02f\n",p->x,p->y,p->dx[0],p->dy[0],p->dx[1],p->dy[1]);
      ret += String(tempBuffer);
    }
  }
  return ret;
}

void applyColorGradingCurves(Layer *dest, Layer *source) {
  if (dest->state.locked) return;
  if (dest->width != source->width || dest->height != source->height) return;
  Array<unsigned char> vals; vals.resize(5*256); getGradingArrayFromCurves(&vals[0]);
  bool hasSelection = dest->hasSelection();
  for (int i = 0; i < dest->width*dest->height; i++) {
    if ((!hasSelection)||(dest->pixelStates[i] & PIXELSTATE_SELECTED)) {
      unsigned int colS = source->pixels[i];
      int r = colS & 255;
      int g = (colS>>8) & 255;
      int b = (colS>>16) & 255;
      int a = (colS>>24) & 255;
      int rs = r;
      int gs = g;
      int bs = b;
      r = (int)vals[1*256+rs];
      g = (int)vals[2*256+gs];
      b = (int)vals[3*256+bs];
      a = (int)vals[4*256+a];
      r += (int)vals[0*256+rs]-rs;
      g += (int)vals[0*256+gs]-gs;
      b += (int)vals[0*256+bs]-bs;
      if (r < 0) r = 0;
      if (g < 0) g = 0;
      if (b < 0) b = 0;
      if (a < 0) a = 0;
      if (r > 255) r = 255;
      if (g > 255) g = 255;
      if (b > 255) b = 255;
      if (a > 255) a = 255;
      dest->pixels[i] = r | (g<<8) | (b<<16) | (a<<24);
    }
  }
}

ColorTransform *gradientCurvesToColorTransform = NULL;

void dialogGradiationCurves(bool openIt) {
  static String curveBefore = "";
  static GLuint gradiationTexture = 0;
  static int gradiationChannel = 0;
  static bool updateGradientCurvesPreview = false;
  static const char *channelNames[5] = {"All","Red","Green","Blue","Alpha"};
  static Layer *clone = NULL;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  if (openIt) {
    beforeUndoPoint();
    gradiationChannel = 0;
    if (gradiationTexture != 0) {glDeleteTextures(1,&gradiationTexture); gradiationTexture = 0;}
    glGenTextures(1,&gradiationTexture);
    updateGradiationTexture(gradiationTexture,l,gradiationChannel);
    if (!reOpen) {
      for (int i = 0; i < 5; i++) {
        curves[i].clear();
        curves[i].points.push_back(CurvePoint(0,0));
        curves[i].points.push_back(CurvePoint(255,255));
      }
      if (gradientCurvesToColorTransform == NULL) {
        if (clone != NULL) {delete clone; clone = NULL;}
        clone = l->clone();
      } else {
        curveBefore = gradientCurvesToColorTransform->gradientCurves;
        stringToCurves(curveBefore);
      }
    }
    reOpen = false;
    updateGradientCurvesPreview = true;
  }
  DIALOGSTART("Gradiation curves");
  if (roundButton("Reset##DIALOGGRADIATIONRESET",true)) {
    for (int i = 0; i < 5; i++) {
      curves[i].clear();
      curves[i].points.push_back(CurvePoint(0,0));
      curves[i].points.push_back(CurvePoint(255,255));
    }
    updateGradientCurvesPreview = true;
  }
  ImGui::SameLine(); 
  if (roundButton("Load##DIALOGGRADIATIONLOAD",true)) {
    dialogLoadSave(true,DIALOG_LOADGRADIENTCURVES);
  }
  ImGui::SameLine(); 
  if (roundButton("Save##DIALOGGRADIATIONLOAD",true)) {
    dialogLoadSave(true,DIALOG_SAVEGRADIENTCURVES);
  }
  ImGui::BeginChild("DIALOGGRADIENTCURVESCHILD",ImVec2(gradiationTextureWidth,gradiationTextureHeight));
  ImGui::Image((ImTextureID)gradiationTexture,ImVec2(gradiationTextureWidth,gradiationTextureHeight));
  ImVec2 minB = ImGui::GetItemRectMin();
  ImVec2 maxB = ImGui::GetItemRectMax();
  drawCurve(minB,maxB,curves[gradiationChannel]);
  ImGui::EndChild();
  ImVec2 minB2 = minB;
  minB2.x -= 1;
  minB2.y -= 1;
  ImGui::GetWindowDrawList()->AddRect(minB2,maxB,0xff808080);
  ImGui::PushItemWidth(64+16*2);
  int colorIndex = (mouseX - minB.x)*256/(maxB.x-minB.x);
  int colorValue = 255-(mouseY - minB.y)*256/(maxB.y-minB.y);
  if (colorIndex < 0) colorIndex = 0;
  if (colorValue < 0) colorValue = 0;
  if (colorIndex > 255) colorIndex = 255;
  if (colorValue > 255) colorValue = 255;
  int colorIndex2 = colorIndex;
  int colorValue2 = colorValue;
  Curve &c = curves[gradiationChannel];
  if ((unsigned int)c.selectedPoint < c.points.size()) {
    CurvePoint &p = c.points[c.selectedPoint];
    colorIndex2 = p.x;
    colorValue2 = p.y;
  }
  bool pointUpdated = false;
  ImGui::Text("Index"); ImGui::SameLine(); if (ImGui::InputInt("##DIALOGGRADIATIONINDEX",&colorIndex2,1,1)) {colorIndex = colorIndex2; colorValue = colorValue2; pointUpdated = true;}
  ImGui::SameLine(); ImGui::Text("Value"); ImGui::SameLine(); if (ImGui::InputInt("##DIALOGGRADIATIONVALUE",&colorValue2,1,1)) {colorIndex = colorIndex2; colorValue = colorValue2; pointUpdated = true;}
  if (colorIndex < 0) colorIndex = 0;
  if (colorValue < 0) colorValue = 0;
  if (colorIndex > 255) colorIndex = 255;
  if (colorValue > 255) colorValue = 255;
  if (pointUpdated) {
    updateGradientCurvesPreview = true;
    CurvePoint &p = c.points[c.selectedPoint];
    if (c.selectedPoint == 0 || c.selectedPoint == c.points.size()-1) {
      p.y = colorValue;
    } else {
      CurvePoint &pm = c.points[c.selectedPoint-1];
      CurvePoint &pp = c.points[c.selectedPoint+1];
      if (colorIndex >= pp.x) colorIndex = pp.x-1;
      if (colorIndex <= pm.x) colorIndex = pm.x+1;
      p.x = colorIndex;
      p.y = colorValue;
    }
  }

  if ((unsigned int)c.dragPoint < c.points.size()) {
    CurvePoint &p = c.points[c.dragPoint];
    updateGradientCurvesPreview = true;
    if (c.selectedCV < 0) {
      if (c.dragPoint == 0 || c.dragPoint == c.points.size()-1) {
        p.y = colorValue;
      } else {
        CurvePoint &pm = c.points[c.dragPoint-1];
        CurvePoint &pp = c.points[c.dragPoint+1];
        if (colorIndex >= pp.x) colorIndex = pp.x-1;
        if (colorIndex <= pm.x) colorIndex = pm.x+1;
        p.x = colorIndex;
        p.y = colorValue;
      }
    } else {
      p.dx[c.selectedCV] = colorIndex - p.x;
      p.dy[c.selectedCV] = colorValue - p.y;
    }
  }

  if (ImGui::IsMouseClicked(1)) {
    if (c.dragPoint<0&&(!showCurveCVs))
      c.insertPoint(mouseX-minB.x);
  }

  dialogNoMove = false;
  for (int i = 0; i < 5; i++) if (curves[i].dragPoint>=0) dialogNoMove = true;
  ImGui::Text("CVs"); ImGui::SameLine(); ImGui::Checkbox("##DIALOGGRADIATIONCVS",&showCurveCVs);
  ImGui::SameLine(); ImGui::Text("Channel");ImGui::SameLine(); if (ImGui::Combo("##DIALOGGRADIATIONCHANNEL",&gradiationChannel, channelNames, 5)) {updateGradiationTexture(gradiationTexture,l,gradiationChannel);}
  ImGui::PopItemWidth();
  ImGui::SameLine(); if (roundButton("Delete##DIALOGGRADIATIONDELETE",c.selectedPoint>0 && c.selectedPoint<c.points.size()-1)) {c.erasePoint(c.selectedPoint); updateGradientCurvesPreview = true;}

  if (updateGradientCurvesPreview) {
    if (gradientCurvesToColorTransform == NULL) {
      updateGradientCurvesPreview = false;
      applyColorGradingCurves(l,clone);
      repaintLayer(l,true);
    } else {
      String c = stringFromCurves();
      gradientCurvesToColorTransform->gradientCurves = c;
      gradientCurvesToColorTransform->shouldUpdate = true;
      repaintLayer(l,true);
    }
  }

  if (ImGui::Button("Apply##DIALOGGRADIATIONCURVEAPPLY")) {
    if (gradientCurvesToColorTransform == NULL) {
      if (clone != NULL) {delete clone; clone = NULL;}
    }
    dialogNoMove = false;
    if (gradiationTexture != 0) {glDeleteTextures(1,&gradiationTexture); gradiationTexture = 0;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    updatedPixels(l);
    afterUndoPoint();
    gradientCurvesToColorTransform = NULL;
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGGRADIATIONCURVESCLOSE")) {
    if (gradientCurvesToColorTransform == NULL) {
      if ((!l->state.locked) && clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
      if (clone != NULL) {delete clone; clone = NULL;}
    } else {
      curveBefore = gradientCurvesToColorTransform->gradientCurves;
    }
    dialogNoMove = false;
    if (gradiationTexture != 0) {glDeleteTextures(1,&gradiationTexture); gradiationTexture = 0;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    gradientCurvesToColorTransform = NULL;
  }
  DIALOGEND()
}

void dialogColorBalance(bool openIt) {
  static Layer *clone = NULL;
  static int channel = 0;
  static float colorBalanceR[3];
  static float colorBalanceG[3];
  static float colorBalanceB[3];
  static bool keepLuminance = true;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  if (openIt) {
    beforeUndoPoint();
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    channel = 1;
    for (int i = 0; i < 3; i++) {
      colorBalanceR[i] = 0;
      colorBalanceG[i] = 0;
      colorBalanceB[i] = 0;
    }
    keepLuminance = true;
  }
  DIALOGSTART("Color balance");

  bool updateImage = false;
  float *k;
  ImGui::PushItemWidth(200);
  k = &colorBalanceR[channel]; ImGui::Text("purple - red - yellow"); if (ImGui::SliderFloat("##DIALOGCOLORBALANCEPRY",k,-1,1)) {updateImage = true;} if (*k<-1) *k = -1; if (*k>1) *k = 1;
  k = &colorBalanceG[channel]; ImGui::Text("yellow - green - cyan"); if (ImGui::SliderFloat("##DIALOGCOLORBALANCEGYC",k,-1,1)) {updateImage = true;} if (*k<-1) *k = -1; if (*k>1) *k = 1;
  k = &colorBalanceB[channel]; ImGui::Text("cyan - blue - purple"); if (ImGui::SliderFloat("##DIALOGCOLORBALANCECBP",k,-1,1)) {updateImage = true;} if (*k<-1) *k = -1; if (*k>1) *k = 1;
  ImGui::Text("");   
  ImGui::Text("Shadows"); ImGui::SameLine(); ImGui::RadioButton("##DIALOGCOLORBALANCECHANNEL0", &channel, 0);
  ImGui::SameLine();ImGui::Text("Mids"); ImGui::SameLine(); ImGui::RadioButton("##DIALOGCOLORBALANCECHANNEL1", &channel, 1);
  ImGui::SameLine();ImGui::Text("Lights"); ImGui::SameLine(); ImGui::RadioButton("##DIALOGCOLORBALANCECHANNEL2", &channel, 2);
  ImGui::Text("Keep luminance"); ImGui::SameLine(); if (ImGui::Checkbox("##DIALOGCOLORBALANCEKEEPLUMINANCE", &keepLuminance)) {updateImage = true;}
  ImGui::PopItemWidth();

  if (updateImage) {
    if ((!l->state.locked) && l->height == clone->height && l->width == clone->width) {
      bool hasSelection = l->hasSelection();
      for (int i = 0; i < l->width * l->height; i++) {
        if ((!hasSelection) || (l->pixelStates[i] & PIXELSTATE_SELECTED)) {
          unsigned int col = clone->pixels[i];
          float r = col & 255;
          float g = (col>>8) & 255;
          float b = (col>>16) & 255;
          float lumaStart = (r+g+b)/3.0;
          float rd = 0;
          float gd = 0;
          float bd = 0;
          float s;
          for (int j = 0; j < 3; j++) {
            float ra = 0, ga = 0, ba = 0;
            s = colorBalanceR[j]; ra += r; if (s>0) {ga += r * s;} else {ba += r * -s;}
            s = colorBalanceG[j]; ga += g; if (s>0) {ba += g * s;} else {ra += g * -s;}
            s = colorBalanceB[j]; ba += b; if (s>0) {ra += b * s;} else {ga += b * -s;}
            float f = lumaStart/255.f;
            switch(j) {
            case 0: {f = 1-f*2;} break;
            case 1: {f = 1-fabs(f-0.5)*2;} break;
            case 2: {f = 1-(1-f)*2;} break;
            }
            if (f < 0) f = 0;
            if (f > 1) f = 1;
            rd += ra * f;
            gd += ga * f;
            bd += ba * f;
          }
          r = rd;
          g = gd;
          b = bd;
          float lumaEnd =  (r+g+b)/3.0;
          if (keepLuminance && lumaEnd != 0) {
            r *= lumaStart/lumaEnd;
            g *= lumaStart/lumaEnd;
            b *= lumaStart/lumaEnd;
          }
          int a = (col >> 24) & 255;
          int ri = (int)r;
          int gi = (int)g;
          int bi = (int)b;
          if (ri < 0) ri = 0; if (ri > 255) ri = 255;
          if (gi < 0) gi = 0; if (gi > 255) gi = 255;
          if (bi < 0) bi = 0; if (bi > 255) bi = 255;
          l->pixels[i] = ri|(gi<<8)|(bi<<16)|(a<<24);        
        }
      }
      repaintLayer(l,true);
    }
  }

  if (ImGui::Button("Apply##DIALOGCOLORBALANCEAPPLY")) {
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    updatedPixels(l);
    afterUndoPoint();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGCOLORBALANCECLOSE")) {
    if ((!l->state.locked) && clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
  }
  DIALOGEND()
}

enum BLURTYPES {
  BLURTYPE_GAUSS = 0,
  BLURTYPE_BOX = 1,
  BLURTYPE_RADIAL = 2,
  BLURTYPE_ITERATIVE = 3,
  BLURTYPE_COUNT = 4
};

void dialogBlur(bool openIt) {
  static Layer *clone = NULL;
  static int boxWidth = 0;
  static int boxHeight = 0;
  static float gaussX = 0;
  static float  gaussY = 0;
  static float radialAngle = 0;
  static float radialSize = 0;
  static int iterativeSteps = 0;
  static int blurType = 0;
  static const char *blurTypes[BLURTYPE_COUNT] = {"Gauss blur", "Box blur", "Radial blur","Iterative"};
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  static unsigned int maxBlurPixels = 100;

  static bool updateBlur = false;

  if (openIt) {
    beforeUndoPoint();
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    boxWidth = 4;
    boxHeight = 4;
    radialAngle = 0;
    radialSize = 4;
    gaussX = 2;
    gaussY = 2; 
    iterativeSteps = 1;
    blurType = BLURTYPE_GAUSS;
    updateBlur = true;
  }
  DIALOGSTART("Blur");

  ImGui::PushItemWidth(7*15);
  ImGui::Text("Type");ImGui::SameLine(); if (ImGui::Combo("##DIALOGBLURTYPE",&blurType, blurTypes, BLURTYPE_COUNT)) updateBlur = true;
  ImGui::PopItemWidth();

  ImGui::PushItemWidth(64);

  switch(blurType) {
    case BLURTYPE_BOX: {
      ImGui::Text("Box blur settings");
      ImGui::Text("Radius X"); ImGui::SameLine(); if (ImGui::InputInt("##DIALOGBLURBOXWIDTH",&boxWidth,0,0)) {updateBlur = true;} if (boxWidth<0) boxWidth = 0; if (boxWidth>maxBlurPixels) boxWidth = maxBlurPixels;
      ImGui::Text("Radius Y"); ImGui::SameLine(); if (ImGui::InputInt("##DIALOGBLURBOXHEIGHT",&boxHeight,0,0)) {updateBlur = true;} if (boxHeight<0) boxHeight = 0; if (boxHeight>maxBlurPixels) boxHeight = maxBlurPixels;
    } break;
    case BLURTYPE_RADIAL: {
      ImGui::Text("Radial blur settings");
      ImGui::PushItemWidth(360);
      ImGui::Text("Angle"); ImGui::SameLine(); if (ImGui::SliderFloat("##DIALOGBLURRADIALANGLE",&radialAngle,0,360)) {updateBlur = true;} if (radialAngle<0) radialAngle = 0; if (radialAngle>360) radialAngle = 360;
      ImGui::PopItemWidth();
      ImGui::Text("Radius"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGBLURRADIALRADIUS",&radialSize,0,0,0)) {updateBlur = true;} if (radialSize<0) radialSize = 0; if (radialSize>maxBlurPixels) radialSize = maxBlurPixels;
    } break;
    case BLURTYPE_GAUSS: {
      ImGui::Text("Gauss blur settings");
      ImGui::Text("Radius X"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGBLURGAUSSX",&gaussX,0,0,2)) {updateBlur = true;} if (gaussX<0) gaussX = 0; if (gaussX>16384) gaussX = 16384;
      ImGui::Text("Radius Y"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGBLURGAUSSY",&gaussY,0,0,2)) {updateBlur = true;} if (gaussY<0) gaussY = 0; if (gaussY>16384) gaussY = 16384;
    } break;
    case BLURTYPE_ITERATIVE: {
      ImGui::Text("Iterative blur settings");
      ImGui::Text("Steps"); ImGui::SameLine(); if (ImGui::InputInt("##DIALOGBLURITERATIVE",&iterativeSteps,0,0)) {updateBlur = true;} if (iterativeSteps<0) iterativeSteps = 0; if (iterativeSteps>32) iterativeSteps = 32;
    } break;
  }
  
  ImGui::PopItemWidth();

  if (updateBlur) {
    if (!ImGui::IsAnyItemActive()) {
      updateBlur = false;
      if (clone->width == l->width && clone->height == l->height && (!l->state.locked)) {
        switch(blurType) {
          case BLURTYPE_BOX: {filterBoxBlur(l->pixels,clone->pixels, l->width, l->height, boxWidth, boxHeight,l->pixelStates,l->hasSelection());} break;
          case BLURTYPE_RADIAL: {filterRadialBlur(l->pixels,clone->pixels, l->width, l->height, radialAngle, radialSize,l->pixelStates,l->hasSelection());} break;
          case BLURTYPE_GAUSS: {filterGaussBlur(l->pixels,clone->pixels, l->width, l->height, gaussX, gaussY,l->pixelStates,l->hasSelection());} break;
          case BLURTYPE_ITERATIVE: {filterIterativeBlur(l->pixels,clone->pixels, l->width, l->height, iterativeSteps, l->pixelStates,l->hasSelection());} break;
        }
        repaintLayer(l,true);
      }
    }
  }

  if (ImGui::Button("Apply##DIALOGBLURAPPLY")) {
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    updatedPixels(l);
    afterUndoPoint();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGBLURCLOSE")) {
    if ((!l->state.locked) && clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
  }
  DIALOGEND()
}

void dialogGlow(bool openIt) {
  static Layer *clone = NULL;
  static float gaussX = 0;
  static float  gaussY = 0;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  static bool updateBlur = false;

  if (openIt) {
    beforeUndoPoint();
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    gaussX = 16;
    gaussY = 16;
    updateBlur = true;
  }
  DIALOGSTART("Turn to glow");

  ImGui::PushItemWidth(64);
  ImGui::Text("Radius X"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGGLOWGAUSSX",&gaussX,0,0,2)) {updateBlur = true;} if (gaussX<0) gaussX = 0; if (gaussX>16384) gaussX = 16384;
  ImGui::Text("Radius Y"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGGLOWGAUSSY",&gaussY,0,0,2)) {updateBlur = true;} if (gaussY<0) gaussY = 0; if (gaussY>16384) gaussY = 16384;
  ImGui::PopItemWidth();

  if (updateBlur) {
    if (!ImGui::IsAnyItemActive()) {
      updateBlur = false;
      if (clone->width == l->width && clone->height == l->height && (!l->state.locked)) {
        filterGaussBlur(l->pixels,clone->pixels, l->width, l->height, gaussX, gaussY,l->pixelStates,l->hasSelection());
        l->greyScale();
        repaintLayer(l,true);
      }
    }
  }
  if (ImGui::Button("Apply##DIALOGGLOWAPPLY")) {
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    updatedPixels(l);
    afterUndoPoint();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGGLOWCLOSE")) {
    if ((!l->state.locked) && clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
  }
  DIALOGEND()
}

void dialogEmboss(bool openIt) {
  static Layer *clone = NULL;
  static float angle = 0;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;

  static bool updateEmboss = false;

  if (openIt) {
    beforeUndoPoint();
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    angle = 0;
    updateEmboss = true;
  }
  DIALOGSTART("Emboss");

  ImGui::PushItemWidth(360);
  ImGui::Text("Angle"); ImGui::SameLine(); if (ImGui::SliderFloat("##DIALOGEMBOSSANGLE",&angle,0,360)) {updateEmboss = true;} if (angle<0) angle = 0; if (angle>360) angle = 360;
  ImGui::PopItemWidth();
  
  if (updateEmboss) {
    if (!ImGui::IsAnyItemActive()) {
      updateEmboss = false;
      if (clone->width == l->width && clone->height == l->height && (!l->state.locked)) {
        filterEmboss(l->pixels,clone->pixels, l->width, l->height, angle, l->pixelStates,l->hasSelection());
        repaintLayer(l,true);
      }
    }
  }

  if (ImGui::Button("Apply##DIALOGEMBOSSAPPLY")) {
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    updatedPixels(l);
    afterUndoPoint();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGEMBOSSCLOSE")) {
    if ((!l->state.locked) && clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
  }
  DIALOGEND()
}

void dialogColorMatrix(bool openIt) {
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;

  DIALOGSTART("Color matrix");

  ImGui::PushItemWidth(7*7);

  float *v; bool update = false;
  ImGui::Text("Channel red"); 
  ImGui::SameLine(); ImGui::Text(""); ImGui::SameLine(); if (roundButton("Normalize####DIALOGCOLORMATRIXNORMALIZER",true)) {
    update = true;
    Vector k(l->colorTransform.colorMatrix[0*4+0],l->colorTransform.colorMatrix[0*4+1],l->colorTransform.colorMatrix[0*4+2]);
    k = normalize(k);
    l->colorTransform.colorMatrix[0*4+0] = k.x;
    l->colorTransform.colorMatrix[0*4+1] = k.y;
    l->colorTransform.colorMatrix[0*4+2] = k.z;
  }
  v = &l->colorTransform.colorMatrix[0*4+0]; ImGui::Text("Into red"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXRR",v,0,0,2)) {update = true;}
  v = &l->colorTransform.colorMatrix[0*4+1]; ImGui::Text("Into green"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXRG",v,0,0,2)) {update = true;}
  v = &l->colorTransform.colorMatrix[0*4+2]; ImGui::Text("Into blue"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXRB",v,0,0,2)) {update = true;}

  ImGui::Text("");
  ImGui::Text("Channel green");
  ImGui::SameLine(); ImGui::Text(""); ImGui::SameLine(); if (roundButton("Normalize####DIALOGCOLORMATRIXNORMALIZEG",true)) {
    update = true;
    update = true;
    Vector k(l->colorTransform.colorMatrix[1*4+0],l->colorTransform.colorMatrix[1*4+1],l->colorTransform.colorMatrix[1*4+2]);
    k = normalize(k);
    l->colorTransform.colorMatrix[1*4+0] = k.x;
    l->colorTransform.colorMatrix[1*4+1] = k.y;
    l->colorTransform.colorMatrix[1*4+2] = k.z;
  }
  v = &l->colorTransform.colorMatrix[1*4+0]; ImGui::Text("Into red"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXGR",v,0,0,2)) {update = true;}
  v = &l->colorTransform.colorMatrix[1*4+1]; ImGui::Text("Into green"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXGG",v,0,0,2)) {update = true;}
  v = &l->colorTransform.colorMatrix[1*4+2]; ImGui::Text("Into blue"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXGB",v,0,0,2)) {update = true;}

  ImGui::Text("");
  ImGui::Text("Channel blue");
  ImGui::SameLine(); ImGui::Text(""); ImGui::SameLine(); if (roundButton("Normalize####DIALOGCOLORMATRIXNORMALIZEB",true)) {
    update = true;
    update = true;
    Vector k(l->colorTransform.colorMatrix[2*4+0],l->colorTransform.colorMatrix[2*4+1],l->colorTransform.colorMatrix[2*4+2]);
    k = normalize(k);
    l->colorTransform.colorMatrix[2*4+0] = k.x;
    l->colorTransform.colorMatrix[2*4+1] = k.y;
    l->colorTransform.colorMatrix[2*4+2] = k.z;
  }
  v = &l->colorTransform.colorMatrix[2*4+0]; ImGui::Text("Into red"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXBR",v,0,0,2)) {update = true;}
  v = &l->colorTransform.colorMatrix[2*4+1]; ImGui::Text("Into green"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXBG",v,0,0,2)) {update = true;}
  v = &l->colorTransform.colorMatrix[2*4+2]; ImGui::Text("Into blue"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXBB",v,0,0,2)) {update = true;}

  ImGui::Text("");
  ImGui::Text("Channel alpha");
  v = &l->colorTransform.colorMatrix[3*4+3]; ImGui::Text("Scale alpha"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXA",v,0,0,2)) {update = true;}

  ImGui::Text("");
  v = &l->colorTransform.colorMatrix[4*4+0]; ImGui::Text("Add to red"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXADDR",v,0,0,2)) {update = true;}
  v = &l->colorTransform.colorMatrix[4*4+1]; ImGui::Text("Add to green"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXADDG",v,0,0,2)) {update = true;}
  v = &l->colorTransform.colorMatrix[4*4+2]; ImGui::Text("Add to blue"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXADDB",v,0,0,2)) {update = true;}
  v = &l->colorTransform.colorMatrix[4*4+3]; ImGui::Text("Add to alpha"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXADDA",v,0,0,2)) {update = true;}

  ImGui::PopItemWidth();

  if (update) {
    l->colorTransform.shouldUpdate = true;
    repaintLayer(l,true);
  }
  
  if (ImGui::Button("Close##DIALOGCOLORMATRIXCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void getQuantizedPaletteKMeans(Array<unsigned int> &palette, int colorCount, Layer *l, bool luminance, int steps) {
  Layer *cloned = l->clone();
  Array<unsigned int> colorsWithClusterId;
  int sizeIncrease = 1024*2;
  int currentIndex = 0;
  int currentSize = 0;
  Array<unsigned char> alreadyFound;
  alreadyFound.resize(0x1000000/8); // 256*256*256/8 = 64kb*32 = 2mb
  memset(&alreadyFound[0],0,alreadyFound.size()*sizeof(unsigned char));
  for (int i = 0; i < cloned->width*cloned->height; i++) {
    unsigned int c = cloned->pixels[i];
    int k = c & 0xffffff;
    int pos8 = k / 8;
    int pos7 = k & 7;
    unsigned char b = (unsigned char)(alreadyFound[pos8] & (1<<pos7));
    if (!b) {   
      alreadyFound[pos8] |= (unsigned char)(1<<pos7);
      if (currentIndex+2 >= currentSize) {
        colorsWithClusterId.resize(colorsWithClusterId.size()+sizeIncrease);
      }
      colorsWithClusterId[currentIndex+0] = c;
      colorsWithClusterId[currentIndex+1] = 0;
      currentIndex += 2;
    }
  }
  colorsWithClusterId.resize(currentIndex);
  if (colorsWithClusterId.size()/2<colorCount) colorCount = colorsWithClusterId.size()/2;
  if (colorCount == 0) return;

#define KMEANSCHANNELS 4
  Array<unsigned int> clusterColorsRGBAW; clusterColorsRGBAW.resize(colorCount*KMEANSCHANNELS);
  srand(0); for (int i2 = 0; i2 < clusterColorsRGBAW.size(); i2++) clusterColorsRGBAW[i2] = (i2/KMEANSCHANNELS)*255/colorCount;

  float sr = luminance ? 0.25 : 1;
  float sg = luminance ? 0.6 : 1;
  float sb = luminance ? 0.15 : 1;

  initStatusBar();
  for (int step = 0; step < steps; step++) {

    for (int i3 = 0; i3 < colorsWithClusterId.size()/2; i3++) {
      unsigned int col = colorsWithClusterId[i3*2+0];
      float r = col & 255;
      float g = (col>>8) & 255;
      float b = (col>>16) & 255;
      r /= sr;
      g /= sg;
      b /= sb;
      float ld = 256*256*256;
      int cluster = 0;
      for (int i4 = 0; i4 < clusterColorsRGBAW.size()/KMEANSCHANNELS; i4++) {
        float r2 = clusterColorsRGBAW[i4*KMEANSCHANNELS+0];
        float g2 = clusterColorsRGBAW[i4*KMEANSCHANNELS+1];
        float b2 = clusterColorsRGBAW[i4*KMEANSCHANNELS+2];
        r2 /= sr;
        g2 /= sg;
        b2 /= sb;
        float dr = r2 - r;
        float dg = g2 - g;
        float db = b2 - b;
        float d = dr * dr + dg * dg + db * db;
        if (d < ld) {
          ld = d;
          cluster = i4;
        }
      }
      colorsWithClusterId[i3*2+1] = cluster;
    }
  
    for (int i5 = 0; i5 < clusterColorsRGBAW.size(); i5++) clusterColorsRGBAW[i5] = 0;
  
    for (int i6 = 0; i6 < colorsWithClusterId.size()/2; i6++) {
      unsigned int col = colorsWithClusterId[i6*2+0];
      unsigned int c = colorsWithClusterId[i6*2+1];
      int r = col & 255;
      int g = (col>>8) & 255;
      int b = (col>>16) & 255;
      int a = (col>>24) & 255;
      clusterColorsRGBAW[c*KMEANSCHANNELS+0] += r;
      clusterColorsRGBAW[c*KMEANSCHANNELS+1] += g;
      clusterColorsRGBAW[c*KMEANSCHANNELS+2] += b;
      clusterColorsRGBAW[c*KMEANSCHANNELS+3] += 1;
    }
  
    for (int i7 = 0; i7 < clusterColorsRGBAW.size()/KMEANSCHANNELS; i7++) {
      int w = clusterColorsRGBAW[i7*KMEANSCHANNELS+3];
      if (w != 0) {
        clusterColorsRGBAW[i7*KMEANSCHANNELS+0] /= w;
        clusterColorsRGBAW[i7*KMEANSCHANNELS+1] /= w;
        clusterColorsRGBAW[i7*KMEANSCHANNELS+2] /= w;
      }
    }

    for (int i9 = 0; i9 < clusterColorsRGBAW.size()/KMEANSCHANNELS; i9++) {
      int r0 = clusterColorsRGBAW[i9*KMEANSCHANNELS+0];
      int g0 = clusterColorsRGBAW[i9*KMEANSCHANNELS+1];
      int b0 = clusterColorsRGBAW[i9*KMEANSCHANNELS+2];
      for (int i10 = i9+1; i10 < clusterColorsRGBAW.size()/KMEANSCHANNELS; i10++) {
        int r1 = clusterColorsRGBAW[i10*KMEANSCHANNELS+0];
        int g1 = clusterColorsRGBAW[i10*KMEANSCHANNELS+1];
        int b1 = clusterColorsRGBAW[i10*KMEANSCHANNELS+2];
        if (r1 != r0) continue;
        if (g1 != g0) continue;
        if (b1 != b0) continue;
        clusterColorsRGBAW[i10*KMEANSCHANNELS+0] = rand() & 255;
        clusterColorsRGBAW[i10*KMEANSCHANNELS+1] = rand() & 255;
        clusterColorsRGBAW[i10*KMEANSCHANNELS+2] = rand() & 255;
      }
    }
    updateStatusBar();
  }

  palette.resize(clusterColorsRGBAW.size()/KMEANSCHANNELS);
  for (int i8 = 0; i8 < palette.size(); i8++) {
    int r = clusterColorsRGBAW[i8*KMEANSCHANNELS+0];
    int g = clusterColorsRGBAW[i8*KMEANSCHANNELS+1];
    int b = clusterColorsRGBAW[i8*KMEANSCHANNELS+2];
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    palette[i8] = r|(g<<8)|(b<<16)|0xff000000;
  }

  delete cloned;
}

Array<ImVec4> lastQuantizedPalette;

void dialogQuantizePalette(bool openIt) {
  static Layer *clone = NULL;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  static int colorCount = 0;
  static bool useLuminance = false;
  if (openIt) {
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    colorCount = 256;
    useLuminance = false;
  }
  DIALOGSTART("Quantized palette");

  ImGui::PushItemWidth(64);
  ImGui::Text("Quantization using KMeans (not precise)");
  ImGui::Text("Number of colors"); ImGui::SameLine(); ImGui::InputInt("##DIALOGQUANTIZEPALETTECOLORCOUNT",&colorCount,0,0); if (colorCount < 1) colorCount = 1; if (colorCount > 256) colorCount = 256;
  ImGui::Text("Use luminance"); ImGui::SameLine(); ImGui::Checkbox("##DIALOGQUANTIZEPALETTELUMINANCE",&useLuminance);
  ImGui::PopItemWidth();

  if (roundButton("Preview",true)) {
    Array<unsigned int> palette;
    getQuantizedPaletteKMeans(palette, colorCount, clone, useLuminance,32);
    if (l->width == clone->width && l->height == clone->height) {
      memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
      l->quantize(palette);
      repaintLayer(l,true);
    }
  }

  if (ImGui::Button("Save palette##DIALOGQUANTIZEPALETTESAVE")) {
    Array<unsigned int> palette;
    getQuantizedPaletteKMeans(palette, colorCount, clone, useLuminance,32);
    static Array<ImVec4> quantizedPalette;
    quantizedPalette.resize(palette.size());
    for (int i = 0; i < quantizedPalette.size(); i++) {
      quantizedPalette[i] = getImVec4Color(palette[i]);
    }
    memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    sortPalette(quantizedPalette);
    paletteToSave = &quantizedPalette; dialogLoadSave(true,DIALOG_SAVEPALETTE);
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGQUANTIZEPALETTECLOSE")) {
    memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogDuplicate(bool openIt) {
  DIALOGSTART("Duplicate layers");

  Array<Layer *> layers;
  bool one = false; for (int i = 0; i < currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected) {one = true; layers.push_back(currentCanvas->layers[i]);}
  if (!one) for (int i2 = 0; i2 < currentCanvas->layers.size(); i2++) if (currentCanvas->layers[i2]->state.active) {layers.push_back(currentCanvas->layers[i2]);}

  if (layers.empty()) {
    ImGui::Text("No layers selected or active");
  } else {
    noUndoMessage();
    ImGui::Text("Layers to duplicate");
    ImGui::BeginChild("DUPLICATELAYERSCHILD",ImVec2(main_window_xres/2,main_window_yres/3));
    for (int i = 0; i < layers.size(); i++) {
      ImGui::Text("\"%s\"",layers[i]->name);
    }
    ImGui::EndChild();
  }

  if (!layers.empty()) {
    if (ImGui::Button("Duplicate these layers##DIALOGDUPLICATELAYERSDUPLICATE")) {
      currentCanvas->duplicateLayers();
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGDUPLICATELAYERSCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogExtrudeSelection(bool openIt) {
  static Layer *clone = NULL;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  static bool updateIt = false;
  static float angle = 0;
  static float size = 10;
  const float maxLength = 100;
  if (openIt) {
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    memcpy(clone->pixelStates,l->pixelStates,clone->width*clone->height*sizeof(unsigned char)); // pixelStates get not cloned by default
    beforeUndoPoint();
    updateIt = true;
  }
  DIALOGSTART("Extrude selection");

  if (!clone->hasSelection()) {
    ImGui::Text("Nothing selected on this layer.");
  } else {
    ImGui::PushItemWidth(360);
    ImGui::Text("Angle"); ImGui::SameLine(); if (ImGui::SliderFloat("##DIALOGEXTRUDESELECTIONANGLE",&angle,0,360)) {updateIt = true;} if (angle<0) angle = 0; if (angle>360) angle = 360;
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(7*8);
    ImGui::Text("Length"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGEXTRUDESELECTIONLENGTH",&size,0,0,0)) {updateIt = true;} if (size<0) size = 0; if (size>maxLength) size = maxLength;
    ImGui::PopItemWidth();

    if (updateIt) {
      if (!ImGui::IsAnyItemActive()) {
        updateIt = false;
        if (clone->width==l->width&&clone->height==l->height) {
          double sx = sin(-angle*2*PI/360.0);
          double sy = cos(-angle*2*PI/360.0);
          int s = (int)floor(size);
          initStatusBar();
          for (int y = 0; y < l->height; y++) {
            if ((y & 63)==0) updateStatusBar();
            for (int x = 0; x < l->width; x++) {
              bool selected = false;
              for (int i = s-1; i >= 0; i--) {
                int lx = (int)(x + sx * i);
                int ly = (int)(y + sy * i);
                if ((unsigned int)lx < clone->width && (unsigned int)ly < clone->height) {
                  unsigned char p = clone->pixelStates[lx+ly*clone->width];
                  selected = p & PIXELSTATE_SELECTED;
                  if (selected) break;
                }
              }
              l->pixelStates[x+y*l->width] &= ~PIXELSTATE_SELECTED;
              if (selected) l->pixelStates[x+y*l->width] |= PIXELSTATE_SELECTED;
            }
          }
        }
        repaintLayer(l,true);
      }
    }

  
    if (ImGui::Button("Apply##DIALOGEXTRUDESELECTIONAPPLY")) {
      if (clone != NULL) {delete clone; clone = NULL;}
      ImGui::CloseCurrentPopup();
      shouldUpdateAll = true;
      updatedPixelStates(l);
      afterUndoPoint();
    }
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGEXTRUDESELECTIONCLOSE")) {
    if (clone->width==l->width&&clone->height==l->height) memcpy(l->pixelStates,clone->pixelStates,l->width*l->height*sizeof(unsigned char));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
  }
  DIALOGEND()
}

void dialogKeying(bool openIt) {
  static Layer *clone = NULL;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  static bool updateIt = false;
  static bool binary = false;
  static bool invert = false;
  static int channel = 0;
  static float factor = 0;
  if (openIt) {
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    beforeUndoPoint();
    updateIt = true;
    channel = 1;
    factor = 4;
    binary = false;
    invert = false;
  }
  DIALOGSTART("Keying");
  ImGui::Text("Simple channel keying");
  ImGui::Text("Red"); ImGui::SameLine(); if (ImGui::RadioButton("##DIALOGKEYINGCHANNEL0", &channel, 0)) {updateIt = true;}
  ImGui::Text("Green"); ImGui::SameLine(); if (ImGui::RadioButton("##DIALOGKEYINGCHANNEL1", &channel, 1)) {updateIt = true;}
  ImGui::Text("Blue"); ImGui::SameLine(); if (ImGui::RadioButton("##DIALOGKEYINGCHANNEL2", &channel, 2)) {updateIt = true;}
  ImGui::PushItemWidth(300);
  ImGui::Text("Factor"); if (ImGui::SliderFloat("##DIALOGKEYINGFACTOR",&factor,0,8)) {updateIt = true;} if (factor<0) factor = 0;
  ImGui::PopItemWidth();
  ImGui::Text("Binary");ImGui::SameLine(); if (ImGui::Checkbox("##DIALOGKEYINGBINARY",&binary)) {updateIt = true;}
  ImGui::SameLine();ImGui::Text("Invert");ImGui::SameLine(); if (ImGui::Checkbox("##DIALOGKEYINGINVERT",&invert)) {updateIt = true;}

  if (updateIt) {
    if (!ImGui::IsAnyItemActive()) {
      updateIt = false;
      if (clone->width == l->width && clone->height == l->height && (!l->state.locked)) {
        filterSimpleKeying(l->pixels,clone->pixels, l->width, l->height, channel, factor, l->pixelStates,l->hasSelection(),l->mode,binary,invert);
        repaintLayer(l,true);
      }
    }
  }

  if (ImGui::Button("Apply##DIALOGKEYINGAPPLY")) {
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    updatedPixels(l);
    afterUndoPoint();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGKEYINGCLOSE")) {
    if (clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
  }
  DIALOGEND()
}

void dialogSelectByAlpha(bool openIt) {
  static int alphaThresholdMin = 0;
  static int alphaThresholdMax = 128;
  DIALOGSTART("Select by alpha");
  ImGui::PushItemWidth(64+16*2);
  ImGui::Text("Alpha >= min"); ImGui::SameLine(); ImGui::InputInt("##DIALOGSELECTBYALPHAMIN",&alphaThresholdMin);
  ImGui::Text("Alpha <= max"); ImGui::SameLine(); ImGui::InputInt("##DIALOGSELECTBYALPHAMAX",&alphaThresholdMax);
  if (alphaThresholdMin<0) alphaThresholdMin = 0;
  if (alphaThresholdMin>255) alphaThresholdMin = 255;
  if (alphaThresholdMax<0) alphaThresholdMax = 0;
  if (alphaThresholdMax>255) alphaThresholdMax = 255;
  ImGui::PopItemWidth();
  if (ImGui::Button("Apply##DIALOGSELECTBYALPHAAPPLY")) {
    beforeUndoPoint();
    currentCanvas->selectByAlpha(alphaThresholdMin,alphaThresholdMax);
    afterUndoPoint();
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGSELECTBYALPHACLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND();
}

void dialogBakeLayers(bool openIt) {
  static int interpolation = INTERPOLATION_NONE;
  DIALOGSTART("Bake layers");
  Array<Layer *> layers;
  bool one = false; for (int i = 0; i < currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected) {one = true; layers.push_back(currentCanvas->layers[i]);}
  if (!one) for (int i2 = 0; i2 < currentCanvas->layers.size(); i2++) if (currentCanvas->layers[i2]->state.active) {layers.push_back(currentCanvas->layers[i2]);}

  if (layers.empty()) {
    ImGui::Text("No layers selected or active");
  } else {
    ImGui::Text("You may lose pixel information through baking");
    ImGui::Text("and the layers may be rotated/resized when saving the layers!");
    ImGui::Text("");
    ImGui::Text("Layers to bake");
    ImGui::BeginChild("DIALOGBAKELAYERSCHILD",ImVec2(main_window_xres/2,main_window_yres/3));
    for (int i = 0; i < layers.size(); i++) {
      ImGui::Text("\"%s\"",layers[i]->name);
    }
    ImGui::EndChild();
    ImGui::PushItemWidth(7*12);
    ImGui::Text("Interpolation");ImGui::SameLine();ImGui::Combo("##DIALOGBAKELAYERSINTERPOLATION",&interpolation, interpolationNames, INTERPOLATION_COUNT);
    ImGui::PopItemWidth();
  }

  if (!layers.empty()) {
    if (ImGui::Button("Bake these layers##DIALOGBAKELAYERSAPPLY")) {
      beforeUndoPoint();
      for (int i = 0; i < layers.size(); i++) {
        Layer *l = layers[i];
        if (l->type == LAYERTYPE_TEXT) {
          l->type = LAYERTYPE_BASIC;
          l->state.locked = false;
          l->state.permanentlyLocked = false;
        }
        l->bakeLayer(interpolation);
      }
      afterUndoPoint();
      shouldUpdateAll = true;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGBAKELAYERSCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogNewImage(bool openIt) {
  static int w = 1920,h = 1080;
  static char imageName[PICTURENAMEMAX];
  if (openIt) {
    static int counter = 0; counter++;
    sprintf(imageName,"New image %d",counter % 1023);
  }
  DIALOGSTART("New picture");
  ImGui::Text("Picture name"); ImGui::SameLine(); ImGui::InputText("##DIALOGNEWIMAGENAME",imageName,PICTURENAMEMAX);
  ImGui::PushItemWidth(64+16*2);
  ImGui::Text("Picture width"); ImGui::SameLine(); ImGui::InputInt("##DIALOGNEWIMAGEWIDTH",&w);
  ImGui::Text("Picture height"); ImGui::SameLine(); ImGui::InputInt("##DIALOGNEWIMAGEHEIGHT",&h);
  ImGui::PopItemWidth();

  if (ImGui::Button("Create##DIALOGNEWIMAGECREATE")) {
    newPicture(w,h,String(imageName));
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGNEWIMAGECLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogGetLayersFromPicture(bool openIt) {
  static int selected = -1;
  if (openIt) {
    selected = -1;
  }
  DIALOGSTART("Get selected layers from other picture");
  ImGui::Text("Insert layers from this picture");
  ImGui::BeginChild("CHILDDIALOGGETLAYERSFROMPICTURE",ImVec2(main_window_xres/2,main_window_yres/3));
  for (int i = 0; i < canvas.size(); i++) {
    int count = 0;
    for (int i2 = 0; i2 < currentCanvas->layers.size(); i2++) {
      if (currentCanvas->layers[i2]->state.selected||currentCanvas->layers[i2]->state.active) {
        count++;
      }
    }

    if (ImGui::Button(combined(String(selected == i ? ">" : " ")+String("\"")+String(canvas[i]->pictureName) + String("\"  selected layers:") + String::fromInt(count) + String("##DIALOGGETLAYERSFROMPICTURELINE")).c_str())) {
      selected = i;
    }
  }
  ImGui::EndChild();

  if (selected >= 0 && selected < canvas.size()) {
    if (ImGui::Button("Get layers##DIALOGGETLAYERSFROMPICTUREGETLAYERS")) {
      int pos = 0;
      for (int i = 0; i < currentCanvas->layers.size(); i++) {
        if (currentCanvas->layers[i]->state.selected||currentCanvas->layers[i]->state.active) {
          pos = i;
          break;
        }
      }
      Canvas *otherCanvas = canvas[selected];
      for (int i2 = 0; i2 < otherCanvas->layers.size(); i2++) {
        Layer *l = otherCanvas->layers[i2];
        if (l->state.active || l->state.selected) {
          Layer *l2 = l->clone();
          currentCanvas->insertLayer(l2,pos); pos++;
        }
      }
      shouldUpdateAll = true;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGGETLAYERSFROMPICTURECLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogUpdateLayer(bool openIt) {
  static int expandLeft = 0, expandRight = 0, expandTop = 0, expandBottom = 0;
  DIALOGSTART("Update layer");
  Layer *l = currentCanvas->getActiveLayer();
  if (l != NULL) {
    ImGui::Text("Old layer pixel data width %d\n", l->width);
    ImGui::Text("Old layer pixel data height %d\n", l->height);
    ImGui::Text("");
    int newWidth = l->width + expandLeft + expandRight;
    int newHeight = l->height + expandTop + expandBottom;
    if (newWidth < 1) {newWidth = 1; expandLeft = 0; expandRight = -l->width+1;}
    if (newHeight < 1) {newHeight = 1; expandTop = 0; expandBottom = -l->height+1;}
    ImGui::Text("New layer pixel data width %d\n", newWidth);
    ImGui::Text("New layer pixel data height %d\n", newHeight);
    ImGui::Text("");
    ImGui::PushItemWidth(64+16*2);
    ImGui::Text("Expand top"); ImGui::SameLine(); ImGui::InputInt("##DIALOGUPDATELAYEREXPANDTOP",&expandTop);
    ImGui::Text("Expand left"); ImGui::SameLine(); ImGui::InputInt("##DIALOGUPDATELAYEREXPANDLEFT",&expandLeft);
    ImGui::Text("Expand bottom"); ImGui::SameLine(); ImGui::InputInt("##DIALOGUPDATELAYEREXPANDBOTTOM",&expandBottom);
    ImGui::Text("Expand right"); ImGui::SameLine(); ImGui::InputInt("##DIALOGUPDATELAYEREXPANDRIGHT",&expandRight);
    ImGui::PopItemWidth();
    ImGui::Text("");
    if (ImGui::Button("Apply##DIALOGUPDATELAYERAPPLY")) {
      l->enlargenLayerBy(expandLeft,expandTop,expandRight,expandBottom);
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGUPDATELAYERCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void exportLayersRawDialogOpen(const char *shortPath, const char *longFileName) {
  dialogShortPath = String(shortPath);
  dialogLongFileName = String(longFileName);
  dialogExportLayersRaw(true);
}

typedef void (*LOADSAVEFUNC)(const char *shortPath, const char *longFileName);
const char *imgs = ".png,.jpg,.tga,.bmp,.psd,.gif,.pic,.pnm,.hdr,.svg";
const char *png = ".png";
const char *psd = ".psd";
const char *all = "*.*";
const char *pal = "*.pa2";
const char *grad = "*.gr2";
const char *brush = "*.br2";
const char *svg = "*.svg";
const char *img = "*.wyp";
const char *fnt = "*.ttf";
const char *gradientCrv = "*.cr2";
const char *dialogCaption[DIALOG_COUNT] = {"NO MODE","Import image","Insert image layer(s)","Export layer(s)","Export png","Load palette","Save palette","Export layer(s) raw","Insert psd","Load gradient","Save gradient","Insert svg","Load brush shape svg","Load brush","Save brush","Load brushes","Save brushes","Load image","Save image", "Add font", "Save font","Load gradient curves","Save gradient curves"};
const char *proceedButton[DIALOG_COUNT] = {"<button>", "Load","Load","Save","Save","Load","Save","Save","Load","Load","Save","Load","Load","Load","Save","Load","Save","Load","Save","Load","Save","Load","Save"};
const char *dialogFileTypes[DIALOG_COUNT] = {"",imgs,imgs,png,png,pal,pal,png,psd,grad,grad,svg,svg,brush,brush,brush,brush,img,img,fnt,fnt,gradientCrv,gradientCrv};
const char *dialogDefaultExtension[DIALOG_COUNT] = {"","","",".png",".png",".pa2",".pa2",".png",".psd",".gr2",".gr2",".svg",".svg",".br2",".br2",".br2",".br2",".wyp",".wyp",".ttf",".ttf",".cr2",".cr2"};
LOADSAVEFUNC funcs[DIALOG_COUNT] = {NULL, importImage,insertImageLayers,exportLayers,exportPNG,loadPalette,savePalette,exportLayersRawDialogOpen,insertPSD,loadGradient,saveGradient,insertSVG,loadBrushShape,loadBrush,saveBrush,loadBrushes,saveBrushes,loadImage,saveImage,loadFont,saveFont,loadGradientCurve,saveGradientCurve};

void sortFileNames(Array<File> &files, int column, bool sortOrder) {
  for (int i = 0; i < files.size(); i++) {
    for (int j = i+1; j < files.size(); j++) {
      bool bigger = false;
      String nameHere = (files[i].name.name + files[i].name.extension);
      bool dirHere = files[i].attributes & DOS_ATTR_SUBDIR;
      bool dirThere = files[j].attributes & DOS_ATTR_SUBDIR;
      switch(dirHere?0:column) {
      case 0: bigger = nameHere < (files[j].name.name + files[j].name.extension); break;
      case 1: bigger = (files[i].name.extension + files[i].name.name) < (files[j].name.extension + files[j].name.name); break;
      case 2: bigger = files[i].size < files[j].size; break;
      case 3: bigger = files[i].date < files[j].date; break;
      }
      if ((!dirHere)&&sortOrder) bigger = !bigger;
      if (dirHere != dirThere) {
        if (dirHere) bigger = false; else bigger = true;
      }
      if (bigger) {
        File t = files[i];
        files[i] = files[j];
        files[j] = t;
      }
    }
  }
}

unsigned int dialogLoadSaveGameObjectUniqueId = 0;
String dialogLoadSaveSetFileNameAndPath="";
int dialogLoadSaveModeId = 0;

LOADSAVEFUNC loadSaveFunc = NULL; 
String loadSaveShortPath; 
String loadSaveLongFileName;

Array<String> particles(const String &full, const char limiter, bool withLastParticle) {
  Array<String> ret;
  int start = 0;
  for (int i = 0; i < full.length(); i++) {
    if (full[i] == limiter) {
      ret.push_back(full.substr(start,i-start));
      start = i + 1;
    }
  }
  if (withLastParticle && start != full.length()) ret.push_back(full.substr(start));
  return ret;
}

void callLoadSaveFunc() {
  if (loadSaveFunc == NULL) {dialogLoadSaveModeId = 0;return;}
  loadSaveFunc(loadSaveShortPath.c_str(), loadSaveLongFileName.c_str());
  dialogLoadSaveModeId = 0;
}

void dialogOverwrite(bool openIt) {
  if (openIt) {
    String shortPath = shortFileName(loadSaveShortPath+loadSaveLongFileName);
    FILE *in = fopen(dosFileName(shortPath).c_str(),"rb");
    if (in == NULL) {
      callLoadSaveFunc();
      return;  // don't open dialog
    }
    fclose(in);
  }
  DIALOGSTART("Overwrite file")
  ImGui::Text("Overwrite old file?");
  ImGui::BeginChild("##DIALOGOVERWRITECHILD",ImVec2(main_window_xres/2,64));
  ImGui::TextWrapped("Filename:%s",loadSaveLongFileName.c_str());
  ImGui::TextWrapped("Path:%s",loadSaveShortPath.c_str());
  ImGui::EndChild();
  ImGui::Separator();
  if (ImGui::Button("Overwrite##DIALOGOVERWRITEOVERWRITE")) {
    callLoadSaveFunc();
    dialogLoadSaveModeId = 0;
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Don't##DIALOGOVERWRITECLOSE")) {
    dialogLoadSaveModeId = 0;
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void resize2(String &s, int len) {
  int a = s.length();
  s.resize(len);
  memset(&s[a],0,len-a);
}

void dialogLoadSave(bool openIt, int dialogMode) {

  static int mode = DIALOG_NONE; if (dialogMode != DIALOG_NONE) mode = dialogMode;
#define FILTERNAMEMAX 16
  static char filter[FILTERNAMEMAX];


  static bool previewOn = false; 
  static bool driveReady = true;
  static bool needsRescanFiles = false;
  static bool longFileNames = false;
  static bool initColumns = false;
  static Array<char> driveLetters;
  static Array<File> files;
  static int sortByColumn = 0;
  static bool sortingOrder[4] = {false};
  static String paths[DIALOG_COUNT];
  static String fileNames[DIALOG_COUNT];
  static String path2 = "";
  static String scrollToFileName;
  static String returnToPath;
  static int scrollToHelper = 0;
  const int NAMEMAXHERE = 513;
  if (openIt) {
    initColumns = true;
    previewOn = false;
    memset(filter,0,sizeof(filter));
    needsRescanFiles = true;
    driveLetters.clear();
    for (int i = 'A'; i <= 'Z'; i++) {
      if (dosHasDrive((char)i)) driveLetters.push_back((char)i);
    }
    for (int j = 0; j < DIALOG_COUNT; j++) {
      fileNames[j] = ""; // little bug
      resize2(fileNames[j],NAMEMAXHERE);
    }
    scrollToHelper=0;
    if (!dialogLoadSaveSetFileNameAndPath.empty()) {
      String zwo = dialogLoadSaveSetFileNameAndPath;
      dialogLoadSaveSetFileNameAndPath = unixFileName(SHORTFILENAME(zwo));
      fileNames[mode] = getName(zwo) + getExtension(zwo);
      scrollToFileName = toLower(fileNames[mode]);
      scrollToHelper = 2;
      resize2(fileNames[mode],NAMEMAXHERE);
      paths[mode] = getPath(dialogLoadSaveSetFileNameAndPath);
      dialogLoadSaveSetFileNameAndPath = "";
    }
  }
  bool result = false;
  DIALOGSTART(dialogCaption[mode])
  noUndoMessage();
  if (paths[mode].empty()) {paths[mode] = getCWD();}
  if (needsRescanFiles) {
    clearPreview();
    needsRescanFiles = false;
    files.clear();
    char driveLetter = paths[mode][0];
    driveReady = true;
    if (driveLetter>='A'&&driveLetter<='B') {driveReady = dosIsDriveReady(driveLetter);}
    if (driveReady) {
      files = dosGetFiles(paths[mode]);
    }
    for (int i = 0; i < files.size(); i++) {
      if (files[i].name.name == "." || files[i].name.name == "..") {files.erase(i,1);i--;} else
      if ((files[i].attributes & 0x1f) != DOS_ATTR_SUBDIR && (files[i].attributes & 0x1f) != DOS_ATTR_NORMAL) {files.erase(i,1);i--;}
    }
    if (longFileNames) {
      for (int i = 0; i < files.size(); i++) {
          files[i].name.full = LONGFILENAME(files[i].name.full);
          files[i].name.name = getName(files[i].name.full);
          files[i].name.extension = getExtension(files[i].name.full);
      }
    }
    sortFileNames(files, sortByColumn, sortingOrder[sortByColumn]);
    path2 = LONGFILENAME(paths[mode]);
  }
#if !defined(__DJGPP__)
  ImGui::Text("Long filenames"); ImGui::SameLine(); if(ImGui::Checkbox("##DIALOGLOADSAVELONGFILENAMES", &longFileNames)) needsRescanFiles = true;
#endif
  ImGui::SameLine();
  for (int i = 0; i < driveLetters.size(); i++) {
    if (i != 0) ImGui::SameLine();
    char a = driveLetters[i];
    if (ImGui::Button((String::fromChar(a)+":##DIALOGLOADSAVEDRIVELETTER"+toString(i)).c_str())) {
      paths[mode] = String::fromChar(a) + ":/";
      needsRescanFiles = true;
      //log_setBiosCursor(0,0);
    }
  }
  ImGui::Text("%s",dialogFileTypes[mode]);
  ImGui::Separator();
  SVG *svg = &svgs["search"];ImGui::Image((ImTextureID)svg->glHandle,ImVec2(svg->w,svg->h));ImGui::SameLine();ImGui::InputText("##DIALOGLOADSAVEFILTER",filter,FILTERNAMEMAX);
  ImGui::SameLine();
  ImGui::Text("Preview"); ImGui::SameLine(); if (ImGui::Checkbox("##DIALOGLOADSAVEPREVIEW",&previewOn)) {if (previewOn) clearPreview();}
  if (previewOn && previewAspect != 0) {
    ImGui::BeginTooltip();
    ImGui::Image((ImTextureID)previewTexture,ImVec2(previewWidth,previewHeight*aspectRatio*main_window_yres/main_window_xres/previewAspect));
    ImGui::EndTooltip();
  }
  ImGui::Separator();
  if (ImGui::Button("Home##DIALOGLOADSAVEHOME")) {
    paths[mode] = appFolder;
    needsRescanFiles=true;
  }

  ImGui::SameLine();
  ImGui::Text("%s",path2.c_str()); // Text Wrapped has no window Size here
  ImGui::Separator();
  bool setScrollY0 = false;
  if (paths[mode].length() > 3 && ImGui::Button("..##DIALOGLOADSAVEPATHBACK")) {
    int k = paths[mode].substr(0,paths[mode].length()-1).findLast("/");
    if (k>=0) paths[mode] = paths[mode].substr(0,k+1);
    needsRescanFiles = true;
    setScrollY0 = true;
  }
  ImGui::SameLine(); ImGui::Text("Sort by:"); ImGui::SameLine();
  for (int l = 0; l < 4; l++) {
    if (l!=0) ImGui::SameLine();
    static const char *headerButtonNames[4] = {"Filename##DIALOGLOADSAVEFILENAMES","Ext##DIALOGLOADSAVEEXTS","Size##DIALOGLOADSAVESIZES","Date##DIALOGLOADSAVEDATES"};
    if (ImGui::Button(headerButtonNames[l])) {
      sortByColumn = l;
      sortingOrder[l] = !sortingOrder[l];
      needsRescanFiles = true;
    }
  }
  ImGui::Separator();
  int w = main_window_xres/2;
  int h = main_window_yres/2;
  if (!driveReady) {
    if (ImGui::Button("Drive not ready.. Retry.##DIALOGLOADSAVERETRY")) {
      needsRescanFiles = true;
    }
  } else {
    ImGui::BeginChild("DIALOGLOADSAVECHILD",ImVec2(w,h));
    if (setScrollY0) {setScrollY0 = false; ImGui::SetScrollY(0);}
    ImGui::Columns(4);
    String filter2 = toLower(filter);  
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(1,0));  
    for (int j = 0; j < 4; j++) {
      if (initColumns) {
          if (j == 1) ImGui::SetColumnOffset(j,ImGui::GetColumnOffset(j-1)+7*20);
          if (j == 2) ImGui::SetColumnOffset(j,ImGui::GetColumnOffset(j-1)+7*5);
          if (j == 3) ImGui::SetColumnOffset(j,ImGui::GetColumnOffset(j-1)+7*10);
      }
      for (int i = 0; i < (int)files.size(); i++) {
        String fullFileName = toLower(files[i].name.name + files[i].name.extension);
        if (!filter2.empty() && fullFileName.findFirst(filter2)<0) continue;
        bool isDir = files[i].attributes & DOS_ATTR_SUBDIR;
        if (isDir) {
          if (j == 0) {
            ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0,0,0,0.5));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0,0,0,0));
            if (ImGui::Button((files[i].name.name+"##DIALOGLOADSAVEDIRBUTTON"+String::fromInt(i)).c_str())) {
              paths[mode] += files[i].name.name + files[i].name.extension + "/";
              paths[mode] = SHORTFILENAME(paths[mode]);
              fileNames[mode] = "";
              resize2(fileNames[mode],NAMEMAXHERE);
              needsRescanFiles = true;
              ImGui::SetScrollY(0);
            } else {
              //if (!previewOn) drawToolTip(files[i].name.name.c_str());
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
          } 
          if (j == 1) {
            ImGui::Text("DIR");
          }
          if (j == 2) {
            ImGui::Text("");
          }
          if (j == 3) {
            FileTime t = files[i].date;
            ImGui::Text("%02d.%02d.%04d %02d:%02d.%02d",t.day,t.month,t.year,t.hour,t.minute,t.second);
          }
        } else {
          if (j == 0) {
            ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0,0,0,0.5));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0,0,0,0));
            if (ImGui::Button((files[i].name.name+"##DIALOGLOADSAVEFILEBUTTON"+String::fromInt(i)).c_str())) {
              fileNames[mode] = files[i].name.name + files[i].name.extension;
              if (previewOn) {
                String p = paths[mode]; 
                String f = fileNames[mode];
                f.resize(strlen(f.c_str()));
                previewAspect = updateToolTip(preview, previewWidth, previewHeight, p.c_str(), f.c_str());
                updatePreview();
              }
              resize2(fileNames[mode],NAMEMAXHERE);
            } else {
              //if (!previewOn) drawToolTip((files[i].name.name + files[i].name.extension).c_str());
            }
            if (fullFileName==scrollToFileName) {ImGui::SetScrollHere();}
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
          }
          if (j == 1) {
            ImGui::Text("%s",files[i].name.extension.c_str());
          }
          if (j == 2) {
            ImGui::Text("%db",files[i].size);
          }
          if (j == 3) {
            FileTime t = files[i].date;
            ImGui::Text("%02d.%02d.%04d %02d:%02d.%02d",t.day,t.month,t.year,t.hour,t.minute,t.second);
          }
        }
      }
      ImGui::NextColumn();
    }
    ImGui::PopStyleVar();  
    initColumns = false;
    ImGui::EndChild();
  }
  ImGui::Separator();
  ImGui::PushItemWidth(w);
  ImGui::Text("Filename");
  DIALOGKEYSHERE();
  ImGui::InputText("##DIALOGLOADSAVEFILENAME",&fileNames[mode][0],NAMEMAXHERE);
  ImGui::PopItemWidth();
  ImGui::Separator();
  if (ImGui::Button(combined(String(proceedButton[mode])+String("##DIALOGLOADSAVEPROCEED")).c_str())) {
    result = true;
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGLOADSAVECLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
  if (scrollToHelper>0) scrollToHelper--; else scrollToFileName="";
  if (result) {
    loadSaveFunc = NULL;
    if (funcs[mode]!=NULL) {
      loadSaveFunc = funcs[mode]; 
      loadSaveShortPath = paths[mode]; 
      loadSaveLongFileName = fileNames[mode];
      loadSaveLongFileName.resize(strlen(loadSaveLongFileName.c_str()));
      if (toLower(proceedButton[mode])=="save") {
        if (loadSaveLongFileName.find(".")<0) {loadSaveLongFileName += dialogDefaultExtension[mode];}
        dialogOverwrite(true);
      } else
        callLoadSaveFunc();
    }
  }
}
