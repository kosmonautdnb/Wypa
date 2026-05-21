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
#include "LAYERS.HPP"
#include "CONFIG.HPP"
#include "STL/VECTOR.HPP"
#include "COLORFLD.HPP"
#include "STL/GL.H"
#include "MENUBAR.HPP"
#include "NAVIGTOR.HPP"
#include "SERIALZE.HPP"
#include "TOOLBAR.HPP"
#include "TOOLVIEW.HPP"
#include "SERIALZE.HPP"
#include "TOOLS.HPP"
#include "COLORFLD.HPP"
#include "UNDO.HPP"
#include "STL/CHARSET.HPP"
#include "DIALOGS.HPP" // for gradiation curves
#include <math.h>

unsigned int pixelsUpdateCounter = 0;
unsigned int pixelStatesUpdateCounter = 0;
int redoBeforePixels = 0;
int redoBeforePixelStates = 0;

void updatedPixels(Layer *l) {
  redoBeforePixels = availableRedos; availableRedos = 0;
  pixelsUpdateCounter++; pixelsUpdateCounter &= 0x7fffffff;
  if (l != NULL) l->undoData.updatePixelsCount = pixelsUpdateCounter;
}

void updatedPixelStates(Layer *l) {
  redoBeforePixelStates = availableRedos; availableRedos = 0;
  pixelStatesUpdateCounter++; pixelStatesUpdateCounter &= 0x7fffffff;
  if (l != NULL) l->undoData.updatePixelStatesCount = pixelStatesUpdateCounter;
}

void reverseLastUpdatedPixels(Layer *l) {
  availableRedos = redoBeforePixels;
  pixelsUpdateCounter--;  pixelsUpdateCounter &= 0x7fffffff;
  if (l != NULL) l->undoData.updatePixelsCount = pixelsUpdateCounter;
}

void reverseLastUpdatedPixelStates(Layer *l) {
  availableRedos = redoBeforePixelStates;
  pixelStatesUpdateCounter--;  pixelStatesUpdateCounter &= 0x7fffffff;
  if (l != NULL) l->undoData.updatePixelStatesCount = pixelStatesUpdateCounter;
}

void signSave(Layer *l) {
  if (l != NULL) l->undoData.signSave();
}


Layer *copyLayer = NULL;
Canvas *currentCanvas = NULL;
Array<Canvas*> canvas;
bool shouldUpdateAll = false;
bool shouldUpdate = false;

Layer *selectionPlane;
bool showSelectionPlane = false;
bool showSelectionPlaneBounding = false;
bool clearOutlineAfterNextPaint = false;
bool shouldRecreateQuantization = true;
bool showOnlyActiveLayersSelection = true;

void initLayers() {
  selectionPlane = new Layer();
  selectionPlane->init(10,10);
}

void freeLayers() {
  for (int i = 0; i < canvas.size(); i++) delete canvas[i];
  canvas.clear();
  delete selectionPlane;
}

Matrix inverse2(const Matrix &a) {
  return inverse(a); // maybe this inverse is too simple
}

const char *layerModeNames[LAYERMODE_COUNT] = {"UNDEFINED","Alpha","Premul","Screen","Add","Sub","Mul","Mul2X","Max","Min","Color","Luma","SoftLight","HardLight","LinearLight","PinLight","Diff"};
const char *interpolationNames[INTERPOLATION_COUNT] = {"Pixelated","Averaging","Linear"};
void LayerState::clear() {
  active = false;
  selected = false;
  visible = true;
}

void TransformationState::transformForward(double x, double y, double &rx, double &ry) {
  rx = x*transformation.m[0+0*4]+y*transformation.m[0+1*4]+transformation.m[0+3*4];
  ry = x*transformation.m[1+0*4]+y*transformation.m[1+1*4]+transformation.m[1+3*4];
}

void TransformationState::transformBackward(double x, double y, double &rx, double &ry) {
  rx = x*inverseTransformation.m[0+0*4]+y*inverseTransformation.m[0+1*4]+inverseTransformation.m[0+3*4];
  ry = x*inverseTransformation.m[1+0*4]+y*inverseTransformation.m[1+1*4]+inverseTransformation.m[1+3*4];
}

void TransformationState::clear() {
  transformation.identity();
  inverseTransformation.identity();
}

void TransformationState::update() {
  inverseTransformation = inverse2(transformation);
}

void TransformationState::translate(double x, double y) {
  transformation.translate(Vector(x,y));
  update();
}

void TransformationState::scale(double x, double y) {
  transformation.scale3(Vector(x,y,1));
  update();
}

void TransformationState::scaleAround(double scaleX, double scaleY, double centerX, double centerY) {
  transformation.translate(Vector(-centerX, -centerY,0));
  Matrix sc; sc.scale3(Vector(scaleX,scaleY,1));
  transformation = sc * transformation;
  transformation.translate(Vector(centerX, centerY,0));
  update();
}

void TransformationState::fresh() {
  clear();
}

void TransformationState::zoom(double zoom) {
  zoom = zoom/100.0;
  if (zoom == 0) return;
  scaleAround(1.0/zoom,1.0/zoom,0,0);
}

void TransformationState::aspect(double aspect) {
  scaleAround(aspect,1,0,0);
}

void TransformationState::pixelSize(double x, double y) {
  if (x == 0 || y == 0) return;
  scaleAround(1.0/x,1.0/y,0,0);
}

float *ColorTransform::processingGet() {
  float *processing = new float[256*4*4];
  double gam0 = pictureGammaInvert ? ((pictureGamma!=0) ? 1.0/pictureGamma:1) : pictureGamma;
  double gam1 = gammaInvert ? ((gamma!=0) ? 1.0/gamma:1) : gamma;
  float h = hue;
  float s = saturation;
  float l = lightness;
  float cv = contrast;
  float bv = brightness;
  float sr = scaleRed;
  float sg = scaleGreen;
  float sb = scaleBlue;
  float sa = scaleAlpha;
  float ar = addRed/3.0;
  float ag = addGreen/3.0;
  float ab = addBlue/3.0;
  float aa = addAlpha;
  Vector a(0.25,0.6,0.15); a = normalize(a);
  Array<unsigned char> gradiation;
  if (!gradientCurves.empty()) {
    stringToCurves(gradientCurves);
    gradiation.resize(256*5);
    getGradingArrayFromCurves(&gradiation[0]);
  }
  for (int i = 0; i < 4*256*4; i++) {
    int channel = i % 4;
    int value = (i / 4) % 256;
    int component = (i/256/4);
    float rf=0,gf=0,bf=0,af=0;
    if (component < 3) {
      ImVec4 c2(component == 0?1:0,component == 1?1:0,component == 2?1:0,1); 
      if (perceptional) {
        c2.x *= a.x;
        c2.y *= a.y;
        c2.z *= a.z;
      }
      float value2 = powd(value/255.0,gam0);
      value2 = ((value2*255-128)*cv+128)/255.0;
      if (value2 < 0) value2 = 0;
      if (value2 > 1) value2 = 1;
      value2 += bv;
      c2.x *= value2;
      c2.y *= value2;
      c2.z *= value2;
      if (c2.x < 0) c2.x = 0;
      if (c2.y < 0) c2.y = 0;
      if (c2.z < 0) c2.z = 0;
      if (c2.x > 1) c2.x = 1;
      if (c2.y > 1) c2.y = 1;
      if (c2.z > 1) c2.z = 1;
      if (removeGreen) c2.y = 0;
      float h2,s2,l2; getHsl(c2,&h2,&s2,&l2); h2 = (h2 * 360 + h)/360.0;
      if (h2 < 0) h2 = 0;
      h2 = fmod(h2,1);
      if (h2 > 1) h2 = 1;
      ImVec4 col = hsl(h2,s*s2,l*l2*2);
      rf = col.x*255;
      gf = col.y*255;
      bf = col.z*255;

      float rf2 = rf * colorMatrix[0*4+0] + gf * colorMatrix[1*4+0] + bf * colorMatrix[2*4+0] + colorMatrix[4*4+0];
      float gf2 = rf * colorMatrix[0*4+1] + gf * colorMatrix[1*4+1] + bf * colorMatrix[2*4+1] + colorMatrix[4*4+1];
      float bf2 = rf * colorMatrix[0*4+2] + gf * colorMatrix[1*4+2] + bf * colorMatrix[2*4+2] + colorMatrix[4*4+2];
      rf = rf2;
      gf = gf2;
      bf = bf2;
      float outputR = 0;
      float outputG = 0;
      float outputB = 0;
      if (middleRedAdjust>0) {
        float adjust = middleRedAdjust;
        outputR += rf*(1-adjust)+gf*(adjust);
        outputG += gf*(1-adjust)+rf*(adjust);
        outputB += bf;
      } else {
        float adjust = -middleRedAdjust;
        outputR += rf*(1-adjust)+bf*(adjust);
        outputG += gf;
        outputB += bf*(1-adjust)+rf*(adjust);
      }
      if (middleGreenAdjust>0) {
        float adjust = middleGreenAdjust;
        outputR += rf;
        outputG += gf*(1-adjust)+bf*(adjust);
        outputB += bf*(1-adjust)+gf*(adjust);
      } else {
        float adjust = -middleGreenAdjust;
        outputR += rf*(1-adjust)+gf*(adjust);
        outputG += gf*(1-adjust)+rf*(adjust);
        outputB += bf;
      }
      if (middleBlueAdjust>0) {
        float adjust = middleBlueAdjust;
        outputR += rf*(1-adjust)+bf*(adjust);
        outputG += gf;
        outputB += bf*(1-adjust)+rf*(adjust);
      } else {
        float adjust = -middleBlueAdjust;
        outputR += rf;
        outputG += gf*(1-adjust)+bf*(adjust);
        outputB += bf*(1-adjust)+gf*(adjust);
      }
      rf = outputR/3.0;
      gf = outputG/3.0;
      bf = outputB/3.0;

      rf *= sr;
      gf *= sg;
      bf *= sb;

      rf += ar;
      gf += ag;
      bf += ab;

      if (perceptional) {
        rf /= a.x;
        gf /= a.y;
        bf /= a.z;
      }

      if (rf < 0) rf = 0;
      if (gf < 0) gf = 0;
      if (bf < 0) bf = 0;
      if (rf > 255) rf = 255;
      if (gf > 255) gf = 255;
      if (bf > 255) bf = 255;
      if (invertR) {rf = -rf; rf += 255.0/3.0;}
      if (invertG) {gf = -gf; gf += 255.0/3.0;}
      if (invertB) {bf = -bf; bf += 255.0/3.0;}
    }
    if (component == 3) {
      af = value;
      float af2 = af * colorMatrix[3*4+3] + colorMatrix[4*4+3];
      af = af2;
      af *= sa;
      af += aa;
      if (af < 0) af = 0;
      if (af > 255) af = 255;
      if (invertA) af = 255-af;
    }
    float f = rf;
    switch(channel) {
    case 0: {f = rf;} break;
    case 1: {f = gf;} break;
    case 2: {f = bf;} break;
    case 3: {f = af;} break;
    }    
    if (f < 0) f = 0;
    if (f > 255) f = 255;
    if (!gradiation.empty()) {
      int fi = (int)floor(f);
      if (fi < 0) fi = 0;
      if (fi > 255) fi = 255;
      f = gradiation[fi+256*(channel+1)];
      if (channel<3) {
        f += gradiation[fi+256*0]-fi;
      }
      if (f < 0) f = 0;
      if (f > 255) f = 255;
    }
    f = powd(f/255.0,gam1)*255.0;
    processing[i] = f;
  }
  return processing;
}

void ColorTransform::fromProcessing(float *processing) {
  float minR[5];
  float minG[5];
  float minB[5];
  float minA[5];
  int component = 0;
  for (component = 0; component < 4; component++) {
    for (int value = 0; value < 256; value++) {
      int i = value + component * 256;
      float rf = processing[i*4+0];
      float gf = processing[i*4+1];
      float bf = processing[i*4+2];
      float af = processing[i*4+3];
      if (value == 0 || rf < minR[component]) minR[component] = rf;
      if (value == 0 || gf < minG[component]) minG[component] = gf;
      if (value == 0 || bf < minB[component]) minB[component] = bf;
      if (value == 0 || af < minA[component]) minA[component] = af;
    }
  }
  for (component = 0; component < 4; component++) {
    for (int value = 0; value < 256; value++) {
      int i = value + component * 256;
      float rf = processing[i*4+0];
      float gf = processing[i*4+1];
      float bf = processing[i*4+2];
      float af = processing[i*4+3];
      rf -= minR[component];
      gf -= minG[component];
      bf -= minB[component];
      af -= minA[component];
      processing[i*4+0] = rf;
      processing[i*4+1] = gf;
      processing[i*4+2] = bf;
      processing[i*4+3] = af;
    }
  }

  float maxR[5];
  float maxG[5];
  float maxB[5];
  float maxA[5];

  for (component = 0; component < 4; component++) {
    for (int value = 0; value < 256; value++) {
      int i = value + component * 256;
      float rf = processing[i*4+0];
      float gf = processing[i*4+1];
      float bf = processing[i*4+2];
      float af = processing[i*4+3];
      if (value == 0 || rf > maxR[component]) maxR[component] = rf;
      if (value == 0 || gf > maxG[component]) maxG[component] = gf;
      if (value == 0 || bf > maxB[component]) maxB[component] = bf;
      if (value == 0 || af > maxA[component]) maxA[component] = af;
    }
  }

  float radd = 0;
  float gadd = 0;
  float badd = 0;
  float aadd = 0;
  for (component = 0; component < 4; component++) {
    radd += minR[component];
    gadd += minG[component];
    badd += minB[component];
    aadd += minA[component];
  }

  for (component = 0; component < 4; component++) {
    float spaceR = (maxR[component]);
    float spaceG = (maxG[component]);
    float spaceB = (maxB[component]);
    float spaceA = (maxA[component]);
    float addiToR = floor(radd+0.5);
    float addiToG = floor(gadd+0.5);
    float addiToB = floor(badd+0.5);
    float addiToA = floor(aadd+0.5);
    if (addiToR < 0 && addiToR < -spaceR) {addiToR = -spaceR;}
    if (addiToG < 0 && addiToG < -spaceG) {addiToG = -spaceG;}
    if (addiToB < 0 && addiToB < -spaceB) {addiToB = -spaceB;}
    if (addiToA < 0 && addiToA < -spaceA) {addiToA = -spaceA;}
    if (addiToR >= 0) {addiToR = 0;} // just sub if negative add is needed
    if (addiToG >= 0) {addiToG = 0;}
    if (addiToB >= 0) {addiToB = 0;}
    if (addiToA >= 0) {addiToA = 0;}
    radd -= addiToR;
    gadd -= addiToG;
    badd -= addiToB;
    aadd -= addiToA;
    for (int value = 0; value < 256; value++) {
      int i = value + component * 256;
      processing[i*4+0] += addiToR;
      processing[i*4+1] += addiToG;
      processing[i*4+2] += addiToB;
      processing[i*4+3] += addiToA;
      if (addiToR < 0 && processing[i*4+0]<0) processing[i*4+0] = 0;
      if (addiToG < 0 && processing[i*4+1]<0) processing[i*4+1] = 0;
      if (addiToB < 0 && processing[i*4+2]<0) processing[i*4+2] = 0;
      if (addiToA < 0 && processing[i*4+3]<0) processing[i*4+3] = 0;
    }
  }

  // new maxs
  for (component = 0; component < 4; component++) {
    for (int value = 0; value < 256; value++) {
      int i = value + component * 256;
      float rf = processing[i*4+0];
      float gf = processing[i*4+1];
      float bf = processing[i*4+2];
      float af = processing[i*4+3];
      if (value == 0 || rf > maxR[component]) maxR[component] = rf;
      if (value == 0 || gf > maxG[component]) maxG[component] = gf;
      if (value == 0 || bf > maxB[component]) maxB[component] = bf;
      if (value == 0 || af > maxA[component]) maxA[component] = af;
    }
  }

  for (component = 0; component < 4; component++) {
    float spaceR = (255-maxR[component]);
    float spaceG = (255-maxG[component]);
    float spaceB = (255-maxB[component]);
    float spaceA = (255-maxA[component]);
    float addiToR = floor(radd+0.5);
    float addiToG = floor(gadd+0.5);
    float addiToB = floor(badd+0.5);
    float addiToA = floor(aadd+0.5);
    if (addiToR >= 0 && addiToR > spaceR) {addiToR = spaceR;}
    if (addiToG >= 0 && addiToG > spaceG) {addiToG = spaceG;}
    if (addiToB >= 0 && addiToB > spaceB) {addiToB = spaceB;}
    if (addiToA >= 0 && addiToA > spaceA) {addiToA = spaceA;}
    if (addiToR < 0) {addiToR = 0;} // just add if positive add is needed
    if (addiToG < 0) {addiToG = 0;}
    if (addiToB < 0) {addiToB = 0;}
    if (addiToA < 0) {addiToA = 0;}
    radd -= addiToR;
    gadd -= addiToG;
    badd -= addiToB;
    aadd -= addiToA;
    for (int value = 0; value < 256; value++) {
      int i = value + component * 256;
      processing[i*4+0] += addiToR;
      processing[i*4+1] += addiToG;
      processing[i*4+2] += addiToB;
      processing[i*4+3] += addiToA;
    }
  }

  for (component = 0; component < 4; component++) {
    for (int value = 0; value < 256; value++) {
      int i = value + component * 256;
      int ri = processing[i*4+0];
      int gi = processing[i*4+1];
      int bi = processing[i*4+2];
      int ai = processing[i*4+3];
      if (ri < 0) ri = 0;
      if (gi < 0) gi = 0;
      if (bi < 0) bi = 0;
      if (ai < 0) ai = 0;
      if (ri > 255) ri = 255;
      if (gi > 255) gi = 255;
      if (bi > 255) bi = 255;
      if (ai > 255) ai = 255;
      unsigned int col = ri|(gi<<8)|(bi<<16)|(ai<<24);
      switch(component) {
      case 0:  {r[value] = col;} break;
      case 1:  {g[value] = col;} break;
      case 2:  {b[value] = col;} break;
      case 3:  {a[value] = col;} break;
      }
    }
  }
}

void ColorTransform::reset() {
  memset(colorMatrix,0,sizeof(colorMatrix));
  colorMatrix[0*4+0] = 1;
  colorMatrix[1*4+1] = 1;
  colorMatrix[2*4+2] = 1;
  colorMatrix[3*4+3] = 1;
  hue = 0;
  saturation = 1;
  lightness = 0.5;
  brightness = 0;
  contrast = 1;
  removeGreen = false;
  gamma = 1;
  gammaInvert = false;
  pictureGamma = 1;
  pictureGammaInvert = false;
  addRed = 0;
  addGreen = 0;
  addBlue = 0;
  addAlpha = 0;
  scaleRed = 1;
  scaleGreen = 1;
  scaleBlue = 1;
  scaleAlpha = 1;
  invertR = false;
  invertG = false;
  invertB = false;
  invertA = false;
  posterizeGrey = 0;
  posterizeRed = 0;
  posterizeGreen = 0;
  posterizeBlue = 0;
  posterizeAlpha = 0;
  posterizeGreyBase = 0;
  posterizeRedBase = 0;
  posterizeGreenBase = 0;
  posterizeBlueBase = 0;
  posterizeAlphaBase = 0;
  posterizeUseGrey = true;
  thresholdRed = 0.333;
  thresholdGreen = 0.333;
  thresholdBlue = 0.333;
  thresholdAlpha = 0;
  thresholdLuma = 128;
  thresholdInvert = false;
  ditherStrength = 0;
  middleRedAdjust = 0;
  middleGreenAdjust = 0;
  middleBlueAdjust = 0;
  perceptional = false;
  shouldUpdate = true;
  gradientCurves = "";
}

