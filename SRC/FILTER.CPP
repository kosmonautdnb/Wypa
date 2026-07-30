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
#include "FILTER.HPP"
#include "STL/GL.H"
#include "IMGUI/IMGUIGL.HPP"
#include "CONFIG.HPP"
#include "LAYERS.HPP"
#include "SERIALZE.HPP"
#include "UNDO.HPP"
#include <math.h>
#define __SUPERXBR_IMPLEMENTATION__
#include "STL/SUPERXBR.HPP"
#include "COLORFLD.HPP"

float sfrand2d(int seed,float x, float y) {
  x *= 3.3333;
  y *= 7.7777;
  void *k1 = (void*)(&x);
  void *k2 = (void*)(&y);
  srand((*((unsigned int*)k1))^seed); int xs = rand();
  srand((*((unsigned int*)k2))^seed); int ys = rand();
  srand(xs+ys);
  return (float)rand()*2 / (RAND_MAX-1) - 1;
}

void initFilter() {
}

void freeFilter() {
}

void filterDifferenceClouds() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;
  beforeUndoPoint();
  Array<float> values;
  values.resize(l->width*l->height);
  for (int i = 0; i < (int)values.size(); i++) values[i] = 0;
  int tileSize = l->width;
  if (l->height > tileSize) tileSize = l->height;
  static int seed = 0; seed += 1337;
  float mul = 1;
  initStatusBar();
  while(tileSize > 0) {
    for (int y = 0; y < l->height; y++) {
      for (int x = 0; x < l->width; x++) {
        int ix0 = x / tileSize;
        int iy0 = y / tileSize;
        float v00 = sfrand2d(seed,ix0,iy0);
        float v10 = sfrand2d(seed,ix0+1,iy0);
        float v01 = sfrand2d(seed,ix0,iy0+1);
        float v11 = sfrand2d(seed,ix0+1,iy0+1);
        float fx = (float)(x % tileSize)/tileSize;
        float fy = (float)(y % tileSize)/tileSize;
        float top = (v10 - v00)*fx + v00;
        float bottom = (v11 - v01)*fx + v01;
        float v = (bottom-top)*fy + top;
        values[x+y*l->width] += v*mul;
      }
    }
    updateStatusBar();
    seed += 571;
    seed *= 3;
    seed &= 0xffff;
    tileSize >>= 1;
    mul *= 0.5;
  }
  float minV = 0;
  float maxV = 0;
  for (int i2 = 0; i2 < l->width*l->height; i2++) {
    float v = values[i2];
    if (i2==0 || v < minV) minV = v;
    if (i2==0 || v > maxV) maxV = v;
  }
  float scale = maxV-minV;
  if (scale != 0) {
    for (int i2 = 0; i2 < l->width*l->height; i2++) {
      float v = values[i2];
      v = (v-minV)/scale;
      values[i2] = v;
    }
  }
  bool hasSelection = l->hasSelection();
  for (int i3 = 0; i3 < l->width*l->height; i3++) {
    if ((!hasSelection)||(l->pixelStates[i3] & PIXELSTATE_SELECTED)) {
      unsigned int col = l->pixels[i3];
      int r = col & 255;
      int g = (col>>8) & 255;
      int b = (col>>16) & 255;
      int a = (col>>24) & 255;
      int d = values[i3] * 255;
      r = d-r;
      g = d-g;
      b = d-b;
      a = 255;
      if (r < 0) r = -r;
      if (g < 0) g = -g;
      if (b < 0) b = -b;
      if (r > 255) r = 255-(r-255);
      if (g > 255) g = 255-(g-255);
      if (b > 255) b = 255-(b-255);
      if (r < 0) r = 0;
      if (g < 0) g = 0;
      if (b < 0) b = 0;
      if (r > 255) r = 255;
      if (g > 255) g = 255;
      if (b > 255) b = 255;
      l->pixels[i3] = r|(g<<8)|(b<<16)|(a<<24);
    }
  }
  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
}

void filterNoise() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;
  beforeUndoPoint();
  bool hasSelection = l->hasSelection();
  srand(0);
  for (int i3 = 0; i3 < l->width*l->height; i3++) {
    if ((!hasSelection)||(l->pixelStates[i3] & PIXELSTATE_SELECTED)) {
      int grey = rand() & 255;
      l->pixels[i3] = grey|(grey<<8)|(grey<<16)|(255<<24);
    }
  }
  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
}

void filterSobelEdgeDetect() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;
  beforeUndoPoint();
  Array<float> values;
  values.resize(l->width*l->height*3);
  int sobelXMat[3*3] = {+1,+0,-1, +2,+0,-2, +1,+0,-1};
  int sobelYMat[3*3] = {+1,+2,+1, 0,0,0, -1,-2,-1};
  for (int y = 0; y < l->height; y++) {
    for (int x = 0; x < l->width; x++) {
      float sobelXr = 0;
      float sobelYr = 0;
      float sobelXg = 0;
      float sobelYg = 0;
      float sobelXb = 0;
      float sobelYb = 0;
      int w = 0;
      for (int y2 = -1; y2 <= 1; y2++) {
        for (int x2 = -1; x2 <= 1; x2++) {
          int x3 = x + x2;
          int y3 = y + y2;
          if ((unsigned int)x3<(unsigned int)l->width && (unsigned int)y3<(unsigned int)l->height) {
            unsigned int col = l->pixels[x3+y3*l->width];
            int r = col & 255;
            int g = (col>>8) & 255;
            int b = (col>>16) & 255;
            sobelXr += r*sobelXMat[(x2+1)+(y2+1)*3];
            sobelXg += g*sobelXMat[(x2+1)+(y2+1)*3];
            sobelXb += b*sobelXMat[(x2+1)+(y2+1)*3];
            sobelYr += r*sobelYMat[(x2+1)+(y2+1)*3];
            sobelYg += g*sobelYMat[(x2+1)+(y2+1)*3];
            sobelYb += b*sobelYMat[(x2+1)+(y2+1)*3];
            //w++;
          }
        }
      }
      if (y == 0 || y == l->height-1) {
        sobelYr = sobelYg = sobelYb = 0;
      }
      if (x == 0 || x == l->width-1) {
        sobelXr = sobelXg = sobelXb = 0;
      }
      if (w != 0) {
        sobelXr /= w;
        sobelXg /= w;
        sobelXb /= w;
        sobelYr /= w;
        sobelYg /= w;
        sobelYb /= w;
      }
      values[(x+y*l->width)*3+0] = maxf(fabs(sobelXr),fabs(sobelYr));
      values[(x+y*l->width)*3+1] = maxf(fabs(sobelXg),fabs(sobelYg));
      values[(x+y*l->width)*3+2] = maxf(fabs(sobelXb),fabs(sobelYb));
    }
  }
  float minV = 0;
  float maxV = 0;
  for (int i2 = 0; i2 < l->width*l->height*3; i2++) {
    float v = values[i2];
    if (i2==0 || v < minV) minV = v;
    if (i2==0 || v > maxV) maxV = v;
  }
  float scale = maxV-minV;
  if (scale != 0) {
    for (int i2 = 0; i2 < l->width*l->height*3; i2++) {
      float v = values[i2];
      v = (v-minV)/scale;
      values[i2] = v;
    }
  }

  bool hasSelection = l->hasSelection();
  for (int i3 = 0; i3 < l->width*l->height; i3++) {
    if ((!hasSelection)||(l->pixelStates[i3] & PIXELSTATE_SELECTED)) {
      int r = values[i3*3+0]*255;
      int g = values[i3*3+1]*255;
      int b = values[i3*3+2]*255;
      int a = 255;
      if (r < 0) r = 0;
      if (g < 0) g = 0;
      if (b < 0) b = 0;
      if (r > 255) r = 255;
      if (g > 255) g = 255;
      if (b > 255) b = 255;
      l->pixels[i3] = r|(g<<8)|(b<<16)|(a<<24);
    }
  }


  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
}

