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
#include "TOOLS.HPP"
#include "STL/GL.H"
#include "IMGUI/IMGUI.H"
#include "IMGUI/IMGUIGL.HPP"
#include "CONFIG.HPP"
#include "LAYERS.HPP"
#include "COLORFLD.HPP"
#include "TOOLBAR.HPP"
#include "TOOLVIEW.HPP"
#include "SERIALZE.HPP"
#include <math.h>


Array<Brush *> brushes;
int currentBrush = 0;

void initTools() {
  for (int i = 0; i < 64; i++) brushes.push_back(new Brush());
}

void freeTools() {
  for (int i = 0; i < brushes.size(); i++) delete brushes[i];
  brushes.clear();
}

double scrollborderSize = 10;
void scrollByCanvasBorders() {
  if (mouseX < currentCanvas->screenPosX + scrollborderSize)  currentCanvas->control.moveByPixels(currentCanvas,scrollKeyPixels,0);
  if (mouseX > currentCanvas->screenPosX + currentCanvas->width - scrollborderSize)  currentCanvas->control.moveByPixels(currentCanvas,-scrollKeyPixels,0);
  if (mouseY < currentCanvas->screenPosY + scrollborderSize)  currentCanvas->control.moveByPixels(currentCanvas,0,scrollKeyPixels);
  if (mouseY > currentCanvas->screenPosY + currentCanvas->height - scrollborderSize)  currentCanvas->control.moveByPixels(currentCanvas,0,-scrollKeyPixels);
}

void BrushInstance::resize(Brush *base, double newPixelWidth, double newPixelHeight) {
  if (newPixelWidth<1) newPixelWidth=1;
  if (newPixelHeight<1) newPixelHeight=1;
  pixelWidth = (int)ceil(newPixelWidth);
  pixelHeight = (int)ceil(newPixelHeight);
  values.resize(pixelWidth*pixelHeight);
  double xf = base->size < 1 ? 1 : newPixelHeight/base->size;
  double a = base->aspect == 0 ? 1 : base->aspect;
  if (a >= 1) a = 1;
  if (xf < 1.0/a) xf = 1.0/a;
  double yf = xf;
  double ax = (newPixelWidth-floor(newPixelWidth))*0.5;
  double ay = (newPixelHeight-floor(newPixelHeight))*0.5;
  double k = 0.1;
  double o = base->ovality + 1;
  double r = -base->rotation * PI / 180.0;
  bool isSinglePixel = pixelHeight < 3 && pixelWidth < 3;
  double pressure = powd(base->pressure,3);
  for (int y = 0; y < pixelHeight; y++) {
    for (int x = 0; x < pixelWidth; x++) {
      double dx = (double)(x-ax) / newPixelWidth * 2.0 - 1.0;
      double dy = (double)(y-ay) / newPixelHeight * 2.0 - 1.0;
      double dx2 = cos(r)*dx - sin(r)*dy;
      dy = sin(r)*dx + cos(r)*dy;
      dx = dx2;
      dx *= xf;
      dy *= yf;
      dx *= base->aspect;
      dx = powd(dx,o);
      dy = powd(dy,o);
      double d = 1-sqrt(dx*dx+dy*dy);
      double rx = fabs(dx);
      double ry = fabs(dy);
      double r = 1-maxd(rx,ry);
      d = base->edginess * (r-d) + d;
      d = powd(d,base->softness);
      d *= pressure;
      if (d > 1) d = 1;
      if (isSinglePixel) d = 1;
      if (isPencil) d = d >= 0 ? 1 : 0;
      //d *= base->opacity;
      values[x+y*pixelWidth] = d;
    }
  }
  updateGLHandle();
}

void BrushInstance::update(Brush *base) {
  resize(base, pixelWidth, pixelHeight);
}

void BrushInstance::updateGLHandle() {
  if (glHandle != 0) {glDeleteTextures(1,&glHandle); glHandle = 0;}
  glGenTextures(1,&glHandle);
  glBindTexture(GL_TEXTURE_2D,glHandle);
  Array<unsigned char> temp;
  temp.resize(values.size());
  double minv = 1; double maxv = 0;
  for (int i = 0; i < temp.size(); i++) {if (values[i]<0) continue; if (values[i]<minv) minv = values[i];if (values[i]>maxv) maxv = values[i];}
  if (maxv==minv) maxv = minv+0.1;
  double d = 255.0/(maxv-minv);
  for (int i2 = 0; i2 < temp.size(); i2++) {temp[i2] = values[i2]>=0?(values[i2]-minv)*d:0;}

  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, pixelWidth, pixelHeight, 0, GL_ALPHA, GL_BYTE, &temp[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);
}

BrushInstance *Brush::getCreateInstance(int brushInstance, int w, int h, bool pencil) {
  if (brushInstance < 0) return NULL;
  if (brushInstance >= BRUSHINSTANCE_COUNT) return NULL;
  if (instances[brushInstance] == NULL) instances[brushInstance] = new BrushInstance();
  BrushInstance *ret = instances[brushInstance];
  if (ret->pixelWidth != w || ret->pixelHeight != h || ret->isPencil != pencil) {
    ret->pixelWidth = w;
    ret->pixelHeight = h;
    ret->isPencil = pencil;
    ret->update(this);
  }
  return ret;
}

void Brush::freeInstance(int brushInstance) {
  if (brushInstance < 0) return;
  if (brushInstance >= BRUSHINSTANCE_COUNT) return;
  if (instances[brushInstance] != NULL) {delete instances[brushInstance]; instances[brushInstance] = NULL;}
}

