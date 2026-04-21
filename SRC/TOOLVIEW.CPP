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
#include "TOOLVIEW.HPP"
#include "IMGUI/IMGUI.H"
#include "MENUBAR.HPP"
#include "NAVIGTOR.HPP"
#include "TOOLBAR.HPP"
#include "CONFIG.HPP"
#include "SVG.HPP"
#include "TOOLS.HPP"
#include "LAYERS.HPP"
#include <math.h>

int toolview_width = 120;

bool roundButton(const char *name, bool enabled);

void initToolView() {
}

void freeToolView() {
}

void drawToolWandView() {
  float k;
    if (shift) {ImGui::Text("Add selection");} 
  else
    if (ctrl) {ImGui::Text("Remove selection");} 
  else
    ImGui::Text("");
  k = wandColorDistance; ImGui::Text("Color distance"); if (ImGui::SliderFloat("##DRAWTOOLWANDVIEWCOLORDISTANCE",&k,0,256)) {wandColorDistance = k;}
  k = wandAlphaDistance; ImGui::Text("Alpha distance"); if (ImGui::SliderFloat("##DRAWTOOLWANDVIEWALPHADISTANCE",&k,0,256)) {wandAlphaDistance = k;}
  ImGui::Text("Surrounding"); ImGui::SameLine(); ImGui::Checkbox("##DRAWTOOLWANDVIEWNEIGHBORS",&wandByNeighbors);
}

void drawToolFillView() {
  float k;
  k = fillColorDistance; ImGui::Text("Color distance"); if (ImGui::SliderFloat("##DRAWTOOLFILLCOLORDISTANCE",&k,0,256)) {fillColorDistance = k;}
}

void drawToolSelectionView() {
    if (shift) {ImGui::Text("Add selection");} 
  else
    if (ctrl) {ImGui::Text("Remove selection");} 
  else
    ImGui::Text("");
  ImGui::RadioButton("Rectangle##DRAWTOOLSELECTIONVIEWRECT",&selectionTool,SELECTIONTOOL_RECTANGLE);
  ImGui::RadioButton("Ellipse##DRAWTOOLSELECTIONVIEWELLIPSE",&selectionTool,SELECTIONTOOL_ELLIPSE);
  ImGui::RadioButton("Lasso##DRAWTOOLSELECTIONVIEWLASSO",&selectionTool,SELECTIONTOOL_LASSO);
  ImGui::RadioButton("Polygon##DRAWTOOLSELECTIONVIEWPOLYGON",&selectionTool,SELECTIONTOOL_POLYGON);
  ImGui::Separator();
  switch(selectionTool) {
    case SELECTIONTOOL_ELLIPSE:
    case SELECTIONTOOL_RECTANGLE: {
      ImGui::Text("Proportional"); ImGui::SameLine(); ImGui::Checkbox("##DRAWTOOLSELECTIONVIEWPROPORTIONAL",&selectionProportional);
    } break;
  }
}

bool notInt(double v) {
  return fabs(v - floor(v))>0.0000001;
}

bool notIntPosX(const Array<Layer *> &selected) {for (int i = 0; i < selected.size(); i++) {Vector k = selected[i]->boundingBox(); if (notInt(k.x)) return true;}  return false;}
bool notIntPosY(const Array<Layer *> &selected) {for (int i = 0; i < selected.size(); i++) {Vector k = selected[i]->boundingBox(); if (notInt(k.y)) return true;}  return false;}
bool notIntWidth(const Array<Layer *> &selected) {for (int i = 0; i < selected.size(); i++) {Vector k = selected[i]->boundingBox(); if (notInt(k.z-k.x)) return true;}  return false;}
bool notIntHeight(const Array<Layer *> &selected) {for (int i = 0; i < selected.size(); i++) {Vector k = selected[i]->boundingBox(); if (notInt(k.w-k.y)) return true;}  return false;}