void filterMorphologicalEdgeDetect() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;
  beforeUndoPoint();
  bool hasSelection = l->hasSelection();
  float maxv2 = 0;
  float minv2 = 0;
  Array<unsigned int> source;
  source.resize(l->width*l->height);
  memcpy(&source[0],l->pixels,l->width*l->height*sizeof(unsigned int));
  int posX[5] ={0,-1,1,0,0};
  int posY[5] ={0,0,0,-1,1};
  for (int y = 0; y < l->height; y++) {
    for (int x = 0; x < l->width; x++) {
      int i3 = x + y * l->width;
      if ((!hasSelection)||(l->pixelStates[i3] & PIXELSTATE_SELECTED)) {
        float maxv = 0;
        float minv = 0;
        bool first = true;
        //for (int y2 = -1; y2 <= 1; y2++) {
        //  for (int x2 = -1; x2 <= 1; x2++) {
        for (int j = 0; j < 5; j++) {{int x2 = posX[j]; int y2 = posY[j];
            int x3 = x + x2;
            int y3 = y + y2;
            if ((unsigned int)x3<(unsigned int)l->width && (unsigned int)y3<(unsigned int)l->height) {
              unsigned int col = source[x3+y3*l->width];
              int r = col & 255;
              int g = (col>>8) & 255;
              int b = (col>>16) & 255;
              int l = r + g + b;
              if (first || l > maxv) maxv = l;
              if (first || l < minv) minv = l;
              first = false;
            }
          }
        }
        int k = maxv-minv;
        if (first || k > maxv2) maxv2 = k;
        if (first || k < minv2) minv2 = k;
        l->pixels[i3] = k;
      }
    }
  }
  float scale = maxv2-minv2;
  if (scale != 0) {
    for (int i3 = 0; i3 < l->width*l->height; i3++) {
      if ((!hasSelection)||(l->pixelStates[i3] & PIXELSTATE_SELECTED)) {
        unsigned int v = l->pixels[i3];
        v = (v-minv2) * 255/scale;
        l->pixels[i3] = v|(v<<8)|(v<<16)|(0xff000000);
      }
    }
  }

  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
}

void filterAlphaSoften() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;
  beforeUndoPoint();
  bool hasSelection = l->hasSelection();
  Array<unsigned int> source;
  source.resize(l->width*l->height);
  memcpy(&source[0],l->pixels,l->width*l->height*sizeof(unsigned int));
  int posX[5] ={0,-1,1,0,0};
  int posY[5] ={0,0,0,-1,1};
  for (int y = 0; y < l->height; y++) {
    for (int x = 0; x < l->width; x++) {
      if ((!hasSelection)||(l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED)) {
        int a = 0;
        int w = 0;
        for (int j = 0; j < 5; ++j) {
          int x3 = x + posX[j];
          int y3 = y + posY[j];
          if ((unsigned int)x3<(unsigned int)l->width && (unsigned int)y3<(unsigned int)l->height) {
            unsigned int col = source[x3+y3*l->width];
            a += (col>>24) & 255;
            w++;
          }
        }
        if (w != 0) {
          a /= w;
        }
        unsigned int col = source[x+y*l->width];
        int r = col & 255;
        int g = (col>>8) & 255;
        int b = (col>>16) & 255;
        switch(l->mode) {
          case LAYERMODE_PREMULTIPLIEDALPHA: {
              r = (r * a) / 255;
              g = (g * a) / 255;
              b = (b * a) / 255;
            } break;
        }
        l->pixels[x+y*l->width] = r|(g<<8)|(b<<16)|(a<<24);
      }
    }
  }
  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
}

void filterBoxBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int boxW, int boxH, unsigned char *pixelStates, bool usePixelStates) {
  if (boxW < 1 || boxH < 1 || boxW >= width*1.5 || boxH >= height*1.5) {memcpy(dest,source,width*height*sizeof(unsigned int)); return;}
  initStatusBar();
  Array<unsigned int> blurred;
  blurred.resize(width*height*5);
  for (int y = 0; y < (int)height; y++) {
    if ((y & 15)==0) updateStatusBar();
    for (int x = 0; x < (int)width; x++) {
      int w = 0;
      int r = 0;
      int g = 0;
      int b = 0;
      int a = 0;
      for (int x2 = -boxW; x2 <= boxW; x2++) {
        int x3 = x2 + x;
        int y3 = y;
        if ((unsigned int)x3<(unsigned int)width&&(unsigned int)y3<(unsigned int)height) {
          unsigned int col = source[x3+y3*width];
          r += col & 255;
          g += (col>>8) & 255;
          b += (col>>16) & 255;
          a += (col>>24) & 255;
          w++;
        }
      }
      blurred[(x+y*width)*5+0] = r;
      blurred[(x+y*width)*5+1] = g;
      blurred[(x+y*width)*5+2] = b;
      blurred[(x+y*width)*5+3] = a;
      blurred[(x+y*width)*5+4] = w;
    }
  }
  for (int y4 = 0; y4 < (int)height; y4++) {
    if ((y4 & 15)==0) updateStatusBar();
    for (int x4 = 0; x4 < (int)width; x4++) {
      if ((!usePixelStates)||(pixelStates[x4+y4*width] & PIXELSTATE_SELECTED)) {
        int w = 0;
        int r = 0;
        int g = 0;
        int b = 0;
        int a = 0;
        for (int y2 = -boxH; y2 <= boxH; y2++) {
          int x3 = x4;
          int y3 = y2 + y4;
          if ((unsigned int)x3<(unsigned int)width&&(unsigned int)y3<(unsigned int)height) {
            r += blurred[(x3+y3*width)*5+0];
            g += blurred[(x3+y3*width)*5+1];
            b += blurred[(x3+y3*width)*5+2];
            a += blurred[(x3+y3*width)*5+3];
            w += blurred[(x3+y3*width)*5+4];
          }
        }
        if (w != 0) {
          r /= w;
          g /= w;
          b /= w;
          a /= w;
        }
        if (r < 0) r = 0;
        if (g < 0) g = 0;
        if (b < 0) b = 0;
        if (a < 0) a = 0;
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        if (a > 255) a = 255;
        dest[x4+y4*width] = r|(g<<8)|(b<<16)|(a<<24);
      }
    }
  }
}

void filterRadialBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float radialAngle, float radialSize, unsigned char *pixelStates, bool usePixelStates) {
  if (radialSize < 1 || radialSize > width+height) {memcpy(dest,source,width*height*sizeof(unsigned int)); return;}
  initStatusBar();
  float r = -radialAngle*PI*2/360.0;
  float stepX = sin(r);
  float stepY = cos(r);
  for (int y4 = 0; y4 < (int)height; y4++) {
    if ((y4 & 15)==0) updateStatusBar();
    for (int x4 = 0; x4 < (int)width; x4++) {
      if ((!usePixelStates)||(pixelStates[x4+y4*width] & PIXELSTATE_SELECTED)) {
        int r = 0;
        int g = 0;
        int b = 0;
        int a = 0;
        int w = 0;
        for (int k = -radialSize; k <= radialSize; k++) {
          int x3 = stepX * k + x4;
          int y3 = stepY * k + y4;
          if ((unsigned int)x3 < (unsigned int)width && (unsigned int)y3 < (unsigned int)height) {
            unsigned int col = source[x3+y3*width];
            r += col & 255;
            g += (col>>8) & 255;
            b += (col>>16) & 255;
            a += (col>>24) & 255;
            w++;
          }
        }
        if (w != 0) {
          r /= w;
          g /= w;
          b /= w;
          a /= w;
        }
        if (r < 0) r = 0;
        if (g < 0) g = 0;
        if (b < 0) b = 0;
        if (a < 0) a = 0;
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        if (a > 255) a = 255;
        dest[x4+y4*width] = r|(g<<8)|(b<<16)|(a<<24);
      }
    }
  }
}

float gaussianTap5Weight0 = 0.054f;
float gaussianTap5Weight1 = 0.244f;
float gaussianTap5Weight2 = 0.403f;
#define BLOOMKERNEL\
    rgb=((((rgba0&0x00ff00ff)+(rgba4&0x00ff00ff))*weight0+((rgba1&0x00ff00ff)+(rgba3&0x00ff00ff))*weight1+(rgba2&0x00ff00ff)*weight2)>>8) & 0x00ff00ff;\
    rgb|=((((((rgba0>>8)&0x00ff00ff)+((rgba4>>8)&0x00ff00ff))*weight0+(((rgba1>>8)&0x00ff00ff)+((rgba3>>8)&0x00ff00ff))*weight1+((rgba2>>8)&0x00ff00ff)*weight2)>>8)&0x00ff00ff)<<8;
