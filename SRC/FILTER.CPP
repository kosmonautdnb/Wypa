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

float sfrand2d(int seed,float x, float y) {
  x *= 3.3333;
  y *= 7.7777;
  srand((*((unsigned int*)&x))^seed); int xs = rand();
  srand((*((unsigned int*)&y))^seed); int ys = rand();
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
  if (l->state.locked) return;
  beforeUndoPoint();
  Array<float> values;
  values.resize(l->width*l->height);
  for (int i = 0; i < values.size(); i++) values[i] = 0;
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

void filterEdgeDetect() {
  Layer *l = currentCanvas->getActiveLayer();
  if (l == NULL) return;
  if (l->state.locked) return;
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
          if ((unsigned int)x3<l->width && (unsigned int)y3<l->height) {
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

void filterBoxBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int boxW, int boxH, unsigned char *pixelStates, bool usePixelStates) {
  if (boxW < 1 || boxH < 1 || boxW >= width*1.5 || boxH >= height*1.5) {memcpy(dest,source,width*height*sizeof(unsigned int)); return;}
  initStatusBar();
  Array<unsigned int> blurred;
  blurred.resize(width*height*5);
  for (int y = 0; y < height; y++) {
    if ((y & 15)==0) updateStatusBar();
    for (int x = 0; x < width; x++) {
      int w = 0;
      int r = 0;
      int g = 0;
      int b = 0;
      int a = 0;
      for (int x2 = -boxW; x2 <= boxW; x2++) {
        int x3 = x2 + x;
        int y3 = y;
        if ((unsigned int)x3<width&&(unsigned int)y3<height) {
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
  for (int y4 = 0; y4 < height; y4++) {
    if ((y4 & 15)==0) updateStatusBar();
    for (int x4 = 0; x4 < width; x4++) {
      if ((!usePixelStates)||(pixelStates[x4+y4*width] & PIXELSTATE_SELECTED)) {
        int w = 0;
        int r = 0;
        int g = 0;
        int b = 0;
        int a = 0;
        for (int y2 = -boxH; y2 <= boxH; y2++) {
          int x3 = x4;
          int y3 = y2 + y4;
          if ((unsigned int)x3<width&&(unsigned int)y3<height) {
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
  for (int y4 = 0; y4 < height; y4++) {
    if ((y4 & 15)==0) updateStatusBar();
    for (int x4 = 0; x4 < width; x4++) {
      if ((!usePixelStates)||(pixelStates[x4+y4*width] & PIXELSTATE_SELECTED)) {
        int r = 0;
        int g = 0;
        int b = 0;
        int a = 0;
        int w = 0;
        for (int k = -radialSize; k <= radialSize; k++) {
          int x3 = stepX * k + x4;
          int y3 = stepY * k + y4;
          if ((unsigned int)x3 < width && (unsigned int)y3 < height) {
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

void bloomX(unsigned int *buffer, unsigned int width, unsigned int height, int tap0, int tap1) {
  int weight0 =  gaussianTap5Weight0 * 0x100;
  int weight1 =  gaussianTap5Weight1 * 0x100;
  int weight2 =  gaussianTap5Weight2 * 0x100; // center
  int weights = (weight0+weight1+weight2+weight1+weight0);
  weight0 = weight0*256/weights;
  weight1 = weight1*256/weights;
  weight2 = weight2*256/weights;
  unsigned int *newBuffer = new unsigned int[width*height];
  for (int y = 0; y < height; y++) {
    unsigned int *s = &buffer[y*width];
    unsigned int *d = &newBuffer[y*width];
    int rgb;
    for (int x = 0; x < width; x++) {
      int t0x = x - tap0; if (t0x < 0) t0x = 0;
      int t1x = x - tap1; if (t1x < 0) t1x = 0;
      int t3x = x + tap1; if (t3x > width-1) t3x = width-1;
      int t4x = x + tap0; if (t4x > width-1) t4x = width-1;
      unsigned int rgba0 = s[t0x];
      unsigned int rgba1 = s[t1x];
      unsigned int rgba2 = s[x];
      unsigned int rgba3 = s[t3x];
      unsigned int rgba4 = s[t4x];
      BLOOMKERNEL
      *d++=rgb;
    }
  }
  memcpy(buffer,newBuffer,width*height*sizeof(unsigned int));
  delete[] newBuffer;
}

void bloomY(unsigned int *buffer, unsigned int width, unsigned int height, int tap0, int tap1) {
  int weight0 =  gaussianTap5Weight0 * 0x100;
  int weight1 =  gaussianTap5Weight1 * 0x100;
  int weight2 =  gaussianTap5Weight2 * 0x100; // center
  int weights = (weight0+weight1+weight2+weight1+weight0);
  weight0 = weight0*256/weights;
  weight1 = weight1*256/weights;
  weight2 = weight2*256/weights;
  unsigned int *newBuffer = new unsigned int[width*height];
  for (int x = 0; x < width; x++) {
    unsigned int *s = &buffer[x];
    unsigned int *d = &newBuffer[x];
    int rgb;
    for (int y = 0; y < height; y++) {
      int t0y = y - tap0; if (t0y < 0) t0y = 0;
      int t1y = y - tap1; if (t1y < 0) t1y = 0;
      int t3y = y + tap1; if (t3y > height-1) t3y = height-1;
      int t4y = y + tap0; if (t4y > height-1) t4y = height-1;
      unsigned int rgba0 = s[t0y*width];
      unsigned int rgba1 = s[t1y*width];
      unsigned int rgba2 = s[y*width];
      unsigned int rgba3 = s[t3y*width];
      unsigned int rgba4 = s[t4y*width];
      BLOOMKERNEL
      *d=rgb;
      d+=width;
    }
  }
  memcpy(buffer,newBuffer,width*height*sizeof(unsigned int));
  delete[] newBuffer;
}


void filterGaussBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float passesX, float passesY, unsigned char *pixelStates, bool usePixelStates) {
  if (passesX < 1 || passesY < 1) {memcpy(dest,source,width*height*sizeof(unsigned int)); return;}

  unsigned int *bloomBuffer = new unsigned int[width*height]; for (int i0 = 0; i0 < width*height; i0++) bloomBuffer[i0] = source[i0];

  int px = (int)floor(passesX);
  for (int i1 = px-1; i1 >= 0; i1--) {
    int kernelSize = pow(2,i1);
    bloomX(bloomBuffer,width,height,kernelSize*2,kernelSize*1);
  }
  int py = (int)floor(passesY);
  for (int i2 = py-1; i2 >= 0; i2--) {
    int kernelSize = pow(2,i2);
    bloomY(bloomBuffer,width,height,kernelSize*2,kernelSize*1);
  }

  for (int i3 = 0; i3 < width*height; i3++) {
    if ((!usePixelStates)||(pixelStates[i3] & PIXELSTATE_SELECTED)) {
      dest[i3] = bloomBuffer[i3];
    }
  }

  delete[] bloomBuffer;
}

void filterEmboss(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float angle, unsigned char *pixelStates, bool usePixelStates) {
  int sobelXMat[3*3] = {+1,+0,-1, +2,+0,-2, +1,+0,-1};
  int sobelYMat[3*3] = {+1,+2,+1, 0,0,0, -1,-2,-1};
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if ((!usePixelStates)||(pixelStates[x+y*width] & PIXELSTATE_SELECTED)) {
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
            if ((unsigned int)x3<width && (unsigned int)y3<height) {
              unsigned int col = source[x3+y3*width];
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
        if (y == 0 || y == height-1) {
          sobelYr = sobelYg = sobelYb = 0;
        }
        if (x == 0 || x == width-1) {
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
        float sobelX = (sobelXr + sobelXg + sobelXb) / 3.0;
        float sobelY = (sobelYr + sobelYg + sobelYb) / 3.0;
        float val = 0;
        if (sobelX != 0 || sobelY != 0) {
          float angleHere = atan2(sobelX,sobelY);
          val = sin(angleHere+(angle-90)*PI*2/360.0);
        }
        int grey = val * 128 + 128;
        if (grey < 0) grey = 0;
        if (grey > 255) grey = 255;
        int r = grey;
        int g = grey;
        int b = grey;
        int a = 255;
        dest[x+y*width] = r|(g<<8)|(b<<16)|(a<<24);
      }
    }
  }
}
