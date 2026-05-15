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
#include "TOOLBAR.HPP"
#include "IMGUI/IMGUI.H"
#include "MENUBAR.HPP"
#include "NAVIGTOR.HPP"
#include "CONFIG.HPP"
#include "SVG.HPP"
#include "DIALOGS.HPP"
#include "TOOLVIEW.HPP"
#include "LAYERS.HPP"

int toolbar_width = 32;
int toolbar_bottom_height = 32;
int tool = TOOL_NONE;
int lastTool = TOOL_NONE;
bool fullScreen = false;

const char *toolNames[TOOL_COUNT] = {"no tool","Selection","Pencil","Paintbrush","Eraser","Magic wand","Move","Gradient","Pipette","Postprocessing","Fill","Line","Text","Measure","Tiling","Blur","Overlays","Modify"};

void initToolBar() {
}

void freeToolBar() {
}

void switchTool(int newTool) {
  lastTool = tool; tool = newTool;
}

bool imageButton(const char *svgName, const char *id, bool selected, bool hilighted, bool disabled) {
  SVG *svg = &svgs[svgName];
  ImGui::PushID(id);
  ImVec4 col = selected ? ImVec4(0.5,0.5,0.5,1) : ImVec4(1,1,1,1);
  bool ret = ImGui::ImageButton((ImTextureID)svg->glHandle,ImVec2(svg->w,svg->h),ImVec2(0,0),ImVec2(1,1),0,col);
  if (hilighted) {
    ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),0x800000ff);
  }
  if (selected) ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),0xc0ffffff);
  if (disabled) ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),0x40000000);
  if (ImGui::IsItemHovered()) {
    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),0x20000000);
  }
  ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),0x20000000);
  ImGui::PopID();
  return ret;
}

bool toolBarButton(const char *svgName, int toolIndex) {
  bool highlighted = false;
  bool disabled = false;
  bool selected = toolIndex == tool;
  Layer *l = currentCanvas->getActiveLayer();
  if (toolIndex == TOOL_MAGIC) {
    if (l != NULL) {
      if (l->colorTransform.useColorTransform) highlighted = true;
      if (l->colorTransform.usePosterize) highlighted = true;
      if (l->colorTransform.useQuantization) highlighted = true;
    }
  }
  if (toolIndex == TOOL_TEXT) {
    Layer *l = currentCanvas->getActiveLayer();
    if (l != NULL) {
      disabled = l->type != LAYERTYPE_TEXT;
    }
  }
  if (l != NULL && l->type == LAYERTYPE_ADJUSTMENT) {
    if (toolIndex == TOOL_MOVE || toolIndex == TOOL_MAGIC) {
    } else {
      disabled = true;
    }
  }
  if (selected && disabled) {switchTool(TOOL_NONE);}
  bool ret = imageButton(svgName, combined(String("##TOOLBARBUTTONINDEXI")+String::fromInt(toolIndex)).c_str(), selected, highlighted, disabled);
  if (ret && !disabled) {switchTool(toolIndex);}
  return ret;
}

void drawToolBar() {
  ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0.7,0.7,0.7,1));
  if (!fullScreen) {
    ImGui::SetNextWindowPos(ImVec2(0,menuBar_height));
    ImGui::Begin("TOOLBAR", NULL, ImVec2(toolbar_width,main_window_yres-menuBar_height-toolbar_bottom_height),1.0,FROZENWINDOW);
    toolBarButton("selectionTool",TOOL_SELECTION);
    toolBarButton("moveTool",TOOL_MOVE);
    toolBarButton("wandTool",TOOL_WAND);
    toolBarButton("pipetteTool",TOOL_PIPETTE);
    toolBarButton("pencilTool",TOOL_PENCIL);
    toolBarButton("paintBrushTool",TOOL_PAINTBRUSH);
    toolBarButton("eraserTool",TOOL_ERASER);
    toolBarButton("fillTool",TOOL_FILL);
    toolBarButton("lineTool",TOOL_LINE);
    toolBarButton("gradientTool",TOOL_GRADIENT);
    toolBarButton("textTool",TOOL_TEXT);
    toolBarButton("magicTool",TOOL_MAGIC);
    toolBarButton("blurTool",TOOL_BLUR);
    toolBarButton("measureTool",TOOL_MEASURE);
    toolBarButton("tilingTool",TOOL_TILING);
    toolBarButton("overlaysTool",TOOL_OVERLAYS);
    toolBarButton("modifyTool",TOOL_MODIFY);
    ImGui::End();
  }
  if (lastTool != tool) {toolChangeMessage(lastTool,tool); lastTool = tool;}
  if (fullScreen) {
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0,0,0,1));
  }
  ImGui::SetNextWindowPos(ImVec2(0,main_window_yres-toolbar_bottom_height));
  ImGui::Begin("TOOLBARBOTTOM", NULL, ImVec2(toolbar_width,toolbar_bottom_height),1.0,FROZENWINDOW);
  ImGui::PopStyleColor();
  drawDialogs();
  ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0.7,0.7,0.7,1));
  ImGui::Checkbox("##TOOLBARBOTTOMFULLSCREEN",&fullScreen);
  ImGui::End();
  ImGui::PopStyleColor();
}
