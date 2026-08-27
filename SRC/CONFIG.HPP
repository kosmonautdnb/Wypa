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
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "STL/TYPES.HPP"
#include "STL/STRING.HPP"

#ifndef PI
#define PI 3.14159265358979323846
#endif // PI

#define FROZENWINDOW (ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoCollapse)
#define UNUSED(__p__) (void)(__p__)
#define SCREENSAVERTIMEINSECONDS (60*5)
#define MAXTEXTEDITTEXT (1024)
#define MAXGLYPHSIZE (4096)

#define CONTROLKEY_NONE 0
#define CONTROLKEY_SHIFT 1
#define CONTROLKEY_CTRL 2
#define CONTROLKEY_ALT 4

#ifdef __DJGPP__
#define INTERNALDOCU
#endif

#ifdef __WATCOMC__
#if __WATCOMC__>1100 // WatcomC11 and PModeW together does have a problem with big data in code
#define INTERNALDOCU
#endif
#endif

#define MEMWARNINGMB 16

//#define OTHERPLATFORM
#ifdef _WIN32
#define OTHERPLATFORM
#endif

extern double fps;
extern double timeDelta;
extern double mouseDX,mouseDY;
extern unsigned int mouseB;
extern bool shift,ctrl,alt;
extern const char *appName;
extern String appFolder;
extern unsigned int controlKeyId;
extern int version;
extern bool captureMouse;

extern uint32_t main_window_xres;
extern uint32_t main_window_yres;
extern double aspectRatio;
extern const char *logFileName;
extern bool refreshMasked;
extern double zoomStep;
extern double scrollKeyPixels;

extern bool useUndo;
extern unsigned int undoStageCount;

extern bool debugWindowOn;
void showDebugWindow();

extern bool showToolTips;
void toolTip(const char *fmt, ...);
void toolTip2(const char *fmt, ...);

#define S_CONFIG_CPP()\
  S_DOUBLE("aspectRatio",aspectRatio);\
  if (fabs(aspectRatio-16.0/9.0) < 0.0001) aspectRatio = 16.0/9.0;\
  S_BOOL("useUndo",useUndo);\
  S_BOOL("showToolTips",showToolTips);\
  int wypaVersion = version; S_INT("wypaVersion",wypaVersion);

String combined(const String &c);
extern char tempBuffer[256];

float getFreeMemoryInKb();

extern float luminanceValues[3];

#ifdef OTHERPLATFORM
bool initOtherPlatform();
#endif // OTHERPLATFORM

#endif //__CONFIG_H__
