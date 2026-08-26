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
extern Array<ImVec4> *paletteToSave;
extern bool showPalette;
extern bool showSkinTone;
extern bool alternativeColorWheel;

extern unsigned short gammaTable[256*3];
void reGenMonitorGamma();
extern float monitorGamma;
extern float monitorMin[3];
extern float monitorMax[3];
extern bool monitorColorsDithered;
extern bool hasMonitorCalibration;
static __inline unsigned int gammaCorrect(unsigned int col, int x, int y) {
  if (!hasMonitorCalibration) return col;
  int r = gammaTable[col & 255];
  int g = gammaTable[((col>>8) & 255)+256*1];
  int b = gammaTable[((col>>16) & 255)+256*2];
  if (monitorColorsDithered) {
    int k;
    if ((y & 1) == 0) k = (x & 1) ? 128 : 0;
    if ((y & 1) != 0) k = (x & 1) ?  64 : 196;
    r += k; if (r > 65535) r = 65535;
    g += k; if (g > 65535) g = 65535;
    b += k; if (b > 65535) b = 65535;
  }
  r >>= 8; g >>= 8; b >>= 8;
  return r|(g<<8)|(b<<16)|(col & 0xff000000);
}
ImVec4 gammaCorrect4(const ImVec4 &col);

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

void sortPalette(Array<ImVec4> &pal);

void reGenColorWheel();

#define COLORTRACEBACKS 8
extern unsigned int colorTraceBacks[COLORTRACEBACKS];

#define S_COLORFLD_CPP()\
  S_IMVEC4("foregroundColor", foregroundColor);\
  S_BOOL("showPalette", showPalette);\
  S_CARRAY("colorTraceBacks",colorTraceBacks,S_UINT(NULL,colorTraceBacks[i]),COLORTRACEBACKS);\
  zipPalette(palette,paletteColorsPerRow,save);\
  if (!save) shouldRecreateQuantization = true;\
  S_FLOAT("monitorGamma", monitorGamma);\
  reGenMonitorGamma();

#endif // __COLORFLD_HPP__