#define __SX0(__x__) s[(int)(__x__)]
#define __SY0(__y__) s[(int)(__y__)*width]

#define BLOOMKERNEL2\
  float rf = ((rgba0 & 255) + (rgba4 & 255)) * weight0 + ((rgba1 & 255)+(rgba3 & 255)) * weight1 + (rgba2 & 255)*weight2;\
  rgba0>>=8;rgba1>>=8;rgba2>>=8;rgba3>>=8;rgba4>>=8;\
  float gf = ((rgba0 & 255) + (rgba4 & 255)) * weight0 + ((rgba1 & 255)+(rgba3 & 255)) * weight1 + (rgba2 & 255)*weight2;\
  rgba0>>=8;rgba1>>=8;rgba2>>=8;rgba3>>=8;rgba4>>=8;\
  float bf = ((rgba0 & 255) + (rgba4 & 255)) * weight0 + ((rgba1 & 255)+(rgba3 & 255)) * weight1 + (rgba2 & 255)*weight2;\
  rgba0>>=8;rgba1>>=8;rgba2>>=8;rgba3>>=8;rgba4>>=8;\
  float af = ((rgba0 & 255) + (rgba4 & 255)) * weight0 + ((rgba1 & 255)+(rgba3 & 255)) * weight1 + (rgba2 & 255)*weight2;\
  int ri = (int)rf;\
  int gi = (int)gf;\
  int bi = (int)bf;\
  int ai = (int)af;\
  rgb = ri|(gi<<8)|(bi<<16)|(ai<<24);

inline unsigned int bloomLookUp(float x, float y, unsigned int *s, int w, int h) {
  int xi = (int)x;
  int yi = (int)y;
  int xf = (int)((x - xi)*256.0);
  int yf = (int)((y - yi)*256.0);
  unsigned int r00 = s[xi+yi*w];
  unsigned int r10 = xi>=w-1?r00:s[xi+1+yi*w];
  unsigned int r01 = yi>=h-1?r00:s[xi+(yi+1)*w];
  unsigned int r11 = xi>=w-1||yi>=h-1?r00:s[xi+1+(yi+1)*w];

  unsigned int toprb      = (((((int)(r10 & 0x00ff00ff)-(int)(r00 & 0x00ff00ff))*xf)>>8)+r00) & 0x00ff00ff;
  unsigned int bottomrb = (((((int)(r11 & 0x00ff00ff)-(int)(r01 & 0x00ff00ff))*xf)>>8)+r01) & 0x00ff00ff;
  r00 >>= 8; r10 >>= 8; r01 >>= 8; r11 >>= 8;
  unsigned int topga      = (((((int)(r10 & 0x00ff00ff)-(int)(r00 & 0x00ff00ff))*xf)>>8)+r00) & 0x00ff00ff;
  unsigned int bottomga = (((((int)(r11 & 0x00ff00ff)-(int)(r01 & 0x00ff00ff))*xf)>>8)+r01) & 0x00ff00ff;

  unsigned int rb = ((((bottomrb - toprb)*yf)>>8)+toprb) & 0x00ff00ff;
  unsigned int ga = ((((bottomga - topga)*yf)>>8)+topga) & 0x00ff00ff;

  return rb | (ga<<8);
}

#define __SX(__x__) bloomLookUp(__x__,y,s,width,height)
#define __SY(__y__) bloomLookUp(x,__y__,s,width,height)

void bloomX(unsigned int *buffer, unsigned int width, unsigned int height, float tap0, float tap1) {
  float weight0 =  gaussianTap5Weight0;
  float weight1 =  gaussianTap5Weight1;
  float weight2 =  gaussianTap5Weight2; // center
  float weights = (weight0+weight1+weight2+weight1+weight0);
  weight0 = weight0/weights;
  weight1 = weight1/weights;
  weight2 = weight2/weights;
  unsigned int *newBuffer = new unsigned int[width*height];
  for (int y = 0; y < (int)height; y++) {
    unsigned int *s = &buffer[y*width];
    unsigned int *d = &newBuffer[y*width];
    int rgb;
    for (int x = 0; x < (int)width; x++) {
      float t0x = x - tap0; if (t0x < 0) t0x = 0;
      float t1x = x - tap1; if (t1x < 0) t1x = 0;
      float t3x = x + tap1; if (t3x > width-1) t3x = width-1;
      float t4x = x + tap0; if (t4x > width-1) t4x = width-1;
      unsigned int rgba0 = __SX0(t0x);
      unsigned int rgba1 = __SX0(t1x);
      unsigned int rgba2 = __SX0(x);
      unsigned int rgba3 = __SX0(t3x);
      unsigned int rgba4 = __SX0(t4x);
      BLOOMKERNEL2
      *d++=rgb;
    }
  }
  memcpy(buffer,newBuffer,width*height*sizeof(unsigned int));
  delete[] newBuffer;
}

void bloomXFiltered(unsigned int *buffer, unsigned int width, unsigned int height, float tap0, float tap1) {
  float weight0 =  gaussianTap5Weight0;
  float weight1 =  gaussianTap5Weight1;
  float weight2 =  gaussianTap5Weight2; // center
  float weights = (weight0+weight1+weight2+weight1+weight0);
  weight0 = weight0/weights;
  weight1 = weight1/weights;
  weight2 = weight2/weights;
  unsigned int *newBuffer = new unsigned int[width*height];
  for (int y = 0; y < (int)height; y++) {
    unsigned int *s = &buffer[0];
    unsigned int *d = &newBuffer[y*width];
    int rgb;
    for (int x = 0; x < (int)width; x++) {
      float t0x = x - tap0; if (t0x < 0) t0x = 0;
      float t1x = x - tap1; if (t1x < 0) t1x = 0;
      float t3x = x + tap1; if (t3x > width-1) t3x = width-1;
      float t4x = x + tap0; if (t4x > width-1) t4x = width-1;
      unsigned int rgba0 = __SX(t0x);
      unsigned int rgba1 = __SX(t1x);
      unsigned int rgba2 = __SX(x);
      unsigned int rgba3 = __SX(t3x);
      unsigned int rgba4 = __SX(t4x);
      BLOOMKERNEL2
      *d++=rgb;
    }
  }
  memcpy(buffer,newBuffer,width*height*sizeof(unsigned int));
  delete[] newBuffer;
}

void bloomY(unsigned int *buffer, unsigned int width, unsigned int height, float tap0, float tap1) {
  float weight0 =  gaussianTap5Weight0;
  float weight1 =  gaussianTap5Weight1;
  float weight2 =  gaussianTap5Weight2; // center
  float weights = (weight0+weight1+weight2+weight1+weight0);
  weight0 = weight0/weights;
  weight1 = weight1/weights;
  weight2 = weight2/weights;
  unsigned int *newBuffer = new unsigned int[width*height];
  for (int x = 0; x < (int)width; x++) {
    unsigned int *s = &buffer[x];
    unsigned int *d = &newBuffer[x];
    int rgb;
    for (int y = 0; y < (int)height; y++) {
      float t0y = y - tap0; if (t0y < 0) t0y = 0;
      float t1y = y - tap1; if (t1y < 0) t1y = 0;
      float t3y = y + tap1; if (t3y > height-1) t3y = height-1;
      float t4y = y + tap0; if (t4y > height-1) t4y = height-1;
      unsigned int rgba0 = __SY0(t0y);
      unsigned int rgba1 = __SY0(t1y);
      unsigned int rgba2 = __SY0(y);
      unsigned int rgba3 = __SY0(t3y);
      unsigned int rgba4 = __SY0(t4y);
      BLOOMKERNEL2
      *d=rgb;
      d+=width;
    }
  }
  memcpy(buffer,newBuffer,width*height*sizeof(unsigned int));
  delete[] newBuffer;
}

