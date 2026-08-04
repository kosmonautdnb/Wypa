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
#include "FILTER.HPP"
#include "STL/IMAGE.HPP"
#include "UNDO.HPP"
#include "TEXT.HPP"
#include "STL/DOS.HPP"
#include "SERIALZE.HPP"
#include "GALLERY.HPP"
#include <math.h>

int toolview_width = 120;

bool roundButton(const char *name, bool enabled);
bool imageButton(const char *svgName, const char *id, bool selected, bool hilighted, bool disabled);

GLuint gradientTexture = 0;
const int gradientTextureWidth = 100;
const int gradientTextureHeight = 50;
//unsigned int gradientTextureContent[gradientTextureWidth*gradientTextureHeight];
Array<unsigned int> gradientTextureContent; // because of PModeW with WatcomC++11

GLuint tilingTexture = 0;
const int tilingTextureMul = 2;
const int tilingTextureWidth = 100*tilingTextureMul;
const int tilingTextureHeight = 100*tilingTextureMul;
//unsigned int tilingTextureContent[tilingTextureWidth*tilingTextureHeight];
//unsigned int tilingTextureContentAnim[tilingTextureWidth*tilingTextureHeight];
Array<unsigned int> tilingTextureContent; // because of PModeW with WatcomC++11
Array<unsigned int> tilingTextureContentAnim; // because of PModeW with WatcomC++11

GLuint tileMapTexture = 0;

void initToolView() {
  gradientTextureContent.resize(gradientTextureWidth*gradientTextureHeight);
  tilingTextureContent.resize(tilingTextureWidth*tilingTextureHeight);
  tilingTextureContentAnim.resize(tilingTextureWidth*tilingTextureHeight);

  glGenTextures(1,&gradientTexture);
  glBindTexture(GL_TEXTURE_2D,gradientTexture);
  memset(&gradientTextureContent[0],0,gradientTextureContent.size()*sizeof(unsigned int));
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, gradientTextureWidth, gradientTextureHeight, 0, GL_RGBA, GL_BYTE, &gradientTextureContent[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);

  glGenTextures(1,&tilingTexture);
  glBindTexture(GL_TEXTURE_2D,tilingTexture);
  memset(&tilingTextureContent[0],0,tilingTextureContent.size()*sizeof(unsigned int));
  memset(&tilingTextureContentAnim[0],0,tilingTextureContentAnim.size()*sizeof(unsigned int));
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, tilingTextureWidth, tilingTextureHeight, 0, GL_RGBA, GL_BYTE, &tilingTextureContent[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);

  glGenTextures(1,&tileMapTexture);
  glBindTexture(GL_TEXTURE_2D,tileMapTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);
}

void freeToolView() {
  glDeleteTextures(1,&gradientTexture);
  glDeleteTextures(1,&tilingTexture);
  glDeleteTextures(1,&tileMapTexture);
}

void repaintGradientTexture(Gradient *gradient) {
  if (gradientTexture != 0) {
    for (int y = 0; y < gradientTextureHeight; y++) {
      for (int x = 0; x < gradientTextureWidth; x++) {
        float fx = (float)x / (gradientTextureWidth-1);
        float fy = (float)y / (gradientTextureHeight-1);
        if (gradient->round) {
          fx = fx * 2 - 1;
          fy = fy * 2 - 1;
        }
        unsigned int col = gradient->getColor2D(fx,fy);
        gradientTextureContent[x+y*gradientTextureWidth] = gammaCorrect(col);
      }
    }
    glBindTexture(GL_TEXTURE_2D,gradientTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, gradientTextureWidth, gradientTextureHeight, 0, GL_RGBA, GL_BYTE, &gradientTextureContent[0]);
    glBindTexture(GL_TEXTURE_2D,0);
  }
}

