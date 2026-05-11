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
#include "HOTKEYS.HPP"
#include "IMGUI/IMGUI.H"
#include "IMGUI/IMGUIGL.HPP"
#include "TOOLS.HPP"
#include "CONFIG.HPP"
#include "TOOLBAR.HPP"
#include "COLORFLD.HPP"
#include "LAYERS.HPP"
#include "LAYERV.HPP"
#include "UNDO.HPP"

#define HOTKEY(__char__,__ctrl__,__alt__,__text__) ((__char__) | (((__ctrl__?CONTROLKEY_CTRL:0)|(__alt__?CONTROLKEY_ALT:0))<<16))

bool hotKeysOff = false;
void disableNativeInput() {
  hotKeysOff = true;
}

bool isHotKeyPossible() {
  return !ImGui::GetIO().WantCaptureKeyboard;
}


void initHotkeys() {
}

void freeHotkeys() {
}

void applyHotKeys() {
  if (hotKeysOff) {hotKeysOff = false; return;}
  if (!isHotKeyPossible()) return;
  if (ImGui::IsMouseDown(0)) return;
  if (ImGui::IsMouseDown(1)) return;
  unsigned int currentHotKey = currentKey | ((controlKeyId&(~CONTROLKEY_SHIFT))<<16);
  switch(currentHotKey) {
    case HOTKEY_TOOL_SELECTION: {switchTool(TOOL_SELECTION);} break;
    case HOTKEY_TOOL_MOVE: {switchTool(TOOL_MOVE);} break;
    case HOTKEY_TOOL_WAND: {switchTool(TOOL_WAND);} break;
    case HOTKEY_TOOL_PIPETTE: {switchTool(TOOL_PIPETTE);} break;
    case HOTKEY_TOOL_PENCIL: {switchTool(TOOL_PENCIL);} break;
    case HOTKEY_TOOL_PAINTBRUSH: {switchTool(TOOL_PAINTBRUSH);} break;
    case HOTKEY_TOOL_ERASER: {switchTool(TOOL_ERASER);} break;
    case HOTKEY_TOOL_FILL: {switchTool(TOOL_FILL);} break;
    case HOTKEY_TOOL_LINE: {switchTool(TOOL_LINE);} break;
    case HOTKEY_TOOL_GRADIENT: {switchTool(TOOL_GRADIENT);} break;
    case HOTKEY_TOOL_TEXT: {switchTool(TOOL_TEXT);} break;
    case HOTKEY_TOOL_MAGIC: {switchTool(TOOL_MAGIC);} break;
    case HOTKEY_TOOL_BLUR: {switchTool(TOOL_BLUR);} break;
    case HOTKEY_TOOL_MEASURE: {switchTool(TOOL_MEASURE);} break;
    case HOTKEY_TOOL_TILING: {switchTool(TOOL_TILING);} break;
    case HOTKEY_TOOL_OVERLAYS: {switchTool(TOOL_OVERLAYS);} break;
    case HOTKEY_TOGGLE_PALETTE: {showPalette = !showPalette;} break;
    case HOTKEY_LAYERUNDERMOUSE: {currentCanvas->setActiveLayerByScreenPos(mouseX, mouseY); Layer *l = currentCanvas->getActiveLayer(); if (l != NULL) scrollToLayerId = l->id;} break;
    case HOTKEY_SELECTNONE: {currentCanvas->selectNone(true);} break;
    case HOTKEY_SELECTALL: {currentCanvas->selectAll();} break;
    case HOTKEY_INVERTSELECTION: {currentCanvas->invertSelection();} break;
    case HOTKEY_COPY: {currentCanvas->copy();} break;
    case HOTKEY_PASTE: {currentCanvas->paste();} break;
    case HOTKEY_FITCANVAS: {currentCanvas->fitCanvas();} break;
    case HOTKEY_FULLSCREEN: {fullScreen = !fullScreen;} break;
    case HOTKEY_ERASE: {if (currentCanvas->hasSelection()) {beforeUndoPoint();currentCanvas->fillSelectionWithColor(0x00000000);afterUndoPoint();}} break;
    case HOTKEY_FILLCOLOR: {if (currentCanvas->hasSelection()) {beforeUndoPoint();currentCanvas->fillSelectionWithColor(getUIntColor(foregroundColor));afterUndoPoint();}} break;
  }
}