void ColorTransform::update() {
  if (!shouldUpdate) return;
  shouldUpdate = false;
  float *processing = processingGet();
  fromProcessing(processing);
  delete[] processing;
}

unsigned int ColorTransform::applyColorTransform(unsigned int pixel) {
  unsigned int colr = r[pixel & 255];
  unsigned int colg = g[(pixel>>8) & 255];
  unsigned int colb = b[(pixel>>16) & 255];
  unsigned int cola = a[(pixel>>24) & 255];
  int r2 = (colr & 255)+(colg & 255)+(colb & 255)+(cola & 255);
  colr >>= 8; colg >>= 8; colb >>= 8; cola >>= 8;
  int g2 = (colr & 255)+(colg & 255)+(colb & 255)+(cola & 255);
  colr >>= 8; colg >>= 8; colb >>= 8; cola >>= 8;
  int b2 = (colr & 255)+(colg & 255)+(colb & 255)+(cola & 255);
  colr >>= 8; colg >>= 8; colb >>= 8; cola >>= 8;
  int a2 = (colr & 255)+(colg & 255)+(colb & 255)+(cola & 255);
  if (r2 > 255) r2 = 255;
  if (g2 > 255) g2 = 255;
  if (b2 > 255) b2 = 255;
  if (a2 > 255) a2 = 255;
  return r2|(g2<<8)|(b2<<16)|(a2<<24);
}

unsigned int ColorTransform::applyPosterize(unsigned int pixel) {
  int r = pixel & 255;
  int g = (pixel>>8) & 255;
  int b = (pixel>>16) & 255;
  int a = (pixel>>24) & 255;
  if (posterizeUseGrey) {
    r -= posterizeGreyBase; if (posterizeGrey>0) r = (int)(r / posterizeGrey)*posterizeGrey; r += posterizeGreyBase;
    g -= posterizeGreyBase; if (posterizeGrey>0) g = (int)(g / posterizeGrey)*posterizeGrey; g += posterizeGreyBase;
    b -= posterizeGreyBase; if (posterizeGrey>0) b = (int)(b / posterizeGrey)*posterizeGrey; b += posterizeGreyBase;
  } else {
    r -= posterizeRedBase; if (posterizeRed>0) r = (int)(r / posterizeRed)*posterizeRed; r += posterizeRedBase;
    g -= posterizeGreenBase; if (posterizeGreen>0) g = (int)(g / posterizeGreen)*posterizeGreen; g += posterizeGreenBase;
    b -= posterizeBlueBase; if (posterizeBlue>0) b = (int)(b / posterizeBlue)*posterizeBlue; b += posterizeBlueBase;
    a -= posterizeAlphaBase; if (posterizeAlpha>0) a = (int)(a / posterizeAlpha)*posterizeAlpha; a += posterizeAlphaBase;
  }
  if (r < 0) r = 0; if (r > 255) r = 255;
  if (g < 0) g = 0; if (g > 255) g = 255;
  if (b < 0) b = 0; if (b > 255) b = 255;
  if (a < 0) a = 0; if (a > 255) a = 255;
  return r|(g<<8)|(b<<16)|(a<<24);
}

unsigned int ColorTransform::applyThreshold(unsigned int pixel) {
  int r = pixel & 255;
  int g = (pixel>>8) & 255;
  int b = (pixel>>16) & 255;
  int a = (pixel>>24) & 255;
  int thresholdRed2 = (int)floor(thresholdRed*256);
  int thresholdGreen2 = (int)floor(thresholdGreen*256);
  int thresholdBlue2 = (int)floor(thresholdBlue*256);
  int thresholdAlpha2 = (int)floor(thresholdAlpha*256);
  int grey = (r*thresholdRed2 + g*thresholdGreen2 + b*thresholdBlue2 + a*thresholdAlpha2)>>8;
  unsigned int rgb1 = 0x00000000;
  if (grey >= thresholdLuma) {
    rgb1 = 0xffffffff;
  }
  if (thresholdInvert) rgb1 ^= 0xffffffff;
  return rgb1;
}

int ColorTransform::byteCount() {
  return sizeof(ColorTransform);
}

void LayerUndoData::freeIt() {
  if (rleDataPixels_forward != NULL) {delete[] rleDataPixels_forward; rleDataPixels_forward = NULL;}
  if (rleDataPixelStates_forward != NULL) {delete[] rleDataPixelStates_forward; rleDataPixelStates_forward = NULL;}
  if (rleDataPixels_backward != NULL) {delete[] rleDataPixels_backward; rleDataPixels_backward = NULL;}
  if (rleDataPixelStates_backward != NULL) {delete[] rleDataPixelStates_backward; rleDataPixelStates_backward = NULL;}
  if (undoLayer != NULL) {delete undoLayer; undoLayer = NULL;}
  completeByteCount = 0;
}

void LayerUndoData::signUndo() {
  updatePixelsCountOfLastUndo = updatePixelsCount;
  updatePixelStatesCountOfLastUndo = updatePixelStatesCount;
}

void LayerUndoData::signSave() {
  updatePixelsCountOfLastSave = updatePixelsCount;
  updatePixelStatesCountOfLastSave = updatePixelStatesCount;
}

void LayerUndoData::signWith(LayerUndoData *source) {
  updatePixelsCount = source->updatePixelsCount;
  updatePixelStatesCount = source->updatePixelStatesCount;
  updatePixelsCountOfLastUndo = source->updatePixelsCountOfLastUndo;
  updatePixelStatesCountOfLastUndo = source->updatePixelStatesCountOfLastUndo;
}

int LayerUndoData::byteCount() {
  int ret = 0;
  ret += completeByteCount;
  if (undoLayer != NULL) ret += undoLayer->byteCount();
  return ret;
}

unsigned int layerIdCounter = 0;
void reverseLastLayerIdCountUp() {
  layerIdCounter--;
}

unsigned int Layer::getPixel(int x, int y) {
  if (state.locked) return 0x00000000;
  if ((unsigned int)x<width&&(unsigned int)y<height) return pixels[x+y*width];
  return 0x00000000;
}

void Layer::setPixel(int x, int y, unsigned int rgba) {
  if (state.locked) return;
  if ((unsigned int)x<width&&(unsigned int)y<height) pixels[x+y*width] = rgba;
}

void Layer::pixelLine(int x0, int y0, int x1, int y1, unsigned int color) {
  if (state.locked) return;
  int deltaX = x1 - x0;
  int deltaY = y1 - y0;
  int delta = abs(deltaX);
  if (abs(deltaY)>delta) delta = abs(deltaY);
  if (delta == 0) return;
  double px = x0;
  double py = y0;
  double xa = (double)(x1 - x0)/delta;
  double ya = (double)(y1 - y0)/delta;
  for (int i = 0; i < delta+1; i++) {
    setPixel(px,py,color);
    px += xa;
    py += ya;
  }
  updatedPixels(this);
}

void Layer::pixelLineOr(int x0, int y0, int x1, int y1, unsigned int _or, bool clampOnLayer) {
  if (state.locked) return;
  int deltaX = x1 - x0;
  int deltaY = y1 - y0;
  int delta = abs(deltaX);
  if (abs(deltaY)>delta) delta = abs(deltaY);
  if (delta == 0) return;
  double px = x0;
  double py = y0;
  double xa = (double)(x1 - x0)/delta;
  double ya = (double)(y1 - y0)/delta;
  for (int i = 0; i < delta+1; i++) {
    int x = px;
    int y = py;
    if (clampOnLayer) {if (x < 0) x = 0; if (y < 0) y = 0; if (x >= width) x = width-1; if (y >= height) y = height-1;}
    if ((unsigned int)x<width&&(unsigned int)y<height) {
      pixels[x+y*width] |= _or;
    }
    px += xa;
    py += ya;
  }
  updatedPixels(this);
}

void Layer::pixelLineEor(int x0, int y0, int x1, int y1, unsigned int _eor, bool clampOnLayer) {
  UNUSED(clampOnLayer);
  if (state.locked) return;
  int deltaX = x1 - x0;
  int deltaY = y1 - y0;
  int delta = abs(deltaX);
  if (abs(deltaY)>delta) delta = abs(deltaY);
  if (delta == 0) return;
  double px = x0;
  double py = y0;
  double xa = (double)(x1 - x0)/delta;
  double ya = (double)(y1 - y0)/delta;
  for (int i = 0; i < delta; i++) {
    int x = px;
    int y = py;
    //if (clampOnLayer) {if (x < 0) x = 0; if (x >= width) x = width-1;  if (y < 0) y = 0; if (y >= height) y = height-1;}
    if ((unsigned int)x<width&&(unsigned int)y<height) {
      pixels[x+y*width] ^= _eor;
    }
    px += xa;
    py += ya;
  }
  updatedPixels(this);
}

void Layer::pixelXLineEor(int x0, int y0, int x1, int y1, unsigned int _eor, bool clampOnLayer) {
  if (state.locked) return;
  if (y1 == y0) return;
  if (y1 < y0) {
    int t;
    t = x0; x0 = x1; x1 = t;
    t = y0; y0 = y1; y1 = t;
  }
  double px = x0;
  int py = y0;
  double xa = (double)(x1 - x0)/(y1-y0);
  for (int i = 0; i < y1-y0; i++) {
    int x = px;
    int y = py;
    if (clampOnLayer) {if (x < 0) x = 0; if (x >= width) x = width-1;} // we don't need y here (xlineeor)
    if ((unsigned int)x<width&&(unsigned int)y<height) {
      pixels[x+y*width] ^= _eor;
    }
    px += xa;
    py ++;
  }
  updatedPixels(this);
}

void Layer::init(int width, int height) {
  if (width*height == 0) return;
  colorTransform = ColorTransform();
  toolInfo = LayerAdditionalToolInfo();
  transform.clear();
  state.clear();
  mode = LAYERMODE_DEFAULT;
  sourceLayerId = 0;
  setContentSize(width, height);
}

void Layer::setContentSize(int width, int height) {
  if (this->width == width && this->height == height) {
    memset(pixels,0,width*height*sizeof(unsigned int));
    memset(pixelStates,0,width*height*sizeof(unsigned char));
    return;
  }
  if (pixels != NULL) {delete[] pixels; pixels = NULL;}
  if (pixelStates != NULL) {delete[] pixelStates; pixelStates = NULL;}
  pixels = new unsigned int[width*height];
  if (pixels==NULL) {
    this->width = 0;
    this->height = 0;
    return;
  }
  memset(pixels,0,width*height*sizeof(unsigned int));
  pixelStates = new unsigned char[width*height];
  if (pixelStates==NULL) {
    this->width = 0;
    this->height = 0;
    return;
  }
  memset(pixelStates,0,width*height*sizeof(unsigned char));
  this->width = width;
  this->height = height;
}

void Layer::invalidateContent() {
  if (pixels != NULL) {delete[] pixels; pixels = NULL;}
  if (pixelStates != NULL) {delete[] pixelStates; pixelStates = NULL;}
  width = 1; height = 1;
  pixels = new unsigned int[width*height];
  pixelStates = new unsigned char[width*height];
  memset(pixels,0,width*height*sizeof(unsigned int));
  memset(pixelStates,0,width*height*sizeof(unsigned char));
}
  
void Layer::clear() {
  if (pixels == NULL) return;
  if (pixelStates == NULL) return;
  if (width*height == 0) return;
  memset(pixels,0,width*height*sizeof(unsigned int));
  memset(pixelStates,0,width*height*sizeof(unsigned char));
  updatedPixels(this);
  updatedPixelStates(this);
}

void Layer::updatePixelStates(unsigned char _and, unsigned char _or, unsigned char _eor) {
  if (state.locked) return;
  unsigned int and2 = _and|(_and<<8)|(_and<<16)|(_and<<24);
  unsigned int or2 = _or|(_or<<8)|(_or<<16)|(_or<<24);
  unsigned int eor2 = _eor|(_eor<<8)|(_eor<<16)|(_eor<<24);
  unsigned int *pixelStates2 = (unsigned int*)pixelStates;
  for (int i = 0; i < width*height/4; i++) {
    unsigned int a = pixelStates2[i];
    a &= and2;
    a |= or2;
    a ^= eor2;
    pixelStates2[i] = a;
  }
  for (int j = width*height/4*4; j < width*height; j++) {unsigned char a = pixelStates[j]; a &= _and; a |= _or; a ^= _eor; pixelStates[j] = a;}
  updatedPixelStates(this);
}

void fixTransform(TransformationState &t) {
  t.translate(0.000001,0.000001); // sadly this is needed the merge of layers fails else by one pixel if layers where mirrored or rotated
  t.update();
}

void Layer::mirrorHorizontally() {
  Matrix center; center.translate(Vector((double)width/2,0,0));
  Matrix modify; modify.scale3(Vector(-1,1,1));
  transform.transformation = center * modify * inverse2(center) * transform.transformation;
  fixTransform(transform);
  transform.update();
  shouldUpdateAll = true;
}

void Layer::mirrorVertically() {
  Matrix center; center.translate(Vector(0,(double)height/2,0));
  Matrix modify; modify.scale3(Vector(1,-1,1));
  transform.transformation = center * modify * inverse2(center) * transform.transformation;
  fixTransform(transform);
  transform.update();
  shouldUpdateAll = true;
}

Matrix straighten(const Matrix &m) {
  Matrix ret;
  for (int i = 0; i < 16; i++) {
    double a = m.m[i];
    if (a < -0.5) a = -1; else
    if (a > 0.5) a = 1; else
    a = 0;
    ret.m[i] = a;
  }
  return ret;
}

void Layer::rotate90DegreesForward() {
  Vector c((double)width/2.0,(double)height/2.0,0);
  c = inverse2(transform.transformation)*c;
  Matrix center; center.translate(Vector(c.x,c.y,0));
  Matrix modify; modify = rotationMatrixAngleAxis(PI*-0.5,0,0,1);
  modify = straighten(modify);
  transform.transformation = transform.transformation * center * modify * inverse2(center);
  fixTransform(transform);
  transform.update();
  shouldUpdateAll = true;
}

void Layer::rotate90DegreesBackward() {
  Vector c((double)width/2.0,(double)height/2.0,0);
  c = inverse2(transform.transformation)*c;
  Matrix center; center.translate(Vector(c.x,c.y,0));
  Matrix modify; modify = rotationMatrixAngleAxis(PI*0.5,0,0,1);
  modify = straighten(modify);
  transform.transformation = transform.transformation * center * modify * inverse2(center);
  fixTransform(transform);
  transform.update();
  shouldUpdateAll = true;
}

void Layer::rotateByDegrees(double degrees) {
  Vector c((double)width/2.0,(double)height/2.0,0);
  c = inverse2(transform.transformation)*c;
  Matrix center; center.translate(Vector(c.x,c.y,0));
  Matrix modify; modify = rotationMatrixAngleAxis(-degrees*PI/180.0,0,0,1);
  transform.transformation = transform.transformation * center * modify * inverse2(center);
  fixTransform(transform);
  transform.update();
  shouldUpdateAll = true;
}

void Layer::selectAll() {
  if (state.locked) return;
  for (int i = 0; i < width * height; i++) pixelStates[i] |= PIXELSTATE_SELECTED;
  shouldUpdateAll = true;
  updatedPixelStates(this);
}

void Layer::selectNone() {
  if (state.locked) return;
  unsigned char f = ~PIXELSTATE_SELECTED; unsigned int f2 = (f)|(f<<8)|(f<<16)|(f<<24);
  unsigned int *pixelStates2 = (unsigned int*)pixelStates;
  for (int i = 0; i < width * height/4; i++) pixelStates2[i] &= f2;
  for (int j = width*height/4*4; j < width*height; j++) {pixelStates[j] &= f;}
  shouldUpdateAll = true;
  updatedPixelStates(this);
}

void Layer::invertSelection() {
  if (state.locked) return;
  for (int i = 0; i < width * height; i++) pixelStates[i] ^= PIXELSTATE_SELECTED;
  shouldUpdateAll = true;
  updatedPixelStates(this);
}

void Layer::fillSelectionWithColor(unsigned int color) {
  if (state.locked) return;
  for (int i = 0; i < width * height; i++) if (pixelStates[i] & PIXELSTATE_SELECTED) pixels[i] = color;
  shouldUpdateAll = true;
  updatedPixels(this);
  addUsedColor(getImVec4Color(color));
}

void Layer::fillSelectionWithAlpha(unsigned char alpha) {
  if (state.locked) return;
  for (int i = 0; i < width * height; i++) if (pixelStates[i] & PIXELSTATE_SELECTED) {
      pixels[i] = (pixels[i] & 0x00ffffff) | (alpha << 24);
  }
  updatedPixels(this);
  shouldUpdateAll = true;
}

unsigned int invertColor2(unsigned int col) {
  int r = 255 - (col & 255);
  int g = 255 - ((col>>8) & 255);
  int b = 255 - ((col>>16) & 255);
  return r|(g<<8)|(b<<16)|(col & 0xff000000);
}

unsigned int invertAlpha2(unsigned int col) {
  int a = 255 - ((col>>24) & 255);
  return (col & 0x00ffffff)|(a<<24);
}

void Layer::invertColor() {
  if (state.locked) return;
  bool hasSel = hasSelection();
  for (int i = 0; i < width * height; i++) if ((!hasSel)||(pixelStates[i] & PIXELSTATE_SELECTED)) pixels[i] = invertColor2(pixels[i]);
  updatedPixels(this);
  shouldUpdateAll = true;
}

void Layer::invertAlpha() {
  if (state.locked) return;
  bool hasSel = hasSelection();
  for (int i = 0; i < width * height; i++) if ((!hasSel)||(pixelStates[i] & PIXELSTATE_SELECTED)) pixels[i] = invertAlpha2(pixels[i]);
  updatedPixels(this);
  shouldUpdateAll = true;
}

void Layer::expandSelection(int pixels) {
  if (state.locked) return;
  if (pixels == 0) return;
  unsigned char *before = new unsigned char[width*height];
  bool forward = pixels > 0;
  initStatusBar();
  for (int i = 0; i < abs(pixels); i++) {
    memcpy(before,pixelStates,width*height*sizeof(unsigned char));
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        unsigned char p00 = (unsigned char)(before[x+y*width] & PIXELSTATE_SELECTED);
        unsigned char pm0 = (unsigned char)(x == 0 ? 0 : before[x-1+y*width] & PIXELSTATE_SELECTED);
        unsigned char pp0 = (unsigned char)(x == width-1 ? 0 : before[x+1+y*width] & PIXELSTATE_SELECTED);
        unsigned char p0m = (unsigned char)(y == 0 ? 0 : before[x+(y-1)*width] & PIXELSTATE_SELECTED);
        unsigned char p0p = (unsigned char)(y == height-1 ? 0 : before[x+(y+1)*width] & PIXELSTATE_SELECTED);
        if (forward) {
          unsigned char k = (unsigned char)(pm0|pp0|p0m|p0p);
          if (!p00) {
            if (k) pixelStates[x+y*width] |= PIXELSTATE_SELECTED;
          }
        } else {
          unsigned char k = (unsigned char)(pm0&pp0&p0m&p0p);
          if (p00) {
            if (!k) pixelStates[x+y*width] &= ~PIXELSTATE_SELECTED;
          }
        }
      }
    }
    updateStatusBar();
  }
  delete[] before;
  shouldUpdateAll = true;
  updatedPixelStates(this);
}