void bloomYFiltered(unsigned int *buffer, unsigned int width, unsigned int height, float tap0, float tap1) {
  float weight0 =  gaussianTap5Weight0;
  float weight1 =  gaussianTap5Weight1;
  float weight2 =  gaussianTap5Weight2; // center
  float weights = (weight0+weight1+weight2+weight1+weight0);
  weight0 = weight0/weights;
  weight1 = weight1/weights;
  weight2 = weight2/weights;
  unsigned int *newBuffer = new unsigned int[width*height];
  for (int x = 0; x < (int)width; x++) {
    unsigned int *s = &buffer[0];
    unsigned int *d = &newBuffer[x];
    int rgb;
    for (int y = 0; y < (int)height; y++) {
      float t0y = y - tap0; if (t0y < 0) t0y = 0;
      float t1y = y - tap1; if (t1y < 0) t1y = 0;
      float t3y = y + tap1; if (t3y > height-1) t3y = height-1;
      float t4y = y + tap0; if (t4y > height-1) t4y = height-1;
      unsigned int rgba0 = __SY(t0y);
      unsigned int rgba1 = __SY(t1y);
      unsigned int rgba2 = __SY(y);
      unsigned int rgba3 = __SY(t3y);
      unsigned int rgba4 = __SY(t4y);
      BLOOMKERNEL2
      *d=rgb;
      d+=width;
    }
  }
  memcpy(buffer,newBuffer,width*height*sizeof(unsigned int));
  delete[] newBuffer;
}


void filterGaussBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float radiusX, float radiusY, unsigned char *pixelStates, bool usePixelStates) {

  if (radiusX < 1 || radiusY < 1) {memcpy(dest,source,width*height*sizeof(unsigned int)); return;}

  double passesX = log(radiusX)/log(2.0);
  double passesY = log(radiusY)/log(2.0);

  int px = (int)floor(passesX);
  int py = (int)floor(passesY);

  if (px < 1 || py < 1) {memcpy(dest,source,width*height*sizeof(unsigned int)); return;}

  initStatusBar();

  unsigned int *bloomBuffer = new unsigned int[width*height]; for (int i0 = 0; i0 < (int)(width*height); i0++) bloomBuffer[i0] = source[i0];

  if (passesX > px) {
    updateStatusBar();
    float kernelSize = passesX;
    bloomXFiltered(bloomBuffer,width,height,kernelSize*2,kernelSize*1);
  }

  for (int i1 = px-1; i1 >= 0; i1--) {
    updateStatusBar();
    float kernelSize = pow(2,i1);
    bloomX(bloomBuffer,width,height,kernelSize*2,kernelSize*1);
  }

  if (passesY > py) {
    updateStatusBar();
    float kernelSize = passesY;
    bloomYFiltered(bloomBuffer,width,height,kernelSize*2,kernelSize*1);
  }

  for (int i2 = py-1; i2 >= 0; i2--) {
    updateStatusBar();
    float kernelSize = pow(2,i2);
    bloomY(bloomBuffer,width,height,kernelSize*2,kernelSize*1);
  }

  for (int i3 = 0; i3 < (int)(width*height); i3++) {
    if ((!usePixelStates)||(pixelStates[i3] & PIXELSTATE_SELECTED)) {
      dest[i3] = bloomBuffer[i3];
    }
  }

  delete[] bloomBuffer;
}

void filterIterativeBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int steps, unsigned char *pixelStates, bool usePixelStates) {
  if (steps < 1) {memcpy(dest,source,width*height*sizeof(unsigned int)); return;}
  initStatusBar();
  Array<unsigned int> f;
  f.resize(width*height);
  memcpy(&f[0],source,width*height*sizeof(unsigned int));
  int posX[5] ={0,-1,1,0,0};
  int posY[5] ={0,0,0,-1,1};
  for (int i = 0; i < steps; i++) {
    for (int y4 = 0; y4 < (int)height; y4++) {
      if ((y4 & 15)==0) updateStatusBar();
      for (int x4 = 0; x4 < (int)width; x4++) {
        if ((!usePixelStates)||(pixelStates[x4+y4*width] & PIXELSTATE_SELECTED)) {
          int r = 0;
          int g = 0;
          int b = 0;
          int a = 0;
          int w = 0;
          for(int j = 0; j < 5; j++) {
            int x3 = x4 + posX[j];
            int y3 = y4 + posY[j];
            if ((unsigned int)x3<(unsigned int)width && (unsigned int)y3<(unsigned int)height) {
              unsigned int col = f[x3+y3*width];
              r += col & 255;
              g += (col>>8) & 255;
              b += (col>>16) & 255;
              a += (col>>24) & 255;
              w++;
            }
          }
          if (w != 0) {
            r /= w;
            g /= w;
            b /= w;
            a /= w;
          }
          if (r < 0) r = 0;
          if (g < 0) g = 0;
          if (b < 0) b = 0;
          if (a < 0) a = 0;
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          if (a > 255) a = 255;
          dest[x4+y4*width] = r|(g<<8)|(b<<16)|(a<<24);
        }
      }
    }
    memcpy(&f[0],dest,width*height*sizeof(unsigned int));
  }
}

void filterEmboss(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float angle, unsigned char *pixelStates, bool usePixelStates) {
  int sobelXMat[3*3] = {+1,+0,-1, +2,+0,-2, +1,+0,-1};
  int sobelYMat[3*3] = {+1,+2,+1, 0,0,0, -1,-2,-1};

  Array<float> temp;
  temp.resize(width*height*2);
  for (int i = 0; i < (int)(width*height); i++) {
    unsigned int col = source[i];
    int r = col & 255;
    int g = (col>>8) & 255;
    int b = (col>>16) & 255;
    temp[i] = r+g+b;
  }
  for (int j = 0; j < 2; j++) {
    for (int y = 0; y < (int)height; y++) {
      for (int x = 0; x < (int)width; x++) {
        float val = 0;
        float w = 0;
        val += temp[x+y*width]; w += 1.0;
        if (y > 0) {val += temp[x+(y-1)*width]; w += 1.0;}
        if (y < (int)height-1) {val += temp[x+(y+1)*width]; w += 1.0;}
        if (x > 0) {val += temp[x-1+y*width]; w += 1.0;}
        if (x < (int)width-1) {val += temp[x+1+y*width]; w += 1.0;}
        if (w != 0) val /= w;
        temp[x+y*width+width*height] = val;
      }
    }
    memcpy(&temp[0],&temp[width*height],width*height*sizeof(float));
  }

  for (int y = 0; y < (int)height; y++) {
    for (int x = 0; x < (int)width; x++) {
      if ((!usePixelStates)||(pixelStates[x+y*width] & PIXELSTATE_SELECTED)) {
        float sobelX = 0;
        float sobelY = 0;
        for (int y2 = -1; y2 <= 1; y2++) {
          for (int x2 = -1; x2 <= 1; x2++) {
            int x3 = x + x2;
            int y3 = y + y2;
            if ((unsigned int)x3<(unsigned int)width && (unsigned int)y3<(unsigned int)height) {
              sobelX += temp[x3+y3*width]*sobelXMat[(x2+1)+(y2+1)*3];
              sobelY += temp[x3+y3*width]*sobelYMat[(x2+1)+(y2+1)*3];
            }
          }
        }
        if (y == 0 || y == (int)height-1) {
          sobelY = 0;
        }
        if (x == 0 || x == (int)width-1) {
          sobelX = 0;
        }
        float val = 0;
        if (sobelX != 0 || sobelY != 0) {
          float angleHere = atan2(sobelX,sobelY);
          val = sin(angleHere+(angle-90)*PI*2/360.0);
          int grey = val * 128 + 128;
          if (grey < 0) grey = 0;
          if (grey > 255) grey = 255;
          int r = grey;
          int g = grey;
          int b = grey;
          int a = 255;
          dest[x+y*width] = r|(g<<8)|(b<<16)|(a<<24);
        } else {
          dest[x+y*width] = 0xff000000;
        }
      }
    }
  }
}

