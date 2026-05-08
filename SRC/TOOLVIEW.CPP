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
#include "IMGUI/IMGUIGL.HPP"
#include "MENUBAR.HPP"
#include "NAVIGTOR.HPP"
#include "TOOLBAR.HPP"
#include "CONFIG.HPP"
#include "SVG.HPP"
#include "TOOLS.HPP"
#include "LAYERS.HPP"
#include "COLORFLD.HPP"
#include "DIALOGS.HPP"
#include "STL/IMAGE.HPP"
#include "UNDO.HPP"
#include "TEXT.HPP"
#include "STL/DOS.HPP"
#include "SERIALZE.HPP"
#include <math.h>

int toolview_width = 120;

bool roundButton(const char *name, bool enabled);
bool imageButton(const char *svgName, const char *id, bool selected, bool hilighted, bool disabled);

GLuint gradientTexture = 0;
const int gradientTextureWidth = 100;
const int gradientTextureHeight = 50;
unsigned int gradientTextureContent[gradientTextureWidth*gradientTextureHeight];

GLuint tilingTexture = 0;
const int tilingTextureWidth = 100;
const int tilingTextureHeight = 100;
unsigned int tilingTextureContent[tilingTextureWidth*tilingTextureHeight];
unsigned int tilingTextureContentAnim[tilingTextureWidth*tilingTextureHeight];

void initToolView() {
  glGenTextures(1,&gradientTexture);
  glBindTexture(GL_TEXTURE_2D,gradientTexture);
  memset(gradientTextureContent,0,sizeof(gradientTextureContent));
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, gradientTextureWidth, gradientTextureHeight, 0, GL_RGBA, GL_BYTE, gradientTextureContent);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);

  glGenTextures(1,&tilingTexture);
  glBindTexture(GL_TEXTURE_2D,tilingTexture);
  memset(tilingTextureContent,0,sizeof(tilingTextureContent));
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, tilingTextureWidth, tilingTextureHeight, 0, GL_RGBA, GL_BYTE, tilingTextureContent);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);
}

void freeToolView() {
  glDeleteTextures(1,&gradientTexture);
  glDeleteTextures(1,&tilingTexture);
}

void repaintGradientTexture(Gradient *gradient) {
  for (int y = 0; y < gradientTextureHeight; y++) {
    for (int x = 0; x < gradientTextureWidth; x++) {
      float fx = (float)x / (gradientTextureWidth-1);
      float fy = (float)y / (gradientTextureHeight-1);
      if (gradient->round) {
        fx = fx * 2 - 1;
        fy = fy * 2 - 1;
      }
      unsigned int col = gradient->getColor2D(fx,fy);
      gradientTextureContent[x+y*gradientTextureWidth] = col;
    }
  }
  glBindTexture(GL_TEXTURE_2D,gradientTexture);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, gradientTextureWidth, gradientTextureHeight, 0, GL_RGBA, GL_BYTE, gradientTextureContent);
  glBindTexture(GL_TEXTURE_2D,0);
}

void drawToolWandView() {
  if (controlKeyId == CONTROLKEY_SHIFT) {ImGui::Text("Add selection");} 
  else
    if (controlKeyId == CONTROLKEY_CTRL) {ImGui::Text("Remove selection");} 
  else
    if (controlKeyId == (CONTROLKEY_SHIFT|CONTROLKEY_CTRL)) {ImGui::Text("Mul selection");} 
  else
    ImGui::Text("");
  float k;
  k = wandColorDistance; ImGui::Text("Color distance"); if (ImGui::SliderFloat("##DRAWTOOLWANDVIEWCOLORDISTANCE",&k,0,256)) {wandColorDistance = k;}
  k = wandAlphaDistance; ImGui::Text("Alpha distance"); if (ImGui::SliderFloat("##DRAWTOOLWANDVIEWALPHADISTANCE",&k,0,256)) {wandAlphaDistance = k;}
  ImGui::Text("Surrounding"); ImGui::SameLine(); ImGui::Checkbox("##DRAWTOOLWANDVIEWNEIGHBORS",&wandByNeighbors);
}

void drawToolFillView() {
  float k;
  k = fillColorDistance; ImGui::Text("Color distance"); if (ImGui::SliderFloat("##DRAWTOOLFILLCOLORDISTANCE",&k,0,256)) {fillColorDistance = k;}
  ImGui::Text("Use valleys"); ImGui::SameLine(); ImGui::Checkbox("##DRAWTOOLFILLVALLEYS",&fillUseValleys);
}

void drawToolSelectionView() {
  if (controlKeyId == CONTROLKEY_SHIFT) {ImGui::Text("Add selection");} 
  else
    if (controlKeyId == CONTROLKEY_CTRL) {ImGui::Text("Remove selection");} 
  else
    if (controlKeyId == (CONTROLKEY_SHIFT|CONTROLKEY_CTRL)) {ImGui::Text("Mul selection");} 
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
      ImGui::Separator();
      if (roundButton("Reset transform##DRAWTOOLMOVEVIEWRESETLAYERTRANSFORM",true)) {
        l->transform.fresh();
        l->transform.translate(-b.x,-b.y);
        shouldUpdateAll = true;
      }
      if (roundButton("Snap to grid##DRAWTOOLMOVEVIEWSNAPTOGRID",currentCanvas->gridOn)) {
        l->snapToGrid(currentCanvas);
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
      if (roundButton("Snap to grid##DRAWTOOLMOVEVIEWBOUNDINGSNAPTOGRID",currentCanvas->gridOn)) {
        currentCanvas->snapToGrid();
      }
    }
  }
}

bool drawBrushWindow(const char *id, int instanceId, Brush *brush, int w, int h, bool pencil, bool selected) {
  h = h * aspectRatio * main_window_yres / main_window_xres;
  BrushInstance *v = brush->getCreateInstance(instanceId,w,h,pencil);
  ImVec2 cursor = ImGui::GetCursorScreenPos();
  ImGui::PushID(combined(String(id)+"_").c_str());
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
    if (drawBrushWindow(combined(String("##DRAWBRUSHSELECT")+String::fromInt(i)).c_str(),BRUSHINSTANCE_SELECT,brush,toolButtonWidth,toolButtonHeight, pencil, i == currentBrush)) {
      brushes[currentBrush]->freeInstance(BRUSHINSTANCE_PAINT);
      brushes[currentBrush]->freeInstance(BRUSHINSTANCE_EDIT);
      currentBrush = i;
    }
  }
  ImGui::EndChild();
  if (roundButton("Load##DRAWBRUSHSELECTLOADBRUSH",true)) {dialogLoadSave(true,DIALOG_LOADBRUSH);}
  ImGui::SameLine();
  if (roundButton("Save##DRAWBRUSHSELECTSAVEBRUSH",true)) {dialogLoadSave(true,DIALOG_SAVEBRUSH);}  
  if (roundButton("Load brushes##DRAWBRUSHSELECTLOADBRUSHES",true)) {dialogLoadSave(true,DIALOG_LOADBRUSHES);}
  if (roundButton("Save brushes##DRAWBRUSHSELECTSAVEBRUSHES",true)) {dialogLoadSave(true,DIALOG_SAVEBRUSHES);}  
  ImGui::Separator();
}

