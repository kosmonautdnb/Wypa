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
#define _CRT_SECURE_NO_WARNINGS
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
  static double t = 0; t += timeDelta; t = fmod(t,100); int k = (int)(t * 2);
  ImVec4 color_button_off_bg = ImVec4(0.7f,0.7f,0.7f,1);
  ImVec4 color_button_on_bg = ImVec4((k & 1)*0.5f+0.5f,0.0,0.0,1);
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

void tinyWait();
unsigned int scrollToLayerId = 0;

const int layerThumbWidth = 16;
const int layerThumbHeight = 16;
Array<unsigned int> layerTextureIds;

unsigned int layerPreview = 0;
int layerPreviewWidth = 64;
int layerPreviewHeight = 64;

void initLayerView() {
  glGenTextures(1,&layerPreview); 
  glBindTexture(GL_TEXTURE_2D,layerPreview); 
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, layerPreviewWidth, layerPreviewHeight, 0, GL_RGBA, GL_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);
}

void freeLayerView() {
  for (int i = 0; i < (int)layerTextureIds.size(); i++) {glDeleteTextures(1,&layerTextureIds[i]);} layerTextureIds.clear();
  glDeleteTextures(1,&layerPreview);
}


void updateLayerTexture(Layer *l, unsigned int layerTexture) {
  unsigned int *dest = glGetTexturePointer(layerTexture);
  unsigned int width = glGetTextureWidth(layerTexture);
  unsigned int height = glGetTextureHeight(layerTexture);
  for (int y = 0; y < (int)height; y++) {
    for (int x = 0; x < (int)width; x++) {
      unsigned int col = 0xff000000;
      if (l != NULL) {
        int sx = x * l->width / width;
        int sy = y * l->height / height;
        if (sx >= 0 && sx < l->width && sy >= 0 && sy < l->height) 
          col = l->pixels[sx+sy*l->width];
      }
      dest[x+y*width] = gammaCorrect(col,0,0);
    }
  }
}

void toolTipLayerPreview(Layer *l) {
  if (!showToolTips) return;
  if (l->width > 0 && l->height > 0) {
    double layerPreviewAspect = (double)l->width / l->height;
    updateLayerTexture(l, layerPreview);
    ImGui::BeginTooltip();
    ImGui::Image((ImTextureID)layerPreview,ImVec2((float)layerPreviewWidth,(float)(layerPreviewHeight*aspectRatio*main_window_yres/main_window_xres/layerPreviewAspect)));
    ImGui::EndTooltip();
  }
}

bool visibleLayerSwitch = false;
void maybeBlinkLayer(Layer *l) {
  if (!visibleLayerSwitch) return;
  if (l == NULL || l->width < 1 || l->height < 1) return;
  for (int i = 0; i < l->width*l->height; i++) l->pixelStates[i] |= PIXELSTATE_OUTLINE;
  l->hasOutline = true; blitOutlineSolidOneFrame = true; tinyWait();
  clearOutlineAfterNextPaint = true;
}

bool showSomethingToFitAnim = true;

