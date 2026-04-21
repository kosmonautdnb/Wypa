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

#include "STL/ARRAY.HPP"

class Brush {
public:
  Brush() {
    isPencil = false;
    pixelWidth = 0;
    pixelHeight = 0;
    glHandle = 0;
    size = 64;
    pressure = 1;
    softness = 1;
    outlinedOnGL = false;
  }
  ~Brush() {
  }
  bool isPencil;
  double pixelWidth;
  double pixelHeight;
  float size;
  float pressure;
  float softness;
  bool outlinedOnGL;
  unsigned int glHandle;
  Array<float> values;
  Brush *clone();
  void resize(double newPixelWidth, double newPixelHeight);
  void updateGLHandle();
  void update();
};

extern Brush brush;
extern Brush pencil;

extern double wandAlphaDistance;
extern double wandColorDistance;
extern bool wandByNeighbors;

bool isPaintAreaActive();

void initTools();
void freeTools();

void useTool();

#endif //__TOOLS_HPP__
