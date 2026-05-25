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
#include "STL/GL.H"
#include "IMGUI/IMGUIGL.HPP"
#include "LAYERV.HPP"
#include "COLORFLD.HPP"
#include "NAVIGTOR.HPP"
#include "CONFIG.HPP"
#include "MENUBAR.HPP"
#include "LAYERS.HPP"
#include "DIALOGS.HPP"
#include "SVG.HPP"
#include "TOOLBAR.HPP"
#include "TOOLVIEW.HPP"
#include "UNDO.HPP"
#include <math.h>

bool roundButton(const char *name, bool enabled);

bool blinkingRoundButton(const char *name, bool enabled) {
  static double t = 0; t += timeDelta; t = fmod(t,100); int k = t * 2;
  ImVec4 color_button_off_bg = ImVec4(0.7,0.7,0.7,1);
  ImVec4 color_button_on_bg = ImVec4((k & 1)*0.5+0.5,0.0,0.0,1);
  ImVec4 color_button_off_text = ImVec4(0,0,0,1);
  ImVec4 color_button_on_text = ImVec4(1,1,1,1);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5,0));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3);
  ImGui::PushStyleColor(ImGuiCol_Button,enabled ? color_button_on_bg : color_button_off_bg);
  ImGui::PushStyleColor(ImGuiCol_Text,enabled ? color_button_on_text : color_button_off_text);
  bool ret = ImGui::Button(name);
  if (!enabled) ret = false;
  ImGui::PopStyleColor();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
  ImGui::PopStyleVar();
  return ret;
}


unsigned int scrollToLayerId = 0;

void initLayerView() {
}

void freeLayerView() {
}

