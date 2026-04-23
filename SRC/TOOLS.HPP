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
#ifndef __TOOLS_HPP__
#define __TOOLS_HPP__

#include "STL/GL.H"
#include "STL/ARRAY.HPP"
#include <string.h>
#include <stdio.h>

enum BRUSHINSTANCE {
  BRUSHINSTANCE_PAINT = 0,
  BRUSHINSTANCE_SELECT = 1,
  BRUSHINSTANCE_EDIT = 2,
  BRUSHINSTANCE_COUNT = 3
};

class BrushInstance {
public:
  BrushInstance() {
    isPencil = false;
    pixelWidth = 0;
    pixelHeight = 0;
    glHandle = 0;
    followedRotation = 0;
  }
  ~BrushInstance() {
    if (glHandle != 0) {glDeleteTextures(1,&glHandle); glHandle = 0;}
  }
  float followedRotation;
  int pixelWidth;
  int pixelHeight;
  bool isPencil;
  unsigned int glHandle;
  Array<float> values;
  void resize(class Brush *base, double newPixelWidth, double newPixelHeight);
  void update(class Brush *base);
  void updateGLHandle(class Brush *base);
};

#define BRUSHNAMEMAX 16

class Brush {
public:
  Brush() {
    size = 64;
    pressure = 1;
    softness = 1;
    edginess = 0;
    ovality = 0;
    aspect = 1;
    rotation = 0;
    randomX = 0;
    randomY = 0;
    opacity = 1;
    rate = 1;
    followRotation = false;
    static int counter = 0; counter++; counter &= 1023;
    sprintf(name,"Brush %d",counter);
    memset(instances,0,sizeof(instances));
  }
  ~Brush() {
    for (int i = 0; i < BRUSHINSTANCE_COUNT; i++) if (instances[i] != NULL) delete instances[i];
  }
  BrushInstance *getCreateInstance(int brushInstance, int w, int h, bool pencil);
  void freeInstance(int brushInstance);
  BrushInstance * instances[BRUSHINSTANCE_COUNT];
  char name[BRUSHNAMEMAX];
  float size;
  float pressure;
  float softness;
  float edginess;
  float ovality;
  float aspect;
  float rotation;
  float randomX;
  float randomY;
  float opacity;
  float rate;
  bool followRotation;
  void update();
  double pixelWidth();
  double pixelHeight();
};

extern Array<Brush *> brushes;
extern int currentBrush;

class GradientElement {
public:
  GradientElement() {
    static int counter = 0; counter++; counter &= 65535;
    id = counter;
    color = 0xff000000;
    position = 0;
    isStep = false;
  }
  ~GradientElement() {
  }
  unsigned int id;
  unsigned int color;
  bool isStep;
  float position;
};

class Gradient {
public:
  Gradient() {
    positionMin = 0;
    positionMax = 0;
    lastAccessPoint = 0;
    inverted = false;
    doubleSided = false;
    round = false;
  }
  ~Gradient() {
  }
  int lastAccessPoint;
  float positionMin;
  float positionMax;
  unsigned int getColor(float between0And1);
  unsigned int getColor2D(float dx,float dy);
  bool inverted;
  bool doubleSided;
  bool round;
  void sort();
  void update();
  Array<GradientElement> elements;
};

extern Gradient gradient;

extern double wandAlphaDistance;
extern double wandColorDistance;
extern bool wandByNeighbors;

extern bool pipetteFromLayer;

extern double fillColorDistance;

enum SELECTIONTOOL {
  SELECTIONTOOL_RECTANGLE,
  SELECTIONTOOL_ELLIPSE,
  SELECTIONTOOL_LASSO,
  SELECTIONTOOL_POLYGON
};

extern int selectionTool;
extern bool selectionProportional;
extern double toolW,toolH;

extern float lineWidth;
extern bool lineCap;
extern bool lineAntialiasing;

extern bool toolOutline;

bool isPaintAreaActive();

void initTools();
void freeTools();

void useTool();

void doFXAA(unsigned int *sourceBuffer, const int width, const int height, unsigned char *maskBuffer, unsigned char mask);

#endif //__TOOLS_HPP__