void drawLayerView() {
  if (singleLayerId != 0) {singleLayerId = 0; shouldUpdateAll = true;}
  if (fullScreen) return;
  ImGui::SetNextWindowPos(ImVec2((float)(main_window_xres-sidebar_width),(float)(menuBar_height+navigator_height+colorField_height)));
  ImGui::Begin("LAYERVIEW", NULL, ImVec2((float)sidebar_width,(float)(main_window_yres-(menuBar_height+navigator_height+colorField_height))),1.0,FROZENWINDOW);

  Layer *currentLayer = currentCanvas->getActiveLayer();
  if (currentLayer != NULL) {
    ImGui::PushItemWidth(7*8+16);
    int mode = currentLayer->mode - 1; if (ImGui::Combo("##DRAWLAYERVIEWTRANSPARENCYTYPE",&mode, &(layerModeNames[1]), LAYERMODE_COUNT-1)) {
      currentLayer->mode = mode + 1;
      repaintLayer(currentLayer,true);
    } else toolTip("Layer paint mode: %s",layerModeNamesFull[currentLayer->mode]);
    if (currentLayer->mode == LAYERMODE_OPMOD) {
      ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),0x40ff8040);
    }
    if (currentLayer->mode == LAYERMODE_PIGMENT) {
      ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),0x40008000);
    }

    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::PushItemWidth((float)(sidebar_width-(7*8+16)-16));
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

    static double time = 0; time += timeDelta;
    static double somethingToFitStartTime = time+1000;
    static bool somethingToFitBefore = false;
    if (somethingToFitBefore != somethingToFit && somethingToFit) somethingToFitStartTime = time;
    somethingToFitBefore = somethingToFit;

    ImGui::SameLine();
    if (blinkingRoundButton("!##LAYERVIEWFITLAYERS",somethingToFit)) {
      beforeUndoPoint();
      if (shift)
        currentLayer->quantizeBoundingBox();
      else
        currentLayer->quantizePosition();
      afterUndoPoint();
    } else {
      if (showSomethingToFitAnim) {
        ImVec2 mn = ImGui::GetItemRectMin();
        ImVec2 mx = ImGui::GetItemRectMax();
        double d = fabs(time - somethingToFitStartTime);
        if (d < 1) {
          double sx = 5*aspectRatio*main_window_yres/main_window_xres; double sy = 5;
          d = 1 - d;
          mn.x -= (float)(d * sx+1); mn.y -= (float)(d * sy+1);
          mx.x += (float)(d * sx); mx.y += (float)(d * sy);
          ImGui::GetWindowDrawList()->AddRect(mn,mx,0xff0000ff,3);
        }
      }
      toolTip("Fit layer coordinates, with <SHIFT> also fit size");
    }
  }

  if (roundButton(currentCanvas->justSingle?"Single off##LAYERVIEWSINGLE":"Single on ##LAYERVIEWSINGLE",true)) {
    currentCanvas->justSingle = !currentCanvas->justSingle;
    shouldUpdateAll = true;
  } else toolTip("Show only the selected layer");
  ImGui::SameLine();
  if (roundButton(currentCanvas->hilightSelection?"Don't darken##LAYERVIEWHILIGHT":"Darken others##LAYERVIEWHILIGHT",true)) {
    currentCanvas->hilightSelection = !currentCanvas->hilightSelection;
    shouldUpdateAll = true;
  } else toolTip("Darken all non selected layer(s)");

#define CELLSIZE 24
#define GROUPCELLSIZE 17
#define CELLPRE 4
  ImGui::BeginChild("LAYERVIEWCHILD",ImVec2((float)sidebar_width,(float)(main_window_yres-ImGui::GetCursorScreenPos().y-20)));

  if (layerTextureIds.size() < currentCanvas->layers.size()) {
    for (int i = (int)layerTextureIds.size(); i < (int)currentCanvas->layers.size(); i++) {
      unsigned int id; glGenTextures(1,&id); 
      glBindTexture(GL_TEXTURE_2D,id); glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, layerThumbWidth, layerThumbHeight, 0, GL_RGBA, GL_BYTE, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glBindTexture(GL_TEXTURE_2D,0);
      layerTextureIds.push_back(id);
    }
  }

  for (int i3 = 0; i3 < (int)currentCanvas->layers.size(); i3++) {
    Layer *l = currentCanvas->layers[i3];
    if (l->state.selected || l->state.active) {currentCanvas->selectedGroup = -1; break;}
  }

  for (int i4 = 0; i4 < (int)currentCanvas->groups.size(); i4++) {
    Group *group = currentCanvas->groups[i4];
    if (group->index > (int)currentCanvas->layers.size()) {group->index = (int)currentCanvas->layers.size();}
    if (group->index < 0) group->index = 0;
  }

  double time = glSeconds();

  int offCounter = 0;
  int onCounter = 0;