void Layer::selectByAlpha(int alphaThresholdMin, int alphaThresholdMax) {
  if (state.locked) return;
  for (int i = 0; i < width * height; i++) {
    pixelStates[i] = (unsigned char)(pixelStates[i] & (~PIXELSTATE_SELECTED));
    int a = (pixels[i]>>24) & 255;
    if (a >= alphaThresholdMin && a <= alphaThresholdMax) pixelStates[i] |= PIXELSTATE_SELECTED;
  }
  updatedPixelStates(this);
  shouldUpdateAll = true;
}

void Layer::selectionAsOutline() {
  if (state.locked) return;
  int addX[4] = {-1,1,0,0};
  int addY[4] = {0,0,-1,1};
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int pos = x + y * width;
      pixelStates[pos] &= ~PIXELSTATE_TEMP;
      if (!(pixelStates[pos] & PIXELSTATE_SELECTED)) {
        bool outline = false;
        for (int i = 0; i < 4; i++) {
          int x3 = x + addX[i];
          int y3 = y + addY[i];
          int pos3 = x3 + y3 * width;
          if ((unsigned int)x3 < width && (unsigned int)y3 < height && (pixelStates[pos3] & PIXELSTATE_SELECTED)) pixelStates[pos] |= PIXELSTATE_TEMP;
        }
      }
    }
  }
  for (int i = 0; i  < width * height; i++) {
    pixelStates[i] &= ~PIXELSTATE_SELECTED;
    if (pixelStates[i] & PIXELSTATE_TEMP) pixelStates[i] |= PIXELSTATE_SELECTED;
  }
  updatedPixelStates(this);
  shouldUpdateAll = true;
}

void Layer::addSelectionRectangle(int lx0, int ly0, int lx1, int ly1) {
  if (state.locked) return;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (x >= lx0 && x < lx1 && y >= ly0 && y < ly1) {
        pixelStates[x+y*width] |= PIXELSTATE_SELECTED;
      }
    }
  }
  updatedPixelStates(this);
  shouldUpdateAll = true;
}

bool Layer::hasSelection() {
  unsigned char f = PIXELSTATE_SELECTED; unsigned int f2 = f|(f<<8)|(f<<16)|(f<<24); unsigned int *pixelStates2 = (unsigned int *)pixelStates;
  for (int i = 0; i < width * height/4; i++) if (pixelStates2[i] & f2) return true;
  for (int j = width*height/4*4; j < width*height; j++) if (pixelStates[j] & f) return true;

  return false;
}

Vector Layer::boundingBox() {
  Vector ret;
  Matrix invM = inverse2(transform.transformation);
  for (int i = 0; i < 4; i++) {
    Vector k(i & 1 ? 0 : width, i & 2 ? 0 : height, 0);
    k = invM * k;
    if (i == 0 || k.x < ret.x) ret.x = k.x;
    if (i == 0 || k.y < ret.y) ret.y = k.y;
    if (i == 0 || k.x > ret.z) ret.z = k.x;
    if (i == 0 || k.y > ret.w) ret.w = k.y;
  }
  return ret;
}

Vector Layer::selectionBounding() {
  if (!hasSelection()) return boundingBox();
  Vector ret;
  bool first = true;
  Matrix invM = inverse2(transform.transformation);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (pixelStates[x+y*width] && PIXELSTATE_SELECTED) {
        for (int i = 0; i < 4; i++) {
          Vector k(x+(i & 1 ? 1 : 0),y+(i & 2 ? 1 : 0),0);
          k = invM * k;
          if (first || k.x < ret.x) ret.x = k.x;
          if (first || k.y < ret.y) ret.y = k.y;
          if (first || k.x > ret.z) ret.z = k.x;
          if (first || k.y > ret.w) ret.w = k.y;
          first = false;
        }
      }
    }
  }
  return ret;
}

Vector Layer::boundingByAlpha(double alphaThreshold) {
  Vector ret;
  bool first = true;
  Matrix invM = inverse2(transform.transformation);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int a = pixels[x+y*width]>>24;
      if (a >= alphaThreshold) {
        for (int i = 0; i < 4; i++) {
          Vector k(x+(i & 1 ? 1 : 0),y+(i & 2 ? 1 : 0),0);
          k = invM * k;
          if (first || k.x < ret.x) ret.x = k.x;
          if (first || k.y < ret.y) ret.y = k.y;
          if (first || k.x > ret.z) ret.z = k.x;
          if (first || k.y > ret.w) ret.w = k.y;
          first = false;
        }
      }
    }
  }
  return ret;
}

void Layer::quantizeBoundingBox() {
  for (int i = 0; i < 10; i++) { // this doesn't work reliably
    Vector b = boundingBox();
    b.x = floor(b.x);
    b.y = floor(b.y);
    b.z = floor(b.z);
    b.w = floor(b.w);
    setLayerPos(b.x,b.y);
    setLayerSize(b.z-b.x,b.w-b.y);
  }
}

void layerNewContent(Layer *l, int w, int h) {
  if (l->state.locked) return;
  if (w < 1) return;
  if (h < 1) return;
  if (l->pixels != NULL) {delete[] l->pixels; l->pixels = NULL;}
  if (l->pixelStates != NULL) {delete[] l->pixelStates; l->pixelStates = NULL;}
  l->width = w;
  l->height = h;
  l->pixels = new unsigned int[w*h];
  l->pixelStates = new unsigned char[w*h];
  memset(l->pixels,0,w*h*sizeof(unsigned int));
  memset(l->pixelStates,0,w*h*sizeof(unsigned char));
  updatedPixels(l);
  updatedPixelStates(l);
}

void layerGetContent(Layer *to, Layer *from) {
  if (to->state.locked) return;
  int w = from->width;
  int h = from->height;
  layerNewContent(to,w,h);
  memcpy(to->pixels,from->pixels,w*h*sizeof(unsigned int));
  memcpy(to->pixelStates,from->pixelStates,w*h*sizeof(unsigned char));
}

void Layer::resizeLayerContent(int interpolation, int newWidth, int newHeight) {
  if (state.locked) return;
  if (newWidth <= 0) return;
  if (newHeight <= 0) return;
  TransformationState before = transform;
  transform.fresh();
  transform.scale((double)width/newWidth,(double)height/newHeight);
  bakeLayer(interpolation);
  transform = before;
}

void Layer::bakeLayer(int interpolation) {
  if (state.locked) return;

  Vector b = boundingBox();
  int x0 = (int)floor(b.x);
  int y0 = (int)floor(b.y);
  int x1 = (int)floor(b.z);
  int y1 = (int)floor(b.w);
  int newWidth = x1-x0;
  int newHeight = y1-y0;
  if (newWidth<1) return;
  if (newHeight<1) return;

  unsigned char *beforeS = new unsigned char[width*height];
  unsigned int *beforeP = new unsigned int[width*height];
  memcpy(beforeS,pixelStates,width*height*sizeof(unsigned char));
  memcpy(beforeP,pixels,width*height*sizeof(unsigned int));
  int ow = this->width;
  int oh = this->height;

  layerNewContent(this,newWidth,newHeight);

  Matrix m = transform.transformation;

  double rounding = 0.5;
  if (interpolation == INTERPOLATION_LINEAR) rounding = 0;

  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      double px = x;
      double py = y;
      double lx3 = px*m.m[0+0*4]+py*m.m[0+1*4]+m.m[0+3*4]+rounding;
      double ly3 = px*m.m[1+0*4]+py*m.m[1+1*4]+m.m[1+3*4]+rounding;
      int lx = (int)lx3;
      int ly = (int)ly3;
      if (lx == 0) lx = (int)floor(px*m.m[0+0*4]+py*m.m[0+1*4]+m.m[0+3*4]+rounding);
      if (ly == 0) ly = (int)floor(px*m.m[1+0*4]+py*m.m[1+1*4]+m.m[1+3*4]+rounding);
      if ((unsigned int)lx >= ow || (unsigned int)ly >= oh) continue;
      pixelStates[(x-x0)+(y-y0)*this->width] = beforeS[lx+ly*ow];
      switch(interpolation) {
        case INTERPOLATION_NONE: {
          pixels[(x-x0)+(y-y0)*this->width] = beforeP[lx+ly*ow];
        } break;
        case INTERPOLATION_LINEAR: {
            double fx = lx3-floor(lx3);
            double fy = ly3-floor(ly3);
            unsigned int p00 = beforeP[lx+ly*ow];
            unsigned int p10 = (lx+1) < ow ? beforeP[(lx+1)+ly*ow] : p00;
            unsigned int p01 = (ly+1) < oh ? beforeP[lx+(ly+1)*ow] : p00;
            unsigned int p11 = (ly+1) < oh && (lx+1) < ow ? beforeP[(lx+1)+(ly+1)*ow] : p00;
            double rt = ((int)(p10 & 255) - (int)(p00 & 255))*fx + (int)(p00 & 255);
            double gt = ((int)((p10>>8) & 255) - (int)((p00>>8) & 255))*fx + (int)((p00>>8) & 255);
            double bt = ((int)((p10>>16) & 255) - (int)((p00>>16) & 255))*fx + (int)((p00>>16) & 255);
            double at = ((int)((p10>>24) & 255) - (int)((p00>>24) & 255))*fx + (int)((p00>>24) & 255);
            double rb = ((int)(p11 & 255) - (int)(p01 & 255))*fx + (int)(p01 & 255);
            double gb = ((int)((p11>>8) & 255) - (int)((p01>>8) & 255))*fx + (int)((p01>>8) & 255);
            double bb = ((int)((p11>>16) & 255) - (int)((p01>>16) & 255))*fx + (int)((p01>>16) & 255);
            double ab = ((int)((p11>>24) & 255) - (int)((p01>>24) & 255))*fx + (int)((p01>>24) & 255);
            int r = floor((rb - rt) * fy + rt);
            int g = floor((gb - gt) * fy + gt);
            int b = floor((bb - bt) * fy + bt);
            int a = floor((ab - at) * fy + at);
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            if (a > 255) a = 255;
            if (r < 0) r = 0;
            if (g < 0) g = 0;
            if (b < 0) b = 0;
            if (a < 0) a = 0;
            pixels[(x-x0)+(y-y0)*this->width] = (r)|(g<<8)|(b<<16)|(a<<24);
        } break;
      }
    }
  }

  transform.fresh();
  transform.translate(-x0,-y0);
  transform.update();

  switch(interpolation) {
    case INTERPOLATION_INTERAREA: {
      unsigned int *accumulationBuffer = new unsigned int[width*height*5]; 
      memset(accumulationBuffer,0,width*height*5*sizeof(unsigned int));
      Matrix m2 = transform.transformation * inverse2(m);
      for (int y = 0; y < oh; y++) {
        for (int x = 0; x < ow; x++) {
          double lx4 = x*m2.m[0+0*4]+y*m2.m[0+1*4]+m2.m[0+3*4]+rounding;
          double ly4 = x*m2.m[1+0*4]+y*m2.m[1+1*4]+m2.m[1+3*4]+rounding;
          int lx2 = (int)lx4;
          int ly2 = (int)ly4;
          if (lx2 == 0) lx2 = (int)floor(x*m2.m[0+0*4]+y*m2.m[0+1*4]+m2.m[0+3*4]+rounding);
          if (ly2 == 0) ly2 = (int)floor(x*m2.m[1+0*4]+y*m2.m[1+1*4]+m2.m[1+3*4]+rounding);
          if ((unsigned int)lx2 >= width || (unsigned int)ly2 >= height) continue;
          unsigned int ps = beforeP[x+y*ow];
          int r = (ps) & 255;
          int g = (ps>>8) & 255;
          int b = (ps>>16) & 255;
          int a = (ps>>24) & 255;
          int pos = (lx2 + ly2 * width)*5;
          accumulationBuffer[pos+0]+=r;
          accumulationBuffer[pos+1]+=g;
          accumulationBuffer[pos+2]+=b;
          accumulationBuffer[pos+3]+=a;
          accumulationBuffer[pos+4]+=1;
        }
      }
      Matrix m3 = inverse2(m2);
      for (int y2 = 0; y2 < height; y2++) {
        for (int x2 = 0; x2 < width; x2++) {
          int pos = (x2 + y2 * width)*5;
          int w = accumulationBuffer[pos+4];
          if (w > 0) {
            int r = accumulationBuffer[pos+0]/w;
            int g = accumulationBuffer[pos+1]/w;
            int b = accumulationBuffer[pos+2]/w;
            int a = accumulationBuffer[pos+3]/w;
            pixels[x2+y2*width] = (r)|(g<<8)|(b<<16)|(a<<24);
          } else {
            double x = x2;
            double y = y2;
            double lx4 = x*m3.m[0+0*4]+y*m3.m[0+1*4]+m3.m[0+3*4]+rounding;
            double ly4 = x*m3.m[1+0*4]+y*m3.m[1+1*4]+m3.m[1+3*4]+rounding;
            int lx2 = (int)lx4;
            int ly2 = (int)ly4;
            if (lx2 == 0) lx2 = (int)floor(x*m3.m[0+0*4]+y*m3.m[0+1*4]+m3.m[0+3*4]+rounding);
            if (ly2 == 0) ly2 = (int)floor(x*m3.m[1+0*4]+y*m3.m[1+1*4]+m3.m[1+3*4]+rounding);
            unsigned int ps = 0;
            if ((unsigned int)lx2 < ow && (unsigned int)ly2 < oh) ps = beforeP[lx2+ly2*ow];
            pixels[x2+y2*width] = ps;
          }
        }
      }
      delete[] accumulationBuffer;
    } break;
  }

  delete[] beforeP;
  delete[] beforeS;
  updatedPixels(this);
  updatedPixelStates(this);
}

Layer *Layer::clone() {
  Layer *ret = new Layer();
  unsigned int id = ret->id;
  *ret = *this;
  ret->id = id;
  ret->pixels = NULL;
  ret->pixelStates = NULL;
  ret->width = 0;
  ret->height = 0;
  ret->setContentSize(width,height);
  memcpy(ret->pixels,pixels,ret->width*ret->height*sizeof(unsigned int));
  return ret;
}

void Layer::convertToPremul() {
  if (mode != LAYERMODE_ALPHA) return;
  for (int i = 0; i < width * height; i++) {
    unsigned int col = pixels[i];
    int r = col & 255;
    int g = (col>>8) & 255;
    int b = (col>>16) & 255;
    int a = (col>>24) & 255;
    r = r * a / 255;
    g = g * a / 255;
    b = b * a / 255;
    pixels[i] = r|(g<<8)|(b<<16)|(a<<24);
  }
  mode = LAYERMODE_PREMULTIPLIEDALPHA;
}

void Layer::setLayerPos(double x, double y) {
  for (int i = 0; i < 5; i++) { // maybe it's not perfect?
    Vector b = boundingBox();
    double moveX = x-b.x;
    double moveY = y-b.y;
    Vector k2 = Vector(moveX,moveY,0);
    transform.translate(-k2.x,-k2.y);
    transform.update();
  }
}

void Layer::setLayerSize(double width, double height) {
  if (width == 0 || height == 0) return;
  for (int i = 0; i < 5; i++) { // maybe it's not perfect?
    Vector b = boundingBox();
    double oldWidth = b.z-b.x;
    double oldHeight = b.w-b.y;
    if (oldWidth == 0 || oldHeight == 0) return;
    double scaleX = width / oldWidth;
    double scaleY = height / oldHeight;
    if (scaleX == 0 || scaleY == 0) return;
    Matrix modify;modify.scale3(Vector(1.0/scaleX,1.0/scaleY,1));
  
    //Vector c((double)width/2.0*0,(double)height/2.0*0,0);
    //c = inverse2(transform.transformation)*c;
    Vector c = b.xy();
    Matrix center; center.translate(Vector(c.x,c.y,0));
    transform.transformation = transform.transformation * center * modify * inverse2(center);
    transform.update();
    shouldUpdateAll = true;
  }
}

double Layer::getAngleDegrees() {
  Vector c((double)width/2.0,(double)height/2.0,0);
  Vector n((double)width/2.0,(double)height/2.0*0,0);
  c = inverse2(transform.transformation)*c;
  n = inverse2(transform.transformation)*n;
  c -= n;
  if (c.x==0&&c.y==0) return 0;
  return fmod((atan2(c.y,c.x)*180.0/PI+360-90),360.0);
}

void Layer::setAngleDegrees(double degrees) {
  if (degrees < 0 || degrees > 360.0) return;
  double currentAngle = getAngleDegrees();
  double modify = degrees - currentAngle;
  rotateByDegrees(modify);
}

void Layer::snapToGrid(class Canvas *canvas) {
  if (canvas->gridStepX == 0 || canvas->gridStepY == 0) return;
  Vector k = boundingBox();
  int px = floor(k.x);
  int py = floor(k.y);
  int mx2 = px - canvas->gridOffsetX;
  int my2 = py - canvas->gridOffsetY;
  int mx = mx2;
  int my = my2;
  mx += canvas->gridStepX/2;
  my += canvas->gridStepY/2;
  mx /= canvas->gridStepX;
  my /= canvas->gridStepY;
  if (mx2 < 0) mx -= 1;
  if (my2 < 0) my -= 1;
  mx *= canvas->gridStepX;
  my *= canvas->gridStepY;
  mx += canvas->gridOffsetX;
  my += canvas->gridOffsetY;
  mx = mx - px;
  my = my - py;
  setLayerPos(px+mx,py+my);
  shouldUpdateAll = true;
}

void Layer::enlargenLayerBy(int left, int top, int right, int bottom) {
  if (state.locked) return;

  int newWidth = width + left + right;
  int newHeight = height + top + bottom;
  if (newWidth < 1 || newHeight < 1) return;
  unsigned int *pixels2 = new unsigned int[newWidth*newHeight];
  unsigned char *pixelStates2 = new unsigned char[newWidth*newHeight];

  for (int y = 0; y < newHeight; y++) {
    for (int x = 0; x < newWidth; x++) {
      int sx = x - left;
      int sy = y - top;
      unsigned int col = 0x00000000;
      unsigned char state = 0;
      if ((unsigned int)sx < width && (unsigned int)sy < height) {
        col = pixels[sx+sy*width];
        state = pixelStates[sx+sy*width];
      }
      pixels2[x+y*newWidth] = col;
      pixelStates2[x+y*newWidth] = state;
    }
  }

  if (pixels != NULL) {delete[] pixels; pixels = NULL;};
  if (pixelStates != NULL) {delete[] pixelStates; pixelStates = NULL;};
  pixels = pixels2;
  pixelStates = pixelStates2;
  width = newWidth;
  height = newHeight;
  Matrix modify; modify.translate(Vector(left,top,0));
  transform.transformation = modify * transform.transformation;
  fixTransform(transform);
  transform.update();
  shouldUpdateAll = true;
  updatedPixels(this);
  updatedPixelStates(this);
}