void Brush::update() {
  for (int i = 0; i < BRUSHINSTANCE_COUNT; i++)
    if (instances[i] != NULL) instances[i]->update(this);
}

double Brush::pixelWidth() {
  double ret = size;
  if (aspect != 0 && size / aspect > ret) ret = size / aspect;
  return ret;
}

double Brush::pixelHeight() {
  return pixelWidth();
}

bool isPaintAreaActive() {
  return (!ImGui::GetIO().WantCaptureMouse) && (!ImGui::GetIO().WantCaptureKeyboard);
}

void moveSelectedLayersByCanvasPixels(double x, double y) {
  bool one = false;
  for (int i2 = 0; i2 < currentCanvas->layers.size(); i2++) {
    Layer *l = currentCanvas->layers[i2];
    if (l->state.selected) {one = true; break;}
  }

  for (int i = 0; i < currentCanvas->layers.size(); i++) {
    Layer *l = currentCanvas->layers[i];
    if ((l->state.selected && one) || (l->state.active && (!one))) {
      Vector b = l->boundingBox();
      l->setLayerPos(b.x+x,b.y+y);
    }
  }
  shouldUpdateAll = true;
}

double wandAlphaDistance = 20;
double wandColorDistance = 20;
bool wandByNeighbors = true;

bool isWandAccepted(unsigned int destColor, unsigned int checkColor) {
  int dr = destColor & 255;
  int dg = (destColor>>8) & 255;
  int db = (destColor>>16) & 255;
  int da = (destColor>>24) & 255;
  int sr = checkColor & 255;
  int sg = (checkColor>>8) & 255;
  int sb = (checkColor>>16) & 255;
  int sa = (checkColor>>24) & 255;
  int rd = dr - sr;
  int gd = dg - sg;
  int bd = db - sb;
  int ad = da - sa;
  double colorDist = sqrt(rd*rd+gd*gd+bd*bd);
  double alphaDist = abs(ad);
  return alphaDist<wandAlphaDistance && colorDist<wandColorDistance;
}

void useToolWand() {
  if (!isPaintAreaActive()) return;
  if (!ImGui::IsMouseClicked(0)) return;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  int lx, ly; double lx2, ly2; currentCanvas->getLayerPos(l, mouseX, mouseY, lx2, ly2); lx = (int)floor(lx2); ly = (int)floor(ly2);
  if ((unsigned int)lx>=l->width||(unsigned int)ly>=l->height) return;
  int colorHere = l->pixels[lx+ly*l->width];
  if (controlKeyId == CONTROLKEY_NONE) for (int i = 0; i < l->width*l->height; i++) l->pixelStates[i] &= ~PIXELSTATE_SELECTED;

  if (wandByNeighbors) {
    initStatusBar();
    for (int i2 = 0; i2 < l->width*l->height; i2++) l->pixelStates[i2] &= ~PIXELSTATE_TEMP;
    l->pixelStates[lx+ly*l->width] |= PIXELSTATE_TEMP;
    l->pixelStates[lx+ly*l->width] |= PIXELSTATE_SELECTED;
    bool somethingFound = true;
    int counter = 0;
    while(somethingFound) {
      somethingFound = false;
      for (int y = 0; y < l->height; y++) {
        for (int x = 0; x < l->width; x++) {
          if (l->pixelStates[x+y*l->width] & PIXELSTATE_TEMP) {
            if (x>0&&(!(l->pixelStates[x-1+y*l->width] & PIXELSTATE_TEMP))) if (isWandAccepted(l->pixels[x-1+y*l->width],colorHere)) {somethingFound = true; l->pixelStates[x-1+y*l->width] |= PIXELSTATE_TEMP;}
            if (x<l->width-1&&(!(l->pixelStates[x+1+y*l->width] & PIXELSTATE_TEMP))) if (isWandAccepted(l->pixels[x+1+y*l->width],colorHere)) {somethingFound = true; l->pixelStates[x+1+y*l->width] |= PIXELSTATE_TEMP;}
            if (y>0&&(!(l->pixelStates[x+(y-1)*l->width] & PIXELSTATE_TEMP))) if (isWandAccepted(l->pixels[x+(y-1)*l->width],colorHere)) {somethingFound = true; l->pixelStates[x+(y-1)*l->width] |= PIXELSTATE_TEMP;}
            if (y<l->height-1&&(!(l->pixelStates[x+(y+1)*l->width] & PIXELSTATE_TEMP))) if (isWandAccepted(l->pixels[x+(y+1)*l->width],colorHere)) {somethingFound = true; l->pixelStates[x+(y+1)*l->width] |= PIXELSTATE_TEMP;}
          }
        }
      }
      counter++; if ((counter % 100)==0) updateStatusBar();
    }
    for (int i = 0; i < l->width*l->height; i++) if (l->pixelStates[i] & PIXELSTATE_TEMP) {
      if (!ctrl) l->pixelStates[i] |= PIXELSTATE_SELECTED; else l->pixelStates[i] &= ~PIXELSTATE_SELECTED;
    }
  } else {
    for (int i = 0; i < l->width*l->height; i++) if (isWandAccepted(l->pixels[i],colorHere)) {
      if (!ctrl) l->pixelStates[i] |= PIXELSTATE_SELECTED; else l->pixelStates[i] &= ~PIXELSTATE_SELECTED;
    }
  }

  repaintLayer(l,true);
}

