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
#include "STL/IMAGE.HPP"
#include "UNDO.HPP"
#include "STL/CHARSET.HPP"
#include "STL/DELAUNTR.HPP"
#include "GALLERY.HPP"
#define __HMAP_IMPLEMENTATION__
#include "STL/HMAP.HPP"
#include <math.h>


Array<Brush *> brushes;
int currentBrush = 0;
bool hardEraser = false;

Brush *onePixelBrush = NULL;

Gradient gradient;
RGBAImage copiedTiling;

Matrix inverse2(const Matrix &a);
void tinyWait();

void initTools() {
  onePixelBrush = new Brush();
  onePixelBrush->size = 1;
}

void freeTools() {
  for (int i = 0; i < (int)brushes.size(); i++) delete brushes[i];
  brushes.clear();
  delete onePixelBrush;
  copiedTiling.free();
}

void drawText(float scale, int xp, int yp, const char *text, uint32_t color, float anchorX, float anchorY) {
  if (text == NULL) return;
  int width2 = 0;
  int width = 0;
  int height = 6;
  const char *s2 = text;
  while(*s2!=0) {
    if (*s2 == '\n') {height += 6;width2=0;}
    Letter *l = getLetter(*s2);
    width2 += l->preAddX + l->width + l->postAddX;
    if (width2 > width) width = width2;
    s2++;
    if (*s2 == 0) break;
    width2 += 1;
  }
  const char *s = text;
  int x2 = (int)(-width * anchorX);
  int y2 = (int)(-height * anchorY);
  int x3 = x2;
  while(*s!=0) {
    Letter *l = getLetter(*s);
    if (*s == '\n') {
      x2 = x3;
      y2 += 6;
      s++;
      continue;
    }
    for (int y = 0; y < l->height; y++) {
      for (int x = 0; x < l->width; x++) {
        int x3 = x2 + x + l->preAddX;
        int y3 = y2 + y + l->preAddY;
        if (l->bitmap[x+y*l->width]!='0') {
          int x0 = (int)floor(x3 * scale);
          int y0 = (int)floor(y3 * scale);
          int x1 = (int)floor((x3+1) * scale);
          int y1 = (int)floor((y3+1) * scale);
          for (int y4 = y0; y4 < y1; y4++) {
            int y5 = yp + y4;
            if ((unsigned int)y5<(unsigned int)glFrameBufferHeight) {
              int y6 = y5 * glFrameBufferWidth;
              for (int x4 = x0; x4 < x1; x4++) {
                int x5 = xp + x4;
                if ((unsigned int)x5<(unsigned int)glFrameBufferWidth) glFrameBuffer[x5+y6] = color;
              }
            }
          }
        }
      }
    }
    x2 += l->preAddX + l->width + l->postAddX + 1;
    s++;
  }
}

void textWidthHeight(const char *text, int *w, int *h) {
  if (text == NULL) return;
  int width2 = 0;
  int width = 0;
  int height = 6;
  const char *s2 = text;
  while(*s2!=0) {
    if (*s2 == '\n') {height += 6;width2=0;}
    Letter *l = getLetter(*s2);
    width2 += l->preAddX + l->width + l->postAddX;
    if (width2 > width) width = width2;
    s2++;
    if (*s2 == 0) break;
    width2 += 1;
  }
  if (w != NULL) *w = width;
  if (h != NULL) *h = height;
}

double scrollborderSize = 10;
void scrollByCanvasBorders() {
  if (mouseX < currentCanvas->screenPosX + scrollborderSize)  currentCanvas->control.moveByPixels(currentCanvas,scrollKeyPixels,0);
  if (mouseX > currentCanvas->screenPosX + currentCanvas->width - scrollborderSize)  currentCanvas->control.moveByPixels(currentCanvas,-scrollKeyPixels,0);
  if (mouseY < currentCanvas->screenPosY + scrollborderSize)  currentCanvas->control.moveByPixels(currentCanvas,0,scrollKeyPixels);
  if (mouseY > currentCanvas->screenPosY + currentCanvas->height - scrollborderSize)  currentCanvas->control.moveByPixels(currentCanvas,0,-scrollKeyPixels);
}

float sampleDistanceField(float *field, double dx, double dy, int fieldWidth, int fieldHeight) {
  if (field == NULL) return 0;
  dx = dx * 0.5 + 0.5;
  dy = dy * 0.5 + 0.5;
  dx *= fieldWidth;
  dy *= fieldHeight;
  int x = (int)floor(dx);
  int y = (int)floor(dy);
  if ((unsigned int)x >= (unsigned int)fieldWidth || (unsigned int)y >= (unsigned int)fieldHeight) return -1;
  float p00 = field[x+y*fieldWidth];
  float p10 = x+1<fieldWidth?field[x+1+y*fieldWidth]:p00;
  float p01 = y+1<fieldHeight?field[x+(y+1)*fieldWidth]:p00;
  float p11 = x+1<fieldWidth&&y+1<fieldHeight?field[x+1+(y+1)*fieldWidth]:p00;
  float xf = (float)(dx - x);
  float yf = (float)(dy - y);
  float top = (p10-p00)*xf+p00;
  float bottom = (p11-p01)*xf+p01;
  return (bottom-top)*yf+top;
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
  double o = base->ovality + 1;
  double r = -base->rotation * PI / 180.0;
  bool isSinglePixel = pixelHeight < 3 && pixelWidth < 3;
  double pressure = powd(base->pressure,3)+0.0001;
  for (int y = 0; y < pixelHeight; y++) {
    for (int x = 0; x < pixelWidth; x++) {
      double dx = (double)(x-ax+0.5) / newPixelWidth * 2.0 - 1.0;
      double dy = (double)(y-ay+0.5) / newPixelHeight * 2.0 - 1.0;
      double dx2 = cos(r)*dx - sin(r)*dy;
      dy = sin(r)*dx + cos(r)*dy;
      dx = dx2;
      dx *= xf;
      dy *= yf;
      dx *= base->aspect;
      dx = powd(dx,o);
      dy = powd(dy,o);
      double d = 1-sqrt(dx*dx+dy*dy);
      if (base->isDistanceField) {
        d = sampleDistanceField(base->distanceField,dx,dy,BRUSHDISTANCEFIELDSIZE,BRUSHDISTANCEFIELDSIZE);
      }
      double rx = fabs(dx);
      double ry = fabs(dy);
      double r = 1-maxd(rx,ry);
      d = base->edginess * (r-d) + d;
      d = powd(d,base->softness);
      d *= pressure;
      if (d > 1) d = 1;
      if (isSinglePixel) d = 1;
      if (isPencil) d = d >= 0 ? 1 : -1;
      //d *= base->opacity;
      values[x+y*pixelWidth] = (float)d;
    }
  }
  updateGLHandle(base);
}

void BrushInstance::update(Brush *base) {
  resize(base, pixelWidth, pixelHeight);
}

void BrushInstance::updateGLHandle(Brush *base) {
  if (glHandle != 0) {glDeleteTextures(1,&glHandle); glHandle = 0;}
  glGenTextures(1,&glHandle);
  glBindTexture(GL_TEXTURE_2D,glHandle);
  Array<unsigned char> temp;
  temp.resize(values.size());
  double minv = 1; double maxv = 0;
  for (int i = 0; i < (int)temp.size(); i++) {double a = values[i]; if (a<0) a = 0; if (a<minv) minv = a;if (a>maxv) maxv = a;}
  if (maxv!=minv) {
    double d = 255.0/(maxv-minv)*base->opacity;
    for (int i2 = 0; i2 < (int)temp.size(); i2++) {temp[i2] = (unsigned char)(values[i2]>=0?(values[i2]-minv)*d:0);}
  } else {
    for (int i2 = 0; i2 < (int)temp.size(); i2++) {temp[i2] = (unsigned char)(255*maxv*base->opacity);}
  }

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

__inline float scanSampleDistanceField2(const RGBAImage &image,float x,float y) {
  int px = (int)floor(x);
  int py = (int)floor(y);
  if ((unsigned int)px >= (unsigned int)image.width || (unsigned int)py >= (unsigned int)image.height) return 0;
  unsigned int col = image.data[px+py*image.width];
  int a = col >> 24;
  return a / 255.f;
}

__inline float scanSampleDistanceField(const RGBAImage &image,float x,float y, bool forward) {
  float d = scanSampleDistanceField2(image,x,y);
  if (forward) return (d < 0.5);
  return d >= 0.5;
}

__inline bool scanSampleDistanceFieldIsForward(const RGBAImage &image,float x,float y) {
  float d = scanSampleDistanceField2(image,x,y);
  return d >= 0.5;
}


void normalizeDistanceField(float *field, int w, int h) {
  float maxV = 0;
  for (int i = 0; i < w*h; i++) {
    if (field[i] > maxV) maxV = field[i];
  }
  if (maxV != 0) {
    for (int i = 0; i < w*h; i++) {
      field[i]/=maxV;
    }
  }
}

void scanSampleDistanceField(Brush *brush, const RGBAImage &image) {
  if (brush->distanceField != NULL) {
    delete[] brush->distanceField;
    brush->distanceField = NULL;
  }
  brush->distanceField = new float[BRUSHDISTANCEFIELDSIZE*BRUSHDISTANCEFIELDSIZE];
  int maxRad = (int)image.width;
  if (image.height > maxRad) maxRad = (int)image.height;
  float scale = (float)image.width / BRUSHDISTANCEFIELDSIZE;
  initStatusBar();
  for (int y = 0; y < BRUSHDISTANCEFIELDSIZE; y++) {
    updateStatusBar();
    float distanceFieldPosY = ((float)y - (float)BRUSHDISTANCEFIELDSIZE/2)*scale + (float)image.height/2;
    int dy = (int)floor(distanceFieldPosY);
    for (int x = 0; x < BRUSHDISTANCEFIELDSIZE; x++) {
      float distanceFieldPosX = ((float)x - (float)BRUSHDISTANCEFIELDSIZE/2)*scale + (float)image.width/2;
      int dx = (int)floor(distanceFieldPosX);
      float distance = -1;
      if ((unsigned int)dx<(unsigned int)image.width&&(unsigned int)dy<(unsigned int)image.height) {
        bool isForward = scanSampleDistanceFieldIsForward(image,(float)dx,(float)dy);
        for (int rad = 0; rad <= maxRad; rad++) {
          for (int x0 = -rad; x0 <= rad; x0++) {
            int y0;
            y0 = rad;
            if (scanSampleDistanceField(image,(float)(x0+dx),(float)(y0+dy),isForward)) {
              float d = (float)sqrt(x0*x0+y0*y0); if (distance < 0 || d < distance) distance = d;
            }
            y0 = -rad;
            if (scanSampleDistanceField(image,(float)(x0+dx),(float)(y0+dy),isForward)) {
              float d = (float)sqrt(x0*x0+y0*y0); if (distance < 0 || d < distance) distance = d;
            }
          }
          for (int y0 = -rad; y0 <= rad; y0++) {
            int x0;
            x0 = rad;
            if (scanSampleDistanceField(image,(float)(x0+dx),(float)(y0+dy),isForward)) {
              float d = (float)sqrt(x0*x0+y0*y0); if (distance < 0 || d < distance) distance = d;
            }
            x0 = -rad;
            if (scanSampleDistanceField(image,(float)(x0+dx),(float)(y0+dy),isForward)) {
              float d = (float)sqrt(x0*x0+y0*y0); if (distance < 0 || d < distance) distance = d;
            }
          }
          if (distance >= 0) break;
        } 
        if (!isForward) distance *= -1;
        brush->distanceField[x+y*BRUSHDISTANCEFIELDSIZE] = distance;
      } else {
        brush->distanceField[x+y*BRUSHDISTANCEFIELDSIZE] = -1;
      }
    }
  }
  normalizeDistanceField(brush->distanceField,BRUSHDISTANCEFIELDSIZE,BRUSHDISTANCEFIELDSIZE);
}

__inline bool rightRange(Gradient *gradient, int index, float value) {
  if ((unsigned int)index >= gradient->elements.size()) return false;
  bool left = value >= gradient->elements[index].position;
  bool right = index + 1 < (int)gradient->elements.size() ? value < gradient->elements[index+1].position : false;
  return left && right;
}

__inline unsigned int interpolateRange(Gradient *gradient, int index, float value) {
  if (gradient->elements.empty()) return 0x00000000;
  if (index < 0) return gradient->elements[0].color;
  if (index >= (int)gradient->elements.size()) return gradient->elements.back().color;
  GradientElement *e0 = &gradient->elements[index];
  if (e0->isStep) return e0->color;
  if (index + 1 >= (int)gradient->elements.size()) return e0->color;
  GradientElement *e1 = &gradient->elements[index+1];
  float d = e1->position-e0->position;
  if (d <= 0) return e0->color;
  float k = (value - e0->position) / d;
  if (k < 0) k = 0;
  if (k > 1) k = 1;
  unsigned int col0 = e0->color;
  unsigned int col1 = e1->color;
  int r0 = col0 & 255;
  int g0 = (col0>>8) & 255;
  int b0 = (col0>>16) & 255;
  int a0 = (col0>>24) & 255;
  int r1 = col1 & 255;
  int g1 = (col1>>8) & 255;
  int b1 = (col1>>16) & 255;
  int a1 = (col1>>24) & 255;
  int r = (int)((int)(r1 - r0) * k + r0);
  int g = (int)((int)(g1 - g0) * k + g0);
  int b = (int)((int)(b1 - b0) * k + b0);
  int a = (int)((int)(a1 - a0) * k + a0);
  return r|(g<<8)|(b<<16)|(a<<24);
}

unsigned int Gradient::getColor(float between0And1) {
  if (elements.empty()) return 0x00000000;
  if (between0And1 <= 0) return elements[0].color;
  if (between0And1 >= 1) return elements.back().color;
  if ((unsigned int)lastAccessPoint>=elements.size()) lastAccessPoint = 0;
  float posToLookUp = between0And1 * (positionMax-positionMin) + positionMin;
  if (rightRange(this, lastAccessPoint, posToLookUp)) {
    return interpolateRange(this, lastAccessPoint, posToLookUp);
  }
  if (posToLookUp > elements[lastAccessPoint].position) {
    for (; lastAccessPoint < (int)elements.size(); lastAccessPoint++) {
      if (rightRange(this, lastAccessPoint, posToLookUp)) {
        return interpolateRange(this, lastAccessPoint, posToLookUp);
      }
    }
    return elements.back().color;
  } 
  for (; lastAccessPoint >= 0; lastAccessPoint--) {
    if (rightRange(this, lastAccessPoint, posToLookUp)) {
      return interpolateRange(this, lastAccessPoint, posToLookUp);
    }
  }
  lastAccessPoint = 0;
  return elements[0].color;
}

unsigned int Gradient::getColor2D(float dx,float dy) {
  float value = dx;
  if (round) {
    value = (float)sqrt(dx*dx+dy*dy);
  } else {
    if (value < 0) value = 0;
  }
  if (doubleSided) {if (value<=0.5) value *= 2; else value = (1-value)*2;}
  if (inverted) value = 1 - value;
  if (value < 0) value = 0;
  if (value > 1) value = 1;
  return getColor(value);
}

int gradientSort(const void *p, const void *q)
{
  const GradientElement* a = (const GradientElement*)p;
  const GradientElement* b = (const GradientElement*)q;

  if (a->position < b->position) return -1;
  if (a->position > b->position) return  1;
  if (a->id > b->id) return -1;
  if (a->id < b->id) return 1;
  return 0;
}

void Gradient::sort() {
  positionMin = 0;
  positionMax = 0;
  if (elements.empty()) return;
  qsort(&elements[0], elements.size(), sizeof(GradientElement), gradientSort);
  positionMin = elements[0].position;
  positionMax = elements.back().position;
}

void Gradient::update() {
  sort();
}

bool isPaintAreaActive() {
  return (!ImGui::GetIO().WantCaptureMouse) && (!ImGui::GetIO().WantCaptureKeyboard) && (!galleryVisible);
}

bool mirrorToolEnabled = false;
bool mirrorToolSideA = true;
float mirrorToolAngle = 0;
Vector mirrorToolPoint(0,0);

Array<unsigned short> quantization;

void reCreateQuantization() {
  if (palette.size()>256 || palette.empty()) {quantization.clear(); return;}
  if (quantization.empty()) quantization.resize(64*64*64);
  Array<unsigned int> colors; colors.resize(palette.size());
  for (int i = 0; i < (int)palette.size(); i++) colors[i] = getUIntColor(palette[i]);
  for (int r = 0; r < 64; r++) {
    int rs = r * 4;
    for (int g = 0; g < 64; g++) {
      int gs = g * 4;
      for (int b = 0; b < 64; b++) {
        int bs = b * 4;
        int found = 0;
        double ld = 256*256*5*2*2;
        for (int i = 0; i < (int)palette.size(); i++) {
          unsigned int col = colors[i];
          double rd = (double)(col & 255)-rs;
          double gd = (double)((col>>8) & 255)-gs;
          double bd = (double)((col>>16) & 255)-bs;
          if (fastQuantWithLuma) {
            rd *= luminanceValues[0];
            gd *= luminanceValues[1];
            bd *= luminanceValues[2];
          }
          double dist = rd*rd+gd*gd+bd*bd;
          if (dist < ld) {
            ld = dist;
            found = i;
          }
        }
        quantization[r+g*64+b*64*64] = (unsigned short)found;
      }
    }
  }

  for (int z = 0; z < 64-1; z++) {
    for (int y = 0; y < 64-1; y++) {
      for (int x = 0; x < 64-1; x++) {
        int here = quantization[x+y*64+z*64*64] & 255;
        bool found = false;
        for (int z2 = 0; z2 < 2; z2++) {
          for (int y2 = 0; y2 < 2; y2++) {
            for (int x2 = 0; x2 < 2; x2++) {
              int there = quantization[(x+x2)+(y+y2)*64+(z+z2)*64*64] & 255;
              if (here != there) {found = true;}
            }
          }
        }
        if (found) quantization[x+y*64+z*64*64] |= 0x01<<8;
      }
    }
  }

}

bool fastQuantWithLuma = false;
bool rulersOn = false;

bool watershed = false;
int watershedSelectedPoint = -1;
bool watershedSelectionOutside = false;
bool watershedSelectionInside = true;
Array<WatershedPoint> watershedPoints;


void moveSelectedLayersByCanvasPixels(double x, double y) {
  bool one = false;
  for (int i2 = 0; i2 < (int)currentCanvas->layers.size(); i2++) {
    Layer *l = currentCanvas->layers[i2];
    if (l->state.selected) {one = true; break;}
  }

  for (int i = 0; i < (int)currentCanvas->layers.size(); i++) {
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

bool isWandAccepted(Layer *l,unsigned int destColor, unsigned int checkColor) {
  if (l->type == LAYERTYPE_TILEMAP) return destColor == checkColor;
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

void useSubToolWatershed() {
  if (!isPaintAreaActive()) return;

  if (ImGui::IsMouseClicked(0)||ImGui::IsMouseClicked(1)) {
    double cx = mouseX, cy = mouseY; currentCanvas->getCanvasPos(cx, cy, cx, cy);
    WatershedPoint point;
    point.x = cx;
    point.y = cy;
    point.selection = ImGui::IsMouseClicked(0);
    point.label = (unsigned int)watershedPoints.size() + 1;
    watershedPoints.push_back(point);
  }
}

void useToolWand() {

  if (watershed) {
    useSubToolWatershed();
    return;
  }

  if (!isPaintAreaActive()) return;

  if (ImGui::IsMouseClicked(1) && isPaintAreaActive()) {foregroundColor = getImVec4Color(currentCanvas->getLayerColor(mouseX, mouseY,false)); addUsedColor(foregroundColor);}

  if (!ImGui::IsMouseClicked(0)) return;

  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return; if (locked(l,LOCK_SELECTION)) return;

  beforeUndoPoint();

  int lx, ly; double lx2, ly2; currentCanvas->getLayerPos(l, mouseX, mouseY, lx2, ly2);
  if (l->type == LAYERTYPE_TILEMAP) {Layer *l2 = currentCanvas->getLayerById(l->tileSourceLayerId); if (l2 != NULL) {lx2 /= l2->toolInfo.tilingWidth; ly2 /= l2->toolInfo.tilingHeight;}}
  lx = (int)floor(lx2); ly = (int)floor(ly2);
  if ((unsigned int)lx>=(unsigned int)l->width||(unsigned int)ly>=(unsigned int)l->height) return;
  unsigned int colorHere = l->pixels[lx+ly*l->width];
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
            if (x>0&&(!(l->pixelStates[x-1+y*l->width] & PIXELSTATE_TEMP))) if (isWandAccepted(l,l->pixels[x-1+y*l->width],colorHere)) {somethingFound = true; l->pixelStates[x-1+y*l->width] |= PIXELSTATE_TEMP;}
            if (x<l->width-1&&(!(l->pixelStates[x+1+y*l->width] & PIXELSTATE_TEMP))) if (isWandAccepted(l,l->pixels[x+1+y*l->width],colorHere)) {somethingFound = true; l->pixelStates[x+1+y*l->width] |= PIXELSTATE_TEMP;}
            if (y>0&&(!(l->pixelStates[x+(y-1)*l->width] & PIXELSTATE_TEMP))) if (isWandAccepted(l,l->pixels[x+(y-1)*l->width],colorHere)) {somethingFound = true; l->pixelStates[x+(y-1)*l->width] |= PIXELSTATE_TEMP;}
            if (y<l->height-1&&(!(l->pixelStates[x+(y+1)*l->width] & PIXELSTATE_TEMP))) if (isWandAccepted(l,l->pixels[x+(y+1)*l->width],colorHere)) {somethingFound = true; l->pixelStates[x+(y+1)*l->width] |= PIXELSTATE_TEMP;}
          }
        }
      }
      counter++; if ((counter % 100)==0) updateStatusBar();
    }
    for (int i = 0; i < l->width*l->height; i++) {
      if (controlKeyId == CONTROLKEY_NONE) l->pixelStates[i] &= ~PIXELSTATE_SELECTED;
      if (l->pixelStates[i] & PIXELSTATE_TEMP) {
        if (controlKeyId == CONTROLKEY_NONE || controlKeyId == CONTROLKEY_SHIFT) l->pixelStates[i] |= PIXELSTATE_SELECTED;
        if (controlKeyId == CONTROLKEY_CTRL) l->pixelStates[i] &= ~PIXELSTATE_SELECTED;
      }
      if (controlKeyId == (CONTROLKEY_SHIFT|CONTROLKEY_CTRL)) {
        if ((l->pixelStates[i] & PIXELSTATE_TEMP)&&(l->pixelStates[i] & PIXELSTATE_SELECTED)) l->pixelStates[i] |= PIXELSTATE_SELECTED; else l->pixelStates[i] &= ~PIXELSTATE_SELECTED;
      }
    }
  } else {
    for (int i = 0; i < l->width*l->height; i++) {bool yes = isWandAccepted(l,l->pixels[i],colorHere); if (yes) {
        if (controlKeyId == CONTROLKEY_NONE) l->pixelStates[i] &= ~PIXELSTATE_SELECTED;
        if (controlKeyId == CONTROLKEY_NONE || controlKeyId == CONTROLKEY_SHIFT) l->pixelStates[i] |= PIXELSTATE_SELECTED;
        if (controlKeyId == CONTROLKEY_CTRL) l->pixelStates[i] &= ~PIXELSTATE_SELECTED;
      }
      if (controlKeyId == (CONTROLKEY_SHIFT|CONTROLKEY_CTRL)) {
        if (yes && (l->pixelStates[i] & PIXELSTATE_SELECTED)) l->pixelStates[i] |= PIXELSTATE_SELECTED; else l->pixelStates[i] &= ~PIXELSTATE_SELECTED;
      }
    }
  }

  repaintLayer(l,true);
  updatedPixelStates(l);
  afterUndoPoint();
}