void Layer::cropByAlpha(double alphaThreshold) {
  if (state.locked) return;
  bool first = true;
  int minX = 0, minY = 0, maxX = 0, maxY = 0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int a = pixels[x+y*width]>>24;
      if (a >= alphaThreshold) {
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
  enlargenLayerBy(-minX,-minY,-(width-maxX),-(height-maxY));
  shouldUpdateAll = true;
}

void Layer::antialiasSelected() {
  if (state.locked) return;
  doFXAA(pixels,width,height,pixelStates,PIXELSTATE_SELECTED);
  updatedPixels(this);
}

void Layer::greyScale() {
  if (state.locked) return;
  bool hasSel = hasSelection();
  for (int i = 0; i < width*height; i++) {
    if ((!hasSel)||(pixelStates[i] & PIXELSTATE_SELECTED)) {
      unsigned int col = pixels[i];
      int r = col & 255;
      int g = (col>>8) & 255;
      int b = (col>>16) & 255;
      int a = (col>>24) & 255;
      int grey = (r+g+b)/3;
      r = grey;
      g = grey;
      b = grey;
      pixels[i] = r|(g<<8)|(b<<16)|(a<<24);
    }
  }
  updatedPixels(this);
}

void Layer::colorFromAlpha() {
  if (state.locked) return;
  for (int i = 0; i < width*height; i++) {
    unsigned int col = pixels[i];
    int a = col>>24;
    pixels[i]=(a)|(a<<8)|(a<<16)|0xff000000;
  }
  updatedPixels(this);
  shouldUpdateAll = true;
}

void Layer::applyAlphaMask(Layer *source) {
  if (state.locked) return;
  Matrix m = source->transform.transformation * inverse2(transform.transformation);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      double lx = x * m.m[4*0+0] + y * m.m[4*1+0] + m.m[4*3+0];
      double ly = x * m.m[4*0+1] + y * m.m[4*1+1] + m.m[4*3+1];
      int lx2 = (int)lx;
      int ly2 = (int)ly;
      if (lx2 == 0) lx2 = (int)floor(lx);
      if (ly2 == 0) ly2 = (int)floor(ly);
      if ((unsigned int)lx2<source->width&&(unsigned int)ly2<source->height) {
        unsigned int col = source->pixels[lx2+ly2*source->width];
        int r = col & 255;
        int g = (col>>8) & 255;
        int b = (col>>16) & 255;
        int a = (r+g+b)/3;
        pixels[x+y*width] = (pixels[x+y*width] & 0x00ffffff) | (a << 24);
      }
    }
  }
  updatedPixels(this);
  shouldUpdateAll = true;
}

void Layer::quantize(const Array<unsigned int> &colors) {
  if (state.locked) return;
  if (colors.empty()) return;
  initStatusBar();
  for (int i = 0; i < width * height; i++) {
    unsigned int col = pixels[i];
    if (colorTransform.useColorTransform) col = colorTransform.applyColorTransform(col);
    if (colorTransform.usePosterize) col = colorTransform.applyPosterize(col);
    if (colorTransform.useThreshold) col = colorTransform.applyThreshold(col);
    int rs = col & 255;
    int gs = (col>>8) & 255;
    int bs = (col>>16) & 255;
    int as = (col>>24) & 255;
    int ld = 256*256*4;
    int found = 0;
    for (int j = 0; j < colors.size(); j++) {
      int rd = colors[j] & 255;
      int gd = (colors[j]>>8) & 255;
      int bd = (colors[j]>>16) & 255;
      int ad = (colors[j]>>24) & 255;
      rd -= rs;
      gd -= gs;
      bd -= bs;
      ad -= as;
      int d = rd*rd+gd*gd+bd*bd+ad*ad;
      if (d < ld) {found = j; ld = d;}
    }
    pixels[i] = colors[found];
    if ((i & 65535) == 0) updateStatusBar();
  }
  updatedPixels(this);
  colorTransform.useColorTransform = false;
  colorTransform.usePosterize = false;
  colorTransform.useQuantization = false;
  shouldUpdateAll = true;
}

ColorTransform copiedColorTransform;

void Layer::copyColorTransform() {
  copiedColorTransform = colorTransform;
}

void Layer::pasteColorTransform() {
  colorTransform = copiedColorTransform;
  colorTransform.shouldUpdate = true;
  shouldUpdateAll = true;
}

unsigned int quantizingPalette[256];

__inline unsigned int quantizeColorFast(unsigned int col, float ditherStrength, int x, int y) {

  if (shouldRecreateQuantization) {
    reCreateQuantization();
    int k = palette.size();
    if (k > 256) k = 256;
    for (int i = 0; i < k; i++) {
      quantizingPalette[i] = getUIntColor(palette[i]);
    }
  }
  shouldRecreateQuantization = false;

  if (quantization.size() != 64*64*64) return col;
  int r = col & 255;
  int g = (col>>8) & 255;
  int b = (col>>16) & 255;
  if (ditherStrength > 0 && ((x+y)&1)) {
    float lBefore = (float)(r+g+b)/255.f/3.f;
    if (lBefore != 0) {
      float scale = (lBefore+ditherStrength)/lBefore;
      r *= scale;
      g *= scale;
      b *= scale;
      if (r > 255) r = 255;
      if (g > 255) g = 255;
      if (b > 255) b = 255;
    }
  }
  r >>= 2;
  g >>= 2;
  b >>= 2;
  unsigned short q = quantization[r+g*64+b*64*64];
  int pal = q & 255;
  bool checkEdges = q & (0x1<<8);
  if (!checkEdges) {
    col = quantizingPalette[pal];
  } else {
    int ld = 256*256*5;
    for (int k = 0; k < 8; k++) {
      unsigned int col2 = quantizingPalette[quantization[(r+(k&1))+(g+(k&2)/2)*64+(b+(k&4)/4)*64*64] & 255];
      int rd = (int)(col2 & 255)-r;
      int gd = (int)((col2>>8) & 255)-g;
      int bd = (int)((col2>>16) & 255)-b;
      int d = rd*rd+gd*gd+bd*bd;
      if (d < ld) {ld = d; col = col2;}
    }
  }
  return col;
}

unsigned int quantizeFast(unsigned int col, float ditherStrength, int x, int y) {
  return quantizeColorFast(col, ditherStrength, x, y);
}

void Layer::bakeMagic(bool onlyInSelection) {
  if (state.locked) return;
  bool quantize = colorTransform.useQuantization && (quantization.size() == 64*64*64);
  for (int i = 0; i < width*height; i++) {
    if (onlyInSelection && (!(pixelStates[i] & PIXELSTATE_SELECTED))) continue;
    unsigned int col = pixels[i];
    if (colorTransform.useColorTransform) col = colorTransform.applyColorTransform(col);
    if (colorTransform.usePosterize) col = colorTransform.applyPosterize(col);
    if (colorTransform.useThreshold) col = colorTransform.applyThreshold(col);
    if (quantize) col = quantizeColorFast(col, colorTransform.ditherStrength, i % width, i / width);
    pixels[i] = col;
  }
  shouldUpdateAll = true;
  updatedPixels(this);
}


void Layer::addSelectionFromLayer(Layer *source) {
  if (state.locked) return;
  if (source == NULL || (!source->hasSelection())) return;
  Layer *s = source;
  Layer *d = this;
  Matrix m = s->transform.transformation * inverse2(d->transform.transformation);
  for (int y = 0; y < d->height; y++) {
    for (int x = 0; x < d->width; x++) {
      unsigned int ps = d->pixelStates[x+y*d->width];
      double lx = x * m.m[4*0+0] + y * m.m[4*1+0] + m.m[4*3+0];
      double ly = x * m.m[4*0+1] + y * m.m[4*1+1] + m.m[4*3+1];
      int lx2 = (int)lx;
      int ly2 = (int)ly;
      if (lx2 == 0) lx2 = (int)floor(lx);
      if (ly2 == 0) ly2 = (int)floor(ly);
      if ((unsigned int)lx2 < s->width && (unsigned int)ly2 < s->height) {
        ps |= s->pixelStates[lx2+ly2*s->width] & PIXELSTATE_SELECTED;
      }
      d->pixelStates[x+y*d->width] = (unsigned char)ps;
    }
  }
  updatedPixelStates(this);
  shouldUpdateAll = true;
}

unsigned int Layer::byteCount() {
  return undoData.byteCount() + width*height*sizeof(unsigned int) + width*height*sizeof(unsigned char) + colorTransform.byteCount() + sizeof(LayerAdditionalToolInfo);
}

bool Layer::hasUpdates() {
  if (state.locked) return false;
  return undoData.updatePixelsCount != undoData.updatePixelsCountOfLastSave;
}

void CanvasControl::setCanvasTransform(Canvas *canvas) {
  canvas->transform.fresh();
  canvas->transform.translate(scrollX,scrollY);
  canvas->transform.zoom(zoom);
  canvas->transform.aspect(1*aspectRatio*glFrameBufferHeight/glFrameBufferWidth);
  canvas->transform.pixelSize(pixelSizeX,pixelSizeY);
  canvas->transform.update();
  shouldUpdateAll = true;
}

void CanvasControl::moveByPixels(Canvas *canvas, double x, double y) {
  scrollX -= x;
  scrollY -= y;
  setCanvasTransform(canvas);
}

void CanvasControl::zoomTo(Canvas *canvas, double screenPixelCenterX, double screenPixelCenterY,double zoom) {
  if (zoom < 0.01) zoom = 0.01;
  double x0,y0; x0 = screenPixelCenterX - canvas->screenPosX; y0 = screenPixelCenterY - canvas->screenPosY; canvas->transform.transformForward(x0,y0,x0,y0);
  double x1,y1; x1 = screenPixelCenterX - canvas->screenPosX; y1 = screenPixelCenterY - canvas->screenPosY; this->zoom = zoom; setCanvasTransform(canvas); canvas->transform.transformForward(x1,y1,x1,y1);
  canvas->transform.transformBackward(x0,y0,x0,y0);                                                                                                                                                                                                                                                                                  
  canvas->transform.transformBackward(x1,y1,x1,y1);                                                                                                                                                                                                                                                                                  
  scrollX -= x1 - x0;
  scrollY -= y1 - y0;
  setCanvasTransform(canvas);
}

void CanvasControl::zoomBy(Canvas *canvas, double screenPixelCenterX, double screenPixelCenterY,double step) {
  zoomTo(canvas,screenPixelCenterX,screenPixelCenterY,zoom * step);
}

void Canvas::init(int width, int height) {
  transform.clear();
  if (pixels != NULL) {delete[] pixels; pixels = NULL;}
  if (pixelStates != NULL) {delete[] pixelStates; pixelStates = NULL;}
  pixels = new unsigned int[width*height];
  if (pixels==NULL) {
    this->width = 0;
    this->height = 0;
    return;
  }
  memset(pixels,0,width*height*sizeof(unsigned int));
  pixelStates = new unsigned char[width*height];
  if (pixelStates==NULL) {
    this->width = 0;
    this->height = 0;
    return;
  }
  memset(pixelStates,0,width*height*sizeof(unsigned char));
  this->width = width;
  this->height = height;
  this->canvasX0 = 0;
  this->canvasY0 = 0;
  this->canvasX1 = width*aspectRatio*glFrameBufferHeight/glFrameBufferWidth;
  this->canvasY1 = height;
}

Canvas *Canvas::clone() {
  Canvas *ret = new Canvas();
  *ret = *this;
  ret->layers.data = new Layer*[2]; ret->layers.allocatedSize = 2; ret->layers.usedSize = 0;
  ret->overlays.data = new Overlay*[2]; ret->overlays.allocatedSize = 2; ret->overlays.usedSize = 0;
  for (int i = 0; i < layers.size(); i++) {
    ret->layers.push_back(layers[i]->clone());
  }
  for (int i2 = 0; i2 < overlays.size(); i2++) {
    Overlay *zwo = new Overlay(); *zwo = *overlays[i2]; ret->overlays.push_back(zwo);
  }
  return ret;
}

void Canvas::clearUpdateFlag() {
  unsigned char f = ~(PIXELSTATE_UPDATED|PIXELSTATE_CANVAS_UPDATED);
  unsigned int f2 = (f)|(f<<8)|(f<<16)|(f<<24);
  unsigned int *pixelStates2 = (unsigned int*)pixelStates;
  for (int i = 0; i < width * height/4; i++) pixelStates2[i] &= f2;
  for (int j = width*height/4*4; j < width*height; j++) pixelStates[j] &= f;

}

void Canvas::updatePixelStates(unsigned char _and, unsigned char _or, unsigned char _eor) {
  unsigned int and2 = _and|(_and<<8)|(_and<<16)|(_and<<24);
  unsigned int or2 = _or|(_or<<8)|(_or<<16)|(_or<<24);
  unsigned int eor2 = _eor|(_eor<<8)|(_eor<<16)|(_eor<<24);
  unsigned int *pixelStates2 = (unsigned int*)pixelStates;
  for (int i = 0; i < width*height/4; i++) {
    unsigned int a = pixelStates2[i];
    a &= and2;
    a |= or2;
    a ^= eor2;
    pixelStates2[i] = a;
  }
  for (int j = width*height/4*4; j < width*height; j++) {unsigned char a = pixelStates[j]; a &= _and; a |= _or; a ^= _eor; pixelStates[j] = a;}
}

void Canvas::getLayerPos(Layer *layer, double screenX, double screenY, double &layerX, double &layerY) {
  double x,y;
  x = (double)screenX-screenPosX;
  y = (double)screenY-screenPosY;
  transform.transformForward(x,y,x,y);
  layer->transform.transformForward(x,y,x,y);
  layerX = x;
  layerY = y;
}

void Canvas::getLayerPosFromCanvasPos(Layer *layer, double canvasX, double canvasY, double &layerX, double &layerY) {
  double x,y;
  x = (double)canvasX;
  y = (double)canvasY;
  layer->transform.transformForward(x,y,x,y);
  layerX = x;
  layerY = y;
}

void Canvas::getCanvasPosFromLayerPos(Layer *layer, double layerX, double layerY, double &canvasX, double &canvasY) {
  double x,y;
  x = (double)layerX;
  y = (double)layerY;
  layer->transform.transformBackward(x,y,x,y);
  canvasX = x;
  canvasY = y;
}

void Canvas::getCanvasPos(double screenX, double screenY, double &canvasX, double &canvasY) {
  double x,y;
  x = (double)screenX-screenPosX;
  y = (double)screenY-screenPosY;
  transform.transformForward(x,y,x,y);
  canvasX = x;
  canvasY = y;
}

void Canvas::getScreenPos(double canvasX, double canvasY, double &screenX, double &screenY) {
  double x,y;
  x = (double)canvasX;
  y = (double)canvasY;
  transform.transformBackward(x,y,x,y);
  screenX = x+screenPosX;
  screenY = y+screenPosY;
}

void Canvas::getScreenPos(Layer *layer, double layerX, double layerY, double &screenX, double &screenY) {
  double x,y;
  x = (double)layerX;
  y = (double)layerY;
  layer->transform.transformBackward(x,y,x,y);
  transform.transformBackward(x,y,x,y);
  screenX = x+screenPosX;
  screenY = y+screenPosY;
}

unsigned int Canvas::getCanvasColor(double screenX, double screenY) {
  int x = (int)floor(screenX - screenPosX);
  int y = (int)floor(screenY - screenPosY);
  if ((unsigned int)x >= width || (unsigned int)y >= height) return 0;
  return pixels[x+y*width];
}

unsigned int Canvas::getLayerColor(double screenX, double screenY, bool useColorTransform) {
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    if (l->state.active || l->state.selected) {
      double lx, ly; getLayerPos(l,screenX,screenY,lx,ly);
      int x = floor(lx), y = floor(ly);
      if ((unsigned int)x < l->width && (unsigned int)y < l->height) {
        unsigned int col = l->pixels[x+y*l->width];
        if (useColorTransform) {
          if (l->colorTransform.useColorTransform) col = l->colorTransform.applyColorTransform(col);
          if (l->colorTransform.usePosterize) col = l->colorTransform.applyPosterize(col);
          if (l->colorTransform.useThreshold) col = l->colorTransform.applyThreshold(col);
          if (l->colorTransform.useQuantization) col = quantizeColorFast(col, l->colorTransform.ditherStrength,x,y);
        }
        return col;
      }
    }
  }
  return 0;
}

void fillCanvas(Canvas *canvas, int x0, int y0, int x1, int y1, unsigned int color, unsigned char additionalPixelState) {
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > canvas->width) x1 = canvas->width;
  if (y1 > canvas->height) y1 = canvas->height;
  if (x0 >= x1) return;
  if (y0 >= y1) return;
  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      if (!(canvas->pixelStates[x+y*canvas->width] & PIXELSTATE_UPDATED)) continue;
      canvas->pixels[x+y*canvas->width] = color;
      unsigned char a = canvas->pixelStates[x+y*canvas->width];
      a = (unsigned char)(a & (~PIXELSTATE_SELECTED));
      a |= additionalPixelState & PIXELSTATE_SELECTED;
      canvas->pixelStates[x+y*canvas->width] = a;
    }
  }
}

bool dontUsePostProcessing = false;