void placeCorner(double mouseX, double mouseY) {
  Array<Layer *> selected;
  for (int i = 0; i < currentCanvas->layers.size(); i++) {if (currentCanvas->layers[i]->state.selected||currentCanvas->layers[i]->state.active) selected.push_back(currentCanvas->layers[i]);}
  if (selected.empty()) return;

  if (selected.size() == 1) {
    Layer *l = selected[0];
    Vector b = l->boundingBox();
    double cx = mouseX,cy=mouseY; currentCanvas->getCanvasPos(cx,cy,cx,cy);
    int found = 0;
    double dist = 0;
    for (int i = 0; i < 4; i++) {
      Vector d = Vector(i & 1 ? b.z : b.x, i & 2 ? b.w : b.y);
      if (i == 0 || length(d-Vector(cx,cy,0))<dist) {
        dist = length(d-Vector(cx,cy,0));
        found = i;
      }
    }
    cx = (int)floor(cx);
    cy = (int)floor(cy);
    int cx2 = cx + 1;
    int cy2 = cy + 1;
    switch(found) {
    case 0: { 
        l->setLayerPos(cx,cy);
        l->setLayerSize(b.z-cx,b.w-cy);
      } break;
    case 1: { 
        l->setLayerPos(b.x,cy);
        l->setLayerSize(cx2-b.x,b.w-cy);
      } break;
    case 2: { 
        l->setLayerPos(cx,b.y);
        l->setLayerSize(b.z-cx,cy2-b.y);
      } break;
    case 3: { 
        l->setLayerPos(b.x,b.y);
        l->setLayerSize(cx2-b.x,cy2-b.y);
      } break;
    }
  } else {
    Vector b = currentCanvas->boundingBox();
    double cx = mouseX,cy=mouseY; currentCanvas->getCanvasPos(cx,cy,cx,cy);
    int found = 0;
    double dist = 0;
    for (int i = 0; i < 4; i++) {
      Vector d = Vector(i & 1 ? b.z : b.x, i & 2 ? b.w : b.y);
      if (i == 0 || length(d-Vector(cx,cy,0))<dist) {
        dist = length(d-Vector(cx,cy,0));
        found = i;
      }
    }
    cx = (int)floor(cx);
    cy = (int)floor(cy);
    int cx2 = cx + 1;
    int cy2 = cy + 1;
    switch(found) {
    case 0: { 
        currentCanvas->setBoundingPos(cx,cy);
        currentCanvas->setBoundingSize(b.z-cx,b.w-cy);
      } break;
    case 1: { 
        currentCanvas->setBoundingPos(b.x,cy);
        currentCanvas->setBoundingSize(cx2-b.x,b.w-cy);
      } break;
    case 2: { 
        currentCanvas->setBoundingPos(cx,b.y);
        currentCanvas->setBoundingSize(b.z-cx,cy2-b.y);
      } break;
    case 3: { 
        currentCanvas->setBoundingPos(b.x,b.y);
        currentCanvas->setBoundingSize(cx2-b.x,cy2-b.y);
      } break;
    }
  }
  shouldUpdateAll = true;
}

void useToolMove() {
  if (!isPaintAreaActive()) return;
  if (ImGui::IsMouseDown(0)) {
    if (ctrl)
      placeCorner(mouseX,mouseY);
    else {
      double nx = 0, ny = 0; currentCanvas->getCanvasPos(nx, ny, nx, ny);
      double cx = mouseDX, cy = mouseDY; currentCanvas->getCanvasPos(cx, cy, cx, cy);
      moveSelectedLayersByCanvasPixels(cx-nx, cy-ny);
    }
  }
  static bool lastRight = ImGui::IsMouseDown(1);
  if (ImGui::IsMouseDown(1)) {
    Layer *l = currentCanvas->getActiveLayer(); 
    if (l != NULL) {
      static double centerX=0,centerY=0;
      static double lastAngle = -1;
      double lx, ly; currentCanvas->getLayerPos(l, mouseX, mouseY, lx, ly);
      if (!lastRight) {centerX = (double)l->width/2.0; centerY = (double)l->height/2.0; lastAngle = -1;}
      double deltaX = lx - centerX; double deltaY = ly - centerY;
      if (deltaX != 0 && deltaY != 0) {
        double angle = fmod((atan2(deltaY,deltaX)*180.0/PI+360-90),360.0);
        if (lastAngle<0) lastAngle = angle;
        double angleDelta = angle - lastAngle;
        l->rotateByDegrees(angleDelta);
      }  
    }
  }
  lastRight = ImGui::IsMouseDown(1);
  double moveX = 1, moveY = 1;
  if (shift) {
    moveX=currentCanvas->gridStepX;
    moveY=currentCanvas->gridStepY;
  }
  if (currentKey == GL_VK_LEFT) moveSelectedLayersByCanvasPixels(-moveX,0);
  if (currentKey == GL_VK_RIGHT) moveSelectedLayersByCanvasPixels(moveX,0);
  if (currentKey == GL_VK_UP) moveSelectedLayersByCanvasPixels(0,-moveY);
  if (currentKey == GL_VK_DOWN) moveSelectedLayersByCanvasPixels(0,moveY);
}

bool pipetteFromLayer = false;

void useToolPipette() {
  if ( (!ImGui::IsMouseDown(0)) && (!ImGui::IsMouseDown(1)) ) return;
  if (!pipetteFromLayer) {
    foregroundColor = getImVec4Color(currentCanvas->getCanvasColor(mouseX,mouseY));
    addUsedColor(foregroundColor);
  } else {
    foregroundColor = getImVec4Color(currentCanvas->getLayerColor(mouseX,mouseY));
    addUsedColor(foregroundColor);
  }
}