void placeCorner(double mouseX, double mouseY) {
  beforeUndoPoint();
  Array<Layer *> selected;
  for (int i = 0; i < (int)currentCanvas->layers.size(); i++) {if (currentCanvas->layers[i]->state.selected||currentCanvas->layers[i]->state.active) selected.push_back(currentCanvas->layers[i]);}
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
    int cx2 = (int)(cx + 1);
    int cy2 = (int)(cy + 1);
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
    int cx2 = (int)(cx + 1);
    int cy2 = (int)(cy + 1);
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
  afterUndoPoint();
}

void mouseClickUndo() {
  static unsigned int counter = 3;
  if (ImGui::IsMouseDown(0)||ImGui::IsMouseDown(1)) {
    if (isPaintAreaActive()) {
      if (counter != 0) beforeUndoPoint();
      counter = 0;
    }
  } else {
    counter++;
    if (counter == 2) afterUndoPoint();
  }
}

void useToolMove() {

  if (!ImGui::GetIO().WantCaptureKeyboard) {
    double moveX = 1, moveY = 1;
    if (shift) {
      moveX=currentCanvas->gridStepX;
      moveY=currentCanvas->gridStepY;
    }
    double x = 0, y = 0;
    switch(currentKey) {
      case '4': {x=-moveX;} break;
      case '6': {x=+moveX;} break;
      case '8': {y=-moveY;} break;
      case '2': {y=+moveY;} break;
      case '7': {x=-moveX;y=-moveY;} break;
      case '1': {x=-moveX;y=+moveY;} break;
      case '9': {x=+moveX;y=-moveY;} break;
      case '3': {x=+moveX;y=+moveY;} break;
    }
    if (x != 0 || y != 0) moveSelectedLayersByCanvasPixels(x,y);
  }

  if (!isPaintAreaActive()) return;

  mouseClickUndo();

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
      if (!lastRight) {
        centerX = (double)l->width/2.0; centerY = (double)l->height/2.0; lastAngle = -1;
        if (l->type == LAYERTYPE_TILEMAP) {
          Layer *l2 = currentCanvas->getLayerById(l->tileSourceLayerId);
          if (l2 != NULL) {
            centerX *= l2->toolInfo.tilingWidth;
            centerY *= l2->toolInfo.tilingHeight;
          }
        }
      }
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
}

bool pipetteFromLayer = false;
bool pipetteWithColorTransform = false;

void useToolPipette() {
  if (!isPaintAreaActive()) return;
  if ( (!ImGui::IsMouseDown(0)) && (!ImGui::IsMouseDown(1)) ) return;
  if (!pipetteFromLayer) {
    foregroundColor = getImVec4Color(currentCanvas->getCanvasColor(mouseX,mouseY));
    addUsedColor(foregroundColor);
  } else {
    foregroundColor = getImVec4Color(currentCanvas->getLayerColor(mouseX,mouseY,pipetteWithColorTransform));
    addUsedColor(foregroundColor);
  }
}

double fillColorDistance = 1;
bool fillAcceptedHasSelection = false;
bool fillUseValleys = false;