void updateCanvas(Canvas *canvas, int x0, int y0, int x1, int y1, Layer *layer) {
  if ((!canvas->justSingle) && (!layer->state.visible)) return;
  if (canvas->justSingle && (!layer->state.active)) return;

  bool darken = !(layer->state.selected || layer->state.active);
  if (!canvas->hilightSelection) darken = false;

  if (layer->sourceLayerId != 0) {
    canvas->getContentFromOtherLayer(layer,layer->sourceLayerId);
  }

  Vector b = layer->boundingBox();
  Vector b1 = b.xy();
  Vector b2; b2.x = b.z; b2.y = b.w;
  b1 = inverse2(canvas->transform.transformation) * b1;
  b2 = inverse2(canvas->transform.transformation) * b2;
  b2.x += 1; // no better solution, yet
  b2.y += 1; // no better solution, yet
  if (b2.x < x0) return;
  if (b2.y < y0) return;
  if (b1.x > x1) return;
  if (b1.y > y1) return;
  if (x0 < b1.x) x0 = b1.x;
  if (y0 < b1.y) y0 = b1.y;
  if (x1 > b2.x) x1 = b2.x;
  if (y1 > b2.y) y1 = b2.y;

  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > canvas->width) x1 = canvas->width;
  if (y1 > canvas->height) y1 = canvas->height;
  if (x0 >= x1) return;
  if (y0 >= y1) return;
  float alpha = layer->state.alpha;
  Matrix cm = canvas->transform.transformation;
  Matrix lm = layer->transform.transformation;
  Matrix m = lm * cm;

  if (layer->colorTransform.useColorTransform) {
    layer->colorTransform.update();
  }

  if (layer->colorTransform.useQuantization) {
    if (shouldRecreateQuantization) {
      reCreateQuantization();
      int k = palette.size();
      if (k > 256) k = 256;
      for (int i = 0; i < k; i++) {
        quantizingPalette[i] = getUIntColor(palette[i]);
      }
    }
    shouldRecreateQuantization = false;
  }

  bool useQuantization = (!dontUsePostProcessing) && (layer->colorTransform.useQuantization && (quantization.size() == 64*64*64));
  bool useColorTransform = (!dontUsePostProcessing) && layer->colorTransform.useColorTransform;
  bool usePosterize = (!dontUsePostProcessing) && layer->colorTransform.usePosterize;
  bool useThreshold =  (!dontUsePostProcessing) && layer->colorTransform.useThreshold;
  int thresholdRed = (int)floor(layer->colorTransform.thresholdRed*256);
  int thresholdGreen = (int)floor(layer->colorTransform.thresholdGreen*256);
  int thresholdBlue = (int)floor(layer->colorTransform.thresholdBlue*256);
  int thresholdAlpha = (int)floor(layer->colorTransform.thresholdAlpha*256);

  bool showSelection = true;
  if (showOnlyActiveLayersSelection) {
    showSelection = layer->state.selected || layer->state.active;
  }

  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      int lx = (int)(x*m.m[0+0*4]+y*m.m[0+1*4]+m.m[0+3*4]);
      int ly = (int)(x*m.m[1+0*4]+y*m.m[1+1*4]+m.m[1+3*4]);
      if (lx == 0) lx = (int)floor(x*m.m[0+0*4]+y*m.m[0+1*4]+m.m[0+3*4]);
      if (ly == 0) ly = (int)floor(x*m.m[1+0*4]+y*m.m[1+1*4]+m.m[1+3*4]);
      if ((unsigned int)lx >= layer->width || (unsigned int)ly >= layer->height) continue;

      unsigned char a = canvas->pixelStates[x+y*canvas->width];
      if (!(a & PIXELSTATE_UPDATED)) continue;
      unsigned char sl = layer->pixelStates[lx+ly*layer->width];
      if (layer->justSelection && (!(sl & PIXELSTATE_SELECTED))) continue;
      unsigned char sl2 = sl;
      if (!showSelection) sl2 &= ~PIXELSTATE_SELECTED;
      canvas->pixelStates[x+y*canvas->width] = (unsigned char)(a | (sl2 & (PIXELSTATE_SELECTED|PIXELSTATE_OUTLINE|PIXELSTATE_GRID|PIXELSTATE_CANVAS_OVERLAY))|PIXELSTATE_CANVAS_UPDATED);

      unsigned int col = 0xffff00ff;
      unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
      unsigned int rgb1 = layer->pixels[lx+ly*layer->width];

      if (layer->type == LAYERTYPE_ADJUSTMENT) {
        rgb1 = rgb0;
      }

      if (useColorTransform) {
        if ((!layer->showColorTransformOnlyInSelection) || (sl & PIXELSTATE_SELECTED)) {
          int r = rgb1 & 255;
          int g = (rgb1>>8) & 255;
          int b = (rgb1>>16) & 255;
          int a = (rgb1>>24) & 255;
          unsigned int colr = layer->colorTransform.r[r];
          unsigned int colg = layer->colorTransform.g[g];
          unsigned int colb = layer->colorTransform.b[b];
          unsigned int cola = layer->colorTransform.a[a];
          r = (colr & 255)+(colg & 255)+(colb & 255) +(cola & 255);
          colr >>= 8; colg >>= 8; colb >>= 8; cola >>= 8;
          g = (colr & 255)+(colg & 255)+(colb & 255) +(cola & 255);
          colr >>= 8; colg >>= 8; colb >>= 8; cola >>= 8;
          b = (colr & 255)+(colg & 255)+(colb & 255) +(cola & 255);
          colr >>= 8; colg >>= 8; colb >>= 8; cola >>= 8;
          a = (colr & 255)+(colg & 255)+(colb & 255) +(cola & 255);
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          if (a > 255) a = 255;
          rgb1 = r|(g<<8)|(b<<16)|(a<<24);
        }
      }

      if (usePosterize) {
        if ((!layer->showColorTransformOnlyInSelection) || (sl & PIXELSTATE_SELECTED)) {
          int r = rgb1 & 255;
          int g = (rgb1>>8) & 255;
          int b = (rgb1>>16) & 255;
          int a = (rgb1>>24) & 255;
          if (layer->colorTransform.posterizeUseGrey) {
            r -= layer->colorTransform.posterizeGreyBase; if (layer->colorTransform.posterizeGrey>0) r = (int)(r / layer->colorTransform.posterizeGrey)*layer->colorTransform.posterizeGrey; r += layer->colorTransform.posterizeGreyBase;
            g -= layer->colorTransform.posterizeGreyBase; if (layer->colorTransform.posterizeGrey>0) g = (int)(g / layer->colorTransform.posterizeGrey)*layer->colorTransform.posterizeGrey; g += layer->colorTransform.posterizeGreyBase;
            b -= layer->colorTransform.posterizeGreyBase; if (layer->colorTransform.posterizeGrey>0) b = (int)(b / layer->colorTransform.posterizeGrey)*layer->colorTransform.posterizeGrey; b += layer->colorTransform.posterizeGreyBase;
          } else {
            r -= layer->colorTransform.posterizeRedBase; if (layer->colorTransform.posterizeRed>0) r = (int)(r / layer->colorTransform.posterizeRed)*layer->colorTransform.posterizeRed; r += layer->colorTransform.posterizeRedBase;
            g -= layer->colorTransform.posterizeGreenBase; if (layer->colorTransform.posterizeGreen>0) g = (int)(g / layer->colorTransform.posterizeGreen)*layer->colorTransform.posterizeGreen; g += layer->colorTransform.posterizeGreenBase;
            b -= layer->colorTransform.posterizeBlueBase; if (layer->colorTransform.posterizeBlue>0) b = (int)(b / layer->colorTransform.posterizeBlue)*layer->colorTransform.posterizeBlue; b += layer->colorTransform.posterizeBlueBase;
            a -= layer->colorTransform.posterizeAlphaBase; if (layer->colorTransform.posterizeAlpha>0) a = (int)(a / layer->colorTransform.posterizeAlpha)*layer->colorTransform.posterizeAlpha; a += layer->colorTransform.posterizeAlphaBase;
          }
          if (r < 0) r = 0; if (r > 255) r = 255;
          if (g < 0) g = 0; if (g > 255) g = 255;
          if (b < 0) b = 0; if (b > 255) b = 255;
          if (a < 0) a = 0; if (a > 255) a = 255;
          rgb1 = r|(g<<8)|(b<<16)|(a<<24);
        }
      }

      if (useThreshold) {
        if ((!layer->showColorTransformOnlyInSelection) || (sl & PIXELSTATE_SELECTED)) {
          int r = rgb1 & 255;
          int g = (rgb1>>8) & 255;
          int b = (rgb1>>16) & 255;
          int a = (rgb1>>24) & 255;
          int grey = (r*thresholdRed + g*thresholdGreen + b*thresholdBlue + a*thresholdAlpha)>>8;
          rgb1 = 0x00000000;
          if (grey >= layer->colorTransform.thresholdLuma) {
            rgb1 = 0xffffffff;
          }
          if (layer->colorTransform.thresholdInvert) rgb1 ^= 0xffffffff;
        }
      }

      if (useQuantization) {
        if ((!layer->showColorTransformOnlyInSelection) || (sl & PIXELSTATE_SELECTED)) {
          int r = rgb1 & 255;
          int g = (rgb1>>8) & 255;
          int b = (rgb1>>16) & 255;
          if (layer->colorTransform.ditherStrength > 0 && ((lx+ly)&1)) {
            float lBefore = (float)(r+g+b)/255.f/3.f;
            if (lBefore != 0) {
              float scale = (lBefore+layer->colorTransform.ditherStrength)/lBefore;
              r *= scale;
              g *= scale;
              b *= scale;
              if (r > 255) r = 255;
              if (g > 255) g = 255;
              if (b > 255) b = 255;
            }
          }
          r >>= 2;
          g >>= 2;
          b >>= 2;
          unsigned short q = quantization[r+g*64+b*64*64];
          int pal = q & 255;
          bool checkEdges = q & (0x1<<8);
          if (!checkEdges) {
            rgb1 = quantizingPalette[pal];
          } else {
            int ld = 256*256*5;
            for (int k = 0; k < 8; k++) {
              unsigned int col = quantizingPalette[quantization[(r+(k&1))+(g+(k&2)/2)*64+(b+(k&4)/4)*64*64] & 255];
              int rd = (int)(col & 255)-r;
              int gd = (int)((col>>8) & 255)-g;
              int bd = (int)((col>>16) & 255)-b;
              int d = rd*rd+gd*gd+bd*bd;
              if (d < ld) {ld = d; rgb1 = col;}
            }
          }
        }
      }

      switch(layer->mode) {
      case LAYERMODE_ALPHA: {
          int a = (rgb1 >> 24)*alpha;
          if (a == 0) {col = rgb0; break;}
          int r = ((int)(rgb1 & 255)-(int)(rgb0 & 255))*a/255+(int)(rgb0 & 255);
          int g = ((int)((rgb1>>8) & 255)-(int)((rgb0>>8) & 255))*a/255+(int)((rgb0>>8) & 255);
          int b = ((int)((rgb1>>16) & 255)-(int)((rgb0>>16) & 255))*a/255+(int)((rgb0>>16) & 255);
          if ((rgb0>>24)>a) a = rgb0>>24;
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_ADD: {
          int a = alpha * (int)(rgb1>>24); 
          if (a == 0) {col = rgb0; break;}
          float alpha2 = a/255.f;
          int r = (int)(rgb0 & 255)+(int)(rgb1 & 255)*alpha2;
          int g = (int)((rgb0>>8) & 255)+(int)((rgb1>>8) & 255)*alpha2;
          int b = (int)((rgb0>>16) & 255)+(int)((rgb1>>16) & 255)*alpha2;
          if (r > 255) r = 255;       
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_SUB: {
          int a = alpha * (int)(rgb1>>24); 
          if (a == 0) {col = rgb0; break;}
          float alpha2 = a/255.f;
          int r = (int)(rgb0 & 255)-(int)(rgb1 & 255)*alpha2;
          int g = (int)((rgb0>>8) & 255)-(int)((rgb1>>8) & 255)*alpha2;
          int b = (int)((rgb0>>16) & 255)-(int)((rgb1>>16) & 255)*alpha2;
          if (r < 0) r = 0;
          if (g < 0) g = 0;
          if (b < 0) b = 0;
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_MUL: {
          int r = (int)(rgb0 & 255)*(int)(rgb1 & 255);
          int g = (int)((rgb0>>8) & 255)*(int)((rgb1>>8) & 255);
          int b = (int)((rgb0>>16) & 255)*(int)((rgb1>>16) & 255);
          r /= 255;
          g /= 255;
          b /= 255;
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_MUL2X: {
          int r = (int)(rgb0 & 255)*(int)(rgb1 & 255);
          int g = (int)((rgb0>>8) & 255)*(int)((rgb1>>8) & 255);
          int b = (int)((rgb0>>16) & 255)*(int)((rgb1>>16) & 255);
          r *= 2;
          g *= 2;
          b *= 2;
          r /= 255;
          g /= 255;
          b /= 255;
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_PREMULTIPLIEDALPHA: {
          int a = (rgb1 >> 24)*alpha;
          int a2 = 255-a;
          int r = (int)(rgb0 & 255)*a2/255+(int)(rgb1 & 255)*alpha;
          int g = (int)((rgb0>>8) & 255)*a2/255+(int)((rgb1>>8) & 255)*alpha;
          int b = (int)((rgb0>>16) & 255)*a2/255+(int)((rgb1>>16) & 255)*alpha;
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          if ((rgb0>>24)>a) a = rgb0>>24;
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_MAX: {
          int r = maxi((int)(rgb0 & 255),(int)(rgb1 & 255));
          int g = maxi((int)((rgb0>>8) & 255),(int)((rgb1>>8) & 255));
          int b = maxi((int)((rgb0>>16) & 255),(int)((rgb1>>16) & 255));
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_MIN: {
          int r = mini((int)(rgb0 & 255),(int)(rgb1 & 255));
          int g = mini((int)((rgb0>>8) & 255),(int)((rgb1>>8) & 255));
          int b = mini((int)((rgb0>>16) & 255),(int)((rgb1>>16) & 255));
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_SCREEN: {
          int a = alpha * (int)(rgb1>>24);
          if (a == 0) {col = rgb0; break;}
           float alpha2 = a/255.f;
          int r = 255-(255-(int)(rgb0 & 255))*(255-(int)(rgb1 & 255))/255;
          int g = 255-(255-(int)((rgb0>>8) & 255))*(255-(int)((rgb1>>8) & 255))/255;
          int b = 255-(255-(int)((rgb0>>16) & 255))*(255-(int)((rgb1>>16) & 255))/255;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_COLOR: {
          int rd = (rgb0&255);
          int gd = ((rgb0>>8)&255);
          int bd = ((rgb0>>16)&255);
          int rs = (rgb1&255);
          int gs = ((rgb1>>8)&255);
          int bs = ((rgb1>>16)&255);
          int lumas = rs+gs+bs;
          int lumad = rd+gd+bd;
          bool zero = lumas == 0; // source has no color/black
          int r = zero ? 0 : lumad*rs/lumas;
          int g = zero ? 0 : lumad*gs/lumas;
          int b = zero ? 0 : lumad*bs/lumas;
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          a = (rgb0>>24) & 255; // don't modify alpha
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_LUMINANCE: {
          int rd = (rgb0&255);
          int gd = ((rgb0>>8)&255);
          int bd = ((rgb0>>16)&255);
          int rs = (rgb1&255);
          int gs = ((rgb1>>8)&255);
          int bs = ((rgb1>>16)&255);
          int lumas = rs+gs+bs;
          int lumad = rd+gd+bd;
          bool zero = lumad == 0; // dest has no color/black
          int r = zero ? 0 : lumas*rd/lumad;
          int g = zero ? 0 : lumas*gd/lumad;
          int b = zero ? 0 : lumas*bd/lumad;
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          a = (rgb0>>24) & 255; // don't modify alpha
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_SOFTLIGHT: {
          int rd = (rgb0&255);
          int gd = ((rgb0>>8)&255);
          int bd = ((rgb0>>16)&255);
          int rs = (rgb1&255);
          int gs = ((rgb1>>8)&255);
          int bs = ((rgb1>>16)&255);
          int r = rs > 128 ? (255-(255-rd)*(255-(rs-128))/255) : (rd*(rs+128)/255);
          int g = gs > 128 ? (255-(255-gd)*(255-(gs-128))/255) : (gd*(gs+128)/255);
          int b = bs > 128 ? (255-(255-bd)*(255-(bs-128))/255) : (bd*(bs+128)/255);
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_HARDLIGHT: {
          int rd = (rgb0&255);
          int gd = ((rgb0>>8)&255);
          int bd = ((rgb0>>16)&255);
          int rs = (rgb1&255);
          int gs = ((rgb1>>8)&255);
          int bs = ((rgb1>>16)&255);
          int r = rs > 128 ? (255-(255-rd)*(255-2*(rs-128))/255) : (rd*(2*rs)/255);
          int g = gs > 128 ? (255-(255-gd)*(255-2*(gs-128))/255) : (gd*(2*gs)/255);
          int b = bs > 128 ? (255-(255-bd)*(255-2*(bs-128))/255) : (bd*(2*bs)/255);
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_LINEARLIGHT: {
          int rd = (rgb0&255);
          int gd = ((rgb0>>8)&255);
          int bd = ((rgb0>>16)&255);
          int rs = (rgb1&255);
          int gs = ((rgb1>>8)&255);
          int bs = ((rgb1>>16)&255);
          int r = rs > 128 ? (rd+2*(rs-128)) : (rd+2*rs-255);
          int g = gs > 128 ? (gd+2*(gs-128)) : (gd+2*gs-255);
          int b = bs > 128 ? (bd+2*(bs-128)) : (bd+2*bs-255);
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          if (r < 0) r = 0;
          if (g < 0) g = 0;
          if (b < 0) b = 0;
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_PINLIGHT: {
          int rd = (rgb0&255);
          int gd = ((rgb0>>8)&255);
          int bd = ((rgb0>>16)&255);
          int rs = (rgb1&255);
          int gs = ((rgb1>>8)&255);
          int bs = ((rgb1>>16)&255);
          int r = rs > 128 ? maxi(rd,2*(rs-128)) : mini(rd,2*rs);
          int g = gs > 128 ? maxi(gd,2*(gs-128)) : mini(gd,2*gs);
          int b = bs > 128 ? maxi(bd,2*(bs-128)) : mini(bd,2*bs);
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_DIFFERENCE: {
          int rd = (rgb0&255);
          int gd = ((rgb0>>8)&255);
          int bd = ((rgb0>>16)&255);
          int rs = (rgb1&255);
          int gs = ((rgb1>>8)&255);
          int bs = ((rgb1>>16)&255);
          rd -= rs;
          gd -= gs;
          bd -= bs;
          int r = rd < 0 ? -rd : rd;
          int g = gd < 0 ? -gd : gd;
          int b = bd < 0 ? -bd : bd;
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      }
      if (darken) {col = ((col>>3) & 0x001f1f1f)|(col & 0xff000000);}
      canvas->pixels[x+y*canvas->width] = col;
    }
  }
}

void drawTextIntoLayer(Layer *layer, int xa, int ya, const char *text, uint32_t color, bool newContentSize) {
  if (text == NULL) return;
  int width = 0; int height = 6; const char *s2 = text;
  while(*s2!=0) {
    Letter *l = getLetter(*s2);
    width += l->preAddX + l->width + l->postAddX;
    s2++; if (*s2 == 0) break; width += 1;
  }
  width += xa;
  height += ya;
  if (newContentSize) layer->setContentSize(width,height);
  const char *s = text;
  int x2 = xa;
  while(*s!=0) {
    Letter *l = getLetter(*s);
    for (int y = 0; y < l->height; y++) {
      for (int x = 0; x < l->width; x++) {
        int x3 = x2 + x + l->preAddX;
        int y3 = ya + y + l->preAddY;
        if (l->bitmap[x+y*l->width]!='0') {
          if ((unsigned int)x3<layer->width && (unsigned int)y3<layer->height) {layer->pixels[x3+y3*layer->width] = color;}
        }
      }
    }
    x2 += l->preAddX + l->width + l->postAddX + 1;
    s++;
  }
}

void updateCanvasPaintFlagByOverlay(Canvas *canvas, Overlay *overlay, Layer *source, Layer *overlayLayer) {
  if (overlay->sourceCounter != source->undoData.updatePixelsCount) {
    overlay->sourceCounter = source->undoData.updatePixelsCount;
    Vector b = overlayLayer->boundingBox();
    Vector b1 = b.xy();
    Vector b2; b2.x = b.z; b2.y = b.w;
    b1 = inverse2(canvas->transform.transformation) * b1;
    b2 = inverse2(canvas->transform.transformation) * b2;
    b2.x += 1; // no better solution, yet
    b2.y += 1; // no better solution, yet
    int x0 = b1.x;
    int y0 = b1.y;
    int x1 = b2.x;
    int y1 = b2.y;
  
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > canvas->width) x1 = canvas->width;
    if (y1 > canvas->height) y1 = canvas->height;
    if (x0 >= x1) return;
    if (y0 >= y1) return;

    for (int y = y0; y < y1; y++) {
      for (int x = x0; x < x1; x++) {
        canvas->pixelStates[x+y*canvas->width] |= PIXELSTATE_UPDATED;
      }
    }
  }
}

void drawOverlay(int x0,int y0, int x1, int y1, Canvas *canvas, Layer *source, Overlay *overlay) {

  int w = overlay->lx1-overlay->lx0;
  int h = overlay->ly1-overlay->ly0;
  switch(overlay->type) {
    case OVERLAYTYPE_NORMAL: {
        if (!canvas->showOverlays) return;
        if (source == NULL) return;
        if (w < 1) return;
        if (h < 1) return;
      } break;
    case OVERLAYTYPE_ANNOTATION: {
        if (!canvas->showAnnotations) return;
      } break;
    case OVERLAYTYPE_SEPERATINGLINEX:
    case OVERLAYTYPE_SEPERATINGLINEY: {
        if (!canvas->showLines) return;
      } break;
    case OVERLAYTYPE_CANVASSPRITE: {
        if (!canvas->showSprites) return;
      } break;
  }

  double s = canvas->control.zoom/100.0;
  if (s != 0) s = 1.0 / s;
  double sx = s;
  double sy = s;

  Layer *l = new Layer(); reverseLastLayerIdCountUp();
  switch(overlay->type) {
    case OVERLAYTYPE_NORMAL: {
      double lx = canvas->screenPosX, ly = canvas->screenPosY; canvas->getCanvasPos(lx,ly,lx,ly);
      l->init(w,h);
      l->setLayerPos(lx+overlay->cx0*sx,ly+overlay->cy0*sy);
      l->setLayerSize((overlay->cx1-overlay->cx0)*sx,(overlay->cy1-overlay->cy0)*sy);
      l->updatePixelStates(0xff,PIXELSTATE_CANVAS_OVERLAY,0);
      for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
          int sx = x + overlay->lx0;
          int sy = y + overlay->ly0;
          unsigned int col = 0x00000000;
          if ((unsigned int)sx < source->width && (unsigned int)sy < source->height) {
            col = source->pixels[sx+sy*source->width];
          }
          l->pixels[x+y*w] = col;
        }
      }
      updateCanvasPaintFlagByOverlay(canvas,overlay,source,l);
      updateCanvas(canvas, x0, y0, x1, y1, l);
    } break;
    case OVERLAYTYPE_ANNOTATION: {
      l->init(1,1);
      drawTextIntoLayer(l,1,1,overlay->annotationText,0xff000000,true);
      drawTextIntoLayer(l,0,0,overlay->annotationText,overlay->annotationColor,false);
      double cx = overlay->lx0; double cy = overlay->ly0; canvas->getCanvasPosFromLayerPos(source,cx,cy,cx,cy);
      l->setLayerPos(cx,cy);
      double px = canvas->control.pixelSizeX;
      double py = canvas->control.pixelSizeY;
      if (px != 0) px = 1.0/px;
      if (py != 0) py = 1.0/py;
      l->setLayerSize(l->width*sx*overlay->annotationFontSize*px,l->height*sy*overlay->annotationFontSize*py);
      l->updatePixelStates(0xff,PIXELSTATE_CANVAS_OVERLAY,0);
      updateCanvas(canvas, x0, y0, x1, y1, l);
    } break;
    case OVERLAYTYPE_SEPERATINGLINEX: {
      double cx = overlay->lx0; double cy = overlay->ly0; canvas->getCanvasPosFromLayerPos(source,cx,cy,cx,cy);
      canvas->getScreenPos(cx,cy,cx,cy); cx -= canvas->screenPosX; cy -= canvas->screenPosY;
      canvas->pixelLine(0,cy,canvas->width,cy,0xff00ffff);
    } break;
    case OVERLAYTYPE_SEPERATINGLINEY: {
      double cx = overlay->lx0; double cy = overlay->ly0; canvas->getCanvasPosFromLayerPos(source,cx,cy,cx,cy);
      canvas->getScreenPos(cx,cy,cx,cy); cx -= canvas->screenPosX; cy -= canvas->screenPosY;
      canvas->pixelLine(cx,0,cx,canvas->height,0xff00ffff);
    } break;
    case OVERLAYTYPE_CANVASSPRITE: {
      l->init(w,h);
      l->setLayerPos(overlay->cx0,overlay->cy0);
      for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
          int sx = x + overlay->lx0;
          int sy = y + overlay->ly0;
          unsigned int col = 0x00000000;
          if ((unsigned int)sx < source->width && (unsigned int)sy < source->height) {
            col = source->pixels[sx+sy*source->width];
          }
          l->pixels[x+y*w] = col;
        }
      }
      updateCanvasPaintFlagByOverlay(canvas,overlay,source,l);
      updateCanvas(canvas, x0, y0, x1, y1, l);
    } break;
  }
  delete l;
}

bool overlaysOn = true;

void Canvas::updateRegion(int screenX0, int screenY0, int screenX1, int screenY1) {
  int x0 = screenX0 - screenPosX;
  int y0 = screenY0 - screenPosY;
  int x1 = screenX1 - screenPosX;
  int y1 = screenY1 - screenPosY;
  fillCanvas(this,x0,y0,x1,y1,0x00000000,0);
  for (int i = layers.size()-1; i >= 0; i--) {
    updateCanvas(this,x0,y0,x1,y1,layers[i]);
  }
  if (layerShapesOn) {
    for (int i = layers.size()-1; i >= 0; i--) {
      Layer *l = layers[i];
      int k[4] = {0,1,3,2};
      for (int j = 0; j < 4; j++) {
        int j0 = k[j];
        int j1 = k[(j+1)&3];
        Vector l0((j0 & 1) ? l->width : 0, (j0 & 2) ? l->height : 0);
        Vector l1((j1 & 1) ? l->width : 0, (j1 & 2) ? l->height : 0);
        getCanvasPosFromLayerPos(l,l0.x,l0.y,l0.x,l0.y);
        getCanvasPosFromLayerPos(l,l1.x,l1.y,l1.x,l1.y);
        getScreenPos(l0.x,l0.y,l0.x,l0.y);
        getScreenPos(l1.x,l1.y,l1.x,l1.y);
        l0.x -= screenPosX;
        l0.y -= screenPosY;
        l1.x -= screenPosX;
        l1.y -= screenPosY;
        pixelLine(l0.x,l0.y,l1.x,l1.y,0xff00ffff);
      }
    }
  }
  if (overlaysOn) {
    for (int i2 = overlays.size()-1; i2 >= 0; i2--) {
      Overlay *o = overlays[i2];
      if (o->visible) {
        Layer *l = NULL;
        for (int i3 = 0; i3 < layers.size(); i3++) {
          if (layers[i3]->id == o->layerId) {
            l = layers[i3];
            break;
          }
        }
        if (l != NULL) {
          drawOverlay(x0,y0,x1,y1,this,l,o);
        }
      }
    } 
  }
}

void Canvas::update(bool forceUpdate) {
  if (forceUpdate) {
    unsigned char f = PIXELSTATE_UPDATED|PIXELSTATE_CANVAS_UPDATED;
    unsigned int f2 = f|(f<<8)|(f<<16)|(f<<24);
    unsigned int *pixelStates2 = (unsigned int*)pixelStates;
    for (int i = 0; i < width*height/4; i++) pixelStates2[i] |= f2;
    for (int j = width*height/4*4; j < width*height; j++) pixelStates[j] |= f;
  }
  updateRegion(screenPosX,screenPosY,screenPosX+width,screenPosY+height);
}

void Canvas::updateLayer(Layer *layer, bool forceUpdate) {
  int x0 = 0;
  int y0 = 0;
  int x1 = width;
  int y1 = height;
  Vector b = layer->boundingBox();
  Vector b1 = b.xy();
  Vector b2; b2.x = b.z; b2.y = b.w;
  b1 = inverse2(transform.transformation) * b1;
  b2 = inverse2(transform.transformation) * b2;
  b2.x += 1;
  b2.y += 1;
  if (b2.x < x0) return;
  if (b2.y < y0) return;
  if (b1.x > x1) return;
  if (b1.y > y1) return;
  if (x0 < b1.x) x0 = b1.x;
  if (y0 < b1.y) y0 = b1.y;
  if (x1 > b2.x) x1 = b2.x;
  if (y1 > b2.y) y1 = b2.y;
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > width) x1 = width;
  if (y1 > height) y1 = height;
  if (x0 >= x1) return;
  if (y0 >= y1) return;
  if (forceUpdate) {
    for (int y = y0; y < y1; y++) {
      for (int x = x0; x < x1; x++) {
        pixelStates[x+y*width] |= PIXELSTATE_UPDATED|PIXELSTATE_CANVAS_UPDATED;
      }
    }
  }
  updateRegion(x0+screenPosX,y0+screenPosY,x1+screenPosX,y1+screenPosY);
}

void Canvas::addLayer(Layer *layer) {
  if (layer == NULL) return;
  layers.push_back(layer);
  shouldUpdateAll = true;
}

void Canvas::insertLayer(Layer *layer, int position) {
  if (layer == NULL) return;
  if (position < 0) position = 0;
  if (position > layers.size()) position = layers.size();
  layers.insert(layer,position);
  shouldUpdateAll = true;
}

void Canvas::eraseLayer(Layer *layer) {
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i] == layer) {
      layers.erase(i,1);
      i--;
      shouldUpdateAll = true;
    }
  }
}

void Canvas::insertTextLayer() {
  int pos = 0;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected || layers[i]->state.active) {
      pos = i;
      break;
    }
  }
  Layer *l = new Layer();
  l->invalidateContent();
  l->mode = LAYERMODE_ALPHA;
  l->type = LAYERTYPE_TEXT;
  l->state.locked = true;
  l->state.permanentlyLocked = true;
  sprintf(l->name,"text");
  insertLayer(l,pos);
  setActiveLayer(pos);
}

void Canvas::insertAdjustmentLayer() {
  int w = canvasX1-canvasX0;
  int h = canvasY1-canvasY0;
  if (w > 0 && h > 0) {
    int pos = 0;
    for (int i = 0; i < layers.size(); i++) {
      if (layers[i]->state.selected || layers[i]->state.active) {
        pos = i;
        break;
      }
    }
    Layer *l = new Layer();
    l->init(w,h);
    l->setLayerPos(canvasX0,canvasY0);
    l->type = LAYERTYPE_ADJUSTMENT;
    sprintf(l->name,"adjustment");
    insertLayer(l,pos);
    setActiveLayer(pos);
  }
}

extern double canvasScrollX;
extern double canvasScrollY;

void applyFlip(Layer *l, const Matrix &scale, const Matrix &center, const Matrix &screenToCanvas) {
  //l->transform.transformation = l->transform.transformation * center * scale * inverse2(center) * inverse2(l->transform.transformation) * l->transform.transformation;
  l->transform.transformation = l->transform.transformation * screenToCanvas * center * scale * inverse2(center) * inverse2(screenToCanvas) * inverse2(l->transform.transformation) * l->transform.transformation;
  l->transform.update();
}

void Canvas::mirrorHorizontally() {
  Matrix screenToCanvas = transform.transformation; // from screen coordinates to canvas space
  Vector ltCanvas = inverse2(screenToCanvas) * Vector(canvasX0,canvasY0,0);
  Vector rbCanvas = inverse2(screenToCanvas) * Vector(canvasX1,canvasY1,0);
  Matrix center; center.translate(Vector((ltCanvas.x+rbCanvas.x)*0.5,0,0));
  Matrix scale; scale.scale3(Vector(-1,1,1));
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    //Matrix centerInLayerSpace = screenToCanvas * center;
    applyFlip(l,scale,center,screenToCanvas);  
  }
  shouldUpdateAll = true;
}

void Canvas::mirrorVertically() {
  Matrix screenToCanvas = transform.transformation; // from screen coordinates to canvas space
  Vector ltCanvas = inverse2(screenToCanvas) * Vector(canvasX0,canvasY0,0);
  Vector rbCanvas = inverse2(screenToCanvas) * Vector(canvasX1,canvasY1,0);
  Matrix center; center.translate(Vector(0,(ltCanvas.y+rbCanvas.y)*0.5,0));
  Matrix scale; scale.scale3(Vector(1,-1,1));
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    //Matrix centerInLayerSpace = screenToCanvas * center;
    applyFlip(l,scale,center,screenToCanvas);  
  }
  shouldUpdateAll = true;
}

