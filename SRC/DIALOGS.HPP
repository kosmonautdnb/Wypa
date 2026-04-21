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

void initDialogs();
void freeDialogs();
void drawDialogs();

enum DIALOGMODE {
  DIALOG_NONE = 0,
  DIALOG_LOADIMAGE = 1,
  DIALOG_INSERTIMAGELAYERS = 2,
  DIALOG_EXPORTLAYERS = 3,
  DIALOG_EXPORTPNG = 4,
  DIALOG_LOADPALETTE = 5,
  DIALOG_SAVEPALETTE = 6,
  DIALOG_EXPORTLAYERSRAW = 7,
  DIALOG_INSERTPSD = 8,
  DIALOG_COUNT = 9
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

void dialogOverwrite(bool openIt);
void dialogLoadSave(bool openIt, int dialogMode);

#endif // __DIALOGS_HPP__
