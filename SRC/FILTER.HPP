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

void initNoise32();
unsigned int noise32();

bool filterDifferenceClouds();
bool filterNoise();
bool filterSobelEdgeDetect();
bool filterMorphologicalEdgeDetect();
bool filterAlphaSoften();
bool filterBoxBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int boxW, int boxH, unsigned char *pixelStates, bool usePixelStates);
bool filterRadialBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float radialAngle, float radialSize, unsigned char *pixelStates, bool usePixelStates);
bool filterGaussBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float radiusX, float radiusY, unsigned char *pixelStates, bool usePixelStates);
bool filterIterativeBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int steps, unsigned char *pixelStates, bool usePixelStates);
bool filterMedianBlur(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int medianW, int medianH, unsigned char *pixelStates, bool usePixelStates, bool circular, bool keepSelection);
bool filterEmboss(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float angle, unsigned char *pixelStates, bool usePixelStates);
bool filterSimpleKeying(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, int channel, float factor, unsigned char *pixelStates, bool usePixelStates, int mode, bool binary, bool invert);
bool filterColorKeying(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, unsigned int color, float factor, unsigned char *pixelStates, bool usePixelStates, int mode, bool binary, bool invert);
bool filterDenoise();
bool filterSharpen();
bool filterXBRAntialias();
bool filterSuperXBR();
bool filterEagle();
bool filterEpx();
bool filterHistogramMatching(class Layer *histogramSource);
bool filterClahe(float claheConstant, int cellSize);
bool filterColorTransfer(class Layer *colorSource);
bool filterSepia();
bool filterSchraffur(float blackLineWidth, float transparentLineWidth, float angle, bool binary);
bool filterCheckerboard(int w, int h, int xp, int yp);
bool filterHistogramMatchingColored(class Layer *histogramSource);
bool filterSoften();
bool filterBlackWhiteOCR();
bool filterBayerDither();
bool filterDisplacementMap(unsigned int *dest,unsigned int *source, unsigned int width, unsigned int height, float displacement1, float displacement2, int type, unsigned char *pixelStates, bool usePixelStates, class Layer *displace);
bool filterWatershed(class Layer *layer);
bool filterSubtract(unsigned int *dest, unsigned int *minuend, unsigned int *subtrahend, unsigned int width, unsigned int height);
bool filterSetAlpha(unsigned int *dest, unsigned int width, unsigned int height, int alpha);
bool filterSelectionDistanceField(class Layer *layer);
bool filterSelectionAverageColor(class Layer *layer);
bool filterPixelate(int cellSizeX, int cellSizeY);
bool filterSelectionVerticalGradient(class Layer *layer);
bool filterFillAlphaHoles(class Layer *layer, bool forAlpha);
bool filterTurnToShadow();
bool filterOutline(unsigned int *dest, unsigned int *source, unsigned int width, unsigned int height, float outlineSize, bool antialiasing);
bool filterHalftone(class Layer *layer, float cellSize, int type, float dotScale);
bool filterMedianize(class Layer *layer, float cellSize);
bool filterSegmentation(class Layer *layer, int cellCountX, int cellCountY);
bool filterSprinkled(class Layer *layer, float minRad, float maxRad, int maxCount);
bool filterSprinkleBlackWhite(class Layer *layer, float minRad, float maxRad, bool colored, float dotScale);
bool filterComic(class Layer *layer, float steps, bool sat);
bool filterLuminanceBlackWhite(class Layer *layer, float luminanceBridgeBottom, float luminanceBridgeTop);
bool filterCache(class Layer *layer, int leftRight, int topBottom, unsigned int color);
bool filterVoronoi(class Layer *layer, int pointCount);

#define S_FILTER_CPP()

#endif // __FILTER_HPP__