void drawToolWandView() {
  Layer *l = currentCanvas->getActiveLayer(); if (l != NULL && locked(l,LOCK_PIXELS_AND_TILEMAP)) return;
  if (!watershed) {
    if (controlKeyId == CONTROLKEY_SHIFT) {ImGui::Text("Add selection");} 
    else
      if (controlKeyId == CONTROLKEY_CTRL) {ImGui::Text("Remove selection");} 
    else
      if (controlKeyId == (CONTROLKEY_SHIFT|CONTROLKEY_CTRL)) {ImGui::Text("Mul selection");} 
    else
    ImGui::Text("");
    if (l->type != LAYERTYPE_TILEMAP) {
      float k;
      k = wandColorDistance; ImGui::Text("Color distance"); if (ImGui::SliderFloat("##DRAWTOOLWANDVIEWCOLORDISTANCE",&k,0,256)) {wandColorDistance = k;} else toolTip("Sameness value for color");
      k = wandAlphaDistance; ImGui::Text("Alpha distance"); if (ImGui::SliderFloat("##DRAWTOOLWANDVIEWALPHADISTANCE",&k,0,256)) {wandAlphaDistance = k;} else toolTip("Sameness value for transparency");
    }
    ImGui::Text("Surrounding"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLWANDVIEWNEIGHBORS",&wandByNeighbors)) {} else toolTip("Use only surrounding areas");
    ImGui::Text("Slow:");
    if (roundButton("Segmentation",true)) {
      watershed = true;
    }
  } else {
    if (roundButton("Close##DRAWTOOLWANDVIEWCLOSE",true)) {
      watershed = false;
    }
    ImGui::Text("Watershed algo.");
    if (roundButton("Render##DRAWTOOLWANDVIEWRENDER",watershedPoints.size()>0)) {
      filterWatershed(l);
      shouldUpdateAll = true;
    }
    if (roundButton("Clear##DRAWTOOLWANDVIEWRESET",true)) {
      watershedPoints.clear();
    }
    if (l != NULL && l->hasSelection()) {
      ImGui::Text("Selection as:");
      if (!watershedSelectionOutside) {ImGui::Text("Inside"); ImGui::SameLine(); ImGui::Checkbox("##DRAWTOOLWANDVIEWSELECTIONINSIDE",&watershedSelectionInside);}
      if (!watershedSelectionInside) {ImGui::Text("Outside"); ImGui::SameLine(); ImGui::Checkbox("##DRAWTOOLWANDVIEWSELECTIONOUTSIDE",&watershedSelectionOutside);}
    } else {
      ImGui::Text("Maybe use");
      ImGui::Text("selections, too.");
    }
    ImGui::Text("RMouse = outside");
    ImGui::Text("LMouse = inside");
    ImGui::Separator();
    ImGui::Text("Points:%d",watershedPoints.size());
    ImGui::BeginChild("DRAWTOOLWANDVIEWCHILD",ImVec2(toolview_width,main_window_yres-ImGui::GetCursorScreenPos().y));
    watershedSelectedPoint = -1;
    for (int i = 0; i < (int)watershedPoints.size(); i++) {
      ImVec2 v0 = ImGui::GetCursorScreenPos();
      WatershedPoint *p = &watershedPoints[i];
      if (ImGui::Checkbox(combined("##DRAWTOOLWANDVIEWSELECTION_"+String::fromInt(i)).c_str(),&p->selection)) {;} else toolTip("Inside selection or outside selection");
      ImGui::SameLine();
      SVG *svg = &svgs["delete"];
      ImGui::PushID(combined("DRAWTOOLWANDVIEWDELETE_"+String::fromInt(i)).c_str());
      bool eraseThis = false;
      if (ImGui::ImageButton((ImTextureID)svg->glHandle,ImVec2(svg->w,svg->h))) {
        eraseThis = true;
      } else toolTip("Delete this marker");
      ImGui::PopID();
      ImGui::SameLine(); ImGui::Text("%d,%d",(int)p->x,(int)p->y); toolTip("X and Y canvas position of this marker");
      if(eraseThis) {
        watershedPoints.erase(i,1); i--;
      }
      ImVec2 v1 = v0;
      v1.x += toolview_width;
      v1.y += svg->h;
      if (ImGui::IsMouseHoveringRect(v0,v1)) watershedSelectedPoint = i;;
    }
    ImGui::EndChild();
  }
}

void drawToolFillView() {
  Layer *l = currentCanvas->getActiveLayer(); if (l != NULL && locked(l,LOCK_PIXELS_AND_TILEMAP)) return;
  float k;
  k = fillColorDistance; ImGui::Text("Color distance"); if (ImGui::SliderFloat("##DRAWTOOLFILLCOLORDISTANCE",&k,0,256)) {fillColorDistance = k;} else toolTip("Sameness value for fill area color");
  ImGui::Text("Use valleys"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLFILLVALLEYS",&fillUseValleys)) {} else toolTip("Flow up and down the filling points");
}

void drawToolSelectionView() {
  if (controlKeyId == CONTROLKEY_SHIFT) {ImGui::Text("Add selection");} 
  else
    if (controlKeyId == CONTROLKEY_CTRL) {ImGui::Text("Remove selection");} 
  else
    if (controlKeyId == (CONTROLKEY_SHIFT|CONTROLKEY_CTRL)) {ImGui::Text("Mul selection");} 
  else
    ImGui::Text("");
  ImGui::RadioButton("Rectangle##DRAWTOOLSELECTIONVIEWRECT",&selectionTool,SELECTIONTOOL_RECTANGLE); toolTip("Select a rectangle by mouse");
  ImGui::RadioButton("Ellipse##DRAWTOOLSELECTIONVIEWELLIPSE",&selectionTool,SELECTIONTOOL_ELLIPSE); toolTip("Select a circle/ellipse by mouse");
  ImGui::RadioButton("Lasso##DRAWTOOLSELECTIONVIEWLASSO",&selectionTool,SELECTIONTOOL_LASSO); toolTip("Paint a selection shape by mouse");
  ImGui::RadioButton("Polygon##DRAWTOOLSELECTIONVIEWPOLYGON",&selectionTool,SELECTIONTOOL_POLYGON); toolTip("Paint a selection shape as polygon");
  ImGui::Separator();
  switch(selectionTool) {
    case SELECTIONTOOL_ELLIPSE:
    case SELECTIONTOOL_RECTANGLE: {
      ImGui::Text("Proportional"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLSELECTIONVIEWPROPORTIONAL",&selectionProportional)) {} else toolTip("Same width and height");
    } break;
  }
}

bool notInt(double v) {
  return fabs(v - floor(v))>0.0000001;
}

bool notIntPosX(const Array<Layer *> &selected) {for (int i = 0; i < (int)selected.size(); i++) {Vector k = selected[i]->boundingBox(); if (notInt(k.x)) return true;}  return false;}
bool notIntPosY(const Array<Layer *> &selected) {for (int i = 0; i < (int)selected.size(); i++) {Vector k = selected[i]->boundingBox(); if (notInt(k.y)) return true;}  return false;}
bool notIntWidth(const Array<Layer *> &selected) {for (int i = 0; i < (int)selected.size(); i++) {Vector k = selected[i]->boundingBox(); if (notInt(k.z-k.x)) return true;}  return false;}
bool notIntHeight(const Array<Layer *> &selected) {for (int i = 0; i < (int)selected.size(); i++) {Vector k = selected[i]->boundingBox(); if (notInt(k.w-k.y)) return true;}  return false;}

void drawToolMoveView() {
  Array<Layer *> selected;
  for (int i = 0; i < (int)currentCanvas->layers.size(); i++) {if (currentCanvas->layers[i]->state.selected||currentCanvas->layers[i]->state.active) selected.push_back(currentCanvas->layers[i]);}
  if (!selected.empty()) {
    Vector b;
    for (int i = 0; i < (int)selected.size(); i++) {
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
      k = x0; ImGui::Text("Layer pos x"); if (ImGui::InputInt("##DRAWTOOLMOVEVIEWPOSX",&k)) {l->setLayerPos(k,y0); shouldUpdateAll = true;} if (notInt(b.x)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;} else toolTip("Change layers x position");
      k = y0; ImGui::Text("Layer pos y"); if (ImGui::InputInt("##DRAWTOOLMOVEVIEWPOSY",&k)) {l->setLayerPos(x0,k); shouldUpdateAll = true;} if (notInt(b.y)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;} else toolTip("Change layers y position");
      k = x1-x0; ImGui::Text("Layer width"); if (ImGui::InputInt("##DRAWTOOLMOVEVIEWWIDTH",&k)) {if (k<1) k = 1;l->setLayerSize(k,y1-y0); shouldUpdateAll = true;} if (notInt(b.z-b.x)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;} else toolTip("Change layers width");
      k = y1-y0; ImGui::Text("Layer height"); if (ImGui::InputInt("##DRAWTOOLMOVEVIEWHEIGHT",&k)) {if (k<1) k = 1;l->setLayerSize(x1-x0,k); shouldUpdateAll = true;} if (notInt(b.w-b.y)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;} else toolTip("Change layers height");
      float k2 = l->getAngleDegrees(); ImGui::Text("Layer rotation"); if (ImGui::InputFloat("##DRAWTOOLMOVEVIEWROTATION",&k2)) {if(k2<0) k2=0; if (k2>360) k2 = 360;l->setAngleDegrees(k2);shouldUpdateAll = true;} else toolTip("Change layer rotation");
      if (roundButton("Fit boundings##DRAWTOOLMOVEVIEWFITTOCANVAS",somethingToFit)) {
        l->quantizeBoundingBox();
      } else toolTip("Put layer on straight coordinates");
      ImGui::Separator();
      if (roundButton("Reset transform##DRAWTOOLMOVEVIEWRESETLAYERTRANSFORM",true)) {
        l->transform.fresh();
        l->transform.translate(-b.x,-b.y);
        shouldUpdateAll = true;
      } else toolTip("Layer with initial size and transform");
      if (roundButton("Snap to grid##DRAWTOOLMOVEVIEWSNAPTOGRID",currentCanvas->gridOn)) {
        l->snapToGrid(currentCanvas);
      } else toolTip("Arange in grid");
    } else {
      k = x0; ImGui::Text("Bounding pos x"); if (ImGui::InputInt("##DRAWTOOLBOUNDINGPOSX",&k)) {currentCanvas->setBoundingPos(k,y0); shouldUpdateAll = true;} if (notIntPosX(selected)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;} else toolTip("Change layers x position");
      k = y0; ImGui::Text("Bounding pos y"); if (ImGui::InputInt("##DRAWTOOLBOUNDINGPOSY",&k)) {currentCanvas->setBoundingPos(x0,k); shouldUpdateAll = true;} if (notIntPosY(selected)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;} else toolTip("Change layers y position");
      k = x1-x0; ImGui::Text("Bounding width"); if (ImGui::InputInt("##DRAWTOOLBOUNDINGWIDTH",&k)) {if (k<1) k = 1;currentCanvas->setBoundingSize(k,y1-y0); shouldUpdateAll = true;} if (notIntWidth(selected)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;} else toolTip("Change layers width");
      k = y1-y0; ImGui::Text("Bounding height"); if (ImGui::InputInt("##DRAWTOOLBOUNDINGHEIGHT",&k)) {if (k<1) k = 1;currentCanvas->setBoundingSize(x1-x0,k); shouldUpdateAll = true;} if (notIntHeight(selected)) {ImGui::SameLine(); ImGui::Text("!"); somethingToFit = true;} else toolTip("Change layers height");
      float k2 = currentCanvas->getBoundingAngleDegrees(); ImGui::Text("Bounding rotation"); if (ImGui::InputFloat("##DRAWTOOLNOUNDINGROTATION",&k2)) {if(k2<0) k2=0; if (k2>360) k2 = 360;currentCanvas->setBoundingAngleDegrees(k2);shouldUpdateAll = true;} toolTip("Change layer rotation");
      if (roundButton("Fit boundings##DRAWTOOLMOVEVIEWFITTOCANVAS",somethingToFit)) {
        for (int i = 0; i < (int)selected.size(); i++)
          selected[i]->quantizeBoundingBox();
      } else toolTip("Put layers on straight coordinates");
      if (roundButton("Snap to grid##DRAWTOOLMOVEVIEWBOUNDINGSNAPTOGRID",currentCanvas->gridOn)) {
        currentCanvas->snapToGrid();
      } else toolTip("Arange in grid");
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
  if ((!brushes.empty()) && currentBrush>=(int)brushes.size()) currentBrush = (int)brushes.size()-1;
}

void drawBrushSelect(bool pencil) {
  int toolButtonXCount = 4;
  int toolButtonYCount = 4;
  int toolButtonHeight = (toolview_width - 16) / toolButtonXCount - 4;
  int toolButtonWidth = toolButtonHeight;
  if (controlKeyId == CONTROLKEY_CTRL)
    ImGui::Text("Erase (hard)");
  else
    ImGui::Text("");
  if (roundButton("Insert##DRAWBRUSHSELECTINSERTBRUSH",true)) {insertBrush();} else toolTip("Insert a new brush");
  ImGui::SameLine();
  if (roundButton("Erase##DRAWBRUSHSELECTERASEBRUSH",true)) {eraseBrush();} else toolTip("Erase the current brush");
  //ImGui::Text("Brushes %d",brushes.size()); toolTip("Current brush count");
  ImGui::BeginChild("DRAWBRUSHSELECTCHILD", ImVec2(toolview_width,toolButtonHeight*toolButtonYCount));
  for (int i = 0; i < (int)brushes.size(); i++) {
    if ((i % toolButtonXCount) != 0) ImGui::SameLine();
    Brush *brush = brushes[i];
    if (drawBrushWindow(combined(String("##DRAWBRUSHSELECT")+String::fromInt(i)).c_str(),BRUSHINSTANCE_SELECT,brush,toolButtonWidth,toolButtonHeight, pencil, i == currentBrush)) {
      brushes[currentBrush]->freeInstance(BRUSHINSTANCE_PAINT);
      brushes[currentBrush]->freeInstance(BRUSHINSTANCE_EDIT);
      currentBrush = i;
    }
  }
  ImGui::EndChild(); toolTip("Brush palette");
  if (roundButton("Load##DRAWBRUSHSELECTLOADBRUSH",true)) {dialogLoadSave(true,DIALOG_LOADBRUSH);} else toolTip("Load a brush to current brush");
  ImGui::SameLine();
  if (roundButton("Save##DRAWBRUSHSELECTSAVEBRUSH",true)) {dialogLoadSave(true,DIALOG_SAVEBRUSH);} else toolTip("Save the current brush");
  ImGui::SameLine();
  if (roundButton("Get##DRAWBRUSHSELECTGETBRUSH1",true)) {
    Layer *l = currentCanvas->getActiveLayer();
    if (l != NULL && l->hasSelection()) {
      if ((unsigned int)currentBrush<brushes.size()) {
        Brush *brush = brushes[currentBrush];
        int minX = 0, minY = 0, maxX = 0, maxY = 0; bool first = true;
        for (int y = 0; y < l->height; y++) {
          for (int x = 0; x < l->width; x++) {
            if (l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED) {
              if (first || x < minX) minX = x;
              if (first || y < minY) minY = y;
              if (first || x > maxX) maxX = x;
              if (first || y > maxY) maxY = y;
              first = false;
            }
          }
        }
        maxX++;
        maxY++;
        RGBAImage img;
        img.width = maxX-minX;
        img.height = maxY-minY;
        img.data = (unsigned int*)malloc(img.width*img.height*sizeof(unsigned int));
        for (int y2 = 0; y2 < img.height; y2++) {
          for (int x2 = 0; x2 < img.width; x2++) {
            int sx = x2 + minX;
            int sy = y2 + minY;
            unsigned int col = 0x00000000;
            if ((unsigned int)sx < (unsigned int)l->width && (unsigned int)sy < (unsigned int)l->height) {
              col = l->pixels[sx+sy*l->width];
            }
            img.data[x2+y2*img.width] = col;
          }
        }
        if (img.width > 128 || img.height > 128) {
          int m = img.width;
          if (img.height > m) m = img.height;
          if (m != 0) {
            RGBAImage img2 = img.getResized(128*img.width/m,128*img.height/m);
            img.free();
            img = img2;
          }
        }
        scanSampleDistanceField(brush, img);
        brush->isDistanceField = true;
        brush->update();
        img.free();
      }
    }
  } else toolTip("Get a transparency based brush from selection");
  if (roundButton("Load brushes##DRAWBRUSHSELECTLOADBRUSHES",true)) {dialogLoadSave(true,DIALOG_LOADBRUSHES);} else toolTip("Load a brush palette");
  if (roundButton("Save brushes##DRAWBRUSHSELECTSAVEBRUSHES",true)) {dialogLoadSave(true,DIALOG_SAVEBRUSHES);} else toolTip("Save the brush palette");
  ImGui::Separator();
}

void drawBrushEdit(const char *id, Brush *brush, bool pencil) {
  ImGui::InputText("##DRAWBRUSHEDITNAME",brush->name,BRUSHNAMEMAX); toolTip("Brush name");
  ImGui::SameLine(); if (ImGui::Checkbox("##DRAWBRUSHEDITOUTLINE",&toolOutline)) {} else toolTip("Show brush shape whilst painting");
  if (drawBrushWindow(id,BRUSHINSTANCE_EDIT,brush,toolview_width-10,toolview_width-10, pencil,false)) {
    dialogLoadSave(true,DIALOG_LOADBRUSHSHAPE);
  } else toolTip("Load a .svg as brush");
  if (tool == TOOL_ERASER) {
    ImGui::Text("Hard"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWBRUSHEDITERASERHARD",&hardEraser)) {} else toolTip("Erase is binary");
  }
  ImGui::Text("Size");
  ImGui::SameLine();if (ImGui::SliderFloat("##DRAWBRUSHEDITSIZE",&brush->size,1,128)) {if (brush->size<1) brush->size = 1; if (brush->size>128) brush->size = 128; brush->update();} else toolTip("Size of brush in pixels");
  if (!pencil) {
    ImGui::Text("Pressure");
    if (ImGui::SliderFloat("##DRAWBRUSHEDITPRESSURE",&brush->pressure,0,2)) {if (brush->pressure<0) brush->pressure = 0; brush->update();} else toolTip("Stroke intensity");
  }
  ImGui::Text("Opacity");
  if (ImGui::SliderFloat("##DRAWBRUSHEDITOPACITY",&brush->opacity,0,1)) {if (brush->opacity<0) brush->opacity = 0; if (brush->opacity>1) brush->opacity = 1; brush->update();} else toolTip("Additional transparency");
  ImGui::Text("Aspect");
  if (ImGui::SliderFloat("##DRAWBRUSHEDITASPECT",&brush->aspect,1,8)) {if (brush->aspect<1) brush->aspect = 0; if (brush->aspect>8) brush->aspect = 8; brush->update();} else toolTip("X vs Y pixel size");
  ImGui::Text("Rotation");
  if (ImGui::SliderFloat("##DRAWBRUSHEDITROTATION",&brush->rotation,0,360)) {if (brush->rotation<0) brush->rotation = 0; if (brush->rotation>360) brush->rotation = 360; brush->update();} else toolTip("Rotation of the brush");
  if (!pencil) {
    ImGui::Text("Softness");
    float k = brush->softness-1; if (ImGui::SliderFloat("##DRAWBRUSHEDITSOFTNESS",&k,-1,1)) {if (k<-1) k = -1; if (k>1) k = 1; brush->softness = k + 1; brush->update();} else toolTip("Intensity of brush shape");
  }
  ImGui::Text("Edge");
  ImGui::SameLine();if (ImGui::SliderFloat("##DRAWBRUSHEDITEDGINESS",&brush->edginess,0,1)) {if (brush->edginess<0) brush->edginess = 0; if (brush->edginess>1) brush->edginess = 1; brush->update();} else toolTip("Brush to rectangle");
  ImGui::Text("Oval");
  ImGui::SameLine();if (ImGui::SliderFloat("##DRAWBRUSHEDITOVALITY",&brush->ovality,0,1)) {if (brush->ovality<0) brush->ovality = 0; if (brush->ovality>1) brush->ovality = 1; brush->update();} else toolTip("Make brush more or less oval");
  float k;
  ImGui::Separator();
  ImGui::PushItemWidth(7*5+16*2);
  ImGui::Text("Rate");
  ImGui::SameLine(); if (ImGui::SliderFloat("##DRAWBRUSHEDITRATE",&brush->rate,0,1)) {if (brush->rate<0) brush->rate = 0; if (brush->rate>1) brush->rate = 1; brush->update();} else toolTip("Sprinkle intensity");
  ImGui::Text("Width");
  ImGui::SameLine(); k = (double)brush->size/(double)brush->aspect; if (ImGui::InputFloat("##DRAWBRUSHEDITWIDTH",&k,1,1)) {if (k<1) k = 1; if (k > 128) k = 128; brush->aspect = (float)((double)brush->size/(double)k); brush->update();} else toolTip("Brush width");
  ImGui::Text("Height");
  ImGui::SameLine(); k = brush->size; if (ImGui::InputFloat("##DRAWBRUSHEDITHEIGHT",&k,1,1)) {double widthBefore = (double)brush->size/(double)brush->aspect; if (widthBefore == 0) widthBefore = 1; if(k<1) k = 1; if (k>128) k = 128; brush->size = k; brush->aspect = (float)((double)brush->size/widthBefore); brush->update();} else toolTip("Brush height");
  ImGui::Text("SpreadX");
  ImGui::SameLine(); if (ImGui::InputFloat("##DRAWBRUSHEDITRANDOMX",&brush->randomX)) {brush->update();} else toolTip("Sprinkle in x");
  ImGui::Text("SpreadY");
  ImGui::SameLine();if (ImGui::InputFloat("##DRAWBRUSHEDITRANDOMY",&brush->randomY)) {brush->update();} else toolTip("Sprinkle in y");
  ImGui::Text("Rotate"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWBRUSHEDITFOLLOWROTATION",&brush->followRotation)) {} else toolTip("Try to follow the paint curve");
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
  drawBrushSelect(hardEraser);
  if ((unsigned int)currentBrush<brushes.size()) drawBrushEdit("##DRAWTOOLERASERVIEWEDIT",brushes[currentBrush],hardEraser);
}

void drawToolPipetteView() {
  if (roundButton(pipetteFromLayer ? "From picture##DRAWTOOLPIPETTEVIEWLAYER" : "From layer##DRAWTOOLPIPETTEVIEWLAYER", true)) {pipetteFromLayer = !pipetteFromLayer;} else toolTip("Take color from picture or single layer");
  if (pipetteFromLayer) {
    ImGui::Text("Postprocessing"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLPIPETTEVIEWLAYERCOLORGRADING",&pipetteWithColorTransform)) {} else toolTip("Get layer color including its postprocessing");
  }
  ImGui::Text("");
  ImVec4 chere; 
  ImGui::Text("Color canvas"); toolTip("Current picture color under mouse");
  chere = getImVec4Color(currentCanvas->getCanvasColor(mouseX,mouseY)); colorItem("", "##DRAWTOOLPIPETTEVIEWCOLORCANVAS", &chere);
  ImGui::Text("Color layer"); toolTip("Current layer color under mouse");
  chere = getImVec4Color(currentCanvas->getLayerColor(mouseX,mouseY,pipetteWithColorTransform)); colorItem("", "##DRAWTOOLPIPETTEVIEWCOLORLAYER", &chere);
}

void drawToolLineView() {
  ImGui::Text("Line width");
  if (ImGui::InputFloat("##DRAWTOOLLINEVIEWLINEWIDTH",&lineWidth,0,0,2)) {} else toolTip("Width of the line(s)"); if (lineWidth < 0) lineWidth = 0;
  ImGui::Text("Line cap"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLLINEVIEWLINECAP",&lineCap)) {} else toolTip("Also draw caps at line ends");
  ImGui::Text("Antialiasing"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLLINEVIEWLINEANTIALIASING",&lineAntialiasing)) {} else toolTip("Use antialiasing for the line(s)");
}

void refreshToolViewTextures() {
  repaintGradientTexture(&gradient);
}

void updateGradient() {
  gradient.update();
  repaintGradientTexture(&gradient);
}

void drawToolGradientView() {
  static String neededScroll = "";
  if (roundButton("Load##DRAWTOOLGRADIENTVIEWLOAD",true)) {dialogLoadSave(true,DIALOG_LOADGRADIENT);} else toolTip("Load a gradient .gr2 file");
  ImGui::SameLine();
  if (roundButton("Save##DRAWTOOLGRADIENTVIEWSAVE",!gradient.elements.empty())) {dialogLoadSave(true,DIALOG_SAVEGRADIENT);} else toolTip("Save the gradient to a .gr2 file");
  ImGui::Image((ImTextureID)gradientTexture,ImVec2(gradientTextureWidth,gradientTextureHeight)); toolTip("The current gradient");
  ImVec2 minB = ImGui::GetItemRectMin();
  ImVec2 maxB = ImGui::GetItemRectMax();
  minB.x -= 1;
  minB.y -= 1;
  ImGui::GetWindowDrawList()->AddRect(minB,maxB,0xff000000);
  ImGui::Text("Start %f", gradient.positionMin); toolTip("First gradient \"value\"");
  ImGui::Text("End   %f", gradient.positionMax); toolTip("Last gradient \"value\"");
  ImGui::Text("Inverted"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLGRADIENTVIEWINVERTED",&gradient.inverted)) updateGradient(); else toolTip("Mirror the gradient");
  ImGui::Text("Double sided"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLGRADIENTVIEWDOUBLESIDED",&gradient.doubleSided)) updateGradient(); else toolTip("Up+down gradient");
  ImGui::Text("Circular"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLGRADIENTVIEWCIRCULAR",&gradient.round)) updateGradient(); else toolTip("Gradient \"sphere\"");
  if (roundButton("Add element##DRAWTOOLGRADIENTVIEWADDELEMENT",true)) {
    gradient.elements.push_back(GradientElement());
    updateGradient();
  } else toolTip("Add a gradient element");
  ImGui::BeginChild("DRAWTOOLGRADIENTVIEWCHILD",ImVec2(toolview_width,main_window_yres-ImGui::GetCursorScreenPos().y));
  for (int i = 0; i < (int)gradient.elements.size(); i++) {
    GradientElement *e = &gradient.elements[i];
    String id = String("##DRAWTOOLGRADIENTVIEWELEMENT")+String::fromInt(e->id);
    if (id == neededScroll) {
      ImGui::SetScrollPosHere();
      neededScroll = "";
    }
    ImGui::PushItemWidth(7*5);
    if (ImGui::InputFloat((id+"FLOAT").c_str(),&e->position,0,0,2)) {if (e->position<0) e->position = 0; if (e->position>100000) e->position = 100000; neededScroll = id; updateGradient();} else toolTip("Gradient \"value\"");
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Checkbox((id+"STEP").c_str(),&e->isStep)) updateGradient(); else toolTip("Step vs. linear gradient element");
    ImGui::SameLine();
    ImGui::PushID((id+"IMAGEBUTTON").c_str());
    SVG *svg = &svgs["delete"];
    if (ImGui::ImageButton((ImTextureID)svg->glHandle,ImVec2(svg->w,svg->h))) {
      gradient.elements.erase(i,1);
      updateGradient();
      if ((unsigned int)i<gradient.elements.size()) e = &gradient.elements[i]; else e = NULL;
      i--;
    } else toolTip("Delete this gradient element");
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

  ImGui::Text("In selection");ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWJUSTINSELECTION",&l->showColorTransformOnlyInSelection)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Do the postprocessing just in the selection");
  if (roundButton("Bake into##DRAWTOOLMAGICVIEWAPPLYCOLORGRADING",true)) {beforeUndoPoint();l->bakeMagic(l->hasSelection()); l->colorTransform.useColorTransform = false; l->colorTransform.useLut = false; l->colorTransform.usePosterize = false; l->colorTransform.useThreshold = false; l->colorTransform.useQuantization = false; afterUndoPoint(); shouldUpdateAll = true;} else toolTip("Apply the postprocessing to the layer");
  ImGui::Separator();
  if (roundButton("Reset##DRAWTOOLMAGICVIEWAPPLYCOLORGRADINGRESET",true)) {l->colorTransform.reset(); shouldUpdateAll = true;} else toolTip("Clear all postprocessing settings");
  if (roundButton("Copy##DRAWTOOLMAGICVIEWAPPLYCOLORGRADINGCOPY",true)) {l->copyColorTransform();} else toolTip("Copy the postprocessing settings");
  ImGui::SameLine();
  if (roundButton("Paste##DRAWTOOLMAGICVIEWAPPLYCOLORGRADINGPASTE",true)) {l->pasteColorTransform();} else toolTip("Paste the postprocessing settings");
  ImGui::Separator();

  ImGui::BeginChild("DRAWTOOLMAGICVIEWCHILD",ImVec2(toolview_width, main_window_yres-ImGui::GetCursorScreenPos().y));

  ImGui::Text("Colorgrading"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWUSECOLORTRANSFORM",&l->colorTransform.useColorTransform)) {
    shouldUpdateAll = true;
  } else toolTip("Use the colorgrading settings");
  if (l->colorTransform.useColorTransform) {

    ImGui::BeginChild("DRAWTOOLMAGICCHILDCOLORTRANSFORM",ImVec2(0,200));

    ImGui::Text("Picture Gamma");
    v = &t->pictureGamma; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPICTUREGAMMA",v,0,0,2)) {if (*v<0) *v = 0; if (*v>10) *v = 10; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Apply this gamma before the colorgrading starts");
    ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWGAPICTUREGAMMAINVERT",&t->pictureGammaInvert)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Invert the gamma here gamma=(1.0/gamma)");

    ImGui::Text("Contrast");
    v = &t->contrast; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWCONTRAST",v,0,2)) {if (*v<0) *v = 0; if (*v>2) *v = 2; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("The less contrast the more grey");
    ImGui::Text("Brightness");
    v = &t->brightness; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWBRIGHTNESS",v,-1,1)) {if (*v<-1) *v = -1; if (*v>1) *v = 1; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Change the brightness");

    ImGui::Text("Remove green"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICREMOVEGREEN",&t->removeGreen)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Set green channel to 0 in order to e.g. change the hue of grey");

    ImGui::Text("Hue");
    v = &t->hue; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWHUE",v,0,360)) {if (*v<0) *v = 0; if (*v>360) *v = 360; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Change the hue of the layer");
    ImGui::Text("Saturation");
    v = &t->saturation; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWSATURATION",v,0,2)) {if (*v<0) *v = 0; if (*v>2) *v = 2; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Change the saturation of the layer");
    ImGui::Text("Lightness");
    v = &t->lightness; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWLIGHTNESS",v,0,1)) {if (*v<0) *v = 0; if (*v>4) *v = 4; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Change the lightness of the layer");

    ImGui::Text("Perceptional"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICPERCEPTIONAL",&t->perceptional)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("The hue change should be handled \"normalized\"");

    if (roundButton("Color matrix##DRAWTOOLMAGICCOLORMATRIX",true)) {dialogColorMatrix(true);} else toolTip("Change the rgb value influences per channel");

    ImGui::Text("p - r - y");
    v = &t->middleRedAdjust; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWMIDDLEREDADJUST",v,-1,1)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Purple - red - yellow change of red influence"); if (*v<-1) *v = -1; if (*v>1) *v = 1;
    ImGui::Text("y - g - c");
    v = &t->middleGreenAdjust; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWMIDDLEGREENADJUST",v,-1,1)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Yellow - green - cyan change of green influence"); if (*v<-1) *v = -1; if (*v>1) *v = 1;
    ImGui::Text("c - b - p");
    v = &t->middleBlueAdjust; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWMIDDLEBLUEADJUST",v,-1,1)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Cyan - blue - purple change of blue influence");  if (*v<-1) *v = -1; if (*v>1) *v = 1; 

    ImGui::Text("Scale red");
    v = &t->scaleRed; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWSCALERED",v,0,0,2)) {if (*v<-512) *v = -512; if (*v>512) *v = 512; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Scale red influence by this factor"); 
    ImGui::Text("Scale green");
    v = &t->scaleGreen; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWSCALEGREEN",v,0,0,2)) {if (*v<-512) *v = -512; if (*v>512) *v = 512;  t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Scale alpha influence by this factor"); 
    ImGui::Text("Scale blue");
    v = &t->scaleBlue; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWSCALEBLUE",v,0,0,2)) {if (*v<-512) *v = -512; if (*v>512) *v = 512;  t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Scale blue influence by this factor"); 
    ImGui::Text("Scale alpha");
    v = &t->scaleAlpha; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWSCALEALPHA",v,0,0,2)) {if (*v<-512) *v = -512; if (*v>512) *v = 512;  t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Scale alpha influence by this factor"); 


    ImGui::Text("Add red");
    v = &t->addRed; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWADDRED",v,0,0,2)) {if (*v<-255) *v = -255; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Add this to red (-255..255)"); 
    ImGui::Text("Add green");
    v = &t->addGreen; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWADDGREEN",v,0,0,2)) {if (*v<-255) *v = -255; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Add this to green (-255..255)"); 
    ImGui::Text("Add blue");
    v = &t->addBlue; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWADDBLUE",v,0,0,2)) {if (*v<-255) *v = -255; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Add this to blue (-255..255)"); 
    ImGui::Text("Add alpha");
    v = &t->addAlpha; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWADDALPHA",v,0,0,2)) {if (*v<-255) *v = -255; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Add this to alpha (-255..255)"); 

    ImGui::Text("Invert R"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWINVERTR",&t->invertR)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Invert red (red = 255-red)"); 
    ImGui::Text("Invert G"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWINVERTG",&t->invertG)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Invert green (green = 255-green)"); 
    ImGui::Text("Invert B"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWINVERTB",&t->invertB)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Invert blue (blue = 255-blue)"); 
    ImGui::Text("Invert A"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWINVERTA",&t->invertA)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Invert alpha (alpha = 255-alpha)"); 

    if (roundButton("Gradiation##DRAWTOOLMAGICGRADIENTCURVES",true)) {gradientCurvesToColorTransform = t; dialogGradiationCurves(true);} else toolTip("Use gradiation curves"); 
   
    ImGui::Text("Output gamma");
    v = &t->gamma; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWGAMMA",v,0,0,2)) {if (*v<0) *v = 0; if (*v>10) *v = 10; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Apply this gamma after the colorgrading");
    ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWGAMMAINVERT",&t->gammaInvert)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Invert the gamma here gamma=(1.0/gamma)");

    ImGui::Text("Warmth"); if (ImGui::SliderFloat("##DRAWTOOLMAGICWARMTH",&t->warmth,-1,1)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("The warmth of the color");
      
    ImGui::EndChild();
    ImGui::Separator();
  }

  ImGui::Text("Lookup table"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWUSELUT",&l->colorTransform.useLut)) {shouldUpdateAll = true;} else toolTip("Use a color lookup table file");
  if (l->colorTransform.useLut) {
    if (roundButton(t->lut == NULL ? "LUT##DRAWTOOLMAGICLUT":">>LUT##DRAWTOOLMAGICLUT",true)) {if (t->lut != NULL) {delete t->lut; t->lut = NULL;} shouldUpdateAll = true; dialogLoadSave(true, DIALOG_LUT);} else toolTip("Use a color lookup table, too"); 
    if (t->lut != NULL) {
      ImGui::SameLine(); ImGui::Text("3D %d", t->lutRSize);
      ImGui::Text("Influence");
      if (ImGui::SliderFloat("##DRAWTOOLMAGICLUTINFLUENCE",&t->lutInfluence,0,1)) {shouldUpdateAll = true;} else toolTip("How strong the LUT is against the normal color");
    }
  }

  ImGui::Text("Posterize"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWUSEPOSTERIZE",&l->colorTransform.usePosterize)) {
    shouldUpdateAll = true;
  } else toolTip("Use just steps per color channel (not linear)");
  if (l->colorTransform.usePosterize) {
    ImGui::BeginChild("DRAWTOOLMAGICCHILDPOSTERIZE",ImVec2(0,60));
    ImGui::Text("Use grey"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWPOSTERIZEUSEGREY",&t->posterizeUseGrey)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Steps are applied the same on red,green and blue");

    if (!t->posterizeUseGrey) {
      ImGui::Text("Posterize R");
      v = &t->posterizeRed; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPOSTERIZERED",v,0,0,2)) {if (*v<0) *v = 0; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Step size on red");
      ImGui::Text("Posterize G");
      v = &t->posterizeGreen; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPOSTERIZEGREEN",v,0,0,2)) {if (*v<0) *v = 0; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Step size on green");
      ImGui::Text("Posterize B");
      v = &t->posterizeBlue; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPOSTERIZEBLUE",v,0,0,2)) {if (*v<0) *v = 0; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Step size on blue");
      ImGui::Text("Posterize A");
      v = &t->posterizeAlpha; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPOSTERIZEALPHA",v,0,0,2)) {if (*v<0) *v = 0; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Step size on alpha");
      ImGui::Text("Base R");
      v = &t->posterizeRedBase; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPOSTERIZEREDBASE",v,0,0,2)) {if (*v<-255) *v = -255; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Step begin on red");
      ImGui::Text("Base G");
      v = &t->posterizeGreenBase; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPOSTERIZEGREENBASE",v,0,0,2)) {if (*v<-255) *v = -255; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Step begin on green");
      ImGui::Text("Base B");
      v = &t->posterizeBlueBase; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPOSTERIZEBLUEBASE",v,0,0,2)) {if (*v<-255) *v = -255; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Step begin on blue");
      ImGui::Text("Base A");
      v = &t->posterizeAlphaBase; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPOSTERIZEALPHABASE",v,0,0,2)) {if (*v<-255) *v = -255; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Step begin on alpha");
    } else {
      ImGui::Text("Posterize");
      v = &t->posterizeGrey; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPOSTERIZEGREY",v,0,0,2)) {if (*v<0) *v = 0; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Step size on red,green and blue, e.g. 64 for 4 steps");
      ImGui::Text("Base");
      v = &t->posterizeGreyBase; if (ImGui::InputFloat("##DRAWTOOLMAGICVIEWPOSTERIZEGREYBASE",v,0,0,2)) {if (*v<-255) *v = -255; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Step begin on red,green and blue");
    }
    ImGui::EndChild();
    ImGui::Separator();
  }

  ImGui::Text("Threshold"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWUSETHRESHOLD",&l->colorTransform.useThreshold)) {
    shouldUpdateAll = true;
  } else toolTip("Make all values above threshold white, the others black (or transparent)");
  if (l->colorTransform.useThreshold) {
    ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWUSEBLACKWHITE",&t->thresholdBlackWhite)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Use black/white instead of transparent/white");
    ImGui::BeginChild("DRAWTOOLMAGICCHILDTHRESHOLD",ImVec2(0,50));
    ImGui::Text("Threshold RGBA");
    v = &t->thresholdLuma; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWTHRESHOLDLUMA",v,0,255)) {if (*v<0) *v = 0; if (*v>255) *v = 255; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Threshold on red,green,blue (or maybe alpha)");
    ImGui::Text("Invert"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWINVERT",&t->thresholdInvert)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Threshold = 1 - threshold");
    ImGui::Text("Scale red");
    v = &t->thresholdRed; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWTHRESHOLDRED",v,0,1)) {if (*v<0) *v = 0; if (*v>1) *v = 1; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Red influence on threshold. Default is 1/3");
    ImGui::Text("Scale green");
    v = &t->thresholdGreen; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWTHRESHOLDGREEN",v,0,1)) {if (*v<0) *v = 0; if (*v>1) *v = 1; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Green influence on threshold. Default is 1/3");
    ImGui::Text("Scale blue");
    v = &t->thresholdBlue; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWTHRESHOLDBLUE",v,0,1)) {if (*v<0) *v = 0; if (*v>1) *v = 1; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Blue influence on threshold. Default is 1/3");
    ImGui::Text("Scale alpha");
    v = &t->thresholdAlpha; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWTHRESHOLDALPHA",v,0,1)) {if (*v<0) *v = 0; if (*v>1) *v = 1; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Alpha influence on threshold. Default is 0");
    ImGui::EndChild();
    ImGui::Separator();
  }

  ImGui::Text("Fast quant"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMAGICVIEWUSEQUANTIZATION",&l->colorTransform.useQuantization)) {
    shouldUpdateAll = true;
  } else toolTip("Use fast palette quantization (it's not a precise mapping)");
  if (l->colorTransform.useQuantization) {
    if (palette.size()<=256) {
      ImGui::Text("Dithering");
      v = &t->ditherStrength; if (ImGui::SliderFloat("##DRAWTOOLMAGICVIEWDITHERSTRENGTH",v,0,1)) {if (*v<0) *v = 0; if (*v>1) *v = 1; t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Apply this dither factor to the pixel luminances before quantizing");
      ImGui::Text("Palette shift");
      if (ImGui::InputInt("##DRAWTOOLMAGICVIEWPALETTESHIFT",&t->paletteShift,1,1)) {t->shouldUpdate = true; shouldUpdateAll = true;} else toolTip("Add or sub from the palette entry after quantizazion");
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
  ImGui::Text("CanvasX %.02f", cx0); toolTip("The mouse position on the picture in x");
  ImGui::Text("CanvasY %.02f", cy0); toolTip("The mouse position on the picture in y");
  if (currentCanvas->isOnlyActiveLayerActive()) {
    ImGui::Text("LayerX %.02f", lx0); toolTip("The mouse position on the current layer in x");
    ImGui::Text("LayerY %.02f", ly0); toolTip("The mouse position on the current layer in y");
  }
  ImGui::Text("");
  ImGui::Text("Start point");
  ImGui::Text("CanvasX %d", (int)floor(measureCX0)); toolTip("The start position of the current mouseclick on the picture in x");
  ImGui::Text("CanvasY %d", (int)floor(measureCY0)); toolTip("The start position of the current mouseclick on the picture in y");
  if (currentCanvas->isOnlyActiveLayerActive()) {
    ImGui::Text("LayerX %d", (int)floor(measureLX0)); toolTip("The start position of the current mouseclick on the current layer in x");
    ImGui::Text("LayerY %d", (int)floor(measureLY0)); toolTip("The start position of the current mouseclick on the current layer in y");
  }
  if (fabs(measureCX1-measureCX0)>0||fabs(measureCY1-measureCY0)>0) {
    ImGui::Text("");
    ImGui::Text("End point");
    ImGui::Text("CanvasX %d", (int)floor(measureCX1)); toolTip("The final position of the current mouseclick on the picture in x");
    ImGui::Text("CanvasY %d", (int)floor(measureCY1)); toolTip("The final position of the current mouseclick on the picture in y");
    if (currentCanvas->isOnlyActiveLayerActive()) {
      ImGui::Text("LayerX %d", (int)floor(measureLX1)); toolTip("The final position of the current mouseclick on the current layer in x");
      ImGui::Text("LayerY %d", (int)floor(measureLY1)); toolTip("The final position of the current mouseclick on the current layer in y");
    }
    ImGui::Text("");
    ImGui::Text("Pixels");
    int dx,dy;
    dx = (int)(floor(measureCX1)-floor(measureCX0));
    dy = (int)(floor(measureCY1)-floor(measureCY0));
    ImGui::Text("CanvasW %d", dx); toolTip("The width of the current mouseclick box on the picture");
    ImGui::Text("CanvasH %d", dy); toolTip("The height of the current mouseclick box on the picture");
    dx = (int)(floor(measureLX1)-floor(measureLX0));
    dy = (int)(floor(measureLY1)-floor(measureLY0));
    if (currentCanvas->isOnlyActiveLayerActive()) {
      ImGui::Text("LayerW %d", dx); toolTip("The width of the current mouseclick box on the current layer");
      ImGui::Text("LayerH %d", dy); toolTip("The height of the current mouseclick box on the current layer");
    }
    ImGui::Text("");
    ImGui::Text("Length");
    double lx,ly;
    lx = measureCX1-measureCX0;
    ly = measureCY1-measureCY0;
    ImGui::Text("Canvas %.02f",sqrt(lx*lx+ly*ly)); toolTip("Length of the diagonal of the current mouseclick box on the current picture");
    lx = measureLX1-measureLX0;
    ly = measureLY1-measureLY0;
    if (currentCanvas->isOnlyActiveLayerActive()) {
      ImGui::Text("Layer %.02f",sqrt(lx*lx+ly*ly)); toolTip("Length of the diagonal of the current mouseclick box on the current layer");
    }
    //lx = measureCX0; ly = measureCY0; currentCanvas->getScreenPos(lx,ly,lx,ly); ImVec2 l0(lx,ly);
    //lx = measureCX1; ly = measureCY1; currentCanvas->getScreenPos(lx,ly,lx,ly); ImVec2 l1(lx,ly);
    //ImGui::GetWindowDrawList()->AddLine(l0,l1,0xff0000ff,4);
  }

  ImGui::Text("");
  ImVec4 chere; 
  ImGui::Text("Color canvas"); toolTip("The color under the mouse on the picture");
  chere = getImVec4Color(currentCanvas->getCanvasColor(mouseX,mouseY)); colorItem("", "##DRAWTOOLMEASUREVIEWCOLORCANVAS", &chere);
  ImGui::Text("Color layer"); toolTip("The color under the mouse on the current layer (no postprocessing applied");
  chere = getImVec4Color(currentCanvas->getLayerColor(mouseX,mouseY,false)); colorItem("", "##DRAWTOOLMEASUREVIEWCOLORLAYER", &chere);
}

void updateTilingGrid(Layer *l, bool visible) {
  if (l == NULL) return;
  if (locked(l,LOCK_PIXELS)) return;
  l->updatePixelStates(~(PIXELSTATE_GRID|PIXELSTATE_TEMP),0,0);
  for (int y = 0; y < (int)l->height; y++) {
    for (int x = 0; x < (int)l->width; x++) {

      if (tilingOn && visible) {
        if ( (x == l->toolInfo.tileDrawX0) || (y == l->toolInfo.tileDrawY0) || (x == l->toolInfo.tileDrawX1) || (y == l->toolInfo.tileDrawY1) ) {
          l->pixelStates[x+y*l->width] |= PIXELSTATE_GRID;
        }
      }

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
                if ((unsigned int)xp < (unsigned int)l->width && (unsigned int)yp < (unsigned int)l->height) {
                  if (l->pixelStates[xp+yp*l->width] & PIXELSTATE_TEMP) continue;
                  unsigned int col = l->pixels[xp+yp*l->width];
                  for (int y4 = 0; y4 < th; y4++) {
                    for (int x4 = 0; x4 < tw; x4++) {
                      int xp = x + x4;
                      int yp = y + y4;
                      if ((unsigned int)xp < (unsigned int)l->width && (unsigned int)yp < (unsigned int)l->height) {
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
              if ((unsigned int)xp < (unsigned int)l->width && (unsigned int)yp < (unsigned int)l->height) l->pixelStates[xp+yp*l->width] |= PIXELSTATE_GRID;
              xp = x + x2;
              yp = y + th;
              if ((unsigned int)xp < (unsigned int)l->width && (unsigned int)yp < (unsigned int)l->height) l->pixelStates[xp+yp*l->width] |= PIXELSTATE_GRID;
            }
            for (int y2 = 0; y2 < th; y2++) {
              int xp = x;
              int yp = y + y2;
              if ((unsigned int)xp < (unsigned int)l->width && (unsigned int)yp < (unsigned int)l->height) l->pixelStates[xp+yp*l->width] |= PIXELSTATE_GRID;
              xp = x + tw;
              yp = y + y2;
              if ((unsigned int)xp < (unsigned int)l->width && (unsigned int)yp < (unsigned int)l->height) l->pixelStates[xp+yp*l->width] |= PIXELSTATE_GRID;
            }
          }
        }
      }
    }
  }
  shouldUpdateAll = true;
}

void updateTilingTexture() {
  if (tilingTexture != 0) {
    glBindTexture(GL_TEXTURE_2D,tilingTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, tilingTextureWidth, tilingTextureHeight, 0, GL_RGBA, GL_BYTE, &tilingTextureContent[0]);
    glBindTexture(GL_TEXTURE_2D,0);
  }
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
        if ((unsigned int)x3<(unsigned int)image.width&&(unsigned int)y3<(unsigned int)image.height) col = image.data[x3+y3*image.width];
      }
      tilingTextureContent[x+y*tilingTextureWidth] = gammaCorrect(col);
    }
  }
  updateTilingTexture();
}

