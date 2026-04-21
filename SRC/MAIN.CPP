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
#include "GL.H"
#include "IMGUIGL.HPP"
#include "UTIL\LOG.HPP"
#include "CONFIG.HPP"
#include "DOS.HPP"
#include "STL/IMAGE.HPP"
#include "MENUBAR.HPP"
#include "SVG.HPP"
#include "LAYERS.HPP"
#include "SERIALZE.HPP"
#include "NAVIGTOR.HPP"
#include "COLORFLD.HPP"
#include "LAYERV.HPP"
#include "DIALOGS.HPP"
#include "TOOLBAR.HPP"
#include "TOOLVIEW.HPP"
#include "TOOLS.HPP"
#include <math.h>

bool roundButton(const char *name, bool enabled) {
  ImVec4 color_button_off_bg = ImVec4(0.7,0.7,0.7,1);
  ImVec4 color_button_on_bg = ImVec4(0.0,0.0,0.0,1);
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

double screenSaverStartTime;
double lastTime;

void perFrame() {
  double time = glSeconds(); timeDelta = time - lastTime; lastTime = time;
  static double fpsTimer = 0; static int fpsCounter = 0; fpsTimer += timeDelta; fpsCounter++; if (fpsTimer>2.0) {fps = fpsCounter/2.0;fpsTimer=0;fpsCounter=0;}
  mouseDX = mouseDeltaX*mouseSpeed; mouseDY = mouseDeltaY*mouseSpeed;
  mouseB = 0;
  mouseB |= mouseButtons & GL_MOUSE_BUTTON_LEFT ? 1 : 0;
  mouseB |= mouseButtons & GL_MOUSE_BUTTON_RIGHT ? 2 : 0;
  GLboolean shift2,ctrl2,alt2; glSpecialKeys(&shift2,&ctrl2,&alt2); shift = shift2 ? true : false;  ctrl = ctrl2 ? true : false; alt = alt2 ? true : false;
  if (currentKey != 0 || shift || ctrl || alt || mouseDX != 0 || mouseDY != 0 || mouseB != 0) screenSaverStartTime = glSeconds();
  controlKeyId = (shift ? CONTROLKEY_SHIFT : 0)+(ctrl ? CONTROLKEY_CTRL : 0)+(alt ? CONTROLKEY_ALT : 0);
}

void select(Layer *l) {
  for (int i = 0; i < l->width * l->height; i++) {
    unsigned int rgba = l->pixels[i];
    int r = rgba & 255;
    int g = (rgba>>8) & 255;
    int b = (rgba>>16) & 255;
    int k = (r+g+b)/3;
    if (k<128) l->pixelStates[i] |= PIXELSTATE_SELECTED; else l->pixelStates[i] &= ~PIXELSTATE_SELECTED;
  }
}

void disableRenderMouseClicks() {
}

ImVec2 normalize(const ImVec2 &v) {
  ImVec2 r = v;
  double l = sqrt(r.x*r.x+r.y*r.y);
  if (l != 0) {r.x /= l; r.y /= l;}
  return r;
}

void screenSaver() {
  if (glSeconds()-screenSaverStartTime >= SCREENSAVERTIMEINSECONDS) {
    static ImVec2 textPos(0,0);
    static ImVec2 textDir = normalize(ImVec2(1,1));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,1));
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::Begin("SCREENSAVER", NULL, ImVec2(main_window_xres,main_window_yres),0.95,FROZENWINDOW);
    String date = "Date " + timeDescString();float fontSize = 2.0;
    ImVec2 textSize = ImGui::CalcTextSize(date.c_str()); textSize.x *= fontSize; textSize.y *= fontSize;
    ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowFont(),ImGui::GetWindowFontSize()*fontSize,ImVec2(textPos.x,textPos.y),0xffffffff,date.c_str());
    textPos.x += textDir.x * timeDelta * main_window_xres / 10;
    textPos.y += textDir.y * timeDelta * main_window_xres / 10;
    mouseX = textPos.x;
    mouseY = textPos.y;
    if (textPos.x > main_window_xres-textSize.x) {
      textDir.x = -(double)rand()/RAND_MAX-1.1;
      textDir.y = (double)rand()/RAND_MAX*2-1;
      textDir = normalize(textDir);
    }
    if (textPos.x < 0) {
      textDir.x = (double)rand()/RAND_MAX+1.1;
      textDir.y = (double)rand()/RAND_MAX*2-1;
      textDir = normalize(textDir);
    }
    if (textPos.y > main_window_yres-textSize.y) {
      textDir.x = (double)rand()/RAND_MAX*2-1;
      textDir.y = -(double)rand()/RAND_MAX-1.1;
      textDir = normalize(textDir);
    }
    if (textPos.y < 0) {
      textDir.x = (double)rand()/RAND_MAX*2-1;
      textDir.y = (double)rand()/RAND_MAX+1.1;
      textDir = normalize(textDir);
    }
    ImGui::End();
    ImGui::PopStyleColor();
  }
}