void drawBrushEdit(const char *id, Brush *brush, bool pencil) {
  ImGui::InputText("##DRAWBRUSHEDITNAME",brush->name,BRUSHNAMEMAX);
  ImGui::SameLine(); ImGui::Checkbox("##DRAWBRUSHEDITOUTLINE",&toolOutline);
  if (drawBrushWindow(id,BRUSHINSTANCE_EDIT,brush,toolview_width-10,toolview_width-10, pencil,false)) {
    dialogLoadSave(true,DIALOG_LOADBRUSHSHAPE);
  }
  ImGui::Text("Size");
  ImGui::SameLine();if (ImGui::SliderFloat("##DRAWBRUSHEDITSIZE",&brush->size,1,128)) {if (brush->size<1) brush->size = 1; if (brush->size>128) brush->size = 128; brush->update();}
  if (!pencil) {
    ImGui::Text("Pressure");
    if (ImGui::SliderFloat("##DRAWBRUSHEDITPRESSURE",&brush->pressure,0,2)) {if (brush->pressure<0) brush->pressure = 0; brush->update();}
  }
  ImGui::Text("Opacity");
  if (ImGui::SliderFloat("##DRAWBRUSHEDITOPACITY",&brush->opacity,0,1)) {if (brush->opacity<0) brush->opacity = 0; if (brush->opacity>1) brush->opacity = 1; brush->update();}
  ImGui::Text("Aspect");
  if (ImGui::SliderFloat("##DRAWBRUSHEDITASPECT",&brush->aspect,1,8)) {if (brush->aspect<1) brush->aspect = 0; if (brush->aspect>8) brush->aspect = 8; brush->update();}
  ImGui::Text("Rotation");
  if (ImGui::SliderFloat("##DRAWBRUSHEDITROTATION",&brush->rotation,0,360)) {if (brush->rotation<0) brush->rotation = 0; if (brush->rotation>360) brush->rotation = 360; brush->update();}
  if (!pencil) {
    ImGui::Text("Softness");
    float k = brush->softness-1; if (ImGui::SliderFloat("##DRAWBRUSHEDITSOFTNESS",&k,-1,1)) {if (k<-1) k = -1; if (k>1) k = 1; brush->softness = k + 1; brush->update();}
  }
  ImGui::Text("Edge");
  ImGui::SameLine();if (ImGui::SliderFloat("##DRAWBRUSHEDITEDGINESS",&brush->edginess,0,1)) {if (brush->edginess<0) brush->edginess = 0; if (brush->edginess>1) brush->edginess = 1; brush->update();}
  ImGui::Text("Ovality");
  if (ImGui::SliderFloat("##DRAWBRUSHEDITOVALITY",&brush->ovality,0,1)) {if (brush->ovality<0) brush->ovality = 0; if (brush->ovality>1) brush->ovality = 1; brush->update();}
  float k;
  ImGui::Separator();
  ImGui::PushItemWidth(7*5+16*2);
  ImGui::Text("Rate");
  ImGui::SameLine(); if (ImGui::SliderFloat("##DRAWBRUSHEDITRATE",&brush->rate,0,1)) {if (brush->rate<0) brush->rate = 0; if (brush->rate>1) brush->rate = 1; brush->update();}
  ImGui::Text("Width");
  ImGui::SameLine(); k = (double)brush->size/(double)brush->aspect; if (ImGui::InputFloat("##DRAWBRUSHEDITWIDTH",&k,1,1)) {if (k<1) k = 1; if (k > 128) k = 128; brush->aspect = (float)((double)brush->size/(double)k); brush->update();}
  ImGui::Text("Height");
  ImGui::SameLine(); k = brush->size; if (ImGui::InputFloat("##DRAWBRUSHEDITHEIGHT",&k,1,1)) {double widthBefore = (double)brush->size/(double)brush->aspect; if (widthBefore == 0) widthBefore = 1; if(k<1) k = 1; if (k>128) k = 128; brush->size = k; brush->aspect = (float)((double)brush->size/widthBefore); brush->update();}
  ImGui::Text("SpreadX");
  ImGui::SameLine(); if (ImGui::InputFloat("##DRAWBRUSHEDITRANDOMX",&brush->randomX)) {brush->update();}
  ImGui::Text("SpreadY");
  ImGui::SameLine();if (ImGui::InputFloat("##DRAWBRUSHEDITRANDOMY",&brush->randomY)) {brush->update();}
  ImGui::Text("Rotate"); ImGui::SameLine(); ImGui::Checkbox("##DRAWBRUSHEDITFOLLOWROTATION",&brush->followRotation);
  ImGui::PopItemWidth();
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
  drawBrushSelect(false);
  if ((unsigned int)currentBrush<brushes.size()) drawBrushEdit("##DRAWTOOLERASERVIEWEDIT",brushes[currentBrush],false);
}

void drawToolPipetteView() {
  if (roundButton(pipetteFromLayer ? "From picture##DRAWTOOLPIPETTEVIEWLAYER" : "From layer##DRAWTOOLPIPETTEVIEWLAYER", true)) {pipetteFromLayer = !pipetteFromLayer;}
  if (pipetteFromLayer) {
    ImGui::Text("Postprocessing"); ImGui::SameLine(); ImGui::Checkbox("##DRAWTOOLPIPETTEVIEWLAYERCOLORGRADING",&pipetteWithColorTransform);
  }
}

void drawToolLineView() {
  ImGui::Text("Line width");
  ImGui::InputFloat("##DRAWTOOLLINEVIEWLINEWIDTH",&lineWidth); if (lineWidth < 0) lineWidth = 0;
  ImGui::Text("Line cap"); ImGui::SameLine(); ImGui::Checkbox("##DRAWTOOLLINEVIEWLINECAP",&lineCap);
  ImGui::Text("Antialiasing"); ImGui::SameLine(); ImGui::Checkbox("##DRAWTOOLLINEVIEWLINEANTIALIASING",&lineAntialiasing);
}

void updateGradient() {
  gradient.update();
  repaintGradientTexture(&gradient);
}