double fillColorDistance = 1;

bool isFillAccepted(unsigned int destColor, unsigned int checkColor) {
  int dr = destColor & 255;
  int dg = (destColor>>8) & 255;
  int db = (destColor>>16) & 255;
  int da = (destColor>>24) & 255;
  int sr = checkColor & 255;
  int sg = (checkColor>>8) & 255;
  int sb = (checkColor>>16) & 255;
  int sa = (checkColor>>24) & 255;
  int rd = dr - sr;
  int gd = dg - sg;
  int bd = db - sb;
  int ad = da - sa;
  double colorDist = sqrt(rd*rd+gd*gd+bd*bd+ad*ad);
  return colorDist<fillColorDistance;
}

void useToolFill() {
  if (!isPaintAreaActive()) return;
  if (!ImGui::IsMouseClicked(0)) return;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  int lx, ly; double lx2, ly2; currentCanvas->getLayerPos(l, mouseX, mouseY, lx2, ly2); lx = (int)floor(lx2); ly = (int)floor(ly2);
  if ((unsigned int)lx>=l->width||(unsigned int)ly>=l->height) return;
  int colorHere = l->pixels[lx+ly*l->width];

  initStatusBar();
  for (int i2 = 0; i2 < l->width*l->height; i2++) l->pixelStates[i2] &= ~PIXELSTATE_TEMP;
  l->pixelStates[lx+ly*l->width] |= PIXELSTATE_TEMP;
  bool somethingFound = true;
  int counter = 0;
  while(somethingFound) {
    somethingFound = false;
    for (int y = 0; y < l->height; y++) {
      for (int x = 0; x < l->width; x++) {
        if (l->pixelStates[x+y*l->width] & PIXELSTATE_TEMP) {
          if (x>0&&(!(l->pixelStates[x-1+y*l->width] & PIXELSTATE_TEMP))) if (isFillAccepted(l->pixels[x-1+y*l->width],colorHere)) {somethingFound = true; l->pixelStates[x-1+y*l->width] |= PIXELSTATE_TEMP;}
          if (x<l->width-1&&(!(l->pixelStates[x+1+y*l->width] & PIXELSTATE_TEMP))) if (isFillAccepted(l->pixels[x+1+y*l->width],colorHere)) {somethingFound = true; l->pixelStates[x+1+y*l->width] |= PIXELSTATE_TEMP;}
          if (y>0&&(!(l->pixelStates[x+(y-1)*l->width] & PIXELSTATE_TEMP))) if (isFillAccepted(l->pixels[x+(y-1)*l->width],colorHere)) {somethingFound = true; l->pixelStates[x+(y-1)*l->width] |= PIXELSTATE_TEMP;}
          if (y<l->height-1&&(!(l->pixelStates[x+(y+1)*l->width] & PIXELSTATE_TEMP))) if (isFillAccepted(l->pixels[x+(y+1)*l->width],colorHere)) {somethingFound = true; l->pixelStates[x+(y+1)*l->width] |= PIXELSTATE_TEMP;}
        }
      }
    }
    counter++; if ((counter % 100)==0) updateStatusBar();
  }
  for (int i = 0; i < l->width*l->height; i++) if (l->pixelStates[i] & PIXELSTATE_TEMP) {
    l->pixels[i] = getUIntColor(foregroundColor);
  }
  repaintLayer(l,true);
}

int selectionTool = SELECTIONTOOL_RECTANGLE;
bool selectionProportional = false;
bool selectionToolActive = false;

double selectionX0 = 0;
double selectionY0 = 0;
double selectionX1 = 0;
double selectionY1 = 0;
double lastSelectionX1 = 0;
double lastSelectionY1 = 0;
double lastSelectionPolygonX1 = 0;
double lastSelectionPolygonY1 = 0;
double toolW = -1, toolH = -1;