void drawToolMoveView() {
  Array<Layer *> selected;
  for (int i = 0; i < currentCanvas->layers.size(); i++) {if (currentCanvas->layers[i]->state.selected||currentCanvas->layers[i]->state.active) selected.push_back(currentCanvas->layers[i]);}
  if (!selected.empty()) {
    Vector b;
    for (int i = 0; i < selected.size(); i++) {
      Vector k = selected[i]->boundingBox();
      if (i == 0 || k.x < b.x) b.x = k.x;
      if (i == 0 || k.y < b.y) b.y = k.y;
      if (i == 0 || k.z > b.z) b.z = k.z;
      if (i == 0 || k.w > b.w) b.w = k.w;
    }
    int x0 = (int)floor(b.x);
    int y0 = (int)floor(b.y);
    int x1 = (int)floor(b.z);
    int y1 = (int)floor(b.w);
    int k;
    if (ctrl) ImGui::Text("Place corners");
    else
    if (shift) ImGui::Text("Move fast");
    else
    if (ImGui::IsMouseDown(0)) ImGui::Text("Move");
    else
    if (ImGui::IsMouseDown(1)) ImGui::Text("Rotate");
    else
    ImGui::Text("");

    bool somethingToFit = false;
    if (selected.size() == 1) {
      Layer *l = selected[0];
      k = x0; ImGui::Text("Layer pos x"); if (ImGui::InputInt("##DRAWTOOLMOVEVIEWPOSX",&k)) {l->setLayerPos(k,y0); shouldUpdateAll = true;} if (notInt(b.x)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;}
      k = y0; ImGui::Text("Layer pos y"); if (ImGui::InputInt("##DRAWTOOLMOVEVIEWPOSY",&k)) {l->setLayerPos(x0,k); shouldUpdateAll = true;} if (notInt(b.y)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;}
      k = x1-x0; ImGui::Text("Layer width"); if (ImGui::InputInt("##DRAWTOOLMOVEVIEWWIDTH",&k)) {if (k<1) k = 1;l->setLayerSize(k,y1-y0); shouldUpdateAll = true;} if (notInt(b.z-b.x)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;}
      k = y1-y0; ImGui::Text("Layer height"); if (ImGui::InputInt("##DRAWTOOLMOVEVIEWHEIGHT",&k)) {if (k<1) k = 1;l->setLayerSize(x1-x0,k); shouldUpdateAll = true;} if (notInt(b.w-b.y)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;}
      float k2 = l->getAngleDegrees(); ImGui::Text("Layer rotation"); if (ImGui::InputFloat("##DRAWTOOLMOVEVIEWROTATION",&k2)) {if(k2<0) k2=0; if (k2>360) k2 = 360;l->setAngleDegrees(k2);shouldUpdateAll = true;}
      if (roundButton("Fit boundings##DRAWTOOLMOVEVIEWFITTOCANVAS",somethingToFit)) {
        l->quantizeBoundingBox();
      }
    } else {
      k = x0; ImGui::Text("Bounding pos x"); if (ImGui::InputInt("##DRAWTOOLBOUNDINGPOSX",&k)) {currentCanvas->setBoundingPos(k,y0); shouldUpdateAll = true;} if (notIntPosX(selected)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;}
      k = y0; ImGui::Text("Bounding pos y"); if (ImGui::InputInt("##DRAWTOOLBOUNDINGPOSY",&k)) {currentCanvas->setBoundingPos(x0,k); shouldUpdateAll = true;} if (notIntPosY(selected)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;}
      k = x1-x0; ImGui::Text("Bounding width"); if (ImGui::InputInt("##DRAWTOOLBOUNDINGWIDTH",&k)) {if (k<1) k = 1;currentCanvas->setBoundingSize(k,y1-y0); shouldUpdateAll = true;} if (notIntWidth(selected)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;}
      k = y1-y0; ImGui::Text("Bounding height"); if (ImGui::InputInt("##DRAWTOOLBOUNDINGHEIGHT",&k)) {if (k<1) k = 1;currentCanvas->setBoundingSize(x1-x0,k); shouldUpdateAll = true;} if (notIntHeight(selected)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;}
      float k2 = currentCanvas->getBoundingAngleDegrees(); ImGui::Text("Bounding rotation"); if (ImGui::InputFloat("##DRAWTOOLNOUNDINGROTATION",&k2)) {if(k2<0) k2=0; if (k2>360) k2 = 360;currentCanvas->setBoundingAngleDegrees(k2);shouldUpdateAll = true;}
      if (roundButton("Fit boundings##DRAWTOOLMOVEVIEWFITTOCANVAS",somethingToFit)) {
        for (int i = 0; i < selected.size(); i++)
          selected[i]->quantizeBoundingBox();
      }
    }
  }
}