extern Array<Canvas *> canvas;

#define SETUPENTITIES(__prefix__) \
  __prefix__##ColorField();   \
  __prefix__##Navigator();\
  __prefix__##MenuBar();\
  __prefix__##Dialogs();\
  __prefix__##ToolBar();\
  __prefix__##ToolView();\
  __prefix__##LayerView();\
  __prefix__##Layers();\
  __prefix__##Tools();


void deleteAll() {
  SETUPENTITIES(free);
}

bool quit = false;

int main(int argc, const char *argv[]) {

  appFolder = unixFileName(getPath(argv[0]));
  //appFolder = getCWD();

  printf("Wypa version: %d.%02d", version / 100, version % 100);
  printf("\n");
  printf("You can find the newest version of Wypa at:\n");
  printf("www.github.com/kosmonautdnb/wypa\n");
  printf("\n");

  glWatcomPrecisionTimer(GL_TRUE);

  if (!glVesa(main_window_xres,main_window_yres,32)) {
    printf("Error 1\n");
    exit(0);
  }

  glClearColor(0.6,0.8,1.0,0);
  glClear(GL_COLOR_BUFFER_BIT);

  InitImGui(glFrameBufferWidth,glFrameBufferHeight);
  ImGui::GetStyle().WindowRounding = 0.f;
  glFastTexturing = true;
  screenSaverStartTime = glSeconds();

  ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(1,1,1,1);
  ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(0,0,0,1);
  ImGui::GetStyle().Colors[ImGuiCol_Button] = ImVec4(0.75,0.75,0.75,1);
  ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = ImVec4(0.5,0.5,1.0,1);
  ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = ImVec4(1,1,1,1);
  ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = ImVec4(0.9,0.9,0.9,1);
  ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = ImVec4(0,0,0,1.f);
  ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2,0.2,0.2,1.f);
  ImGui::GetStyle().Colors[ImGuiCol_SliderGrab] = ImVec4(0.2,0.2,0.2,1.f);
  ImGui::GetStyle().Colors[ImGuiCol_TooltipBg] = ImVec4(1,1,1,0.8);
  loadBaseSVGs();
  screenSaverStartTime = glSeconds();
  lastTime = glSeconds();

  addTempPicture();
  currentCanvas->control.setCanvasTransform(currentCanvas);
  shouldUpdateAll = true;
  SETUPENTITIES(init);

  while(!quit) {

    if (currentKey == GL_VK_ESCAPE) quit = true;
    perFrame();


    if (isPaintAreaActive()) {
      if (currentKey == '-') {
        currentCanvas->control.zoomBy(currentCanvas,mouseX,mouseY,1.0/zoomStep);
      }
  
      if (currentKey == '+') {
        currentCanvas->control.zoomBy(currentCanvas,mouseX,mouseY,zoomStep);
      }
  
      if (controlKeyId == CONTROLKEY_ALT) {
        currentCanvas->control.moveByPixels(currentCanvas,mouseDX,mouseDY);
      }

      bool enableScrollKeys = tool != TOOL_MOVE;      
      if (enableScrollKeys && currentKey == GL_VK_LEFT) currentCanvas->control.moveByPixels(currentCanvas,scrollKeyPixels,0);
      if (enableScrollKeys && currentKey == GL_VK_RIGHT) currentCanvas->control.moveByPixels(currentCanvas,-scrollKeyPixels,0);
      if (enableScrollKeys && currentKey == GL_VK_UP) currentCanvas->control.moveByPixels(currentCanvas,0,scrollKeyPixels);
      if (enableScrollKeys && currentKey == GL_VK_DOWN) currentCanvas->control.moveByPixels(currentCanvas,0,-scrollKeyPixels);
    }

    useTool();

    if (shouldUpdateAll) {
      currentCanvas->control.setCanvasTransform(currentCanvas);
    }

    //select();
    blitCanvas();
    
    ImGuiNewFrame();
    drawMenuBar();
    drawNavigator();
    drawColorField();
    drawLayerView();
    drawToolBar();
    drawToolView();
    screenSaver();
    ImGui::Render();
      
    if (refreshMasked) glRefreshMasked(); else glRefresh();
  }

  glDone();

  printf("Wypa version %d.%02d!\n",version / 100, version % 100);
  printf("-----------------------\n");
  printf("A tiny tool by Puls4r productions.\n");
  printf("This experimental DOS application\n");
  printf("is just designed for simple painting\n");
  printf("It's by far not a professional tool.\n");
  printf("If you just need to paint a picture\n");
  printf("for a game and alikes (thus pixelling)\n");
  printf("maybe it already suffices.\n");
  printf("It's freeware and can be found here:\n");
  printf("www.github.com/kosmonautdnb/wypa\n");
  printf("-----------------------\n");
  printf("Have a good day and maybe till next time.\n");

  deleteAll();

  return 0;
}
