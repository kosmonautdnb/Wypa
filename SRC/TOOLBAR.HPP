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
  TOOL_COUNT = 7
};
extern const char *toolNames[TOOL_COUNT];

extern int tool;

void initToolBar();
void drawToolBar();

#endif // __TOOLBAR_HPP__
