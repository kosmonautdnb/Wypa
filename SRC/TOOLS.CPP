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
#include "STL/IMAGE.HPP"
#include "UNDO.HPP"
#include "STL/CHARSET.HPP"
#include <math.h>


Array<Brush *> brushes;
int currentBrush = 0;

Brush *onePixelBrush = NULL;

Gradient gradient;
RGBAImage copiedTiling;

Matrix inverse2(const Matrix &a);

void initTools() {
  onePixelBrush = new Brush();
  onePixelBrush->size = 1;
}

void freeTools() {
  for (int i = 0; i < brushes.size(); i++) delete brushes[i];
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
  int x2 = -width * anchorX;
  int y2 = -height * anchorY;
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
            for (int x4 = x0; x4 < x1; x4++) {
              int x5 = xp + x4;
              int y5 = yp + y4;
              if ((unsigned int)x5<glFrameBufferWidth && (unsigned int)y5<glFrameBufferHeight) glFrameBuffer[x5+y5*glFrameBufferWidth] = color;
            }
          }
        }
      }
    }
    x2 += l->preAddX + l->width + l->postAddX + 1;
    s++;
  }
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
  if ((unsigned int)x >= fieldWidth || (unsigned int)y >= fieldHeight) return -1;
  float p00 = field[x+y*fieldWidth];
  float p10 = x+1<fieldWidth?field[x+1+y*fieldWidth]:p00;
  float p01 = y+1<fieldHeight?field[x+(y+1)*fieldWidth]:p00;
  float p11 = x+1<fieldWidth&&y+1<fieldHeight?field[x+1+(y+1)*fieldWidth]:p00;
  float xf = dx - x;
  float yf = dy - y;
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
  double k = 0.1;
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
      values[x+y*pixelWidth] = d;
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
  for (int i = 0; i < temp.size(); i++) {double a = values[i]; if (a<0) a = 0; if (a<minv) minv = a;if (a>maxv) maxv = a;}
  if (maxv!=minv) {
    double d = 255.0/(maxv-minv)*base->opacity;
    for (int i2 = 0; i2 < temp.size(); i2++) {temp[i2] = values[i2]>=0?(values[i2]-minv)*d:0;}
  } else {
    for (int i2 = 0; i2 < temp.size(); i2++) {temp[i2] = 255*maxv*base->opacity;}
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
  if ((unsigned int)px >= image.width || (unsigned int)py >= image.height) return 0;
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
      if ((unsigned int)dx<image.width&&(unsigned int)dy<image.height) {
        bool isForward = scanSampleDistanceFieldIsForward(image,dx,dy);
        for (int rad = 0; rad <= maxRad; rad++) {
          for (int x0 = -rad; x0 <= rad; x0++) {
            int y0;
            y0 = rad;
            if (scanSampleDistanceField(image,x0+dx,y0+dy,isForward)) {
              float d = sqrt(x0*x0+y0*y0); if (distance < 0 || d < distance) distance = d;
            }
            y0 = -rad;
            if (scanSampleDistanceField(image,x0+dx,y0+dy,isForward)) {
              float d = sqrt(x0*x0+y0*y0); if (distance < 0 || d < distance) distance = d;
            }
          }
          for (int y0 = -rad; y0 <= rad; y0++) {
            int x0;
            x0 = rad;
            if (scanSampleDistanceField(image,x0+dx,y0+dy,isForward)) {
              float d = sqrt(x0*x0+y0*y0); if (distance < 0 || d < distance) distance = d;
            }
            x0 = -rad;
            if (scanSampleDistanceField(image,x0+dx,y0+dy,isForward)) {
              float d = sqrt(x0*x0+y0*y0); if (distance < 0 || d < distance) distance = d;
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
  bool right = index + 1 < gradient->elements.size() ? value < gradient->elements[index+1].position : false;
  return left && right;
}

__inline unsigned int interpolateRange(Gradient *gradient, int index, float value) {
  if (gradient->elements.empty()) return 0x00000000;
  if (index < 0) return gradient->elements[0].color;
  if (index >= gradient->elements.size()) return gradient->elements.back().color;
  GradientElement *e0 = &gradient->elements[index];
  if (e0->isStep) return e0->color;
  if (index + 1 >= gradient->elements.size()) return e0->color;
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
  int r = (int)(r1 - r0) * k + r0;
  int g = (int)(g1 - g0) * k + g0;
  int b = (int)(b1 - b0) * k + b0;
  int a = (int)(a1 - a0) * k + a0;
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
    for (; lastAccessPoint < elements.size(); lastAccessPoint++) {
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
    value = sqrt(dx*dx+dy*dy);
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
  return (!ImGui::GetIO().WantCaptureMouse) && (!ImGui::GetIO().WantCaptureKeyboard);
}

Array<unsigned short> quantization;

void reCreateQuantization() {
  if (palette.size()>256 || palette.empty()) {quantization.clear(); return;}
  if (quantization.empty()) quantization.resize(64*64*64);
  Array<unsigned int> colors; colors.resize(palette.size());
  for (int i = 0; i < palette.size(); i++) colors[i] = getUIntColor(palette[i]);
  for (int r = 0; r < 64; r++) {
    int rs = r * 4;
    for (int g = 0; g < 64; g++) {
      int gs = g * 4;
      for (int b = 0; b < 64; b++) {
        int bs = b * 4;
        int found = 0;
        int ld = 256*256*5;
        for (int i = 0; i < palette.size(); i++) {
          unsigned int col = colors[i];
          int rd = (int)(col & 255)-rs;
          int gd = (int)((col>>8) & 255)-gs;
          int bd = (int)((col>>16) & 255)-bs;
          int dist = rd*rd+gd*gd+bd*bd;
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

  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return; if (l->state.locked) return;

  beforeUndoPoint();

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
    for (int i = 0; i < l->width*l->height; i++) {bool yes = isWandAccepted(l->pixels[i],colorHere); if (yes) {
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
    if (currentKey == GL_VK_LEFT) moveSelectedLayersByCanvasPixels(-moveX,0);
    if (currentKey == GL_VK_RIGHT) moveSelectedLayersByCanvasPixels(moveX,0);
    if (currentKey == GL_VK_UP) moveSelectedLayersByCanvasPixels(0,-moveY);
    if (currentKey == GL_VK_DOWN) moveSelectedLayersByCanvasPixels(0,moveY);
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

void useToolFill() {
  if (!isPaintAreaActive()) return;

  mouseClickUndo();

  if (!ImGui::IsMouseClicked(0)) return;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return; if (l->state.locked) return;
  int lx, ly; double lx2, ly2; currentCanvas->getLayerPos(l, mouseX, mouseY, lx2, ly2); lx = (int)floor(lx2); ly = (int)floor(ly2);
  if ((unsigned int)lx>=l->width||(unsigned int)ly>=l->height) return;
  int colorHere = l->pixels[lx+ly*l->width];
  
  fillAcceptedHasSelection = l->hasSelection();

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

void useToolSelection() {
  if (wasActiveLayerSwitched()) {selectionToolActive = false; showSelectionPlane = false; showSelectionPlaneBounding = false; toolW = -1; toolH = -1; return;}

  Layer *activeLayer = currentCanvas->getActiveLayer(); if (activeLayer != NULL && activeLayer->state.locked) return;
  static Layer *selectionToolLayer = activeLayer;
  selectionToolLayer = activeLayer;
  if (selectionToolLayer != NULL && selectionToolLayer->width * selectionToolLayer->height==0) return;

  mouseClickUndo();

  int selectionTool2 = tool == TOOL_OVERLAYS ? SELECTIONTOOL_RECTANGLE : selectionTool;

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
            if (selectionTool2 == SELECTIONTOOL_LASSO) selectionPlane->pixelXLineEor(lx0,ly0,lx1,ly1,1,true);
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
    if (selectionProportional) if (lx-selectionX0>ly-selectionY0) {
      selectionY1 = selectionY0+(lx-selectionX0)*pixelAspect; 
    } else {
      selectionX1 = selectionX0+(ly-selectionY0)/pixelAspect;
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
    showSelectionPlaneBounding = selectionTool2 == SELECTIONTOOL_ELLIPSE;
    switch(selectionTool2) {
      case SELECTIONTOOL_ELLIPSE:
      case SELECTIONTOOL_RECTANGLE: {
        toolW = x1 - x0;
        toolH = y1 - y0;
        selectionPlane->clear();
        Matrix m = inverse2(selectionPlane->transform.transformation);
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
              if (selectionTool2 == SELECTIONTOOL_ELLIPSE) {
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
        selectionPlane->pixelLineOr(lx0,ly0,lx1,ly1,2,true);
        selectionPlane->pixelXLineEor(lx0,ly0,lx1,ly1,1,true);
      } break;
      case SELECTIONTOOL_POLYGON: {
        double lx0,ly0; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,lastSelectionPolygonX1,lastSelectionPolygonY1,lx0,ly0);
        double lx1,ly1; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,selectionX1,selectionY1,lx1,ly1);
        double lx2,ly2; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,lastSelectionX1,lastSelectionY1,lx2,ly2);
        double lx3,ly3; currentCanvas->getLayerPosFromCanvasPos(selectionPlane,selectionX0,selectionY0,lx3,ly3);
        if (ImGui::IsMouseDown(1)) {
          if (pointCount == 0) {selectionPlane->pixelXLineEor(lx0,ly0,lx2,ly2,1,true); selectionPlane->pixelLineEor(lx0,ly0,lx2,ly2,2,true);}
          lastSelectionPolygonX1 = lx;
          lastSelectionPolygonY1 = ly;
          pointCount++;
        } 
        selectionPlane->pixelXLineEor(lx0,ly0,lx2,ly2,1,true); // delete
        selectionPlane->pixelXLineEor(lx0,ly0,lx1,ly1,1,true); // paint
        selectionPlane->pixelLineEor(lx0,ly0,lx2,ly2,2,true);
        selectionPlane->pixelLineEor(lx0,ly0,lx1,ly1,2,true);
        if (pointCount > 0) {
          selectionPlane->pixelXLineEor(lx3,ly3,lx2,ly2,1,true); // delete
          selectionPlane->pixelXLineEor(lx3,ly3,lx1,ly1,1,true); // paint
          selectionPlane->pixelLineEor(lx3,ly3,lx2,ly2,2,true);
          selectionPlane->pixelLineEor(lx3,ly3,lx1,ly1,2,true);
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
        unsigned int col = gradient->getColor2D(gradx,grady);
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
  Layer *activeLayer = currentCanvas->getActiveLayer(); if (activeLayer != NULL && activeLayer->state.locked) return;
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
    selectionPlane->pixelLineEor(lx0,ly0,lx2,ly2,1,true);
    selectionPlane->pixelLineEor(lx0,ly0,lx1,ly1,1,true);
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

void drawTool(Layer *layer, Brush *brush,double canvasX, double canvasY, double angle, bool justSelection, bool paintJustIntoOutline) {
   if (layer->state.locked) return;

  double xs = (rand() & 1023)/1023.0*2.0-1.0;
  double ys = (rand() & 1023)/1023.0*2.0-1.0;
  canvasX += xs * brush->randomX * toolAspectX;
  canvasY += ys * brush->randomY * toolAspectY;
  bool pencil = tool == TOOL_PENCIL;
  BrushInstance *b = brush->getCreateInstance(BRUSHINSTANCE_PAINT,brush->pixelWidth(),brush->pixelHeight(),pencil);
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
  if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = layerBoundings.x;
  if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = layerBoundings.y;
  if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = layerBoundings.z;
  if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = layerBoundings.w;
  double tpx = canvasX-toolWidth, tpy = canvasY-toolHeight;
  Matrix t; t.translate(Vector(tpx,tpy,0));
  Matrix m = inverse2(t)*inverse2(layer->transform.transformation);
  bool interpolate = false;
  if (brush->followRotation) {
    interpolate = true;
    if (angle >= 0) {b->followedRotation = angle;}
    Matrix toCenter; toCenter.translate(Vector(toolWidth,toolHeight,0));
    m = toCenter * rotationMatrixAngleAxis(b->followedRotation, 0, 0, 1) * inverse2(toCenter) * m;
  }
  Matrix s; s.scale3(Vector(toolAspectX,toolAspectY,1));
  m = inverse2(s) * m;
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
          if (!justSelection || (layer->pixelStates[p] & PIXELSTATE_SELECTED)) {
            if (paintJustIntoOutline) {
              layer->pixelStates[p] |= PIXELSTATE_OUTLINE;
            } else {
              paintValues[p] = value;
            }
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
  if (layer->state.locked) return;

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
  if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = layerBoundings.x;
  if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = layerBoundings.y;
  if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = layerBoundings.z;
  if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = layerBoundings.w;

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
        float value = paintValues[p];
        value = 1-(1-here)*(1-value);
        if (!justSelection || (layer->pixelStates[p] & PIXELSTATE_SELECTED)) {
          paintValues[p] = value;
        }
      }
    }
  }
}

void updateUpdateFlagByBrushLine(Layer *l,double cx0,double cy0, double cx1, double cy1, double toolWidth, double toolHeight) {
  if (l->state.locked) return;
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
  if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = layerBoundings.x;
  if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = layerBoundings.y;
  if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = layerBoundings.z;
  if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = layerBoundings.w;

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
  if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = layerBoundings.x;
  if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = layerBoundings.y;
  if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = layerBoundings.z;
  if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = layerBoundings.w;

  static int someTimer = 0; someTimer++; if ((someTimer & 3)==0) currentCanvas->clearUpdateFlag();
  for (int y2 = layerUpdateY0; y2 < layerUpdateY1; y2++) {
    for (int x2 = layerUpdateX0; x2 < layerUpdateX1; x2++) {
      currentCanvas->pixelStates[x2+y2*currentCanvas->width] |= PIXELSTATE_UPDATED;
    }
  }
}

void useToolPainting() {

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
  Layer *activeLayer = currentCanvas->getActiveLayer(); if (activeLayer != NULL && activeLayer->state.locked) return;
  static Layer *paintToolLayer = activeLayer;
  bool layerSwitched = paintToolLayer != activeLayer;
  paintToolLayer = activeLayer;
  if (paintToolLayer != NULL && paintToolLayer->width * paintToolLayer->height==0) return;

  if (paintToolLayer != NULL && (!paintingOn) && toolOutline && isPaintAreaActive()) {
    double cx,cy; currentCanvas->getCanvasPos(mouseX,mouseY,cx,cy);
    cx = floor(cx);
    cy = floor(cy);
    paintToolLayer->updatePixelStates(~PIXELSTATE_OUTLINE,0,0); reverseLastUpdatedPixelStates(paintToolLayer);
    drawTool(paintToolLayer,brush,cx,cy,0,false,true);

    double toolWidth = (brush->pixelWidth()*0.5+brush->randomX) * toolAspectX;
    double toolHeight = (brush->pixelHeight()*0.5+brush->randomY) * toolAspectY;
    updateUpdateFlagByBrushLine(paintToolLayer,cx,cy,cx,cy,toolWidth,toolHeight);

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
    switch(tool) {
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
    switch(tool) {
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
          double angle = -1,dx,dy;
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
            drawTool(paintToolLayer,brush,cx,cy,angle,justSelection,false);
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
        memset(&paintValues[0],0,paintToolLayer->width * paintToolLayer->height * sizeof(float));
        drawLine(paintToolLayer,paintingPointX0,paintingPointY0,paintingPointX1,paintingPointY1,justSelection);
      } break;
    }

    for (int y = layerUpdateY0; y < layerUpdateY1; y++) {
      for (int x = layerUpdateX0; x < layerUpdateX1; x++) {
        int p = x + y * paintToolLayer->width;
        float value = paintValues[p];
        unsigned int col = paintBackground[p];
        switch(tool) {
          case TOOL_LINE:
          case TOOL_PAINTBRUSH:
          case TOOL_MAGIC:
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
            switch(paintToolLayer->mode) {
              case LAYERMODE_PREMULTIPLIEDALPHA: {
                  int r = col & 255;
                  int g = (col>>8) & 255;
                  int b = (col>>16) & 255;
                  int a = (col>>24) & 255;
                  r *= 1-value;
                  g *= 1-value;
                  b *= 1-value;
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

float FXAA_REDUCE_MIN = 1.f/128.f;
float FXAA_REDUCE_MUL = 1.f/(8.f*(1.f/2.f));
float FXAA_SPAN_MAX = 8.f;

static __inline const float luma(const unsigned int rgba) {
  return (float)((rgba & 255)*25+(float)((rgba>>8) & 255)*60+(float)((rgba>>16) & 255)*15)*(1.f/255.f/100.f);
}

__inline unsigned int tex(unsigned int *source, const int width, const int height, const float tx, const float ty) {
  int itx = (int)(tx); // tx,ty not below 0 (trunc)
  int ity = (int)(ty);
  const float fx = tx - (float)itx;
  const float fy = ty - (float)ity;
  const int txf = (int)(fx * 256.0); // fx,fy not below 0 (trunc)
  const int tyf = (int)(fy * 256.0);
  if (itx<0) itx = 0;
  if (ity<0) ity = 0;
  if (itx>=width-1) itx = width-2;
  if (ity>=height-1) ity = height-2;
  const int p1v = ((256-txf)*(256-tyf))>>8; 
  const int p2v = ((txf)*(256-tyf))>>8; 
  const int p3v = ((txf)*(tyf))>>8; 
  const int p4v = ((256-txf)*(tyf))>>8;
  const unsigned int rgba00 = source[itx+ity*width];
  const unsigned int rgba10 = source[itx+1+ity*width];
  const unsigned int rgba01 = source[itx+(ity+1)*width];
  const unsigned int rgba11 = source[itx+1+(ity+1)*width];
  unsigned int rgba1 = (rgba00 & 0x0000ff00)*p1v;
  rgba1 += (rgba10 & 0x0000ff00)*p2v;
  rgba1 += (rgba11 & 0x0000ff00)*p3v;
  rgba1 += (rgba01 & 0x0000ff00)*p4v;
  unsigned int rgba2 = (rgba00 & 0x00ff00ff)*p1v;
  rgba2 += (rgba10 & 0x00ff00ff)*p2v;
  rgba2 += (rgba11 & 0x00ff00ff)*p3v;
  rgba2 += (rgba01 & 0x00ff00ff)*p4v;
  return ((rgba1 & 0x00ff0000)+(rgba2 & 0xff00ff00))>>8;
}

void doFXAA(unsigned int *sourceBuffer, const int width, const int height, unsigned char *maskBuffer, unsigned char mask) {
  unsigned int *tempBuffer = sourceBuffer;
  float *tr = new float[width];

  float *k = tr;
  const float fy = 0;
  for (int x = 0; x < width; x++) {
    const float fx = x;
    *k++=luma(tex(sourceBuffer,width,height,fx+0.5f,fy-0.5f));
  }
  for (int y = 0; y < height; y++) {
    unsigned int *source = &sourceBuffer[y*width];
    unsigned int *dest = &tempBuffer[y*width];
    const float fy = y;
    const float fx = 1;
    float lumaNW = luma(tex(sourceBuffer,width,height,fx-0.5f,fy-0.5f));
    float lumaSW = luma(tex(sourceBuffer,width,height,fx-0.5f,fy+0.5f));
    k = tr;
    for (int x = 0; x < width; x++) {
      const float fx = x;

      const float lumaM = luma(*source);
      const float lumaNE = *k;
      const float lumaSE = luma(tex(sourceBuffer,width,height,fx+0.5f,fy+0.5f));

      if ((maskBuffer[x+y*width] & mask)==mask) {
        const float lumaMin = minf(lumaM, minf(minf(lumaNW, lumaNE), minf(lumaSW,lumaSE)));
        const float lumaMax = maxf(lumaM, maxf(maxf(lumaNW, lumaNE), maxf(lumaSW,lumaSE)));
        const float dx = -((lumaNW+lumaNE)-(lumaSW+lumaSE));
        const float dy = ((lumaNW+lumaSW)-(lumaNE+lumaSE));
        const float dirReduce = maxf((lumaNW+lumaNE+lumaSW+lumaSE)*(0.25f*FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
        const float rcpDirMin=1.0/(minf(fabs(dx),fabs(dy))+dirReduce);
        const float dirx = minf(FXAA_SPAN_MAX,maxf(-FXAA_SPAN_MAX,dx*rcpDirMin));
        const float diry = minf(FXAA_SPAN_MAX,maxf(-FXAA_SPAN_MAX,dy*rcpDirMin));
    
        const float da1 = 1.0/3.0-0.5;
        const float da2 = 2.0/3.0-0.5;
        const float db1 = 0.0/3.0-0.5;
        const float db2 = 3.0/3.0-0.5;
        const unsigned int rgbA1 = tex(sourceBuffer,width,height,fx+dirx*da1,fy+diry*da1);
        const unsigned int rgbA2 = tex(sourceBuffer,width,height,fx+dirx*da2,fy+diry*da2);
        const unsigned int rgbB1 = tex(sourceBuffer,width,height,fx+dirx*db1,fy+diry*db1);
        const unsigned int rgbB2 = tex(sourceBuffer,width,height,fx+dirx*db2,fy+diry*db2);
        const unsigned int rgbA = ((rgbA1 & 0x00fefefe)+(rgbA2 & 0x00fefefe))>>1;
        const unsigned int rgbB = ((rgbA & 0x00fefefe)>>1)+(((rgbB1 & 0x00fcfcfc)+(rgbB2 & 0x00fcfcfc))>>2);
        const float lumaB = luma(rgbB);
        if ((lumaB<lumaMin)||(lumaB>lumaMax)) {
          *dest = rgbA|(*dest & 0xff000000);
        } else {
          *dest = rgbB|(*dest & 0xff000000);
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
  if (l->state.locked) return ret;
  if (sx < 0) {width += sx; sx = 0;}
  if (sy < 0) {height += sy; sy = 0;}
  if (width > l->width) width = l->width;
  if (height > l->height) height = l->height;
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

void putImageToLayer(Layer *l, const RGBAImage &c, int sx,int sy) {
  if (c.data == NULL || c.width <= 0 || c.height <= 0) return;
  if (l == NULL) return;
  if (l->state.locked) return;
  for (int y = 0; y < c.height; y++) {
    for (int x = 0; x < c.width; x++) {
      int dx = sx + x;
      int dy = sy + y;
      if ((unsigned int)dx < l->width && (unsigned int)dy < l->height) l->pixels[dx+dy*l->width] = c.data[x+y*c.width];
    }
  }
  updatedPixels(l);
}

void useToolTiling() {
  if (!isPaintAreaActive()) return;
  Layer *l = currentCanvas->getActiveLayer(); if (l != NULL && l->state.locked) return;
  if (l == NULL) return;
  mouseClickUndo();
  double lx = mouseX, ly = mouseY; currentCanvas->getLayerPos(l,lx,ly,lx,ly);
  int sx = 0, sy = 0;
  if (l->toolInfo.tilingWidth>0) sx = (int)floor((lx-l->toolInfo.tilingOffsetX)/l->toolInfo.tilingWidth)*l->toolInfo.tilingWidth;
  if (l->toolInfo.tilingHeight>0) sy = (int)floor((ly-l->toolInfo.tilingOffsetY)/l->toolInfo.tilingHeight)*l->toolInfo.tilingHeight;
  if (ImGui::IsMouseClicked(0)) {
    if (controlKeyId == CONTROLKEY_SHIFT) {
      l->selectNone();
      l->addSelectionRectangle(sx,sy,sx+l->toolInfo.tilingWidth,sy+l->toolInfo.tilingHeight);
    } else {
      copiedTiling.free();
      copiedTiling = getImageFromLayer(l,sx,sy,l->toolInfo.tilingWidth,l->toolInfo.tilingHeight);
      updateTilingImage(copiedTiling);
    }
  }
  if (ImGui::IsMouseDown(1)) {
    putImageToLayer(l,copiedTiling,sx,sy);
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
  bool first = true; for (int y = 0; y < height; y++) { for (int x = 0; x < width; x++) {
      char c = bitmap[x+y*width];
      if (c != '0') {unsigned int color = 0xffffffff;
         if (c == '9') color = 0xff000000;
         if (c == '6') color = 0xffffffff;
        int dx = x+mouseX;
        int dy = y+mouseY;
        if ((unsigned int)dx < glFrameBufferWidth && (unsigned int)dy < glFrameBufferHeight) glFrameBuffer[dx+dy*glFrameBufferWidth]=color;
        first = false;
      }
    }
  }
}

void toolCanvasStuff() {
  switch(tool) {
  case TOOL_TILING: {
      Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) break; if (l->state.locked) return;
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
      float scale = currentCanvas->control.zoom/100.f;
      if (scale < 1) scale = 1;
      if (scale > 4) scale = 4;
      if (l->toolInfo.showTilingGridIndices) {
        if (cx >= 0 && cy >= 0 && cx < tcx && cy < tcy) {
          drawText(scale, lx+1, ly+1, String::fromInt(index).c_str(), 0xff000000, 0.5, 0.5);
          drawText(scale, lx, ly, String::fromInt(index).c_str(), 0xffffffff, 0.5, 0.5);
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
          float scale = currentCanvas->control.zoom/100.f;
          if (scale < 1) scale = 1;
          if (scale > 4) scale = 4;
          drawText(scale, lx+1, ly+1, String::fromInt(index).c_str(), 0xff0000ff, 0.5, 0.5);
          drawText(scale, lx, ly, String::fromInt(index).c_str(), 0xff000000, 0.5, 0.5);
        }

      }

    } break;
  }

  if (lockedMouseX) {
    int x = lockedMousePosX;
    if (x >= 0 && x < glFrameBufferWidth) {
      for (int y = 0; y < glFrameBufferHeight; y++)
        glFrameBuffer[x+y*glFrameBufferWidth]=0xff0000ff;
    }
    drawMouseArrow2(x,mouseY);
  }
  if (lockedMouseY) {
    int y = lockedMousePosY;
    if (y >= 0 && y < glFrameBufferHeight) {
      for (int x = 0; x < glFrameBufferWidth; x++)
        glFrameBuffer[x+y*glFrameBufferWidth]=0xff0000ff;
    }
    drawMouseArrow2(mouseX,y);
  }
}

int blurCircleSize = 64;

void drawCircle(Layer *layer, double canvasX, double canvasY, bool justSelection, bool paintJustIntoOutline) {
  if (layer == NULL) return;
  if (layer->state.locked) return;

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
  if (layerBoundings.x > layerUpdateX0) layerUpdateX0 = layerBoundings.x;
  if (layerBoundings.y > layerUpdateY0) layerUpdateY0 = layerBoundings.y;
  if (layerBoundings.z < layerUpdateX1) layerUpdateX1 = layerBoundings.z;
  if (layerBoundings.w < layerUpdateY1) layerUpdateY1 = layerBoundings.w;
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
        if ((unsigned int)bx < blurCircleSize && (unsigned int)by < blurCircleSize) {
          paintBackground[bx+by*blurCircleSize] = layer->pixels[x+y*layer->width];
          paintValues[bx+by*blurCircleSize] = 1;
        }
      }
    }
  }

  for (int y = layerUpdateY0; y < layerUpdateY1; y++) {
    for (int x = layerUpdateX0; x < layerUpdateX1; x++) {
      double px = x*m.m[4*0+0]+y*m.m[4*1+0]+m.m[4*3+0];
      double py = x*m.m[4*0+1]+y*m.m[4*1+1]+m.m[4*3+1];
      int bx = (int)px;
      int by = (int)py;
      if (bx == 0) bx = (int)floor(px);
      if (by == 0) by = (int)floor(py);
      if ((unsigned int)bx < blurCircleSize && (unsigned int)by < blurCircleSize) {
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
              for (int ky = -1; ky <= 1; ky++) {
                int y2 = by+ky;
                for (int kx = -1; kx <= 1; kx++) {
                  int x2 = bx+kx;
                  if ((unsigned int)x2<blurCircleSize && (unsigned int)y2<blurCircleSize) {
                    if (paintValues[x2+y2*blurCircleSize] != 0) {
                      float where = 1000.0;
                      if (where > 0) {
                        unsigned int col = paintBackground[x2+y2*blurCircleSize];
                        r += ((col & 255)+0.5f)*where;
                        g += (((col>>8) & 255)+0.5f)*where;
                        b += (((col>>16) & 255)+0.5f)*where;
                        a += (((col>>24) & 255)+0.5f)*where;
                        w += where;
                      }
                    }
                  }
                }
              }
              if (w != 0) {
                r /= w;
                g /= w;
                b /= w;
                a /= w;
              }
              unsigned int col = layer->pixels[p];
              int r2 = col & 255;
              int g2 = (col>>8) & 255;
              int b2 = (col>>16) & 255;
              int a2 = (col>>24) & 255;
              float f = 1-d;
              if (f > 0) {
                f = f*timeDelta*2;
                if (f > 1) f = 1;
                r = (r - r2)*f+r2;
                g = (g - g2)*f+g2;
                b = (b - b2)*f+b2;
                a = (a - a2)*f+a2;
                layer->pixels[p] = r|(g<<8)|(b<<16)|(a<<24);
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

  Layer *activeLayer = currentCanvas->getActiveLayer(); if (activeLayer != NULL && activeLayer->state.locked) return;
  static Layer *blurToolLayer = activeLayer;
  blurToolLayer = activeLayer;
  if (blurToolLayer != NULL && blurToolLayer->width * blurToolLayer->height == 0) return;

  if (blurToolLayer != NULL && isPaintAreaActive() && (!ImGui::IsMouseDown(0))) {
    double cx,cy; currentCanvas->getCanvasPos(mouseX,mouseY,cx,cy);
    blurToolLayer->updatePixelStates(~PIXELSTATE_OUTLINE,0,0); reverseLastUpdatedPixelStates(blurToolLayer);
    drawCircle(blurToolLayer,cx,cy,false,true);
    double toolWidth =  blurCircleSize * toolAspectX;
    double toolHeight =  blurCircleSize * toolAspectY;
    updateUpdateFlagByBrushLine(blurToolLayer,cx,cy,cx,cy,toolWidth,toolHeight);
    repaintLayer(blurToolLayer,false);
    blurToolLayer->hasOutline = true;
    clearOutlineAfterNextPaint = true;
  }

  if (blurToolLayer != NULL && isPaintAreaActive() && ImGui::IsMouseDown(0)) {
    double cx,cy; currentCanvas->getCanvasPos(mouseX,mouseY,cx,cy);
    drawCircle(blurToolLayer,cx,cy,blurToolLayer->hasSelection(),false);
    repaintLayer(blurToolLayer,false);
    updatedPixels(blurToolLayer);
  }
}

void useTool() {
  double mouseXBefore = mouseX;
  double mouseYBefore = mouseY;
  if (lockedMouseX) mouseX = lockedMousePosX;
  if (lockedMouseY) mouseY = lockedMousePosY;
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
  }
  mouseX = mouseXBefore;
  mouseY = mouseYBefore;
}