void Canvas::rotate90DegreesForward() {
  Matrix screenToCanvas = transform.transformation; // from screen coordinates to canvas space
  Vector ltCanvas = inverse2(screenToCanvas) * Vector(canvasX0,canvasY0,0);
  Vector rbCanvas = inverse2(screenToCanvas) * Vector(canvasX1,canvasY1,0);
  Matrix center; center.translate(Vector((ltCanvas.x+rbCanvas.x)*0.5,(ltCanvas.y+rbCanvas.y)*0.5,0));
  Matrix rotate = rotationMatrixAngleAxis(PI*-0.5,0,0,1);
  rotate = straighten(rotate);
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    Matrix centerInLayerSpace; centerInLayerSpace.translate(screenToCanvas * center * Vector(0,0,0));
    l->transform.transformation = l->transform.transformation * centerInLayerSpace * rotate * inverse2(centerInLayerSpace);
    l->transform.update();
  }

  double cWidth = canvasX1 - canvasX0;
  double cHeight = canvasY1 - canvasY0;
  double cMX = (canvasX1 + canvasX0) * 0.5;
  double cMY = (canvasY1 + canvasY0) * 0.5;
  canvasX0 = cMX - cHeight * 0.5;
  canvasX1 = cMX + cHeight * 0.5;
  canvasY0 = cMY - cWidth * 0.5;
  canvasY1 = cMY + cWidth * 0.5;

  shouldUpdateAll = true;
}

void Canvas::rotate90DegreesBackward() {
  Matrix screenToCanvas = transform.transformation; // from screen coordinates to canvas space
  Vector ltCanvas = inverse2(screenToCanvas) * Vector(canvasX0,canvasY0,0);
  Vector rbCanvas = inverse2(screenToCanvas) * Vector(canvasX1,canvasY1,0);
  Matrix center; center.translate(Vector((ltCanvas.x+rbCanvas.x)*0.5,(ltCanvas.y+rbCanvas.y)*0.5,0));
  Matrix rotate = rotationMatrixAngleAxis(PI*0.5,0,0,1);
  rotate = straighten(rotate);
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    Matrix centerInLayerSpace; centerInLayerSpace.translate(screenToCanvas * center * Vector(0,0,0));
    l->transform.transformation = l->transform.transformation * centerInLayerSpace * rotate * inverse2(centerInLayerSpace);
    l->transform.update();
  }

  double cWidth = canvasX1 - canvasX0;
  double cHeight = canvasY1 - canvasY0;
  double cMX = (canvasX1 + canvasX0) * 0.5;
  double cMY = (canvasY1 + canvasY0) * 0.5;
  canvasX0 = cMX - cHeight * 0.5;
  canvasX1 = cMX + cHeight * 0.5;
  canvasY0 = cMY - cWidth * 0.5;
  canvasY1 = cMY + cWidth * 0.5;

  shouldUpdateAll = true;
}

void Canvas::mergeDownLayers() {
  int lastPos = 0;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected || layers[i]->state.active) {
      lastPos = i;
    }
  }
  if ((unsigned int)lastPos >= layers.size()) return;
  Layer *l = getSelectionAsLayer(false,false,false);
  if (l == NULL) return;
  layerGetContent(layers[lastPos],l); layers[lastPos]->transform = l->transform; layers[lastPos]->colorTransform = l->colorTransform;
  setActiveLayer(lastPos);
  delete l;
  for (int i2 = 0; i2 < layers.size(); i2++) {
    if (layers[i2]->state.selected || layers[i2]->state.active) {
      if (i2 != lastPos) {
        if (i2 < lastPos) lastPos--;
        delete layers[i2];
        layers.erase(i2,1);
        i2--;
      }
    }
  }
  shouldUpdateAll = true;
}

void Canvas::insertMergeOfLayers() {
  int pos = 0;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected || layers[i]->state.active) {
      pos = i;
      break;
    }
  }
  Layer *l = getSelectionAsLayer(false,false,false);
  if (l == NULL) return;
  sprintf(l->name,"merged");
  shouldUpdateAll = true;
  layers.insert(l,pos);
  setActiveLayer(pos);
}

void Canvas::insertAlphaMasks() {
  int pos = 0;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected || layers[i]->state.active) {
      if (pos == 0) pos = i;
      Layer *l = layers[i]->clone();
      sprintf(l->name,"alpha mask");
      l->colorFromAlpha();
      layers.insert(l,i);
      i++;
    }
  }
  noSelectedLayers();
  setActiveLayer(pos);
}

void Canvas::applyAlphaMaskToBelow() {
  int pos0 = -1;
  int pos1 = -1;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected || layers[i]->state.active) {
      if (pos0 < 0) pos0 = i; else
      if (pos1 < 0) pos1 = i; else return;
    }
  }
  if (pos0<0||pos1<0) return;
  Layer *alphaMask = layers[pos0];
  Layer *dest = layers[pos1];
  dest->applyAlphaMask(alphaMask);
  delete layers[pos0];
  layers.erase(pos0,1);
}

void Canvas::quantizeLayers() {
  Array<unsigned int> palette2;
  palette2.resize(palette.size());
  for (int j = 0; j < palette2.size(); j++) palette2[j] = getUIntColor(palette[j]);
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected || layers[i]->state.active) {
      layers[i]->quantize(palette2);
    }
  }
}

void Canvas::convertFromAlphaToPremul() {
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected || layers[i]->state.active) {
      layers[i]->mode = LAYERMODE_ALPHA;
      layers[i]->convertToPremul();
    }
  }
  shouldUpdateAll = true;
}

void Canvas::resizeCanvas(double w, double h, bool quantizeLayers, int interpolation) {
  double w2 = canvasX1 - canvasX0;
  double h2 = canvasY1 - canvasY0;
  if (w == 0) w = 1;
  if (h == 0) h = 1;
  double scaleX = w2 / w;
  double scaleY = h2 / h;
  Matrix m; m.scale3(Vector(scaleX,scaleY,1));
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    if (quantizeLayers) l->bakeLayer(interpolation);
    l->transform.transformation = l->transform.transformation * m;
    if (quantizeLayers) {
      int wBefore = l->width;
      int hBefore = l->height;
      l->resizeLayerContent(interpolation,wBefore/scaleX,hBefore/scaleY);
      int wAfter = l->width;
      int hAfter = l->height;
      if (wBefore == 0) wBefore = 1;
      if (hBefore == 0) hBefore = 1;
      double scaleX2 = (double)wAfter/wBefore;
      double scaleY2 = (double)hAfter/hBefore;
      Matrix m2; m2.scale3(Vector(scaleX2,scaleY2,1));
      l->transform.transformation = m2 * l->transform.transformation;
    }
    l->transform.update();
  }
  canvasX1 = canvasX0 + w;
  canvasY1 = canvasY0 + h;
  shouldUpdateAll = true;
}

void Canvas::fitCanvasToLayers() {
  if (layers.empty()) return;
  Vector boundingBox;
  for (int i3 = 0; i3 < layers.size(); i3++) {
    Vector here = layers[i3]->boundingBox();
    if (i3 == 0 || here.x < boundingBox.x) boundingBox.x = here.x;
    if (i3 == 0 || here.y < boundingBox.y) boundingBox.y = here.y;
    if (i3 == 0 || here.z > boundingBox.z) boundingBox.z = here.z;
    if (i3 == 0 || here.w > boundingBox.w) boundingBox.w = here.w;
  }
  double x0 = floor(boundingBox.x); // the inverse is too bad here
  double y0 = floor(boundingBox.y);
  double x1 = floor(boundingBox.z);
  double y1 = floor(boundingBox.w);
  if (floor(x1)-floor(x0)<1) x1 = x0 + 1;
  if (floor(y1)-floor(y0)<1) y1 = y0 + 1;
  canvasX0 = x0;
  canvasY0 = y0;
  canvasX1 = x1;
  canvasY1 = y1;
  shouldUpdateAll = true;
}