bool isFillAccepted(unsigned int here, unsigned int destColor, unsigned int checkColor, unsigned char destPixelState) {
  if (fillAcceptedHasSelection && (!(destPixelState & PIXELSTATE_SELECTED))) return false;
  int dr = destColor & 255;
  int dg = (destColor>>8) & 255;
  int db = (destColor>>16) & 255;
  int da = (destColor>>24) & 255;
  if (fillUseValleys) checkColor = here;
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

bool isFillAccepted1(unsigned int here, unsigned int destColor, unsigned int checkColor, unsigned char destPixelState) {
  UNUSED(here);
  if (fillAcceptedHasSelection && (!(destPixelState & PIXELSTATE_SELECTED))) return false;
  return destColor == checkColor;
}

void useToolFill() {
  if (!isPaintAreaActive()) return;

  mouseClickUndo();

  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return; if (locked(l,LOCK_PIXELS_AND_TILEMAP)) return;
  int lx, ly; double lx2, ly2; currentCanvas->getLayerPos(l, mouseX, mouseY, lx2, ly2); lx = (int)floor(lx2); ly = (int)floor(ly2);

  bool tiling = false;
  if (l->type == LAYERTYPE_TILEMAP) {
    Layer *l2 = currentCanvas->getLayerById(l->tileSourceLayerId);
    if (l2 != NULL && l2->toolInfo.tilingWidth > 0 && l2->toolInfo.tilingHeight > 0) {
      lx /= l2->toolInfo.tilingWidth;
      ly /= l2->toolInfo.tilingHeight;
      tiling = true;
    }
  }

  if ((unsigned int)lx>=(unsigned int)l->width||(unsigned int)ly>=(unsigned int)l->height) return;

  unsigned int clickBack = currentCanvas->getLayerColor(mouseX, mouseY,false);
  if (ImGui::IsMouseClicked(1)) {
    if (!tiling) {
      foregroundColor = getImVec4Color(clickBack); addUsedColor(foregroundColor);
    } else l->toolInfo.tileMapSelectedTile = clickBack;
  }

  if (!ImGui::IsMouseClicked(0)) return;

  int colorHere = l->pixels[lx+ly*l->width];
  
  fillAcceptedHasSelection = l->hasSelection();

  if (fillAcceptedHasSelection) {
    if ((l->pixelStates[lx+ly*l->width] & PIXELSTATE_SELECTED) == 0) return;
  }

  initStatusBar();
  for (int i2 = 0; i2 < l->width*l->height; i2++) l->pixelStates[i2] &= ~PIXELSTATE_TEMP;
  l->pixelStates[lx+ly*l->width] |= PIXELSTATE_TEMP;
  bool somethingFound = true;
  int counter = 0;

  if (!tiling) {
    while(somethingFound) {
      somethingFound = false;
      for (int y = 0; y < l->height; y++) {
        for (int x = 0; x < l->width; x++) {
          if (l->pixelStates[x+y*l->width] & PIXELSTATE_TEMP) {
            unsigned int c0 = l->pixels[x+y*l->width];
            if (x>0&&(!(l->pixelStates[x-1+y*l->width] & PIXELSTATE_TEMP))) if (isFillAccepted(c0,l->pixels[x-1+y*l->width],colorHere,l->pixelStates[x-1+y*l->width])) {somethingFound = true; l->pixelStates[x-1+y*l->width] |= PIXELSTATE_TEMP;}
            if (x<l->width-1&&(!(l->pixelStates[x+1+y*l->width] & PIXELSTATE_TEMP))) if (isFillAccepted(c0,l->pixels[x+1+y*l->width],colorHere,l->pixelStates[x+1+y*l->width])) {somethingFound = true; l->pixelStates[x+1+y*l->width] |= PIXELSTATE_TEMP;}
            if (y>0&&(!(l->pixelStates[x+(y-1)*l->width] & PIXELSTATE_TEMP))) if (isFillAccepted(c0,l->pixels[x+(y-1)*l->width],colorHere,l->pixelStates[x+(y-1)*l->width])) {somethingFound = true; l->pixelStates[x+(y-1)*l->width] |= PIXELSTATE_TEMP;}
            if (y<l->height-1&&(!(l->pixelStates[x+(y+1)*l->width] & PIXELSTATE_TEMP))) if (isFillAccepted(c0,l->pixels[x+(y+1)*l->width],colorHere,l->pixelStates[x+(y+1)*l->width])) {somethingFound = true; l->pixelStates[x+(y+1)*l->width] |= PIXELSTATE_TEMP;}
          }
        }
      }
      counter++; if ((counter % 100)==0) updateStatusBar();
    }
    for (int i = 0; i < l->width*l->height; i++) if (l->pixelStates[i] & PIXELSTATE_TEMP) {
      l->pixels[i] = getUIntColor(foregroundColor);
    }
    addUsedColor(foregroundColor);
  } else {
    while(somethingFound) {
      somethingFound = false;
      for (int y = 0; y < l->height; y++) {
        for (int x = 0; x < l->width; x++) {
          if (l->pixelStates[x+y*l->width] & PIXELSTATE_TEMP) {
            unsigned int c0 = l->pixels[x+y*l->width];
            if (x>0&&(!(l->pixelStates[x-1+y*l->width] & PIXELSTATE_TEMP))) if (isFillAccepted1(c0,l->pixels[x-1+y*l->width],colorHere,l->pixelStates[x-1+y*l->width])) {somethingFound = true; l->pixelStates[x-1+y*l->width] |= PIXELSTATE_TEMP;}
            if (x<l->width-1&&(!(l->pixelStates[x+1+y*l->width] & PIXELSTATE_TEMP))) if (isFillAccepted1(c0,l->pixels[x+1+y*l->width],colorHere,l->pixelStates[x+1+y*l->width])) {somethingFound = true; l->pixelStates[x+1+y*l->width] |= PIXELSTATE_TEMP;}
            if (y>0&&(!(l->pixelStates[x+(y-1)*l->width] & PIXELSTATE_TEMP))) if (isFillAccepted1(c0,l->pixels[x+(y-1)*l->width],colorHere,l->pixelStates[x+(y-1)*l->width])) {somethingFound = true; l->pixelStates[x+(y-1)*l->width] |= PIXELSTATE_TEMP;}
            if (y<l->height-1&&(!(l->pixelStates[x+(y+1)*l->width] & PIXELSTATE_TEMP))) if (isFillAccepted1(c0,l->pixels[x+(y+1)*l->width],colorHere,l->pixelStates[x+(y+1)*l->width])) {somethingFound = true; l->pixelStates[x+(y+1)*l->width] |= PIXELSTATE_TEMP;}
          }
        }
      }
      counter++; if ((counter % 100)==0) updateStatusBar();
    }
    for (int i = 0; i < l->width*l->height; i++) if (l->pixelStates[i] & PIXELSTATE_TEMP) {
      l->pixels[i] = l->toolInfo.tileMapSelectedTile;
    }
  }

  repaintLayer(l,true);
  updatedPixels(l);
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

double toolAspectX = 1;
double toolAspectY = 1;

int startControlKeyId = CONTROLKEY_NONE;

bool selectionToolMoveActive = false;
double selectionToolMoveStartX = 0;
double selectionToolMoveStartY = 0;

bool useToolSelectionMove(Layer *activeLayer) {

  if (!ImGui::IsMouseDown(1)) { // also handles moveByKey
    showSelectionPlane = false; showSelectionPlaneBounding = false;
    if (selectionToolMoveActive) {
      selectionToolMoveActive = false;
      int c = startControlKeyId;
      for (int i = 0; i < activeLayer->width*activeLayer->height; i++) {
        if (c == CONTROLKEY_NONE) activeLayer->pixelStates[i] &= ~PIXELSTATE_SELECTED;
        if (selectionPlane->pixels[i]!=0) {
          if (c == CONTROLKEY_NONE || c == CONTROLKEY_SHIFT) activeLayer->pixelStates[i] |= PIXELSTATE_SELECTED;
          if (c == CONTROLKEY_CTRL) activeLayer->pixelStates[i] &= ~PIXELSTATE_SELECTED;
        }
        if (c == (CONTROLKEY_CTRL|CONTROLKEY_SHIFT)) {
          if (selectionPlane->pixels[i]!=0&&(activeLayer->pixelStates[i] & PIXELSTATE_SELECTED)) activeLayer->pixelStates[i] |= PIXELSTATE_SELECTED; else activeLayer->pixelStates[i] &= ~PIXELSTATE_SELECTED;
        }
      }
      updatedPixelStates(activeLayer);
      shouldUpdateAll = true;
    } 
  }

  double x = 0, y = 0, moveX = 1, moveY = 1;
  switch(currentKey) {
    case '4': {x=-moveX;} break;
    case '6': {x=+moveX;} break;
    case '8': {y=-moveY;} break;
    case '2': {y=+moveY;} break;
    case '7': {x=-moveX;y=-moveY;} break;
    case '1': {x=-moveX;y=+moveY;} break;
    case '9': {x=+moveX;y=-moveY;} break;
    case '3': {x=+moveX;y=+moveY;} break;
  }
  bool moveByKey = x != 0 || y != 0;

  double lx,ly; currentCanvas->getLayerPos(activeLayer,mouseX,mouseY,lx,ly);
  if (isPaintAreaActive() && (ImGui::IsMouseDown(1)||moveByKey)) {
    if (!selectionToolMoveActive) {
      startControlKeyId = controlKeyId;
      selectionPlane->init(activeLayer->width, activeLayer->height);
      selectionPlane->transform = activeLayer->transform;
      selectionPlane->type = activeLayer->type;
      selectionPlane->tileSourceLayerId = activeLayer->tileSourceLayerId;
      selectionToolMoveActive  = true;
      selectionToolMoveStartX = lx;
      selectionToolMoveStartY = ly;
    }
  }

  if (moveByKey) {
    lx += x;
    ly += y;
  }

  bool tiling = false; int tw = 1, th = 1;
  if (selectionPlane->type == LAYERTYPE_TILEMAP) {
    Layer *l2 = currentCanvas->getLayerById(selectionPlane->tileSourceLayerId);
    if (l2 != NULL) {
      tw = l2->toolInfo.tilingWidth;
      th = l2->toolInfo.tilingHeight;
      if (tw>0&&th>0) tiling = true;
    }
  }

  if (selectionToolMoveActive || moveByKey) {
    showSelectionPlane = true;
    if (activeLayer->width == selectionPlane->width && activeLayer->height == selectionPlane->height) {
      int deltaX = (int)(selectionToolMoveStartX-lx);
      int deltaY = (int)(selectionToolMoveStartY-ly);
      if (tiling) {
        deltaX /= tw;
        deltaY /= th;
      }
      for (int y = 0; y < activeLayer->height; y++) {
        for (int x = 0; x < activeLayer->width; x++) {
          bool selected = false;
          int x2 = x + deltaX;
          int y2 = y + deltaY;
          if ((unsigned int)x2 < (unsigned int)activeLayer->width && (unsigned int)y2 < (unsigned int)activeLayer->height) {
            unsigned char p = activeLayer->pixelStates[x2+y2*activeLayer->width];
            if (p & PIXELSTATE_SELECTED) selected = true;
          }
          selectionPlane->pixels[x+y*selectionPlane->width] = selected ? 1 : 0;
        }
      }
    }
  }

  return selectionToolMoveActive;
}

void useToolSelection() {
  if (wasActiveLayerSwitched()) {selectionToolActive = false; selectionToolMoveActive = false; showSelectionPlane = false; showSelectionPlaneBounding = false; toolW = -1; toolH = -1; return;}

  bool polygonSpanning = selectionTool == SELECTIONTOOL_POLYGON && selectionToolActive;
  if (ImGui::IsMouseClicked(1) && isPaintAreaActive() && (!polygonSpanning)) {foregroundColor = getImVec4Color(currentCanvas->getLayerColor(mouseX, mouseY,false)); addUsedColor(foregroundColor);}

  Layer *activeLayer = currentCanvas->getActiveLayer(); 
  if (activeLayer == NULL) return; 
  if (locked(activeLayer,LOCK_SELECTION)) return;

  static Layer *selectionToolLayer = activeLayer;
  selectionToolLayer = activeLayer;
  if (selectionToolLayer != NULL && selectionToolLayer->width * selectionToolLayer->height==0) return;

  mouseClickUndo();

  if ((!selectionToolActive) && useToolSelectionMove(activeLayer)) return; // move the selection

  bool tiling = false; int tw = 1, th = 1;
  if (selectionPlane->type == LAYERTYPE_TILEMAP) {
    Layer *l2 = currentCanvas->getLayerById(selectionPlane->tileSourceLayerId);
    if (l2 != NULL) {
      tw = l2->toolInfo.tilingWidth;
      th = l2->toolInfo.tilingHeight;
      if (tw>0&&th>0) tiling = true;
    }
  }

  int selectionTool2 = tool == TOOL_OVERLAYS ? SELECTIONTOOL_RECTANGLE : selectionTool;
  bool selectionProportional2 = false;
  switch(selectionTool) {
    case SELECTIONTOOL_ELLIPSE: 
    case SELECTIONTOOL_RECTANGLE: {selectionProportional2 = tool == TOOL_OVERLAYS ? false : selectionProportional;} break;
  }

#define XEORLINE 1
#define EORLINE 2
#define SELECTEDLINE 4

  if (selectionToolLayer == NULL ||  (!ImGui::IsMouseDown(0)) || (ImGui::IsMouseDown(1) && selectionTool2 != SELECTIONTOOL_POLYGON)) {
    if (selectionToolActive) {
      selectionToolActive = false; showSelectionPlane = false; showSelectionPlaneBounding = false; toolW = -1; toolH = -1;
      if (!ImGui::IsMouseDown(1)) {
        int c = startControlKeyId;
        switch(selectionTool2) {
          case SELECTIONTOOL_ELLIPSE:
          case SELECTIONTOOL_RECTANGLE: {
            for (int i = 0; i < activeLayer->width*activeLayer->height; i++) {
              if (c == CONTROLKEY_NONE) activeLayer->pixelStates[i] &= ~PIXELSTATE_SELECTED;
              if (selectionPlane->pixels[i]!=0) {
                if (c == CONTROLKEY_NONE || c == CONTROLKEY_SHIFT) activeLayer->pixelStates[i] |= PIXELSTATE_SELECTED;
                if (c == CONTROLKEY_CTRL) activeLayer->pixelStates[i] &= ~PIXELSTATE_SELECTED;
              }
              if (c == (CONTROLKEY_CTRL|CONTROLKEY_SHIFT)) {
                if (selectionPlane->pixels[i]!=0&&(activeLayer->pixelStates[i] & PIXELSTATE_SELECTED)) activeLayer->pixelStates[i] |= PIXELSTATE_SELECTED; else activeLayer->pixelStates[i] &= ~PIXELSTATE_SELECTED;
              }
            }
            updatedPixelStates(activeLayer);
            shouldUpdateAll = true;
          } break;
          case SELECTIONTOOL_POLYGON:
          case SELECTIONTOOL_LASSO: {
            double lx0,ly0; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,selectionX0,selectionY0,lx0,ly0);
            double lx1,ly1; 
            currentCanvas->getLayerPosFromCanvasPos(selectionPlane,selectionX1,selectionY1,lx1,ly1);
            if (tiling) {
              lx0 = lx0/tw; ly0 = ly0/th;
              lx1 = lx1/tw; ly1 = ly1/th;
            }
            if (selectionTool2 == SELECTIONTOOL_LASSO) selectionPlane->pixelXLineEor((int)lx0, (int)ly0, (int)lx1,(int)ly1,1,true);
            for (int y = 0; y < selectionPlane->height; y++) {
              bool isInSelection = false;
              for (int x = 0; x < selectionPlane->width; x++) {
                bool edge = selectionPlane->pixels[x+y*selectionPlane->width] & XEORLINE ? true : false;
                bool edge2 = selectionPlane->pixels[x+y*selectionPlane->width] & EORLINE ? true : false;
                if (x == 0) {isInSelection = edge;edge = false;}
                if (isInSelection||edge||edge2) selectionPlane->pixels[x+y*selectionPlane->width] = SELECTEDLINE;
                if (edge) {isInSelection = !isInSelection;}
              }
            }

            for (int i = 0; i < activeLayer->width*activeLayer->height; i++) {
              if (c == CONTROLKEY_NONE) activeLayer->pixelStates[i] &= ~PIXELSTATE_SELECTED;
              if (selectionPlane->pixels[i]==SELECTEDLINE) {
                if (c == CONTROLKEY_NONE || c == CONTROLKEY_SHIFT) activeLayer->pixelStates[i] |= PIXELSTATE_SELECTED;
                if (c == CONTROLKEY_CTRL) activeLayer->pixelStates[i] &= ~PIXELSTATE_SELECTED;
              }
              if (c == (CONTROLKEY_CTRL|CONTROLKEY_SHIFT)) {
                if (selectionPlane->pixels[i]==SELECTEDLINE&&(activeLayer->pixelStates[i] & PIXELSTATE_SELECTED)) activeLayer->pixelStates[i] |= PIXELSTATE_SELECTED; else activeLayer->pixelStates[i] &= ~PIXELSTATE_SELECTED;
              }
            }
            updatedPixelStates(activeLayer);
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
      startControlKeyId = controlKeyId;
      selectionPlane->init(activeLayer->width, activeLayer->height);
      selectionPlane->transform = activeLayer->transform;
      selectionPlane->type = activeLayer->type;
      selectionPlane->tileSourceLayerId = activeLayer->tileSourceLayerId;
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
    double pixelAspect = currentCanvas->control.pixelSizeX<=0||currentCanvas->control.pixelSizeY<=0?1:currentCanvas->control.pixelSizeX/currentCanvas->control.pixelSizeY;
    if (selectionProportional2) {
      if (lx-selectionX0>ly-selectionY0) {
        selectionY1 = selectionY0+(lx-selectionX0)*pixelAspect; 
      } else {
        selectionX1 = selectionX0+(ly-selectionY0)/pixelAspect;
      }
    }
    scrollByCanvasBorders();
    double x0 = selectionX0 < selectionX1 ? selectionX0 : selectionX1;
    double y0 = selectionY0 < selectionY1 ? selectionY0 : selectionY1;
    double x1 = selectionX0 < selectionX1 ? selectionX1 : selectionX0;
    double y1 = selectionY0 < selectionY1 ? selectionY1 : selectionY0;
    x0 = floor(x0);
    y0 = floor(y0);
    x1 = floor(x1)+1;
    y1 = floor(y1)+1;
    selectionToolActive = true;
    showSelectionPlane = true;
    showSelectionPlaneBounding = selectionTool2 == SELECTIONTOOL_ELLIPSE;

    switch(selectionTool2) {
      case SELECTIONTOOL_ELLIPSE:
      case SELECTIONTOOL_RECTANGLE: {
        toolW = x1 - x0;
        toolH = y1 - y0;
        selectionPlane->clear();
        Matrix m = inverse2(selectionPlane->transform.transformation);
        Vector b = currentCanvas->getLayerPixelBounding(selectionPlane,x0,y0,x1,y1);
        int ox=1,oy=1;
        if (tiling) {
          //x0 = floor(x0/tw);
          //y0 = floor(y0/th);
          //x1 = floor((x1-1)/tw)+1;
          //y1 = floor((y1-1)/th)+1;
          ox = tw < 1 ? 1 : tw; oy = th < 1 ? 1 : th;
          b.x *= ox; b.z *= ox;
          b.y *= oy; b.w *= oy;
        }
        for (int yl = (int)b.y; yl < (int)b.w; yl++) {
          for (int xl = (int)b.x; xl < (int)b.z; xl++) {
            double x2 = xl * m.m[0*4+0] + yl * m.m[1*4+0] + m.m[3*4+0];
            double y2 = xl * m.m[0*4+1] + yl * m.m[1*4+1] + m.m[3*4+1];
            int x = (int)x2;
            int y = (int)y2;
            if (x == 0) x = (int)floor(x2);
            if (y == 0) y = (int)floor(y2);
            if (x >= x0 && x < x1 && y >= y0 && y < y1) {
              if (selectionTool2 == SELECTIONTOOL_ELLIPSE) {
                double dx = (double)(x-x0+0.5)/(x1-x0)*2.0-1.0;
                double dy = (double)(y-y0+0.5)/(y1-y0)*2.0-1.0;
                double d = sqrt(dx*dx+dy*dy);
                if (d > 1) continue;
              }
              int xl2 = xl / ox;
              int yl2 = yl / oy;
              if ((unsigned int)xl2 < (unsigned int)selectionPlane->width && (unsigned int)yl2 < (unsigned int)selectionPlane->height)
                selectionPlane->pixels[xl2+yl2*selectionPlane->width] = 1;
            }
          }
        }
      } break;
      case SELECTIONTOOL_LASSO: {
        double lx0,ly0; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,lastSelectionX1,lastSelectionY1,lx0,ly0);
        double lx1,ly1; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,selectionX1,selectionY1,lx1,ly1);
        if (tiling) {
          lx0 = lx0/tw; ly0 = ly0/th;
          lx1 = lx1/tw; ly1 = ly1/th;
        }
        selectionPlane->pixelLineEor((int)lx0, (int)ly0, (int)lx1, (int)ly1,EORLINE,true);
        selectionPlane->pixelXLineEor((int)lx0, (int)ly0, (int)lx1, (int)ly1,XEORLINE,true);
      } break;
      case SELECTIONTOOL_POLYGON: {
        double lx0,ly0; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,lastSelectionPolygonX1,lastSelectionPolygonY1,lx0,ly0);
        double lx1,ly1; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,selectionX1,selectionY1,lx1,ly1);
        double lx2,ly2; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,lastSelectionX1,lastSelectionY1,lx2,ly2);
        double lx3,ly3; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,selectionX0,selectionY0,lx3,ly3);
        if (tiling) {
          lx0 = lx0/tw; ly0 = ly0/th;
          lx1 = lx1/tw; ly1 = ly1/th;
          lx2 = lx2/tw; ly2 = ly2/th;
          lx3 = lx3/tw; ly3 = ly3/th;
        }
        if (ImGui::IsMouseDown(1)) {
          if (pointCount == 0) {selectionPlane->pixelXLineEor((int)lx0, (int)ly0, (int)lx2, (int)ly2,XEORLINE,true); selectionPlane->pixelLineEor((int)lx0, (int)ly0, (int)lx2, (int)ly2,EORLINE,true);}
          lastSelectionPolygonX1 = lx;
          lastSelectionPolygonY1 = ly;
          pointCount++;
        } 
        selectionPlane->pixelXLineEor((int)lx0, (int)ly0, (int)lx2, (int)ly2,XEORLINE,true); // delete
        selectionPlane->pixelXLineEor((int)lx0, (int)ly0, (int)lx1, (int)ly1,XEORLINE,true); // paint
        selectionPlane->pixelLineEor((int)lx0, (int)ly0, (int)lx2, (int)ly2,EORLINE,true);
        selectionPlane->pixelLineEor((int)lx0, (int)ly0, (int)lx1, (int)ly1,EORLINE,true);
        if (pointCount > 0) {
          selectionPlane->pixelXLineEor((int)lx3, (int)ly3, (int)lx2, (int)ly2,XEORLINE,true); // delete
          selectionPlane->pixelXLineEor((int)lx3, (int)ly3, (int)lx1, (int)ly1,XEORLINE,true); // paint
          selectionPlane->pixelLineEor((int)lx3, (int)ly3, (int)lx2, (int)ly2,EORLINE,true);
          selectionPlane->pixelLineEor((int)lx3, (int)ly3, (int)lx1, (int)ly1,EORLINE,true);
        }
      } break;
    }
  }
}

void applyGradient(Layer *layer, Gradient *gradient, double cx0, double cy0, double cx1, double cy1) {
  double dx = cx1 - cx0;
  double dy = cy1 - cy0;
  double lineLength = sqrt(dx*dx+dy*dy);
  if (lineLength == 0) return;
  double nx = dx / lineLength;
  double ny = dy / lineLength;
  double tx = ny;
  double ty = -nx;

  bool selection = layer->hasSelection();

  Matrix m = inverse(layer->transform.transformation);

  double cx2 = 1.0 / toolAspectX;
  double cy2 = 1.0 / toolAspectY;

  for (int y = 0; y < layer->height; y++) {
    for (int x = 0; x < layer->width; x++) {
      if ((!selection)||(layer->pixelStates[x+y*layer->width] & PIXELSTATE_SELECTED)) {
        double cx = x*m.m[4*0+0] + y*m.m[4*1+0] + m.m[4*3+0];
        double cy = x*m.m[4*0+1] + y*m.m[4*1+1] + m.m[4*3+1];
        cx -= cx0;
        cy -= cy0;
        cx *= cx2;
        cy *= cy2;
        double gradx = cx * nx + cy * ny;
        double grady = cx * tx + cy * ty;
        gradx /= lineLength;
        grady /= lineLength;
        unsigned int col = gradient->getColor2D((float)gradx,(float)grady);
        layer->pixels[x+y*layer->width] = col;
      }
    }
  }
  shouldUpdateAll = true;
}

void useToolGradient() {

  if (wasActiveLayerSwitched()) {selectionToolActive = false; showSelectionPlane = false; showSelectionPlaneBounding = false; toolW = -1; toolH = -1; return;}

  mouseClickUndo();

  toolAspectX = currentCanvas->control.pixelSizeX != 0 ? 1.0 / currentCanvas->control.pixelSizeX : 1;
  toolAspectY = currentCanvas->control.pixelSizeY != 0 ? 1.0 / currentCanvas->control.pixelSizeY : 1;
  Layer *activeLayer = currentCanvas->getActiveLayer(); if (activeLayer != NULL && locked(activeLayer,LOCK_PIXELS)) return;
  static Layer *selectionToolLayer = activeLayer;
  selectionToolLayer = activeLayer;
  if (selectionToolLayer != NULL && selectionToolLayer->width * selectionToolLayer->height==0) return;
  if (selectionToolLayer == NULL || (!ImGui::IsMouseDown(0)) || ImGui::IsMouseDown(1)) {
    if (selectionToolActive) {
      selectionToolActive = false; showSelectionPlane = false; showSelectionPlaneBounding = false; toolW = -1; toolH = -1;
      if (!ImGui::IsMouseDown(1)) {
        if (tool == TOOL_GRADIENT) {applyGradient(activeLayer,&gradient,selectionX0,selectionY0,selectionX1,selectionY1); updatedPixels(activeLayer);}
      }
    } 
    return;
  }
  if (isPaintAreaActive()) {
    double lx,ly; currentCanvas->getCanvasPos(mouseX,mouseY,lx,ly);
    if (!selectionToolActive) {
      selectionPlane->init(activeLayer->width, activeLayer->height);
      selectionPlane->transform = activeLayer->transform;
      selectionX0 = lx;
      selectionY0 = ly;
      selectionX1 = lx;
      selectionY1 = ly;
      lastSelectionPolygonX1 = lx;
      lastSelectionPolygonY1 = ly;
    }
    lastSelectionX1 = selectionX1;
    lastSelectionY1 = selectionY1;
    selectionX1 = lx;
    selectionY1 = ly;
    switch(tool) {
    case TOOL_MEASURE: {
        measureCX0 = selectionX0;
        measureCY0 = selectionY0;
        measureCX1 = selectionX1;
        measureCY1 = selectionY1;
        double lx0,ly0; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,measureCX0,measureCY0,lx0,ly0);
        double lx1,ly1; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,measureCX1,measureCY1,lx1,ly1);
        measureLX0 = lx0;
        measureLY0 = ly0;
        measureLX1 = lx1;
        measureLY1 = ly1;
      } break;
    }
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
    showSelectionPlaneBounding = tool == TOOL_MEASURE;
    double lx0,ly0; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,lastSelectionPolygonX1,lastSelectionPolygonY1,lx0,ly0);
    double lx1,ly1; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,selectionX1,selectionY1,lx1,ly1);
    double lx2,ly2; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,lastSelectionX1,lastSelectionY1,lx2,ly2);
    double lx3,ly3; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,selectionX0,selectionY0,lx3,ly3);
    selectionPlane->pixelLineEor((int)lx0, (int)ly0, (int)lx2, (int)ly2,1,true);
    selectionPlane->pixelLineEor((int)lx0, (int)ly0, (int)lx1, (int)ly1,1,true);
  }
}

bool paintingOn = false;
Array<float> paintValues;
Array<unsigned int> paintBackground;

double paintingPointX0 = 0;
double paintingPointY0 = 0;
double paintingPointX1 = 0;
double paintingPointY1 = 0;
double paintingPointLastX1 = 0;
double paintingPointLastY1 = 0;
bool toolOutline = true;

bool lockedMouseX = false;
bool lockedMouseY = false;
int lockedMousePosX = 0;
int lockedMousePosY = 0;
bool tilingOn = false;