void filterSimpleKeying(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int channel, float factor, unsigned char *pixelStates, bool usePixelStates, int mode, bool binary, bool invert) {
  int rm = channel == 0 ? 1 : 0;
  int gm = channel == 1 ? 1 : 0;
  int bm = channel == 2 ? 1 : 0;
  for (int y = 0; y < (int)height; y++) {
    for (int x = 0; x < (int)width; x++) {
      if ((!usePixelStates)||(pixelStates[x+y*width] & PIXELSTATE_SELECTED)) {
        unsigned int col = source[x+y*width];
        int r = col & 255;
        int g = (col>>8) & 255;
        int b = (col>>16) & 255;
        int a = (col>>24) & 255;
        int a2 = a;
        int fc = rm * r + gm * g + bm * b; // g
        int o = (1-rm) * r; int v = o; o = (1-gm) * g; if (o > v) v = o; o = (1-bm) * b; if (o > v) v = o; // max rb
        a = (fc - v)*factor;
        if (a < 0) a = 0;
        if (a > 255) a = 255;
        if (!invert) a = 255 - a;
        if (binary) a = a > 127 ? 255 : 0;
        if (a > 255) a = 255;
        a = a * a2 / 255;
        switch(mode) {
        case LAYERMODE_PREMULTIPLIEDALPHA: {
          r = (r*a)/255;
          g = (g*a)/255;
          b = (b*a)/255;
          } break;
        }
        dest[x+y*width] = r | (g<<8) | (b<<16) | (a<<24);
      }
    }
  }
}

void filterColorKeying(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, unsigned int color, float factor, unsigned char *pixelStates, bool usePixelStates, int mode, bool binary, bool invert) {
  int cr = color & 255;
  int cg = (color>>8) & 255;
  int cb = (color>>16) & 255;
  for (int y = 0; y < (int)height; y++) {
    for (int x = 0; x < (int)width; x++) {
      if ((!usePixelStates)||(pixelStates[x+y*width] & PIXELSTATE_SELECTED)) {
        unsigned int col = source[x+y*width];
        int r = col & 255;
        int g = (col>>8) & 255;
        int b = (col>>16) & 255;
        int a = (col>>24) & 255;
        int a2 = a;
        int rd = abs(r - cr);
        int gd = abs(g - cg);
        int bd = abs(b - cb);
        int q = rd;
        if (gd > q) q = gd;
        if (bd > q) q = bd;
        a = q * factor;
        if (a < 0) a = 0;
        if (a > 255) a = 255;
        if (invert) a = 255 - a;
        if (binary) a = a > 127 ? 255 : 0;
        if (a > 255) a = 255;
        a = a * a2 / 255;
        switch(mode) {
        case LAYERMODE_PREMULTIPLIEDALPHA: {
          r = (r*a)/255;
          g = (g*a)/255;
          b = (b*a)/255;
          } break;
        }
        dest[x+y*width] = r | (g<<8) | (b<<16) | (a<<24);
      }
    }
  }
}

void filterDenoise() {
  int posX[5] = {0,1,-1,0,0};
  int posY[5] = {0,0,0,1,-1};
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;
  Array<unsigned int> copy;
  copy.resize(l->width*l->height);
  memcpy(&copy[0],l->pixels,l->width*l->height*sizeof(unsigned int));
  beforeUndoPoint();
  bool hasSelection = l->hasSelection();
  Array<unsigned int> _a;
  Array<unsigned int> _b;
  _a.resize(3*3+1);
  _b.resize(3*3+1);
  for (int y = 0; y < (int)l->height; y++) {
    for (int x = 0; x < (int)l->width; x++) {
      if ((!hasSelection)||(l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED)) {
        int pos = 0;
        //for (int y2 = -1; y2 <= 1; y2++) {
        //  for (int x2 = -1; x2 <= 1; x2++) {
        for (int j = 0; j < 5; j++) {{int x2 = posX[j]; int y2 = posY[j];
            int x3 = x + x2;
            int y3 = y + y2;
            if ((unsigned int)x3 < (unsigned int)l->width && (unsigned int)y3 < (unsigned int)l->height) {
              unsigned int col = copy[x3+y3*l->width];
              int r = col & 255;
              int g = (col>>8) & 255;
              int b = (col>>16) & 255;
              int grey = r*25 + g*60 + b*15;
              int p = 0;
              for (; p < pos; p++) {
                if ((int)_a[p]>=grey) break;
              }
              for (int k = pos-1;k>=p;k--) {
                _a[k+1] = _a[k];
                _b[k+1] = _b[k];
              }
              pos++;
              _a[p] = grey;
              _b[p] = col;
            }
          }
        }
        l->pixels[x+y*l->width] = _b[pos/2];
      }
    }
  }
  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
}

void filterSharpen() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;
  beforeUndoPoint();
  bool hasSelection = l->hasSelection();
  Array<unsigned int> source;
  source.resize(l->width*l->height);
  memcpy(&source[0],l->pixels,l->width*l->height*sizeof(unsigned int));
  int posX[5] ={0,-1,1,0,0};
  int posY[5] ={0,0,0,-1,1};
  for (int y = 0; y < (int)l->height; y++) {
    for (int x = 0; x < (int)l->width; x++) {
      int i3 = x + y * (int)l->width;
      if ((!hasSelection)||(l->pixelStates[i3] & PIXELSTATE_SELECTED)) {
        //bool first = true;
        int r = 0;
        int g = 0;
        int b = 0;
        int w = 0;
        for (int j = 0; j < 5; j++) {int x2 = posX[j]; int y2 = posY[j];
          int x3 = x + x2;
          int y3 = y + y2;
          if ((unsigned int)x3<(unsigned int)l->width && (unsigned int)y3<(unsigned int)l->height) {
            unsigned int col = source[x3+y3*l->width];
            int ra = col & 255;
            int ga = (col>>8) & 255;
            int ba = (col>>16) & 255;
            r += ra;
            g += ga;
            b += ba;
            w++;
          }
        }
        if (w != 0) {
          r /= w;
          g /= w;
          b /= w;
        }
        unsigned int col = source[x+y*l->width];
        int rb = col & 255;
        int gb = (col>>8) & 255;
        int bb = (col>>16) & 255;
        int rd = rb - r;
        int gd = gb - g;
        int bd = bb - b;
        rb += rd;
        gb += gd;
        bb += bd;
        if (rb < 0) rb = 0;
        if (gb < 0) gb = 0;
        if (bb < 0) bb = 0;
        if (rb > 255) rb = 255;
        if (gb > 255) gb = 255;
        if (bb > 255) bb = 255;
        l->pixels[i3] = rb|(gb<<8)|(bb<<16)|(col & 0xff000000);
      }
    }
  }
  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
}

void filterSuperXBR() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;
  beforeUndoPoint();
  int w = l->width, h = l->height;
  Array<unsigned int> source;
  source.resize(w*h);
  memcpy(&source[0],l->pixels,w*h*sizeof(unsigned int));
  l->setContentSize(w*2, h*2);
  superXBR(&source[0],l->pixels,w,h); // somehow this algorithm is a bit broken, sorry.. Not my fault currently..
  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
}

void filterEagle() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;
  beforeUndoPoint();
  int w = l->width, h = l->height;
  Array<unsigned int> source;
  source.resize(w*h);
  memcpy(&source[0],l->pixels,w*h*sizeof(unsigned int));
  l->setContentSize(w*2, h*2);
  //eagle(&source[0],l->pixels,w,h);
  eagle(&source[0],l->pixels,w,h);
  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
}

void filterEpx() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;
  beforeUndoPoint();
  int w = l->width, h = l->height;
  Array<unsigned int> source;
  source.resize(w*h);
  memcpy(&source[0],l->pixels,w*h*sizeof(unsigned int));
  l->setContentSize(w*2, h*2);
  epx(&source[0],l->pixels,w,h);
  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
}

double __round(double v) {
  return floor(v+0.5);
}