bool drawBrushWindow(const char *id, int instanceId, Brush *brush, int w, int h, bool pencil, bool selected) {
  h = h * aspectRatio * main_window_yres / main_window_xres;
  BrushInstance *v = brush->getCreateInstance(instanceId,w,h,pencil);
  ImVec2 cursor = ImGui::GetCursorScreenPos();
  ImGui::PushID((String(id)+"_").c_str());
  ImGui::Image((ImTextureID)v->glHandle,ImVec2(w,h),ImVec2(0,0),ImVec2(1,1),ImVec4(0,0,0,1));
  if (selected) ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(cursor.x,cursor.y),ImVec2(cursor.x+w,cursor.y+h),0x400000ff);
  ImGui::PopID();
  ImGui::SetCursorScreenPos(cursor);
  bool ret = ImGui::InvisibleButton(id,ImVec2(w,h));
  return ret;
}

void insertBrush() {
  if ((unsigned int)currentBrush>=brushes.size()) return;
  brushes.insert(new Brush(),currentBrush);
}

void eraseBrush() {
  if ((unsigned int)currentBrush>=brushes.size()) return;
  delete brushes[currentBrush];
  brushes.erase(currentBrush,1);
  if ((!brushes.empty()) && currentBrush>=brushes.size()) currentBrush = brushes.size()-1;
}

void drawBrushSelect(bool pencil) {
  int toolButtonXCount = 4;
  int toolButtonYCount = 4;
  int toolButtonHeight = (toolview_width - 16) / toolButtonXCount - 4;
  int toolButtonWidth = toolButtonHeight;
  if (roundButton("Insert##DRAWBRUSHSELECTINSERTBRUSH",true)) {insertBrush();}
  ImGui::SameLine();
  if (roundButton("Erase##DRAWBRUSHSELECTERASEBRUSH",true)) {eraseBrush();}
  ImGui::Text("Brushes %d",brushes.size());
  ImGui::BeginChild("DRAWBRUSHSELECTCHILD", ImVec2(toolview_width,toolButtonHeight*toolButtonYCount));
  for (int i = 0; i < brushes.size(); i++) {
    if ((i % toolButtonXCount) != 0) ImGui::SameLine();
    Brush *brush = brushes[i];
    if (drawBrushWindow((String("##DRAWBRUSHSELECT")+String::fromInt(i)).c_str(),BRUSHINSTANCE_SELECT,brush,toolButtonWidth,toolButtonHeight, pencil, i == currentBrush)) {
      brushes[currentBrush]->freeInstance(BRUSHINSTANCE_PAINT);
      brushes[currentBrush]->freeInstance(BRUSHINSTANCE_EDIT);
      currentBrush = i;
    }
  }
  ImGui::EndChild();
  roundButton("Load##DRAWBRUSHSELECTLOADBRUSH",true);
  ImGui::SameLine();
  roundButton("Save##DRAWBRUSHSELECTSAVEBRUSH",true);
  roundButton("Load brushes##DRAWBRUSHSELECTLOADBRUSHES",true);
  roundButton("Save brushes##DRAWBRUSHSELECTSAVEBRUSHES",true);
  ImGui::Separator();
}