void drawToolGradientView() {
  static String neededScroll = "";
  if (roundButton("Load##DRAWTOOLGRADIENTVIEWLOAD",true)) {dialogLoadSave(true,DIALOG_LOADGRADIENT);}
  ImGui::SameLine();
  if (roundButton("Save##DRAWTOOLGRADIENTVIEWSAVE",!gradient.elements.empty())) {dialogLoadSave(true,DIALOG_SAVEGRADIENT);}
  ImGui::Image((ImTextureID)gradientTexture,ImVec2(gradientTextureWidth,gradientTextureHeight));
  ImVec2 minB = ImGui::GetItemRectMin();
  ImVec2 maxB = ImGui::GetItemRectMax();
  minB.x -= 1;
  minB.y -= 1;
  ImGui::GetWindowDrawList()->AddRect(minB,maxB,0xff000000);
  ImGui::Text("Start %f", gradient.positionMin);
  ImGui::Text("End   %f", gradient.positionMax);
  ImGui::Text("Inverted"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLGRADIENTVIEWINVERTED",&gradient.inverted)) updateGradient();
  ImGui::Text("Double sided"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLGRADIENTVIEWDOUBLESIDED",&gradient.doubleSided)) updateGradient();
  ImGui::Text("Circular"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLGRADIENTVIEWCIRCULAR",&gradient.round)) updateGradient();
  if (roundButton("Add element##DRAWTOOLGRADIENTVIEWADDELEMENT",true)) {
    gradient.elements.push_back(GradientElement());
    updateGradient();
  }
  ImGui::BeginChild("DRAWTOOLGRADIENTVIEWCHILD",ImVec2(toolview_width,main_window_yres-ImGui::GetCursorScreenPos().y));
  for (int i = 0; i < gradient.elements.size(); i++) {
    GradientElement *e = &gradient.elements[i];
    String id = String("##DRAWTOOLGRADIENTVIEWELEMENT")+String::fromInt(e->id);
    if (id == neededScroll) {
      ImGui::SetScrollPosHere();
      neededScroll = "";
    }
    ImGui::PushItemWidth(7*5);
    if (ImGui::InputFloat((id+"FLOAT").c_str(),&e->position,0,0,2)) {if (e->position<0) e->position = 0; if (e->position>100000) e->position = 100000; neededScroll = id; updateGradient();}
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Checkbox((id+"STEP").c_str(),&e->isStep)) updateGradient();
    ImGui::SameLine();
    ImGui::PushID((id+"IMAGEBUTTON").c_str());
    SVG *svg = &svgs["delete"];
    if (ImGui::ImageButton((ImTextureID)svg->glHandle,ImVec2(svg->w,svg->h))) {
      gradient.elements.erase(i,1);
      updateGradient();
      if ((unsigned int)i<gradient.elements.size()) e = &gradient.elements[i]; else e = NULL;
      i--;
    }
    ImGui::PopID();
    if (e != NULL) {
      ImVec4 chere = getImVec4Color(e->color);
      ImGui::Text(" "); ImGui::SameLine();
      ImVec2 cursor = ImGui::GetCursorScreenPos();
      if (colorItem("", (id+"COLOR").c_str(), &chere)) {e->color = getUIntColor(chere); addUsedColor(chere); updateGradient();}
      ImGui::SetCursorScreenPos(cursor);
      if (ImGui::InvisibleButton((id+"INVISIBLE").c_str(),ImVec2(16/aspectRatio*main_window_xres/main_window_yres,16))) {
        e->color = getUIntColor(foregroundColor); addUsedColor(foregroundColor); updateGradient();
        updateGradient();
      }
    }
  }
  ImGui::EndChild();
}

void drawToolMagicView() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  ColorTransform *t = &l->colorTransform;
  float *v;

  ImGui::Text("In selection");ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWJUSTINSELECTION",&l->showColorTransformOnlyInSelection)) {t->shouldUpdate = true; shouldUpdateAll = true;}
  if (roundButton("Bake into##DRAWTOOLMAGICVIEWAPPLYCOLORGRADING",true)) {beforeUndoPoint();l->bakeMagic(l->hasSelection()); l->colorTransform.useColorTransform = false; l->colorTransform.usePosterize = false; l->colorTransform.useQuantization = false; afterUndoPoint(); shouldUpdateAll = true;}
  ImGui::Separator();
  if (roundButton("Reset##DRAWTOOLMAGICVIEWAPPLYCOLORGRADINGRESET",true)) {l->colorTransform.reset(); shouldUpdateAll = true;}
  if (roundButton("Copy##DRAWTOOLMAGICVIEWAPPLYCOLORGRADINGCOPY",true)) {l->copyColorTransform();}
  ImGui::SameLine();
  if (roundButton("Paste##DRAWTOOLMAGICVIEWAPPLYCOLORGRADINGPASTE",true)) {l->pasteColorTransform();}
  ImGui::Separator();

  ImGui::BeginChild("DRAWTOOLMAGICVIEWCHILD",ImVec2(toolview_width, main_window_yres-ImGui::GetCursorScreenPos().y));

  ImGui::Text("Colorgrading"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWUSECOLORTRANSFORM",&l->colorTransform.useColorTransform)) {
    shouldUpdateAll = true;
  }
  if (l->colorTransform.useColorTransform) {

    ImGui::BeginChild("DRAWTOOLMAGICCHILDCOLORTRANSFORM",ImVec2(0,200));

    ImGui::Text("Picture Gamma");
    v = &t->pictureGamma; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPICTUREGAMMA",v,0,0,2)) {if (*v<0) *v = 0; if (*v>10) *v = 10; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWGAPICTUREGAMMAINVERT",&t->pictureGammaInvert)) {t->shouldUpdate = true; shouldUpdateAll = true;}

    ImGui::Text("Contrast");
    v = &t->contrast; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWCONTRAST",v,0,2)) {if (*v<0) *v = 0; if (*v>2) *v = 2; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Brightness");
    v = &t->brightness; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWBRIGHTNESS",v,-1,1)) {if (*v<-1) *v = -1; if (*v>1) *v = 1; t->shouldUpdate = true; shouldUpdateAll = true;}

    ImGui::Text("Hue");
    v = &t->hue; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWHUE",v,0,360)) {if (*v<0) *v = 0; if (*v>360) *v = 360; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Saturation");
    v = &t->saturation; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWSATURATION",v,0,2)) {if (*v<0) *v = 0; if (*v>2) *v = 2; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Lightness");
    v = &t->lightness; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWLIGHTNESS",v,0,1)) {if (*v<0) *v = 0; if (*v>4) *v = 4; t->shouldUpdate = true; shouldUpdateAll = true;}

    ImGui::Text("Perceptional"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICPERCEPTIONAL",&t->perceptional)) {t->shouldUpdate = true; shouldUpdateAll = true;}

    if (roundButton("Color matrix##DRAWTOOLMAGICCOLORMATRIX",true)) {dialogColorMatrix(true);}

    ImGui::Text("p - r - y");
    v = &t->middleRedAdjust; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWMIDDLEREDADJUST",v,-1,1)) {t->shouldUpdate = true; shouldUpdateAll = true;} if (*v<-1) *v = -1; if (*v>1) *v = 1; 
    ImGui::Text("y - g - c");
    v = &t->middleGreenAdjust; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWMIDDLEGREENADJUST",v,-1,1)) {t->shouldUpdate = true; shouldUpdateAll = true;} if (*v<-1) *v = -1; if (*v>1) *v = 1; 
    ImGui::Text("c - b - p");
    v = &t->middleBlueAdjust; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWMIDDLEBLUEADJUST",v,-1,1)) {t->shouldUpdate = true; shouldUpdateAll = true;} if (*v<-1) *v = -1; if (*v>1) *v = 1; 

    ImGui::Text("Scale red");
    v = &t->scaleRed; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWSCALERED",v,0,0,2)) {if (*v<-512) *v = -512; if (*v>512) *v = 512; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Scale green");
    v = &t->scaleGreen; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWSCALEGREEN",v,0,0,2)) {if (*v<-512) *v = -512; if (*v>512) *v = 512;  t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Scale blue");
    v = &t->scaleBlue; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWSCALEBLUE",v,0,0,2)) {if (*v<-512) *v = -512; if (*v>512) *v = 512;  t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Scale alpha");
    v = &t->scaleAlpha; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWSCALEALPHA",v,0,0,2)) {if (*v<-512) *v = -512; if (*v>512) *v = 512;  t->shouldUpdate = true; shouldUpdateAll = true;}


    ImGui::Text("Add red");
    v = &t->addRed; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWADDRED",v,0,0,2)) {if (*v<-255) *v = -255; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Add green");
    v = &t->addGreen; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWADDGREEN",v,0,0,2)) {if (*v<-255) *v = -255; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Add blue");
    v = &t->addBlue; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWADDBLUE",v,0,0,2)) {if (*v<-255) *v = -255; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Add alpha");
    v = &t->addAlpha; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWADDALPHA",v,0,0,2)) {if (*v<-255) *v = -255; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;}

    ImGui::Text("Invert R"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWINVERTR",&t->invertR)) {t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Invert G"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWINVERTG",&t->invertG)) {t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Invert B"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWINVERTB",&t->invertB)) {t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Invert A"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWINVERTA",&t->invertA)) {t->shouldUpdate = true; shouldUpdateAll = true;}

    ImGui::Text("Monitor gamma");
    v = &t->gamma; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWGAMMA",v,0,0,2)) {if (*v<0) *v = 0; if (*v>10) *v = 10; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWGAMMAINVERT",&t->gammaInvert)) {t->shouldUpdate = true; shouldUpdateAll = true;}
      
    ImGui::EndChild();
    ImGui::Separator();
  }

  ImGui::Text("Posterize"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWUSEPOSTERIZE",&l->colorTransform.usePosterize)) {
    shouldUpdateAll = true;
  }
  if (l->colorTransform.usePosterize) {
    ImGui::BeginChild("DRAWTOOLMAGICCHILDPOSTERIZE",ImVec2(0,50));
    ImGui::Text("Posterize R");
    v = &t->posterizeRed; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPOSTERIZERED",v,0,0,2)) {if (*v<0) *v = 0; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Posterize G");
    v = &t->posterizeGreen; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPOSTERIZEGREEN",v,0,0,2)) {if (*v<0) *v = 0; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Posterize B");
    v = &t->posterizeBlue; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPOSTERIZEBLUE",v,0,0,2)) {if (*v<0) *v = 0; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Posterize A");
    v = &t->posterizeAlpha; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPOSTERIZEALPHA",v,0,0,2)) {if (*v<0) *v = 0; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::EndChild();
    ImGui::Separator();
  }

  ImGui::Text("Threshold"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWUSETHRESHOLD",&l->colorTransform.useThreshold)) {
    shouldUpdateAll = true;
  }
  if (l->colorTransform.useThreshold) {
    ImGui::BeginChild("DRAWTOOLMAGICCHILDTHRESHOLD",ImVec2(0,50));
    ImGui::Text("Threshold RGBA");
    v = &t->thresholdLuma; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWTHRESHOLDLUMA",v,0,255)) {if (*v<0) *v = 0; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Invert"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWINVERT",&t->thresholdInvert)) {t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Scale red");
    v = &t->thresholdRed; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWTHRESHOLDRED",v,0,1)) {if (*v<0) *v = 0; if (*v>1) *v = 1; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Scale green");
    v = &t->thresholdGreen; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWTHRESHOLDGREEN",v,0,1)) {if (*v<0) *v = 0; if (*v>1) *v = 1; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Scale blue");
    v = &t->thresholdBlue; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWTHRESHOLDBLUE",v,0,1)) {if (*v<0) *v = 0; if (*v>1) *v = 1; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::Text("Scale alpha");
    v = &t->thresholdAlpha; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWTHRESHOLDALPHA",v,0,1)) {if (*v<0) *v = 0; if (*v>1) *v = 1; t->shouldUpdate = true; shouldUpdateAll = true;}
    ImGui::EndChild();
    ImGui::Separator();
  }

  ImGui::Text("Fast quant"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWUSEQUANTIZATION",&l->colorTransform.useQuantization)) {
    shouldUpdateAll = true;
  }
  if (l->colorTransform.useQuantization) {
    if (palette.size()<=256) {
      ImGui::Text("Dithering");
      v = &t->ditherStrength; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWDITHERSTRENGTH",v,0,1)) {if (*v<0) *v = 0; if (*v>1) *v = 1; t->shouldUpdate = true; shouldUpdateAll = true;}
    } else {
      if (palette.size()>256) ImGui::Text("Quantization\nonly with\npalettes\n<=256 colors");
    }
    ImGui::Separator();
  }

  ImGui::EndChild();
}