void filterHistogramMatching(Layer *histogramSource) {
  Layer *a = currentCanvas->getActiveLayer();
  if (a == NULL) return;
  if (locked(a,LOCK_ANY)) return;

  Layer *l = histogramSource;

  int iw = l->width;  int ih = l->height;
  int tw = a->width;  int th = a->height;

  // converted from github/KIMMINTAE98/Histogram-matching
  Array<int> inputHistogram; inputHistogram.resize(256); memset(&inputHistogram[0],0,256*sizeof(int));
  Array<int> targetHistogram; targetHistogram.resize(256); memset(&targetHistogram[0],0,256*sizeof(int));
  Array<int> matching; matching.resize(256);  memset(&matching[0],0,256*sizeof(int)); // matching function
  Array<double> inputCDF; inputCDF.resize(256); memset(&inputCDF[0],0,256*sizeof(double));
  Array<double> targetCDF; targetCDF.resize(256); memset(&targetCDF[0],0,256*sizeof(double));

  // currently only "luminace"

  for (int i0 = 0; i0 < (int)(l->width*l->height); i0++) {
    unsigned int col = l->pixels[i0];
    int r = col & 255;
    int g = (col>>8) & 255;
    int b = (col>>16) & 255;
    int grey = (r + g + b)/3;
    inputHistogram[grey]++;
  }

  for (int i1 = 0; i1 < (int)(a->width*a->height); i1++) {
    unsigned int col = a->pixels[i1];
    int r = col & 255;
    int g = (col>>8) & 255;
    int b = (col>>16) & 255;
    int grey = (r + g + b)/3;
    targetHistogram[grey]++;
  }

  inputCDF[0] = (double)inputHistogram[0]/(iw*ih);
  targetCDF[0] = (double)targetHistogram[0]/(tw*th);

  for (int i = 1; i < 256; i++) {
    inputCDF[i] = inputCDF[i-1]+(double)inputHistogram[i]/(iw*ih);
    targetCDF[i] = targetCDF[i-1]+(double)targetHistogram[i]/(tw*th);
  }

  for (int a_ = 0; a_ < 256; a_++) {
    double lastDist = 0; bool first = true; int found = 0;
    for (int b_ = 0; b_ < 256; b_++) {
      double dist = fabs(inputCDF[a_]-targetCDF[b_]);
      if (first || dist < lastDist) {found = b_; lastDist = dist; first = false;}
    }
    matching[a_] = found;
  }
  for (int c_ = 1; c_ < 256-1; c_++) { // not perfect, but whatever
    if (matching[c_]==0) {
      int d = c_, e = c_;
      for(;d>0;d--) if (matching[d]!=0) break;
      for(;e<256;e++) if (matching[e]!=0) break;
      if (e<256)
      for (int j = d+1; j < e; j++) matching[j] = (j-d)*(matching[e]-matching[d])/(e-d)+matching[d];
    }
  }

  beforeUndoPoint();
  for (int i2 = 0; i2 < (int)(a->width*a->height); i2++) {
    unsigned int col = a->pixels[i2];
    int r = col & 255;
    int g = (col>>8) & 255;
    int b = (col>>16) & 255;
    int grey = (r + g + b)/3;
    int grey2 = matching[grey];
    if (grey != 0) {
      r = r * grey2 / grey;
      g = g * grey2 / grey;
      b = b * grey2 / grey;
    }
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    a->pixels[i2] = r|(g<<8)|(b<<16)|(col & 0xff000000);
  }
  updatedPixels(a);
  afterUndoPoint();
  repaintLayer(a,true);
}

void filterClahe(float claheConstant, int cellSize) {
  // not a perfect Clahe implementation
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;

  int tileW = cellSize;
  int tileH = cellSize;
  int iw = tileW, tw = tileW;
  int ih = tileH, th = tileH;
  double claheClip = 4; // x times the same color in a 16x16 region is a "no go"
  double claheAverageFactor = claheConstant;

  Array<int> remap;
  Array<double> inputHistogram; inputHistogram.resize(256);
  Array<double> targetHistogram; targetHistogram.resize(256);
  Array<double> inputCDF; inputCDF.resize(256); memset(&inputCDF[0],0,256*sizeof(double));
  Array<double> targetCDF; targetCDF.resize(256); memset(&targetCDF[0],0,256*sizeof(double));

  beforeUndoPoint();

  initStatusBar();

  int hcount = 0;
  int vcount = 0;

  for (int y = 0; y < l->height; y += tileH) {
    vcount++;
    int tileH2 = tileH; if (y + tileH2 > l->height) tileH2 = l->height - y;
    updateStatusBar();
    hcount = 0;
    for (int x = 0; x < l->width; x += tileW) {
      hcount++;
      int tileW2 = tileW; if (x + tileW2 > l->width) tileW2 = l->width - x;

      int remapStart = remap.size(); remap.resize(remapStart + 256); int *matching = &remap[remapStart];

      memset(&inputHistogram[0],0,256*sizeof(double));
      memset(&targetHistogram[0],0,256*sizeof(double));

      // generate histogram
      for (int y2 = 0; y2 < tileH2; y2++) {
        int y3 = y2 + y;
        for (int x2 = 0; x2 < tileW2; x2++) {
          int x3 = x2 + x;
          unsigned int col = l->pixels[x3 + y3*l->width];
          int r = col & 255;
          int g = (col>>8) & 255;
          int b = (col>>16) & 255;
          int grey = (r+g+b)/3;
          inputHistogram[grey]+=1.0;
          targetHistogram[grey]+=1.0;
        }
      }

      int count = 0; double average = 0; for (int j = 0; j < 256; j++) {
        if (targetHistogram[j] > 0) {count++; average += targetHistogram[j];}
      }
      if (count > 0) average /= count;
      if (true) {
        claheClip = average * claheAverageFactor;
      }

      // diffuse histogram by clip value      
      while(count > 1) {
        double error = 0;
        for (int j = 0; j < 256; j++) {
          double a = targetHistogram[j];
          if (a > 0 && a > claheClip) {
            error += a - claheClip;
            targetHistogram[j] = claheClip;
          }
        }
        if (error == 0) break;
        error /= count;
        int count2 = count;
        for (int j2 = 0; j2 < 256; j2++) {
          double a = targetHistogram[j2];
          if (a > 0 && a + error <= claheClip) {
            targetHistogram[j2] += error;
            count2--;
          }
        }
        if (count2 > 0) break;
      }

      // cumulative distribution function
      inputCDF[0] = (double)inputHistogram[0]/(iw*ih);
      targetCDF[0] = (double)targetHistogram[0]/(tw*th);
    
      for (int k = 1; k < 256; k++) {
        inputCDF[k] = inputCDF[k-1]+(double)inputHistogram[k]/(iw*ih);
        targetCDF[k] = targetCDF[k-1]+(double)targetHistogram[k]/(tw*th);
      }

      for (int a_ = 0; a_ < 256; a_++) {
        double lastDist = 0; bool first = true; int found = 0;
        for (int b_ = 0; b_ < 256; b_++) {
          double dist = fabs(inputCDF[a_]-targetCDF[b_]);
          if (first || dist < lastDist) {found = b_; lastDist = dist; first = false;}
        }
        matching[a_] = found;
      }

      for (int c_ = 1; c_ < 256-1; c_++) { // not perfect, but whatever
        if (matching[c_]==0) {
          int d = c_, e = c_;
          for(;d>0;d--) if (matching[d]!=0) break;
          for(;e<256;e++) if (matching[e]!=0) break;
          if (e<256)
          for (int j = d+1; j < e; j++) matching[j] = (j-d)*(matching[e]-matching[d])/(e-d)+matching[d];
        }
      }
    }
  }

  double grey2s[3*3];
  double lk = sqrt(tileW*tileW+tileH*tileH);
  {for (int y = 0, ky = 0; y < l->height; y += tileH,ky++) {
    int tileH2 = tileH; if (y + tileH2 > l->height) tileH2 = l->height - y;
    updateStatusBar();
    for (int x = 0, kx = 0; x < l->width; x += tileW,kx++) {
      int tileW2 = tileW; if (x + tileW2 > l->width) tileW2 = l->width - x;

      for (int y4 = 0; y4 < tileH2; y4++) {
        int y3 = y4 + y;
        for (int x4 = 0; x4 < tileW2; x4++) {
          int x3 = x4 + x;
          unsigned int col = l->pixels[x3 + y3*l->width];
          int r = col & 255;
          int g = (col>>8) & 255;
          int b = (col>>16) & 255;
          int grey = (r+g+b)/3;
          for (int y5 = -1; y5 <= 1; y5++) {
            for (int x5 = -1; x5 <= 1; x5++) {
              int x6 = kx + x5;
              int y6 = ky + y5;
              if (x6 >= hcount) x6 = hcount-1;
              if (y6 >= vcount) y6 = vcount-1;
              if (x6 < 0) x6 = 0;
              if (y6 < 0) y6 = 0;
              grey2s[(x5+1)+(y5+1)*3] = remap[(y6*hcount+x6)*256+grey];
            }
          }

          double grey2 = 0;
          double w = 0;
          for (int y6 = -1; y6 <= 1; y6++) {
            for (int x6 = -1; x6 <= 1; x6++) {
              double dx = x + x6 * tileW + tileW/2 - x3;
              double dy = y + y6 * tileH + tileH/2 - y3;
              double l = 1-sqrt(dx*dx+dy*dy)/lk;
              if (l<0) l = 0;
              grey2 += grey2s[(x6+1)+(y6+1)*3]*l;
              w +=l;
            }
          }
          grey2 /= w;
          if (grey != 0) {
            r = r * grey2 / grey;
            g = g * grey2 / grey;
            b = b * grey2 / grey;
          }
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          l->pixels[x3 + y3*l->width] = r|(g<<8)|(b<<16)|(col & 0xff000000);
        }
      }
    }
  }}

  updatedPixels(l);
  afterUndoPoint();
  repaintLayer(l,true);
}