void drawTool(Layer *layer, Brush *brush,double canvasX, double canvasY, double angle, bool justSelection, bool paintJustIntoOutline, bool pencil) {
  if (locked(layer,LOCK_PIXELS)) return;

  double xs = (rand() & 1023)/1023.0*2.0-1.0;
  double ys = (rand() & 1023)/1023.0*2.0-1.0;
  canvasX += xs * brush->randomX * toolAspectX;
  canvasY += ys * brush->randomY * toolAspectY;
  BrushInstance *b = brush->getCreateInstance(BRUSHINSTANCE_PAINT,(int)brush->pixelWidth(),(int)brush->pixelHeight(),pencil);
  double toolWidth = b->pixelWidth*0.5*toolAspectX;
  double toolHeight = b->pixelHeight*0.5*toolAspectY;
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
  if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = (int)layerBoundings.x;
  if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = (int)layerBoundings.y;
  if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = (int)layerBoundings.z;
  if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = (int)layerBoundings.w;
  double tpx = canvasX-toolWidth, tpy = canvasY-toolHeight;
  Matrix t; t.translate(Vector(tpx,tpy,0));
  Matrix m = inverse2(t)*inverse2(layer->transform.transformation);
  bool interpolate = false;
  if (brush->followRotation) {
    interpolate = true;
    if (angle >= 0) {b->followedRotation = (float)angle;}
    Matrix toCenter; toCenter.translate(Vector(toolWidth,toolHeight,0));
    m = toCenter * rotationMatrixAngleAxis(b->followedRotation, 0, 0, 1) * inverse2(toCenter) * m;
  }
  Matrix s; s.scale3(Vector(toolAspectX,toolAspectY,1));
  m = inverse2(s) * m;
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0, dx = 0, dy = 0;
  if (tilingOn) {
    x0 = layer->toolInfo.tileDrawX0; y0 = layer->toolInfo.tileDrawY0;
    x1 = layer->toolInfo.tileDrawX1; y1 = layer->toolInfo.tileDrawY1;
    dx = x1 - x0; if (dx < 1) dx = 1;
    dy = y1 - y0; if (dy < 1) dy = 1;
    layerUpdateX0 -= dx; layerUpdateY0 -= dy;
    layerUpdateX1 += dx; layerUpdateY1 += dy;
  }
  for (int y = layerUpdateY0; y < layerUpdateY1; y++) {
    for (int x = layerUpdateX0; x < layerUpdateX1; x++) {
      double px = x*m.m[4*0+0]+y*m.m[4*1+0]+m.m[4*3+0];
      double py = x*m.m[4*0+1]+y*m.m[4*1+1]+m.m[4*3+1];
      int bx = (int)px;
      int by = (int)py;
      if (bx == 0) bx = (int)floor(px);
      if (by == 0) by = (int)floor(py);
      if ((unsigned int)bx < (unsigned int)b->pixelWidth && (unsigned int)by < (unsigned int)b->pixelHeight) {
        int p = x + y * layer->width;
        if (tilingOn) {
          int kx = x - x0;
          int ky = y - y0;
          if (kx >= 0) kx = (kx % dx) + x0; else kx = (abs(dx+kx) % dx) + x0;
          if (ky >= 0) ky = (ky % dy) + y0; else ky = (abs(dy+ky) % dy) + y0;
          if ((unsigned int)kx >= (unsigned int)layer->width || (unsigned int)ky >= (unsigned int)layer->height) continue;
          p = kx + ky * layer->width;
        }
        float here = b->values[bx+by*b->pixelWidth];
        if (interpolate) {
          float hereX = here;
          float hereY = here;
          float hereXY = here;
          if (bx + 1 < b->pixelWidth) hereX = b->values[bx+1+by*b->pixelWidth];;
          if (by + 1 < b->pixelHeight) hereY = b->values[bx+(by+1)*b->pixelWidth];;
          if (bx + 1 < b->pixelWidth && by + 1 < b->pixelHeight) hereXY = b->values[bx+1+(by+1)*b->pixelWidth];;
          float xf = (float)(px-bx);
          float yf = (float)(py-by);
          float top = (hereX-here)*xf + here;
          float bottom = (hereXY-hereY)*xf + hereY;
          here = (bottom-top)*yf+top;
        }
        if (here >= 0) {
          if (!justSelection || (layer->pixelStates[p] & PIXELSTATE_SELECTED)) {
            if (paintJustIntoOutline) {
              layer->pixelStates[p] |= PIXELSTATE_OUTLINE;
            } else {
                float value = paintValues[p];
                value = 1 - (1 - here) * (1 - value);
                paintValues[p] = value;
            }
            layer->pixelStates[p] |= PIXELSTATE_TEMP;
          }
          if (b->pixelWidth<=1||b->pixelHeight<=1) { // just one pixel (maybe a hack)
            x = layerUpdateX1;
            y = layerUpdateY1;
            break;
          }
        }
      }
    }
  }
  if (tool != TOOL_ERASER) addUsedColor(foregroundColor);
}

float lineWidth = 1;
bool lineCap = false;
bool lineAntialiasing = true;


void drawLine(Layer *layer, double canvasX0, double canvasY0, double canvasX1, double canvasY1, bool justSelection) {
  if (locked(layer,LOCK_PIXELS)) return;

  if (lineWidth == 0) return;
  Vector layerBoundings;
  bool first = true;
  int layerUpdateX0 = 0;
  int layerUpdateY0 = 0;
  int layerUpdateX1 = layer->width;
  int layerUpdateY1 = layer->height;
  for (int i = 0; i < 4; i++) {
    Vector k(i & 1 ? canvasX1 : canvasX0,i & 2 ? canvasY1 : canvasY0,0);
    double lx0,ly0; currentCanvas->getLayerPosFromCanvasPos(layer,k.x,k.y,lx0,ly0);
    if (first || lx0 < layerBoundings.x) layerBoundings.x = lx0;
    if (first || ly0 < layerBoundings.y) layerBoundings.y = ly0;
    if (first || lx0 > layerBoundings.z) layerBoundings.z = lx0;
    if (first || ly0 > layerBoundings.w) layerBoundings.w = ly0;
    first = false;
  }
  layerBoundings.x -= 2 + (lineWidth + 1)/toolAspectX;
  layerBoundings.y -= 2 + (lineWidth + 1)/toolAspectY;
  layerBoundings.z += 2 + (lineWidth + 1)/toolAspectX;
  layerBoundings.w += 2 + (lineWidth + 1)/toolAspectY;
  layerBoundings.z += 1;
  layerBoundings.w += 1;
  if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = (int)layerBoundings.x;
  if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = (int)layerBoundings.y;
  if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = (int)layerBoundings.z;
  if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = (int)layerBoundings.w;

  double lx0 = canvasX0; double ly0 = canvasY0; currentCanvas->getLayerPosFromCanvasPos(layer,lx0,ly0,lx0,ly0);
  double lx1 = canvasX1; double ly1 = canvasY1; currentCanvas->getLayerPosFromCanvasPos(layer,lx1,ly1,lx1,ly1);
  if (lineAntialiasing) {
    lx0 = lx0-0.5;
    ly0 = ly0-0.5;
    lx1 = lx1-0.5;
    ly1 = ly1-0.5;
  } else {
    lx0 = floor(lx0);
    ly0 = floor(ly0);
    lx1 = floor(lx1);
    ly1 = floor(ly1);
  }

  double lineNormalLX = lx1-lx0;
  double lineNormalLY = ly1-ly0;
  double lineLength = sqrt(lineNormalLX*lineNormalLX+lineNormalLY*lineNormalLY);
  if (lineLength == 0) {
    lineNormalLX = 0.5;
    lineNormalLY = 0.5;
    lineLength = sqrt(lineNormalLX*lineNormalLX+lineNormalLY*lineNormalLY);
  }
  lineNormalLX /= lineLength;
  lineNormalLY /= lineLength;
  double lineTangentLX = lineNormalLY;
  double lineTangentLY = -lineNormalLX;
  double lineCenterLX = (lx1+lx0)*0.5;
  double lineCenterLY = (ly1+ly0)*0.5;
  double centerLength = lineLength * 0.5 + (lineAntialiasing ? 0 : 0.5)*0.5;
  double lw2 = (lineWidth-(lineAntialiasing ? 0.5 : 0))*0.5;
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0, dx = 0, dy = 0;
  if (tilingOn) {
    x0 = layer->toolInfo.tileDrawX0; y0 = layer->toolInfo.tileDrawY0;
    x1 = layer->toolInfo.tileDrawX1; y1 = layer->toolInfo.tileDrawY1;
    dx = x1 - x0; if (dx < 1) dx = 1;
    dy = y1 - y0; if (dy < 1) dy = 1;
    layerUpdateX0 -= dx; layerUpdateY0 -= dy;
    layerUpdateX1 += dx; layerUpdateY1 += dy;
  }
  for (int y = layerUpdateY0; y < layerUpdateY1; y++) {
    for (int x = layerUpdateX0; x < layerUpdateX1; x++) {
      double lx = x - lineCenterLX;
      double ly = y - lineCenterLY;
      double distY = fabs(lx * lineNormalLX + ly * lineNormalLY);
      double distX = fabs(lx * lineTangentLX + ly * lineTangentLY);
      double distX0 = distX;
      distX -= lw2;
      distY -= centerLength;
      if (lineCap) {
        if (distY > 0 && distX0 > 0) {
          distY = sqrt(distX0*distX0+distY*distY);
          distY -= lw2;
          distX = 0;
        }
      }
      if (distX < 0) distX = 0;
      if (distY < 0) distY = 0;
      if (distY >= 1) continue;
      if (distX >= 1) continue;
      double here = distX;
      if (distY > here) here = distY;
      if (!lineAntialiasing) here = here>0?2:0;
      here = 1 - here;
      if (here >= 0) {
        int p = x + y * layer->width;
        if (tilingOn) {
          int kx = x - x0;
          int ky = y - y0;
          if (kx >= 0) kx = (kx % dx) + x0; else kx = (abs(dx+kx) % dx) + x0;
          if (ky >= 0) ky = (ky % dy) + y0; else ky = (abs(dy+ky) % dy) + y0;
          if ((unsigned int)kx >= (unsigned int)layer->width || (unsigned int)ky >= (unsigned int)layer->height) continue;
          p = kx + ky * layer->width;
        }
        float value = paintValues[p];
        value = (float)(1-(1-here)*(1-value));
        if (!justSelection || (layer->pixelStates[p] & PIXELSTATE_SELECTED)) {
          paintValues[p] = value;
          layer->pixelStates[p] |= PIXELSTATE_TEMP;
        }
      }
    }
  }
}

void mirrorPoint(double &x, double &y, float rounding = 0.5) {
  double ax = cos(mirrorToolAngle*PI/180.0);
  double ay = sin(mirrorToolAngle*PI/180.0);
  x += rounding;
  y += rounding;
  double x2 = x - mirrorToolPoint.x;
  double y2 = y - mirrorToolPoint.y;
  double d = x2 * ay + y2 * -ax;
  x = x - 2 * d * ay;
  y = y - 2 * d * -ax;
  x -= rounding;
  y -= rounding;
}

void drawLine2(Layer *layer, double canvasX0, double canvasY0, double canvasX1, double canvasY1, bool justSelection) {
  drawLine(layer, canvasX0, canvasY0, canvasX1, canvasY1, justSelection);
  if (mirrorToolEnabled) {
    mirrorPoint(canvasX0,canvasY0,0);
    mirrorPoint(canvasX1,canvasY1,0);
    drawLine(layer, canvasX0, canvasY0, canvasX1, canvasY1, justSelection);
  }
}

void drawTool2(Layer *layer, Brush *brush,double canvasX, double canvasY, double angle, bool justSelection, bool paintJustIntoOutline, bool pencil) {
  drawTool(layer, brush, canvasX, canvasY, angle, justSelection, paintJustIntoOutline, pencil);
  if (mirrorToolEnabled) {
    mirrorPoint(canvasX,canvasY);
    if (angle >= 0) {
      angle += mirrorToolAngle*PI/180.0;
    }
    drawTool(layer, brush, canvasX, canvasY, angle, justSelection, paintJustIntoOutline, pencil);
  }
}