void drawLayerView() {
  if (fullScreen) return;
  ImGui::SetNextWindowPos(ImVec2(main_window_xres-sidebar_width,menuBar_height+navigator_height+colorField_height));
  ImGui::Begin("LAYERVIEW", NULL, ImVec2(sidebar_width,main_window_yres-(menuBar_height+navigator_height+colorField_height)),1.0,FROZENWINDOW);

  Layer *currentLayer = currentCanvas->getActiveLayer();
  if (currentLayer != NULL) {
    ImGui::PushItemWidth(7*8+16);
    int mode = currentLayer->mode - 1; if (ImGui::Combo("##DRAWLAYERVIEWTRANSPARENCYTYPE",&mode, &(layerModeNames[1]), LAYERMODE_COUNT-1)) {
      currentLayer->mode = mode + 1;
      repaintLayer(currentLayer,true);
    } else toolTip("Layer paint mode");
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::PushItemWidth(sidebar_width-(7*8+16)-16);
    float alpha = currentLayer->state.alpha; if (ImGui::SliderFloat("##DRAWLAERVIEWTRANSPARENCYALPHA",&alpha,0,1)) {
      if (alpha < 0) alpha = 0;
      if (alpha > 1) alpha = 1;
      currentLayer->state.alpha = alpha;
      repaintLayer(currentLayer,true);
    } else toolTip("Layer opacity");
    ImGui::PopItemWidth();
  }

  if (roundButton("Add layer##LAYERVIEWADDLAYER",true)) {
    dialogAddLayer(true);
  } else toolTip("Add a picture sized layer");
  ImGui::SameLine();
  if (roundButton("Bake layers##LAYERVIEWBAKELAYERS",true)) {
    dialogBakeLayers(true);
  } else toolTip("Bake/rectify current layer");
  if (currentLayer != NULL) {
    Vector b = currentLayer->boundingBox();
    bool somethingToFit = notInt(b.x)||notInt(b.y)||notInt(b.z)||notInt(b.w);
    ImGui::SameLine();
    if (blinkingRoundButton("!##LAYERVIEWFITLAYERS",somethingToFit)) {
      beforeUndoPoint();
      currentLayer->quantizeBoundingBox();
      afterUndoPoint();
    } else toolTip("Fit layer coordinates");
  }

  if (roundButton(currentCanvas->justSingle?"Single off##LAYERVIEWSINGLE":"Single on ##LAYERVIEWSINGLE",true)) {
    currentCanvas->justSingle = !currentCanvas->justSingle;
    shouldUpdateAll = true;
  } else toolTip("Show only the selected layer");
  ImGui::SameLine();
  if (roundButton(currentCanvas->hilightSelection?"No highlight##LAYERVIEWHILIGHT":"Hilight layers##LAYERVIEWHILIGHT",true)) {
    currentCanvas->hilightSelection = !currentCanvas->hilightSelection;
    shouldUpdateAll = true;
  } else toolTip("Darken all non selected layer(s)");

#define CELLSIZE 24
#define CELLPRE 4
  ImGui::BeginChild("LAYERVIEWCHILD",ImVec2(sidebar_width,main_window_yres-ImGui::GetCursorScreenPos().y-20));
  for (int i = 0; i < currentCanvas->layers.size(); i++) {
    Layer *l = currentCanvas->layers[i];
    ImVec2 cursor = ImGui::GetCursorScreenPos();

    if (scrollToLayerId != 0) {
      if (l->id == scrollToLayerId) {ImGui::SetScrollPosHere(); scrollToLayerId = 0;}
    }

    SVG *svg = &svgs["visible"];
    ImGui::SetCursorScreenPos(ImVec2(cursor.x,cursor.y+CELLPRE));
    ImGui::Image((ImTextureID)svg->glHandle,ImVec2(svg->w,svg->h),ImVec2(0,0),ImVec2(1,1),ImVec4(0,0,0,l->state.visible ? 1:0.5)); toolTip("Layer visibility");


    ImGui::SetCursorScreenPos(ImVec2(cursor.x,cursor.y+CELLPRE));
    if (ImGui::InvisibleButton(combined(String("##LAYERVIEWCHILDVISIBLE")+String::fromInt(i)).c_str(),ImVec2(svg->w,svg->h))) {l->state.visible = !l->state.visible; shouldUpdateAll = true;}

    ImGui::SameLine();
    //ImGui::Checkbox(combined(String("##LAYERVIEWCHILDSELECTED")+String::fromInt(i)).c_str(),&l->state.selected);
    //ImGui::SameLine();
    bool a = ImGui::GetIO().MouseClicked[0];
    ImGui::GetIO().MouseClicked[0] = (ImGui::GetIO().MouseClicked[0] && (l->state.imGuiActive))||(ImGui::GetIO().MouseDoubleClicked[0] && (!l->state.imGuiActive));
    if (l->state.permanentlyLocked) {ImGui::Text(">"); ImGui::SameLine();}
    ImGui::InputText(combined(String("##LAYERVIEWCHILDNAME")+String::fromInt(i)).c_str(),l->name,LAYERNAMEMAX); toolTip("Layer name");
    l->state.imGuiActive = ImGui::IsItemActive();
    ImGui::GetIO().MouseClicked[0] = a;
    ImGui::SetCursorScreenPos(ImVec2(cursor.x,cursor.y));

    ImGui::InvisibleButton("##LAYERVIEWINVISIBLEBUTTON",ImVec2(sidebar_width,CELLSIZE));
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && controlKeyId == CONTROLKEY_NONE) {currentCanvas->setActiveLayer(i);shouldUpdateAll = true;currentCanvas->noSelectedLayers();}
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && controlKeyId == CONTROLKEY_CTRL) {currentCanvas->copyActiveSelectionToLayer(currentCanvas->getActiveLayerIndex(),i);currentCanvas->setActiveLayer(i);shouldUpdateAll = true;currentCanvas->noSelectedLayers();}
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && controlKeyId == CONTROLKEY_SHIFT) {l->state.selected = !l->state.selected;Layer *k = currentCanvas->getActiveLayer(); if (k != NULL) k->state.selected = true; shouldUpdateAll = true;}// currentCanvas->setActiveLayer(-1);}
    if (l->type == LAYERTYPE_ADJUSTMENT) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x40ff00ff);
    }
    if (l->type == LAYERTYPE_TILEMAP) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x4000ffff);
    }
    if (l->state.active) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x40000000);
    }
    if (l->state.selected) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x200000ff);
    }
    if (l->state.locked && (!l->state.permanentlyLocked)) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x80000000);
    }
    if (ImGui::IsItemHovered() && controlKeyId == CONTROLKEY_CTRL) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x40ffff00);
    }
    if (l->toolInfo.blink) {
      l->toolInfo.blink = false;
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x8000ff00);
    }
    //ImGui::SetCursorScreenPos(ImVec2(cursor.x,cursor.y+CELLSIZE));
    ImGui::Separator();
  }

  bool enableScrollKeys = tool != TOOL_MOVE; // actually tool move should not work instead of this
  if (enableScrollKeys && ImGui::IsMouseHoveringWindow()) {
    int scrollY = ImGui::GetScrollY();
    if (currentKey == GL_VK_DOWN) {scrollY += CELLSIZE;}
    if (currentKey == GL_VK_UP) {scrollY -= CELLSIZE;}
    if (scrollY < 0) scrollY = 0;
    ImGui::SetScrollY(scrollY);
  }
  ImGui::EndChild();

  int count = 0; bool one = false; for (int i2 = 0; i2 < currentCanvas->layers.size(); i2++) if (currentCanvas->layers[i2]->state.selected || currentCanvas->layers[i2]->state.active) {one = true; count++;}

  if (!currentCanvas->layers.empty()) {
    if (roundButton("Move up##LAYERVIEWADMOVEUP",one && (!(currentCanvas->layers[0]->state.selected || currentCanvas->layers[0]->state.active)))) {
      scrollToLayerId = currentCanvas->moveLayersUp();
    } else toolTip("Move selected layers up in hierarchy");
    ImGui::SameLine();
    if (roundButton("Move down##LAYERVIEWADMOVEDOWN",one && (!(currentCanvas->layers[currentCanvas->layers.size()-1]->state.selected || currentCanvas->layers[currentCanvas->layers.size()-1]->state.active)))) {
      scrollToLayerId = currentCanvas->moveLayersDown();
    } else toolTip("Move selected layers down in hierarchy");
    ImGui::SameLine();
    //if (roundButton("Compact##LAYERVIEWADMOVECOMPACT",count>1)) {
    //  currentCanvas->compactLayers();
    //} else toolTip("Selected layers one after another");
    if (roundButton("Merge##LAYERVIEWADMOVEMERGE",count>1)) {
      dialogMergeDown(true);
      //beforeUndoPoint();
      //currentCanvas->mergeDownLayers();
      //afterUndoPoint();
    } else toolTip("Merge the layers onto the lowest selected layer");
  }


  ImGui::End();

}