void filterColorTransfer(class Layer *colorSource) {
  Layer *al = currentCanvas->getActiveLayer();
  if (al == NULL) return;
  if (locked(al,LOCK_ANY)) return;
  int bins = 256;
  Array<float> sourceColors; sourceColors.resize(bins*4); memset(&sourceColors[0],0,sourceColors.size() * sizeof(float));
  Array<unsigned int> binColors; binColors.resize(bins); memset(&binColors[0],0,binColors.size() * sizeof(unsigned int));

  float *lumis = luminanceValues;

  for (int i = 0; i < colorSource->width*colorSource->height; i++) {
    unsigned int col = colorSource->pixels[i];
    int r = col & 255;
    int g = (col>>8) & 255;
    int b = (col>>16) & 255;
    int bin = (int)(lumis[0]*r+lumis[1]*g+lumis[2]*b)*bins/256;
    if (bin >= bins) bin = bins-1;
    sourceColors[bin*4+0] += r;
    sourceColors[bin*4+1] += g;
    sourceColors[bin*4+2] += b;
    sourceColors[bin*4+3] += 1;
  }
  if (sourceColors[(bins-1)*4+3]==0) {
    sourceColors[(bins-1)*4+0] = 255;
    sourceColors[(bins-1)*4+1] = 255;
    sourceColors[(bins-1)*4+2] = 255;
    sourceColors[(bins-1)*4+3] = 1;
  }
  for (int j = 0; j < bins; j++) {
    float r = sourceColors[j*4+0];
    float g = sourceColors[j*4+1];
    float b = sourceColors[j*4+2];
    float w = sourceColors[j*4+3];
    if (w != 0) {r /= w; g /= w; b /= w;}
    sourceColors[j*4+0] = r;
    sourceColors[j*4+1] = g;
    sourceColors[j*4+2] = b;
  }
  for (int l = 1; l < bins - 1; l++) {
    if (sourceColors[l*4+3] == 0) {
      int d = l, e = l;
      for(;d>0;d--) if (sourceColors[d*4+3]!=0) break;
      for(;e<bins;e++) if (sourceColors[e*4+3]!=0) break;
      if (e < bins)
      for (int j = d+1; j < e; j++) {
        sourceColors[j*4+0] = (j-d)*(sourceColors[e*4+0]-sourceColors[d*4+0])/(e-d)+sourceColors[d*4+0];
        sourceColors[j*4+1] = (j-d)*(sourceColors[e*4+1]-sourceColors[d*4+1])/(e-d)+sourceColors[d*4+1];
        sourceColors[j*4+2] = (j-d)*(sourceColors[e*4+2]-sourceColors[d*4+2])/(e-d)+sourceColors[d*4+2];
      }
    }
  }
  for (int k = 0; k < bins; k++) {
    int ri = (int)sourceColors[k*4+0];
    int gi = (int)sourceColors[k*4+1];
    int bi = (int)sourceColors[k*4+2];
    binColors[k] = ri|(gi<<8)|(bi<<16);
  }

  beforeUndoPoint();
  for (int m = 0; m < al->width*al->height; m++) {
    unsigned int col = al->pixels[m];
    int r = col & 255;
    int g = (col>>8) & 255;
    int b = (col>>16) & 255;
    int bin = (int)(lumis[0]*r+lumis[1]*g+lumis[2]*b)*bins/256;
    if (bin >= bins) bin = bins-1;
    al->pixels[m] = binColors[bin]|(col & 0xff000000);
  }
  updatedPixels(al);
  afterUndoPoint();
  repaintLayer(al,true);
}

void filterSepia() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;

  beforeUndoPoint();
  for (int i = 0; i < l->width*l->height; i++) {
    unsigned int col = l->pixels[i];
    int r = col & 255;
    int g = (col>>8) & 255;
    int b = (col>>16) & 255;
    int rNew = (int)(r * 0.393 + g * 0.769 + b * 0.189);
    int gNew = (int)(r * 0.349 + g * 0.686 + b * 0.168);
    int bNew = (int)(r * 0.272 + g * 0.534 + b * 0.131);
    if (rNew > 255) rNew = 255;
    if (gNew > 255) gNew = 255;
    if (bNew > 255) bNew = 255;
    l->pixels[i] = rNew | (gNew << 8) | (bNew << 16) | (col & 0xff000000);
  }
  updatedPixels(l);
  afterUndoPoint();
  repaintLayer(l,true);
}

void filterCheckerboard(int w, int h, int xp, int yp) {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;
  if (w <= 0 || h <= 0) return;

  bool hasSelection = l->hasSelection();

  beforeUndoPoint();
  for (int y = 0; y < l->height; y++) {
    for (int x = 0; x < l->width; x++) {
      if ((!hasSelection)||(l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED)) {
        int grey = ((int)floor((double)(x-xp)/w) + (int)floor((double)(y-yp)/h)) & 1;
        grey *= 255;
        l->pixels[x+y*l->width] = grey|(grey<<8)|(grey<<16)|0xff000000;
      }
    }
  }
  updatedPixels(l);
  afterUndoPoint();
  repaintLayer(l,true);
}