void Canvas::fitCanvasToSelection() {
  if (layers.empty()) return;
  Vector boundingBox;
  for (int i3 = 0; i3 < layers.size(); i3++) {
    if (layers[i3]->state.active || layers[i3]->state.selected) {
      if (layers[i3]->hasSelection()) {
        Vector here = layers[i3]->selectionBounding();
        if (i3 == 0 || here.x < boundingBox.x) boundingBox.x = here.x;
        if (i3 == 0 || here.y < boundingBox.y) boundingBox.y = here.y;
        if (i3 == 0 || here.z > boundingBox.z) boundingBox.z = here.z;
        if (i3 == 0 || here.w > boundingBox.w) boundingBox.w = here.w;
      }
    }
  }
  double x0 = floor(boundingBox.x); // the inverse is too bad here
  double y0 = floor(boundingBox.y);
  double x1 = floor(boundingBox.z);
  double y1 = floor(boundingBox.w);
  if (floor(x1)-floor(x0)<1) x1 = x0 + 1;
  if (floor(y1)-floor(y0)<1) y1 = y0 + 1;
  canvasX0 = x0;
  canvasY0 = y0;
  canvasX1 = x1;
  canvasY1 = y1;
  shouldUpdateAll = true;
}

void Canvas::cropCanvasByAlpha(double alpha) {
  if (layers.empty()) return;
  Vector boundingBox;
  for (int i3 = 0; i3 < layers.size(); i3++) {
    Vector here = layers[i3]->boundingByAlpha(alpha);
    if (i3 == 0 || here.x < boundingBox.x) boundingBox.x = here.x;
    if (i3 == 0 || here.y < boundingBox.y) boundingBox.y = here.y;
    if (i3 == 0 || here.z > boundingBox.z) boundingBox.z = here.z;
    if (i3 == 0 || here.w > boundingBox.w) boundingBox.w = here.w;
  }
  double x0 = floor(boundingBox.x); // the inverse is too bad here
  double y0 = floor(boundingBox.y);
  double x1 = floor(boundingBox.z);
  double y1 = floor(boundingBox.w);
  if (floor(x1)-floor(x0)<1) x1 = x0 + 1;
  if (floor(y1)-floor(y0)<1) y1 = y0 + 1;
  canvasX0 = x0;
  canvasY0 = y0;
  canvasX1 = x1;
  canvasY1 = y1;
  shouldUpdateAll = true;
}

void Canvas::mirrorHorizontallyLayers() {
  bool one = false; for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active)) l->mirrorHorizontally();
  }
}

void Canvas::mirrorVerticallyLayers() {
  bool one = false; for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active)) l->mirrorVertically();
  }
}

void Canvas::rotate90DegreesForwardLayers() {
  bool one = false; for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active)) l->rotate90DegreesForward();
  }
}

void Canvas::rotate90DegreesBackwardLayers() {
  bool one = false; for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active)) l->rotate90DegreesBackward();
  }
}

void Canvas::selectAll() {
  bool one = false; 
  bool two = false; 
  for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true; if (l->state.active) two = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active) || ((!one)&&(!two))) l->selectAll();
  }
}

void Canvas::selectNone(bool all) {
  UNUSED(all);
  //bool one = false; 
  //bool two = false; 
  //for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true; if (l->state.active) two = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    //if ((one && l->state.selected) || ((!one)&&l->state.active) || ((!one)&&(!two)) || (all)) l->selectNone();
    l->selectNone();
  }
}

void Canvas::invertSelection() {
  bool one = false; 
  bool two = false; 
  for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true; if (l->state.active) two = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active) || ((!one)&&(!two))) l->invertSelection();
  }
}

bool Canvas::hasSelection() {
  for (int i = 0; i < layers.size(); i++) {if (layers[i]->hasSelection()) return true;}
  return false;
}

void Canvas::copyActiveSelectionToLayer(int layerIndexSource,int layerIndexDest) {
  if ((unsigned int)layerIndexSource>=layers.size()) return;
  if ((unsigned int)layerIndexDest>=layers.size()) return;
  Layer *s = layers[layerIndexSource];
  Layer *d = layers[layerIndexDest];
  if (d->state.locked) return;
  Matrix m = s->transform.transformation * inverse2(d->transform.transformation);
  for (int y = 0; y < d->height; y++) {
    for (int x = 0; x < d->width; x++) {
      unsigned int ps = d->pixelStates[x+y*d->width];
      ps &= ~PIXELSTATE_SELECTED;
      double lx = x * m.m[4*0+0] + y * m.m[4*1+0] + m.m[4*3+0];
      double ly = x * m.m[4*0+1] + y * m.m[4*1+1] + m.m[4*3+1];
      int lx2 = (int)lx;
      int ly2 = (int)ly;
      if (lx2 == 0) lx2 = (int)floor(lx);
      if (ly2 == 0) ly2 = (int)floor(ly);
      if ((unsigned int)lx2 < s->width && (unsigned int)ly2 < s->height) {
        ps |= s->pixelStates[lx2+ly2*s->width] & PIXELSTATE_SELECTED;
      }
      d->pixelStates[x+y*d->width] = (unsigned char)ps;
    }
  }
  updatedPixelStates(d);
  shouldUpdateAll = true;
}

void Canvas::selectByAlpha(int alphaThresholdMin, int alphaThresholdMax) {
  bool one = false; 
  bool two = false; 
  for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true; if (l->state.active) two = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active) || ((!one)&&(!two))) l->selectByAlpha(alphaThresholdMin, alphaThresholdMax);
  }
}

void Canvas::selectionAsOutline() {
  bool one = false; 
  bool two = false; 
  for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true; if (l->state.active) two = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active) || ((!one)&&(!two))) l->selectionAsOutline();
  }
}

void Canvas::selectAllLayers() {
  for (int i = 0; i < layers.size(); i++) {layers[i]->state.selected = true;layers[i]->state.active = false;}
  shouldUpdateAll = true;
}

void Canvas::showAllLayers(bool show) {
  for (int i = 0; i < layers.size(); i++) {layers[i]->state.visible = show;}
  shouldUpdateAll = true;
}

unsigned int Canvas::moveLayersUp() {
  unsigned int ret = 0;
  if ((!layers.empty()) && (layers[0]->state.selected || layers[0]->state.active)) return ret;
  for (int i = 1; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Layer *t = layers[i-1];
      if (ret == 0) ret = layers[i]->id;
      layers[i-1] = layers[i];
      layers[i] = t;
    }
  }
  shouldUpdateAll = true;
  return ret;
}

unsigned int Canvas::moveLayersDown() {
  unsigned int ret = 0;
  if ((!layers.empty()) && (layers.back()->state.selected || layers.back()->state.active)) return ret;
  for (int i = layers.size()-2; i >= 0; i--) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Layer *t = layers[i+1];
      ret = layers[i]->id;
      layers[i+1] = layers[i];
      layers[i] = t;
    }
  }
  shouldUpdateAll = true;
  return ret;
}

void Canvas::compactLayers() {
  int firstIndex = -1;
  for (int i2 = 0; i2 < layers.size(); i2++) {
    if (layers[i2]->state.selected||layers[i2]->state.active) {firstIndex = i2; break;}
  }
  for (int i = firstIndex+1; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      firstIndex++; 
      Layer *t = layers[i];
      layers.erase(i,1);
      layers.insert(t,firstIndex);

    }
  }
  shouldUpdateAll = true;
}

void Canvas::fillSelectionWithColor(unsigned int color) {
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    if ((l->state.selected || l->state.active) && l->hasSelection()) l->fillSelectionWithColor(color);
  }
}

void Canvas::fillSelectionWithAlpha(unsigned char alpha) {
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    if ((l->state.selected || l->state.active) && l->hasSelection()) l->fillSelectionWithAlpha(alpha);
  }
}

void Canvas::invertColor() {
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    if (l->state.selected || l->state.active) l->invertColor();
  }
}

void Canvas::invertAlpha() {
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    if (l->state.selected || l->state.active) l->invertAlpha();
  }
}

void Canvas::expandSelection(int pixels) {
  if (pixels == 0) return;
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    if ((l->state.selected || l->state.active) && l->hasSelection()) l->expandSelection(pixels);
  }
}

void Canvas::antialiasSelected() {
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    if ((l->state.selected || l->state.active) && l->hasSelection()) l->antialiasSelected();
  }
  shouldUpdateAll = true;
}

void Canvas::greyScale() {
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    if (l->state.selected || l->state.active) l->greyScale();
  }
  shouldUpdateAll = true;
}
   
void Canvas::setPixel(double x, double y, unsigned int color) {
  int xi = (int)floor(x);
  int yi = (int)floor(y);
  if ((unsigned int)xi>=width||(unsigned int)yi>=height) return;
  pixels[xi+yi*width] = color;
}

void Canvas::pixelLine(double x0, double y0,double x1, double y1,unsigned int color) {
  int deltaX = x1 - x0;
  int deltaY = y1 - y0;
  int delta = abs(deltaX);
  if (abs(deltaY)>delta) delta = abs(deltaY);
  if (delta == 0) return;
  double px = x0;
  double py = y0;
  double xa = (double)(x1 - x0)/delta;
  double ya = (double)(y1 - y0)/delta;
  for (int i = 0; i < delta+1; i++) {
    setPixel(px,py,color);
    px += xa;
    py += ya;
  }
}

Layer *layerFromCanvas(Canvas *canvas) {
  Layer *ret = NULL;
  ret = new Layer();
  int w = (int)canvas->width;
  int h = (int)canvas->height;
  if (w < 0) w = 1;
  if (h < 0) h = 1;
  ret->init(w,h);
  for (int i = 0; i < ret->width*ret->height; i++) ret->pixels[i] = canvas->pixels[i];
  ret->transform.transformation = inverse2(canvas->transform.transformation);
  ret->transform.update();
  updatedPixels(ret);
  updatedPixelStates(ret);
  return ret;
}

Layer *Canvas::getMergedLayer(const Array<Layer*> &layers,bool pixelSelection, bool keepCanvasSize) {
  Layer *ret = NULL;
  Vector boundingBox;
  for (int i3 = 0; i3 < layers.size(); i3++) {
    layers[i3]->justSelection = pixelSelection && layers[i3]->hasSelection();
    Vector here = pixelSelection ? layers[i3]->selectionBounding() : layers[i3]->boundingBox();
    if (i3 == 0 || here.x < boundingBox.x) boundingBox.x = here.x;
    if (i3 == 0 || here.y < boundingBox.y) boundingBox.y = here.y;
    if (i3 == 0 || here.z > boundingBox.z) boundingBox.z = here.z;
    if (i3 == 0 || here.w > boundingBox.w) boundingBox.w = here.w;
  }
  int x0 = (int)floor(boundingBox.x);
  int y0 = (int)floor(boundingBox.y);
  int x1 = (int)floor(boundingBox.z);
  int y1 = (int)floor(boundingBox.w);
  Canvas *copyCanvas = new Canvas();
  if (!keepCanvasSize) {
    int w = x1-x0;
    int h = y1-y0;
    if (w<1||h<1) return NULL;
    copyCanvas->init(w,h);
    copyCanvas->canvasX0 = 0;
    copyCanvas->canvasY0 = 0;
    copyCanvas->canvasX1 = w;
    copyCanvas->canvasY1 = h;
    copyCanvas->transform.fresh();
    copyCanvas->transform.translate(x0,y0);
  } else {
    int w = canvasX1-canvasX0;
    int h = canvasY1-canvasY0;
    if (w<1||h<1) return NULL;
    copyCanvas->init(w,h);
    copyCanvas->canvasX0 = 0;
    copyCanvas->canvasY0 = 0;
    copyCanvas->canvasX1 = w;
    copyCanvas->canvasY1 = h;
    copyCanvas->transform.fresh();
    copyCanvas->transform.translate(canvasX0,canvasY0);
  }
  dontUsePostProcessing = layers.size() == 1;
  copyCanvas->transform.update();
  copyCanvas->layers = layers;
  hilightSelection = false;
  overlaysOn = false;
  copyCanvas->update(true);
  overlaysOn = true;
  copyCanvas->layers.clear();
  ret = layerFromCanvas(copyCanvas);
  if (dontUsePostProcessing) {
    ret->colorTransform = layers[0]->colorTransform;
    ret->toolInfo = layers[0]->toolInfo;
  }
  dontUsePostProcessing = false;
  sprintf(ret->name,"paste");
  delete copyCanvas;
  for (int i4 = 0; i4 < layers.size(); i4++) {
    layers[i4]->justSelection = false;
  }
  return ret;
}

Layer *Canvas::getSelectionAsLayer(bool pixelSelection,bool selectAllLayers, bool keepCanvasSize) {
  Layer *l = NULL;
  bool one = false; for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true;}
  Array<Layer*> selection;
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active) || selectAllLayers) {selection.push_back(l); l->toolInfo.blink = true;}
  }
  if (selection.empty()) return l;
  l = getMergedLayer(selection,pixelSelection,keepCanvasSize);
  return l;
}


void Canvas::duplicateLayers() {
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected || layers[i]->state.active) {
      layers[i]->state.selected = false;
      layers[i]->state.active = false;
      Layer *l = layers[i]->clone();
      l->state.active = true;
      sprintf(l->name,"duplicate");
      insertLayer(l,i);
      i++;
    }
  }
  shouldUpdateAll = true;
}

void Canvas::duplicateLayerAsReference(Layer *l) {
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->id == l->id) {
      layers[i]->state.selected = false;
      Layer *l = layers[i]->clone();
      l->sourceLayerId = layers[i]->id;
      l->state.locked = true;
      l->state.permanentlyLocked = true;
      sprintf(l->name,"reference");
      insertLayer(l,i);
      i++;
      break;
    }
  }
  shouldUpdateAll = true;
}

void Canvas::insertLayerLikeThisLayer() {
  bool first = true;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected || layers[i]->state.active) {
      layers[i]->state.selected = false;
      Layer *l = layers[i]->clone();
      l->clear();
      sprintf(l->name,"empty");
      insertLayer(l,i);
      if (first) setActiveLayer(i);
      first = false;
      i++;
    }
  }
  noSelectedLayers();
  shouldUpdateAll = true;
}

void Canvas::copy() {
  if (copyLayer != NULL) {delete copyLayer; copyLayer = NULL;}
  copyLayer = getSelectionAsLayer(true,false,false);
  selectNone(true);
  noSelectedLayers();
}

void Canvas::paste() {
  if (copyLayer == NULL) return;
  bool one = false; for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true;}
  int layerPos = 0;
  for (int i2 = 0; i2 < layers.size(); i2++) {Layer *l = layers[i2]; if ((l->state.selected && one)||(l->state.active && (!one))) {layerPos = i2; break;}}
  insertLayer(copyLayer,layerPos); copyLayer = NULL;
  //insertLayer(copyLayer->clone(),layerPos); //copyLayer = NULL;
  setActiveLayer(layerPos);
  updatedPixels(getActiveLayer());
  updatedPixelStates(getActiveLayer());
}

int Canvas::getActiveLayerIndex() {
  int ret = -1;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.active) {
      ret = i;
      break;
    }
  }
  return ret;
}

Layer *Canvas::getActiveLayer() {
  Layer *ret = NULL;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.active) {
      ret = layers[i];
      break;
    }
  }
  return ret;
}

unsigned int activeLayerSwitchIndex = 0;
unsigned int lastActiveLayerSwitchIndex = 0;
bool wasActiveLayerSwitched() {
  return activeLayerSwitchIndex != lastActiveLayerSwitchIndex;
}

void Canvas::setActiveLayer(int layer) {
  Layer *before = getActiveLayer();
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    l->state.active = i == layer ? true : false;
  }
  if (before != getActiveLayer()) activeLayerSwitchIndex++;
}

void Canvas::noSelectedLayers() {
  for (int i = 0; i < currentCanvas->layers.size(); i++) {
    Layer *l = currentCanvas->layers[i];
    l->state.selected = false;
  }
}

void Canvas::setActiveLayerByScreenPos(double screenX, double screenY) {
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    if (l->state.visible) {
      double lx = screenX, ly = screenY; getLayerPos(l, lx, ly, lx, ly);
      int lxi = (int)floor(lx);
      int lyi = (int)floor(ly);
      if ((unsigned int)lxi < l->width && (unsigned int)lyi < l->height) {
        unsigned int col = l->pixels[lxi+lyi*l->width];
        int a = (col >> 24) & 255;
        if (a != 0) {
          setActiveLayer(i);
          break;
        }
      }
    }
  }
}

Vector Canvas::boundingBox() {
  Vector ret;
  bool first = true;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Vector k = layers[i]->boundingBox();
      if (first || k.x < ret.x) ret.x = k.x;
      if (first || k.y < ret.y) ret.y = k.y;
      if (first || k.z > ret.z) ret.z = k.z;
      if (first || k.w > ret.w) ret.w = k.w;
      first = false;
    }
  }
  return ret;
}

Vector Canvas::getLayerPixelBounding(Layer *l, double canvasX0, double canvasY0, double canvasX1, double canvasY1) {
  Vector ret;
  bool first = true;
  for (int i = 0; i < 4; i++) {
    Vector k(i & 1 ? canvasX1 : canvasX0, i & 2 ? canvasY1 : canvasY0,0);
    k = l->transform.transformation * k;
    if (first || k.x < ret.x) ret.x = k.x;
    if (first || k.y < ret.y) ret.y = k.y;
    if (first || k.x > ret.z) ret.z = k.x;
    if (first || k.y > ret.w) ret.w = k.y;
    first = false;
  }
  if (ret.x < 0) ret.x =0;
  if (ret.y < 0) ret.y =0;
  if (ret.x > l->width-1) ret.x = l->width-1; // not perfect but ok
  if (ret.y > l->height-1) ret.y = l->height-1; // not perfect but ok
  if (ret.z > l->width) ret.z = l->width;
  if (ret.w > l->height) ret.w = l->height;
  if (ret.z < 0) ret.z = 0;
  if (ret.w < 0) ret.w = 0;
  return ret;
}

void Canvas::setBoundingPos(double x, double y) {
  Vector k = boundingBox();
  if (k.z-k.x<=0) return;
  if (k.w-k.y<=0) return;
  double updateX = x - k.x;
  double updateY = y - k.y;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Layer *l = layers[i]; Vector b2 = l->boundingBox(); l->setLayerPos(b2.x+updateX, b2.y+updateY);
    }
  }
}

void Canvas::setBoundingSize(double width, double height) {
  Vector k = boundingBox();
  if (k.z-k.x<=0) return;
  if (k.w-k.y<=0) return;
  double updateX = width / (k.z-k.x);
  double updateY = height / (k.w-k.y);
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Layer *l = layers[i]; Vector b2 = l->boundingBox(); 
      l->setLayerPos((b2.x-k.x)*updateX+k.x,(b2.y-k.y)*updateY+k.y);
      l->setLayerSize((b2.z-b2.x)*updateX, (b2.w-b2.y)*updateY);
    }
  }
}

double Canvas::getBoundingAngleDegrees() {
  double angle = 0;
  int k = 0;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Layer *l = layers[i]; angle += l->getAngleDegrees(); k++;
    }
  }
  if (k > 0) angle = angle / k;
  return angle;
}

