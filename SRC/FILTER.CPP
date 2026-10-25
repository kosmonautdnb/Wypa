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
#include "FILTER.HPP"
#include "STL/GL.H"
#include "IMGUI/IMGUIGL.HPP"
#include "CONFIG.HPP"
#include "LAYERS.HPP"
#include "SERIALZE.HPP"
#include "UNDO.HPP"
#include "TOOLS.HPP"
#include <math.h>
#define __SUPERXBR_IMPLEMENTATION__
#include "STL/SUPERXBR.HPP"
#include "COLORFLD.HPP"

float sfrand2d(int seed,float x, float y) {
  x *= 3.3333f;
  y *= 7.7777f;
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

void redFlash();
#define UPDATESTATUSBARWITHBREAK(__func__) {if (updateStatusBar()) {__func__; redFlash(); return false;}}

bool filterDifferenceClouds() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
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
        float v00 = sfrand2d(seed, (float)ix0,(float)iy0);
        float v10 = sfrand2d(seed, (float)ix0+1, (float)iy0);
        float v01 = sfrand2d(seed, (float)ix0, (float)iy0+1);
        float v11 = sfrand2d(seed, (float)ix0+1, (float)iy0+1);
        float fx = (float)(x % tileSize)/tileSize;
        float fy = (float)(y % tileSize)/tileSize;
        //float top = (v10 - v00)*fx + v00;
        //float bottom = (v11 - v01)*fx + v01;
        //float v = (bottom-top)*fy + top;
        float influence00 = 1.f-(float)sqrt(fx*fx+fy*fy);
        float influence10 = 1.f-(float)sqrt((1-fx)*(1-fx)+fy*fy);
        float influence01 = 1.f-(float)sqrt(fx*fx+(1-fy)*(1-fy));
        float influence11 = 1.f-(float)sqrt((1-fx)*(1-fx)+(1-fy)*(1-fy));
        if (influence00<0) influence00 = 0;
        if (influence10<0) influence10 = 0;
        if (influence01<0) influence01 = 0;
        if (influence11<0) influence11 = 0;
        float all = influence00 + influence10 + influence01 + influence11;
        float v = (influence00 * v00 + influence10 * v10 + influence01 * v01 + influence11 * v11) / all;
        values[x+y*l->width] += v*mul;
      }
    }
    UPDATESTATUSBARWITHBREAK(;);
    seed += 571;
    seed *= 3;
    seed &= 0xffff;
    tileSize >>= 1;
    mul *= 0.5f;
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
      int r = (int)(col & 255);
      int g = (int)((col>>8) & 255);
      int b = (int)((col>>16) & 255);
      int a = (int)((col>>24) & 255);
      int d = (int)(values[i3] * 255);
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
  return true;
}

// KISS (Keep it simple stupid) random numbers by George Marsaglia
unsigned int noise_x = 123456789; unsigned int noise_y = 362436000; unsigned int noise_z = 521288629; unsigned int noise_c = 7654321;
void initNoise32() {noise_x = 123456789; noise_y = 362436000; noise_z = 521288629; noise_c = 7654321;}
unsigned int noise32() {
  noise_x = 69069 * noise_x + 12345;
  noise_y ^= noise_y << 13; noise_y ^= noise_y >> 17; noise_y ^= noise_y << 5;
  unsigned int t_0 = (698769069 & 0xffff) * (noise_z & 0xffff);
  unsigned int t_1 = (698769069 & 0xffff) * ((noise_z >> 16) & 0xffff);
  unsigned int t_2 = (698769069 >> 16) * (noise_z & 0xffff);
  unsigned int t_3 = (698769069 >> 16) * ((noise_z >> 16) & 0xffff);
  noise_z = t_0 + (t_1<<16) + (t_2<<16) + noise_c;
  noise_c = ((t_0 + (t_1<<16) + (t_2<<16))>>16)+(t_3<<16);
  return noise_x + noise_y + noise_z;
}

bool filterNoise() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
  beforeUndoPoint();
  bool hasSelection = l->hasSelection();
  //srand(0);
  initNoise32();
  for (int i3 = 0; i3 < l->width*l->height; i3++) {
    if ((!hasSelection)||(l->pixelStates[i3] & PIXELSTATE_SELECTED)) {
      int grey = noise32() & 255; //rand() & 255;
      l->pixels[i3] = grey|(grey<<8)|(grey<<16)|(255<<24);
    }
  }
  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
  return true;
}

bool filterSobelEdgeDetect() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
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
      values[(x+y*l->width)*3+0] = (float)maxf((float)fabs(sobelXr), (float)fabs(sobelYr));
      values[(x+y*l->width)*3+1] = (float)maxf((float)fabs(sobelXg), (float)fabs(sobelYg));
      values[(x+y*l->width)*3+2] = (float)maxf((float)fabs(sobelXb), (float)fabs(sobelYb));
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
      int r = (int)(values[i3*3+0]*255);
      int g = (int)(values[i3*3+1]*255);
      int b = (int)(values[i3*3+2]*255);
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
  return true;
}

bool filterMorphologicalEdgeDetect() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
  beforeUndoPoint();
  bool hasSelection = l->hasSelection();
  float maxv2 = 0;
  float minv2 = 0;
  Array<unsigned int> source;
  source.resize(l->width*l->height);
  memcpy(&source[0],l->pixels,l->width*l->height*sizeof(unsigned int));
  int posX[5] ={0,-1,1,0,0};
  int posY[5] ={0,0,0,-1,1};
  bool first2 = true;
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
              if (first || l > maxv) maxv = (float)l;
              if (first || l < minv) minv = (float)l;
              first = false;
            }
          }
        }
        int k = (int)(maxv-minv);
        if (first2 || k > maxv2) maxv2 = (float)k;
        if (first2 || k < minv2) minv2 = (float)k;
        first2 = false;
        l->pixels[i3] = k;
      }
    }
  }
  float scale = maxv2-minv2;
  if (scale != 0) {
    for (int i3 = 0; i3 < l->width*l->height; i3++) {
      if ((!hasSelection)||(l->pixelStates[i3] & PIXELSTATE_SELECTED)) {
        unsigned int v = l->pixels[i3];
        v = (unsigned int)((v-minv2) * 255/scale);
        l->pixels[i3] = v|(v<<8)|(v<<16)|(0xff000000);
      }
    }
  }

  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
  return true;
}

bool filterAlphaSoften() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
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
  return true;
}

