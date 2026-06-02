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
#ifndef __DIALOGS_HPP__
#define __DIALOGS_HPP__

#include "STL/STRING.HPP"
#include "STL/DOS.HPP"

extern bool reOpen;

void initDialogs();
void freeDialogs();
void drawDialogs();

enum DIALOGMODE {
  DIALOG_NONE = 0,
  DIALOG_IMPORTIMAGE = 1,
  DIALOG_INSERTIMAGELAYERS = 2,
  DIALOG_EXPORTLAYERS = 3,
  DIALOG_EXPORTPNG = 4,
  DIALOG_LOADPALETTE = 5,
  DIALOG_SAVEPALETTE = 6,
  DIALOG_EXPORTLAYERSRAW = 7,
  DIALOG_INSERTPSD = 8,
  DIALOG_LOADGRADIENT = 9,
  DIALOG_SAVEGRADIENT = 10,
  DIALOG_INSERTSVG = 11,
  DIALOG_LOADBRUSHSHAPE = 12,
  DIALOG_LOADBRUSH = 13,
  DIALOG_SAVEBRUSH = 14,
  DIALOG_LOADBRUSHES = 15,
  DIALOG_SAVEBRUSHES = 16,
  DIALOG_LOADIMAGE = 17,
  DIALOG_SAVEIMAGE = 18,
  DIALOG_ADDFONT = 19,
  DIALOG_SAVEFONT = 20,
  DIALOG_LOADGRADIENTCURVES = 21,
  DIALOG_SAVEGRADIENTCURVES = 22,
  DIALOG_LOADTILINGANIM = 23,
  DIALOG_SAVETILINGANIM = 24,
  DIALOG_LOADTILEMAP = 25,
  DIALOG_SAVETILEMAP = 26,
  DIALOG_SELECTGALLERYFOLDER = 27,
  DIALOG_IMPORTFOLDER = 28,
  DIALOG_LOADCOLORMATRIX = 29,
  DIALOG_SAVECOLORMATRIX = 30,
  DIALOG_COUNT = 31
};

void dialogCanvas(bool openIt);
void dialogImageResize(bool openIt);
void dialogExpandShrinkSelection(bool openIt);
void dialogDeletePicture(bool openIt);
void dialogAddLayer(bool openIt);
void dialogDeleteLayers(bool openIt);
void dialogSelectByAlpha(bool openIt);
void dialogBakeLayers(bool openIt);
void dialogNewImage(bool openIt);
void dialogGetLayersFromPicture(bool openIt);
void dialogUpdateLayer(bool openIt);
void dialogAbout(bool openIt);
void dialogDocumentation(bool openIt);
void dialogNewHere(bool openIt);
void dialogSuite(bool openIt);
void dialogExit(bool openIt);
void dialogDuplicate(bool openIt);
void dialogMergeDown(bool openIt);
void dialogInsertSVG(bool openIt,int width, int height);
void dialogExportLayersRaw(bool openIt);
void dialogPictureInfo(bool openIt);
void dialogGradiationCurves(bool openIt);
void dialogColorBalance(bool openIt);
void dialogBlur(bool openIt);
void dialogGlow(bool openIt);
void dialogEmboss(bool openIt);
void dialogHotkeys(bool openIt);
void dialogColorMatrix(bool openIt);
void dialogQuantizePalette(bool openIt);
void dialogExtrudeSelection(bool openIt);
void dialogKeying(bool openIt);
void dialogMonitorGamma(bool openIt);
void dialogInsertTileMapLayer(bool openIt);
void dialogImportFolder(bool openIt);

void dialogOverwrite(bool openIt);
void dialogCreateFolder(bool openIt);
void dialogLoadSave(bool openIt, int dialogMode);

extern class ColorTransform *gradientCurvesToColorTransform;
void stringToCurves(const String &string);
String stringFromCurves();
void getGradingArrayFromCurves(unsigned char *dest);

void sortFileNames(Array<File> &files, int column, bool sortOrder);

#define S_DIALOGS_CPP()

#endif // __DIALOGS_HPP__