void updateUpdateFlagByBrushLine(Layer *l,double cx0,double cy0, double cx1, double cy1, double toolWidth, double toolHeight) {
  if (locked(l,LOCK_PIXELS)) return;
  double paintingPointX0 = cx0;
  double paintingPointY0 = cy0;
  double paintingPointX1 = cx1;
  double paintingPointY1 = cy1;
  int layerUpdateX0 = 0;
  int layerUpdateY0 = 0;
  int layerUpdateX1 = l->width;
  int layerUpdateY1 = l->height;
  Vector layerBoundings;
  bool first = true;
  for (int i = 0; i < 8; i++) {
    Vector k;
    if (i < 4) 
      k = Vector(i & 1 ? paintingPointX0-toolWidth : paintingPointX0+toolWidth,i & 2 ? paintingPointY0-toolHeight : paintingPointY0+toolHeight,0);
    else 
      k = Vector(i & 1 ? paintingPointX1-toolWidth : paintingPointX1+toolWidth,i & 2 ? paintingPointY1-toolHeight : paintingPointY1+toolHeight,0);
    double lx0,ly0; currentCanvas->getLayerPosFromCanvasPos(l,k.x,k.y,lx0,ly0);
    if (first || lx0 < layerBoundings.x) layerBoundings.x = lx0;
    if (first || ly0 < layerBoundings.y) layerBoundings.y = ly0;
    if (first || lx0 > layerBoundings.z) layerBoundings.z = lx0;
    if (first || ly0 > layerBoundings.w) layerBoundings.w = ly0;
    first = false;
  }
  layerBoundings.z += 1;
  layerBoundings.w += 1;
  if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = (int)layerBoundings.x;
  if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = (int)layerBoundings.y;
  if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = (int)layerBoundings.z;
  if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = (int)layerBoundings.w;

  first = true;
  for (int i2 = 0; i2 < 4; i2++) {
    Vector k(i2 & 1 ? layerUpdateX1 : layerUpdateX0,i2 & 2 ? layerUpdateY1 : layerUpdateY0,0);
    double lx0,ly0; currentCanvas->getScreenPos(l,k.x,k.y,lx0,ly0);
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
  if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = (int)layerBoundings.x;
  if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = (int)layerBoundings.y;
  if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = (int)layerBoundings.z;
  if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = (int)layerBoundings.w;

  static int someTimer = 0; someTimer++; if ((someTimer & 3)==0) currentCanvas->clearUpdateFlag();
  for (int y2 = layerUpdateY0; y2 < layerUpdateY1; y2++) {
    for (int x2 = layerUpdateX0; x2 < layerUpdateX1; x2++) {
      currentCanvas->pixelStates[x2+y2*currentCanvas->width] |= PIXELSTATE_UPDATED;
    }
  }
}

void useToolPainting() {

  int tool2 = tool;
  if (controlKeyId == CONTROLKEY_CTRL) tool2 = TOOL_ERASER;
  bool pencil = tool == TOOL_PENCIL; 
  if (tool == TOOL_ERASER && hardEraser) pencil = true;
  if (tool2 == TOOL_ERASER && tool != TOOL_ERASER) pencil = true;

  if (wasActiveLayerSwitched()) {paintingOn = false; return;}

  toolAspectX = currentCanvas->control.pixelSizeX != 0 ? 1.0 / currentCanvas->control.pixelSizeX : 1;
  toolAspectY = currentCanvas->control.pixelSizeY != 0 ? 1.0 / currentCanvas->control.pixelSizeY : 1;

  mouseClickUndo();

  if (!paintingOn && ImGui::IsMouseClicked(1) && isPaintAreaActive()) {foregroundColor = getImVec4Color(currentCanvas->getLayerColor(mouseX, mouseY,false)); addUsedColor(foregroundColor);}
  bool useTempBrush = tool == TOOL_LINE;
  if ((unsigned int)currentBrush >= brushes.size() && (!useTempBrush)) return;
  Brush *brush = useTempBrush ? onePixelBrush : brushes[currentBrush];
  static double rateCounter = 0;
  static bool waitForMouse0 = false;
  if (waitForMouse0) {
    if (ImGui::IsMouseDown(0)) return;
    waitForMouse0 = false;
  }
  Layer *activeLayer = currentCanvas->getActiveLayer(); if (activeLayer != NULL && locked(activeLayer,LOCK_PIXELS)) return;
  static Layer *paintToolLayer = activeLayer;
  paintToolLayer = activeLayer;
  if (paintToolLayer != NULL && paintToolLayer->width * paintToolLayer->height==0) return;

  if (paintToolLayer != NULL && (!paintingOn) && toolOutline && isPaintAreaActive()) {
    Matrix m2 = inverse2(paintToolLayer->transform.transformation);
    double cx = mouseX, cy = mouseY; currentCanvas->getCanvasPos(cx,cy,cx,cy);
    double px = cx, py = cy; currentCanvas->getLayerPosFromCanvasPos(paintToolLayer,px,py,px,py);
    px = floor(px); py = floor(py);
    cx = px*m2.m[4*0+0]+py*m2.m[4*1+0]+m2.m[4*3+0];
    cy = px*m2.m[4*0+1]+py*m2.m[4*1+1]+m2.m[4*3+1];
    paintToolLayer->updatePixelStates(~PIXELSTATE_OUTLINE,0,0); reverseLastUpdatedPixelStates(paintToolLayer);
    drawTool(paintToolLayer,brush,cx,cy,0,false,true,pencil);

    double toolWidth = (brush->pixelWidth()*0.5+brush->randomX) * toolAspectX;
    double toolHeight = (brush->pixelHeight()*0.5+brush->randomY) * toolAspectY;
    updateUpdateFlagByBrushLine(paintToolLayer,cx,cy,cx,cy,toolWidth,toolHeight);

    if (tilingOn) {
      Matrix m1 = paintToolLayer->transform.transformation * currentCanvas->transform.transformation;
      for (int y = 0; y < currentCanvas->height; y++) {
        for (int x = 0; x < currentCanvas->width; x++) {
          int ix = (int)(x*m1.m[4*0+0]+y*m1.m[4*1+0]+m1.m[4*3+0]);
          int iy = (int)(x*m1.m[4*0+1]+y*m1.m[4*1+1]+m1.m[4*3+1]);
          if ((unsigned int)ix < (unsigned int)paintToolLayer->width && (unsigned int)iy < (unsigned int)paintToolLayer->height) {
            unsigned char p = paintToolLayer->pixelStates[ix+iy*paintToolLayer->width];
            if (p & PIXELSTATE_OUTLINE) {
              currentCanvas->pixelStates[x+y*currentCanvas->width] |= PIXELSTATE_UPDATED;
            }
          }
        }
      }
    }

    repaintLayer(paintToolLayer,false);
    paintToolLayer->hasOutline = true;
    clearOutlineAfterNextPaint = true;
  }

  if ( paintToolLayer == NULL || (!ImGui::IsMouseDown(0)) || ImGui::IsMouseDown(1)) {
    if (paintingOn) {
      paintingOn = false;
      if (ImGui::IsMouseDown(1)) {
        memcpy(paintToolLayer->pixels,&paintBackground[0],paintToolLayer->width * paintToolLayer->height * sizeof(unsigned int));
        waitForMouse0 = true;
        shouldUpdateAll = true;
      }
      updatedPixels(paintToolLayer);
    } 
    return;
  }
  if (isPaintAreaActive()) {
    double lx,ly; currentCanvas->getCanvasPos(mouseX,mouseY,lx,ly);
    static double dx2,dy2;
    static bool initRotation = false;
    static bool justSelection = false;
    static double initialAngle = 0;
    if (!paintingOn) {
      paintingOn = true;
      paintingPointX0 = lx;
      paintingPointY0 = ly;
      paintingPointX1 = lx;
      paintingPointY1 = ly;
      paintingPointLastX1 = lx;
      paintingPointLastY1 = ly;
      paintValues.resize(paintToolLayer->width * paintToolLayer->height);
      paintBackground.resize(paintToolLayer->width * paintToolLayer->height);
      memset(&paintValues[0],0,paintToolLayer->width * paintToolLayer->height * sizeof(float));
      memcpy(&paintBackground[0],paintToolLayer->pixels,paintToolLayer->width * paintToolLayer->height * sizeof(unsigned int));
      currentCanvas->clearUpdateFlag();
      rateCounter = 1;
      initRotation = true;
      initialAngle = 0;
      justSelection = paintToolLayer->hasSelection();
      dx2 = 1;
      dy2 = 0;
    }
    paintingPointLastX1 = paintingPointX1;
    paintingPointLastY1 = paintingPointY1;
    switch(tool2) {
      case TOOL_PAINTBRUSH:
      case TOOL_PENCIL:
      case TOOL_ERASER:
      case TOOL_MAGIC: {
        paintingPointX0 = paintingPointX1;
        paintingPointY0 = paintingPointY1;
      } break;
    }
    paintingPointX1 = lx;
    paintingPointY1 = ly;

    int layerUpdateX0 = 0;
    int layerUpdateY0 = 0;
    int layerUpdateX1 = paintToolLayer->width;
    int layerUpdateY1 = paintToolLayer->height;
    double toolWidth = (brush->pixelWidth()*0.5+brush->randomX) * toolAspectX;
    double toolHeight = (brush->pixelHeight()*0.5+brush->randomY) * toolAspectY;
    Vector layerBoundings;
    bool first = true;
    int count = 8;
    if (tool == TOOL_LINE) {count = 10; toolWidth += (lineWidth+1)/toolAspectX; toolHeight += (lineWidth+1)/toolAspectY;}
    for (int i = 0; i < count; i++) {
      Vector k;
      if (i < 4) 
        k = Vector(i & 1 ? paintingPointX0-toolWidth : paintingPointX0+toolWidth,i & 2 ? paintingPointY0-toolHeight : paintingPointY0+toolHeight,0);
      else 
      if (i < 8)
        k = Vector(i & 1 ? paintingPointX1-toolWidth : paintingPointX1+toolWidth,i & 2 ? paintingPointY1-toolHeight : paintingPointY1+toolHeight,0);
      if (i == 8) k = Vector(paintingPointLastX1-toolWidth,paintingPointLastY1-toolHeight,0);
      if (i == 9) k = Vector(paintingPointLastX1+toolWidth,paintingPointLastY1+toolHeight,0);
      double lx0,ly0; currentCanvas->getLayerPosFromCanvasPos(paintToolLayer,k.x,k.y,lx0,ly0);
      if (first || lx0 < layerBoundings.x) layerBoundings.x = lx0;
      if (first || ly0 < layerBoundings.y) layerBoundings.y = ly0;
      if (first || lx0 > layerBoundings.z) layerBoundings.z = lx0;
      if (first || ly0 > layerBoundings.w) layerBoundings.w = ly0;
      first = false;
    }
    layerBoundings.z += 1;
    layerBoundings.w += 1;
    if (tool == TOOL_LINE) {
      layerBoundings.x -= 1;
      layerBoundings.y -= 1;
      layerBoundings.z += 1;
      layerBoundings.w += 1;
    }
    if ((!mirrorToolEnabled) && (!tilingOn)) { // if mirror tool is on just take the full canvas (slow)
      if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = (int)layerBoundings.x;
      if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = (int)layerBoundings.y;
      if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = (int)layerBoundings.z;
      if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = (int)layerBoundings.w;
    }

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
    switch(tool2) {
      case TOOL_PAINTBRUSH:
      case TOOL_PENCIL:
      case TOOL_ERASER:
      case TOOL_MAGIC: {
        double px = lpx0;
        double py = lpy0;
        double ax = delta == 0 ? 0 : (double)(lpx1-lpx0)/delta;
        double ay = delta == 0 ? 0 : (double)(lpy1-lpy0)/delta;
        Matrix m2 = inverse2(paintToolLayer->transform.transformation);
        for (int i3 = 0; i3 < delta+1; i3++) {
          double angle = -1,dx=0,dy=0;
          if (brush->followRotation) {
            dx = paintingPointX1 - paintingPointX0;
            dy = paintingPointY1 - paintingPointY0;
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
            drawTool2(paintToolLayer,brush,cx,cy,angle,justSelection,false,pencil);
          }
          px += ax;
          py += ay;
          if (brush->followRotation) {
            if (delta > 0) {
              double l = sqrt(dx*dx+dy*dy);
              if (l != 0) {dx /= l; dy /= l;}
              dx2 = (dx-dx2)*(1.0/delta)+dx2;
              dy2 = (dy-dy2)*(1.0/delta)+dy2;
            }
          }
        }
      } break;
      case TOOL_LINE: {
        for (int i = 0; i < paintToolLayer->width * paintToolLayer->height; i++) {
          if (paintValues[i] != 0) {
            paintToolLayer->pixelStates[i] |= PIXELSTATE_TEMP;
            paintValues[i] = 0;
          }
        }
        drawLine2(paintToolLayer,paintingPointX0,paintingPointY0,paintingPointX1,paintingPointY1,justSelection);
      } break;
    }

    for (int y = layerUpdateY0; y < layerUpdateY1; y++) {
      for (int x = layerUpdateX0; x < layerUpdateX1; x++) {
        int p = x + y * paintToolLayer->width;
        float value = paintValues[p];
        if (paintToolLayer->pixelStates[p] & PIXELSTATE_TEMP) {
          paintToolLayer->pixelStates[p] &= ~PIXELSTATE_TEMP;
          unsigned int col = paintBackground[p];
          switch(tool2) {
            case TOOL_LINE:
            case TOOL_PAINTBRUSH:
            case TOOL_MAGIC:
            case TOOL_PENCIL: {
              float rd = (float)(col & 255);
              float gd = (float)((col>>8) & 255);
              float bd = (float)((col>>16) & 255);
              int ad = (col>>24) & 255;
              float rs = foregroundColor.x*255.f;
              float gs = foregroundColor.y*255.f;
              float bs = foregroundColor.z*255.f;
              int as = (int)(foregroundColor.w*value*255.f*brush->opacity);
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
              int r = ((int)rs-(int)rd)*(int)as/255+(int)rd;
              int g = ((int)gs-(int)gd)*(int)as/255+(int)gd;
              int b = ((int)bs-(int)bd)*(int)as/255+(int)bd;
              paintToolLayer->pixels[p] = (r)|(g<<8)|(b<<16)|(a<<24);
  
            } break;
            case TOOL_ERASER: {
              int ad = (col>>24) & 255;
              ad = (int)(ad*(1-value));
              if (ad < 0) ad = 0;
              switch(paintToolLayer->mode) {
                case LAYERMODE_PREMULTIPLIEDALPHA: {
                    int r = col & 255;
                    int g = (col>>8) & 255;
                    int b = (col>>16) & 255;
                    int a = (col>>24) & 255;
                    r = (int)(r*(1-value));
                    g = (int)(g*(1-value));
                    b = (int)(b*(1-value));
                    if (r < 0) r = 0;
                    if (g < 0) g = 0;
                    if (b < 0) b = 0;
                    col = r|(g<<8)|(b<<16)|(a<<24);
                  } break;
              }
              paintToolLayer->pixels[p] = (col & 0x00ffffff)|(ad<<24);
            } break;
          }
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

    if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = (int)layerBoundings.x;
    if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = (int)layerBoundings.y;
    if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = (int)layerBoundings.z;
    if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = (int)layerBoundings.w;

    static int someTimer = 0; someTimer++; if ((someTimer & 15)==0) currentCanvas->clearUpdateFlag();
    for (int y2 = layerUpdateY0; y2 < layerUpdateY1; y2++) {
      for (int x2 = layerUpdateX0; x2 < layerUpdateX1; x2++) {
        currentCanvas->pixelStates[x2+y2*currentCanvas->width] |= PIXELSTATE_UPDATED;
      }
    }

    repaintLayer(paintToolLayer,false);
  }
}

float FXAA_REDUCE_MIN = 1.f/128.f;
float FXAA_REDUCE_MUL = 1.f/(8.f*(1.f/2.f));
float FXAA_SPAN_MAX = 8.f;

static __inline float luma(const unsigned int rgba) {
  return (float)((rgba & 255)*luminanceValues[0]+(float)((rgba>>8) & 255)*luminanceValues[1]+(float)((rgba>>16) & 255)*luminanceValues[2])*(1.f/255.f);
}

__inline unsigned int tex(unsigned int *source, const int width, const int height, const float tx, const float ty) {
  int itx = (int)(tx); // tx,ty not below 0 (trunc)
  int ity = (int)(ty);
  const float fx = tx - (float)itx;
  const float fy = ty - (float)ity;
  if (itx<0) itx = 0;
  if (ity<0) ity = 0;
  if (itx>=width-1) itx = width-2;
  if (ity>=height-1) ity = height-2;
  const float p1v = (1-fx)*(1-fy); 
  const float p2v = fx*(1-fy); 
  const float p3v = fx*fy; 
  const float p4v = (1-fx)*fy;
  const unsigned int rgba00 = source[itx+ity*width];
  const unsigned int rgba10 = source[itx+1+ity*width];
  const unsigned int rgba01 = source[itx+(ity+1)*width];
  const unsigned int rgba11 = source[itx+1+(ity+1)*width];
  const float r = (rgba00 & 255)*p1v + (rgba10 & 255)*p2v + (rgba11 & 255)*p3v + (rgba01 & 255)*p4v;
  const float g = ((rgba00>>8) & 255)*p1v + ((rgba10>>8) & 255)*p2v + ((rgba11>>8) & 255)*p3v + ((rgba01>>8) & 255)*p4v;
  const float b = ((rgba00>>16) & 255)*p1v + ((rgba10>>16) & 255)*p2v + ((rgba11>>16) & 255)*p3v + ((rgba01>>16) & 255)*p4v;
  const float a = ((rgba00>>24) & 255)*p1v + ((rgba10>>24) & 255)*p2v + ((rgba11>>24) & 255)*p3v + ((rgba01>>24) & 255)*p4v;
  const int ri = (int)r;
  const int gi = (int)g;
  const int bi = (int)b;
  const int ai = (int)a;
  return ri|(gi<<8)|(bi<<16)|(ai<<24);
}

#define FXAA_EDGE_SHARPNESS 8 // 2,4,8 are proposed whilst 8 is sharper and 2 is softer (proposed are 4,8)
#define FXAA_EDGE_THRESHOLD 0.125f // you need atleast this luma difference for the algorithm to apply
#define FXAA_EDGE_THRESHOLD_MIN 0.05f // algorithms doesn't process any darker colors

static __inline unsigned int combineColor(unsigned int c1, unsigned int c2) {
  int r = ((c1 & 255) + (c2 & 255))/2;
  int g = (((c1>>8) & 255) + ((c2>>8) & 255))/2;
  int b = (((c1>>16) & 255) + ((c2>>16) & 255))/2;
  int a = (((c1>>24) & 255) + ((c2>>24) & 255))/2;
  return r|(g<<8)|(b<<16)|(a<<24);
}

// to be found on github (original algorithm is by Timothy Lottes for NVIDIA):
// NVIDIAGameWorks-GraphicsSamples/samples/es3-kepler/FXAA/FXAA3_11.h
// license in the documentation file of this program
unsigned int doFXAASinglePixel(int x, int y, unsigned int *sourceBuffer, const int width, const int height, unsigned char *maskBuffer, unsigned char mask) {
  UNUSED(maskBuffer);
  UNUSED(mask);
  const float fy = (float)y; const float fx = (float)x;
  const unsigned int rgbaM = tex(sourceBuffer,width,height,fx,fy); 
  const float lumaM = luma(rgbaM);
  const float lumaNW = luma(tex(sourceBuffer,width,height,fx-0.5f,fy-0.5f));
  const float lumaSW = luma(tex(sourceBuffer,width,height,fx-0.5f,fy+0.5f));
  const float lumaNE = luma(tex(sourceBuffer,width,height,fx+0.5f,fy-0.5f));
  const float lumaSE = luma(tex(sourceBuffer,width,height,fx+0.5f,fy+0.5f));
  float lumaMin = minf( minf(lumaNW,lumaSW), minf(lumaNE,lumaSE) );
  float lumaMax = maxf( minf(lumaNW,lumaSW), maxf(lumaNE,lumaSE) );
  if ((maxf(lumaMax, lumaM)-minf(lumaMin, lumaM)) < maxf(FXAA_EDGE_THRESHOLD_MIN, lumaMax * FXAA_EDGE_THRESHOLD)) {
    return rgbaM;
  }
  float ndirx = -lumaNE + lumaSW - lumaNW + lumaSE;
  float ndiry = -lumaNE + lumaSW + lumaNW - lumaSE;
  float dirxyzl = (float)sqrt(ndirx*ndirx+ndiry*ndiry); if (dirxyzl != 0) dirxyzl = 1.f / dirxyzl;
  ndirx *= dirxyzl, ndiry *= dirxyzl;
  float dirAbsMinTimesC = minf((float)fabs(ndirx), (float)fabs(ndiry)) * FXAA_EDGE_SHARPNESS; if (dirAbsMinTimesC < 0.0001f) dirAbsMinTimesC = 0.0001f;
  const float udirx = clampf(ndirx / dirAbsMinTimesC, -2.f, 2.f); const float udiry = clampf(ndiry / dirAbsMinTimesC, -2.f, 2.f);
  const unsigned int rgbn1 = tex(sourceBuffer, width, height, x - ndirx * 0.5f, y - ndiry * 0.5f);
  const unsigned int rgby1 = combineColor(tex(sourceBuffer, width, height, x + ndirx * 0.5f, y + ndiry * 0.5f), rgbn1);
  const unsigned int rgbn2 = tex(sourceBuffer, width, height, x - udirx * 0.5f, y - udiry * 0.5f);
  const unsigned int rgby2 = combineColor(tex(sourceBuffer, width, height, x + udirx * 0.5f, y + udiry * 0.5f), rgbn2);
  const float lumaRgby2 = (luma(rgby1) + luma(rgby2)) * 0.5f;
  if ((lumaRgby2 < lumaMin) || (lumaRgby2 > lumaMax)) return rgby1;
  const int r2 = ((rgby1 & 255) + (rgby2 & 255))/2;
  const int g2 = (((rgby1>>8) & 255) + ((rgby2>>8) & 255))/2;
  const int b2 = (((rgby1>>16) & 255) + ((rgby2>>16) & 255))/2;
  const int a2 = (((rgby1>>24) & 255) + ((rgby2>>24) & 255))/2;
  return (r2)|(g2<<8)|(b2<<16)|(a2<<24);
}

void doFXAA(unsigned int *sourceBuffer, const int width, const int height, unsigned char *maskBuffer, unsigned char mask) {
  unsigned int *tempBuffer = sourceBuffer;
  float *tr = new float[width];

  float *k = tr;
  const float fy = 0;
  for (int x = 0; x < width; x++) {
    const float fx = (float)x;
    *k++=luma(tex(sourceBuffer,width,height,fx+0.5f,fy-0.5f));
  }
  initStatusBar();
  int portionSize = height / 16; if (portionSize<1) portionSize = 1;
  for (int y = 0; y < height; y++) {
    if ((y % portionSize)==0) updateStatusBar();
    unsigned int *source = &sourceBuffer[y*width];
    unsigned int *dest = &tempBuffer[y*width];
    const float fy = (float)y;
    const float fx = 1; // why 1?
    float lumaNW = luma(tex(sourceBuffer,width,height,fx-0.5f,fy-0.5f));
    float lumaSW = luma(tex(sourceBuffer,width,height,fx-0.5f,fy+0.5f));
    k = tr;
    for (int x = 0; x < width; x++) {
      const float fx = (float)x;

      const float lumaM = luma(*source);
      const float lumaNE = *k;
      const float lumaSE = luma(tex(sourceBuffer,width,height,fx+0.5f,fy+0.5f));

      if (maskBuffer == NULL || (maskBuffer[x+y*width] & mask)==mask) {

        float lumaMin = minf( minf(lumaNW,lumaSW), minf(lumaNE,lumaSE) );
        float lumaMax = maxf( minf(lumaNW,lumaSW), maxf(lumaNE,lumaSE) );
        if ((maxf(lumaMax, lumaM)-minf(lumaMin, lumaM)) < maxf(FXAA_EDGE_THRESHOLD_MIN, lumaMax * FXAA_EDGE_THRESHOLD)) {
          *dest = *source;
        } else {
          float ndirx = -lumaNE + lumaSW - lumaNW + lumaSE;
          float ndiry = -lumaNE + lumaSW + lumaNW - lumaSE;
          float dirxyzl = (float)sqrt(ndirx*ndirx+ndiry*ndiry); if (dirxyzl != 0) dirxyzl = 1.f / dirxyzl;
          ndirx *= dirxyzl, ndiry *= dirxyzl;
          float dirAbsMinTimesC = minf((float)fabs(ndirx), (float)fabs(ndiry)) * FXAA_EDGE_SHARPNESS; if (dirAbsMinTimesC < 0.0001f) dirAbsMinTimesC = 0.0001f;
          const float udirx = clampf(ndirx / dirAbsMinTimesC, -2.f, 2.f); const float udiry = clampf(ndiry / dirAbsMinTimesC, -2.f, 2.f);
          const unsigned int rgbn1 = tex(sourceBuffer, width, height, x - ndirx * 0.5f, y - ndiry * 0.5f);
          const unsigned int rgby1 = combineColor(tex(sourceBuffer, width, height, x + ndirx * 0.5f, y + ndiry * 0.5f), rgbn1);
          const unsigned int rgbn2 = tex(sourceBuffer, width, height, x - udirx * 0.5f, y - udiry * 0.5f);
          const unsigned int rgby2 = combineColor(tex(sourceBuffer, width, height, x + udirx * 0.5f, y + udiry * 0.5f), rgbn2);
          const float lumaRgby2 = (luma(rgby1) + luma(rgby2)) * 0.5f;
          if ((lumaRgby2 < lumaMin) || (lumaRgby2 > lumaMax)) {
            *dest = rgby1;
          } else {
            const int r2 = ((rgby1 & 255) + (rgby2 & 255))/2;
            const int g2 = (((rgby1>>8) & 255) + ((rgby2>>8) & 255))/2;
            const int b2 = (((rgby1>>16) & 255) + ((rgby2>>16) & 255))/2;
            const int a2 = (((rgby1>>24) & 255) + ((rgby2>>24) & 255))/2;
            *dest =  (r2)|(g2<<8)|(b2<<16)|(a2<<24);
          }
        }
      }

      *k++ = lumaSE;
      lumaNW = lumaNE;
      lumaSW = lumaSE;
      source++;
      dest++;
    }
  }
  delete[] tr;
}

double measureCX0 = 0;
double measureCY0 = 0;
double measureLX0 = 0;
double measureLY0 = 0;
double measureCX1 = 0;
double measureCY1 = 0;
double measureLX1 = 0;
double measureLY1 = 0;

RGBAImage getImageFromLayer(Layer *l, int sx,int sy, int width, int height) {
  RGBAImage ret;
  if (l == NULL) return ret;
  if (locked(l,LOCK_PIXELS)) return ret;
  if (sx < 0) {width += sx; sx = 0;}
  if (sy < 0) {height += sy; sy = 0;}
  if (width + sx > l->width) width = l->width - sx;
  if (height + sy > l->height) height = l->height - sy;
  if (width<=0 || height<=0) return ret;
  ret.width = width;
  ret.height = height;
  ret.data = (unsigned int*)malloc(width*height*sizeof(unsigned int));
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      ret.data[x+y*width] = l->pixels[(x+sx)+(y+sy)*l->width];
    }
  }
  return ret;
}

void putImageToLayer(Layer *l, const RGBAImage &c, int sx,int sy, bool useAlpha) {
  if (c.data == NULL || c.width <= 0 || c.height <= 0) return;
  if (l == NULL) return;
  if (locked(l,LOCK_PIXELS)) return;
  for (int y = 0; y < c.height; y++) {
    for (int x = 0; x < c.width; x++) {
      int dx = sx + x;
      int dy = sy + y;
      if ((unsigned int)dx < (unsigned int)l->width && (unsigned int)dy < (unsigned int)l->height) {
        unsigned int col = c.data[x+y*c.width];
        if ((!useAlpha)||(((col>>24)&255)!=0))
          l->pixels[dx+dy*l->width] = col;
      }
    }
  }
  updatedPixels(l);
}

void selectTileDrawingRectangle(Layer *l,int x0, int y0, int x1, int y1) {
  for (int i = 0; i < (int)currentCanvas->layers.size(); i++) {
    LayerAdditionalToolInfo *t = &currentCanvas->layers[i]->toolInfo; t->tileDrawX0 = 0; t->tileDrawY0 = 0; t->tileDrawX1 = 0; t->tileDrawY1 = 0;
  }
  if (l != NULL) {
    LayerAdditionalToolInfo *t = &l->toolInfo; t->tileDrawX0 = x0; t->tileDrawY0 = y0; t->tileDrawX1 = x1; t->tileDrawY1 = y1;
  }
  if (tilingOn) {
    l->toolInfo.showTilingGrid = false;
    updateTilingGrid(l,true);
    l->selectNone();
    l->addSelectionRectangle(x0,y0,x1,y1);
  }
}

void useToolTiling() {
  if (!isPaintAreaActive()) return;
  Layer *l = currentCanvas->getActiveLayer(); if (l != NULL && locked(l,LOCK_PIXELS)) return;
  if (l == NULL) return;
  mouseClickUndo();
  double lx = mouseX, ly = mouseY; currentCanvas->getLayerPos(l,lx,ly,lx,ly);
  int sx = 0, sy = 0;
  if (l->toolInfo.tilingWidth>0) sx = (int)floor((lx-l->toolInfo.tilingOffsetX)/l->toolInfo.tilingWidth)*l->toolInfo.tilingWidth+l->toolInfo.tilingOffsetX;
  if (l->toolInfo.tilingHeight>0) sy = (int)floor((ly-l->toolInfo.tilingOffsetY)/l->toolInfo.tilingHeight)*l->toolInfo.tilingHeight+l->toolInfo.tilingOffsetY;
  if (ImGui::IsMouseClicked(0)) {
    if (controlKeyId == CONTROLKEY_SHIFT) {
      l->selectNone();
      l->addSelectionRectangle(sx,sy,sx+l->toolInfo.tilingWidth,sy+l->toolInfo.tilingHeight);
      selectTileDrawingRectangle(l,sx,sy,sx+l->toolInfo.tilingWidth,sy+l->toolInfo.tilingHeight);
    } else {
      copiedTiling.free();
      copiedTiling = getImageFromLayer(l,sx,sy,l->toolInfo.tilingWidth,l->toolInfo.tilingHeight);
      updateTilingImage(copiedTiling);
      selectTileDrawingRectangle(l,sx,sy,sx+l->toolInfo.tilingWidth,sy+l->toolInfo.tilingHeight);
    }
  }
  if (ImGui::IsMouseDown(1)) {
    putImageToLayer(l,copiedTiling,sx,sy,controlKeyId == CONTROLKEY_CTRL);
    repaintLayer(l,true);
  }
}

void drawMouseArrow2(int mouseX, int mouseY) {
  const int width = 8; const int height = 8; const char *bitmap =
  "60000000"
  "98900000"
  "98889000"
  "09888990"
  "09888889"
  "00989999"
  "00989000"
  "00099000";
  for (int y = 0; y < height; y++) { for (int x = 0; x < width; x++) {
      char c = bitmap[x+y*width];
      if (c != '0') {unsigned int color = 0xffffffff;
         if (c == '9') color = 0xff0000ff;
         if (c == '6') color = 0xffffffff;
        int dx = x+mouseX;
        int dy = y+mouseY;
        if ((unsigned int)dx < (unsigned int)glFrameBufferWidth && (unsigned int)dy < (unsigned int)glFrameBufferHeight) glFrameBuffer[dx+dy*glFrameBufferWidth]=color;
      }
    }
  }
}

void drawOpenGLMarkerCircle(double sx, double sy, double r, unsigned int color) {
  double aspect = aspectRatio * main_window_yres/main_window_xres;
  if (aspect == 0) aspect = 1;
  for (int y = (int)-r - 1; y <= (int)r + 1; y++) {
    for (int x = (int)(-r/aspect - 1); x <= (int)(r / aspect + 1); x++) {
      double px = sx + x;
      double py = sy + y;
      int ix = (int)floor(px);
      int iy = (int)floor(py);
      double dx = (ix - sx)*aspect;
      double dy = iy - sy;
      double d = sqrt(dx*dx + dy*dy);
      if (d <= r) {
        int d2 = (int)floor(d);
        int alpha = (int)(d2<r-1 ? 255 : (1-(d - d2))*255.0);
        if ((unsigned int)ix < (unsigned int)glFrameBufferWidth && (unsigned int)iy < (unsigned int)glFrameBufferHeight) {
          unsigned int c = glFrameBuffer[ix+iy*glFrameBufferWidth];
          int r = ((int)(color & 255) - (int)(c & 255))*alpha/255+(int)(c & 255);
          int g = ((int)((color>>8) & 255) - (int)((c>>8) & 255))*alpha/255+(int)((c>>8) & 255);
          int b = ((int)((color>>16) & 255) - (int)((c>>16) & 255))*alpha/255+(int)((c>>16) & 255);
          glFrameBuffer[ix+iy*glFrameBufferWidth] = r|(g<<8)|(b<<16)|(c & 0xff000000);
        }
      }
    }
  }
}

void drawOpenGLSimpleLine(double sx0, double sy0, double sx1, double sy1, unsigned int color) {
  sx0 = floor(sx0);
  sy0 = floor(sy0);
  sx1 = floor(sx1);
  sy1 = floor(sy1);
  int dx = (int)(sx1 - sx0);
  int dy = (int)(sy1 - sy0);
  int d = abs(dx);
  if (abs(dy)>abs(dx)) d = abs(dy);
  if (d <= 0) return;
  double px = sx0;
  double py = sy0;
  double xa = (double)dx / d;
  double ya = (double)dy / d;
  for (int i = 0; i < d + 1; i++) {
    int x = (int)floor(px);
    int y = (int)floor(py);

    if ((unsigned int)x < (unsigned int)glFrameBufferWidth && (unsigned int)y < (unsigned int)glFrameBufferHeight) {
      glFrameBuffer[x+y*glFrameBufferWidth] = color;
    }

    px += xa;
    py += ya;
  }
}

void drawOpenGLRect(double sx0, double sy0, double sx1, double sy1, unsigned int color) {
  sx0 = floor(sx0);
  sy0 = floor(sy0);
  sx1 = floor(sx1);
  sy1 = floor(sy1);

  for (int x = (int)sx0; x <= (int)sx1; x++) {
    int y = (int)sy0;
    if ((unsigned int)x < (unsigned int)glFrameBufferWidth && (unsigned int)y < (unsigned int)glFrameBufferHeight) {
      glFrameBuffer[x+y*glFrameBufferWidth] = color;
    }
    y = (int)sy1;
    if ((unsigned int)x < (unsigned int)glFrameBufferWidth && (unsigned int)y < (unsigned int)glFrameBufferHeight) {
      glFrameBuffer[x+y*glFrameBufferWidth] = color;
    }
  }

  for (int y = (int)sy0; y <= (int)sy1; y++) {
    int x = (int)sx0;
    if ((unsigned int)x < (unsigned int)glFrameBufferWidth && (unsigned int)y < (unsigned int)glFrameBufferHeight) {
      glFrameBuffer[x+y*glFrameBufferWidth] = color;
    }
    x = (int)sx1;
    if ((unsigned int)x < (unsigned int)glFrameBufferWidth && (unsigned int)y < (unsigned int)glFrameBufferHeight) {
      glFrameBuffer[x+y*glFrameBufferWidth] = color;
    }
  }
}

void drawNumberVert(float siz, int x, int y, int number) {
  String k = String::fromInt(number); char p[2]; p[1] = 0;
  if ( (!k.empty()) && number<0) k[0] = 'I';
  int charHeight = 6;
  y -= k.length()*charHeight/2;
  y += 1;
  for (int i = 0; i < k.length(); i++) {
    p[0] = k[i];
    drawText(siz, x+1, i*charHeight+y+1, p, 0xff000000, 0.5, 0);
    drawText(siz, x, i*charHeight+y, p, 0xffffffff, 0.5, 0);
  }
}

void toolCanvasStuff() {
  switch(tool) {
  case TOOL_TILING: {
      Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) break; if (locked(l,LOCK_PIXELS)) return;
      if (l->toolInfo.tilingWidth <= 0) break;
      if (l->toolInfo.tilingHeight <= 0) break;
      double lx = mouseX, ly = mouseY; currentCanvas->getLayerPos(l,lx,ly,lx,ly);
      int outaX = (l->toolInfo.tilingOffsetX+l->toolInfo.tilingWidth*100000) % l->toolInfo.tilingWidth;
      int outaY = (l->toolInfo.tilingOffsetY+l->toolInfo.tilingHeight*100000) % l->toolInfo.tilingHeight;
      int cx = (int)floor((double)(lx-outaX)/l->toolInfo.tilingWidth);
      int cy =  (int)floor((double)(ly-outaY)/l->toolInfo.tilingHeight);
      int tcx = l->width/l->toolInfo.tilingWidth;
      int tcy = l->height/l->toolInfo.tilingHeight;
      if (outaX != 0) tcx--;
      if (outaY != 0) tcy--;
      int index = cx+cy*tcx;
      lx = cx * l->toolInfo.tilingWidth + outaX;
      ly = cy * l->toolInfo.tilingHeight + outaY;
      lx += l->toolInfo.tilingWidth*0.5;
      ly += l->toolInfo.tilingHeight*0.5;
      currentCanvas->getScreenPos(l,lx,ly,lx,ly);
      float scale = (float)currentCanvas->control.zoom/100.f;
      if (scale < 1) scale = 1;
      if (scale > 4) scale = 4;
      if (l->toolInfo.showTilingGridIndices) {
        if (cx >= 0 && cy >= 0 && cx < tcx && cy < tcy) {
          drawText(scale, (int)lx+1, (int)ly+1, String::fromInt(index).c_str(), 0xff000000, 0.5, 0.5);
          drawText(scale, (int)lx, (int)ly, String::fromInt(index).c_str(), 0xffffffff, 0.5, 0.5);
        }
      }
      if (l->toolInfo.tilingAnimPlaying) {
        int cx,cy;
        index = l->toolInfo.tilingAnimFrame;
        if (getTilingAnimationFrameLayerCell(l,index,cx,cy,outaX,outaY)) {
          lx = cx * l->toolInfo.tilingWidth + outaX;
          ly = cy * l->toolInfo.tilingHeight + outaY;
          lx += l->toolInfo.tilingWidth*0.5;
          ly += l->toolInfo.tilingHeight*0.5;
          currentCanvas->getScreenPos(l,lx,ly,lx,ly);
          float scale = (float)currentCanvas->control.zoom/100.f;
          if (scale < 1) scale = 1;
          if (scale > 4) scale = 4;
          drawText(scale, (int)lx+1, (int)ly+1, String::fromInt(index).c_str(), 0xff0000ff, 0.5, 0.5);
          drawText(scale, (int)lx, (int)ly, String::fromInt(index).c_str(), 0xff000000, 0.5, 0.5);
        }
      }

    } break;
    case TOOL_MODIFY: {
      Array<Vector> *points = modifyMode ?&modifyPointsDest : &modifyPointsSource;
      if (points->empty()) break;
      if (modifyMode) {
        Layer *l = currentCanvas->getActiveLayer();
        if (l != NULL && l->width>0 && l->height>0) {
          static unsigned int lastUpdatePixelsCountHere = 0;
          static GLuint modifyGLHandle = 0;
          bool update = wasActiveLayerSwitched();
          if (modifyGLHandle == 0) update = true;
          if (l->undoData.updatePixelsCount != lastUpdatePixelsCountHere) update = true;
          lastUpdatePixelsCountHere = l->undoData.updatePixelsCount;
          if (update) {
            if (modifyGLHandle!=0) glDeleteTextures(1,&modifyGLHandle);
            glGenTextures(1,&modifyGLHandle);
            glBindTexture(GL_TEXTURE_2D,modifyGLHandle);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, l->width, l->height, 0, GL_RGBA, GL_BYTE, l->pixels);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D,0);
          }
          glViewport(0, 0, glFrameBufferWidth, glFrameBufferHeight);
          glMatrixMode(GL_PROJECTION);
          glLoadIdentity();
          glOrtho(0,(GLfloat)glFrameBufferWidth, (GLfloat)glFrameBufferHeight,0,-1,1);
          glMatrixMode(GL_MODELVIEW);
          glLoadIdentity();

          if (modifyClearBackground) {
            glBegin(GL_TRIANGLES);
            glColor4f(1,0,1,1);
            for (int i2 = 0; i2 < (int)modifyTriangles.size()/3; i2++) {
              for (int i3 = 0; i3 < 3; i3++) {
                int p = modifyTriangles[i2*3+i3];
                if ((unsigned int)p<points->size()) {
                  Vector s = modifyPointsSource[p];
                  double sx = s.x, sy = s.y; currentCanvas->getScreenPos(sx,sy,sx,sy);
                  glVertex4f((GLfloat)sx, (GLfloat)sy,0,1);
                }
              }
            }
            glEnd();
          }

          glEnable(GL_TEXTURE_2D);
          glBindTexture(GL_TEXTURE_2D,modifyGLHandle);
          glBegin(GL_TRIANGLES);
          glColor4f(1,1,1,1);
          for (int i2 = 0; i2 < (int)modifyTriangles.size()/3; i2++) {
            for (int i3 = 0; i3 < 3; i3++) {
              int p = modifyTriangles[i2*3+i3];
              if ((unsigned int)p<points->size()) {
                Vector s = (*points)[p];
                Vector t = modifyPointsSource[p];
                double sx = s.x, sy = s.y; currentCanvas->getScreenPos(sx,sy,sx,sy);
                double tx = t.x, ty = t.y; currentCanvas->getLayerPosFromCanvasPos(l,tx,ty,tx,ty);
                glTexCoord2d(tx/l->width,ty/l->height);
                glVertex4f((GLfloat)sx, (GLfloat)sy,0,1);
              }
            }
          }
          glEnd();
          glDisable(GL_TEXTURE_2D);
          glBindTexture(GL_TEXTURE_2D,0);
        }
      }
      for (int i2 = 0; i2 < (int)modifyTriangles.size()/3; i2++) {
        for (int i3 = 0; i3 < 3; i3++) {
          int p0 = modifyTriangles[i2*3+i3];
          int p1 = modifyTriangles[i2*3+((i3+1)%3)];
          if ((unsigned int)p0<points->size() && (unsigned int)p1<points->size()) {
            Vector s0 = (*points)[p0];
            Vector s1 = (*points)[p1];
            double sx0 = s0.x, sy0 = s0.y; currentCanvas->getScreenPos(sx0,sy0,sx0,sy0);
            double sx1 = s1.x, sy1 = s1.y; currentCanvas->getScreenPos(sx1,sy1,sx1,sy1);
            drawOpenGLSimpleLine(sx0,sy0,sx1,sy1,modifyMode ? 0xffffff00 : 0xff0000ff);
          }
        }
      }
      for (int i = 0; i < (int)points->size(); i++) {
        double sx = (*points)[i].x, sy = (*points)[i].y; currentCanvas->getScreenPos(sx,sy,sx,sy);
        drawOpenGLMarkerCircle(sx+1,sy+1,4,0xff000000);
        unsigned int col = 0xff0000ff;
        if (i == selectedModifyPoint) col = 0xffffffff;
        drawOpenGLMarkerCircle(sx,sy,4,col);
      }
    } break;
    case TOOL_TILEMAP: {
      Layer *l = currentCanvas->getActiveLayer();
      if (l == NULL) return;
      if (l->type != LAYERTYPE_TILEMAP) return;
      Layer *l2 = currentCanvas->getLayerById(l->tileSourceLayerId);
      if (l2 == NULL || l2->width <= 0 || l2->height <= 0 || l2->toolInfo.tilingWidth < 1 || l2->toolInfo.tilingHeight < 1) return;
      double lx = mouseX, ly = mouseY; currentCanvas->getLayerPos(l,lx,ly,lx,ly);
      int lxi = (int)floor(lx); int lyi = (int)floor(ly);
      int txi = (int)floor((double)lxi/l2->toolInfo.tilingWidth);
      int tyi = (int)floor((double)lyi/l2->toolInfo.tilingHeight);
      if ((unsigned int)txi < (unsigned int)l->width && (unsigned int)tyi < (unsigned int)l->height) {
        double sx0 = txi*l2->toolInfo.tilingWidth;
        double sy0 = tyi*l2->toolInfo.tilingHeight;
        double sx1 = (txi+1)*l2->toolInfo.tilingWidth;
        double sy1 = (tyi+1)*l2->toolInfo.tilingHeight;
        currentCanvas->getScreenPos(l,sx0,sy0,sx0,sy0);
        currentCanvas->getScreenPos(l,sx1,sy1,sx1,sy1);
        drawOpenGLRect(sx0,sy0,sx1+1,sy1+1,0xff0000ff); // actually sx0-1???
      }
    } break;
    case TOOL_MIRROR: {
      if (mirrorToolEnabled) {
        double lx = mirrorToolPoint.x, ly = mirrorToolPoint.y; currentCanvas->getScreenPos(lx,ly,lx,ly);
        double ax = cos(mirrorToolAngle*PI/180.0) + mirrorToolPoint.x;
        double ay = sin(mirrorToolAngle*PI/180.0) + mirrorToolPoint.y;
        currentCanvas->getScreenPos(ax,ay,ax,ay);
        ax -= lx;
        ay -= ly;

        double s = 10000.0;
        drawOpenGLSimpleLine(lx+ax*s,ly+ay*s,lx-ax*s,ly-ay*s,0xff0000ff);
        for (int i = 0; i < 8; i++) {
          double ax = cos(i/8.0*PI*2);
          double ay = sin(i/8.0*PI*2);
          drawOpenGLSimpleLine(lx+ax*4,ly+ay*4,lx-ax*4,ly-ay*4,0xff0000ff);
        }
      }
    } break;
    case TOOL_WAND: {
      if (watershed) {
        for (int i = 0; i < (int)watershedPoints.size(); i++) {
          WatershedPoint *p = &watershedPoints[i];
          double cx = p->x, cy = p->y; currentCanvas->getScreenPos(cx,cy,cx,cy);
          unsigned int color = p->selection ? 0xffffff00 : 0xff0000ff;
          if (watershedSelectedPoint == i) color = 0xff00ffff;
          for (int j = 0; j < 8; j++) {
            double ax = cos(j/8.0*PI*2);
            double ay = sin(j/8.0*PI*2);
            drawOpenGLSimpleLine(cx+ax*4+1,cy+ay*4+1,cx-ax*4+1,cy-ay*4+1,0xff000000);
            drawOpenGLSimpleLine(cx+ax*4,cy+ay*4,cx-ax*4,cy-ay*4,color);
          }
        }
      }
    }
  }

  if (lockedMouseX) {
    int x = lockedMousePosX;
    if (x >= 0 && x < glFrameBufferWidth) {
      for (int y = 0; y < glFrameBufferHeight; y++)
        glFrameBuffer[x+y*glFrameBufferWidth]=0xff0000ff;
    }
    drawMouseArrow2(x,(int)mouseY);
  }
  if (lockedMouseY) {
    int y = lockedMousePosY;
    if (y >= 0 && y < glFrameBufferHeight) {
      for (int x = 0; x < glFrameBufferWidth; x++)
        glFrameBuffer[x+y*glFrameBufferWidth]=0xff0000ff;
    }
    drawMouseArrow2((int)mouseX,y);
  }
  if (mirrorToolEnabled) {
    double lx = mouseX, ly = mouseY; currentCanvas->getCanvasPos(lx,ly,lx,ly);
    mirrorPoint(lx,ly, 0);
    currentCanvas->getScreenPos(lx,ly,lx,ly);
    drawMouseArrow2((int)lx,(int)ly);
  }
  if (rulersOn) {
    int subSteps = 10;
    int borderOffset = 10;
    int lineOffset = 10;
    int lineLengthFull = 7;
    int lineLengthSmall = 3;
    int lineLengthMiddle = 5;
    int textDist = 7;
    int x0 = currentCanvas->screenPosX+borderOffset; int y0 = currentCanvas->screenPosY+borderOffset;
    int sx0 = x0 + lineOffset; int sy0 = y0;
    int sx1 = x0+currentCanvas->width-borderOffset*2-lineOffset; int sy1 = y0;
    int sx2 = x0; int sy2 = y0 + lineOffset;
    int sx3 = x0; int sy3 = y0+currentCanvas->height-borderOffset*2-lineOffset;

    drawOpenGLSimpleLine(sx0, sy0, sx1, sy1, 0xffffffff);
    drawOpenGLSimpleLine(sx0, sy0+1, sx1, sy1+1, 0xff000000);

    double px0 = x0, py0 = y0; currentCanvas->getCanvasPos(px0,py0,px0,py0);
    double px1 = x0+1, py1 = y0; currentCanvas->getCanvasPos(px1,py1,px1,py1);
    double px2 = x0, py2 = y0+1; currentCanvas->getCanvasPos(px2,py2,px2,py2);

    double rulerStep = (px1-px0)*100; rulerStep = floor(rulerStep/10)*10;rulerStep /= (double)subSteps; if (rulerStep < 1) rulerStep = 1;
    double lastC = 0;
    for (int x = sx0-1; x <= sx1; x++) {
      double cx = x,cy = sy0; currentCanvas->getCanvasPos(cx,cy,cx,cy);
      int vcx = (int)floor(cx/rulerStep);
      int vlcx = (int)floor(lastC/rulerStep);
      if (x >= sx0) {
        if (vcx != vlcx) {
          int vc = vcx < 0 ? subSteps-1-(abs(vcx+1) % subSteps) : (vcx % subSteps);
          int lineLength = lineLengthSmall; if (vc == 0) lineLength = lineLengthFull;
          if (vc == subSteps/2) lineLength = lineLengthMiddle;
          drawOpenGLSimpleLine(x, sy0-1, x, sy0-1-lineLength, 0xffffffff);
          drawOpenGLSimpleLine(x+1, sy0-1, x+1, sy0-1-lineLength, 0xff000000);
          if (vc == 0) {
            double k = cx;
            drawText(1, x+1, sy0 + textDist+1, String::fromInt((int)k).c_str(), 0xff000000, 0.5, 0);
            drawText(1, x, sy0 + textDist, String::fromInt((int)k).c_str(), 0xffffffff, 0.5, 0);
          }
        }
      }
      if ((int)mouseX==(int)x) drawOpenGLSimpleLine(x, sy0, x+1, sy0, 0xff000000);
      lastC = cx;
    }
    drawOpenGLSimpleLine(sx2, sy2, sx3, sy3, 0xffffffff);
    drawOpenGLSimpleLine(sx2+1, sy2, sx3+1, sy3, 0xff000000);
    rulerStep = (py2-py0)*100; rulerStep = floor(rulerStep/10)*10;rulerStep /= (double)subSteps; if (rulerStep < 1) rulerStep = 1;
    lastC = 0;
    for (int y = sy2-1; y <= sy3; y++) {
      double cx = sx2,cy = y; currentCanvas->getCanvasPos(cx,cy,cx,cy);
      int vcy = (int)floor(cy/rulerStep);
      int vlcy = (int)floor(lastC/rulerStep);
      if (y >= sy2) {
        if (vcy != vlcy) {
          int vc = vcy < 0 ? subSteps-1-(abs(vcy+1) % subSteps) : (vcy % subSteps);
          int lineLength = lineLengthSmall; if (vc == 0) lineLength = lineLengthFull;
          if (vc == subSteps/2) lineLength = lineLengthMiddle;
          drawOpenGLSimpleLine(sx2-1, y, sx2-1-lineLength, y, 0xffffffff);
          drawOpenGLSimpleLine(sx2-1, y+1, sx2-1-lineLength, y+1, 0xff000000);
          if (vc == 0) {
            double k = cy;
            drawNumberVert(1, sx2 + textDist, y, (int)k);
          }
        }
      }
      if ((int)mouseY==(int)y) drawOpenGLSimpleLine(sx2, y, sx2, y+1, 0xff000000);
      lastC = cy;
    }

  }
}

