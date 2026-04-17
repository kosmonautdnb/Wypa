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
    for (int i2 = 0; i2 < l->width*l->height; i2++) l->pixelStates[i2] &= ~PIXELSTATE_FILLED;
    l->pixelStates[lx+ly*l->width] |= PIXELSTATE_FILLED;
    l->pixelStates[lx+ly*l->width] |= PIXELSTATE_SELECTED;
    bool somethingFound = true;
    int counter = 0;
    while(somethingFound) {
      somethingFound = false;
      for (int y = 0; y < l->height; y++) {
        for (int x = 0; x < l->width; x++) {
          if (l->pixelStates[x+y*l->width] & PIXELSTATE_FILLED) {
            if (x>0&&(!(l->pixelStates[x-1+y*l->width] & PIXELSTATE_FILLED))) if (isWandAccepted(l->pixels[x-1+y*l->width],colorHere)) {somethingFound = true; l->pixelStates[x-1+y*l->width] |= PIXELSTATE_FILLED;}
            if (x<l->width-1&&(!(l->pixelStates[x+1+y*l->width] & PIXELSTATE_FILLED))) if (isWandAccepted(l->pixels[x+1+y*l->width],colorHere)) {somethingFound = true; l->pixelStates[x+1+y*l->width] |= PIXELSTATE_FILLED;}
            if (y>0&&(!(l->pixelStates[x+(y-1)*l->width] & PIXELSTATE_FILLED))) if (isWandAccepted(l->pixels[x+(y-1)*l->width],colorHere)) {somethingFound = true; l->pixelStates[x+(y-1)*l->width] |= PIXELSTATE_FILLED;}
            if (y<l->height-1&&(!(l->pixelStates[x+(y+1)*l->width] & PIXELSTATE_FILLED))) if (isWandAccepted(l->pixels[x+(y+1)*l->width],colorHere)) {somethingFound = true; l->pixelStates[x+(y+1)*l->width] |= PIXELSTATE_FILLED;}
          }
        }
      }
      counter++; if ((counter % 100)==0) updateStatusBar();
    }
    for (int i = 0; i < l->width*l->height; i++) if (l->pixelStates[i] & PIXELSTATE_FILLED) {
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
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
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

void useTool() {
  switch(tool) {
  case TOOL_WAND: {
    useToolWand();
    } break;
  case TOOL_MOVE: {
    useToolMove();
    } break;

  }
/*
  if (isPaintAreaActive()) {
    if (ImGui::IsMouseDown(1)) {
      Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
      int lx, ly; currentCanvas->getLayerPos(l, (int)floor(mouseX), (int)floor(mouseY), lx, ly);
      if ((unsigned int)lx<l->width&&(unsigned int)ly<l->height) {
        l->pixels[lx+ly*l->width] = getUIntColor(foregroundColor);
        addUsedColor(foregroundColor);
        shouldUpdateAll = true;
      }
    }
  }
*/
}