void drawToolMeasureView() {
  double cx0=0,cy0=0,lx0=0,ly0=0;
  cx0 = mouseX; cy0 = mouseY; currentCanvas->getCanvasPos(cx0,cy0,cx0,cy0); 
  if (currentCanvas->isOnlyActiveLayerActive()) {lx0 = mouseX; ly0 = mouseY; currentCanvas->getLayerPos(currentCanvas->getActiveLayer(),lx0,ly0,lx0,ly0);}

  ImGui::Text("Cursor");
  ImGui::Text("CanvasX %.02f", cx0);
  ImGui::Text("CanvasY %.02f", cy0);
  if (currentCanvas->isOnlyActiveLayerActive()) {
    ImGui::Text("LayerX %.02f", lx0);
    ImGui::Text("LayerY %.02f", ly0);
  }
  ImGui::Text("");
  ImGui::Text("Start point");
  ImGui::Text("CanvasX %d", (int)floor(measureCX0));
  ImGui::Text("CanvasY %d", (int)floor(measureCY0));
  if (currentCanvas->isOnlyActiveLayerActive()) {
    ImGui::Text("LayerX %d", (int)floor(measureLX0));
    ImGui::Text("LayerY %d", (int)floor(measureLY0));
  }
  if (fabs(measureCX1-measureCX0)>0||fabs(measureCY1-measureCY0)>0) {
    ImGui::Text("");
    ImGui::Text("End point");
    ImGui::Text("CanvasX %d", (int)floor(measureCX1));
    ImGui::Text("CanvasY %d", (int)floor(measureCY1));
    if (currentCanvas->isOnlyActiveLayerActive()) {
      ImGui::Text("LayerX %d", (int)floor(measureLX1));
      ImGui::Text("LayerY %d", (int)floor(measureLY1));
    }
    ImGui::Text("");
    ImGui::Text("Pixels");
    int dx,dy;
    dx = (int)(floor(measureCX1)-floor(measureCX0));
    dy = (int)(floor(measureCY1)-floor(measureCY0));
    ImGui::Text("CanvasW %d", dx);
    ImGui::Text("CanvasH %d", dy);
    dx = (int)(floor(measureLX1)-floor(measureLX0));
    dy = (int)(floor(measureLY1)-floor(measureLY0));
    if (currentCanvas->isOnlyActiveLayerActive()) {
      ImGui::Text("LayerW %d", dx);
      ImGui::Text("LayerH %d", dy);
    }
    ImGui::Text("");
    ImGui::Text("Length");
    double lx,ly;
    lx = measureCX1-measureCX0;
    ly = measureCY1-measureCY0;
    ImGui::Text("Canvas %.02f",sqrt(lx*lx+dy*dy));
    lx = measureLX1-measureLX0;
    ly = measureLY1-measureLY0;
    if (currentCanvas->isOnlyActiveLayerActive()) {
      ImGui::Text("Layer %.02f",sqrt(lx*lx+dy*dy));
    }
    //lx = measureCX0; ly = measureCY0; currentCanvas->getScreenPos(lx,ly,lx,ly); ImVec2 l0(lx,ly);
    //lx = measureCX1; ly = measureCY1; currentCanvas->getScreenPos(lx,ly,lx,ly); ImVec2 l1(lx,ly);
    //ImGui::GetWindowDrawList()->AddLine(l0,l1,0xff0000ff,4);
  }

  ImGui::Text("");
  ImVec4 chere; 
  ImGui::Text("Color canvas");
  chere = getImVec4Color(currentCanvas->getCanvasColor(mouseX,mouseY)); colorItem("", "##DRAWTOOLMEASUREVIEWCOLORCANVAS", &chere);
  ImGui::Text("Color layer");
  chere = getImVec4Color(currentCanvas->getLayerColor(mouseX,mouseY,false)); colorItem("", "##DRAWTOOLMEASUREVIEWCOLORLAYER", &chere);
}