bool filterBoxBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int boxW, int boxH, unsigned char *pixelStates, bool usePixelStates) {
  if (boxW < 1 || boxH < 1 || boxW >= width*1.5 || boxH >= height*1.5) {memcpy(dest,source,width*height*sizeof(unsigned int)); return false;}
  initStatusBar();
  Array<unsigned int> blurred;
  blurred.resize(width*height*5);
  for (int y = 0; y < (int)height; y++) {
    if ((y & 15)==0) UPDATESTATUSBARWITHBREAK(memcpy(dest,source,width*height*sizeof(unsigned int)));
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
    if ((y4 & 15)==0) UPDATESTATUSBARWITHBREAK(memcpy(dest,source,width*height*sizeof(unsigned int)));
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
  return true;
}

bool filterRadialBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float radialAngle, float radialSize, unsigned char *pixelStates, bool usePixelStates) {
  if (radialSize < 1 || radialSize > width+height) {memcpy(dest,source,width*height*sizeof(unsigned int)); return false;}
  initStatusBar();
  float r = (float)(-radialAngle * PI * 2 / 360.0);
  float stepX = (float)sin(r);
  float stepY = (float)cos(r);
  for (int y4 = 0; y4 < (int)height; y4++) {
    if ((y4 & 15)==0) UPDATESTATUSBARWITHBREAK(memcpy(dest,source,width*height*sizeof(unsigned int)));
    for (int x4 = 0; x4 < (int)width; x4++) {
      if ((!usePixelStates)||(pixelStates[x4+y4*width] & PIXELSTATE_SELECTED)) {
        int r = 0;
        int g = 0;
        int b = 0;
        int a = 0;
        int w = 0;
        for (int k = (int)-radialSize; k <= (int)radialSize; k++) {
          int x3 = (int)(stepX * k + x4);
          int y3 = (int)(stepY * k + y4);
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
  return true;
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

#define __SX(__x__) bloomLookUp((float)__x__,(float)y,s,width,height)
#define __SY(__y__) bloomLookUp((float)x,(float)__y__,s,width,height)

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
      float t3x = x + tap1; if (t3x > width-1) t3x = (float)width-1;
      float t4x = x + tap0; if (t4x > width-1) t4x = (float)width-1;
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
      float t3x = x + tap1; if (t3x > width-1) t3x = (float)width-1;
      float t4x = x + tap0; if (t4x > width-1) t4x = (float)width-1;
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
      float t3y = y + tap1; if (t3y > height-1) t3y = (float)height-1;
      float t4y = y + tap0; if (t4y > height-1) t4y = (float)height-1;
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
      float t3y = y + tap1; if (t3y > height-1) t3y = (float)height-1;
      float t4y = y + tap0; if (t4y > height-1) t4y = (float)height-1;
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


bool filterGaussBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float radiusX, float radiusY, unsigned char *pixelStates, bool usePixelStates) {

  if (radiusX < 1 || radiusY < 1) {memcpy(dest,source,width*height*sizeof(unsigned int)); return false;}

  double passesX = log(radiusX)/log(2.0);
  double passesY = log(radiusY)/log(2.0);

  int px = (int)floor(passesX);
  int py = (int)floor(passesY);

  if (px < 1 || py < 1) {memcpy(dest,source,width*height*sizeof(unsigned int)); return false;}

  initStatusBar();

  Array<unsigned int> bloomBuffer2; bloomBuffer2.resize(width*height);
  unsigned int *bloomBuffer = &bloomBuffer2[0]; for (int i0 = 0; i0 < (int)(width*height); i0++) bloomBuffer[i0] = source[i0];

  if (passesX > px) {
    UPDATESTATUSBARWITHBREAK(memcpy(dest,source,width*height*sizeof(unsigned int)));
    float kernelSize = (float)passesX;
    bloomXFiltered(bloomBuffer,width,height,kernelSize*2,kernelSize*1);
  }

  for (int i1 = px-1; i1 >= 0; i1--) {
    UPDATESTATUSBARWITHBREAK(memcpy(dest,source,width*height*sizeof(unsigned int)));
    float kernelSize = (float)pow(2,i1);
    bloomX(bloomBuffer,width,height,kernelSize*2,kernelSize*1);
  }

  if (passesY > py) {
    UPDATESTATUSBARWITHBREAK(memcpy(dest,source,width*height*sizeof(unsigned int)));
    float kernelSize = (float)passesY;
    bloomYFiltered(bloomBuffer,width,height,kernelSize*2,kernelSize*1);
  }

  for (int i2 = py-1; i2 >= 0; i2--) {
    UPDATESTATUSBARWITHBREAK(memcpy(dest,source,width*height*sizeof(unsigned int)));
    float kernelSize = (float)pow(2,i2);
    bloomY(bloomBuffer,width,height,kernelSize*2,kernelSize*1);
  }

  for (int i3 = 0; i3 < (int)(width*height); i3++) {
    if ((!usePixelStates)||(pixelStates[i3] & PIXELSTATE_SELECTED)) {
      dest[i3] = bloomBuffer[i3];
    }
  }

  return true;
}

bool filterIterativeBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int steps, unsigned char *pixelStates, bool usePixelStates) {
  if (steps < 1) {memcpy(dest,source,width*height*sizeof(unsigned int)); return false;}
  initStatusBar();
  Array<unsigned int> f;
  f.resize(width*height);
  memcpy(&f[0],source,width*height*sizeof(unsigned int));
  int posX[5] ={0,-1,1,0,0};
  int posY[5] ={0,0,0,-1,1};
  for (int i = 0; i < steps; i++) {
    for (int y4 = 0; y4 < (int)height; y4++) {
      if ((y4 & 15)==0) UPDATESTATUSBARWITHBREAK(memcpy(dest,source,width*height*sizeof(unsigned int)));
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
  return true;
}

int medianIntSort(const void *p, const void *q)
{
  const int a = *((const int*)p);
  const int b = *((const int*)q);
  if (a > b) return 1;
  if (a < b) return -1;
  return 0;
}

bool filterMedianBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int medianW, int medianH, unsigned char *pixelStates, bool usePixelStates, bool circular, bool keepSelection) {
  if (medianW < 1 || medianH < 1 || medianW >= width*1.5 || medianH >= height*1.5) {memcpy(dest,source,width*height*sizeof(unsigned int)); return false;}
  // dest and source are different here
  memcpy(dest,source,width*height*sizeof(unsigned int));
  const int mw = (medianW*2+1) * (medianH*2+1);
  Array<unsigned int> medianx; medianx.resize(mw); unsigned int *medianx2 = &medianx[0];
  Array<unsigned char> medianxs; medianxs.resize(mw); unsigned char *medianxs2 = &medianxs[0];
  Array<int> medians; medians.resize(mw*3); int *medians2 = &medians[0];
  Array<int> countSort; countSort.resize(256); int *countSort2 = &countSort[0];
  initStatusBar();
  for (int y = 0; y < (int)height; y++) {
    if ((y % 100)==0) UPDATESTATUSBARWITHBREAK(memcpy(dest,source,width*height*sizeof(unsigned int)));
    bool reScan = true; bool scanned = false;
    for (int x = 0; x < (int)width; x++) {
      if (scanned) {
        if (x < (int)width - medianW) {
          for (int y2 = 0; y2 < medianH*2+1; y2++) {
            for (int x2 = 0; x2 < medianW*2+1-1; x2++) {
              int siz2 = x2 + y2 * (medianW*2+1);
              medianx2[siz2] = medianx2[siz2+1];
              if (keepSelection) medianxs2[siz2] = medianxs2[siz2+1];
            }
            medianx2[medianW*2+y2*(medianW*2+1)] = source[x+medianW+(y+y2-medianH)*width];
            if (keepSelection) medianxs2[medianW*2+y2*(medianW*2+1)] = pixelStates[x+medianW+(y+y2-medianH)*width];
          }
        } else scanned = false;
      }
      if (reScan && x >= medianW && y >= medianH && x < (int)width - medianW && y < (int)height - medianH) {
        reScan = false; scanned = true; int siz2 = 0;
        for (int y2 = -medianH; y2 <= medianH; y2++) {
          for (int x2 = -medianW; x2 <= medianW; x2++) {
            medianx2[siz2] = source[x2+x+(y2+y)*width];
            if (keepSelection) medianxs2[siz2] = pixelStates[x2+x+(y2+y)*width];
            siz2++;
          }
        }
      }
      if ((!usePixelStates)||(pixelStates[x+y*width] & PIXELSTATE_SELECTED)) {
        bool some = false;
        if (scanned) {
          int m[3]; for (int i = 0; i < 3; i++) {
            int lk = i * 8;
            memset(&countSort[0],0,sizeof(int)*countSort.size());
            int all = 0;
            if (!circular) {
              if (keepSelection) {
                for (int j = 0; j < mw; j++) if (medianxs2[j] & PIXELSTATE_SELECTED) {countSort2[(medianx2[j]>>lk)&255]++; all++;}
              } else {
                all = mw;
                for (int j = 0; j < all; j++) countSort2[(medianx2[j]>>lk)&255]++;
              }
            } else {
              int j = 0;
              if (keepSelection) {
                for (int y2 = -medianH; y2 <= medianH; y2++) { float yd = (float)y2 / medianH; yd *= yd;
                  for (int x2 = -medianW; x2 <= medianW; x2++) { float xd = (float)x2 / medianW;
                    if (xd*xd+yd <= 1) {
                      if (medianxs2[j] & PIXELSTATE_SELECTED) { countSort2[(medianx2[j]>>lk)&255]++; all++;}
                    }
                    j++;
                  }
                }
              } else {
                for (int y2 = -medianH; y2 <= medianH; y2++) { float yd = (float)y2 / medianH; yd *= yd;
                  for (int x2 = -medianW; x2 <= medianW; x2++) { float xd = (float)x2 / medianW;
                    if (xd*xd+yd <= 1) {
                      countSort2[(medianx2[j]>>lk)&255]++;
                      all++;
                    }
                    j++;
                  }
                }
              }
            }
            int countHere = 0; int overAllCount = 0;
            int k = 0; for (k = 0; k < 256; k++) {countHere = countSort2[k]; overAllCount += countHere; if (overAllCount>all/2) break;}
            m[i] = k < 256 ? k : 255;
            if (all > 0) some = true;
          }
          if (some) {
            int r = m[0]; int g = m[1]; int b = m[2];
            dest[x+y*width] = r | (g<<8) | (b<<16) | (source[x+y*width] & 0xff000000);
          }
        } else {
          // slow but fully working version
          int siz = 0;
          for (int y2 = -medianH; y2 <= medianH; y2++) {
            float yd = (float)y2 / medianH;
            for (int x2 = -medianW; x2 <= medianW; x2++) {
              if (circular) {
                float xd = (float)x2 / medianW;
                float d = xd*xd+yd*yd;
                if (d > 1) continue;
              }
              int x3 = x + x2;
              int y3 = y + y2;
              if ((unsigned int)x3<(unsigned int)width&&(unsigned int)y3<(unsigned int)height) {
                if ((!keepSelection) || (!usePixelStates) || (pixelStates[x3+y3*width] & PIXELSTATE_SELECTED)) {
                  unsigned int col = source[x3+y3*width];
                  int r = col & 255;
                  int g = (col>>8) & 255;
                  int b = (col>>16) & 255;
                  medians2[siz+mw*0] = r;
                  medians2[siz+mw*1] = g;
                  medians2[siz+mw*2] = b;
                  siz++;
                }
              }
            }
          }
          if (siz > 2) for (int i = 0; i < 3; i++) qsort(&medians2[mw*i], siz, sizeof(int), medianIntSort);
          if (siz > 0) {
            int r = medians2[siz/2+mw*0];
            int g = medians2[siz/2+mw*1];
            int b = medians2[siz/2+mw*2];
            dest[x+y*width] = r | (g<<8) | (b<<16) | (source[x+y*width] & 0xff000000);
          }
        }
      }
    }
  }
  return true;
}

bool filterEmboss(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float angle, unsigned char *pixelStates, bool usePixelStates) {
  int sobelXMat[3*3] = {+1,+0,-1, +2,+0,-2, +1,+0,-1};
  int sobelYMat[3*3] = {+1,+2,+1, 0,0,0, -1,-2,-1};

  Array<float> temp;
  temp.resize(width*height*2);
  for (int i = 0; i < (int)(width*height); i++) {
    unsigned int col = source[i];
    int r = col & 255;
    int g = (col>>8) & 255;
    int b = (col>>16) & 255;
    temp[i] = (float)(r+g+b);
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
          float angleHere = (float)atan2(sobelX,sobelY);
          val = (float)sin(angleHere+(angle-90)*PI*2/360.0);
          int grey = (int)(val * 128 + 128);
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
  return true;
}

bool filterSimpleKeying(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int channel, float factor, unsigned char *pixelStates, bool usePixelStates, int mode, bool binary, bool invert) {
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
        a = (int)((fc - v)*factor);
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
  return true;
}

bool filterColorKeying(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, unsigned int color, float factor, unsigned char *pixelStates, bool usePixelStates, int mode, bool binary, bool invert) {
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
        a = (int)(q * factor);
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
  return true;
}

bool filterDenoise() {
  int posX[5] = {0,1,-1,0,0};
  int posY[5] = {0,0,0,1,-1};
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
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
  return true;
}

bool filterSharpen() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
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
  return true;
}

bool filterXBRAntialias() {
  // this algorithm could take very long
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
  int w = l->width, h = l->height;
  int x0 = 0; int x1 = w; int y0 = 0; int y1 = h;
  bool hasSelection = l->hasSelection();
  Array<unsigned int> source;
  if (hasSelection) {
    x0 = -1; y0 = -1; x1 = -1; y1 = -1;
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        if (l->pixelStates[x+y*w] & PIXELSTATE_SELECTED) {
          if (x < x0 || x0 < 0) x0 = x;
          if (y < y0 || y0 < 0) y0 = y;
          if (x > x1 || x1 < 0) x1 = x;
          if (y > y1 || y1 < 0) y1 = y;
        }
      }  
    }
    x1++;
    y1++;
    if (x0 < 0 || y0 < 0 || x1 <= 0 || y1 <= 0) return false;
    w = x1 - x0;
    h = y1 - y0;
    if (w <= 0 || h <= 0) return false;
    source.resize(w*h);
    for (int y2 = 0; y2 < h; y2++) {
      for (int x2 = 0; x2 < w; x2++) {
        source[x2+y2*w] = l->pixels[x2+x0+(y2+y0)*l->width];
      }
    }
  } else {
    source.resize(w*h);
    memcpy(&source[0],l->pixels,w*h*sizeof(unsigned int));
  }
  beforeUndoPoint();
  Array<unsigned int> dest; dest.resize(w*2*h*2);
  superXBR(&source[0],&dest[0],w,h); // somehow this algorithm is a bit broken, sorry.. Not my fault currently..

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      if ((!hasSelection)||(l->pixelStates[(x+x0)+(y+y0)*l->width] & PIXELSTATE_SELECTED)) {
        int adr = x * 2 + y * 2 * w * 2;
        unsigned int c00 = dest[adr];
        unsigned int c10 = dest[adr+1];
        unsigned int c11 = dest[adr+1+w*2];
        unsigned int c01 = dest[adr+w*2];
        int r = ((c00 & 255) + (c10 & 255) + (c01 & 255) + (c11 & 255))>>2;
        int g = (((c00>>8) & 255) + ((c10>>8) & 255) + ((c01>>8) & 255) + ((c11>>8) & 255))>>2;
        int b = (((c00>>16) & 255) + ((c10>>16) & 255) + ((c01>>16) & 255) + ((c11>>16) & 255))>>2;
        int a = (((c00>>24) & 255) + ((c10>>24) & 255) + ((c01>>24) & 255) + ((c11>>24) & 255))>>2;
        l->pixels[(x+x0)+(y+y0)*l->width] = r|(g<<8)|(b<<16)|(a<<24);
      }
    }
  }

  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
  return true;
}

bool filterSuperXBR() {
  // this algorithm could take very long
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
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
  return true;
}

bool filterEagle() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
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
  return true;
}

bool filterEpx() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
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
  return true;
}

double __round(double v) {
  return floor(v+0.5);
}

bool filterHistogramMatching(Layer *histogramSource) {
  Layer *a = currentCanvas->getActiveLayer();
  if (a == NULL) return false;
  if (locked(a,LOCK_ANY)) return false;

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
  return true;
}

bool filterClahe(float claheConstant, int cellSize) {
  // not a perfect Clahe implementation
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;

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
    UPDATESTATUSBARWITHBREAK(;);
    hcount = 0;
    for (int x = 0; x < l->width; x += tileW) {
      hcount++;
      int tileW2 = tileW; if (x + tileW2 > l->width) tileW2 = l->width - x;

      int remapStart = (int)remap.size(); remap.resize(remapStart + 256); int *matching = &remap[remapStart];

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
      claheClip = average * claheAverageFactor; // currently we use this as local clahe clip factor

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
    updateStatusBar(); // pixels are modified so no break allowed
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
            r = (int)(r * grey2 / grey);
            g = (int)(g * grey2 / grey);
            b = (int)(b * grey2 / grey);
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
  return true;
}

bool filterColorTransfer(class Layer *colorSource) {
  Layer *al = currentCanvas->getActiveLayer();
  if (al == NULL) return false;
  if (locked(al,LOCK_ANY)) return false;
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
  return true;
}

bool filterSepia() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;

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
  return true;
}

bool filterSchraffur(float blackLineWidth, float transparentLineWidth, float angle, bool binary) {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
  double angle2 = (double)angle * PI / 180.0;
  double ax = cos(angle2);
  double ay = -sin(angle2);
  // since pixel size is different at e.g. 45 degrees (0.707..), the whole image gets warbled in !binary mode
  // this routine is currently very suboptimal
  double all = (double)blackLineWidth + (double)transparentLineWidth;
  if (all <= 0 || blackLineWidth <= 0 || transparentLineWidth <= 0) return false;
  bool hasSelection = l->hasSelection();
  beforeUndoPoint();
  for (int y = 0; y < l->height; y++) {
    for (int x = 0; x < l->width; x++) {
      if ((!hasSelection)||(l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED)) {
        double d = ax*(double)x+ay*(double)y;
        if (d >= 0) {
          d = fmod(d,all);
        } else {
          d = all-fmod(-d,all);
        }
        double value = 0;
        if (!binary) {
          if (d > all - 1) {
            value = d - (all-1);
          } else {
            if (d > (double)blackLineWidth-1 && d < (double)blackLineWidth) {
              value = 1 - (d - ((double)blackLineWidth-1));
            } else if (d <= (double)blackLineWidth-1.f) {
              value = 1;
            }
          }
        } else {
          if (d < (double)blackLineWidth) value = 1;
        }
        int alpha = (int)((1-value) * 255.0);
        if (alpha < 0) alpha = 0;
        if (alpha > 255) alpha = 255;
        unsigned int col = l->pixels[x+y*l->width];
        int r = (int)(col & 255) * alpha / 255;
        int g = (int)((col>>8) & 255) * alpha / 255;
        int b = (int)((col>>16) & 255) * alpha / 255;
        int a = (int)((col>>24) & 255) * alpha / 255 + (255-alpha);
        l->pixels[x+y*l->width] = r | (g << 8) | (b << 16) | (a << 24);
      }
    }
  }
  updatedPixels(l);
  afterUndoPoint();
  repaintLayer(l,true);
  return true;
}

bool filterCheckerboard(int w, int h, int xp, int yp) {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
  if (w <= 0 || h <= 0) return false;

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
  return true;
}

bool filterHistogramMatchingColored(Layer *histogramSource) {
  Layer *a = currentCanvas->getActiveLayer();
  if (a == NULL) return false;
  if (locked(a,LOCK_ANY)) return false;

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
      ch[channel] = (int)(grey + cSat);
      if (ch[channel]<0) ch[channel] = 0;
      if (ch[channel]>255) ch[channel] = 255;
      a->pixels[i2] = ch[0]|(ch[1]<<8)|(ch[2]<<16)|(col & 0xff000000);
    }
  }
  updatedPixels(a);
  afterUndoPoint();
  repaintLayer(a,true);
  return true;
}

