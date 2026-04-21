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
  if (newPixelWidth<2) return;
  if (newPixelHeight<2) return;
  pixelWidth = (int)ceil(newPixelWidth);
  pixelHeight = (int)ceil(newPixelHeight);
  values.resize(pixelWidth*pixelHeight);
  double xf = base->size < 1 ? 1 : newPixelHeight/base->size;
  if (xf < 1) xf = 1;
  double yf = xf;
  double ax = (newPixelWidth-floor(newPixelWidth))*0.5;
  double ay = (newPixelHeight-floor(newPixelHeight))*0.5;
  double k = 0.1;
  double o = base->ovality + 1;
  double r = -base->rotation * PI / 180.0;
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
      d *= base->pressure;
      if (d > 1) d = 1;
      values[x+y*pixelWidth] = d;
    }
  }
  updateGLHandle(isPencil);
}

void BrushInstance::update(Brush *base) {
  resize(base, pixelWidth, pixelHeight);
}

void BrushInstance::updateGLHandle(bool pencil) {
  if (glHandle != 0) {glDeleteTextures(1,&glHandle); glHandle = 0;}
  glGenTextures(1,&glHandle);
  glBindTexture(GL_TEXTURE_2D,glHandle);
  Array<unsigned char> temp;
  temp.resize(values.size());
  if (!pencil) 
    for (int i = 0; i < temp.size(); i++) {temp[i] = values[i]>=0?values[i]*255:0;}
  else
    for (int i = 0; i < temp.size(); i++) {temp[i] = values[i]>=0?255:0;}

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
  double colorDist = sqrt(rd*rd+gd*gd+bd*bd);
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
  if ( (!ImGui::IsMouseDown(0)) || (layerSwitched) || (ImGui::IsMouseDown(1) && selectionTool != SELECTIONTOOL_POLYGON)) {
    if (selectionToolActive) {
      selectionToolActive = false; showSelectionPlane = false; showSelectionPlaneBounding = false; toolW = -1; toolH = -1;
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
  }
}