void updateTilingGrid(Layer *l, bool visible) {
  if (l == NULL) return;
  if (l->state.locked) return;
  l->updatePixelStates(~(PIXELSTATE_GRID|PIXELSTATE_TEMP),0,0);
  for (int y = 0; y < l->height; y++) {
    for (int x = 0; x < l->width; x++) {
      int i = x + y * l->width;
      if (l->toolInfo.showTilingGrid && visible) {
        int gridX = x-l->toolInfo.tilingOffsetX + l->toolInfo.tilingWidth * 100000;
        int gridY = y-l->toolInfo.tilingOffsetY + l->toolInfo.tilingHeight * 100000;
        if (l->toolInfo.tilingWidth > 0) gridX %= l->toolInfo.tilingWidth;
        if (l->toolInfo.tilingHeight > 0) gridY %= l->toolInfo.tilingHeight;
        if ((gridX == 0)&&(gridY==0)) {
          int tw = l->toolInfo.tilingWidth;  if (tw > l->width) tw = l->width;
          int th = l->toolInfo.tilingHeight;  if (th > l->height) th = l->height;

          int colorCount = 0;
          if (l->toolInfo.tilingMinColorCount > 0) {
            for (int y3 = 0; y3 < th; y3++) {
              for (int x3 = 0; x3 < tw; x3++) {
                int xp = x + x3;
                int yp = y + y3;
                if ((unsigned int)xp < l->width && (unsigned int)yp < l->height) {
                  if (l->pixelStates[xp+yp*l->width] & PIXELSTATE_TEMP) continue;
                  unsigned int col = l->pixels[xp+yp*l->width];
                  for (int y4 = 0; y4 < th; y4++) {
                    for (int x4 = 0; x4 < tw; x4++) {
                      int xp = x + x4;
                      int yp = y + y4;
                      if ((unsigned int)xp < l->width && (unsigned int)yp < l->height) {
                        unsigned int col2 = l->pixels[xp+yp*l->width];
                        if (col2 == col) l->pixelStates[xp+yp*l->width] |= PIXELSTATE_TEMP;
                      }
                    }
                  }
                  colorCount++;
                }
              }
            }
          }

          if (colorCount >= l->toolInfo.tilingMinColorCount) {
            for (int x2 = 0; x2 < tw; x2++) {
              int xp = x + x2;
              int yp = y;
              if ((unsigned int)xp < l->width && (unsigned int)yp < l->height) l->pixelStates[xp+yp*l->width] |= PIXELSTATE_GRID;
              xp = x + x2;
              yp = y + th;
              if ((unsigned int)xp < l->width && (unsigned int)yp < l->height) l->pixelStates[xp+yp*l->width] |= PIXELSTATE_GRID;
            }
            for (int y2 = 0; y2 < th; y2++) {
              int xp = x;
              int yp = y + y2;
              if ((unsigned int)xp < l->width && (unsigned int)yp < l->height) l->pixelStates[xp+yp*l->width] |= PIXELSTATE_GRID;
              xp = x + tw;
              yp = y + y2;
              if ((unsigned int)xp < l->width && (unsigned int)yp < l->height) l->pixelStates[xp+yp*l->width] |= PIXELSTATE_GRID;
            }
          }
        }
      }
    }
  }
  shouldUpdateAll = true;
}

void updateTilingTexture() {
  glBindTexture(GL_TEXTURE_2D,tilingTexture);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, tilingTextureWidth, tilingTextureHeight, 0, GL_RGBA, GL_BYTE, tilingTextureContent);
  glBindTexture(GL_TEXTURE_2D,0);
}

void updateTilingImage(const RGBAImage &image) {
  double px = currentCanvas->control.pixelSizeX;
  double py = currentCanvas->control.pixelSizeY;
  for (int y = 0; y < tilingTextureHeight; y++) {
    for (int x = 0; x < tilingTextureWidth; x++) {
      unsigned int col = 0x00000000;
      if (image.data != NULL && image.width > 0 && image.height > 0 && px > 0 && py > 0) {
        double scaleX = (double)image.width/tilingTextureWidth*px;
        double scaleY = (double)image.height/tilingTextureHeight*py;
        double scale = scaleX > scaleY ? scaleX : scaleY;
        double x2 = (double)(x - tilingTextureWidth*0.5)*scale/px;
        double y2 = (double)(y - tilingTextureHeight*0.5)*scale/py;
        x2 += image.width*0.5;
        y2 += image.height*0.5;
        int x3 = (int)floor(x2);
        int y3 = (int)floor(y2);
        if ((unsigned int)x3<image.width&&(unsigned int)y3<image.height) col = image.data[x3+y3*image.width];
      }
      tilingTextureContent[x+y*tilingTextureWidth] = col;
    }
  }
  updateTilingTexture();
}