void updateTilingImageAnim(const RGBAImage &image) {
  if (tilingTexture != 0) {
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
          if ((unsigned int)x3<(unsigned int)image.width&&(unsigned int)y3<(unsigned int)image.height) {
            col = image.data[x3+y3*image.width];
            int r = (col) & 255;
            int g = (col>>8) & 255;
            int b = (col>>16) & 255;
            int a = (col>>24) & 255;
            r = (r-0x80)*a/255+0x80;
            g = (g-0x80)*a/255+0x80;
            b = (b-0x80)*a/255+0x80;
            col = r|(g<<8)|(b<<16)|0xff000000;
          }
        }
        tilingTextureContentAnim[x+y*tilingTextureWidth] = gammaCorrect(col);
      }
    }
    glBindTexture(GL_TEXTURE_2D,tilingTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, tilingTextureWidth, tilingTextureHeight, 0, GL_RGBA, GL_BYTE, &tilingTextureContentAnim[0]);
    glBindTexture(GL_TEXTURE_2D,0);
  }
}

RGBAImage getImageFromLayer(Layer *l, int sx,int sy, int width, int height);
void putImageToLayer(Layer *l, const RGBAImage &c, int sx,int sy, bool useAlpha);

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

RGBAImage getTilingAnimRGBAImage(Layer *l,int frame) {
  RGBAImage ret;
  int cx,cy,outaX,outaY; 
  if (!getTilingAnimationFrameLayerCell(l,frame,cx,cy,outaX,outaY)) return ret;
  cx *= l->toolInfo.tilingWidth;
  cy *= l->toolInfo.tilingHeight;
  cx += outaX;
  cy += outaY;
  int tw = l->toolInfo.tilingWidth;
  int th = l->toolInfo.tilingHeight;
  ret = getImageFromLayer(l,cx,cy,tw,th);
  return ret;
}

