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
#include "CONFIG.HPP"
#include "IMGUI/IMGUI.H"
#include "STL/STRING.HPP"

double fps = 30.0;
double timeDelta = 0;
double mouseDX = 0, mouseDY = 0;
unsigned int mouseB = 0;
bool shift = false, ctrl = false, alt = false;
unsigned int controlKeyId = 0;
const char *appName = "WYPA";
uint32_t main_window_xres = 1024;
uint32_t main_window_yres = 768;
double aspectRatio = 16.0/9.0;
//double aspectRatio = 4.0/3.0;
const char *logFileName = "WYPA.LOG";
String appFolder;
bool refreshMasked = true;
int version = 10;
double zoomStep = 1.25;
double scrollKeyPixels = main_window_xres/20;
unsigned int undoStageCount = 10;
bool useUndo = true;