void Canvas::setBoundingAngleDegrees(double degrees) {
  double angleDelta = degrees - getBoundingAngleDegrees();
  angleDelta += 360;
  if (angleDelta < 0) return;
  angleDelta = fmod(angleDelta,360);

  Vector center;
  for (int i2 = 0; i2 < layers.size(); i2++) {
    if (layers[i2]->state.selected||layers[i2]->state.active) {
      Layer *l = layers[i2];
      Vector c = l->boundingBox();
      center += Vector((c.z-c.x)*0.5+c.x,(c.w-c.y)*0.5+c.y,1);
    }
  }
  if (center.z != 0) center /= center.z;

  Matrix modify; modify = rotationMatrixAngleAxis(-angleDelta*PI/180.0,0,0,1);

  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Layer *l = layers[i];
      Matrix t; t.translate(Vector(center.x,center.y,0));
      l->transform.transformation = l->transform.transformation * t * modify * inverse2(t);
      fixTransform(l->transform);
      l->transform.update();
    }
  }
}

void Canvas::cropLayersByAlpha(double alpha) {
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      layers[i]->cropByAlpha(alpha);
    }
  }
}

void Canvas::snapToGrid() {
  if (gridStepX == 0 || gridStepY == 0) return;
  Vector k = boundingBox();
  int px = floor(k.x);
  int py = floor(k.y);
  int mx2 = px - gridOffsetX;
  int my2 = py - gridOffsetY;
  int mx = mx2;
  int my = my2;
  mx += gridStepX/2;
  my += gridStepY/2;
  mx /= gridStepX;
  my /= gridStepY;
  if (mx2 < 0) mx -= 1;
  if (my2 < 0) my -= 1;
  mx *= gridStepX;
  my *= gridStepY;
  mx += gridOffsetX;
  my += gridOffsetY;
  mx = mx - px;
  my = my - py;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Layer *l = layers[i]; Vector b2 = l->boundingBox(); 
      l->setLayerPos(floor(b2.x)+mx,floor(b2.y)+my);
    }
  }
  shouldUpdateAll = true;
}

void Canvas::centerCanvas() {
  double lx = 0, ly = 0; getScreenPos(lx,ly,lx,ly);
  double lx0 = canvasX0, ly0 = canvasY0; getScreenPos(lx0,ly0,lx0,ly0);
  double lx1 = canvasX1, ly1 = canvasY1; getScreenPos(lx1,ly1,lx1,ly1);
  control.scrollX = (lx1-lx0)*0.5+(lx0-lx)-width/2;
  control.scrollY = (ly1-ly0)*0.5+(ly0-ly)-height/2;
  control.setCanvasTransform(this);
}

void Canvas::fitCanvas() {
  control.setCanvasTransform(this);
  double lx = 0, ly = 0; getScreenPos(lx,ly,lx,ly);
  double lx0 = canvasX0, ly0 = canvasY0; getScreenPos(lx0,ly0,lx0,ly0);
  double lx1 = canvasX1, ly1 = canvasY1; getScreenPos(lx1,ly1,lx1,ly1);
  control.scrollX = (lx1-lx0)*0.5+(lx0-lx)-width/2;
  control.scrollY = (ly1-ly0)*0.5+(ly0-ly)-height/2;
  control.setCanvasTransform(this);
  lx0 = canvasX0;
  ly0 = canvasY0;
  lx1 = canvasX1;
  ly1 = canvasY1;
  double scaleX = (lx1-lx0)!=0?width/(lx1-lx0)*aspectRatio*main_window_yres/main_window_xres:1;
  double scaleY = (ly1-ly0)!=0?height/(ly1-ly0):1;
  double scale = scaleX<scaleY?scaleX:scaleY;
  control.zoomTo(this,screenPosX+width*0.5,screenPosY+height*0.5,scale*100.f);
}

bool Canvas::isOnlyActiveLayerActive() {
  if (getActiveLayer()==NULL) return false;
  int count = 0;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      count++;
    }
  }
  return count == 1;
}

void Canvas::getLayerSelectionFromLayers(Layer *l) {
  if (l == NULL) return;
  l->selectNone();
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected&&(layers[i] != l)) {
      l->addSelectionFromLayer(layers[i]);
    }
  }
}

void Canvas::clearAllOtherLayerSelections(Layer *l) {
  if (l == NULL) return;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i] != l) {
      layers[i]->selectNone();
    }
  }
}

bool Canvas::hasUpdates() {
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->hasUpdates()) 
      return true;
  }
  return false;
}

void Canvas::signSave() {
  for (int i = 0; i < layers.size(); i++) {
    ::signSave(layers[i]);
  }
}

void Canvas::lockLayers(bool lockOrUnlock) {
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      if (!layers[i]->state.permanentlyLocked) {
        layers[i]->state.locked = lockOrUnlock;
      }
    }
  }
}

void Canvas::getContentFromOtherLayer(Layer *dest, unsigned int sourceId) {
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->id == sourceId) {
      Layer *l = layers[i];
      if (dest->undoData.updatePixelsCount != l->undoData.updatePixelsCount) {
        l->undoData.updatePixelsCount = dest->undoData.updatePixelsCount; // attention here may interfer with undo if some implementations change
        if (dest->pixels == NULL || l->width != dest->width || l->height != dest->height) {
          if (dest->pixels != NULL) {delete[] dest->pixels; dest->pixels = NULL;}
          if (dest->pixelStates != NULL) {delete[] dest->pixelStates; dest->pixelStates = NULL;}
          dest->width = l->width;
          dest->height = l->height;
          dest->pixels = new unsigned int[dest->width*dest->height];
          dest->pixelStates = new unsigned char[dest->width*dest->height];
           shouldUpdateAll = true;
        }
        if (memcmp(dest->pixels,l->pixels,dest->width*dest->height*sizeof(unsigned int)) != 0) shouldUpdateAll = true;
        memcpy(dest->pixels,l->pixels,dest->width*dest->height*sizeof(unsigned int));
        //memcpy(dest->pixelStates,l->pixelStates,dest->width*dest->height*sizeof(unsigned char));
        memset(dest->pixelStates,0,dest->width*dest->height*sizeof(unsigned char));
      }
      break;
    }
  }
}

void repaintLayer(Layer *layer, bool fullUpdate) {
  layer->repaintState = fullUpdate ? REPAINTSTATE_FULLUPDATE : REPAINTSTATE_PIXELUPDATE;
  shouldUpdate = true;
}

void alphaPixel(int x, int y, unsigned int col) {
  if ((unsigned int)x >= glFrameBufferWidth || (unsigned int)y >= glFrameBufferHeight) return;
  unsigned int p = glFrameBuffer[x+y*glFrameBufferWidth];
  int sr = col & 255;
  int sg = (col>>8) & 255;
  int sb = (col>>16) & 255;
  int sa = (col>>24) & 255;
  int dr = p & 255;
  int dg = (p>>8) & 255;
  int db = (p>>16) & 255;
  int da = (p>>24) & 255;
  int r = (sr-dr)*sa/255+dr;
  int g = (sg-dg)*sa/255+dg;
  int b = (sb-db)*sa/255+db;
  glFrameBuffer[x+y*glFrameBufferWidth] = r|(g<<8)|(b<<16)|(p & 0xff000000);
}

void blitCanvas() {
  int count = 0;
  Layer *repaintLayer = NULL;
  int repaintLayerRepaintState = REPAINTSTATE_NONE;
  for (int i = 0; i < currentCanvas->layers.size(); i++) {
    if (currentCanvas->layers[i]->repaintState != REPAINTSTATE_NONE) {
      repaintLayer = currentCanvas->layers[i];
      repaintLayerRepaintState = repaintLayer->repaintState;
      repaintLayer->repaintState = REPAINTSTATE_NONE;
      count++;
    }
  }
  if (count>1) repaintLayer = NULL;

  if (shouldUpdateAll||shouldUpdate) {
    if (shouldUpdateAll) repaintLayer = NULL;
    if (repaintLayer != NULL) {
      shouldUpdateAll = false;
      shouldUpdate = false;
      currentCanvas->updateLayer(repaintLayer,repaintLayerRepaintState == REPAINTSTATE_FULLUPDATE);
    } else {
      bool all = shouldUpdateAll;
      shouldUpdateAll = false;
      shouldUpdate = false;
      currentCanvas->update(all);
    }
  }

  Matrix sp = selectionPlane->transform.transformation;
  int time = (int)(glSeconds()*8.0);
  Matrix m = currentCanvas->transform.transformation;
  int gridValX = -currentCanvas->gridOffsetX+currentCanvas->gridStepX * 10000;
  int gridValY = -currentCanvas->gridOffsetY+currentCanvas->gridStepY * 10000;
  int selectionX0 = -1;
  int selectionY0 = -1;
  int selectionX1 = -1;
  int selectionY1 = -1;
  for (int y = 0; y < currentCanvas->height; y++) {
    int dy = y + currentCanvas->screenPosY;
    if ((unsigned int)dy >= glFrameBufferHeight) continue;
    for (int x = 0; x < currentCanvas->width; x++) {
      int dx = x + currentCanvas->screenPosX;
      if ((unsigned int)dx >= glFrameBufferWidth) continue;
      unsigned int col = currentCanvas->pixels[x+y*currentCanvas->width];
      if (monitorGamma != 1.f) {
        col = gammaTable[col & 255]|(gammaTable[(col>>8) & 255]<<8)|(gammaTable[(col>>16) & 255]<<16)|(col & 0xff000000);
      }
      double lx3 = x*m.m[0+0*4]+y*m.m[0+1*4]+m.m[0+3*4];
      double ly3 = x*m.m[1+0*4]+y*m.m[1+1*4]+m.m[1+3*4];
      int lx2 = (int)(lx3+4*100000)-4*100000;
      int ly2 = (int)(ly3+4*100000)-4*100000;
      if ((col & 0xff000000) != 0xff000000) {
        int lx = lx2 + 4*100000;
        int ly = ly2 + 4*100000;
        lx /= 4;
        ly /= 4;
        int k = (lx + ly) & 1;
        if (k) {
          int r = col & 0xff;
          int g = (col>>8) & 0xff;
          int b = (col>>16) & 0xff;
          int a = (col>>24) & 0xff;
          int aadd = 64*(255-a);
          r = (r*a+aadd)/255;
          col = r|(g<<8)|(b<<16)|0xff000000;
        }
      }
      if (lx3 < currentCanvas->canvasX0 || ly3 < currentCanvas->canvasY0 || lx3 >= currentCanvas->canvasX1 || ly3 >= currentCanvas->canvasY1) {
        if (!(currentCanvas->pixelStates[x+y*currentCanvas->width] & PIXELSTATE_CANVAS_OVERLAY)) {
          col >>= 3;
          col &= 0x1f1f1f1f;
          col |= 0x20202020;
        }
      }
      if (currentCanvas->gridOn) {
        int lx2x = (int)((x+1)*m.m[0+0*4]+y*m.m[0+1*4]+m.m[0+3*4]+4*100000)-4*100000;
        int ly2y = (int)(x*m.m[1+0*4]+(y+1)*m.m[1+1*4]+m.m[1+3*4]+4*100000)-4*100000;
        lx2x =  (lx2x + gridValX) % currentCanvas->gridStepX;
        ly2y =  (ly2y + gridValY) % currentCanvas->gridStepY;
        int lx2a = (lx2 + gridValX) % currentCanvas->gridStepX;
        int ly2a =  (ly2 + gridValY) % currentCanvas->gridStepY;
        if (lx2a > lx2x || ly2a > ly2y) {
          int g = (col>>8) & 255;
          col = g > 128 ? 0xff000000 : 0xff00ff00;
        }
      }
      unsigned char state = currentCanvas->pixelStates[x+y*currentCanvas->width];
      if (state & PIXELSTATE_CANVAS_UPDATED) {
        state &= ~(PIXELSTATE_CANVAS_UPDATED|PIXELSTATE_CANVAS_GRID|PIXELSTATE_CANVAS_OVERLAY); 
        if (state & PIXELSTATE_GRID) {
          state |= PIXELSTATE_CANVAS_GRID;
          state &= ~PIXELSTATE_GRID;
        }
        currentCanvas->pixelStates[x+y*currentCanvas->width] = state;
      }

      if (state & (PIXELSTATE_SELECTED|PIXELSTATE_OUTLINE|PIXELSTATE_CANVAS_GRID)) {
        if (x != 0 && x != currentCanvas->width-1 && y != 0 && y != currentCanvas->height-1) {
          unsigned char before = state;
          state &= currentCanvas->pixelStates[x-1+y*currentCanvas->width];
          state &= currentCanvas->pixelStates[x+(y-1)*currentCanvas->width];
          unsigned char stateTopLeft = state;
          state &= currentCanvas->pixelStates[x+1+y*currentCanvas->width];
          state &= currentCanvas->pixelStates[x+(y+1)*currentCanvas->width];
          if (before & PIXELSTATE_CANVAS_GRID) {
            if (!(stateTopLeft & PIXELSTATE_CANVAS_GRID)) col = (((x+y+time)/4) & 1) ? 0xff404040 : 0xff400040;
          }
          if (before & PIXELSTATE_SELECTED) {
            if (!(state & PIXELSTATE_SELECTED)) col = (((x+y+time)/4) & 1) ? 0xffffffff : 0xff0000ff;
          }
          if (before & PIXELSTATE_OUTLINE) {
            if (!(state & PIXELSTATE_OUTLINE)) col = (((x+y+time)/4) & 1) ? 0xffffffff : 0xffff00ff;
          }
        }
      }
      if (showSelectionPlane) {
        double lxs = lx3*sp.m[0+0*4]+ly3*sp.m[0+1*4]+sp.m[0+3*4];
        double lys = lx3*sp.m[1+0*4]+ly3*sp.m[1+1*4]+sp.m[1+3*4];
        int lx = (int)lxs;
        int ly = (int)lys;
        if (lx == 0) lx = (int)floor(lxs);
        if (ly == 0) ly = (int)floor(lys);
        if ((unsigned int)lx < selectionPlane->width &&  (unsigned int)ly < selectionPlane->height) {
          unsigned int s = selectionPlane->pixels[lx+ly*selectionPlane->width];
          if (s != 0) {
            s = (((x+y+time)/4) & 1) ? 0xffffffff : 0xff0000ff;
            int r = (col & 255)>>1;
            int g = ((col>>8) & 255)>>1;
            int b = ((col>>16) & 255)>>1;
            r += (s & 255)>>1;
            g += ((s>>8) & 255)>>1;
            b += ((s>>16) & 255)>>1;
            col = (r)|(g<<8)|(b<<16)|(col & 0xff000000);
            if (showSelectionPlaneBounding) {
              bool first = selectionX0<0;
              if (first || dx < selectionX0) selectionX0 = dx;
              if (first || dy < selectionY0) selectionY0 = dy;
              if (first || dx > selectionX1) selectionX1 = dx;
              if (first || dy > selectionY1) selectionY1 = dy;
            }
          }
        }
      }
      glFrameBuffer[dx+dy*glFrameBufferWidth] = col;
    }
  }
  if (selectionX0>=0 && showSelectionPlaneBounding) {
    selectionX0 -= 1;
    selectionY0 -= 1;
    selectionX1 += 1;
    selectionY1 += 1;
    unsigned int col = ((time/4) & 1) ? 0x40ffffff : 0x400000ff;

    for (int x = selectionX0; x <= selectionX1; x++) {
      if (x >= 0 && x < glFrameBufferWidth) {
        int y;
        y = selectionY0; if (y >= 0 && y < glFrameBufferHeight) alphaPixel(x,y,col);
        y = selectionY1; if (y >= 0 && y < glFrameBufferHeight) alphaPixel(x,y,col);
      }
    }
    for (int y = selectionY0; y <= selectionY1; y++) {
      if (y >= 0 && y < glFrameBufferHeight) {
        int x;
        x = selectionX0; if (x >= 0 && x < glFrameBufferWidth) alphaPixel(x,y,col);
        x = selectionX1; if (x >= 0 && x < glFrameBufferWidth) alphaPixel(x,y,col);
      }
    }
  }

  if (clearOutlineAfterNextPaint) {
    for (int i = 0; i < currentCanvas->layers.size(); i++) {
      Layer *l = currentCanvas->layers[i];
      if (l->hasOutline) {l->updatePixelStates(~PIXELSTATE_OUTLINE,0,0); reverseLastUpdatedPixelStates(l);}
      l->hasOutline = false;
    }
    currentCanvas->updatePixelStates(~(PIXELSTATE_OUTLINE),0,0);
  }
}

void canvasDefault(Canvas *canvas) {
  canvas->init(glFrameBufferWidth-sidebar_width-toolbar_width-toolview_width,glFrameBufferHeight-menuBar_height);
  canvas->screenPosX = toolbar_width+toolview_width;
  canvas->screenPosY = menuBar_height;
}

void addPicture(String fileName) {
  currentCanvas = new Canvas();
  canvas.push_back(currentCanvas);
  canvasDefault(currentCanvas);
  currentCanvas->addLayer(loadImage("",fileName));
  currentCanvas->setActiveLayer(0);
  currentCanvas->fitCanvasToLayers();
  shouldUpdateAll = true;
  updatedPixels(currentCanvas->getActiveLayer());
  updatedPixelStates(currentCanvas->getActiveLayer());
  currentCanvas->fitCanvas();
  currentCanvas->signSave(); signSaveForUndo();
}

void addCanvas(Canvas *canvas) {
  currentCanvas = canvas;
  ::canvas.push_back(currentCanvas);
  shouldUpdateAll = true;
  updatedPixels(currentCanvas->getActiveLayer());
  updatedPixelStates(currentCanvas->getActiveLayer());
  currentCanvas->signSave(); signSaveForUndo();
}

void insertPicture(String fileName) {
  Layer *l = loadImage("",fileName);
  if (l == NULL) return;
  int pos = 0;
  for (int i = 0; i < currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.active) {pos = i; break;}
  currentCanvas->layers.insert(l,pos);
  currentCanvas->setActiveLayer(pos);
  shouldUpdateAll = true;
  updatedPixels(currentCanvas->getActiveLayer());
  updatedPixelStates(currentCanvas->getActiveLayer());
  signSave(currentCanvas->getActiveLayer());
}

void addTempPicture() {
  currentCanvas = new Canvas();
  sprintf(currentCanvas->pictureName,"empty");
  canvas.push_back(currentCanvas);
  canvasDefault(currentCanvas);
  shouldUpdateAll = true;
}

void newPicture(int w, int h,const String &pictureName) {
  currentCanvas = new Canvas();
  sprintf(currentCanvas->pictureName,pictureName.c_str());
  canvas.push_back(currentCanvas);
  canvasDefault(currentCanvas);
  Layer *l = new Layer();
  l->init(w,h);
  l->selectAll();
  l->fillSelectionWithColor(0xff000000);
  l->selectNone();
  sprintf(l->name,"layer");
  currentCanvas->addLayer(l);
  currentCanvas->fitCanvasToLayers();
  currentCanvas->setActiveLayer(0);
  currentCanvas->fitCanvas();
  shouldUpdateAll = true;
  updatedPixels(currentCanvas->getActiveLayer());
  updatedPixelStates(currentCanvas->getActiveLayer());
  currentCanvas->signSave(); signSaveForUndo();
}

void duplicatePicture(bool mergeLayers) {
  Canvas *zwo = currentCanvas->clone();
  sprintf(zwo->pictureName,"duplicated");
  if (mergeLayers) {
    zwo->selectAllLayers();
    zwo->mergeDownLayers();
    if (zwo->layers.size()>0) {
      zwo->setActiveLayer(0);
      sprintf(zwo->getActiveLayer()->name,"merged");
    }
  }
  canvas.push_back(zwo);
  shouldUpdateAll = true;
}
