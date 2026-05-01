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
#ifndef __TOOLBAR_HPP__
#define __TOOLBAR_HPP__

extern int toolbar_width;

enum TOOL {
  TOOL_NONE = 0,
  TOOL_SELECTION = 1,
  TOOL_PENCIL = 2,
  TOOL_PAINTBRUSH = 3,
  TOOL_ERASER = 4,
  TOOL_WAND = 5,
  TOOL_MOVE = 6,
  TOOL_GRADIENT = 7,
  TOOL_PIPETTE = 8,
  TOOL_MAGIC = 9,
  TOOL_FILL = 10,
  TOOL_LINE = 11,
  TOOL_TEXT = 12,
  TOOL_MEASURE = 13,
  TOOL_TILING = 14,
  TOOL_COUNT = 15
};
extern const char *toolNames[TOOL_COUNT];

extern int tool;
extern int lastTool;
extern bool fullScreen;

void initToolBar();
void freeToolBar();
void drawToolBar();

#define S_TOOLBAR_CPP()\
  S_INT("tool",tool);\
  S_BOOL("fullScreen",fullScreen);

#endif // __TOOLBAR_HPP__