int medianLumaSort(const void *p, const void *q)
{
  const double a = *((const double*)p);
  const double b = *((const double*)q);
  if (a > b) return 1;
  if (a < b) return -1;
  return 0;
}

bool filterSoften() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;

  int medianW = 4, medianH = 4;
  int width = l->width;
  int height = l->height;
  Array<unsigned int> source2; source2.resize(l->width*l->height); memcpy(&source2[0],l->pixels,l->width*l->height*sizeof(unsigned int)); 
  unsigned int *source = &source2[0];
  unsigned int *dest = l->pixels;
  bool usePixelStates = l->hasSelection();
  unsigned char *pixelStates = l->pixelStates;

  // dest and source are different here
  const int mw = (medianW*2+1) * (medianH*2+1);
  Array<unsigned int> medianx; medianx.resize(mw); unsigned int *medianx2 = &medianx[0];
  Array<int> medians; medians.resize(mw*3); int *medians2 = &medians[0];
  Array<int> countSort; countSort.resize(256); int *countSort2 = &countSort[0];
  beforeUndoPoint();
  initStatusBar();
  for (int y = 0; y < (int)height; y++) {
    if ((y % 100)==0) UPDATESTATUSBARWITHBREAK(memcpy(dest,source,width*height*sizeof(unsigned int)));
    bool reScan = true; bool scanned = false;
    for (int x = 0; x < (int)width; x++) {
      if (scanned) {
        if (x < width - medianW) {
          for (int y2 = 0; y2 < medianH*2+1; y2++) {
            for (int x2 = 0; x2 < medianW*2+1-1; x2++) {
              int siz2 = x2 + y2 * (medianW*2+1);
              medianx2[siz2] = medianx2[siz2+1];
            }
            medianx2[medianW*2+y2*(medianW*2+1)] = source[x+medianW+(y+y2-medianH)*width];
          }
        } else scanned = false;
      }
      if (reScan && x >= medianW && y >= medianH && x < width - medianW && y < height - medianH) {
        reScan = false; scanned = true; int siz2 = 0;
        for (int y2 = -medianH; y2 <= medianH; y2++) {
          for (int x2 = -medianW; x2 <= medianW; x2++) {
            medianx2[siz2++] = source[x2+x+(y2+y)*width];
          }
        }
      }
      if ((!usePixelStates)||(pixelStates[x+y*width] & PIXELSTATE_SELECTED)) {
        double m[3]; 
        double a[3];
        double here[3];
        if (scanned) {
          for (int i = 0; i < 3; i++) {
            int lk = i * 8;
            memset(&countSort[0],0,sizeof(int)*countSort.size());
            a[i] = 0;
            for (int j = 0; j < mw; j++) {int v = (medianx2[j]>>lk)&255;countSort2[v]++; a[i] += v;}
            if (mw != 0) a[i] /= mw;
            int countHere = 0; int overAllCount = 0;
            int k = 0; for (k = 0; k < 256; k++) {countHere = countSort2[k]; overAllCount += countHere; if (overAllCount>mw/2) break;}
            m[i] = k < 256 ? k : 255;
          }
        } else {
          // slow but fully working version
          int siz = 0;
          a[0] = 0; a[1] = 0; a[2] = 0;
          for (int y2 = -medianH; y2 <= medianH; y2++) {
            for (int x2 = -medianW; x2 <= medianW; x2++) {
              int x3 = x + x2;
              int y3 = y + y2;
              if ((unsigned int)x3<(unsigned int)width&&(unsigned int)y3<(unsigned int)height) {
                unsigned int col = source[x3+y3*width];
                int r = col & 255;
                int g = (col>>8) & 255;
                int b = (col>>16) & 255;
                medians2[siz+mw*0] = r;
                medians2[siz+mw*1] = g;
                medians2[siz+mw*2] = b;
                a[0] += r;
                a[1] += g;
                a[2] += b;
                siz++;
              }
            }
          }
          if (siz > 2) for (int i = 0; i < 3; i++) qsort(&medians[mw*i], siz, sizeof(int), medianIntSort);
          if (siz != 0) {a[0] /= siz; a[1] /= siz; a[2] /= siz;}
          m[0] = medians[siz/2+mw*0];
          m[1] = medians[siz/2+mw*1];
          m[2] = medians[siz/2+mw*2];
        }
        unsigned int col = source[x+y*l->width];
        here[0] = col & 255;
        here[1] = (col>>8) & 255;
        here[2] = (col>>16) & 255;

        col = 0x00000000;
        for (int i = 0; i < 3; i++) {
          a[i] /= 255.0;
          m[i] /= 255.0;
          here[i] /= 255.0;
          double median = m[i];
          double delta = 1-fabs(here[i]-a[i]); // the smaller this value the more highfrequent
          double grey = here[i] * (1-delta) + median * delta;
          int gr = (int)(grey * 255.0);
          if (gr < 0) gr = 0;
          if (gr > 255) gr = 255;
          col |= gr << (i*8);
        }
        dest[x+y*width] = col | (source[x+y*width] & 0xff000000);
      }
    }
  }
  updatedPixels(l);
  afterUndoPoint();
  repaintLayer(l,true);
  return true;
}

bool filterBlackWhiteOCR() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;

  beforeUndoPoint();
  int kernel = 4;
  float *lumis = luminanceValues;

  Array<unsigned int> source;
  source.resize(l->width*l->height);
  memcpy(&source[0],l->pixels,l->width*l->height*sizeof(unsigned int));

  initStatusBar();
  bool hasSelection = l->hasSelection();

  for (int y = 0; y < l->height; y++) {
    if ((y % 100)==0) UPDATESTATUSBARWITHBREAK(memcpy(l->pixels,&source[0],l->width*l->height*sizeof(unsigned int)));
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
  return true;
}

bool filterBayerDither() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;

  beforeUndoPoint();

  bool hasSelection = l->hasSelection();

  unsigned int bayer[4*4] = {
    0,8,2,10,
    12,4,14,6,
    3,11,1,9,
    15,7,13,5};

  float *lumis = luminanceValues;

  for (int y = 0; y < l->height; y++) {
    for (int x = 0; x < l->width; x++) {
      if ((!hasSelection)||(l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED)) {
        int thresh = bayer[(x % 4)+(y % 4)*4]*256/16;
        unsigned int col = l->pixels[x+y*l->width];
        int r = (int)(col & 255);
        int g = (int)((col>>8) & 255);
        int b = (int)((col>>16) & 255);
        int a = (int)((col>>24) & 255);
        float k = (float)r*lumis[0] + (float)g*lumis[1] + (float)b*lumis[2];
        int gr = k > thresh ? 255 : 0;
        l->pixels[x+y*l->width] = gr|(gr<<8)|(gr<<16)|(a<<24);
      }
    }
  }
  updatedPixels(l);
  afterUndoPoint();
  repaintLayer(l,true);
  return true;
}

static __inline void lookupInterpolated(unsigned int *colors, int width, int height, float x, float y, float &red, float &green, float &blue, float &alpha) {
  if (width<1||height<1) return;
  int lx = (int)floor(x);
  int ly = (int)floor(y);
  int lx2 = lx + 1;
  int ly2 = ly + 1;
  if (lx < 0) {lx = 0; x = 0;}
  if (ly < 0) {ly = 0; y = 0;}
  if (lx >= width) {lx = width-1; lx2 = lx; x = 0;}
  if (ly >= height) {ly = height-1; ly2 = ly; y = 0;}
  if (lx2 >= width-1) lx2 = width - 1;
  if (ly2 >= height-1) ly2 = height - 1;
  if (lx2 < 0) {lx2 = 0;}
  if (ly2 < 0) {ly2 = 0;}
  double fx = x-floor(x);
  double fy = y-floor(y);
  unsigned int p00 = colors[lx+ly*width];
  unsigned int p10 = colors[lx2+ly*width];
  unsigned int p01 = colors[lx+ly2*width];
  unsigned int p11 = colors[lx2+ly2*width];
  double rt = ((int)(p10 & 255) - (int)(p00 & 255))*fx + (int)(p00 & 255);
  double gt = ((int)((p10>>8) & 255) - (int)((p00>>8) & 255))*fx + (int)((p00>>8) & 255);
  double bt = ((int)((p10>>16) & 255) - (int)((p00>>16) & 255))*fx + (int)((p00>>16) & 255);
  double at = ((int)((p10>>24) & 255) - (int)((p00>>24) & 255))*fx + (int)((p00>>24) & 255);
  double rb = ((int)(p11 & 255) - (int)(p01 & 255))*fx + (int)(p01 & 255);
  double gb = ((int)((p11>>8) & 255) - (int)((p01>>8) & 255))*fx + (int)((p01>>8) & 255);
  double bb = ((int)((p11>>16) & 255) - (int)((p01>>16) & 255))*fx + (int)((p01>>16) & 255);
  double ab = ((int)((p11>>24) & 255) - (int)((p01>>24) & 255))*fx + (int)((p01>>24) & 255);
  red = (float)((rb - rt) * fy + rt);
  green = (float)((gb - gt) * fy + gt);
  blue = (float)((bb - bt) * fy + bt);
  alpha = (float)((ab - at) * fy + at);
}

bool filterDisplacementMap(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float displacement1, float displacement2, int type, unsigned char *pixelStates, bool usePixelStates, Layer *displace) {
  if (displace == NULL) return false;
  for (int y = 0; y < (int)height; y++) {
    for (int x = 0; x < (int)width; x++) {
      if ((!usePixelStates)||(pixelStates[x+y*width] & PIXELSTATE_SELECTED)) {
        float dx = (float)(x * displace->width)/width;
        float dy = (float)(y * displace->height)/height;
        float lx = 0, ly = 0;
        switch(type) {
          case 0: {
            float cr=0,cg=0,cb=0,ca=0; lookupInterpolated(displace->pixels, displace->width, displace->height, dx-1, dy, cr, cg, cb, ca);
            float dr=0,dg=0,db=0,da=0; lookupInterpolated(displace->pixels, displace->width, displace->height, dx, dy-1, dr, dg, db, da);
            float er=0,eg=0,eb=0,ea=0; lookupInterpolated(displace->pixels, displace->width, displace->height, dx+1, dy, er, eg, eb, ea);
            float fr=0,fg=0,fb=0,fa=0; lookupInterpolated(displace->pixels, displace->width, displace->height, dx, dy+1, fr, fg, fb, fa);
            lx = (float)(x + (er-cr) * displacement1 / 255.0);
            ly = (float)(y + (fg-dg) * displacement2 / 255.0);
          } break;
          case 1: {
            float cr=0,cg=0,cb=0,ca=0; lookupInterpolated(displace->pixels, displace->width, displace->height, dx, dy, cr, cg, cb, ca);
            lx = (float)(x + (cr-128) * displacement1/128.0);
            ly = (float)(y + (cg-128) * displacement2/128.0);
          } break;
          case 2: {
            float cr=0,cg=0,cb=0,ca=0; lookupInterpolated(displace->pixels, displace->width, displace->height, dx, dy, cr, cg, cb, ca);
            float xn = (float)(cr * width / 255.0);
            float yn = (float)(cg * height / 255.0);
            lx = (xn-x) * displacement1 + x;
            ly = (yn-y) * displacement2 + y;
          } break;
        }
        float lr=0,lg=0,lb=0,la=0;
        lookupInterpolated(source, width, height, lx, ly, lr, lg, lb, la);
        int r = (int)lr; int g = (int)lg; int b = (int)lb; int a = (int)la;
        if (r < 0) r = 0;
        if (g < 0) g = 0;
        if (b < 0) b = 0;
        if (a < 0) a = 0;
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        if (a > 255) a = 255;
        dest[x+y*width] = r|(g<<8)|(b<<16)|(a<<24);
      }
    }
  }
  return true;
}

class WatershedEntry {
public:
  WatershedEntry() : x(0), y(0), priority(0) {;}
  WatershedEntry(int x, int y, float priority) : x(x), y(y), priority(priority) {;}
  int x,y;
  float priority;
};

static __inline void putInPriorityQueue(Array<WatershedEntry> &queue, int x, int y, float priority) {
  int siz = (int)queue.size();
  WatershedEntry *queue2 = &queue[0];
  if (siz == 0) {
    queue.push_back(WatershedEntry(x,y,priority));
    return;
  }
  int last = siz-1;
  if (priority <= queue2[last].priority) {
    queue.push_back(WatershedEntry(x,y,priority));
    return;
  }
  if (priority >= queue2[0].priority) {
    queue.insert(WatershedEntry(x,y,priority),0);
    return;
  }
  int first = 0;
  
  while (1) {
    int middle = (last + first)/2;
    if (priority == queue2[middle].priority || (last == middle || first == middle)) {
      queue.insert(WatershedEntry(x,y,priority),middle);
      break;
    }
    if (priority > queue2[middle].priority) {
      last = middle;
    } else {
      first = middle;
    }
  }
}