void useToolSelection() {
  Layer *activeLayer = currentCanvas->getActiveLayer();
  static Layer *selectionToolLayer = activeLayer;
  bool layerSwitched = selectionToolLayer != activeLayer;
  selectionToolLayer = activeLayer;
  if (selectionToolLayer != NULL && selectionToolLayer->width * selectionToolLayer->height==0) return;
  if (selectionToolLayer == NULL||  (!ImGui::IsMouseDown(0)) || (layerSwitched) || (ImGui::IsMouseDown(1) && selectionTool != SELECTIONTOOL_POLYGON)) {
    if (selectionToolActive) {
      selectionToolActive = false; showSelectionPlane = false; showSelectionPlaneBounding = false; toolW = -1; toolH = -1;
      if (layerSwitched) return;
      if (!ImGui::IsMouseDown(1)) {
        switch(selectionTool) {
          case SELECTIONTOOL_ELLIPSE:
          case SELECTIONTOOL_RECTANGLE: {
            for (int i = 0; i < activeLayer->width*activeLayer->height; i++) {
              if ((!shift) && (!ctrl)) activeLayer->pixelStates[i] &= ~PIXELSTATE_SELECTED;
              if (selectionPlane->pixels[i]!=0) {
                if (!ctrl) activeLayer->pixelStates[i] |= PIXELSTATE_SELECTED; else activeLayer->pixelStates[i] &= ~PIXELSTATE_SELECTED;
              }
            }
            shouldUpdateAll = true;
          } break;
          case SELECTIONTOOL_POLYGON:
          case SELECTIONTOOL_LASSO: {
            double lx0,ly0; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,selectionX0,selectionY0,lx0,ly0);
            double lx1,ly1; 
            currentCanvas->getLayerPosFromCanvasPos(selectionPlane,selectionX1,selectionY1,lx1,ly1);
            if (selectionTool == SELECTIONTOOL_LASSO) selectionPlane->pixelXLineEor(lx0,ly0,lx1,ly1,1);
            for (int y = 0; y < selectionPlane->height; y++) {
              bool isInSelection = false;
              for (int x = 0; x < selectionPlane->width; x++) {
                bool edge = selectionPlane->pixels[x+y*selectionPlane->width] & 1 ? true : false;
                if (x == 0) {isInSelection = edge;edge = false;}
                if (isInSelection) selectionPlane->pixels[x+y*selectionPlane->width] = 1;
                if (edge) {isInSelection = !isInSelection;}
              }
            }

            for (int i = 0; i < activeLayer->width*activeLayer->height; i++) {
              if ((!shift) && (!ctrl)) activeLayer->pixelStates[i] &= ~PIXELSTATE_SELECTED;
              if (selectionPlane->pixels[i]!=0) {
                if (!ctrl) activeLayer->pixelStates[i] |= PIXELSTATE_SELECTED; else activeLayer->pixelStates[i] &= ~PIXELSTATE_SELECTED;
              }
            }
            shouldUpdateAll = true;
          } break;
        }
      }
    } 
    return;
  }
  if (isPaintAreaActive()) {
    double lx,ly; currentCanvas->getCanvasPos(mouseX,mouseY,lx,ly);
    static int pointCount = 0;
    if (!selectionToolActive) {
      selectionPlane->init(activeLayer->width, activeLayer->height);
      selectionPlane->transform = activeLayer->transform;
      selectionX0 = lx;
      selectionY0 = ly;
      selectionX1 = lx;
      selectionY1 = ly;
      lastSelectionPolygonX1 = lx;
      lastSelectionPolygonY1 = ly;
      pointCount = 0;
    }
    lastSelectionX1 = selectionX1;
    lastSelectionY1 = selectionY1;
    selectionX1 = lx;
    selectionY1 = ly;
    if (selectionProportional) if (lx-selectionX0>ly-selectionY0) selectionY1 = selectionY0+lx-selectionX0; else selectionX1 = selectionX0+ly-selectionY0;
    scrollByCanvasBorders();
    double x0 = selectionX0 < selectionX1 ? selectionX0 : selectionX1;
    double y0 = selectionY0 < selectionY1 ? selectionY0 : selectionY1;
    double x1 = selectionX0 < selectionX1 ? selectionX1 : selectionX0;
    double y1 = selectionY0 < selectionY1 ? selectionY1 : selectionY0;
    x0 = floor(x0);
    y0 = floor(y0);
    x1 = floor(x1);
    y1 = floor(y1);
    selectionToolActive = true;
    showSelectionPlane = true;
    showSelectionPlaneBounding = selectionTool == SELECTIONTOOL_ELLIPSE;
    switch(selectionTool) {
      case SELECTIONTOOL_ELLIPSE:
      case SELECTIONTOOL_RECTANGLE: {
        toolW = x1 - x0;
        toolH = y1 - y0;
        selectionPlane->clear();
        Matrix m = inverse(selectionPlane->transform.transformation);
        Vector b = currentCanvas->getLayerPixelBounding(selectionPlane,x0,y0,x1,y1);
        for (int yl = b.y; yl < b.w; yl++) {
          for (int xl = b.x; xl < b.z; xl++) {
            double x2 = xl * m.m[0*4+0]+yl * m.m[1*4+0]+m.m[3*4+0];
            double y2 = xl * m.m[0*4+1]+yl * m.m[1*4+1]+m.m[3*4+1];
            int x = (int)x2;
            int y = (int)y2;
            if (x == 0) x = (int)floor(x2);
            if (y == 0) y = (int)floor(y2);
            if (x >= x0 && x < x1 && y >= y0 && y < y1) {
              if (selectionTool == SELECTIONTOOL_ELLIPSE) {
                double dx = (double)(x-x0+0.5)/(x1-x0)*2.0-1.0;
                double dy = (double)(y-y0+0.5)/(y1-y0)*2.0-1.0;
                double d = sqrt(dx*dx+dy*dy);
                if (d > 1) continue;
              }
              if ((unsigned int)xl < selectionPlane->width && (unsigned int)yl < selectionPlane->height)
                selectionPlane->pixels[xl+yl*selectionPlane->width] = 1;
            }
          }
        }
      } break;
      case SELECTIONTOOL_LASSO: {
        double lx0,ly0; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,lastSelectionX1,lastSelectionY1,lx0,ly0);
        double lx1,ly1; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,selectionX1,selectionY1,lx1,ly1);
        selectionPlane->pixelLineOr(lx0,ly0,lx1,ly1,2);
        selectionPlane->pixelXLineEor(lx0,ly0,lx1,ly1,1);
      } break;
      case SELECTIONTOOL_POLYGON: {
        double lx0,ly0; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,lastSelectionPolygonX1,lastSelectionPolygonY1,lx0,ly0);
        double lx1,ly1; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,selectionX1,selectionY1,lx1,ly1);
        double lx2,ly2; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,lastSelectionX1,lastSelectionY1,lx2,ly2);
        double lx3,ly3; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,selectionX0,selectionY0,lx3,ly3);
        if (ImGui::IsMouseDown(1)) {
          if (pointCount == 0) {selectionPlane->pixelXLineEor(lx0,ly0,lx1,ly1,1); selectionPlane->pixelLineEor(lx0,ly0,lx1,ly1,2);}
          lastSelectionPolygonX1 = lx;
          lastSelectionPolygonY1 = ly;
          pointCount++;
        } else {
          selectionPlane->pixelXLineEor(lx0,ly0,lx2,ly2,1); // delete
          selectionPlane->pixelXLineEor(lx0,ly0,lx1,ly1,1); // paint
          selectionPlane->pixelLineEor(lx0,ly0,lx2,ly2,2);
          selectionPlane->pixelLineEor(lx0,ly0,lx1,ly1,2);
          if (pointCount > 0) {
            selectionPlane->pixelXLineEor(lx3,ly3,lx2,ly2,1); // delete
            selectionPlane->pixelXLineEor(lx3,ly3,lx1,ly1,1); // paint
            selectionPlane->pixelLineEor(lx3,ly3,lx2,ly2,2);
            selectionPlane->pixelLineEor(lx3,ly3,lx1,ly1,2);
          }
        }
      } break;
    }
  }
}