void filterHistogramMatchingColored(Layer *histogramSource) {
  Layer *a = currentCanvas->getActiveLayer();
  if (a == NULL) return;
  if (locked(a,LOCK_ANY)) return;

  Layer *l = histogramSource;

  int iw = l->width;  int ih = l->height;
  int tw = a->width;  int th = a->height;

  // converted from github/KIMMINTAE98/Histogram-matching
  Array<int> inputHistogram; inputHistogram.resize(256);
  Array<int> targetHistogram; targetHistogram.resize(256);
  Array<int> matching; matching.resize(256); 
  Array<double> inputCDF; inputCDF.resize(256);
  Array<double> targetCDF; targetCDF.resize(256);

  beforeUndoPoint();
  // it may be that the order of channels is "important" here
  int ch[3];
  for (int channel = 0; channel < 3; channel++) {

    memset(&inputHistogram[0],0,256*sizeof(int));
    memset(&targetHistogram[0],0,256*sizeof(int));
    memset(&matching[0],0,256*sizeof(int));
    memset(&inputCDF[0],0,256*sizeof(double));
    memset(&targetCDF[0],0,256*sizeof(double));

    for (int i0 = 0; i0 < (int)(l->width*l->height); i0++) {
      unsigned int col = l->pixels[i0];
      ch[0] = col & 255;
      ch[1] = (col>>8) & 255;
      ch[2] = (col>>16) & 255;
      int v = (ch[channel]-((ch[0] + ch[1] + ch[2]) / 3))/2+128;
      inputHistogram[v]++;
    }
  
    for (int i1 = 0; i1 < (int)(a->width*a->height); i1++) {
      unsigned int col = a->pixels[i1];
      ch[0] = col & 255;
      ch[1] = (col>>8) & 255;
      ch[2] = (col>>16) & 255;
      int v = (ch[channel]-((ch[0] + ch[1] + ch[2]) / 3))/2+128;
      targetHistogram[v]++;
    }
  
    inputCDF[0] = (double)inputHistogram[0]/(iw*ih);
    targetCDF[0] = (double)targetHistogram[0]/(tw*th);
  
    for (int i = 1; i < 256; i++) {
      inputCDF[i] = inputCDF[i-1]+(double)inputHistogram[i]/(iw*ih);
      targetCDF[i] = targetCDF[i-1]+(double)targetHistogram[i]/(tw*th);
    }
  
    for (int a_ = 0; a_ < 256; a_++) {
      double lastDist = 0; bool first = true; int found = 0;
      for (int b_ = 0; b_ < 256; b_++) {
        double dist = fabs(inputCDF[a_]-targetCDF[b_]);
        if (first || dist < lastDist) {found = b_; lastDist = dist; first = false;}
      }
      matching[a_] = found;
    }
    for (int c_ = 1; c_ < 256-1; c_++) { // not perfect, but whatever
      if (matching[c_]==0) {
        int d = c_, e = c_;
        for(;d>0;d--) if (matching[d]!=0) break;
        for(;e<256;e++) if (matching[e]!=0) break;
        if (e<256)
        for (int j = d+1; j < e; j++) matching[j] = (j-d)*(matching[e]-matching[d])/(e-d)+matching[d];
      }
    }
  
    for (int i2 = 0; i2 < (int)(a->width*a->height); i2++) {
      unsigned int col = a->pixels[i2];
      ch[0] = col & 255;
      ch[1] = (col>>8) & 255;
      ch[2] = (col>>16) & 255;
      int v = (ch[channel]-((ch[0] + ch[1] + ch[2]) / 3))/2+128;
      int v2 = matching[v];
      double cSat = (v2 - 128) * 2;
      double grey = (ch[0] + ch[1] + ch[2]) / 3;
      ch[channel] = grey + cSat;
      if (ch[channel]<0) ch[channel] = 0;
      if (ch[channel]>255) ch[channel] = 255;
      a->pixels[i2] = ch[0]|(ch[1]<<8)|(ch[2]<<16)|(col & 0xff000000);
    }
  }
  updatedPixels(a);
  afterUndoPoint();
  repaintLayer(a,true);
}

int medianLumaSort(const void *p, const void *q)
{
  const double a = *((const double*)p);
  const double b = *((const double*)q);
  if (a > b) return 1;
  if (a < b) return -1;
  return 0;
}

void filterSoften() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;

  beforeUndoPoint();
  int kernel = 4;
  Array<double> medians; medians.resize((kernel*2+1) * (kernel*2+1)*3);

  Array<unsigned int> source;
  source.resize(l->width*l->height);
  memcpy(&source[0],l->pixels,l->width*l->height*sizeof(unsigned int));

  initStatusBar();
  bool hasSelection = l->hasSelection();

  for (int y = 0; y < l->height; y++) {
    if ((y % 100)==0) updateStatusBar();
    for (int x = 0; x < l->width; x++) {
      if ((!hasSelection)||(l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED)) {
        int siz = 0;
        double average[3]; average[0] = 0; average[1] = 0; average[2] = 0; 
        for (int y2 = -kernel; y2 <= kernel; y2++) {
          for (int x2 = -kernel; x2 <= kernel; x2++) {
            int x3 = x + x2;
            int y3 = y + y2;
            if ((unsigned int)x3<(unsigned int)l->width&&(unsigned int)y3<(unsigned int)l->height) {
              unsigned int col = source[x3+y3*l->width];
              double r = col & 255;
              double g = (col>>8) & 255;
              double b = (col>>16) & 255;
              r /= 255.0; g /= 255.0; b /= 255.0;
              medians[siz+(kernel*2+1) * (kernel*2+1) * 0] = r;
              medians[siz+(kernel*2+1) * (kernel*2+1) * 1] = g;
              medians[siz+(kernel*2+1) * (kernel*2+1) * 2] = b;
              siz++;
              average[0] += r;
              average[1] += g;
              average[2] += b;
            }
          }
        }
        unsigned int col = source[x+y*l->width];
        double r2 = col & 255;
        double g2 = (col>>8) & 255;
        double b2 = (col>>16) & 255;
        r2 /= 255.0; g2 /= 255.0; b2 /= 255.0;
        double here[3]; here[0] = r2; here[1] = g2; here[2] = b2;
  
        if (siz > 2) for (int i = 0; i < 3; i++) qsort(&medians[(kernel*2+1) * (kernel*2+1) * i], siz, sizeof(double), medianLumaSort);
        if (siz > 1) {average[0] /= siz; average[1] /= siz; average[2] /= siz;}
        col = 0;
        for (int i = 0; i < 3; i++) {
          double median = medians[siz/2+(kernel*2+1) * (kernel*2+1) * i];
          double delta = 1-fabs(here[i]-average[i]); // the smaller this value the more highfrequent
          double grey = here[i] * (1-delta) + median * delta;
          int gr = (int)(grey * 255.0);
          if (gr < 0) gr = 0;
          if (gr > 255) gr = 255;
          col |= gr << (i*8);
        }
        l->pixels[x+y*l->width] = col|(source[x+y*l->width]&0xff000000);
      }
    }
  }
  updatedPixels(l);
  afterUndoPoint();
  repaintLayer(l,true);
}

void filterBlackWhiteOCR() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (locked(l,LOCK_ANY)) return;

  beforeUndoPoint();
  int kernel = 4;
  float *lumis = luminanceValues;

  Array<unsigned int> source;
  source.resize(l->width*l->height);
  memcpy(&source[0],l->pixels,l->width*l->height*sizeof(unsigned int));

  initStatusBar();
  bool hasSelection = l->hasSelection();

  for (int y = 0; y < l->height; y++) {
    if ((y % 100)==0) updateStatusBar();
    for (int x = 0; x < l->width; x++) {
      if ((!hasSelection)||(l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED)) {
        int siz = 0;
        double average = 0;
        double minLuma = 0;
        double maxLuma = 0;
        for (int y2 = -kernel; y2 <= kernel; y2++) {
          for (int x2 = -kernel; x2 <= kernel; x2++) {
            int x3 = x + x2;
            int y3 = y + y2;
            if ((unsigned int)x3<(unsigned int)l->width&&(unsigned int)y3<(unsigned int)l->height) {
              unsigned int col = source[x3+y3*l->width];
              double r = col & 255;
              double g = (col>>8) & 255;
              double b = (col>>16) & 255;
              r /= 255.0; g /= 255.0; b /= 255.0;
              double luma = r * lumis[0] + g * lumis[1] + b * lumis[2];
              average += luma;
              if (siz == 0 || luma < minLuma) minLuma = luma;
              if (siz == 0 || luma > maxLuma) maxLuma = luma;
              siz++;
            }
          }
        }
        if (siz > 1) {average /= siz;}
        unsigned int col = source[x+y*l->width];
        double r2 = col & 255;
        double g2 = (col>>8) & 255;
        double b2 = (col>>16) & 255;
        r2 /= 255.0; g2 /= 255.0; b2 /= 255.0;
        double luma = r2 * lumis[0] + g2 * lumis[1] + b2 * lumis[2];
        //double l2 = luma < average ? minLuma : maxLuma;
        double l2 = maxLuma == 0 ? 0 : luma / maxLuma;
        int gr = (int)(l2 * 255.0);
        if (gr < 0) gr = 0;
        if (gr > 255) gr = 255;
        l->pixels[x+y*l->width] = gr|(gr<<8)|(gr<<16)|(0xff000000);
      }
    }
  }
  updatedPixels(l);
  afterUndoPoint();
  repaintLayer(l,true);
}