int blurCircleSize = 64;
int blurTool = BLURTOOL_BLUR;

int medianRGBASort(const void *p, const void *q)
{
  const unsigned int a = *((const unsigned int*)p);
  const unsigned int b = *((const unsigned int*)q);
  float a_ = luma(a);
  float b_ = luma(b);
  if (a_ > b_) return 1;
  if (a_ < b_) return -1;
  return 0;
}

void drawCircle(Layer *layer, double canvasX, double canvasY, bool justSelection, bool paintJustIntoOutline) {
  if (layer == NULL) return;
  if (locked(layer,LOCK_PIXELS)) return;

  double toolWidth = blurCircleSize*0.5*toolAspectX;
  double toolHeight = blurCircleSize*0.5*toolAspectY;
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
  if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = (int)layerBoundings.x;
  if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = (int)layerBoundings.y;
  if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = (int)layerBoundings.z;
  if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = (int)layerBoundings.w;
  double tpx = canvasX-toolWidth, tpy = canvasY-toolHeight;
  Matrix t; t.translate(Vector(tpx,tpy,0));
  Matrix m = inverse2(t)*inverse2(layer->transform.transformation);
  Matrix s; s.scale3(Vector(toolAspectX,toolAspectY,1));
  m = inverse2(s) * m;

  if (!paintJustIntoOutline) {
    paintBackground.resize(blurCircleSize * blurCircleSize);
    paintValues.resize(blurCircleSize * blurCircleSize);
    for (int i = 0; i < blurCircleSize * blurCircleSize; i++) paintValues[i] = 0;
    for (int y = layerUpdateY0; y < layerUpdateY1; y++) {
      for (int x = layerUpdateX0; x < layerUpdateX1; x++) {
        double px = x*m.m[4*0+0]+y*m.m[4*1+0]+m.m[4*3+0];
        double py = x*m.m[4*0+1]+y*m.m[4*1+1]+m.m[4*3+1];
        int bx = (int)px;
        int by = (int)py;
        if (bx == 0) bx = (int)floor(px);
        if (by == 0) by = (int)floor(py);
        if ((unsigned int)bx < (unsigned int)blurCircleSize && (unsigned int)by < (unsigned int)blurCircleSize) {
          paintBackground[bx+by*blurCircleSize] = layer->pixels[x+y*layer->width];
          paintValues[bx+by*blurCircleSize] = 1;
        }
      }
    }
  }

  Array<unsigned int> medians; medians.resize(3*3); unsigned int *medians_ = &medians[0];


  for (int y = layerUpdateY0; y < layerUpdateY1; y++) {
    for (int x = layerUpdateX0; x < layerUpdateX1; x++) {
      double px = x*m.m[4*0+0]+y*m.m[4*1+0]+m.m[4*3+0];
      double py = x*m.m[4*0+1]+y*m.m[4*1+1]+m.m[4*3+1];
      int bx = (int)px;
      int by = (int)py;
      if (bx == 0) bx = (int)floor(px);
      if (by == 0) by = (int)floor(py);
      if ((unsigned int)bx < (unsigned int)blurCircleSize && (unsigned int)by < (unsigned int)blurCircleSize) {
        int p = x + y * layer->width;
        float dx = (bx+0.5f)/blurCircleSize*2.f-1.f;
        float dy = (by+0.5f)/blurCircleSize*2.f-1.f;
        float d = dx*dx+dy*dy;
        if (d <= 1 && d >= 0) {
          if (!justSelection || (layer->pixelStates[p] & PIXELSTATE_SELECTED)) {
            if (paintJustIntoOutline) {
              layer->pixelStates[p] |= PIXELSTATE_OUTLINE;
            } else {
              int r = 0;
              int g = 0;
              int b = 0;
              int a = 0;
              float w = 0;
              switch(blurTool) {
                case BLURTOOL_BLUR: {
                  for (int ky = -1; ky <= 1; ky++) {
                    int y2 = by+ky;
                    for (int kx = -1; kx <= 1; kx++) {
                      int x2 = bx+kx;
                      if ((unsigned int)x2<(unsigned int)blurCircleSize && (unsigned int)y2<(unsigned int)blurCircleSize) {
                        if (paintValues[x2+y2*blurCircleSize] != 0) {
                          float where = 1000.0;
                          if (where > 0) {
                            unsigned int col = paintBackground[x2+y2*blurCircleSize];
                            r += (int)(((col & 255)+0.5f)*where);
                            g += (int)((((col>>8) & 255)+0.5f)*where);
                            b += (int)((((col>>16) & 255)+0.5f)*where);
                            a += (int)((((col>>24) & 255)+0.5f)*where);
                            w += where;
                          }
                        }
                      }
                    }
                  }
                } break;
                case BLURTOOL_ANTIALIAS: {
                  if ((unsigned int)bx<(unsigned int)blurCircleSize && (unsigned int)by<(unsigned int)blurCircleSize) {
                    unsigned int col = doFXAASinglePixel(bx, by, &paintBackground[0], blurCircleSize, blurCircleSize, NULL, 0);
                    float where = 1;
                    r += (int)(((col & 255)+0.5f)*where);
                    g += (int)((((col>>8) & 255)+0.5f)*where);
                    b += (int)((((col>>16) & 255)+0.5f)*where);
                    a += (int)((((col>>24) & 255)+0.5f)*where);
                    w += where;
                  }
                  d = 1-d;
                  d *= d;
                  d = 1-d;
                } break;
                case BLURTOOL_MEDIAN: {
                  int currentMedian = 0;
                  for (int ky = -1; ky <= 1; ky++) {
                    int y2 = by+ky;
                    for (int kx = -1; kx <= 1; kx++) {
                      int x2 = bx+kx;
                      if ((unsigned int)x2<(unsigned int)blurCircleSize && (unsigned int)y2<(unsigned int)blurCircleSize) {
                        if (paintValues[x2+y2*blurCircleSize] != 0) {
                          medians_[currentMedian] = paintBackground[x2+y2*blurCircleSize];
                          currentMedian++;
                        }
                      }
                    }
                  }
                  if (currentMedian>1) {
                    qsort(medians_, currentMedian, sizeof(unsigned int), medianRGBASort);
                  }
                  if (currentMedian>0) {
                    unsigned int col = medians_[currentMedian/2];
                    r = col & 255;
                    g = (col>>8) & 255;
                    b = (col>>16) & 255;
                    a = (col>>24) & 255;
                    w = 1;
                  }
                } break;
                case BLURTOOL_PULL: {
                  float distanceFromCenterX = (bx+0.5f)/blurCircleSize*2.f-1.f;
                  float distanceFromCenterY = (by+0.5f)/blurCircleSize*2.f-1.f;
                  float distanceFromCenter = (float)sqrt(distanceFromCenterX*distanceFromCenterX+distanceFromCenterY*distanceFromCenterY);
                  float mdx = (float)mouseDX;
                  float mdy = (float)mouseDY;
                  float mDistance = (float)sqrt(mdx*mdx+mdy*mdy);
                  if (mDistance>blurCircleSize) {
                    float mAngle = (float)atan2(mdx,mdy);
                    mdx = (float)(sin(mAngle) * blurCircleSize);
                    mdy = (float)(cos(mAngle) * blurCircleSize);
                  }
                  float d = 1-distanceFromCenter; 
                  if (d < 0) d = 0;
                  if (d > 1) d = 1;
                  d = d * d * d * d * d;
                  float newX = (bx - mdx)*d + bx * (1-d);
                  float newY = (by - mdy)*d + by * (1-d);
                  unsigned int col = tex(&paintBackground[0], blurCircleSize, blurCircleSize, newX, newY);
                  r = col & 255;
                  g = (col>>8) & 255;
                  b = (col>>16) & 255;
                  a = (col>>24) & 255;
                  w = 1;
                } break;
              }
              if (w != 0) {
                r = (int)(r/w);
                g = (int)(g/w);
                b = (int)(b/w);
                a = (int)(a/w);
                unsigned int col = layer->pixels[p];
                int r2 = col & 255;
                int g2 = (col>>8) & 255;
                int b2 = (col>>16) & 255;
                int a2 = (col>>24) & 255;
                float f = 1-d;
                if (f > 0) {
                  if (f > 1) f = 1;
                  r = (int)((r - r2)*f+r2);
                  g = (int)((g - g2)*f+g2);
                  b = (int)((b - b2)*f+b2);
                  a = (int)((a - a2)*f+a2);
                  layer->pixels[p] = r|(g<<8)|(b<<16)|(a<<24);
                }
              }
            }
          }
        }
      }
    }
  }
}