void updateTilingImageAnim(const RGBAImage &image) {
  double px = currentCanvas->control.pixelSizeX;
  double py = currentCanvas->control.pixelSizeY;
  for (int y = 0; y < tilingTextureHeight; y++) {
    for (int x = 0; x < tilingTextureWidth; x++) {
      unsigned int col = 0x00000000;
      if (image.data != NULL && image.width > 0 && image.height > 0 && px > 0 && py > 0) {
        double scaleX = (double)image.width/tilingTextureWidth*px;
        double scaleY = (double)image.height/tilingTextureHeight*py;
        double scale = scaleX > scaleY ? scaleX : scaleY;
        double x2 = (double)(x - tilingTextureWidth*0.5)*scale/px;
        double y2 = (double)(y - tilingTextureHeight*0.5)*scale/py;
        x2 += image.width*0.5;
        y2 += image.height*0.5;
        int x3 = (int)floor(x2);
        int y3 = (int)floor(y2);
        if ((unsigned int)x3<image.width&&(unsigned int)y3<image.height) col = image.data[x3+y3*image.width];
      }
      tilingTextureContentAnim[x+y*tilingTextureWidth] = col;
    }
  }
  glBindTexture(GL_TEXTURE_2D,tilingTexture);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, tilingTextureWidth, tilingTextureHeight, 0, GL_RGBA, GL_BYTE, tilingTextureContentAnim);
  glBindTexture(GL_TEXTURE_2D,0);
}

RGBAImage getImageFromLayer(Layer *l, int sx,int sy, int width, int height);

bool getTilingAnimationFrameLayerCell(Layer *l,int frame, int &cx, int &cy, int &outaX, int &outaY) {
  if (l->toolInfo.tilingWidth <= 0) return false;
  if (l->toolInfo.tilingHeight <= 0) return false;
  outaX = (l->toolInfo.tilingOffsetX+l->toolInfo.tilingWidth*100000) % l->toolInfo.tilingWidth;
  outaY = (l->toolInfo.tilingOffsetY+l->toolInfo.tilingHeight*100000) % l->toolInfo.tilingHeight;
  int tcx = l->width/l->toolInfo.tilingWidth;
  if (outaX != 0) tcx--;
  if (tcx <= 0) return false;
  cx = frame % tcx;
  cy = frame / tcx;
  return true;
}

void blitTilingAnimFrame(Layer *l,int frame) {
  int cx,cy,outaX,outaY; 
  if (!getTilingAnimationFrameLayerCell(l,frame,cx,cy,outaX,outaY)) return;
  cx *= l->toolInfo.tilingWidth;
  cy *= l->toolInfo.tilingHeight;
  cx += outaX;
  cy += outaY;
  int tw = l->toolInfo.tilingWidth;
  int th = l->toolInfo.tilingHeight;
  RGBAImage tile = getImageFromLayer(l,cx,cy,tw,th);
  updateTilingImageAnim(tile);
  tile.free();
}

void playTilingAnimation(Layer *l) {
  if (l == NULL) return;
  int f0 = l->toolInfo.tilingAnimFirstFrame;
  int fn = l->toolInfo.tilingAnimLastFrame;
  int s = fn-f0;
  if (s<=0||l->toolInfo.tilingAnimFPS<=0) {blitTilingAnimFrame(l,f0); return;}
  l->toolInfo.tilingAnimTime += timeDelta * l->toolInfo.tilingAnimFPS * (l->toolInfo.tilingAnimReverse ? -1 : 1); // actually the current frame
  if (l->toolInfo.tilingAnimTime<0) {l->toolInfo.tilingAnimTime += s * floor(-l->toolInfo.tilingAnimTime/s+1);}
  int tilingFrame = floor(l->toolInfo.tilingAnimTime);
  if (tilingFrame<0) {blitTilingAnimFrame(l,f0); return;}
  tilingFrame %= s;
  tilingFrame += l->toolInfo.tilingAnimFirstFrame;
  l->toolInfo.tilingAnimFrame = tilingFrame;
  blitTilingAnimFrame(l,tilingFrame);
}