#define GROUPINS 10
  for (int i = 0; i < (int)currentCanvas->layers.size()+1; i++) { // there is a break with // !!! below
    for (int j = 0; j < (int)currentCanvas->groups.size(); j++) {
      Group *group = currentCanvas->groups[j];
      if (group->index == i) {
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        cursor.x += GROUPINS;
        ImGui::SetCursorScreenPos(cursor);
        ImGui::Text("%c%02d", group->open ? '{':'}', group->length);
        toolTip("This group is %s and contains %d layers", group->open ? "open" : "closed", group->length);
        ImGui::SameLine();
        ImVec2 c2 = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(cursor.x,cursor.y));
        if (ImGui::InvisibleButton(combined(String("##LAYERVIEWGROUPOPEN")+String::fromInt(j)).c_str(),ImVec2((float)c2.x-cursor.x,(float)20))) {group->open = !group->open; currentCanvas->selectedGroup = j; currentCanvas->deSelectAllLayers();}
        ImGui::SameLine();
        ImGui::SetCursorScreenPos(ImVec2(c2.x,cursor.y));
        bool a = ImGui::GetIO().MouseClicked[0];
        ImGui::GetIO().MouseClicked[0] = (ImGui::GetIO().MouseClicked[0] && (group->imGuiActive))||(ImGui::GetIO().MouseDoubleClicked[0] && (!group->imGuiActive));
        if (ImGui::IsWindowHovered() && currentKey == GL_VK_F2) ImGui::GetIO().MouseClicked[0] = true;
        ImGui::PushItemWidth(7*LAYERNAMEMAX);
        ImGui::InputText(combined(String("##LAYERVIEWGROUPNAME")+String::fromInt(j)).c_str(),group->name,LAYERNAMEMAX); toolTip("Group name");
        ImGui::PopItemWidth();
        ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + GROUPCELLSIZE),0x80ffff00);
        group->imGuiActive = ImGui::IsItemActive();
        ImGui::GetIO().MouseClicked[0] = a;
        ImGui::SameLine(); if (ImGui::Button(combined(String("-##LAYERVIEWGROUPMINUS")+String::fromInt(j)).c_str())) {group->length--; currentCanvas->selectedGroup = j; currentCanvas->deSelectAllLayers();} else toolTip("Exclude the bottom layer from the group");
        ImGui::SameLine(); if (ImGui::Button(combined(String("+##LAYERVIEWGROUPPLUS")+String::fromInt(j)).c_str())) {
          bool canMoveDown = true;
          for (int k = 0; k < (int)currentCanvas->groups.size(); k++) {
            Group *group2 = currentCanvas->groups[k];
            if (group != group2 && group2->index != group->index) {
              if (group->index+group->length <= group2->index && group->index+group->length+1 > group2->index) canMoveDown = false;
            }
          }
          if (canMoveDown) group->length++; 
          currentCanvas->selectedGroup = j; currentCanvas->deSelectAllLayers();
        }  else toolTip("Include another layer into the group");
        if (group->length+group->index>(int)currentCanvas->layers.size()) group->length = (int)currentCanvas->layers.size() - group->index;
        if (group->length<0) group->length = 0;
        if (group->length>99) group->length = 99;
        ImGui::SetCursorScreenPos(ImVec2(cursor.x,cursor.y));
        ImGui::InvisibleButton("##GROUPVIEWINVISIBLEBUTTON",ImVec2((float)sidebar_width,GROUPCELLSIZE));
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && controlKeyId == CONTROLKEY_NONE) {currentCanvas->selectedGroup = j; currentCanvas->deSelectAllLayers();}
        static double startTime = time;
        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(1) && controlKeyId == CONTROLKEY_NONE) {
          unsigned int k = (unsigned int)((time - startTime)*4.0);
          ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + GROUPCELLSIZE),0x800080ff);
          if (group->length != 0) {
            int p = (k % group->length) + group->index;
            if ((unsigned int)p < (unsigned int)currentCanvas->layers.size()) {
              singleLayerId = currentCanvas->layers[p]->id;
              shouldUpdateAll = true;
            }
          }
        } else {startTime = time;}
        if (currentCanvas->selectedGroup == j) {
          ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + GROUPCELLSIZE),0x400000ff);
        }
        if (!group->open) offCounter = group->length; else onCounter = group->length;
      }
    }
    if (i >= (int)currentCanvas->layers.size()) break; // !!!
    bool belongsToGroup = false;
    if (offCounter > 0) {offCounter--; continue;}
    if (onCounter > 0) {onCounter--; belongsToGroup = true;}

    Layer *l = currentCanvas->layers[i];

    ImVec2 cursor = ImGui::GetCursorScreenPos();

    if ( (cursor.y + 64 >= ImGui::GetWindowPos().y) && (cursor.y - 64 < ImGui::GetWindowPos().y + ImGui::GetWindowSize().y))  updateLayerTexture(l, layerTextureIds[i]);

    if (scrollToLayerId != 0) {
      if (l->id == scrollToLayerId) {ImGui::SetScrollPosHere(); scrollToLayerId = 0;}
    }

    SVG *svg = &svgs["visible"];
    ImGui::SetCursorScreenPos(ImVec2(cursor.x,cursor.y+CELLPRE));
    ImGui::Image((ImTextureID)svg->glHandle,ImVec2((float)svg->w,(float)svg->h),ImVec2(0,0),ImVec2(1,1),ImVec4(0,0,0,l->state.visible ? 1:0.5f)); toolTip("Layer visibility");


    ImGui::SetCursorScreenPos(ImVec2(cursor.x,cursor.y+CELLPRE));
    if (ImGui::InvisibleButton(combined(String("##LAYERVIEWCHILDVISIBLE")+String::fromInt(i)).c_str(),ImVec2((float)svg->w,(float)svg->h))) {l->state.visible = !l->state.visible; shouldUpdateAll = true;}

    ImGui::SameLine();
    //ImGui::Checkbox(combined(String("##LAYERVIEWCHILDSELECTED")+String::fromInt(i)).c_str(),&l->state.selected);
    //ImGui::SameLine();
    bool a = ImGui::GetIO().MouseClicked[0];
    ImGui::GetIO().MouseClicked[0] = (ImGui::GetIO().MouseClicked[0] && (l->state.imGuiActive))||(ImGui::GetIO().MouseDoubleClicked[0] && (!l->state.imGuiActive));
    if (ImGui::IsWindowHovered() && currentKey == GL_VK_F2) ImGui::GetIO().MouseClicked[0] = true;
    if (locked(l,LOCK_PERMANENT)) {ImGui::Text(">"); ImGui::SameLine();}
    ImGui::InputText(combined(String("##LAYERVIEWCHILDNAME")+String::fromInt(i)).c_str(),l->name,LAYERNAMEMAX); toolTip("%s%s layer name",locked(l,LOCK_USER) ? "(locked)":"",layerTypeNames[l->type]);
    l->state.imGuiActive = ImGui::IsItemActive();
    ImGui::GetIO().MouseClicked[0] = a;
    ImGui::SetCursorScreenPos(ImVec2(cursor.x,cursor.y));

    ImGui::InvisibleButton("##LAYERVIEWINVISIBLEBUTTON",ImVec2((float)sidebar_width,CELLSIZE));
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && controlKeyId == CONTROLKEY_NONE) {maybeBlinkLayer(l); currentCanvas->setActiveLayer(i);shouldUpdateAll = true;currentCanvas->noSelectedLayers();}
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && controlKeyId == CONTROLKEY_CTRL) {maybeBlinkLayer(l); currentCanvas->copyActiveSelectionToLayer(currentCanvas->getActiveLayerIndex(),i);currentCanvas->setActiveLayer(i);shouldUpdateAll = true;currentCanvas->noSelectedLayers();}
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && controlKeyId == CONTROLKEY_SHIFT) {maybeBlinkLayer(l); l->state.selected = !l->state.selected;Layer *k = currentCanvas->getActiveLayer(); if (k != NULL) k->state.selected = true; shouldUpdateAll = true;}// currentCanvas->setActiveLayer(-1);}
    if (ImGui::IsItemHovered() && ImGui::IsMouseDown(1) && controlKeyId == CONTROLKEY_NONE) {singleLayerId = l->id; shouldUpdateAll = true;}
    if (belongsToGroup) {
      ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(cursor.x+GROUPINS+5,cursor.y),ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x40ffff00);
    }
    if (singleLayerId != 0 && singleLayerId == l->id) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x800080ff);
    }
    if (l->type == LAYERTYPE_ADJUSTMENT) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x40ff00ff);
    }
    if (l->type == LAYERTYPE_TILEMAP) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x4000ffff);
    }
    if (l->mode == LAYERMODE_OPMOD) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x40ff8040);
    }
    if (l->mode == LAYERMODE_PIGMENT) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),l->state.visible ? 0x40008000 : 0xc0c0ffc0);
    }
    if (l->state.active) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x40000000);
    }
    if (l->state.selected) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x200000ff);
    }
    if (locked(l,LOCK_USER)) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x80000000);
    }
    if (ImGui::IsItemHovered() && controlKeyId == CONTROLKEY_CTRL) {
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x40ffff00);
    }
    if (l->toolInfo.blink) {
      l->toolInfo.blink = false;
      ImGui::GetWindowDrawList()->AddRectFilled(cursor,ImVec2(cursor.x+sidebar_width,cursor.y + CELLSIZE),0x8000ff00);
    }
    ImVec2 p0; p0.x = cursor.x+sidebar_width-16-layerThumbWidth; p0.y = cursor.y; 
    ImVec2 p1; p1.x = cursor.x+sidebar_width-16; p1.y = cursor.y + CELLSIZE;
    ImGui::GetWindowDrawList()->AddImage((ImTextureID)layerTextureIds[i],p0,p1,ImVec2(0,0),ImVec2(1,1));
    if (ImGui::IsMouseHoveringRect(p0,p1)) toolTipLayerPreview(l);

    //ImGui::SetCursorScreenPos(ImVec2(cursor.x,cursor.y+CELLSIZE));
    ImGui::Separator();
  }

  bool enableScrollKeys = tool != TOOL_MOVE; // actually tool move should not work instead of this
  if (enableScrollKeys && ImGui::IsMouseHoveringWindow()) {
    int scrollY = (int)ImGui::GetScrollY();
    if (currentKey == GL_VK_DOWN) {scrollY += CELLSIZE;}
    if (currentKey == GL_VK_UP) {scrollY -= CELLSIZE;}
    if (scrollY < 0) scrollY = 0;
    ImGui::SetScrollY((float)scrollY);
  }
  ImGui::EndChild();

  int count = 0; bool one = false; for (int i2 = 0; i2 < (int)currentCanvas->layers.size(); i2++) if (currentCanvas->layers[i2]->state.selected || currentCanvas->layers[i2]->state.active) {one = true; count++;}

  if (currentCanvas->selectedGroup < 0) {
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
  } else {
    if ((unsigned int)currentCanvas->selectedGroup < (unsigned int)currentCanvas->groups.size()) {
      Group *group = currentCanvas->groups[currentCanvas->selectedGroup];
      bool canMoveUp = group->index > 0;
      bool canMoveDown = group->index + group->length < (int)currentCanvas->layers.size();
      if (roundButton("Move up##LAYERVIEWGROUPMOVEUP", canMoveUp)) {
        int stepUp = 1, count = 0; Group *group3 = NULL;
        for (int i2 = 0; i2 < (int)currentCanvas->groups.size(); i2++) {
          Group *group2 = currentCanvas->groups[i2];
          if (group != group2 && group2->index != group->index) {
            if (group->index >= group2->index + group2->length && group->index-1 < group2->index + group2->length) {count++; group3 = group2; stepUp = group3->length;}
          }
        }
        if (count > 1) stepUp = 1;
        if (count == 1) {group3->index += group->length;}
        group->index -= stepUp;
        group->open = true;
        for (int i = group->index+stepUp; i < group->index + group->length + stepUp; i++) {
          if (i >= stepUp && i < (int)currentCanvas->layers.size()) {
            Layer *t = currentCanvas->layers[i-stepUp];
            currentCanvas->layers[i-stepUp] = currentCanvas->layers[i];
            currentCanvas->layers[i] = t;
          }
        }
        shouldUpdateAll = true;
        if ((unsigned int)group->index < (unsigned int)currentCanvas->layers.size()) scrollToLayerId = currentCanvas->layers[group->index]->id;
      } else toolTip("Move the selected group up in hierarchy");
      ImGui::SameLine();
      if (roundButton("Move down##LAYERVIEWGROUPMOVEDOWN", canMoveDown)) {
        int stepDown = 1, count = 0; Group *group3 = NULL;
        for (int i2 = 0; i2 < (int)currentCanvas->groups.size(); i2++) {
          Group *group2 = currentCanvas->groups[i2];
          if (group != group2 && group2->index != group->index) {
            if (group->index+group->length <= group2->index && group->index+group->length+1 > group2->index) {count++; group3 = group2; stepDown = group3->length;}
          }
        }
        if (count > 1) stepDown = 1;
        if (count == 1) {group3->index -= group->length;}
        group->index += stepDown;
        group->open = true;
        for (int i = group->index + group->length - stepDown-1; i >= group->index-stepDown; i--) {
          if (i >= 0 && i < (int)currentCanvas->layers.size()-stepDown) {
            Layer *t = currentCanvas->layers[i+stepDown];
            currentCanvas->layers[i+stepDown] = currentCanvas->layers[i];
            currentCanvas->layers[i] = t;
          }
        }
        shouldUpdateAll = true;
        if ((unsigned int)group->index < (unsigned int)currentCanvas->layers.size()) scrollToLayerId = currentCanvas->layers[group->index]->id;
      } else toolTip("Move the selected group down in hierarchy");
    }
  }


  ImGui::End();

}