void useToolBlur() {

  toolAspectX = currentCanvas->control.pixelSizeX != 0 ? 1.0 / currentCanvas->control.pixelSizeX : 1;
  toolAspectY = currentCanvas->control.pixelSizeY != 0 ? 1.0 / currentCanvas->control.pixelSizeY : 1;

  mouseClickUndo();

  Layer *activeLayer = currentCanvas->getActiveLayer(); if (activeLayer != NULL && locked(activeLayer,LOCK_PIXELS)) return;
  static Layer *blurToolLayer = activeLayer;
  blurToolLayer = activeLayer;
  if (blurToolLayer != NULL && blurToolLayer->width * blurToolLayer->height == 0) return;

  double toolWidth =  blurCircleSize * toolAspectX;
  double toolHeight =  blurCircleSize * toolAspectY;
  if (blurToolLayer != NULL && isPaintAreaActive() && (!ImGui::IsMouseDown(0))) {
    double cx,cy; currentCanvas->getCanvasPos(mouseX,mouseY,cx,cy);
    blurToolLayer->updatePixelStates(~PIXELSTATE_OUTLINE,0,0); reverseLastUpdatedPixelStates(blurToolLayer);
    drawCircle(blurToolLayer,cx,cy,false,true);
    updateUpdateFlagByBrushLine(blurToolLayer,cx,cy,cx,cy,toolWidth,toolHeight);
    repaintLayer(blurToolLayer,false);
    blurToolLayer->hasOutline = true;
    clearOutlineAfterNextPaint = true;
  }

  if (blurToolLayer != NULL && isPaintAreaActive() && ImGui::IsMouseDown(0)) {
    double cx,cy; currentCanvas->getCanvasPos(mouseX,mouseY,cx,cy);
    drawCircle(blurToolLayer,cx,cy,blurToolLayer->hasSelection(),false);
    updateUpdateFlagByBrushLine(blurToolLayer,cx,cy,cx,cy,toolWidth,toolHeight);
    tinyWait();
    updatedPixels(blurToolLayer);
    repaintLayer(blurToolLayer,false);
  }
}

Array<Vector> modifyPointsSource;
Array<Vector> modifyPointsDest;
Array<unsigned int> modifyTriangles;
int selectedModifyPoint = -1;
bool modifyMode = false;
bool modifyClearBackground = false;
bool modifyFiltered = true;
bool modifyOutlined = false;
bool modifyPerspective = false;

void createModifyTriangles() {
  Array<double> points;

  for (int i = 0; i < (int)modifyPointsSource.size(); i++) {
    points.push_back(modifyPointsSource[i].x);
    points.push_back(modifyPointsSource[i].y);
  }

  Delaunator delau;
  delau.delaunator(&points);

  modifyTriangles.clear();
  for (int i2 = 0; i2 < (int)delau.triangles.size(); i2++) modifyTriangles.push_back((unsigned int)delau.triangles[i2]);
}