void drawToolTilingView() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l != NULL) {
    ImGui::PushItemWidth(7*5);
    double lx = mouseX,ly = mouseY; currentCanvas->getLayerPos(l,lx,ly,lx,ly);
    int x = (int)floor((lx - l->toolInfo.tilingOffsetX) / (double)l->toolInfo.tilingWidth);
    int y = (int)floor((ly - l->toolInfo.tilingOffsetY) / (double)l->toolInfo.tilingHeight);
    if (controlKeyId == CONTROLKEY_SHIFT) 
      ImGui::Text("Select cell");
    else
      ImGui::Text("");
    ImGui::Text("[%d] [%d]",x,y);
    ImGui::Text("Cells");
    ImGui::Text("Offset X"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLTILINGVIEWOFSX",&l->toolInfo.tilingOffsetX,0,0)) {updateTilingGrid(l,true);}
    ImGui::Text("Offset Y"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLTILINGVIEWOFSY",&l->toolInfo.tilingOffsetY,0,0)) {updateTilingGrid(l,true);}
    ImGui::Text("Width"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLTILINGVIEWGRIDWIDTH",&l->toolInfo.tilingWidth,0,0)) {updateTilingGrid(l,true);}
    ImGui::Text("Height"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLTILINGVIEWGRIDHEIGHT",&l->toolInfo.tilingHeight,0,0)) {updateTilingGrid(l,true);}
    ImGui::Text("Show grid"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLTILINGVIEWSHOWGRID",&l->toolInfo.showTilingGrid)) {updateTilingGrid(l,true);}
    if (l->toolInfo.showTilingGrid) {
      ImGui::Separator();
      ImGui::Text("Tile colors");
      ImGui::Text("count >=");ImGui::SameLine();if (ImGui::InputInt("##DRAWTOOLTILINGVIEWCOLORCOUNT",&l->toolInfo.tilingMinColorCount,0,0)) {updateTilingGrid(l,true);}
    }
    if (l->toolInfo.tilingWidth < 1) l->toolInfo.tilingWidth = 1;
    if (l->toolInfo.tilingHeight < 1) l->toolInfo.tilingHeight = 1;
    if (l->toolInfo.tilingWidth > 1024) l->toolInfo.tilingWidth = 1024;
    if (l->toolInfo.tilingHeight > 1024) l->toolInfo.tilingHeight = 1024;
    ImGui::PopItemWidth();
    ImGui::Separator();
    ImGui::Image((ImTextureID)tilingTexture,ImVec2(tilingTextureWidth/(aspectRatio*main_window_yres/main_window_xres),tilingTextureHeight));
    ImVec2 minB = ImGui::GetItemRectMin();
    ImVec2 maxB = ImGui::GetItemRectMax();
    minB.x -= 1;
    minB.y -= 1;
    ImGui::GetWindowDrawList()->AddRect(minB,maxB,0xff000000);
    ImGui::Separator();
    ImGui::PushItemWidth(7*5);
    ImGui::Text("Animation"); ImGui::SameLine(); 
    if (imageButton(l->toolInfo.tilingAnimPlaying?"pause":"play", "##DRAWTOOLTILINGANIMATIONPLAY", false, false,false)) {
      l->toolInfo.tilingAnimPlaying = !l->toolInfo.tilingAnimPlaying;
      l->toolInfo.tilingAnimTime = 0;updateTilingTexture();   
    }
    if (l->toolInfo.tilingAnimPlaying) {
      ImGui::Text("Index %d",l->toolInfo.tilingAnimFrame);
    }
    ImGui::Text("Show indices"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLTILINGANIMATIONSHOWINDICES",&l->toolInfo.showTilingGridIndices)) {updateTilingGrid(l,true);}
    ImGui::Text("FPS"); ImGui::SameLine(); if (ImGui::InputFloat("##DRAWTOOLTILINGANIMATIONFPS",&l->toolInfo.tilingAnimFPS,0,0,2)) {} if (l->toolInfo.tilingAnimFPS<0) l->toolInfo.tilingAnimFPS=0;
    ImGui::Text("Index0"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLTILINGANIMATIONFIRSTFRAME",&l->toolInfo.tilingAnimFirstFrame,0,0)) {l->toolInfo.tilingAnimPlaying=false;} if (l->toolInfo.tilingAnimFirstFrame<0) l->toolInfo.tilingAnimFirstFrame=0;
    ImGui::Text("IndexN"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLTILINGANIMATIONLASTFRAME",&l->toolInfo.tilingAnimLastFrame,0,0)) {l->toolInfo.tilingAnimPlaying=false;} if (l->toolInfo.tilingAnimLastFrame<0) l->toolInfo.tilingAnimLastFrame=0;
    ImGui::Text("Reverse"); ImGui::SameLine(); ImGui::Checkbox("##DRAWTOOLTILINGANIMATIONREVERSE",&l->toolInfo.tilingAnimReverse);
    ImGui::PopItemWidth();
    if (l->toolInfo.tilingAnimPlaying) playTilingAnimation(l);
  }
}

void drawToolBlurView() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l != NULL) {
    ImGui::Text("Size");
    float k = blurCircleSize; ImGui::SameLine();if (ImGui::SliderFloat("##DRAWTOOLBLURSIZE",&k,1,128)) {if (k<1) k = 1; if (k>128) k = 128; blurCircleSize = k;}
  }
}

void drawToolTextView() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (l->type != LAYERTYPE_TEXT) return;
  if (roundButton("Add font",true)) {dialogLoadSave(true,DIALOG_ADDFONT);}
  if (roundButton("Save font",l->toolInfo.fontId != 0)) {saveFontId = l->toolInfo.fontId; dialogLoadSave(true,DIALOG_SAVEFONT);}
  ImGui::Text("Fonts [%d]",fonts.size());
  ImGui::BeginChild("DRAWTOOLTEXTVIEWFONTS",ImVec2(toolview_width,200));
  String fileName = "";
  for (int i = 0; i < fonts.size(); i++) {
    FontSource *f = fonts[i];
    if (f->glHandle == 0) f->glHandle = f->createTexture(getName(f->fileName),20,aspectRatio*main_window_yres/main_window_xres,f->glWidth,f->glHeight,0x00000000);
    ImVec2 minB = ImGui::GetCursorScreenPos();
    ImGui::Image((ImTextureID)f->glHandle,ImVec2(f->glWidth,f->glHeight));
    ImVec2 maxB = ImGui::GetItemRectMax();
    ImGui::SetCursorScreenPos(minB);
    maxB.x = minB.x + toolview_width;
    if (ImGui::InvisibleButton(combined(String("##DRAWTOOLTEXTUREVIEWLINE")+String::fromInt(i)).c_str(),ImVec2(maxB.x-minB.x,maxB.y-minB.y))) {l->toolInfo.fontId = f->id;}
    if (f->id == l->toolInfo.fontId) {fileName = f->fileName; ImGui::GetWindowDrawList()->AddRectFilled(minB,maxB,0x40ffff00);}
  }
  ImGui::EndChild();
  if (!fileName.empty()) ImGui::Text("%s",(fileName.last(16)).c_str());
  if (roundButton("Remove font",l->toolInfo.fontId != 0)) {removeFont(l->toolInfo.fontId);l->toolInfo.fontId = 0;}
  ImGui::PushItemWidth(7*5);
  ImGui::Text("Font"); 
  LayerAdditionalToolInfo *ati = &l->toolInfo;
  ImGui::Text("Height"); ImGui::SameLine(); ImGui::InputFloat("##DRAWTOOLTEXTUREVIEWFONTSIZE",&ati->fontSize,0,0,2); if (ati->fontSize < 1) ati->fontSize = 1;
  ImVec2 cursor = ImGui::GetCursorScreenPos();
  ImVec4 chere = getImVec4Color(ati->fontColor); colorItem("", "##DRAWTOOLTEXTUREVIEWFONTCOLOR", &chere);
  ImGui::SetCursorScreenPos(cursor);
  if (ImGui::InvisibleButton("##DRAWTOOLTEXTUREVIEWFONTCOLORINVISIBLE",ImVec2(16/aspectRatio*main_window_xres/main_window_yres,16))) {
    ati->fontColor = getUIntColor(foregroundColor);
  }
  ImGui::Text("Scale x"); ImGui::SameLine(); ImGui::InputFloat("##DRAWTOOLTEXTUREVIEWFONTSCALEX",&ati->fontScaleX,0,0,2); if (ati->fontScaleX < 0) ati->fontScaleX = 0;
  ImGui::Text("Scale y"); ImGui::SameLine(); ImGui::InputFloat("##DRAWTOOLTEXTUREVIEWFONTSCALEY",&ati->fontScaleY,0,0,2); if (ati->fontScaleY < 0) ati->fontScaleY = 0;
  ImGui::Text("Space x"); ImGui::SameLine(); ImGui::InputFloat("##DRAWTOOLTEXTUREVIEWFONTSPACEX",&ati->fontSpaceX,0,0,2);
  ImGui::Text("Space y"); ImGui::SameLine(); ImGui::InputFloat("##DRAWTOOLTEXTUREVIEWFONTSPACEY",&ati->fontSpaceY,0,0,2);
  ImGui::Text("");
  ImGui::RadioButton("Align left##DRAWTOOLTEXTUREVIEWALIGNLEFT",&ati->fontAlignment,ALIGN_LEFT);
  ImGui::RadioButton("Align right##DRAWTOOLTEXTUREVIEWALIGNRIGHT",&ati->fontAlignment,ALIGN_RIGHT);
  ImGui::RadioButton("Align center##DRAWTOOLTEXTUREVIEWALIGNCENTER",&ati->fontAlignment,ALIGN_CENTER);
  ImGui::PopItemWidth();
  ImGui::Text("Text");
  ImGui::PushItemWidth(toolview_width-10);
  ImGui::InputTextMultiline("##DRAWTOOLTEXTUREVIEWALIGNTEXT", ati->textEditText, MAXTEXTEDITTEXT);
  ImGui::PopItemWidth();
  if (roundButton("Bake into layer##DRAWTOOLTEXTUREVIEWBAKE",ati->fontId != 0)) {paintTextIntoLayer(l);}
}

void drawToolOverlaysView() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  bool hasSel = l->hasSelection();
  if (roundButton("Add overlay##DRAWTOOLOVERLAYSVIEWADDOVERLAY",hasSel)) {
    int lx0=0,ly0=0,lx1=0,ly1=0;
    bool first = true;
    for (int y = 0; y < l->height; y++) {
      for (int x = 0; x < l->width; x++) {
        if (l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED) {
          if (first || x < lx0) lx0 = x;
          if (first || y < ly0) ly0 = y;
          if (first || x > lx1) lx1 = x;
          if (first || y > ly1) ly1 = y;
          first = false;
        }
      }
    }
    lx1++;
    ly1++;
    Overlay *o = new Overlay();
    o->layerId = l->id;
    o->lx0 = lx0;
    o->ly0 = ly0;
    o->lx1 = lx1;
    o->ly1 = ly1;
    o->cx0 = 0;
    o->cy0 = 0;
    o->cx1 = lx1-lx0;
    o->cy1 = ly1-ly0;
    currentCanvas->overlays.insert(o,0);
    shouldUpdateAll = true;
  }
  if (roundButton("Add annotation##DRAWTOOLOVERLAYSVIEWADDANNOTATION",hasSel)) {
    int lx0=0,ly0=0,lx1=0,ly1=0;
    bool first = true;
    for (int y = 0; y < l->height; y++) {
      for (int x = 0; x < l->width; x++) {
        if (l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED) {
          if (first || x < lx0) lx0 = x;
          if (first || y < ly0) ly0 = y;
          if (first || x > lx1) lx1 = x;
          if (first || y > ly1) ly1 = y;
          first = false;
        }
      }
    }
    lx1++;
    ly1++;
    Overlay *o = new Overlay();
    o->layerId = l->id;
    o->isAnnotation = true;
    double cx = lx0, cy = ly0; currentCanvas->getCanvasPosFromLayerPos(l,cx,cy,cx,cy);
    o->cx0 = cx;
    o->cy0 = cy;
    currentCanvas->overlays.insert(o,0);
    shouldUpdateAll = true;
  }
  ImGui::Text("Overlays"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLOVERLAYSVIEWSHOWOVERLAYS",&currentCanvas->showOverlays)) {shouldUpdateAll = true;}
  ImGui::Text("Annotations"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLOVERLAYSVIEWSHOWANNOTATIONS",&currentCanvas->showAnnotations)) {shouldUpdateAll = true;}
  ImGui::BeginChild("DRAWTOOLOVERLAYVIEWCHILD",ImVec2(toolview_width,main_window_yres-ImGui::GetCursorScreenPos().y));
  for (int i = 0; i < currentCanvas->overlays.size(); i++) {
    Overlay *o = currentCanvas->overlays[i];
    Layer *l = NULL;
    for (int i2 = 0; i2 < currentCanvas->layers.size(); i2++) {
      if (currentCanvas->layers[i2]->id == o->layerId) {
        l = currentCanvas->layers[i2];
        break;
      }
    }
    if (l != NULL) {
      ImGui::Text("%s",l->name);
      ImGui::Text("Visible"); ImGui::SameLine(); if (ImGui::Checkbox(combined(String("##DRAWTOOLOVERLAYSVIEWVISIBLE")+String::fromInt(i)).c_str(),&o->visible)) {shouldUpdateAll = true;}
      if (!o->isAnnotation) {
        ImGui::Text("W %d H %d",o->lx1-o->lx0,o->ly1-o->ly0);
        int x = o->cx0;
        int y = o->cy0;
        int w = o->cx1 - o->cx0;
        int h = o->cy1 - o->cy0;
        ImGui::PushItemWidth(7*5);
        ImGui::Text("Pos X"); ImGui::SameLine(); if (ImGui::InputInt(combined(String("##DRAWTOOLOVERLAYSVIEWCX0")+String::fromInt(i)).c_str(),&x,0,0)) {shouldUpdateAll = true;}
        ImGui::Text("Pos Y"); ImGui::SameLine(); if (ImGui::InputInt(combined(String("##DRAWTOOLOVERLAYSVIEWCY0")+String::fromInt(i)).c_str(),&y,0,0)) {shouldUpdateAll = true;}
        ImGui::Text("Width"); ImGui::SameLine(); if (ImGui::InputInt(combined(String("##DRAWTOOLOVERLAYSVIEWCX1")+String::fromInt(i)).c_str(),&w,0,0)) {shouldUpdateAll = true;}
        ImGui::Text("Height"); ImGui::SameLine(); if (ImGui::InputInt(combined(String("##DRAWTOOLOVERLAYSVIEWCY1")+String::fromInt(i)).c_str(),&h,0,0)) {shouldUpdateAll = true;}
        ImGui::PopItemWidth();
        o->cx0 = x;
        o->cy0 = y;
        o->cx1 = x+w;
        o->cy1 = y+h;
      } else {
        if (ImGui::InputText(combined(String("##DRAWTOOLOVERLAYSVIEWNAMETEXT")+String::fromInt(i)).c_str(),o->annotationText,ANNOTATIONTEXTMAX)) {shouldUpdateAll = true;}
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImVec4 chere = getImVec4Color(o->annotationColor); if (colorItem("", "##DRAWTOOLOVERLAYANNOTATIONCOLOR", &chere)) {o->annotationColor = getUIntColor(chere);shouldUpdateAll = true;}
        ImGui::SetCursorScreenPos(cursor);
        if (ImGui::InvisibleButton("##DRAWTOOLOVERLAYVIEWANNOTATIONCOLORINVISIBLE",ImVec2(16/aspectRatio*main_window_xres/main_window_yres,16))) {
          o->annotationColor = getUIntColor(foregroundColor);
          shouldUpdateAll = true;
        }
        ImGui::Text("Font size"); ImGui::SameLine(); if (ImGui::InputFloat(combined(String("##DRAWTOOLOVERLAYSVIEWANNOTATIONFONTSIZE")+String::fromInt(i)).c_str(),&o->annotationFontSize,0,0,2)) {shouldUpdateAll = true;} if (o->annotationFontSize < 1) o->annotationFontSize = 1;
      }
    }
    if (roundButton(combined(String("Remove##DRAWTOOLOVERLAYSVIEWREMOVEOVERLAY")+String::fromInt(i)).c_str(),true)) {
      shouldUpdateAll = true;
      delete o; currentCanvas->overlays.erase(i,1);
      i--;
    }
    ImGui::Separator();
  }
  ImGui::EndChild();
}

void drawToolView() {
  if (fullScreen) return;
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
  case TOOL_LINE: {
      drawToolLineView();
    } break;
  case TOOL_GRADIENT: {
      drawToolGradientView();
    } break;
  case TOOL_MAGIC: {
      drawToolMagicView();
    } break;
  case TOOL_MEASURE: {
      drawToolMeasureView();
    } break;
  case TOOL_TILING: {
      drawToolTilingView();
    } break;
  case TOOL_BLUR: {
      drawToolBlurView();
    } break;
  case TOOL_TEXT: {
      drawToolTextView();
    } break;
  case TOOL_OVERLAYS: {
      drawToolOverlaysView();
    } break;
  }
  ImGui::End();
}

void toolChangeMessage(int lastTool, int tool) {
  switch(lastTool) {
  case TOOL_TILING : {Layer *l = currentCanvas->getActiveLayer(); if (l != NULL) updateTilingGrid(l,false);} break;
  }
  switch(tool) {
  case TOOL_TILING : {Layer *l = currentCanvas->getActiveLayer(); if (l != NULL) updateTilingGrid(l,true);} break;
  }
}
