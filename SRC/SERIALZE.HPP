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
#ifndef __SERIALZE_HPP__
#define __SERIALZE_HPP__

#include "STL/STRING.HPP"
#include "STL/ARRAY.HPP"

#define LONGFILENAME(__str__) (longFileNames ? longFileName(__str__) : unixFileName(__str__))
#define SHORTFILENAME(__str__) (longFileNames ? shortFileName(__str__) : unixFileName(__str__))
extern bool longFileNames;

String fromLocalFileName(const String &str);

void updateStatusBar();
void initStatusBar();

String longFileName(const String &a);
String shortFileName(const String &a);

void importImage(const char *shortPath, const char *longFileName);
void insertImageLayers(const char *shortPath, const char *longFileName);
void exportLayers(const char *shortPath, const char *longFileName);
void exportPNG(const char *shortPath, const char *longFileName);
void exportLayersRaw(const char *shortPath, const char *longFileName, bool applyColorTransform);
void insertPSD(const char *shortPath, const char *longFileName);
void insertSVG(const char *shortPath, const char *longFileName);

void loadPalette(const char *shortPath, const char *longFileName);
void savePalette(const char *shortPath, const char *longFileName);

void loadGradient(const char *shortPath, const char *longFileName);
void saveGradient(const char *shortPath, const char *longFileName);

void loadBrush(const char *shortPath, const char *longFileName);
void saveBrush(const char *shortPath, const char *longFileName);
void loadBrushes(const char *shortPath, const char *longFileName);
void saveBrushes(const char *shortPath, const char *longFileName);

void loadBrushShape(const char *shortPath, const char *longFileName);

void loadImage(const char *shortPath, const char *longFileName);
void saveImage(const char *shortPath, const char *longFileName);

void loadFont(const char *shortPath, const char *longFileName);
extern unsigned int saveFontId;
void saveFont(const char *shortPath, const char *longFileName);

void loadGradientCurve(const char *shortPath, const char *longFileName);
void saveGradientCurve(const char *shortPath, const char *longFileName);

void loadTilingAnim(const char *shortPath, const char *longFileName);
void saveTilingAnim(const char *shortPath, const char *longFileName);

void loadTileMap(const char *shortPath, const char *longFileName);
void saveTileMap(const char *shortPath, const char *longFileName);

void importFolder(const char *shortPath, const char *longFileName);

double updateToolTip(unsigned int *rgba, int width, int height, const char *path, const char *fileName);
class Layer *loadImage(const String &filePath, const String &fileName);

void insertSVGResized(int layerWidth, int layerHeight, float scale);
Array<String> getLines(const char *string);

void getFileArray(Array<unsigned char> *arr,const String &maybeExtendedFileName);

#endif //__SERIALZE_HPP__