void drawBrushEdit(const char *id, Brush *brush, bool pencil) {
  drawBrushWindow(id,BRUSHINSTANCE_EDIT,brush,toolview_width-10,toolview_width-10, pencil,false);
  ImGui::Text("Size");
  if (ImGui::SliderFloat("##DRAWBRUSHEDITSIZE",&brush->size,1,128)) {if (brush->size<1) brush->size = 1; if (brush->size>128) brush->size = 128; brush->update();}
  ImGui::Text("Aspect");
  if (ImGui::SliderFloat("##DRAWBRUSHEDITASPECT",&brush->aspect,1,8)) {if (brush->aspect<1) brush->aspect = 0; if (brush->aspect>8) brush->aspect = 8; brush->update();}
  ImGui::Text("Rotation");
  if (ImGui::SliderFloat("##DRAWBRUSHEDITROTATION",&brush->rotation,0,360)) {if (brush->rotation<0) brush->rotation = 0; if (brush->rotation>360) brush->rotation = 360; brush->update();}
  if (!pencil) {
    ImGui::Text("Pressure");
    if (ImGui::SliderFloat("##DRAWBRUSHEDITPRESSURE",&brush->pressure,0,16)) {if (brush->pressure<0) brush->pressure = 0; brush->update();}
    ImGui::Text("Softness");
    float k = brush->softness-1; if (ImGui::SliderFloat("##DRAWBRUSHEDITSOFTNESS",&k,-1,1)) {if (k<-1) k = -1; if (k>1) k = 1; brush->softness = k + 1; brush->update();}
  }
  ImGui::Text("Edginess");
  if (ImGui::SliderFloat("##DRAWBRUSHEDITEDGINESS",&brush->edginess,0,1)) {if (brush->edginess<0) brush->edginess = 0; if (brush->edginess>1) brush->edginess = 1; brush->update();}
  ImGui::Text("Ovality");
  if (ImGui::SliderFloat("##DRAWBRUSHEDITOVALITY",&brush->ovality,0,1)) {if (brush->ovality<0) brush->ovality = 0; if (brush->ovality>1) brush->ovality = 1; brush->update();}
}

void drawToolPencilView() {
  drawBrushSelect(true);
  if ((unsigned int)currentBrush<brushes.size()) drawBrushEdit("##DRAWTOOLPENCILVIEWEDIT",brushes[currentBrush],true);
}

void drawToolBrushView() {
  drawBrushSelect(false);
  if ((unsigned int)currentBrush<brushes.size()) drawBrushEdit("##DRAWTOOLBRUSHVIEWEDIT",brushes[currentBrush],false);
}

void drawToolEraserView() {
  drawBrushSelect(true);
  if ((unsigned int)currentBrush<brushes.size()) drawBrushEdit("##DRAWTOOLERASERVIEWEDIT",brushes[currentBrush],true);
}

void drawToolMagicView() {
  drawBrushSelect(false);
  if ((unsigned int)currentBrush<brushes.size()) drawBrushEdit("##DRAWTOOLMAGICVIEWEDIT",brushes[currentBrush],false);
}

void drawToolPipetteView() {
  if (roundButton(pipetteFromLayer ? "From picture##DRAWTOOLPIPETTEVIEWLAYER" : "From layer##DRAWTOOLPIPETTEVIEWLAYER", true)) {pipetteFromLayer = !pipetteFromLayer;}
}

void drawToolView() {
  ImGui::SetNextWindowPos(ImVec2(toolbar_width,menuBar_height));
  ImGui::Begin("TOOLVIEW", NULL, ImVec2(toolview_width,main_window_yres-menuBar_height),1.0,FROZENWINDOW);
  ImGui::Text("%s",toolNames[tool]);
  ImGui::Separator();
  switch(tool) {
  case TOOL_WAND: {
      drawToolWandView();
    } break;
  case TOOL_MOVE: {
      drawToolMoveView();
    } break;
  case TOOL_PENCIL: {
      drawToolPencilView();
    } break;
  case TOOL_PAINTBRUSH: {
      drawToolBrushView();
    } break;
  case TOOL_ERASER: {
      drawToolEraserView();
    } break;
  case TOOL_PIPETTE: {
      drawToolPipetteView();
    } break;
  case TOOL_FILL: {
      drawToolFillView();
    } break;
  case TOOL_SELECTION: {
      drawToolSelectionView();
    } break;
  case TOOL_MAGIC: {
      drawToolMagicView();
    } break;
  }
  ImGui::End();
}