void putTilingAnimRGBAImage(const RGBAImage &img, Layer *l,int frame) {
  int cx,cy,outaX,outaY; 
  if (!getTilingAnimationFrameLayerCell(l,frame,cx,cy,outaX,outaY)) return;
  cx *= l->toolInfo.tilingWidth;
  cy *= l->toolInfo.tilingHeight;
  cx += outaX;
  cy += outaY;
  int tw = l->toolInfo.tilingWidth;
  int th = l->toolInfo.tilingHeight;
  if (tw != img.width || th != img.height) return;
  putImageToLayer(l,img,cx,cy,false);
}

void blitTilingAnimFrame(Layer *l,int frame) {
  RGBAImage tile = getTilingAnimRGBAImage(l,frame);
  if (tile.data == NULL) return;
  updateTilingImageAnim(tile);
  tile.free();
}

void playTilingAnimation(Layer *l) {
  if (l == NULL) return;
  int f0 = l->toolInfo.tilingAnimFirstFrame;
  int fn = l->toolInfo.tilingAnimLastFrame;
  int s = fn-f0;
  if (s<=0||l->toolInfo.tilingAnimFPS<=0) {blitTilingAnimFrame(l,f0); return;}
  int s2 = s;
  bool doubleSided = l->toolInfo.tilingAnimDoubleSided;
  if (doubleSided) {s += s-1;}
  l->toolInfo.tilingAnimTime += timeDelta * l->toolInfo.tilingAnimFPS * (l->toolInfo.tilingAnimReverse ? -1 : 1); // actually the current frame
  if (l->toolInfo.tilingAnimTime<0) {l->toolInfo.tilingAnimTime += s * floor(-l->toolInfo.tilingAnimTime/s+1);}
  int tilingFrame = floor(l->toolInfo.tilingAnimTime);
  if (tilingFrame<0) {blitTilingAnimFrame(l,f0); return;}
  tilingFrame %= (s+1);
  if (doubleSided && tilingFrame >= s2) {
    tilingFrame = s2 - (tilingFrame - s2);
  }
  tilingFrame += l->toolInfo.tilingAnimFirstFrame;
  l->toolInfo.tilingAnimFrame = tilingFrame;
  blitTilingAnimFrame(l,tilingFrame);
}