bool paintingOn = false;
Array<float> paintValues;
Array<unsigned int> paintBackground;

double paintingPointX0 = 0;
double paintingPointY0 = 0;
double paintingPointX1 = 0;
double paintingPointY1 = 0;

void drawTool(Layer *layer, Brush *brush,double canvasX, double canvasY, double angle) {
  double xs = (rand() & 1023)/1023.0*2.0-1.0;
  double ys = (rand() & 1023)/1023.0*2.0-1.0;
  canvasX += xs * brush->randomX;
  canvasY += ys * brush->randomY;
  bool pencil = tool == TOOL_PENCIL;
  BrushInstance *b = brush->getCreateInstance(BRUSHINSTANCE_PAINT,brush->pixelWidth(),brush->pixelHeight(),pencil);
  double toolWidth = b->pixelWidth*0.5;
  double toolHeight = b->pixelHeight*0.5;
  Vector layerBoundings;
  bool first = true;
  int layerUpdateX0 = 0;
  int layerUpdateY0 = 0;
  int layerUpdateX1 = layer->width;
  int layerUpdateY1 = layer->height;
  for (int i = 0; i < 4; i++) {
    Vector k(i & 1 ? canvasX-toolWidth : canvasX+toolWidth,i & 2 ? canvasY-toolHeight : canvasY+toolHeight,0);
    double lx0,ly0; currentCanvas->getLayerPosFromCanvasPos(layer,k.x,k.y,lx0,ly0);
    if (first || lx0 < layerBoundings.x) layerBoundings.x = lx0;
    if (first || ly0 < layerBoundings.y) layerBoundings.y = ly0;
    if (first || lx0 > layerBoundings.z) layerBoundings.z = lx0;
    if (first || ly0 > layerBoundings.w) layerBoundings.w = ly0;
    first = false;
  }
  layerBoundings.z += 1;
  layerBoundings.w += 1;
  if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = layerBoundings.x;
  if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = layerBoundings.y;
  if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = layerBoundings.z;
  if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = layerBoundings.w;
  double tpx = canvasX-toolWidth, tpy = canvasY-toolHeight;
  Matrix t; t.translate(Vector(tpx,tpy,0));
  Matrix m = inverse(t)*inverse(layer->transform.transformation);
  bool interpolate = false;
  if (brush->followRotation) {
    interpolate = true;
    if (angle >= 0) {b->followedRotation = angle;}
    Matrix toCenter; toCenter.translate(Vector(toolWidth,toolHeight,0));
    m = toCenter * rotationMatrixAngleAxis(b->followedRotation, 0, 0, 1) * inverse(toCenter) * m;
  }
  for (int y = layerUpdateY0; y < layerUpdateY1; y++) {
    for (int x = layerUpdateX0; x < layerUpdateX1; x++) {
      double px = x*m.m[4*0+0]+y*m.m[4*1+0]+m.m[4*3+0];
      double py = x*m.m[4*0+1]+y*m.m[4*1+1]+m.m[4*3+1];
      int bx = (int)px;
      int by = (int)py;
      if (bx == 0) bx = (int)floor(px);
      if (by == 0) by = (int)floor(py);
      if ((unsigned int)bx < b->pixelWidth && (unsigned int)by < b->pixelHeight) {
        int p = x + y * layer->width;
        float here = b->values[bx+by*b->pixelWidth];
        if (interpolate) {
          float hereX = here;
          float hereY = here;
          float hereXY = here;
          if (bx + 1 < b->pixelWidth) hereX = b->values[bx+1+by*b->pixelWidth];;
          if (by + 1 < b->pixelHeight) hereY = b->values[bx+(by+1)*b->pixelWidth];;
          if (bx + 1 < b->pixelWidth && by + 1 < b->pixelHeight) hereXY = b->values[bx+1+(by+1)*b->pixelWidth];;
          float xf = px-bx;
          float yf = py-by;
          float top = (hereX-here)*xf + here;
          float bottom = (hereXY-hereY)*xf + hereY;
          here = (bottom-top)*yf+top;
        }
        if (here >= 0) {
          float value = paintValues[p];
          value = 1-(1-here)*(1-value);
          paintValues[p] = value;
        }
      }
    }
  }
  if (tool != TOOL_ERASER) addUsedColor(foregroundColor);
}