static __inline void proceedWatershed(int x, int y, Array<WatershedEntry> &queue, float *gradient, unsigned int *labels, int dx, int dy, int width, int height) {
  int x0 = x; int y0 = y;
  int x1 = x0 + dx; int y1 = y0 + dy;
  if ((unsigned int)x0>=(unsigned int)width || (unsigned int)y0>=(unsigned int)height||(unsigned int)x1>=(unsigned int)width || (unsigned int)y1>=(unsigned int)height) return;
  unsigned int label0 = labels[x0+y0*width];
  unsigned int label1 = labels[x1+y1*width];
  if (label0 != 0 && label1 == 0) {
    labels[x1+y1*width] = label0;
    putInPriorityQueue(queue,x1,y1,gradient[x1+y1*width]);
  }
}

bool filterWatershed(class Layer *layer) { // actually does the segmentation

  Layer *l = layer;
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
   
  Array<unsigned int> blurred; blurred.resize(layer->width*layer->height);
  int kernel = 1; filterMedianBlur(&blurred[0], layer->pixels, layer->width, layer->height, kernel, kernel, layer->pixelStates, false, false, false);
  if (escapeWasPressed) return false;

  // calculate the gradient in values
  float *lumis = luminanceValues;
  Array<float> values; values.resize(l->width*l->height); int sobelXMat[3*3] = {+1,+0,-1, +2,+0,-2, +1,+0,-1}; int sobelYMat[3*3] = {+1,+2,+1, 0,0,0, -1,-2,-1};
  for (int y = 0; y < l->height; y++) {
    for (int x = 0; x < l->width; x++) {
      float sobelX[5] = {0,0,0,0,0};
      float sobelY[5] = {0,0,0,0,0};
      for (int y2 = -1; y2 <= 1; y2++) {
        for (int x2 = -1; x2 <= 1; x2++) {
          int x3 = x + x2;
          int y3 = y + y2;
          if ((unsigned int)x3<(unsigned int)l->width && (unsigned int)y3<(unsigned int)l->height) {
            unsigned int col = blurred[x3+y3*l->width];
            int r = col & 255;
            int g = (col>>8) & 255;
            int b = (col>>16) & 255;
            int a = (col>>24) & 255;
            double sl = r * lumis[0] + g * lumis[1] + b * lumis[2];
            sobelX[0] += r*sobelXMat[(x2+1)+(y2+1)*3]; sobelY[0] += r*sobelYMat[(x2+1)+(y2+1)*3];
            sobelX[1] += g*sobelXMat[(x2+1)+(y2+1)*3]; sobelY[1] += g*sobelYMat[(x2+1)+(y2+1)*3];
            sobelX[2] += b*sobelXMat[(x2+1)+(y2+1)*3]; sobelY[2] += b*sobelYMat[(x2+1)+(y2+1)*3];
            sobelX[3] += (float)(sl*sobelXMat[(x2+1)+(y2+1)*3]); sobelY[3] += (float)(sl*sobelYMat[(x2+1)+(y2+1)*3]);
            sobelX[4] += a*sobelXMat[(x2+1)+(y2+1)*3]; sobelY[4] += a*sobelYMat[(x2+1)+(y2+1)*3];
          }
        }
      }
      if (y == 0 || y == l->height-1) {sobelY[0] = sobelY[1] = sobelY[2] = sobelY[3] = sobelY[4] = 0;}
      if (x == 0 || x == l->width-1) {sobelX[0] = sobelX[1] = sobelX[2] = sobelX[3] = sobelX[4] = 0;}
      values[x+y*l->width] = (float)sqrt(sobelX[0]*sobelX[0]+sobelY[0]*sobelY[0]+sobelX[1]*sobelX[1]+sobelY[1]*sobelY[1]+sobelX[2]*sobelX[2]+sobelY[2]*sobelY[2]+sobelX[3]*sobelX[3]+sobelY[3]*sobelY[3]+sobelX[4]*sobelX[4]+sobelY[4]*sobelY[4]);
    }
  }
  float minV = 0; float maxV = 0;
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

  initStatusBar();

  Array<WatershedPoint> watershedPoints2; watershedPoints2 = watershedPoints;
  Array<WatershedEntry> priorityQueue;
  Array<unsigned int> labels; labels.resize(layer->width * layer->height); memset(&labels[0],0,sizeof(unsigned int)*labels.size());
  for (int i = 0; i < (int)watershedPoints2.size(); i++) {
    WatershedPoint *p = &watershedPoints2[i];
    double cx = p->x, cy = p->y; currentCanvas->getLayerPosFromCanvasPos(layer,cx,cy,cx,cy);
    int lx = (int)floor(cx); int ly = (int)floor(cy);
    if ((unsigned int)lx < (unsigned int)layer->width && (unsigned int)ly < (unsigned int)layer->height) {
      int index = lx+ly*layer->width;
      labels[index] = p->label;
      putInPriorityQueue(priorityQueue, lx, ly, values[index]);
    }
  }

  beforeUndoPoint();

  if (watershedSelectionInside || watershedSelectionOutside) {
    WatershedPoint q;
    q.selection = watershedSelectionInside;
    q.label = 0x1234561;
    watershedPoints2.push_back(q);
    for (int i4 = 0; i4 < l->width*l->height; i4++) {
      unsigned char state = l->pixelStates[i4];
      if (state & PIXELSTATE_SELECTED) {
        labels[i4] = q.label;
      }
      l->pixelStates[i4] &= ~PIXELSTATE_SELECTED;
    }
    for (int y = 0; y < l->height; y++) {
      if ((y % 50)==0) UPDATESTATUSBARWITHBREAK(revertToBeforeUndoPoint());
      for (int x = 0; x < l->width; x++) {
        unsigned int b = labels[x+y*l->width];
        if (b != 0) {
          bool hasNeighbor = false;
          for (int y2 = -1; y2 <= 1; y2++) { int y3 = y + y2;
            for (int x2 = -1; x2 <= 1; x2++) { int x3 = x + x2;
              if ((unsigned int)x3 < (unsigned int)l->width && (unsigned int)y3 < (unsigned int)l->height) {
                unsigned int a = labels[x3+y3*l->width];
                if (a == 0) {hasNeighbor = true; break;}
              }
            }
            if (hasNeighbor) break;
          }
          if (hasNeighbor) {
            putInPriorityQueue(priorityQueue, x, y, values[x+y*l->width]);
          }
        }
      }
    }
  }

  initStatusBar();

  int count = 0;
  float *values0 = &values[0];
  unsigned int *labels0 = &labels[0]; int lw = l->width; int lh = l->height;
  int portion = l->width * l->height / 15; if (portion < 1) portion = 1;
  while(!priorityQueue.empty()) {
    if ((count % portion)==0) UPDATESTATUSBARWITHBREAK(revertToBeforeUndoPoint());
    WatershedEntry here = priorityQueue.back(); priorityQueue.pop_back();
    int x = here.x; int y = here.y;
    proceedWatershed(x,y, priorityQueue, values0, labels0, 1, 0, lw, lh);
    proceedWatershed(x,y, priorityQueue, values0, labels0, -1, 0, lw, lh);
    proceedWatershed(x,y, priorityQueue, values0, labels0, 0, 1, lw, lh);
    proceedWatershed(x,y, priorityQueue, values0, labels0, 0, -1, lw, lh);

    proceedWatershed(x,y, priorityQueue, values0, labels0, 1, 1, lw, lh);
    proceedWatershed(x,y, priorityQueue, values0, labels0, -1, 1, lw, lh);
    proceedWatershed(x,y, priorityQueue, values0, labels0, 1, -1, lw, lh);
    proceedWatershed(x,y, priorityQueue, values0, labels0, -1, -1, lw, lh);
    count++;   
  }

  for (int i3 = 0; i3 < l->width*l->height; i3++) {
    unsigned int label = labels[i3];
    for (int i = 0; i < (int)watershedPoints2.size(); i++) {
      WatershedPoint *p = &watershedPoints2[i];
      if (p->label == label) {
        if (p->selection) l->pixelStates[i3] |= PIXELSTATE_SELECTED;
        break;
      }
    }
  }

  repaintLayer(l,true);
  updatedPixelStates(l);
  afterUndoPoint();
  return true;
}

bool filterSubtract(unsigned int *dest, unsigned int *minuend, unsigned int *subtrahend, unsigned int width, unsigned int height) {
  for (int i = 0; i < (int)(width * height); i++) {
    unsigned int col0 = minuend[i];
    unsigned int col1 = subtrahend[i];
    int r = (int)(col0 & 255) - (int)(col1 & 255);
    int g = (int)((col0>>8) & 255) - (int)((col1>>8) & 255);
    int b = (int)((col0>>16) & 255) - (int)((col1>>16) & 255);
    int a = (int)((col0>>24) & 255) - (int)((col1>>24) & 255);
    r += 128;
    g += 128;
    b += 128;
    a += 128;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (a < 0) a = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    if (a > 255) a = 255;
    dest[i] = r|(g<<8)|(b<<16)|(a<<24);
  }
  return true;
}

bool filterSetAlpha(unsigned int *dest, unsigned int width, unsigned int height, int alpha) {
  if (alpha < 0) alpha = 0;
  if (alpha > 255) alpha = 255;
  for (int i = 0; i < (int)(width * height); i++) {
    dest[i] = (dest[i] & 0x00ffffff)|(alpha<<24);
  }
  return true;
}