void drawToolTilingView() {
  if (wasActiveLayerSwitched()) {
    for (int i = 0; i < (int)currentCanvas->layers.size(); i++) updateTilingGrid(currentCanvas->layers[i],false);
    Layer *l = currentCanvas->getActiveLayer(); if (l != NULL) updateTilingGrid(l,true);
  }

  Layer *l = currentCanvas->getActiveLayer();
  if (l != NULL) {
    ImGui::PushItemWidth(7*5);
    double lx = mouseX,ly = mouseY; currentCanvas->getLayerPos(l,lx,ly,lx,ly);
    int x = (int)floor((lx - l->toolInfo.tilingOffsetX) / (double)l->toolInfo.tilingWidth);
    int y = (int)floor((ly - l->toolInfo.tilingOffsetY) / (double)l->toolInfo.tilingHeight);
    if (controlKeyId == CONTROLKEY_SHIFT) 
      ImGui::Text("Select cell");
    else
    if (controlKeyId == CONTROLKEY_CTRL) 
      ImGui::Text("Alpha paste");
    else
      ImGui::Text("");
    ImGui::Text("[%d] [%d]",x,y);
    ImGui::Text("Show grid"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLTILINGVIEWSHOWGRID",&l->toolInfo.showTilingGrid)) {updateTilingGrid(l,true);} else toolTip("Display the grid in the layer");
    ImGui::Separator();
    ImGui::Text("Tiled drawing"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLTILINGVIEWTILINGON",&tilingOn)) {updateTilingGrid(l,true);} else toolTip("Edit/Paint in tiling mode (select tile by clicking)");
    ImGui::Separator();
    ImGui::Text("Cells");
    ImGui::Text("Offset X"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLTILINGVIEWOFSX",&l->toolInfo.tilingOffsetX,0,0)) {updateTilingGrid(l,true);} else toolTip("Grid displacement in pixels in x");
    ImGui::Text("Offset Y"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLTILINGVIEWOFSY",&l->toolInfo.tilingOffsetY,0,0)) {updateTilingGrid(l,true);} else toolTip("Grid displacement in pixels in y");
    ImGui::Text("Width"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLTILINGVIEWGRIDWIDTH",&l->toolInfo.tilingWidth,0,0)) {updateTilingGrid(l,true);} else toolTip("Grid cell size in pixels in x");
    ImGui::Text("Height"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLTILINGVIEWGRIDHEIGHT",&l->toolInfo.tilingHeight,0,0)) {updateTilingGrid(l,true);} else toolTip("Grid cell size in pixels in y");
    if (l->toolInfo.showTilingGrid) {
      ImGui::Separator();
      ImGui::Text("Tile colors");
      ImGui::Text("count >=");ImGui::SameLine();if (ImGui::InputInt("##DRAWTOOLTILINGVIEWCOLORCOUNT",&l->toolInfo.tilingMinColorCount,0,0)) {updateTilingGrid(l,true);} else toolTip("Maximum count of different colors in a cell within all cells (where the cell gets a grid outline)");
    }
    if (l->toolInfo.tilingWidth < 1) l->toolInfo.tilingWidth = 1;
    if (l->toolInfo.tilingHeight < 1) l->toolInfo.tilingHeight = 1;
    if (l->toolInfo.tilingWidth > 1024) l->toolInfo.tilingWidth = 1024;
    if (l->toolInfo.tilingHeight > 1024) l->toolInfo.tilingHeight = 1024;
    ImGui::PopItemWidth();
    ImGui::Separator();
    ImGui::Image((ImTextureID)tilingTexture, ImVec2(tilingTextureWidth/(aspectRatio*main_window_yres/main_window_xres)/tilingTextureMul,tilingTextureHeight/tilingTextureMul));
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
    } else toolTip("Play a simple tile/gridcell animation here");
    if (l->toolInfo.tilingAnimPlaying) {
      ImGui::Text("Index %d",l->toolInfo.tilingAnimFrame); toolTip("The current tile index in the animation");
    }
    ImGui::Text("Show indices"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLTILINGANIMATIONSHOWINDICES",&l->toolInfo.showTilingGridIndices)) {updateTilingGrid(l,true);} else toolTip("Show tile index under the mouse");
    ImGui::Text("FPS"); ImGui::SameLine(); if (ImGui::InputFloat("##DRAWTOOLTILINGANIMATIONFPS",&l->toolInfo.tilingAnimFPS,0,0,2)) {} else toolTip("How fast is the animation"); if (l->toolInfo.tilingAnimFPS<0) l->toolInfo.tilingAnimFPS=0;
    ImGui::Text("Index0"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLTILINGANIMATIONFIRSTFRAME",&l->toolInfo.tilingAnimFirstFrame,0,0)) {l->toolInfo.tilingAnimPlaying=false;} else toolTip("The first frame of the animation"); if (l->toolInfo.tilingAnimFirstFrame<0) l->toolInfo.tilingAnimFirstFrame=0;
    ImGui::Text("IndexN"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLTILINGANIMATIONLASTFRAME",&l->toolInfo.tilingAnimLastFrame,0,0)) {l->toolInfo.tilingAnimPlaying=false;} else toolTip("The last frame of the animation"); if (l->toolInfo.tilingAnimLastFrame<0) l->toolInfo.tilingAnimLastFrame=0;
    ImGui::Text("Reverse"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLTILINGANIMATIONREVERSE",&l->toolInfo.tilingAnimReverse)) {} else toolTip("Play animation backwards");
    ImGui::Text("Double sided"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLTILINGANIMATIONDOUBLESIDED",&l->toolInfo.tilingAnimDoubleSided)) {} else toolTip("Play animation forward+backwards");
    ImGui::PopItemWidth();
    if (l->toolInfo.tilingAnimPlaying) playTilingAnimation(l);
    ImGui::Separator();
    if (roundButton("Save anim",l->toolInfo.tilingAnimLastFrame>=l->toolInfo.tilingAnimFirstFrame)) {dialogLoadSave(true, DIALOG_SAVETILINGANIM);} else toolTip("Save animation into a continuous .png (with header)");
    if (roundButton("Load anim",true)) {dialogLoadSave(true, DIALOG_LOADTILINGANIM);} else toolTip("Load animation from a .png (with header)");
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
  if (roundButton("Add font",true)) {dialogLoadSave(true,DIALOG_ADDFONT);} else toolTip("Load a .ttf font");
  if (roundButton("Save font",l->toolInfo.fontId != 0)) {saveFontId = l->toolInfo.fontId; dialogLoadSave(true,DIALOG_SAVEFONT);} else toolTip("Save the selected .ttf font");
  ImGui::Text("Fonts [%d]",fonts.size()); toolTip("Currently active font count");
  ImGui::BeginChild("DRAWTOOLTEXTVIEWFONTS",ImVec2(toolview_width,200));
  String fileName = "";
  for (int i = 0; i < (int)fonts.size(); i++) {
    FontSource *f = fonts[i];
    if (f->glHandle == 0) f->glHandle = f->createTexture(getName(f->fileName),20,aspectRatio*main_window_yres/main_window_xres,f->glWidth,f->glHeight,0x00000000);
    ImVec2 minB = ImGui::GetCursorScreenPos();
    ImGui::Image((ImTextureID)f->glHandle,ImVec2(f->glWidth,f->glHeight)); toolTip("A .ttf font");
    ImVec2 maxB = ImGui::GetItemRectMax();
    ImGui::SetCursorScreenPos(minB);
    maxB.x = minB.x + toolview_width;
    if (ImGui::InvisibleButton(combined(String("##DRAWTOOLTEXTUREVIEWLINE")+String::fromInt(i)).c_str(),ImVec2(maxB.x-minB.x,maxB.y-minB.y))) {l->toolInfo.fontId = f->id;}
    if (f->id == l->toolInfo.fontId) {fileName = f->fileName; ImGui::GetWindowDrawList()->AddRectFilled(minB,maxB,0x40ffff00);}
  }
  ImGui::EndChild();
  if (!fileName.empty()) ImGui::Text("%s",(fileName.last(16)).c_str()); 
  LayerAdditionalToolInfo *ati = &l->toolInfo;
  toolTip("The .ttf font filename");
  if (roundButton("Remove font",l->toolInfo.fontId != 0)) {removeFont(l->toolInfo.fontId);l->toolInfo.fontId = 0;} else toolTip("Remove the selected font");
  if (roundButton("Font glyphs##DRAWTOOLTEXTUREVIEWSHOWFONTGLYPHS",ati->fontId != 0)) {dialogFontGlyphs(true, fileName.c_str());} else toolTip("Show the glyphs to enter special chars");
  ImGui::PushItemWidth(7*5);
  ImGui::Text("Font"); 
  ImGui::Text("Height"); ImGui::SameLine(); if (ImGui::InputFloat("##DRAWTOOLTEXTUREVIEWFONTSIZE",&ati->fontSize,0,0,2)) {} else toolTip("The selected font size in pixels in y"); if (ati->fontSize < 1) ati->fontSize = 1;
  ImVec2 cursor = ImGui::GetCursorScreenPos();
  ImVec4 chere = getImVec4Color(ati->fontColor); colorItem("", "##DRAWTOOLTEXTUREVIEWFONTCOLOR", &chere);
  ImGui::SetCursorScreenPos(cursor);
  if (ImGui::InvisibleButton("##DRAWTOOLTEXTUREVIEWFONTCOLORINVISIBLE",ImVec2(16/aspectRatio*main_window_xres/main_window_yres,16))) {
    ati->fontColor = getUIntColor(foregroundColor);
  } else toolTip("The color the text is to be painted with");
  ImGui::Text("Scale x"); ImGui::SameLine(); if (ImGui::InputFloat("##DRAWTOOLTEXTUREVIEWFONTSCALEX",&ati->fontScaleX,0,0,2)) {} else toolTip("Scaling of the text in x"); if (ati->fontScaleX < 0) ati->fontScaleX = 0;
  ImGui::Text("Scale y"); ImGui::SameLine(); if (ImGui::InputFloat("##DRAWTOOLTEXTUREVIEWFONTSCALEY",&ati->fontScaleY,0,0,2)) {} else toolTip("Scaling of the text in y");; if (ati->fontScaleY < 0) ati->fontScaleY = 0;
  ImGui::Text("Space x"); ImGui::SameLine(); if (ImGui::InputFloat("##DRAWTOOLTEXTUREVIEWFONTSPACEX",&ati->fontSpaceX,0,0,2)) {} else toolTip("Letter spacing of the text in x");
  ImGui::Text("Space y"); ImGui::SameLine(); if (ImGui::InputFloat("##DRAWTOOLTEXTUREVIEWFONTSPACEY",&ati->fontSpaceY,0,0,2)) {} else toolTip("Letter spacing of the text in y");
  ImGui::Text("");
  if (ImGui::RadioButton("Align left##DRAWTOOLTEXTUREVIEWALIGNLEFT",&ati->fontAlignment,ALIGN_LEFT)) {} else toolTip("Text alignment in x to the left");
  if (ImGui::RadioButton("Align right##DRAWTOOLTEXTUREVIEWALIGNRIGHT",&ati->fontAlignment,ALIGN_RIGHT)) {} else toolTip("Text alignment in x centered");
  if (ImGui::RadioButton("Align center##DRAWTOOLTEXTUREVIEWALIGNCENTER",&ati->fontAlignment,ALIGN_CENTER)) {} else toolTip("Text alignment in x to the right");
  ImGui::PopItemWidth();
  ImGui::Text("Text"); ImGui::SameLine(); if (roundButton("Edit##DRAWTOOLTEXTUREVIEWEDITTEXT",true)) {textToSave = ati->textEditText; textToSaveLength = MAXTEXTEDITTEXT; dialogEditText(true);}
  ImGui::PushItemWidth(toolview_width-10);
  ImGui::InputTextMultiline("##DRAWTOOLTEXTUREVIEWALIGNTEXT", ati->textEditText, MAXTEXTEDITTEXT); toolTip("The text to be painted");
  ImGui::PopItemWidth();
  if (roundButton("Bake into layer##DRAWTOOLTEXTUREVIEWBAKE",ati->fontId != 0)) {paintTextIntoLayer(l);} else toolTip("Now paint this text into the layer");
  if (roundButton("Copy from file##DRAWTOOLTEXTUREVIEWTEXTCOPYPASTE",true)) {textToSave = ati->textEditText; textToSaveLength = MAXTEXTEDITTEXT; dialogLoadSave(true,DIALOG_LOADTEXT);} else toolTip("Copy/paste some text from a text file");
  if (roundButton("Save text##DRAWTOOLTEXTUREVIEWTEXTSAVETEXT",true)) {textToSave = ati->textEditText; dialogLoadSave(true,DIALOG_SAVETEXT);} else toolTip("Save this text to a text file");
  if (roundButton("Load UTF8##DRAWTOOLTEXTUREVIEWTEXTLOADUTF8",true)) {textToSave = ati->textEditText; textToSaveLength = MAXTEXTEDITTEXT; dialogLoadSave(true,DIALOG_LOADTEXTUTF8);} else toolTip("Load an UTF8 based text");
}

void drawToolOverlaysView() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  bool hasSel = l->hasSelection();
  if (roundButton("Add overlay##DRAWTOOLOVERLAYSVIEWADDOVERLAY",hasSel)) {
    int lx0=0,ly0=0,lx1=0,ly1=0; bool first = true; for (int y = 0; y < l->height; y++) {for (int x = 0; x < l->width; x++) {if (l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED) {if (first || x < lx0) lx0 = x; if (first || y < ly0) ly0 = y; if (first || x > lx1) lx1 = x; if (first || y > ly1) ly1 = y; first = false; } } }
    lx1++;
    ly1++;
    Overlay *o = new Overlay();
    o->type = OVERLAYTYPE_NORMAL; o->layerId = l->id;
    o->lx0 = lx0;
    o->ly0 = ly0;
    o->lx1 = lx1;
    o->ly1 = ly1;
    o->cx0 = 0;
    o->cy0 = 0;
    o->cx1 = lx1-lx0;
    o->cy1 = ly1-ly0;
    currentCanvas->overlays.insert(o,0); shouldUpdateAll = true;
  } toolTip("Add a screen overlay from selection");
  if (roundButton("Add annotation##DRAWTOOLOVERLAYSVIEWADDANNOTATION",hasSel)) {
    int lx0=0,ly0=0,lx1=0,ly1=0; bool first = true; for (int y = 0; y < l->height; y++) {for (int x = 0; x < l->width; x++) {if (l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED) {if (first || x < lx0) lx0 = x; if (first || y < ly0) ly0 = y; if (first || x > lx1) lx1 = x; if (first || y > ly1) ly1 = y; first = false; } } }
    lx1++;
    ly1++;
    Overlay *o = new Overlay();
    o->type = OVERLAYTYPE_ANNOTATION; o->layerId = l->id;
    o->lx0 = lx0;
    o->ly0 = ly0;
    currentCanvas->overlays.insert(o,0); shouldUpdateAll = true;
  } else toolTip("Add a text as annotation");
  if (roundButton("Add hor. line##DRAWTOOLOVERLAYSVIEWADDXLINE",hasSel)) {
    int lx0=0,ly0=0,lx1=0,ly1=0; bool first = true; for (int y = 0; y < l->height; y++) {for (int x = 0; x < l->width; x++) {if (l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED) {if (first || x < lx0) lx0 = x; if (first || y < ly0) ly0 = y; if (first || x > lx1) lx1 = x; if (first || y > ly1) ly1 = y; first = false; } } }
    lx1++;
    ly1++;
    Overlay *o = new Overlay();
    o->type = OVERLAYTYPE_SEPERATINGLINEX; o->layerId = l->id;
    o->lx0 = lx0;
    o->ly0 = ly0;
    currentCanvas->overlays.insert(o,0); shouldUpdateAll = true;
  } toolTip("Add a whole canvas line on y");
  if (roundButton("Add vert. line##DRAWTOOLOVERLAYSVIEWADDYLINE",hasSel)) {
    int lx0=0,ly0=0,lx1=0,ly1=0; bool first = true; for (int y = 0; y < l->height; y++) {for (int x = 0; x < l->width; x++) {if (l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED) {if (first || x < lx0) lx0 = x; if (first || y < ly0) ly0 = y; if (first || x > lx1) lx1 = x; if (first || y > ly1) ly1 = y; first = false; } } }
    lx1++;
    ly1++;
    Overlay *o = new Overlay();
    o->type = OVERLAYTYPE_SEPERATINGLINEY; o->layerId = l->id;
    o->lx0 = lx0;
    o->ly0 = ly0;
    currentCanvas->overlays.insert(o,0); shouldUpdateAll = true;
  } else toolTip("Add a whole canvas line on x");
  if (roundButton("Add sprite##DRAWTOOLOVERLAYSVIEWADDSPRITE",hasSel)) {
    int lx0=0,ly0=0,lx1=0,ly1=0; bool first = true; for (int y = 0; y < l->height; y++) {for (int x = 0; x < l->width; x++) {if (l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED) {if (first || x < lx0) lx0 = x; if (first || y < ly0) ly0 = y; if (first || x > lx1) lx1 = x; if (first || y > ly1) ly1 = y; first = false; } } }
    lx1++;
    ly1++;
    Overlay *o = new Overlay();
    o->type = OVERLAYTYPE_CANVASSPRITE; o->layerId = l->id;
    o->lx0 = lx0;
    o->ly0 = ly0;
    o->lx1 = lx1;
    o->ly1 = ly1;
    o->cx0 = 0;
    o->cy0 = 0;
    currentCanvas->overlays.insert(o,0); shouldUpdateAll = true;
  } else toolTip("Add a sprite onto the picture");
  ImGui::Text("Overlays"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLOVERLAYSVIEWSHOWOVERLAYS",&currentCanvas->showOverlays)) {shouldUpdateAll = true;} else toolTip("Display all the screen overlays");
  ImGui::Text("Annotations"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLOVERLAYSVIEWSHOWANNOTATIONS",&currentCanvas->showAnnotations)) {shouldUpdateAll = true;} else toolTip("Display all the annotation texts");
  ImGui::Text("Lines"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLOVERLAYSVIEWSHOWLINES",&currentCanvas->showLines)) {shouldUpdateAll = true;} else toolTip("Display all the seperating lines");
  ImGui::Text("Sprites"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLOVERLAYSVIEWSHOWSPRITES",&currentCanvas->showSprites)) {shouldUpdateAll = true;} else toolTip("Display all the sprites");
  ImGui::BeginChild("DRAWTOOLOVERLAYVIEWCHILD",ImVec2(toolview_width,main_window_yres-ImGui::GetCursorScreenPos().y-20));
  static int selectedOverlay = -1;
  static bool scrollToSelectedOverlay = false;
  for (int i = 0; i < (int)currentCanvas->overlays.size(); i++) {
    Overlay *o = currentCanvas->overlays[i];
    Layer *l = NULL;
    for (int i2 = 0; i2 < (int)currentCanvas->layers.size(); i2++) {
      if (currentCanvas->layers[i2]->id == o->layerId) {
        l = currentCanvas->layers[i2];
        break;
      }
    }
    if (l != NULL) {
      ImVec2 cursor2 = ImGui::GetCursorScreenPos();
      if (scrollToSelectedOverlay && selectedOverlay == i) {scrollToSelectedOverlay = false; ImGui::SetScrollPosHere();}
      ImGui::Text("%s",l->name); toolTip("The source layer name for this overlay");
      ImGui::Text("Visible"); ImGui::SameLine(); if (ImGui::Checkbox(combined(String("##DRAWTOOLOVERLAYSVIEWVISIBLE")+String::fromInt(i)).c_str(),&o->visible)) {shouldUpdateAll = true;} else toolTip("Is this overlay visible");
      switch(o->type) {
        case OVERLAYTYPE_NORMAL: {
          ImGui::Text("W %d H %d",o->lx1-o->lx0,o->ly1-o->ly0); toolTip("Original width and height");
          int x = o->cx0;
          int y = o->cy0;
          int w = o->cx1 - o->cx0;
          int h = o->cy1 - o->cy0;
          ImGui::PushItemWidth(7*5);
          ImGui::Text("Pos X"); ImGui::SameLine(); if (ImGui::InputInt(combined(String("##DRAWTOOLOVERLAYSVIEWCX0")+String::fromInt(i)).c_str(),&x,0,0)) {shouldUpdateAll = true;} else toolTip("Modify overlays screen x position");
          ImGui::Text("Pos Y"); ImGui::SameLine(); if (ImGui::InputInt(combined(String("##DRAWTOOLOVERLAYSVIEWCY0")+String::fromInt(i)).c_str(),&y,0,0)) {shouldUpdateAll = true;} else toolTip("Modify overlays screen y position");
          ImGui::Text("Width"); ImGui::SameLine(); if (ImGui::InputInt(combined(String("##DRAWTOOLOVERLAYSVIEWCX1")+String::fromInt(i)).c_str(),&w,0,0)) {shouldUpdateAll = true;} else toolTip("Modify overlays screen width");
          ImGui::Text("Height"); ImGui::SameLine(); if (ImGui::InputInt(combined(String("##DRAWTOOLOVERLAYSVIEWCY1")+String::fromInt(i)).c_str(),&h,0,0)) {shouldUpdateAll = true;} else toolTip("Modify overlays screen height");
          ImGui::PopItemWidth();
          o->cx0 = x;
          o->cy0 = y;
          o->cx1 = x+w;
          o->cy1 = y+h;
        } break;
        case OVERLAYTYPE_ANNOTATION: {
          if (ImGui::InputText(combined(String("##DRAWTOOLOVERLAYSVIEWNAMETEXT")+String::fromInt(i)).c_str(),o->annotationText,ANNOTATIONTEXTMAX)) {shouldUpdateAll = true;} else toolTip("Annotation text");
          ImVec2 cursor = ImGui::GetCursorScreenPos();
          ImVec4 chere = getImVec4Color(o->annotationColor); if (colorItem("", "##DRAWTOOLOVERLAYANNOTATIONCOLOR", &chere)) {o->annotationColor = getUIntColor(chere);shouldUpdateAll = true;}
          ImGui::SetCursorScreenPos(cursor);
          if (ImGui::InvisibleButton("##DRAWTOOLOVERLAYVIEWANNOTATIONCOLORINVISIBLE",ImVec2(16/aspectRatio*main_window_xres/main_window_yres,16))) {
            o->annotationColor = getUIntColor(foregroundColor);
            shouldUpdateAll = true;
          } else toolTip("Annotation color");
          ImGui::Text("Font size"); ImGui::SameLine(); if (ImGui::InputFloat(combined(String("##DRAWTOOLOVERLAYSVIEWANNOTATIONFONTSIZE")+String::fromInt(i)).c_str(),&o->annotationFontSize,0,0,2)) {shouldUpdateAll = true;} else toolTip("Annotation font size in pixels"); if (o->annotationFontSize < 1) o->annotationFontSize = 1;
        } break;
        case OVERLAYTYPE_SEPERATINGLINEX: {
          ImGui::Text("Horizontal %d",o->ly0); toolTip("Seperating line y position");
        } break;
        case OVERLAYTYPE_SEPERATINGLINEY: {
          ImGui::Text("Vertical %d",o->lx0); toolTip("Seperating line x position");
        } break;
        case OVERLAYTYPE_CANVASSPRITE: {
          ImGui::Text("W %d H %d",o->lx1-o->lx0,o->ly1-o->ly0); toolTip("Original width and height");
          int x = o->cx0;
          int y = o->cy0;
          ImGui::PushItemWidth(7*5);
          ImGui::Text("Pos X"); ImGui::SameLine(); if (ImGui::InputInt(combined(String("##DRAWTOOLOVERLAYSVIEWSCX0")+String::fromInt(i)).c_str(),&x,0,0)) {shouldUpdateAll = true;} else toolTip("Sprites picture position in x");
          ImGui::Text("Pos Y"); ImGui::SameLine(); if (ImGui::InputInt(combined(String("##DRAWTOOLOVERLAYSVIEWSCY0")+String::fromInt(i)).c_str(),&y,0,0)) {shouldUpdateAll = true;} else toolTip("Sprites picture position in y");
          ImGui::PopItemWidth();
          o->cx0 = x;
          o->cy0 = y;
        } break;
      }
      ImVec2 cursor3 = ImGui::GetCursorScreenPos();
      cursor3.x = cursor2.x + toolview_width;
      if (ImGui::IsMouseDown(0) && ImGui::IsMouseHoveringRect(cursor2,cursor3)) selectedOverlay = i;
      if (selectedOverlay == i) ImGui::GetWindowDrawList()->AddRectFilled(cursor2,cursor3,0x40c0c000);
    }
    if (roundButton(combined(String("Remove##DRAWTOOLOVERLAYSVIEWREMOVEOVERLAY")+String::fromInt(i)).c_str(),true)) {
      shouldUpdateAll = true;
      delete o; currentCanvas->overlays.erase(i,1);
      i--;
    } else toolTip("Remove this overlay");
    ImGui::Separator();
  }
  ImGui::EndChild();

  if ((unsigned int)selectedOverlay < currentCanvas->overlays.size()) {
    if (roundButton("Up##DRAWTOOLOVERLAYVIEWMOVEUP",selectedOverlay>0)) {
      Overlay *o = currentCanvas->overlays[selectedOverlay];
      currentCanvas->overlays.erase(selectedOverlay,1);
      selectedOverlay--;
      scrollToSelectedOverlay = true;
      currentCanvas->overlays.insert(o,selectedOverlay);
    } else toolTip("Move selected overlay up in hierarchy");
    ImGui::SameLine();
    if (roundButton("Down##DRAWTOOLOVERLAYVIEWMOVEDOWN",selectedOverlay<(int)currentCanvas->overlays.size()-1)) {
      Overlay *o = currentCanvas->overlays[selectedOverlay];
      currentCanvas->overlays.erase(selectedOverlay,1);
      selectedOverlay++;
      scrollToSelectedOverlay = true;
      currentCanvas->overlays.insert(o,selectedOverlay);
    } else toolTip("Move selected overlay down in hierarchy");
  }
}

Array<Vector> modifyPointsSourceSaved;
Array<Vector> modifyPointsDestSaved;

void drawToolModifyView() {
  bool modifyModeAvailable = modifyMode;
  if (modifyPointsSource.size()>=3) modifyModeAvailable = true;
  if (roundButton(modifyMode?"Base mode##DRAWTOOLMODIFYMODE":"Modify mode##DRAWTOOLMODIFYMODE",modifyModeAvailable)) {
    modifyMode = !modifyMode;
    if (modifyMode) {
      modifyPointsDest.clear();
      for (int i = 0; i < (int)modifyPointsSource.size(); i++) modifyPointsDest.push_back(modifyPointsSource[i]);
    }
  } else toolTip("Switch between source and destination polygon/shape");
  bool hasWarp = currentCanvas->getActiveLayer()!=NULL;
  if (roundButton("Bake into##DRAWTOOLMODIFYBAKE",hasWarp)) {
    bakeModify(false);
  } else toolTip("Apply the warp to the current layer");
  if (roundButton("Make selection##DRAWTOOLMODIFYCREATESELECTION",hasWarp)) {
    bakeModify(true);
  } else toolTip("Select the background of the current warp");
  int k;
  ImGui::Separator();
  ImGui::Text("Pop out"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMODIFYBG",&modifyClearBackground)) {} else toolTip("Clear background behind warp");
  if (!modifyPerspective) {
    ImGui::Text("Outline mode"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMODIFYOUTLINED",&modifyOutlined)) {} else toolTip("Smooth warp using only the outlines");
  }
  if ((!modifyOutlined) && modifyPointsSource.size()==4) {
    ImGui::Text("Persp. mode"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMODIFYPERSPECTIVE",&modifyPerspective)) {} else toolTip("Warp using 3D perspective (only available for 4 points) e.g. to rectify documents");
  }
  ImGui::Text("Filtered"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMODIFYFILTERED",&modifyFiltered)) {} else toolTip("Smoothen the colors when baking");
  ImGui::Separator();
  if (roundButton("Store##DRAWTOOLMODIFYSTORE",modifyPointsSource.size()==modifyPointsDest.size()&&modifyPointsSource.size()!=0&&modifyMode)) {
    modifyPointsSourceSaved = modifyPointsSource;
    modifyPointsDestSaved = modifyPointsDest;
  } else toolTip("Rember the current shape");
  ImGui::SameLine();
  if (roundButton("Load##DRAWTOOLMODIFYLOAD",modifyPointsSourceSaved.size()!=0)) {
    modifyPointsSource = modifyPointsSourceSaved;
    modifyPointsDest = modifyPointsDestSaved;
    createModifyTriangles();
    modifyMode = true;
  } else toolTip("Apply the remembered shape again");
  ImGui::Text("Point count %d",modifyPointsSource.size()); toolTip("Shape point count");
  ImGui::Separator();
  if ((unsigned int)selectedModifyPoint<modifyPointsSource.size()) {
    ImGui::Text("Point %d",selectedModifyPoint); toolTip("Point number in shape");
    ImGui::PushItemWidth(7*5);
    if (!modifyMode) {
      k = modifyPointsSource[selectedModifyPoint].x;ImGui::Text("Source x"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLMODIFYPOINTSX",&k,0,0)) {modifyPointsSource[selectedModifyPoint].x = k;} else toolTip("Modify the shape point in x");
      k = modifyPointsSource[selectedModifyPoint].y;ImGui::Text("Source y"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLMODIFYPOINTSY",&k,0,0)) {modifyPointsSource[selectedModifyPoint].y = k;} else toolTip("Modify the shape point in y");
      if (roundButton("Remove point##DRAWTOOLMODIFYREMOVE",true)) {
        modifyPointsSource.erase(selectedModifyPoint,1);
        if ((unsigned int)selectedModifyPoint<modifyPointsDest.size()) modifyPointsDest.erase(selectedModifyPoint,1);
        if (selectedModifyPoint>=(int)modifyPointsSource.size()) selectedModifyPoint=modifyPointsSource.size()-1;
        createModifyTriangles();
      } else toolTip("Remove this point again");
    } else {
      k = modifyPointsDest[selectedModifyPoint].x;ImGui::Text("Dest x"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLMODIFYPOINTDX",&k,0,0)) {modifyPointsDest[selectedModifyPoint].x = k;} else toolTip("Modify the shape point in x");
      k = modifyPointsDest[selectedModifyPoint].y;ImGui::Text("Dest y"); ImGui::SameLine(); if (ImGui::InputInt("##DRAWTOOLMODIFYPOINTDY",&k,0,0)) {modifyPointsDest[selectedModifyPoint].y = k;} else toolTip("Modify the shape point in y");
    }
    ImGui::PopItemWidth();
    ImGui::Separator();
  }
  if (roundButton("Reset##DRAWTOOLMODIFYRESET",true)) {
    modifyPointsSource.clear();
    modifyPointsDest.clear();
    selectedModifyPoint = -1;
  } else toolTip("Erase the current warp setting");
}

int scrollToTileMapIndex = -1;
bool excludeEmptyTiles = false;

void drawToolTileMapView() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (l->type != LAYERTYPE_TILEMAP) return;
  Layer *l2 = currentCanvas->getLayerById(l->tileSourceLayerId);
  if (l2 != NULL && l2->width > 0 && l2->height > 0 && l2->toolInfo.tilingWidth > 0 && l2->toolInfo.tilingHeight > 0) {
    static unsigned int lastTextureUpdateCount = (unsigned int)((int)-1);
    if (lastTextureUpdateCount != l2->undoData.updatePixelsCount) {
      lastTextureUpdateCount = l2->undoData.updatePixelsCount;
      glBindTexture(GL_TEXTURE_2D,tileMapTexture);
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, l2->width, l2->height, 0, GL_RGBA, GL_BYTE, l2->pixels);
      glBindTexture(GL_TEXTURE_2D,0);
    }
    ImGui::Text("\"%s\"",l2->name);
    if (roundButton("Scroll to##DRAWTOOLTILEMAPVIEWSCROLLTO",true)) {
      scrollToTileMapIndex = l->toolInfo.tileMapSelectedTile;
    }
    if (roundButton("Save tilemap##DRAWTOOLTILEMAPVIEWSAVETILEMAP",true)) {
      dialogLoadSave(true, DIALOG_SAVETILEMAP);
    }
    if (roundButton("Load tilemap##DRAWTOOLTILEMAPVIEWLOADTILEMAP",true)) {
      dialogLoadSave(true, DIALOG_LOADTILEMAP);
    }
    ImGui::Text("Exclude empty"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLTILEMAPVIEWEXCLUDEEMPTY",&excludeEmptyTiles)) {scrollToTileMapIndex = l->toolInfo.tileMapSelectedTile;}

    int tileWidth = l2->toolInfo.tilingWidth;
    int tileHeight = l2->toolInfo.tilingHeight;
    //int tileOffsetX = l2->toolInfo.tilingOffsetX;
    //int tileOffsetY = l2->toolInfo.tilingOffsetY;
    int width = toolview_width; int height = 600;
    ImVec2 cursor0 = ImGui::GetCursorScreenPos();
    ImGui::BeginChild("DRAWTOOLTILEMAPVIEWCHILD", ImVec2(width,height));
    int border = 2;
    int tw = 16 + border;
    double ratio = (double)tileHeight/tileWidth;
    if (ratio < 0.5) ratio = 0.5;
    if (ratio > 3) ratio = 3;
    int th = 16*aspectRatio*main_window_yres/main_window_xres*ratio + border;

    static Array<int> tileIndices; tileIndices.clear(); // a first loop to collect all tiles
    int index = 0;
    while(true) {
      int outaX = 0;
      int outaY = 0;
      int tileX = 0;
      int tileY = 0;
      if (!getTilingAnimationFrameLayerCell(l2, index, tileX, tileY, outaX, outaY)) break;
      ImVec2 tx0,tx1; tx0.x = (double)(tileX*tileWidth+outaX)/l2->width; tx0.y = (double)(tileY*tileHeight+outaY)/l2->height;
      if (tx0.y >= 1) break;
      bool excluded = false;
      if (excludeEmptyTiles) {
        int sx = tileX*tileWidth+outaX;
        int sy = tileY*tileHeight+outaY;
        bool found = false;
        for (int y = 0; y < tileHeight; y++) {
          for (int x = 0; x < tileWidth; x++) {
            int tx = x + sx;
            int ty = y + sy;
            if ((unsigned int)tx<(unsigned int)l2->width && (unsigned int)ty<(unsigned int)l2->height) {
              unsigned int col = l2->pixels[tx+ty*l2->width];
              int a = (col >> 24) & 255;
              if (a != 0) {
                found = true;
                break;
              }
            }
          }
          if (found) break;
        }
        excluded = !found;
      }
      if (!excluded) tileIndices.push_back(index);
      index++;
    }

    index = 0;
    int k = (width - 16)/tw;
    if (k != 0) {
      if (scrollToTileMapIndex >= 0) {
        for (int i = 0; i < (int)tileIndices.size(); i++) {
          if (tileIndices[i] == scrollToTileMapIndex) {
            ImGui::SetScrollY((i / k-5)*th);
            break;
          }
        }
      }
      int row = ImGui::GetScrollY() / th;
      index = row * k;
      ImVec2 c = ImGui::GetCursorScreenPos(); c.y += row * th; ImGui::SetCursorScreenPos(c);
    }
    scrollToTileMapIndex = -1;

    ImVec2 cursor1 = ImGui::GetCursorScreenPos(); cursor1.x += 1; cursor1.y += 3; int posX = cursor1.x; int posY = cursor1.y;

    while(index < (int)tileIndices.size()) {

      int outaX = 0;
      int outaY = 0;
      int tileX = 0;
      int tileY = 0;
      if (!getTilingAnimationFrameLayerCell(l2, tileIndices[index], tileX, tileY, outaX, outaY)) break;

      ImVec2 tx0,tx1;
      tx0.x = (double)(tileX*tileWidth+outaX)/l2->width; tx0.y = (double)(tileY*tileHeight+outaY)/l2->height;
      if (tx0.y >= 1) break;

      if (posY < cursor0.y + height) {
        tx1.x = (double)(tileX*tileWidth+tileWidth+outaX)/l2->width; tx1.y = (double)(tileY*tileHeight+tileHeight+outaY)/l2->height;
        ImGui::SetCursorScreenPos(ImVec2(posX,posY)); ImGui::Image((ImTextureID)tileMapTexture,ImVec2(tw-border,th-border),tx0,tx1); posX += tw;
        ImVec2 v0 = ImGui::GetItemRectMin();
        ImVec2 v1 = ImGui::GetItemRectMax();
        if (ImGui::IsMouseHoveringRect(v0,v1) && ImGui::IsMouseClicked(0)) l->toolInfo.tileMapSelectedTile = tileIndices[index];
        v0.x -= 1;
        v0.y -= 1;
        if (tileIndices[index] == l->toolInfo.tileMapSelectedTile) {
          ImGui::GetWindowDrawList()->AddRect(v0,v1,0xff0000ff);
        }
  
        if (posX + tw > cursor0.x + width - 16) {
          posX = cursor1.x;
          posY += th;
        }
      }
      index++;
    }
    posY = index / k * th - ImGui::GetScrollY();
    ImGui::SetCursorScreenPos(ImVec2(posX,posY));
    ImGui::Text(""); // ScrollBar
    ImGui::EndChild();
  }
}

void drawToolMirrorView() {
  ImGui::Text("Enabled"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMIRRORENABLED",&mirrorToolEnabled)) {;}

  if (mirrorToolEnabled) {
    bool updateSelection = false;
    if (roundButton("Make selection##DRAWTOOLMIRRORCREATESELECTION",true)) {updateSelection = true;}
    ImGui::Text("Selection >"); ImGui::SameLine(); if (ImGui::Checkbox("##DRAWTOOLMIRRORSELECTIONSIDEA",&mirrorToolSideA)) {;}
    ImGui::PushItemWidth(7*6);
    ImGui::Text("Angle"); ImGui::SameLine(); if (ImGui::InputFloat("##DRAWTOOLMIRRORANGLE",&mirrorToolAngle,0,0,2)) {;}; if (mirrorToolAngle < 0) mirrorToolAngle = 0; if (mirrorToolAngle>360) mirrorToolAngle = 360;
    float v;
    v = mirrorToolPoint.x; ImGui::Text("CanvasX"); ImGui::SameLine(); if (ImGui::InputFloat("##DRAWTOOLMIRRORPOINTX",&v,0,0,2)) {;}; mirrorToolPoint.x = v;
    v = mirrorToolPoint.y; ImGui::Text("CanvasY"); ImGui::SameLine(); if (ImGui::InputFloat("##DRAWTOOLMIRRORPOINTY",&v,0,0,2)) {;}; mirrorToolPoint.y = v;
    if (ImGui::IsMouseClicked(0) && isPaintAreaActive()) {
      double lx = mouseX, ly = mouseY; currentCanvas->getCanvasPos(lx,ly,lx,ly);
      mirrorToolPoint.x = lx;
      mirrorToolPoint.y = ly;
    }
    if (updateSelection) {
      Layer *l = currentCanvas->getActiveLayer();
      if (l != NULL && (!locked(l,LOCK_SELECTION))) {
        double px = mirrorToolPoint.x, py = mirrorToolPoint.y;
        double ax = cos(mirrorToolAngle*PI/180.0); double ay = sin(mirrorToolAngle*PI/180.0);
        for (int y = 0; y < l->height; y++) {
          for (int x = 0; x < l->width; x++) {
            double lx = x, ly = y; currentCanvas->getCanvasPosFromLayerPos(l, lx, ly, lx, ly);
            double dx = lx - px;
            double dy = ly - py;
            double d = -(dx * ay + dy * -ax);
            bool occluded = d > 0;
            if (!mirrorToolSideA) occluded = !occluded;
            l->pixelStates[x+y*l->width] &= ~PIXELSTATE_SELECTED;
            if (occluded) l->pixelStates[x+y*l->width] |= PIXELSTATE_SELECTED;
          }
        }
      }
      updatedPixelStates(l);
      repaintLayer(l,true);
    }
    ImGui::PopItemWidth();
  }
}

void drawToolView() {
  if (fullScreen) return;
  ImGui::SetNextWindowPos(ImVec2(toolbar_width,menuBar_height));
  ImGui::Begin("TOOLVIEW", NULL, ImVec2(toolview_width,main_window_yres-menuBar_height),1.0,FROZENWINDOW);
  if (galleryVisible) {
    ImGui::Text("Wypa gallery");
    ImGui::Separator();
    drawGalleryView();
  } else {
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
    case TOOL_MODIFY: {
        drawToolModifyView();
      } break;
    case TOOL_TILEMAP: {
        drawToolTileMapView();
      } break;
    case TOOL_MIRROR: {
        drawToolMirrorView();
      } break;
    }
  }
  ImGui::End();
}

void toolChangeMessage(int lastTool, int tool) {
  switch(lastTool) {
  case TOOL_TILING : {for (int i = 0; i < (int)currentCanvas->layers.size(); i++) updateTilingGrid(currentCanvas->layers[i],false);} break;
  }
  switch(tool) {
  case TOOL_TILING : {Layer *l = currentCanvas->getActiveLayer(); if (l != NULL) updateTilingGrid(l,true);} break;
  }
}