void bakeModify(bool justCreateSelection, bool createNewLayer) {
  Layer *l = currentCanvas->getActiveLayer();

  if (l != NULL && justCreateSelection) {
    
    beforeUndoPoint();

    Array<Vector> here;
    int pointCount = modifyMode ? (int)modifyPointsDest.size() : (int)modifyPointsSource.size();
    int triangleCount = (int)modifyTriangles.size()/3;
    for (int i3 = 0; i3 < pointCount; i3++) {
      double lx,ly; lx = modifyMode ? modifyPointsDest[i3].x : modifyPointsSource[i3].x; ly = modifyMode ? modifyPointsDest[i3].y : modifyPointsSource[i3].y; currentCanvas->getLayerPosFromCanvasPos(l,lx,ly,lx,ly); here.push_back(Vector(floor(lx),floor(ly)));
    }

    for (int y = 0; y < l->height; y++) {
      for (int x = 0; x < l->width; x++) {
        Vector p(x,y);
        //double sum = 0;
        bool thisPixelShouldBeProcessed = false;
        for (int i = 0; i < triangleCount; i++) {
          Vector d[3];
          d[0] = (unsigned int)modifyTriangles[i*3+0]<(unsigned int)pointCount?here[modifyTriangles[i*3+0]]:Vector();
          d[1] = (unsigned int)modifyTriangles[i*3+1]<(unsigned int)pointCount?here[modifyTriangles[i*3+1]]:Vector();
          d[2] = (unsigned int)modifyTriangles[i*3+2]<(unsigned int)pointCount?here[modifyTriangles[i*3+2]]:Vector();
  
          bool allIn = true;
          for (int t = 0; t < 3; t++) {
            int t2 = (t+1)%3;
            double nx = d[t2].x-d[t].x;
            double ny = d[t2].y-d[t].y;
            double tx = ny;
            double ty = -nx;
            double dist = tx*(p.x-d[t].x)+ty*(p.y-d[t].y);
            if (dist < 0) allIn = false;
          }
          if (allIn) thisPixelShouldBeProcessed = true;
        }         
        if (thisPixelShouldBeProcessed) l->pixelStates[x+y*l->width] |= PIXELSTATE_SELECTED; else l->pixelStates[x+y*l->width] &= ~PIXELSTATE_SELECTED;
      }
    }
    updatedPixelStates(l);
    repaintLayer(l,true);
    afterUndoPoint();
    return;
  }

  if (l == NULL || l->width<1 || l->height<1 || modifyPointsSource.size() != modifyPointsDest.size() || modifyPointsSource.size()<3) return;

  beforeUndoPoint();
  Vector minB,maxB;
  bool first = true;
  for (int i = 0; i < (int)modifyPointsSource.size(); i++) {
    Vector k = modifyPointsSource[i];
    if (first || k.x < minB.x) minB.x = k.x;
    if (first || k.y < minB.y) minB.y = k.y;
    if (first || k.x > maxB.x) maxB.x = k.x;
    if (first || k.y > maxB.y) maxB.y = k.y;
    first = false;
  }
  for (int i2 = 0; i2 < (int)modifyPointsDest.size(); i2++) {
    Vector k = modifyPointsDest[i2];
    if (first || k.x < minB.x) minB.x = k.x;
    if (first || k.y < minB.y) minB.y = k.y;
    if (first || k.x > maxB.x) maxB.x = k.x;
    if (first || k.y > maxB.y) maxB.y = k.y;
    first = false;
  }
  double lx0 = minB.x, ly0 = minB.y; currentCanvas->getLayerPosFromCanvasPos(l,lx0,ly0,lx0,ly0);
  double lx1 = maxB.x, ly1 = maxB.y; currentCanvas->getLayerPosFromCanvasPos(l,lx1,ly1,lx1,ly1);
  int layerStartX = (int)floor(lx0);
  int layerStartY = (int)floor(ly0);
  int layerEndX = (int)floor(lx1)+1;
  int layerEndY = (int)floor(ly1)+1;
  if (layerStartX < 0) layerStartX = 0;
  if (layerStartY < 0) layerStartY = 0;
  if (layerEndX > l->width) layerEndX = l->width;
  if (layerEndY > l->height) layerEndY = l->height;
  Layer *clone = l->clone();
  Array<Vector> source;
  Array<Vector> dest;
  int pointCount = (int)modifyPointsSource.size();
  for (int i3 = 0; i3 < pointCount; i3++) {
    double lx,ly;
    lx = modifyPointsSource[i3].x; ly = modifyPointsSource[i3].y; currentCanvas->getLayerPosFromCanvasPos(l,lx,ly,lx,ly); source.push_back(Vector(floor(lx),floor(ly)));
    lx = modifyPointsDest[i3].x; ly = modifyPointsDest[i3].y; currentCanvas->getLayerPosFromCanvasPos(l,lx,ly,lx,ly); dest.push_back(Vector(floor(lx),floor(ly)));
  }
  int triangleCount = (int)modifyTriangles.size()/3;
  Array<double> values;
  values.resize(triangleCount*3);

  // generate homography if perspective mode is selected
  Matrix hmap;
  if (triangleCount == 2 && modifyPerspective) {
    int points[4];
    points[0] = modifyTriangles[0*3+0];
    points[1] = modifyTriangles[0*3+1];
    points[2] = modifyTriangles[0*3+2];
    points[3] = -1;
    for (int j = 0; j < 3; j++) {
      bool found = true;
      for (int k = 0; k < 3; k++) {
        if ((int)modifyTriangles[1*3+j]==(int)points[k]) {found = false; break;}
      }
      if (found) {points[3] = modifyTriangles[1*3+j]; break;}
    }
    bool valid = true; for (int l = 0; l < 4; l++) if (points[l] < 0 || points[l] >= pointCount) {valid = false; break;}
    if (valid) {
      double src[4*2]; double dst[4*2];
      for (int i = 0; i < 4; i++) {
        src[i*2+0] = source[points[i]].x;
        src[i*2+1] = source[points[i]].y;
        dst[i*2+0] = dest[points[i]].x;
        dst[i*2+1] = dest[points[i]].y;
      }
      hmap = perspectiveHomography(dst, src); // inverse
    }
  }

  initStatusBar();
  if (modifyClearBackground) {
    for (int y = layerStartY; y < layerEndY; y++) {
      if ((y & 15)==0) updateStatusBar();
      for (int x = layerStartX; x < layerEndX; x++) {
        Vector p(x,y);
        //double sum = 0;
        bool thisPixelShouldBeProcessed = false;
        for (int i = 0; i < triangleCount; i++) {
          Vector d[3];
          d[0] = (unsigned int)modifyTriangles[i*3+0]<(unsigned int)pointCount?source[modifyTriangles[i*3+0]]:Vector();
          d[1] = (unsigned int)modifyTriangles[i*3+1]<(unsigned int)pointCount?source[modifyTriangles[i*3+1]]:Vector();
          d[2] = (unsigned int)modifyTriangles[i*3+2]<(unsigned int)pointCount?source[modifyTriangles[i*3+2]]:Vector();
  
          bool allIn = true;
          for (int t = 0; t < 3; t++) {
            int t2 = (t+1)%3;
            double nx = d[t2].x-d[t].x;
            double ny = d[t2].y-d[t].y;
            double tx = ny;
            double ty = -nx;
            double dist = tx*(p.x-d[t].x)+ty*(p.y-d[t].y);
            if (dist < 0) allIn = false;
          }
          if (allIn) thisPixelShouldBeProcessed = true;
        }         
        if (thisPixelShouldBeProcessed) l->pixels[x+y*l->width] = 0;
      }
    }
  }

  Layer *destLayer = l; if (createNewLayer) {destLayer = l->clone(); destLayer->clear();}

  // meanValueCoordinates (don't work for inner latice points)
  double bary[3];
  int baryTriangle = 0;
  for (int y = layerStartY; y < layerEndY; y++) {
    if ((y & 15)==0) updateStatusBar();
    for (int x = layerStartX; x < layerEndX; x++) {
      Vector p(x,y);
      double sum = 0;
      bool thisPixelShouldBeProcessed = false;
      baryTriangle = -1;
      for (int i = 0; i < triangleCount; i++) {
        Vector d[3];
        d[0] = (unsigned int)modifyTriangles[i*3+0]<(unsigned int)pointCount?dest[modifyTriangles[i*3+0]]:Vector();
        d[1] = (unsigned int)modifyTriangles[i*3+1]<(unsigned int)pointCount?dest[modifyTriangles[i*3+1]]:Vector();
        d[2] = (unsigned int)modifyTriangles[i*3+2]<(unsigned int)pointCount?dest[modifyTriangles[i*3+2]]:Vector();

        bool allIn = true;
        for (int t = 0; t < 3; t++) {
          int t2 = (t+1)%3;
          double nx = d[t2].x-d[t].x;
          double ny = d[t2].y-d[t].y;
          double tx = ny;
          double ty = -nx;
          double dist = tx*(p.x-d[t].x)+ty*(p.y-d[t].y);
          if (dist < 0) allIn = false;
        }
        if (allIn) thisPixelShouldBeProcessed = true;
        if (allIn) {
          double w = 0;
          for (int t = 0; t < 3; t++) {
            int t2 = (t+1)%3;
            double nx = d[t2].x-d[t].x;
            double ny = d[t2].y-d[t].y;
            double tx = ny;
            double ty = -nx;
            double dist = tx*(p.x-d[t].x)+ty*(p.y-d[t].y);
            bary[t] = dist;
            w += bary[t];
          }
          if (w != 0) {bary[0]/=w;bary[1]/=w;bary[2]/=w;}
          baryTriangle = i;
        }

        if (modifyOutlined) {
          d[0] -= p;
          d[1] -= p;
          d[2] -= p;
          double tans[3];
          for (int j = 0; j < 3; j++) {
            int j2 = (j+1)%3;
            double r = length(d[j]);
            double r2 = length(d[j2]);
            if (r < 0.01) r = 0.01;
            if (r2 < 0.01) r2 = 0.01;
            double k = r*r2;
            double ang = 0;
            if (k != 0) {
              double dotp = (d[j].x*d[j2].x+d[j].y*d[j2].y)/k;
              double crossp = (d[j].x*d[j2].y-d[j].y*d[j2].x)/k;
              if (crossp != 0 || dotp != 0) ang = atan2(crossp,dotp);
            }
            tans[j] = tan(ang*0.5);
          }
          for (int j2 = 0; j2 < 3; j2++) {
            double r = length(d[j2]);
            if (r < 0.01) r = 0.01;
            values[i*3+j2] = (tans[(j2+3-1)%3]+tans[j2])/r;
            sum += values[i*3+j2];
          }
        }
      }

      if (thisPixelShouldBeProcessed) {
        double tx = 0, ty = 0;
        if (modifyOutlined) {
          if (sum != 0) {
            for (int k = 0; k < (int)values.size(); k++) {
              values[k] /= sum;
            }
  
            for (int i = 0; i < triangleCount; i++) {
              for (int j = 0; j < 3; j++) {
                double f = values[i*3+j];
                Vector d; d = (unsigned int)modifyTriangles[i*3+j]<(unsigned int)pointCount?source[modifyTriangles[i*3+j]]:Vector();
                tx += d.x * f;
                ty += d.y * f;
              }
            }
          }    
        } else {
          if (modifyPerspective) {
            Vector v(x,y);
            v = hmap * v;
            if (v.w != 0) {v /= v.w;}
            tx = v.x;
            ty = v.y;
          } else {
            if ((unsigned int)baryTriangle < (unsigned int)triangleCount) {
              tx = 0;
              ty = 0;
              for (int j = 0; j < 3; j++) {
                double f = bary[j];
                int j2 = (j + 3 - 1) % 3;
                Vector d; d = (unsigned int)modifyTriangles[baryTriangle*3+j2]<(unsigned int)pointCount?source[modifyTriangles[baryTriangle*3+j2]]:Vector();
                tx += d.x * f;
                ty += d.y * f;
              }
            }
          }
        }
  
        int txi = (int)floor(tx);
        int tyi = (int)floor(ty);
        if ((unsigned int)txi<(unsigned int)clone->width&&(unsigned int)tyi<(unsigned int)clone->height) {
          if (!modifyFiltered) {
            unsigned int col = clone->pixels[txi+tyi*clone->width];
            destLayer->pixels[x+y*destLayer->width] = col;
          } else {
            double fx = tx-txi;
            double fy = ty-tyi;
            unsigned int p00 = clone->pixels[txi+tyi*clone->width];
            unsigned int p10 = txi<clone->width-1?clone->pixels[txi+1+tyi*clone->width]:p00;
            unsigned int p01 = tyi<clone->height-1?clone->pixels[txi+(tyi+1)*clone->width]:p00;
            unsigned int p11 = txi<clone->width-1 && tyi<clone->height-1?clone->pixels[txi+1+(tyi+1)*clone->width]:p00;
            double rt = ((int)(p10 & 255) - (int)(p00 & 255))*fx + (int)(p00 & 255);
            double gt = ((int)((p10>>8) & 255) - (int)((p00>>8) & 255))*fx + (int)((p00>>8) & 255);
            double bt = ((int)((p10>>16) & 255) - (int)((p00>>16) & 255))*fx + (int)((p00>>16) & 255);
            double at = ((int)((p10>>24) & 255) - (int)((p00>>24) & 255))*fx + (int)((p00>>24) & 255);
            double rb = ((int)(p11 & 255) - (int)(p01 & 255))*fx + (int)(p01 & 255);
            double gb = ((int)((p11>>8) & 255) - (int)((p01>>8) & 255))*fx + (int)((p01>>8) & 255);
            double bb = ((int)((p11>>16) & 255) - (int)((p01>>16) & 255))*fx + (int)((p01>>16) & 255);
            double ab = ((int)((p11>>24) & 255) - (int)((p01>>24) & 255))*fx + (int)((p01>>24) & 255);
            int r = (int)floor((rb - rt) * fy + rt);
            int g = (int)floor((gb - gt) * fy + gt);
            int b = (int)floor((bb - bt) * fy + bt);
            int a = (int)floor((ab - at) * fy + at);
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            if (a > 255) a = 255;
            if (r < 0) r = 0;
            if (g < 0) g = 0;
            if (b < 0) b = 0;
            if (a < 0) a = 0;
            destLayer->pixels[x+y*destLayer->width] = r|(g<<8)|(b<<16)|(a<<24);
          }
        }
      }
    }
  }
  modifyPointsDest.clear();
  modifyMode = false;
  updatedPixels(l);

  delete clone;
  repaintLayer(l,true);
  afterUndoPoint();

  if (createNewLayer) {
    destLayer->cropByAlpha(1); sprintf(destLayer->name,"modify"); destLayer->state.active = false; destLayer->state.selected = false;
    int pos = 0; for (int i = 0; i < (int)currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.active || currentCanvas->layers[i]->state.selected) {pos = i; break;}
    currentCanvas->insertLayer(destLayer,pos);
    updatedPixels(destLayer);
    repaintLayer(destLayer,true);
  }
}

void useToolModify() {
  if (!isPaintAreaActive()) return;

  static bool lastMouseDown = false;
  static bool clickEnabled = false;
  static double startMouseX=0;
  static double startMouseY=0;
  bool clicked = false;
  static bool prohibit = false;
  if (ImGui::IsMouseDown(0)) {
    if (!lastMouseDown) {
      startMouseX = mouseX;
      startMouseY = mouseY;
      clickEnabled = true;
      prohibit = false;
    }
    double dx = mouseX - startMouseX;
    double dy = mouseY - startMouseY;
    double d = sqrt(dx*dx+dy*dy);
    if (d > 4) clickEnabled = false;
    lastMouseDown = true;
  } else {
    if (lastMouseDown) clicked = true;
    lastMouseDown = false;
  }

  if (ImGui::IsMouseDown(1)) {
    prohibit = true;
  }

  Array<Vector> *points = modifyMode ? &modifyPointsDest : &modifyPointsSource;

  static double lastClickDist = 0;
  if (ImGui::IsMouseClicked(0)) {
    double cx = mouseX, cy = mouseY; currentCanvas->getCanvasPos(cx,cy,cx,cy);
    int found = -1;
    double ld = 0;
    for (int i = 0; i < (int)points->size(); i++) {
      double dx = (*points)[i].x - cx;
      double dy = (*points)[i].y - cy;
      double d = dx*dx+dy*dy;
      if (d < ld || i == 0) {
        ld = d;
        found = i;
      }
    }
    lastClickDist = sqrt(ld);
    selectedModifyPoint = found;
  }

  if (clicked && clickEnabled) {
    if (!prohibit && (lastClickDist > 4 || points->empty())) {
      if (!modifyMode) {
        double cx = mouseX, cy = mouseY; currentCanvas->getCanvasPos(cx,cy,cx,cy);
        points->push_back(Vector(cx,cy));
        selectedModifyPoint = (int)points->size()-1;
        createModifyTriangles();
      }
    }
  }

  if (ImGui::IsMouseDown(0) && (!clickEnabled)) {
    if (!prohibit) {
      if ((unsigned int)selectedModifyPoint<points->size()) {
        double cx = mouseX, cy = mouseY; currentCanvas->getCanvasPos(cx,cy,cx,cy);
        (*points)[selectedModifyPoint].x = cx;
        (*points)[selectedModifyPoint].y = cy;
        if (!modifyMode) createModifyTriangles();
      }
    }
  }
}

void useToolTileMap() {
  if (!isPaintAreaActive()) return;

  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (l->type != LAYERTYPE_TILEMAP) return;
  Layer *l2 = currentCanvas->getLayerById(l->tileSourceLayerId);
  if (l2 == NULL || l2->width <= 0 || l2->height <= 0 || l2->toolInfo.tilingWidth < 1 || l2->toolInfo.tilingHeight < 1) return;
  double lx = mouseX, ly = mouseY; currentCanvas->getLayerPos(l,lx,ly,lx,ly);
  int lxi = (int)floor(lx); int lyi = (int)floor(ly);
  int txi = (int)floor((double)lxi/l2->toolInfo.tilingWidth);
  int tyi = (int)floor((double)lyi/l2->toolInfo.tilingHeight);
  if ((unsigned int)txi < (unsigned int)l->width && (unsigned int)tyi < (unsigned int)l->height) {
    static bool wasDown = false;
    static int lastTxi = -1;
    static int lastTyi = -1;
    static bool first = false;
    if (ImGui::IsMouseClicked(0)) {
      beforeUndoPoint();
      wasDown = false;
      lastTxi = txi;
      lastTyi = tyi;
      first = true;
    }
    if (ImGui::IsMouseDown(1)) {l->toolInfo.tileMapSelectedTile = l->pixels[txi+tyi*l->width]; scrollToTileMapIndex = l->toolInfo.tileMapSelectedTile; return;}
    if (ImGui::IsMouseDown(0)) {
      wasDown = true;
      double px = lastTxi;
      double py = lastTyi;
      int dx = txi-lastTxi;
      int dy = tyi-lastTyi;
      lastTxi = txi;
      lastTyi = tyi;
      int d = abs(dx);
      if (abs(dy)>d) d = abs(dy);
      if (d != 0 || first) {
        first = false;
        double xa = d == 0 ? 0 : (double)dx / d;
        double ya = d == 0 ? 0 : (double)dy / d;
        bool hasSelection = l->hasSelection();
        for (int i = 0; i < d+1; i++) {
          txi = (int)floor(px);
          tyi = (int)floor(py);
          if ((unsigned int)txi < (unsigned int)l->width && (unsigned int)tyi < (unsigned int)l->height) {
            if ( (!hasSelection) || (l->pixelStates[txi+tyi*l->width] & PIXELSTATE_SELECTED)) {
              l->pixels[txi+tyi*l->width] = l->toolInfo.tileMapSelectedTile;
              double cx0 = txi, cy0 = tyi; currentCanvas->getCanvasPosFromLayerPos(l,cx0,cy0,cx0,cy0);
              double cx1 = txi+1, cy1 = tyi+1; currentCanvas->getCanvasPosFromLayerPos(l,cx1,cy1,cx1,cy1);
              for (int y = (int)cy0; y < (int)cy1; y++) {
                for (int x = (int)cx0; x < (int)cx1; x++) {
                  if ((unsigned int)x < (unsigned int)currentCanvas->width && (unsigned int)y < (unsigned int)currentCanvas->height) {
                    currentCanvas->pixelStates[x+y*currentCanvas->width] |= PIXELSTATE_UPDATED;
                  }
                }
              }
            }
          }
          px += xa;
          py += ya;
        }
        updatedPixels(l);
        repaintLayer(l, false);
      }
    } else {if (wasDown) {repaintLayer(l, false); afterUndoPoint();} wasDown = false;}
  }
}

void useTool() {
  double mouseXBefore = mouseX;
  double mouseYBefore = mouseY;
  double mouseDXBefore = mouseDX;
  double mouseDYBefore = mouseDY;
  if (lockedMouseX) mouseX = lockedMousePosX;
  if (lockedMouseY) mouseY = lockedMousePosY;
  if (lockedMouseX) mouseDX = 0;
  if (lockedMouseY) mouseDY = 0;
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
  case TOOL_LINE: {
    useToolPainting();
    } break;
  case TOOL_GRADIENT: {
    useToolGradient();
    } break;
  case TOOL_MEASURE: {
    useToolGradient();
    } break;
  case TOOL_TILING: {
    useToolTiling();
    } break;
  case TOOL_BLUR: {
    useToolBlur();
    } break;
  case TOOL_OVERLAYS: {
    useToolSelection();
    } break;
  case TOOL_MODIFY: {
    useToolModify();
    } break;
  case TOOL_TILEMAP: {
    useToolTileMap();
    } break;
  }
  mouseX = mouseXBefore;
  mouseY = mouseYBefore;
  mouseDX = mouseDXBefore;
  mouseDY = mouseDYBefore;
}
