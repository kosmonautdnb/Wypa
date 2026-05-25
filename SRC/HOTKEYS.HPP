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
#ifndef __HOTKEYS_HPP__
#define __HOTKEYS_HPP__

#define HOTKEY_TOOL_SELECTION HOTKEY('s',false,false, "<s> Selection tool")
#define HOTKEY_TOOL_MOVE HOTKEY('m',false,false,"<m> Move tool")
#define HOTKEY_TOOL_WAND HOTKEY('w',false,false,"<w> Magic wand tool")
#define HOTKEY_TOOL_PIPETTE HOTKEY('p',false,false,"<p> Pipette tool")
#define HOTKEY_TOOL_PENCIL HOTKEY('h',false,false,"<h> Pencil tool")
#define HOTKEY_TOOL_PAINTBRUSH HOTKEY('b',false,false,"<b> Paintbrush tool")
#define HOTKEY_TOOL_ERASER HOTKEY('e',false,false,"<e> Eraser tool")
#define HOTKEY_TOOL_FILL HOTKEY('f',false,false,"<f> Fill tool")
#define HOTKEY_TOOL_LINE HOTKEY('l',false,false,"<l> Line tool")
#define HOTKEY_TOOL_GRADIENT HOTKEY('g',false,false,"<g> Gradient tool")
#define HOTKEY_TOOL_TEXT HOTKEY('t',false,false,"<t> Text tool")
#define HOTKEY_TOOL_MAGIC HOTKEY('c',false,false,"<c> Postprocess tool")
#define HOTKEY_TOOL_BLUR HOTKEY('r',false,false,"<r> Blur tool")
#define HOTKEY_TOOL_MEASURE HOTKEY('o',false,false,"<o> Measure tool")
#define HOTKEY_TOOL_TILING HOTKEY('i',false,false,"<i> Tiling tool")
#define HOTKEY_TOOL_OVERLAYS HOTKEY('d',false,false,"<d> Overlay tool")
#define HOTKEY_TOGGLE_PALETTE HOTKEY('q',false,false,"<q> Palette/colorwheel")
#define HOTKEY_LAYERUNDERMOUSE HOTKEY('a',false,false,"<a> Select layer under mouse")
#define HOTKEY_SELECTNONE HOTKEY('N',false,false,"SHIFT<N> Select none")
#define HOTKEY_SELECTALL HOTKEY('A',false,false,"SHIFT<A> Select all")
#define HOTKEY_INVERTSELECTION HOTKEY('I',false,false,"SHIFT<I> Invert selection")
#define HOTKEY_COPY HOTKEY(GL_VK_C_CTRL,true,false,"CTRL<c> Copy")
#define HOTKEY_PASTE HOTKEY(GL_VK_V_CTRL,true,false,"CTRL<v> Paste")
#define HOTKEY_FITCANVAS HOTKEY('F',false,false,"SHIFT<F> Fit image into view")
#define HOTKEY_ZOOMIN HOTKEY('+',false,false,"<+> Zoom in")
#define HOTKEY_ZOOMOUT HOTKEY('-',false,false,"<-> Zoom out")
#define HOTKEY_ZOOMPAN HOTKEY(' ',false,false,"ALT pan picture")
#define HOTKEY_MOUSELOCKX HOTKEY('x',false,false,"<x> Lock mouse on x axis")
#define HOTKEY_MOUSELOCKY HOTKEY('y',false,false,"<y> Lock mouse on y axis")
#define HOTKEY_FULLSCREEN HOTKEY('u',false,false,"<u> Raw view without panels")
#define HOTKEY_ERASE HOTKEY(GL_VK_DELETE,false,false,"<DEL> Erase selection area")
#define HOTKEY_FILLCOLOR HOTKEY(GL_VK_INSERT,false,false,"<INS> Fill selection area with color")
#define HOTKEY_ZOOM100 HOTKEY('j',false,false,"<j> Zoom to 100% of canvas size")
#define HOTKEY_FITLAYER HOTKEY('k',false,false,"<k> Fit layer coordinates")
#define HOTKEY_MOUSETOGRID HOTKEY('n',false,false,"<n> Mouse to grid")
#define HOTKEY_SAVEIMAGE HOTKEY(GL_VK_S_CTRL,true,false,"CTRL<s> Save image")

void initHotkeys();
void freeHotkeys();

void applyHotKeys();
void disableNativeInput();

#define S_HOTKEYS_CPP()

#endif // __HOTKEYS_HPP__