bool filterSelectionDistanceField(Layer *layer) {
  Layer *l = layer;
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
  if (!l->hasSelection()) return false;

  beforeUndoPoint();

  initStatusBar();

  Array<unsigned int> astar; astar.resize(layer->width*layer->height); memset(&astar[0],0xff,astar.size() * sizeof(unsigned int));
  unsigned int *astar_ = &astar[0];

  for (int y = 0; y < layer->height; y++) {
    for (int x = 0; x < layer->width; x++) {
      unsigned char here = layer->pixelStates[x+y*layer->width];
      if (here & PIXELSTATE_SELECTED) {
        bool found = false;
        for (int y2 = -1; y2 <= 1; y2++) {
          for (int x2 = -1; x2 <= 1; x2++) {
            if ((unsigned int)(x+x2) >= (unsigned int)layer->width || (unsigned int)(y+y2) >= (unsigned int)layer->height) {found = true; break;}
            unsigned char there = layer->pixelStates[(x+x2)+(y+y2)*layer->width];
            if (!(there & PIXELSTATE_SELECTED)) {found = true; break;}
          }
        }
        if (found) {
          astar_[x+y*layer->width] = (x & 0xffff) + ((y & 0xffff)<<16);
        }
      }
    }
  }

  bool somethingUpdated = true;
  unsigned int counter = 0;
  int maxDist = 0;
  while(somethingUpdated) {
    somethingUpdated = false;
    counter++; counter &= 15; if (counter == 0) UPDATESTATUSBARWITHBREAK(;);
    maxDist = 0;
    for (int y = 0; y < layer->height; y++) {
      for (int x = 0; x < layer->width; x++) {
        unsigned int here = astar_[x+y*layer->width];
        if (here != 0xffffffff) {
          int dx0 = (int)(here & 0xffff) - x;
          int dy0 = (int)((here>>16) & 0xffff) - y;
          int d0 = dx0*dx0 + dy0*dy0;
          if (d0 > maxDist) maxDist = d0;
          for (int y2 = -1; y2 <= 1; y2++) {
            for (int x2 = -1; x2 <= 1; x2++) {
              if ((unsigned int)(x+x2) >= (unsigned int)layer->width || (unsigned int)(y+y2) >= (unsigned int)layer->height) continue;
              unsigned char therep = layer->pixelStates[(x+x2)+(y+y2)*layer->width];
              if (therep & PIXELSTATE_SELECTED) {
                unsigned int therea = astar_[(x+x2)+(y+y2)*layer->width];
                if (therea == 0xffffffff) {
                  astar_[(x+x2)+(y+y2)*layer->width] = here;
                  somethingUpdated = true;
                } else {
                  int dx1 = (int)(therea & 0xffff) - (x+x2);
                  int dy1 = (int)((therea>>16) & 0xffff) - (y+y2);
                  int d1 = dx1*dx1 + dy1*dy1;
                  int dx2 = (int)(here & 0xffff) - (x+x2);
                  int dy2 = (int)((here>>16) & 0xffff) - (y+y2);
                  int d2 = dx2*dx2 + dy2*dy2;
                  if (d2 < d1) {
                    astar_[(x+x2)+(y+y2)*layer->width] = here;
                    somethingUpdated = true;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  double md = sqrt(maxDist); if (md == 0) md = 1;
  for (int y2 = 0; y2 < layer->height; y2++) {
    for (int x2 = 0; x2 < layer->width; x2++) {
      unsigned char here = layer->pixelStates[x2+y2*layer->width];
      if (here & PIXELSTATE_SELECTED) {
        int dx = (int)(astar_[x2+y2*layer->width] & 0xffff)-x2;
        int dy = (int)((astar_[x2+y2*layer->width]>>16) & 0xffff)-y2;
        double d = sqrt(dx*dx+dy*dy)/md;
        int grey = (int)(d * 255);
        if (grey < 0) grey = 0;
        if (grey > 255) grey = 255;
        layer->pixels[x2+y2*layer->width] = grey|(grey<<8)|(grey<<16)|0xff000000;
      }
    }
  }

  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
  return true;
}

bool filterSelectionAverageColor(Layer *layer) {
  Layer *l = layer;
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
  if (!l->hasSelection()) return false;

  beforeUndoPoint();

  double r = 0, g = 0, b = 0, a = 0, w = 0;
  for (int y2 = 0; y2 < layer->height; y2++) {
    for (int x2 = 0; x2 < layer->width; x2++) {
      unsigned char here = layer->pixelStates[x2+y2*layer->width];
      if (here & PIXELSTATE_SELECTED) {
        unsigned int col = layer->pixels[x2+y2*layer->width];
        r += (col) & 255;
        g += (col>>8) & 255;
        b += (col>>16) & 255;
        a += (col>>24) & 255;
        w += 1;
      }
    }
  }
  if (w > 0) {
    r /= w;
    g /= w;
    b /= w;
    a /= w;
    int ri = (int)r;
    int gi = (int)g;
    int bi = (int)b;
    int ai = (int)a;
    unsigned int col = ri|(gi<<8)|(bi<<16)|(ai<<24);
    for (int y2 = 0; y2 < layer->height; y2++) {
      for (int x2 = 0; x2 < layer->width; x2++) {
        unsigned char here = layer->pixelStates[x2+y2*layer->width];
        if (here & PIXELSTATE_SELECTED) {
          layer->pixels[x2+y2*layer->width] = col;
        }
      }
    }
  }

  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
  return true;
}

bool filterPixelate(int cellSizeX, int cellSizeY) {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
  if (cellSizeX < 1 || cellSizeY < 1) return false;

  beforeUndoPoint();
  int countX = l->width / cellSizeX + 1;
  int countY = l->height / cellSizeY + 1;
  Array<Vector> collected; collected.resize(countX*countY);
  Array<unsigned int> collectedW; collectedW.resize(countX*countY); collectedW.fill(0);

  bool hasSelection = l->hasSelection();

  {for (int y = 0; y < l->height; y++) {
    int dy = y / cellSizeY;
    for (int x = 0; x < l->width; x++) {
      int dx = x / cellSizeX;
      unsigned int col = l->pixels[x+y*l->width];
      Vector &k = collected[dx+dy*countX];
      k.x += col & 255;
      k.y += (col>>8) & 255;
      k.z += (col>>16) & 255;
      k.w += (col>>24) & 255;
      collectedW[dx+dy*countX]++;
    }
  }}


  {for (int y = 0; y < l->height; y++) {
    int dy = y / cellSizeY;
    for (int x = 0; x < l->width; x++) {
      if ((!hasSelection)||(l->pixelStates[x+y*l->width] & PIXELSTATE_SELECTED)) {
        int dx = x / cellSizeX;
        Vector &k = collected[dx+dy*countX];
        double w = collectedW[dx+dy*countX]; if (w < 1) w = 1;
        int r = (int)(k.x / w);
        int g = (int)(k.y / w);
        int b = (int)(k.z / w);
        int a = (int)(k.w / w);
        l->pixels[x+y*l->width] = r|(g<<8)|(b<<16)|(a<<24);
      }
    }
  }}

  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
  return true;
}

bool filterSelectionVerticalGradient(Layer *layer) {
  Layer *l = layer;
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
  if (!l->hasSelection()) return false;

  beforeUndoPoint();

  for (int x2 = 0; x2 < layer->width; x2++) {
    int gradientStart = -1;
    for (int y2 = 0; y2 < layer->height+1; y2++) {
      unsigned char here = (unsigned char)(y2 < layer->height ? layer->pixelStates[x2+y2*layer->width] : 0);
      if (here & PIXELSTATE_SELECTED) {
        if (gradientStart < 0) gradientStart = y2;
      } else {
        if (gradientStart >= 0) {
          int delta = y2 - gradientStart;
          if (delta > 0) {
            if (delta > 1) delta--;
            for (int y3 = gradientStart; y3 < y2; y3++) {
              int grey = (y3 - gradientStart)*255/delta;
              if (y3 < layer->height) layer->pixels[x2+y3*layer->width] = grey|(grey<<8)|(grey<<16)|0xff000000;
            }
          }
          gradientStart = -1;
        }
      }
    }
  }

  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
  return true;
}


bool filterFillAlphaHoles(class Layer *layer, bool forAlpha) {
  Layer *l = layer;
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;

  bool hasSelection = l->hasSelection(); if ((!forAlpha) && (!hasSelection)) return false;

  initStatusBar();
  beforeUndoPoint();

  Array<float> p2; p2.resize(l->width*l->height*5); p2.fill(0);

  for (int x2 = 0; x2 < l->width; x2++) {
    int gradientStart = -1;
    for (int y2 = 0; y2 < l->height+1; y2++) {
      bool hole = y2 < l->height ? ((l->pixels[x2+y2*l->width]>>24) & 255) != 255 : false;
      if (!forAlpha) hole = y2 < l->height ? (l->pixelStates[x2+y2*l->width] & PIXELSTATE_SELECTED ? true : false) : false;
      if (hole) {
        if (gradientStart < 0) gradientStart = y2;
      } else {
        if (gradientStart >= 0) {
          int delta = y2 - gradientStart;
          if (delta > 0) {
            int y0 = gradientStart - 1; if (y0 < 0) y0 = 0;
            int y1 = y2; if (y1 >= l->height) y1 = l->height - 1;
            unsigned int col0 = l->pixels[x2+y0 * l->width];
            unsigned int col1 = l->pixels[x2+y1 * l->width];
            for (int y3 = gradientStart; y3 < y2; y3++) {
              float k = (float)(y3 - gradientStart)/delta;
              float r = ((float)((int)(col1 & 255) - (int)(col0 & 255))*k + (float)(col0 & 255));
              float g = ((float)((int)((col1>>8) & 255) - (int)((col0>>8) & 255))*k + (float)((col0>>8) & 255));
              float b = ((float)((int)((col1>>16) & 255) - (int)((col0>>16) & 255))*k + (float)((col0>>16) & 255));
              float a = ((float)((int)((col1>>24) & 255) - (int)((col0>>24) & 255))*k + (float)((col0>>24) & 255));
              if (y3 < layer->height && ((!hasSelection)||(l->pixelStates[x2+y3*layer->width] & PIXELSTATE_SELECTED))) {
                p2[(x2+y3*layer->width)*5+0] += r;
                p2[(x2+y3*layer->width)*5+1] += g;
                p2[(x2+y3*layer->width)*5+2] += b;
                p2[(x2+y3*layer->width)*5+3] += a;
                p2[(x2+y3*layer->width)*5+4] += 1;
              }
            }
          }
          gradientStart = -1;
        }
      }
    }
  }

  for (int y2 = 0; y2 < l->height; y2++) {
    int gradientStart = -1;
    for (int x2 = 0; x2 < l->width+1; x2++) {
      bool hole = x2 < l->width ? ((l->pixels[x2+y2*l->width]>>24) & 255) != 255 : false;
      if (!forAlpha) hole = x2 < l->width ? (l->pixelStates[x2+y2*l->width] & PIXELSTATE_SELECTED ? true : false) : false;
      if (hole) {
        if (gradientStart < 0) gradientStart = x2;
      } else {
        if (gradientStart >= 0) {
          int delta = x2 - gradientStart;
          if (delta > 0) {
            int x0 = gradientStart - 1; if (x0 < 0) x0 = 0;
            int x1 = x2; if (x1 >= l->width) x1 = l->width - 1;
            unsigned int col0 = l->pixels[x0+y2 * l->width];
            unsigned int col1 = l->pixels[x1+y2 * l->width];
            for (int x3 = gradientStart; x3 < x2; x3++) {
              float k = (float)(x3 - gradientStart)/delta;
              float r = ((float)((int)(col1 & 255) - (int)(col0 & 255))*k + (float)(col0 & 255));
              float g = ((float)((int)((col1>>8) & 255) - (int)((col0>>8) & 255))*k + (float)((col0>>8) & 255));
              float b = ((float)((int)((col1>>16) & 255) - (int)((col0>>16) & 255))*k + (float)((col0>>16) & 255));
              float a = ((float)((int)((col1>>24) & 255) - (int)((col0>>24) & 255))*k + (float)((col0>>24) & 255));
              if (x3 < layer->width && ((!hasSelection)||(l->pixelStates[x3+y2*layer->width] & PIXELSTATE_SELECTED))) {
                p2[(x3+y2*layer->width)*5+0] += r;
                p2[(x3+y2*layer->width)*5+1] += g;
                p2[(x3+y2*layer->width)*5+2] += b;
                p2[(x3+y2*layer->width)*5+3] += a;
                p2[(x3+y2*layer->width)*5+4] += 1;
              }
            }
          }
          gradientStart = -1;
        }
      }
    }
  }

  {for (int k2 = 0; k2 < l->width + l->height; k2++) {
    int xp = k2 < l->width ? k2 : 0;
    int yp = k2 < l->width ? 0 : k2 - l->width;
    int gradientStart = -1;
    for (int k3 = 0; k3 < l->width + l->height; k3++) {
      int xh = xp + k3;
      int yh = yp + k3;
      bool hole = (unsigned int)xh < (unsigned int)l->width && (unsigned int)yh < (unsigned int)l->height ? ((l->pixels[xh+yh*l->width]>>24) & 255) != 255 : false;
      if (!forAlpha) hole = (unsigned int)xh < (unsigned int)l->width && (unsigned int)yh < (unsigned int)l->height ? (l->pixelStates[xh+yh*l->width] & PIXELSTATE_SELECTED ? true : false) : false;
      if (hole) {
        if (gradientStart < 0) gradientStart = k3;
      } else {
        if (gradientStart >= 0) {
          int delta = k3 - gradientStart;
          if (delta > 0) {
            int xh0 = xp + gradientStart-1; 
            int yh0 = yp + gradientStart-1; 
            int xh1 = xp + k3;
            int yh1 = yp + k3; 
            if (xh0 >= l->width) xh0 = l->width - 1;
            if (yh0 >= l->height) yh0 = l->height - 1;
            if (xh1 >= l->width) xh1 = l->width - 1;
            if (yh1 >= l->height) yh1 = l->height - 1;
            if (xh0 < 0) xh0 = 0;
            if (yh0 < 0) yh0 = 0;
            if (xh1 < 0) xh1 = 0;
            if (yh1 < 0) yh1 = 0;
            unsigned int col0 = l->pixels[xh0+yh0 * l->width];
            unsigned int col1 = l->pixels[xh1+yh1 * l->width];
            for (int i = gradientStart; i < k3; i++) {
              float k = (float)(i - gradientStart)/delta;
              float r = ((float)((int)(col1 & 255) - (int)(col0 & 255))*k + (float)(col0 & 255));
              float g = ((float)((int)((col1>>8) & 255) - (int)((col0>>8) & 255))*k + (float)((col0>>8) & 255));
              float b = ((float)((int)((col1>>16) & 255) - (int)((col0>>16) & 255))*k + (float)((col0>>16) & 255));
              float a = ((float)((int)((col1>>24) & 255) - (int)((col0>>24) & 255))*k + (float)((col0>>24) & 255));
              int xh2 = xp + i;
              int yh2 = yp + i;
              if ((unsigned int)xh2 < (unsigned int)layer->width && (unsigned int)yh2 < (unsigned int)layer->height && ((!hasSelection)||(l->pixelStates[xh2+yh2*layer->width] & PIXELSTATE_SELECTED))) {
                p2[(xh2+yh2*layer->width)*5+0] += r;
                p2[(xh2+yh2*layer->width)*5+1] += g;
                p2[(xh2+yh2*layer->width)*5+2] += b;
                p2[(xh2+yh2*layer->width)*5+3] += a;
                p2[(xh2+yh2*layer->width)*5+4] += 1;
              }
            }
          }
          gradientStart = -1;
        }
      }
    }
  }}

  {for (int k2 = 0; k2 < l->width + l->height; k2++) {
    int xp = k2 < l->width ? k2 : 0;
    int yp = k2 < l->width ? l->height - 1 : k2 - l->width;
    int gradientStart = -1;
    for (int k3 = 0; k3 < l->width + l->height; k3++) {
      int xh = xp + k3;
      int yh = yp - k3;
      bool hole = (unsigned int)xh < (unsigned int)l->width && (unsigned int)yh < (unsigned int)l->height ? ((l->pixels[xh+yh*l->width]>>24) & 255) != 255 : false;
      if (!forAlpha) hole = (unsigned int)xh < (unsigned int)l->width && (unsigned int)yh < (unsigned int)l->height ? (l->pixelStates[xh+yh*l->width] & PIXELSTATE_SELECTED ? true : false) : false;
      if (hole) {
        if (gradientStart < 0) gradientStart = k3;
      } else {
        if (gradientStart >= 0) {
          int delta = k3 - gradientStart;
          if (delta > 0) {
            int xh0 = xp + (gradientStart-1); 
            int yh0 = yp - (gradientStart-1); 
            int xh1 = xp + k3;
            int yh1 = yp - k3; 
            if (xh0 >= l->width) xh0 = l->width - 1;
            if (yh0 >= l->height) yh0 = l->height - 1;
            if (xh1 >= l->width) xh1 = l->width - 1;
            if (yh1 >= l->height) yh1 = l->height - 1;
            if (xh0 < 0) xh0 = 0;
            if (yh0 < 0) yh0 = 0;
            if (xh1 < 0) xh1 = 0;
            if (yh1 < 0) yh1 = 0;
            unsigned int col0 = l->pixels[xh0+yh0 * l->width];
            unsigned int col1 = l->pixels[xh1+yh1 * l->width];
            for (int i = gradientStart; i < k3; i++) {
              float k = (float)(i - gradientStart)/delta;
              float r = ((float)((int)(col1 & 255) - (int)(col0 & 255))*k + (float)(col0 & 255));
              float g = ((float)((int)((col1>>8) & 255) - (int)((col0>>8) & 255))*k + (float)((col0>>8) & 255));
              float b = ((float)((int)((col1>>16) & 255) - (int)((col0>>16) & 255))*k + (float)((col0>>16) & 255));
              float a = ((float)((int)((col1>>24) & 255) - (int)((col0>>24) & 255))*k + (float)((col0>>24) & 255));
              int xh2 = xp + i;
              int yh2 = yp - i;
              if ((unsigned int)xh2 < (unsigned int)layer->width && (unsigned int)yh2 < (unsigned int)layer->height && ((!hasSelection)||(l->pixelStates[xh2+yh2*layer->width] & PIXELSTATE_SELECTED))) {
                p2[(xh2+yh2*layer->width)*5+0] += r;
                p2[(xh2+yh2*layer->width)*5+1] += g;
                p2[(xh2+yh2*layer->width)*5+2] += b;
                p2[(xh2+yh2*layer->width)*5+3] += a;
                p2[(xh2+yh2*layer->width)*5+4] += 1;
              }
            }
          }
          gradientStart = -1;
        }
      }
    }
  }}

  for (int i = 0; i < layer->width * layer->height; i++) {
    float r = p2[i*5+0];
    float g = p2[i*5+1];
    float b = p2[i*5+2];
    float a = p2[i*5+3];
    float w = p2[i*5+4];
    if (w > 0) {
      int ri = (int)(r/w);
      int gi = (int)(g/w);
      int bi = (int)(b/w);
      int ai = (int)(a/w);
      if (forAlpha) ai = 0xff;
      layer->pixels[i] = (ri)|(gi<<8)|(bi<<16)|(ai<<24);
    }
  }

  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
  return true;
}

bool filterTurnToShadow() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return false;
  if (locked(l,LOCK_ANY)) return false;
  beforeUndoPoint();
  for (int i = 0; i < l->width*l->height; i++) l->pixels[i] = l->pixels[i] & 0xff000000;
  repaintLayer(l,true);
  updatedPixels(l);
  afterUndoPoint();
  return true;
}

bool filterOutline(unsigned int *dest, unsigned int *source, unsigned int width, unsigned int height, float outlineSize, bool antialiasing) {

  initStatusBar();
  memset(dest,0,width*height*sizeof(unsigned int));

  Array<unsigned int> astar; astar.resize(width*height); memset(&astar[0],0xff,astar.size() * sizeof(unsigned int));
  unsigned int *astar_ = &astar[0];

  int alphaThresh = 1;
  outlineSize += 1.f;

  for (int y = 0; y < (int)height; y++) {
    for (int x = 0; x < (int)width; x++) {
      unsigned char alphaHere = (unsigned char)((source[x+y*width]>>24) & 255);
      if (alphaHere < alphaThresh) {
        bool found = false;
        int foundX = 0; int foundY = 0;
        for (int y2 = -1; y2 <= 1; y2++) {
          for (int x2 = -1; x2 <= 1; x2++) {
            if ((unsigned int)(x+x2) >= (unsigned int)width || (unsigned int)(y+y2) >= (unsigned int)height) {break;}
            unsigned char alphaThere = (unsigned char)((source[(x+x2)+(y+y2)*width]>>24) & 255);
            if (alphaThere>=alphaThresh) {found = true; foundX = x + x2; foundY = y + y2; break;}
          }
        }
        if (found) {
          astar_[x+y*width] = (foundX & 0xffff) + ((foundY & 0xffff)<<16);
        }
      }
    }
  }

  bool somethingUpdated = true;
  unsigned int counter = 0;
  int outlineSizeSqr = (int)((outlineSize+1)*(outlineSize+1));
  while(somethingUpdated) {
    somethingUpdated = false;
    counter++; counter &= 15; if (counter == 0) UPDATESTATUSBARWITHBREAK(;);
    for (int y = 0; y < (int)height; y++) {
      for (int x = 0; x < (int)width; x++) {
        unsigned int here = astar_[x+y*width];
        if (here != 0xffffffff) {
          int dx0 = (int)(here & 0xffff) - x;
          int dy0 = (int)((here>>16) & 0xffff) - y;
          int d0 = dx0*dx0 + dy0*dy0;
          if (d0 > outlineSizeSqr) continue;
          for (int y2 = -1; y2 <= 1; y2++) {
            for (int x2 = -1; x2 <= 1; x2++) {
              if ((unsigned int)(x+x2) >= (unsigned int)width || (unsigned int)(y+y2) >= (unsigned int)height) continue;
              unsigned char thereAlpha = (unsigned char)((source[(x+x2)+(y+y2)*width]>>24)&255);
              if (thereAlpha<alphaThresh) {
                unsigned int therea = astar_[(x+x2)+(y+y2)*width];
                if (therea == 0xffffffff) {
                  astar_[(x+x2)+(y+y2)*width] = here;
                  somethingUpdated = true;
                } else {
                  int dx1 = (int)(therea & 0xffff) - (x+x2);
                  int dy1 = (int)((therea>>16) & 0xffff) - (y+y2);
                  int d1 = dx1*dx1 + dy1*dy1;
                  int dx2 = (int)(here & 0xffff) - (x+x2);
                  int dy2 = (int)((here>>16) & 0xffff) - (y+y2);
                  int d2 = dx2*dx2 + dy2*dy2;
                  if (d2 < d1) {
                    astar_[(x+x2)+(y+y2)*width] = here;
                    somethingUpdated = true;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  for (int y2 = 0; y2 < (int)height; y2++) {
    for (int x2 = 0; x2 < (int)width; x2++) {
      unsigned char hereAlpha = (unsigned char)((source[x2+y2*width]>>24) & 255);
      if (hereAlpha < alphaThresh) {
        unsigned int astr = astar_[x2+y2*width];
        if (astr == 0xffffffff) {dest[x2+y2*width] = 0x00000000; continue;}
        int dx = (int)(astr & 0xffff)-x2;
        int dy = (int)((astr>>16) & 0xffff)-y2;
        double d = sqrt(dx*dx+dy*dy);
        int alpha = d >= outlineSize ? 0 : 255;
        if (antialiasing) {
          if (d < outlineSize - 1) alpha = 255; else {
            if (d >= outlineSize) {
              alpha = 0;
            } else {
              double k = d - (outlineSize-1);
              if (k < 0) k = 0;
              if (k > 1) k = 1;
              alpha = (int)((1-k) * 255.0);
            }
          }
        }
        int grey = alpha; // this is just for premul
        dest[x2+y2*width] = grey|(grey<<8)|(grey<<16)|(alpha<<24);
      } else dest[x2+y2*width] = 0xffffffff;
    }
  }
  return true;
}

class HalfToneMap {
public:
  HalfToneMap() : angle(0), cellCount(0), cells(NULL), pixelsToCells(NULL), cellToCellCenterX(NULL), cellToCellCenterY(NULL) {;}
  ~HalfToneMap() {
    if (cells != NULL) {delete[] cells; cells = NULL;}
    if (pixelsToCells != NULL) {delete[] pixelsToCells; pixelsToCells = NULL;}
    if (cellToCellCenterX != NULL) {delete[] cellToCellCenterX; cellToCellCenterX = NULL;}
    if (cellToCellCenterY != NULL) {delete[] cellToCellCenterY; cellToCellCenterY = NULL;}
  }
  float angle;
  int cellCount;
  float *cells;
  int *pixelsToCells;
  float *cellToCellCenterX;
  float *cellToCellCenterY;
};

enum HALFTONEMAPTYPE {
  HALFTONE_GREY = 0,
  HALFTONE2_C = 1,
  HALFTONE2_M = 2,
  HALFTONE2_Y = 3,
  HALFTONE2_K = 4,
  HALFTONE_R = 5,
  HALFTONE_G = 6,
  HALFTONE_B = 7,
  HALFTONE3_R = 8,
  HALFTONE3_Y = 9,
  HALFTONE3_B = 10,
};

static unsigned int *hf_xy = NULL;
static unsigned int *hf_sortedPixels = NULL;
static Layer *hf_layer = NULL;
static int hfSort(const void *p, const void *q)
{
  const unsigned int a = *((const unsigned int*)p);
  const unsigned int b = *((const unsigned int*)q);
  int ax = a & 0xffff; int ay = (a>>16) & 0xffff;
  int bx = b & 0xffff; int by = (b>>16) & 0xffff;
  const unsigned int anew = hf_xy[ax+ay*hf_layer->width];
  const unsigned int bnew = hf_xy[bx+by*hf_layer->width];
  if (anew > bnew) return 1;
  if (anew < bnew) return -1;
  return 0;
}

HalfToneMap *getHalftoneMap(Layer *l, int type, float angle, float cellSize) {

  updateStatusBar(); // no need for a break

  HalfToneMap *ret = new HalfToneMap();
  ret->angle = angle;

  hf_xy = new unsigned int[l->width*l->height];
  hf_sortedPixels = new unsigned int[l->width*l->height];
  hf_layer = l;

  float *hf_pixelCenterX = new float[l->width*l->height];
  float *hf_pixelCenterY = new float[l->width*l->height];

  double xcosa = cos(angle * PI / 180.0);
  double xsina = sin(angle * PI / 180.0);
  double xcosa_ = cos(-angle * PI / 180.0);
  double xsina_ = sin(-angle * PI / 180.0);

  for (int y = 0; y < l->height; y++) {
    for (int x = 0; x < l->width; x++) {
      double x2 = x; double y2 = y;
      x2 /= (double)(cellSize == 0 ? 1 : cellSize);
      y2 /= (double)(cellSize == 0 ? 1 : cellSize);
      double xf = x2 * xcosa - y2 * xsina;
      double yf = x2 * xsina + y2 * xcosa;
      int xi = (int)xf; FIXNEG(xi,xf);
      int yi = (int)yf; FIXNEG(yi,yf);
      hf_xy[x+y*l->width] = (unsigned int)(xi & 0xffff) | (((unsigned int)(yi  & 0xffff))<<16); // where to put(accumulate) the pixels to (which cell)
      hf_sortedPixels[x+y*l->width] = (unsigned int)(x & 0xffff) | (((unsigned int)(y  & 0xffff))<<16); // original pixels

      double x_ = (double)xi + 0.5;
      double y_ = (double)yi + 0.5;
      double xf_ = x_ * xcosa_ - y_ * xsina_;
      double yf_ = x_ * xsina_ + y_ * xcosa_;
      hf_pixelCenterX[x+y*l->width] = (float)xf_ * cellSize;
      hf_pixelCenterY[x+y*l->width] = (float)yf_ * cellSize;
    }
  }
  qsort(&hf_sortedPixels[0], l->width * l->height, sizeof(unsigned int), hfSort);

  updateStatusBar(); // no need for a break

  ret->pixelsToCells = new int[l->width*l->height];
  int cellIndex = 0;
  unsigned int cellValue = 0;
  for (int i = 0; i < l->width * l->height; i++) {
    unsigned int x = hf_sortedPixels[i] & 0xffff;
    unsigned int y = (hf_sortedPixels[i]>>16) & 0xffff;
    unsigned int v = hf_xy[x+y*l->width];
    if (cellValue != v || i == 0) {
      cellValue = v;
      cellIndex++;
    }
    ret->pixelsToCells[x+y*l->width] = cellIndex;
  }
  cellIndex++;

  delete[] hf_xy;
  delete[] hf_sortedPixels;

  ret->cellCount = cellIndex;
  ret->cells = new float[ret->cellCount*2]; memset(ret->cells, 0, sizeof(float)*ret->cellCount*2);
  ret->cellToCellCenterX = new float[ret->cellCount]; memset(ret->cellToCellCenterX, 0, sizeof(float)*ret->cellCount);
  ret->cellToCellCenterY = new float[ret->cellCount]; memset(ret->cellToCellCenterY, 0, sizeof(float)*ret->cellCount);
  float lumas[3] = {luminanceValues[0], luminanceValues[1], luminanceValues[2]};
  for (int i2 = 0; i2 < l->width * l->height; i2++) {
    unsigned int col = l->pixels[i2];
    float r = (float)(col & 255);
    float g = (float)((col>>8) & 255);
    float b = (float)((col>>16) & 255);
    float a = 0;
    float v = 0;
    switch(type) {
    case HALFTONE_GREY: {v = r*lumas[0] + g*lumas[1] + b*lumas[2];} break;
    case HALFTONE_R: {v = r;} break;
    case HALFTONE_G: {v = g;} break;
    case HALFTONE_B: {v = b;} break;
    case HALFTONE3_R: {toRYB_(r,g,b); v = r;} break;
    case HALFTONE3_Y: {toRYB_(r,g,b); v = g;} break;
    case HALFTONE3_B: {toRYB_(r,g,b); v = b;} break;
    case HALFTONE2_C: {toCMYK_(r,g,b,a); v = 255-r;} break;
    case HALFTONE2_M: {toCMYK_(r,g,b,a); v = 255-g;} break;
    case HALFTONE2_Y: {toCMYK_(r,g,b,a); v = 255-b;} break;
    case HALFTONE2_K: {toCMYK_(r,g,b,a); v = 255-a;} break;
    }
    ret->cells[ret->pixelsToCells[i2]*2 + 0] += v;
    ret->cells[ret->pixelsToCells[i2]*2 + 1] += 1.f;
    ret->cellToCellCenterX[ret->pixelsToCells[i2]] = hf_pixelCenterX[i2];
    ret->cellToCellCenterY[ret->pixelsToCells[i2]] = hf_pixelCenterY[i2];
    //ret->cellToCellCenterX[ret->pixelsToCells[i2]] += i2 % l->width;
    //ret->cellToCellCenterY[ret->pixelsToCells[i2]] += (int)i2 / (int)l->width;
  }

  delete[] hf_pixelCenterX;
  delete[] hf_pixelCenterY;

  updateStatusBar(); // no need for a break

  for (int i3 = 0; i3 < ret->cellCount; i3++) {
    float val = ret->cells[i3*2+0]; float count = ret->cells[i3*2+1];
    if (count == 0) {count = 1;}
    ret->cells[i3] = val / count; // gets into single value from value pair here
    //ret->cellToCellCenterX[i3] /= count;
    //ret->cellToCellCenterY[i3] /= count;
  }

  return ret;
}

bool filterHalftone(Layer *layer, float cellSize, int type, float dotScale) {
  if (layer == NULL) return false;
  if (locked(layer,LOCK_ANY)) return false;
  beforeUndoPoint();

  initStatusBar();

  Array<HalfToneMap *> toneMaps;
  switch(type) {
  case 0: {
    toneMaps.push_back(getHalftoneMap(layer,HALFTONE_GREY,45.f,cellSize));
    } break;
  case 1: {
    toneMaps.push_back(getHalftoneMap(layer,HALFTONE2_C,15.f,cellSize));
    toneMaps.push_back(getHalftoneMap(layer,HALFTONE2_M,75.f,cellSize));
    toneMaps.push_back(getHalftoneMap(layer,HALFTONE2_Y,0.f,cellSize));
    toneMaps.push_back(getHalftoneMap(layer,HALFTONE2_K,45.f,cellSize));
    } break;
  case 2: {
    toneMaps.push_back(getHalftoneMap(layer,HALFTONE_R,22.5f,cellSize));
    toneMaps.push_back(getHalftoneMap(layer,HALFTONE_G,45.f,cellSize));
    toneMaps.push_back(getHalftoneMap(layer,HALFTONE_B,0.f,cellSize));
    } break;
  case 3: {
    toneMaps.push_back(getHalftoneMap(layer,HALFTONE3_R,22.5f,cellSize));
    toneMaps.push_back(getHalftoneMap(layer,HALFTONE3_Y,45.f,cellSize));
    toneMaps.push_back(getHalftoneMap(layer,HALFTONE3_B,0.f,cellSize));
    } break;
  case 4: {
    toneMaps.push_back(getHalftoneMap(layer,HALFTONE_R,45.f,cellSize));
    toneMaps.push_back(getHalftoneMap(layer,HALFTONE_G,45.f,cellSize));
    toneMaps.push_back(getHalftoneMap(layer,HALFTONE_B,45.f,cellSize));
    } break;
  }

  HalfToneMap **_toneMaps = &toneMaps[0]; int _toneMapsCount = (int)toneMaps.size();
  if (_toneMapsCount <= 4) {
    for (int y = 0; y < layer->height; y++) {
      if ((y % 100) == 0) updateStatusBar(); // no need for a break
      for (int x = 0; x < layer->width; x++) {
        float vals[4] = {0,0,0,0};
        for (int i = 0; i < _toneMapsCount; i++) {
          HalfToneMap *t = _toneMaps[i];
          int cell = t->pixelsToCells[x+y*layer->width];
          float v = t->cells[cell] / 255.f;
          v *= (float)sqrt(2.0)*dotScale;
          float distX = t->cellToCellCenterX[cell] - (float)x;
          float distY = t->cellToCellCenterY[cell] - (float)y;
          float dist = (float)sqrt(distX*distX+distY*distY);
          if (v != 0) dist /= v; else dist = 1000.f;
          float threshTop = (float)(cellSize * 0.5+1);
          float threshBottom = (float)(cellSize * 0.5);
          vals[i] = dist < threshBottom ? 1 : (dist > threshTop ? 0 : 1-(dist-threshBottom)/(threshTop-threshBottom));
        }
        float r = 0, g = r, b = r;
        switch(type) {
        case 0: {
            r = vals[0] * 255.f;
            g = vals[0] * 255.f;
            b = vals[0] * 255.f;
          } break;
        case 1: {
            float c = 255-vals[0] * 255.f;
            float m = 255-vals[1] * 255.f;
            float y = 255-vals[2] * 255.f;
            float k = 255-vals[3] * 255.f;
            fromCMYK_(c,m,y,k);
            r = c;
            g = m;
            b = y;
          } break;
        case 4:
        case 2: {
            r = vals[0] * 255.f;
            g = vals[1] * 255.f;
            b = vals[2] * 255.f;
          } break;
        case 3: {
            r = vals[0] * 255.f;
            g = vals[1] * 255.f;
            b = vals[2] * 255.f;
            fromRYB_(r,g,b);
          } break;
        }
        int ri = (int)r; int gi = (int)g; int bi = (int)b;
        if (ri < 0) ri = 0;
        if (gi < 0) gi = 0;
        if (bi < 0) bi = 0;
        if (ri > 255) ri = 255;
        if (gi > 255) gi = 255;
        if (bi > 255) bi = 255;
        layer->pixels[x+y*layer->width] = ri|(gi<<8)|(bi<<16)|(layer->pixels[x+y*layer->width] & 0xff000000);
      }
    }
  }  

  for (int i = 0; i < (int)toneMaps.size(); i++) {delete toneMaps[i];} toneMaps.clear();
  repaintLayer(layer,true);
  updatedPixels(layer);
  afterUndoPoint();
  return true;
}

float luminanceDist(unsigned int col1, unsigned int col2) {
  int ra = (int)(col1 & 255);
  int ba = (int)((col1>>8) & 255);
  int ga = (int)((col1>>16) & 255);
  int aa = (int)((col1>>24) & 255);

  int rb = (int)(col2 & 255);
  int bb = (int)((col2>>8) & 255);
  int gb = (int)((col2>>16) & 255);
  int ab = (int)((col2>>24) & 255);

  float *lumis = luminanceValues;
  float la = (float)ra*lumis[0] + (float)ba*lumis[1] + (float)ga*lumis[2] + aa;
  float lb = (float)rb*lumis[0] + (float)bb*lumis[1] + (float)gb*lumis[2] + ab;
  return lb - la;
}

int colorDistSqr(unsigned int col1, unsigned int col2) {
  int ra = (int)(col1 & 255);
  int ba = (int)((col1>>8) & 255);
  int ga = (int)((col1>>16) & 255);
  int aa = (int)((col1>>24) & 255);
  int rb = (int)(col2 & 255);
  int bb = (int)((col2>>8) & 255);
  int gb = (int)((col2>>16) & 255);
  int ab = (int)((col2>>24) & 255);
  ra -= rb; ga -= gb; ba -= bb; aa -= ab;
  return ra*ra + ga*ga + ba*ba + aa*aa;
}

int medianColorSort(const void *p, const void *q)
{
  const unsigned int a = *((const unsigned int*)p);
  const unsigned int b = *((const unsigned int*)q);
  float d = luminanceDist(a,b);
  if (d < 0) return -1;
  if (d > 0) return 1;
  return 0;
}


bool filterMedianize(Layer *layer, float cellSize) {
  if (layer == NULL) return false;
  if (locked(layer,LOCK_ANY)) return false;
  beforeUndoPoint();
  Layer *l = layer;

  int cellX = (int)cellSize;
  int cellY = (int)cellSize;

  int cellCountX = (l->width + cellX - 1) / cellX;
  int cellCountY = (l->height + cellY - 1) / cellY;

  Array<unsigned int> medianColorsForCell; medianColorsForCell.resize(cellCountX*cellCountY);
  Array<unsigned int> cellToSort; cellToSort.resize(cellX*cellY);

  {for (int y = 0; y < cellCountY; y++) {
    for (int x = 0; x < cellCountX; x++) {
      int w = 0;
      for (int y2 = 0; y2 < cellY; y2++) {
        for (int x2 = 0; x2 < cellX; x2++) {
          int lx = x * cellX + x2;
          int ly = y * cellY + y2;
          if ((unsigned int)lx < (unsigned int)l->width && (unsigned int)ly < (unsigned int)l->height) {
            cellToSort[w] = l->pixels[lx+ly*l->width]; 
            w++;
          }
        }
      }
      if (w > 0) {
        if (w > 1) qsort(&cellToSort[0],w,sizeof(unsigned int),medianColorSort);
        medianColorsForCell[x+y*cellCountX] = cellToSort[w/2];
      }
    }
  }}

  {for (int y = 0; y < l->height; y++) {
    for (int x = 0; x < l->width; x++) {
      unsigned int col = l->pixels[x+y*l->width];
      unsigned int color = col;
      int cx = x / cellX; int cy = y / cellY;
      int lowestColorDist = 256*256*5;
      for (int y2 = -1; y2 <= 1; y2++) {
        for (int x2 = -1; x2 <= 1; x2++) {
          int lx = cx + x2;
          int ly = cy + y2;
          if ((unsigned int)lx<(unsigned int)cellCountX && (unsigned int)ly<(unsigned int)cellCountY) {
            int dist = colorDistSqr(col, medianColorsForCell[lx+ly*cellCountX]);
            if (dist < lowestColorDist) {
              lowestColorDist = dist;
              color = medianColorsForCell[lx+ly*cellCountX];
            }
          }
        }
      }
      l->pixels[x+y*l->width] = color;
    }
  }}

  repaintLayer(layer,true);
  updatedPixels(layer);
  afterUndoPoint();
  return true;
}

bool filterSegmentation(class Layer *layer, int cellCountX, int cellCountY) {
  if (layer == NULL) return false;
  if (locked(layer,LOCK_ANY)) return false;
  beforeUndoPoint();

  Layer *l = layer;

  Array<unsigned int> blurred; blurred.resize(layer->width*layer->height);
  int kernel = 1; filterMedianBlur(&blurred[0], layer->pixels, layer->width, layer->height, kernel, kernel, layer->pixelStates, false, false, false);
  if (escapeWasPressed) return false;

  // calculate the gradient in values
  float *lumis = luminanceValues;
  Array<float> values; values.resize(l->width*l->height); int sobelXMat[3*3] = {+1,+0,-1, +2,+0,-2, +1,+0,-1}; int sobelYMat[3*3] = {+1,+2,+1, 0,0,0, -1,-2,-1};
  for (int y = 0; y < l->height; y++) {
    for (int x = 0; x < l->width; x++) {
      float sobelX[5] = {0,0,0,0,0};
      float sobelY[5] = {0,0,0,0,0};
      for (int y2 = -1; y2 <= 1; y2++) {
        for (int x2 = -1; x2 <= 1; x2++) {
          int x3 = x + x2;
          int y3 = y + y2;
          if ((unsigned int)x3<(unsigned int)l->width && (unsigned int)y3<(unsigned int)l->height) {
            unsigned int col = blurred[x3+y3*l->width];
            int r = col & 255;
            int g = (col>>8) & 255;
            int b = (col>>16) & 255;
            int a = (col>>24) & 255;
            double sl = r * lumis[0] + g * lumis[1] + b * lumis[2];
            sobelX[0] += r*sobelXMat[(x2+1)+(y2+1)*3]; sobelY[0] += r*sobelYMat[(x2+1)+(y2+1)*3];
            sobelX[1] += g*sobelXMat[(x2+1)+(y2+1)*3]; sobelY[1] += g*sobelYMat[(x2+1)+(y2+1)*3];
            sobelX[2] += b*sobelXMat[(x2+1)+(y2+1)*3]; sobelY[2] += b*sobelYMat[(x2+1)+(y2+1)*3];
            sobelX[3] += (float)(sl*sobelXMat[(x2+1)+(y2+1)*3]); sobelY[3] += (float)(sl*sobelYMat[(x2+1)+(y2+1)*3]);
            sobelX[4] += a*sobelXMat[(x2+1)+(y2+1)*3]; sobelY[4] += a*sobelYMat[(x2+1)+(y2+1)*3];
          }
        }
      }
      if (y == 0 || y == l->height-1) {sobelY[0] = sobelY[1] = sobelY[2] = sobelY[3] = sobelY[4] = 0;}
      if (x == 0 || x == l->width-1) {sobelX[0] = sobelX[1] = sobelX[2] = sobelX[3] = sobelX[4] = 0;}
      values[x+y*l->width] = (float)sqrt(sobelX[0]*sobelX[0]+sobelY[0]*sobelY[0]+sobelX[1]*sobelX[1]+sobelY[1]*sobelY[1]+sobelX[2]*sobelX[2]+sobelY[2]*sobelY[2]+sobelX[3]*sobelX[3]+sobelY[3]*sobelY[3]+sobelX[4]*sobelX[4]+sobelY[4]*sobelY[4]);
    }
  }
  float minV = 0; float maxV = 0;
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

  initStatusBar();

  Array<WatershedEntry> priorityQueue;
  Array<unsigned int> labels; labels.resize(layer->width * layer->height); memset(&labels[0],0,sizeof(unsigned int)*labels.size());

  int labelCount = 1;
  for (int x = 0; x < cellCountX; x++) {
    for (int y = 0; y < cellCountY; y++) {
      int lx = (int)(layer->width * ((double)x+0.5) / cellCountX);
      int ly = (int)(layer->height * ((double)y+0.5) / cellCountY);
      if ((unsigned int)lx < (unsigned int)layer->width && (unsigned int)ly < (unsigned int)layer->height) {
        labels[lx+ly*layer->width] = labelCount; labelCount++;
        putInPriorityQueue(priorityQueue, lx, ly, values[lx+ly*layer->width]);
      }
    }
  }

  int count = 0;
  float *values0 = &values[0];
  unsigned int *labels0 = &labels[0]; int lw = l->width; int lh = l->height;
  int portion = l->width * l->height / 15; if (portion < 1) portion = 1;
  while(!priorityQueue.empty()) {
    if ((count % portion) == 0) UPDATESTATUSBARWITHBREAK(;);
    WatershedEntry here = priorityQueue.back(); priorityQueue.pop_back();
    int x = here.x; int y = here.y;
    proceedWatershed(x,y, priorityQueue, values0, labels0, 1, 0, lw, lh);
    proceedWatershed(x,y, priorityQueue, values0, labels0, -1, 0, lw, lh);
    proceedWatershed(x,y, priorityQueue, values0, labels0, 0, 1, lw, lh);
    proceedWatershed(x,y, priorityQueue, values0, labels0, 0, -1, lw, lh);

    proceedWatershed(x,y, priorityQueue, values0, labels0, 1, 1, lw, lh);
    proceedWatershed(x,y, priorityQueue, values0, labels0, -1, 1, lw, lh);
    proceedWatershed(x,y, priorityQueue, values0, labels0, 1, -1, lw, lh);
    proceedWatershed(x,y, priorityQueue, values0, labels0, -1, -1, lw, lh);
    count++;   
  }

  // despeckle, doesn't need to be double buffered
  // the (single pixel) cells already had an asignment which is never overwriten so despeckle removes it
  Array<unsigned int> prio2; prio2.resize(3*3);
  unsigned int *prio2_ = &prio2[0];
  {for (int y = 0; y < l->height; y++) {
    if ((y % 100) == 0) UPDATESTATUSBARWITHBREAK(;);
    for (int x = 0; x < l->width; x++) {
      int prioCount = 0;
      for (int y2 = -1; y2 <= 1; y2++) {
        for (int x2 = -1; x2 <= 1; x2++) {
          int kx = x + x2;
          int ky = y + y2;
          if ( ((unsigned int)kx >= (unsigned int)l->width) && ((unsigned int)ky >= (unsigned int)l->height) ) continue;
          prio2_[prioCount] = labels[kx+ky*l->width];
          prioCount++;
        }
      }
      qsort(prio2_, prioCount, sizeof(unsigned int), medianIntSort);
      int sameAsMid = 0;
      unsigned int current = prio2_[0];
      int currentCount = 0;
      unsigned int currentBest = current;
      int currentBestCount = 0;
      for (int i = 0; i < prioCount + 1; i++) {
        if (i == prioCount || prio2_[i] != current) {
          if (currentCount > currentBestCount) {
            currentBest = current;
            currentBestCount = currentCount;
          }
          currentCount = 0;
          current = i == prioCount ? 0 : prio2_[i];
        }
        if (i != prioCount && prio2_[i] == labels[x+y*l->width]) sameAsMid++;
        currentCount++;
      }
      bool shouldFilterOut = sameAsMid == 1 || sameAsMid == 2;
      blurred[x+y*layer->width] = shouldFilterOut ? currentBest : labels[x+y*l->width];
    }
  }}

  Array<int> red; red.resize(labelCount); red.fill(0);
  Array<int> green; green.resize(labelCount); green.fill(0);
  Array<int> blue; blue.resize(labelCount); blue.fill(0);
  Array<int> alpha; alpha.resize(labelCount); alpha.fill(0);
  Array<int> weight; weight.resize(labelCount); weight.fill(0);

  for (int i4 = 0; i4 < l->width * l->height; i4++) {
    unsigned int label = blurred[i4];
    unsigned int col = l->pixels[i4];
    int r = col & 255;
    int g = (col>>8) & 255;
    int b = (col>>16) & 255;
    int a = (col>>24) & 255;
    red[label] += r;
    green[label] += g;
    blue[label] += b;
    alpha[label] += a;
    weight[label] += 1;
  }

  for (int i5 = 0; i5 < l->width * l->height; i5++) {
    unsigned int label = blurred[i5];
    int w = weight[label];
    if ( w == 0) w = 1;
    int r = red[label] / w;
    int g = green[label] / w;
    int b = blue[label] / w;
    int a = alpha[label] / w;
    l->pixels[i5] = r | (g<<8) | (b<<16) | (a<<24);
  }

  repaintLayer(layer,true);
  updatedPixels(layer);
  afterUndoPoint();
  return true;
}

bool filterSprinkled(class Layer *layer, float minRad, float maxRad) {
  if (layer == NULL) return false;
  if (locked(layer,LOCK_ANY)) return false;
  beforeUndoPoint();
  Layer *l = layer;

  if (minRad < 1) minRad = 1;
  if (maxRad < 1) maxRad = 1;

  for (int i = 0; i < l->width*l->height; i++) l->pixelStates[i] |= PIXELSTATE_TEMP;

  Array<unsigned int> potentialPixels; potentialPixels.resize(l->width * l->height);
  unsigned int *potentialPixels_ = &potentialPixels[0];
  for (int y = 0; y < (int)l->height; y++) {
    for (int x = 0; x < (int)l->width; x++) {
      potentialPixels[x+y*l->width] = (unsigned int)x | ((unsigned int)y << 16);
    }
  }

  int potentialPixelsCount = (int)potentialPixels.size();

  Array<float> red; red.resize(l->width*l->height); red.fill(0);
  Array<float> green; green.resize(l->width*l->height); green.fill(0);
  Array<float> blue; blue.resize(l->width*l->height); blue.fill(0);
  Array<float> alpha; alpha.resize(l->width*l->height); alpha.fill(0);
  Array<float> weight; weight.resize(l->width*l->height); weight.fill(0);
  float *red_ = &red[0];
  float *green_ = &green[0];
  float *blue_ = &blue[0];
  float *alpha_ = &alpha[0];
  float *weight_ = &weight[0];

  initStatusBar();

  srand(0);
  unsigned int portionSize = potentialPixelsCount / 15; if (portionSize < 1) portionSize = 1;
  unsigned int lastPortion = potentialPixelsCount / portionSize;

  Array<float> k; int ks = 0; float *kd = &k[0];

  while(potentialPixelsCount > 0) {
    unsigned int portion = (unsigned int)potentialPixelsCount / portionSize; 
    if (portion != lastPortion) UPDATESTATUSBARWITHBREAK(;);
    lastPortion = portion;

    int i = ((int)rand()+(int)rand()*510+(int)rand()*11100) % potentialPixelsCount; // doesn't have to be fully 32 bit

    float centerX = (float)(potentialPixels_[i] & 0xffff);
    float centerY = (float)((potentialPixels_[i]>>16) & 0xffff);
    float circleSize = (float)(rand() % RAND_MAX)/(float)RAND_MAX;
    circleSize *= circleSize;
    float radius = minRad + circleSize*(maxRad - minRad);

    int ri = (int)(radius + 1);
    int cx = (int)(centerX);
    int cy = (int)(centerY);
    float threshTop = radius+1;
    float threshBottom = radius;

    float r = 0, g = 0, b = 0, a = 0, w = 0;
    int ki = 0;
    for (int y = -ri + cy; y <= ri + cy; y++) {
      float dy = (float)y - centerY;
      dy *= dy;
      for (int x = -ri + cx; x <= ri + cx; x++) {
        float dx = (float)x - centerX;
        float d = (float)sqrt(dx*dx+dy); 
        float shade = d < threshBottom ? 1 : (d > threshTop ? 0 : 1-(d-threshBottom)/(threshTop-threshBottom));
        if (ki >= ks) {ks = ki+1; k.resize(ks); kd = &k[0];}
        kd[ki] = shade; ki++;
        if (shade > 0) {
          if ((unsigned int)x < (unsigned int)l->width && (unsigned int)y < (unsigned int)l->height) {
            unsigned int col = l->pixels[x+y*l->width]; 
            l->pixelStates[x+y*l->width] &= ~PIXELSTATE_TEMP;
            float h = shade;
            r += (float)(col & 255) * h;
            g += (float)((col>>8) & 255) * h;
            b += (float)((col>>16) & 255) * h;
            a += (float)((col>>24) & 255) * h;
            w += h;
          }
        }
      }
    }

    if (w > 0) {
      ki = 0;
      for (int y = -ri + cy; y <= ri + cy; y++) {
        for (int x = -ri + cx; x <= ri + cx; x++) {
          float shade = kd[ki]; ki++;
          if (shade > 0) {
            if ((unsigned int)x < (unsigned int)l->width && (unsigned int)y < (unsigned int)l->height) {
              int j = x + y * l->width;
              red_[j] += r*shade;
              green_[j] += g*shade;
              blue_[j] += b*shade;
              alpha_[j] += a*shade;
              weight_[j] += w*shade;
            }
          }
        }
      }
    }

    for (int j = 0; j < potentialPixelsCount; j++) {
      int x = (int)(potentialPixels_[j] & 0xffff);
      int y = (int)((potentialPixels_[j]>>16) & 0xffff);
      bool marked = false;
      if ((unsigned int)x < (unsigned int)l->width && (unsigned int)y < (unsigned int)l->height) {
        if ( (l->pixelStates[x+y*l->width] & PIXELSTATE_TEMP) == 0 ) marked = true;
      } else {marked = true;}
      if (marked) {
        potentialPixelsCount--;
        if (potentialPixelsCount >= 0 && j >= 0) {
          potentialPixels_[j] = potentialPixels_[potentialPixelsCount];
        }
        j--;
      }
    }
  }

  for (int j = 0; j < l->width * l->height; j++) {
    float w = weight_[j]; if (w == 0) w = 1;
    int r = (int)(red_[j]/w);
    int g = (int)(green_[j]/w);
    int b = (int)(blue_[j]/w);
    int a = (int)(alpha_[j]/w);
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (a < 0) a = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    if (a > 255) a = 255;
    l->pixels[j] = r|(g<<8)|(b<<16)|(a<<24);
  }

  repaintLayer(layer,true);
  updatedPixels(layer);
  updatedPixelStates(layer);
  afterUndoPoint();
  return true;
}

bool filterComic(class Layer *layer, float steps, bool sat) {
  if (layer == NULL) return false;
  if (locked(layer,LOCK_ANY)) return false;
  beforeUndoPoint();

  if (steps < 1) steps = 1;
                                                                                                       
  Array<float> hue; hue.resize(layer->width * layer->height);
  Array<float> saturation; saturation.resize(layer->width * layer->height);
  Array<float> lightness; lightness.resize(layer->width * layer->height);

  float minLightness = 1.f;
  float maxLightness = 0.f;

  {for (int i = 0; i < layer->width * layer->height; i++) {
    unsigned int col = layer->pixels[i];
    ImVec4 rgb = getImVec4Color(col);
    getHsl(rgb, &hue[i], &saturation[i], &lightness[i]);
    if (lightness[i] < minLightness) minLightness = lightness[i];
    if (lightness[i] > maxLightness) maxLightness = lightness[i];
  }}

  float lightnessRange = maxLightness - minLightness;

  {for (int i = 0; i < layer->width * layer->height; i++) {
    double h = hue[i];
    double s = saturation[i];
    double l = lightness[i];
    l = l - minLightness; if (lightnessRange > 0) l /= lightnessRange;
    int k = (int)(l * steps);
    l = l - (double)k / (double)steps;
    l = l / 2.0; l = 0;
    l = l + (double)k / (double)steps;
    l = l * lightnessRange; l = l + minLightness;
    if (sat) {
      s = 1 - s;
      s = s*s*s;
      s = 1 - s;
    }
    ImVec4 rgb = hsl(h,s,l);
    unsigned int col = getUIntColor(rgb);
    layer->pixels[i] = (col & 0x00ffffff) | (layer->pixels[i] & 0xff000000);
  }}

  Array<unsigned int> blurred; blurred.resize(layer->width*layer->height);
  int kernel = 2; 
  filterMedianBlur(&blurred[0], layer->pixels, layer->width, layer->height, kernel, kernel, layer->pixelStates, false, false, false);
  if (escapeWasPressed) {revertToBeforeUndoPoint(); return false;}
  filterMedianBlur(layer->pixels, &blurred[0], layer->width, layer->height, kernel, kernel, layer->pixelStates, false, false, false);
  if (escapeWasPressed) {revertToBeforeUndoPoint(); return false;}

  repaintLayer(layer,true);
  updatedPixels(layer);
  afterUndoPoint();
  return true;
}