void useToolPainting() {
  if (!paintingOn && ImGui::IsMouseClicked(1)) {foregroundColor = getImVec4Color(currentCanvas->getLayerColor(mouseX, mouseY)); addUsedColor(foregroundColor);}
  if ((unsigned int)currentBrush >= brushes.size()) return;
  Brush *brush = brushes[currentBrush];
  static double rateCounter = 0;
  static bool waitForMouse0 = false;
  if (waitForMouse0) {
    if (ImGui::IsMouseDown(0)) return;
    waitForMouse0 = false;
  }
  Layer *activeLayer = currentCanvas->getActiveLayer();
  static Layer *paintToolLayer = activeLayer;
  bool layerSwitched = paintToolLayer != activeLayer;
  paintToolLayer = activeLayer;
  if (paintToolLayer != NULL && paintToolLayer->width * paintToolLayer->height==0) return;
  if ( paintToolLayer == NULL || (!ImGui::IsMouseDown(0)) || layerSwitched || ImGui::IsMouseDown(1)) {
    if (paintingOn) {
      paintingOn = false;
      if (layerSwitched) return;
      if (ImGui::IsMouseDown(1)) {
        memcpy(paintToolLayer->pixels,&paintBackground[0],paintToolLayer->width * paintToolLayer->height * sizeof(unsigned int));
        waitForMouse0 = true;
        shouldUpdateAll = true;
      }
    } 
    return;
  }
  if (isPaintAreaActive()) {
    double lx,ly; currentCanvas->getCanvasPos(mouseX,mouseY,lx,ly);
    static double dx2,dy2;
    static bool initRotation = false;
    static double initialAngle = 0;
    if (!paintingOn) {
      paintingOn = true;
      paintingPointX0 = lx;
      paintingPointY0 = ly;
      paintingPointX1 = lx;
      paintingPointY1 = ly;
      paintValues.resize(paintToolLayer->width * paintToolLayer->height);
      paintBackground.resize(paintToolLayer->width * paintToolLayer->height);
      memset(&paintValues[0],0,paintToolLayer->width * paintToolLayer->height * sizeof(float));
      memcpy(&paintBackground[0],paintToolLayer->pixels,paintToolLayer->width * paintToolLayer->height * sizeof(unsigned int));
      currentCanvas->clearUpdateFlag();
      rateCounter = 1;
      initRotation = true;
      initialAngle = 0;
      dx2 = 1;
      dy2 = 0;
    }
    paintingPointX0 = paintingPointX1;
    paintingPointY0 = paintingPointY1;
    paintingPointX1 = lx;
    paintingPointY1 = ly;

    int layerUpdateX0 = 0;
    int layerUpdateY0 = 0;
    int layerUpdateX1 = paintToolLayer->width;
    int layerUpdateY1 = paintToolLayer->height;
    double toolWidth = brush->pixelWidth()*0.5+brush->randomX;
    double toolHeight = brush->pixelHeight()*0.5+brush->randomY;
    Vector layerBoundings;
    bool first = true;
    for (int i = 0; i < 8; i++) {
      Vector k;
      if (i < 4) 
        k = Vector(i & 1 ? paintingPointX0-toolWidth : paintingPointX0+toolWidth,i & 2 ? paintingPointY0-toolHeight : paintingPointY0+toolHeight,0);
      else 
        k = Vector(i & 1 ? paintingPointX1-toolWidth : paintingPointX1+toolWidth,i & 2 ? paintingPointY1-toolHeight : paintingPointY1+toolHeight,0);
      double lx0,ly0; currentCanvas->getLayerPosFromCanvasPos(paintToolLayer,k.x,k.y,lx0,ly0);
      if (first || lx0 < layerBoundings.x) layerBoundings.x = lx0;
      if (first || ly0 < layerBoundings.y) layerBoundings.y = ly0;
      if (first || lx0 > layerBoundings.z) layerBoundings.z = lx0;
      if (first || ly0 > layerBoundings.w) layerBoundings.w = ly0;
      first = false;
    }
    layerBoundings.z += 1;
    layerBoundings.w += 1;
    if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = layerBoundings.x;
    if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = layerBoundings.y;
    if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = layerBoundings.z;
    if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = layerBoundings.w;

    double px0 = paintingPointX0; double py0 = paintingPointY0; currentCanvas->getLayerPosFromCanvasPos(paintToolLayer,px0,py0,px0,py0);
    double px1 = paintingPointX1; double py1 = paintingPointY1; currentCanvas->getLayerPosFromCanvasPos(paintToolLayer,px1,py1,px1,py1);

    int lpx0 = (int)floor(px0);
    int lpy0 = (int)floor(py0);
    int lpx1 = (int)floor(px1);
    int lpy1 = (int)floor(py1);
    int deltaX = lpx1-lpx0;
    int deltaY = lpy1-lpy0;
    int delta = abs(deltaX);
    if (abs(deltaY)>delta) delta = abs(deltaY);

    double px = lpx0;
    double py = lpy0;
    double ax = delta == 0 ? 0 : (double)(lpx1-lpx0)/delta;
    double ay = delta == 0 ? 0 : (double)(lpy1-lpy0)/delta;
    Matrix m2 = inverse(paintToolLayer->transform.transformation);
    for (int i3 = 0; i3 < delta+1; i3++) {
      double angle = -1;
      if (brush->followRotation) {
        double dx = paintingPointX1 - paintingPointX0;
        double dy = paintingPointY1 - paintingPointY0;
        if (delta > 0) {
          double l = sqrt(dx*dx+dy*dy);
          if (l != 0) {dx /= l; dy /= l;}
          dx2 = (dx-dx2)*(1.0/delta)+dx2;
          dy2 = (dy-dy2)*(1.0/delta)+dy2;
        }
        if (dx != 0 || dy != 0) {
          if (initRotation) {
            initialAngle = atan2(dx,dy) + PI*0.5;
            dx2 = dx;
            dy2 = dy;
          }
          initRotation = false;
        }
        if (dx2 != 0 || dy2 != 0) {
          angle = atan2(dx2,dy2)-initialAngle + PI*0.5 + PI * 8;
        }
      }
      rateCounter += powd(brush->rate,2.0);
      if (rateCounter>0.99) {
        rateCounter = 0;
        double cx = px*m2.m[4*0+0]+py*m2.m[4*1+0]+m2.m[4*3+0];
        double cy = px*m2.m[4*0+1]+py*m2.m[4*1+1]+m2.m[4*3+1];
        drawTool(paintToolLayer,brush,cx,cy,angle);
      }
      px += ax;
      py += ay;
    }

    for (int y = layerUpdateY0; y < layerUpdateY1; y++) {
      for (int x = layerUpdateX0; x < layerUpdateX1; x++) {
        int p = x + y * paintToolLayer->width;
        float value = paintValues[p];
        unsigned int col = paintBackground[p];
        switch(tool) {
          case TOOL_PAINTBRUSH:
          case TOOL_PENCIL: {
            float rd = col & 255;
            float gd = (col>>8) & 255;
            float bd = (col>>16) & 255;
            int ad = (col>>24) & 255;
            float rs = foregroundColor.x*255.f;
            float gs = foregroundColor.y*255.f;
            float bs = foregroundColor.z*255.f;
            int as = foregroundColor.w*value*255.f*brush->opacity;
            if (rs < 0) rs = 0;
            if (gs < 0) gs = 0;
            if (bs < 0) bs = 0;
            if (as < 0) as = 0;
            if (rs > 255) rs = 255;
            if (gs > 255) gs = 255;
            if (bs > 255) bs = 255;
            if (as > 255) as = 255;
            //int a = ad>as?ad:as;
            int a = 255-(255-ad)*(255-as)/255; if (a > 255) a = 255;
            if (a > 0) {
              rd *= ad;
              gd *= ad;
              bd *= ad;
              rd /= a;
              gd /= a;
              bd /= a;
            }
            int r = (rs-rd)*as/255+rd;
            int g = (gs-gd)*as/255+gd;
            int b = (bs-bd)*as/255+bd;
            paintToolLayer->pixels[p] = (r)|(g<<8)|(b<<16)|(a<<24);
          } break;
          case TOOL_ERASER: {
            int ad = (col>>24) & 255;
            ad *= 1-value;
            if (ad < 0) ad = 0;
            paintToolLayer->pixels[p] = (col & 0x00ffffff)|(ad<<24);
          } break;
        }
      }
    }

    first = true;
    for (int i2 = 0; i2 < 4; i2++) {
      Vector k(i2 & 1 ? layerUpdateX1 : layerUpdateX0,i2 & 2 ? layerUpdateY1 : layerUpdateY0,0);
      double lx0,ly0; currentCanvas->getScreenPos(paintToolLayer,k.x,k.y,lx0,ly0);
      lx0 -= currentCanvas->screenPosX;
      ly0 -= currentCanvas->screenPosY;
      if (first || lx0 < layerBoundings.x) layerBoundings.x = lx0;
      if (first || ly0 < layerBoundings.y) layerBoundings.y = ly0;
      if (first || lx0 > layerBoundings.z) layerBoundings.z = lx0;
      if (first || ly0 > layerBoundings.w) layerBoundings.w = ly0;
      first = false;
    }
    layerBoundings.z += 1;
    layerBoundings.w += 1;
    layerUpdateX0 = 0;
    layerUpdateY0 = 0;
    layerUpdateX1 = currentCanvas->width;
    layerUpdateY1 = currentCanvas->height;
    if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = layerBoundings.x;
    if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = layerBoundings.y;
    if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = layerBoundings.z;
    if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = layerBoundings.w;

    static int someTimer = 0; someTimer++; if ((someTimer & 15)==0) currentCanvas->clearUpdateFlag();
    for (int y2 = layerUpdateY0; y2 < layerUpdateY1; y2++) {
      for (int x2 = layerUpdateX0; x2 < layerUpdateX1; x2++) {
        currentCanvas->pixelStates[x2+y2*currentCanvas->width] |= PIXELSTATE_UPDATED;
      }
    }

    repaintLayer(paintToolLayer,false);
  }
}

void useTool() {
  switch(tool) {
  case TOOL_WAND: {
    useToolWand();
    } break;
  case TOOL_MOVE: {
    useToolMove();
    } break;
  case TOOL_PIPETTE: {
    useToolPipette();
    } break;
  case TOOL_FILL: {
    useToolFill();
    } break;
  case TOOL_SELECTION: {
    useToolSelection();
    } break;
  case TOOL_PENCIL: {
    useToolPainting();
    } break;
  case TOOL_ERASER: {
    useToolPainting();
    } break;
  case TOOL_PAINTBRUSH: {
    useToolPainting();
    } break;
  case TOOL_MAGIC: {
    useToolPainting();
    } break;
  }
}
