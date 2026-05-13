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
#ifndef __FILTER_HPP__
#define __FILTER_HPP__

void initFilter();
void freeFilter();

void filterDifferenceClouds();
void filterEdgeDetect();
void filterAlphaSoften();
void filterBoxBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int boxW, int boxH, unsigned char *pixelStates, bool usePixelStates);
void filterRadialBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float radialAngle, float radialSize, unsigned char *pixelStates, bool usePixelStates);
void filterGaussBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float radiusX, float radiusY, unsigned char *pixelStates, bool usePixelStates);
void filterIterativeBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int steps, unsigned char *pixelStates, bool usePixelStates);
void filterEmboss(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float angle, unsigned char *pixelStates, bool usePixelStates);
void filterSimpleKeying(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int channel, float factor, unsigned char *pixelStates, bool usePixelStates, int mode, bool binary);

#define S_FILTER_CPP()

#endif // __FILTER_HPP__
