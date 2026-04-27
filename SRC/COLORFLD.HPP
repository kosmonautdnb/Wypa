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
#ifndef __COLORFLD_HPP__
#define __COLORFLD_HPP__

#include "IMGUI/IMGUI.H"
#include "STL/ARRAY.HPP"
#include "STL/STRING.HPP"

extern unsigned int colorField_height;
extern ImVec4 foregroundColor;

extern int paletteColorsPerRow;
extern Array<ImVec4> palette;

unsigned int swapRB(unsigned int color);
unsigned int getUIntColor(const ImVec4 &k);
ImVec4 getImVec4Color(unsigned int v);

void initColorField();
void freeColorField();
void drawColorField();

bool colorItem(const String &name, const String &id, ImVec4 *color);
void addUsedColor(const ImVec4 &color);

ImVec4 hsl(double h,double s,double l);
void getHsl(const ImVec4 &rgb, float *hv, float *sv, float *lv);

void updatePalette();

#endif // __COLORFLD_HPP__
