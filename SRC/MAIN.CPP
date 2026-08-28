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
#include "UTIL/LOG.HPP"
#include "CONFIG.HPP"
#include "STL/DOS.HPP"
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
#include "UNDO.HPP"
#include "TEXT.HPP"
#include "HOTKEYS.HPP"
#include "FILTER.HPP"
#include "GALLERY.HPP"
#include <math.h>
#include <conio.h>
#include "STL/STBIMGWR.HPP" // to patch SRGB gamma for PNG writes

bool roundButton(const char *name, bool enabled) {
  ImVec4 color_button_off_bg = ImVec4(0.7f,0.7f,0.7f,1);
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
  controlKeyId = (shift ? CONTROLKEY_SHIFT : 0)|(ctrl ? CONTROLKEY_CTRL : 0)|(alt ? CONTROLKEY_ALT : 0);
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

ImVec2 normalize(const ImVec2 &v) {
  ImVec2 r = v;
  float l = (float)sqrt(r.x*r.x+r.y*r.y);
  if (l != 0) {r.x /= l; r.y /= l;}
  return r;
}

void turnOnScreenSaver() {
  screenSaverStartTime = glSeconds()-SCREENSAVERTIMEINSECONDS;
}

void drawText(float scale, int xp, int yp, const char *text, uint32_t color, float anchorX, float anchorY);
void textWidthHeight(const char *text, int *w, int *h);

void screenSaver() {
  static ImVec2 textPos(0,0);
  static ImVec2 textDir = normalize(ImVec2(1,1));

  if (glSeconds()-screenSaverStartTime >= SCREENSAVERTIMEINSECONDS) {
    static double k = 0; k += timeDelta; k = fmod(k,2*100); int k2 = (int)(k*4);
    String date = extensiveTimeDescString(); if ((((k2/3)%2) < 1) && date.length()>3) {date[date.length()-3] = '.';}
    float textScale = 4.f;
    int w, h; textWidthHeight(date.c_str(),&w,&h);
    ImVec2 textSize;
    textSize.x = w * textScale;
    textSize.y = h * textScale;
    for (int i = 0; i < glFrameBufferWidth*glFrameBufferHeight; i++) {
      unsigned int col = glFrameBuffer[i]; glFrameBuffer[i] = ((col >> 5) & 0x00070707) | (col & 0xff000000);
    }
    textPos.x += (float)(textDir.x * timeDelta * main_window_xres / 10);
    textPos.y += (float)(textDir.y * timeDelta * main_window_xres / 10);
    if (textPos.x > main_window_xres-textSize.x) {
      textDir.x = (float)( - (double)rand() / RAND_MAX - 1.1);
      textDir.y = (float)((double)rand()/RAND_MAX*2-1);
      textDir = normalize(textDir);
    }
    if (textPos.x < 0) {
      textDir.x = (float)((double)rand()/RAND_MAX+1.1);
      textDir.y = (float)((double)rand()/RAND_MAX*2-1);
      textDir = normalize(textDir);
    }
    if (textPos.y > main_window_yres-textSize.y) {
      textDir.x = (float)((double)rand()/RAND_MAX*2-1);
      textDir.y = (float)(-(double)rand()/RAND_MAX-1.1);
      textDir = normalize(textDir);
    }
    if (textPos.y < 0) {
      textDir.x = (float)((double)rand()/RAND_MAX*2-1);
      textDir.y = (float)((double)rand()/RAND_MAX+1.1);
      textDir = normalize(textDir);
    }
    static int xr = 0; static int yr = 0; static double l = glSeconds(); double l2 = glSeconds();
    if ((int)(l/8) != (int)(l2/8)) {
      int w, h; textWidthHeight("WYPA",&w,&h);
      xr = rand() % (main_window_xres-(int)(w * textScale));
      yr = rand() % (main_window_yres-(int)(h * textScale));
    }
    l = l2;
    drawText(textScale, xr, yr, "WYPA", 0xffffffff,0,0);
    drawText(textScale, (int)textPos.x, (int)textPos.y, date.c_str(), 0xffffffff, 0, 0);
  } else {
    textDir = normalize(ImVec2(1,1));
    textPos = ImVec2(0,0);
  }
}

void whiteFade() {
  static bool whiteFadeOff = false; if (whiteFadeOff) return;
  static double start = glSeconds();
  double duration = 0.25;
  double ratio = fabs(glSeconds() - start) / duration;
  if (ratio > 1) {whiteFadeOff = true; return;}
  int white = (int)(255 - ratio * 255);
  for (int i = 0; i < glFrameBufferWidth*glFrameBufferHeight; i++) {
    unsigned int col = glFrameBuffer[i]; int r = col & 255; int g = (col>>8) & 255; int b = (col>>16) & 255; int a = (col>>24) & 255;
    r += white; g += white; b += white;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    glFrameBuffer[i] = r|(g<<8)|(b<<16)|(a<<24);
  }
}

void splashScreen() {
  static bool splashScreenOn = true;
  static Layer *splash = NULL;
  static GLuint glHandle = 0;
  if (splashScreenOn) {
    if (splash == NULL) {
      splash = loadImage("", fromLocalFileName("./DATA/SPLASH.PNG"));
      if (splash == NULL) {
        splashScreenOn = false;
      } else {
        glGenTextures(1,&glHandle);
        glBindTexture(GL_TEXTURE_2D,glHandle);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, splash->width, splash->height, 0, GL_RGBA, GL_BYTE, splash->pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      }
    }
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0.8f));
    ImGui::Begin("SPLASHSCREEN", NULL, ImVec2((float)main_window_xres,(float)main_window_yres),0.95f,FROZENWINDOW);
    float scale = 6;
    ImGui::SetWindowFontScale(scale);
    if (splash != NULL) {
      int w = splash->width;
      int h = splash->height;
      if (aspectRatio != 0) {w = (int)(w / aspectRatio * main_window_xres/main_window_yres);}
      ImGui::SetCursorScreenPos(ImVec2((float)(main_window_xres/2-w/2),(float)(main_window_yres/2-h/2)));
      ImGui::Image((ImTextureID)glHandle,ImVec2((float)w,(float)h));
      ImGui::SetWindowFontScale(1);
      ImVec2 m = ImGui::GetItemRectMax();
      ImVec2 m2 = ImGui::GetItemRectMin();
      String text2 = String("Version ")+String::fromInt(version / 100) + String(".") + String((version%100)<10 ? "0" : "") + String::fromInt(version % 100);
      ImVec2 textSize = ImGui::CalcTextSize(text2.c_str());
      ImVec2 textPos;
      textPos.x = m.x - textSize.x - 4;
      textPos.y = m.y - textSize.y - 4;
      ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowFont(),ImGui::GetWindowFontSize(),textPos,0xff000000,text2.c_str());
      textPos.x = m2.x+10;
      textPos.y = m.y - textSize.y*4;
      ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowFont(),ImGui::GetWindowFontSize(),textPos,0xff000000,"By no means a professional program.\nJust if you need to create games or images on DOS.");
      

      ImGui::SetWindowFontScale(scale);
    }

    const char *descLine = "Wypa";
    ImVec2 textSize = ImGui::CalcTextSize(descLine);
    ImGui::SetCursorScreenPos(ImVec2(main_window_xres/2-textSize.x/2+scale,main_window_yres/2-textSize.y/2+scale));
    ImGui::TextColored(ImVec4(0,0,0,0.125),"%s",descLine);
    ImGui::SetCursorScreenPos(ImVec2(main_window_xres/2-textSize.x/2,main_window_yres/2-textSize.y/2));
    ImGui::TextColored(ImVec4(1,1,1,1),"%s",descLine);
    ImGui::End();
    ImGui::PopStyleColor();
    if (ImGui::IsMouseDown(0)||ImGui::IsMouseDown(1)||currentKey != 0) {
      splashScreenOn = false;
      glDeleteTextures(1,&glHandle);
    }
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
  __prefix__##Tools();\
  __prefix__##Undo();\
  __prefix__##Text();\
  __prefix__##Hotkeys();\
  __prefix__##Filter();\
  __prefix__##Gallery();

void deleteAll() {
  SETUPENTITIES(free);
}

bool quit = false;

void checkMemory() {
#ifndef _WIN32
  if (getFreeMemoryInKb() < 60*1024) {
    printf("You !seem! to not have atleast 60mb of memory free.\n");
    printf("So, maybe it's not a good idea to run this program.\n");
    printf("Advisable is to use e.g. the hdpmi32i.exe from SBEMU,\n");
    printf("which can give you around 4GB of memory even on DOS.\n");
    printf("You can use this program, but it is strongly advised to\n");
    printf("open the status window (in Help) and monitor the memory\n");
    printf("usage.\n");
    printf("There will be a memory warning anyways in the \"Navigator\"\n");
    printf("panel if you get below 16mb.\n");
    printf("\n");
    printf("<Press a key>\n");
    getch(); // not available in _WIN32
  }
#endif // _WIN32
}

int waitTiny = 0;
void tinyWait() {
  waitTiny = 2;
}

int graphicsMode = 0;

void dosShell() {
  if (!system(NULL)) return;
  switch(graphicsMode) {
    case 16:
    case 32: {glDone();} break;
    default: return;
  }
  printf(">>> Be aware that this shell may crash the program.\n");
  printf(">>> To return to Wypa type: exit\n");
  int result = system("command"); UNUSED(result);
  switch(graphicsMode) {
    case 32: {glVesa(main_window_xres,main_window_yres,32); glRefresh();} break;
    default: {glVesa(main_window_xres,main_window_yres,16); glRefresh();}
  }
}
  
int main(int argc, const char *argv[]) {

  appFolder = unixFileName(getPath(argv[0]));
#ifdef _WIN32
  appFolder = getCWD();
#endif
  //appFolder = getCWD();

  printf("Wypa version: %d.%02d", version / 100, version % 100);
  printf("\n");
  printf("You can find the newest version of Wypa at:\n");
  printf("www.github.com/kosmonautdnb/wypa\n");
  printf("\n");

#ifndef OTHERPLATFORM
  checkMemory();
  glWatcomPrecisionTimer(GL_TRUE);
  graphicsMode = 32;
  if (!glVesa(main_window_xres,main_window_yres,32)) {
    graphicsMode = 16;
    printf("Could not initialize video: %d %d %dbit\n",(int)main_window_xres,(int)main_window_yres,32);
    double w;
    w = glSeconds(); while(fabs(w-glSeconds())<1) {;}
    printf("Trying: %d %d %dbit (not faithful in colors)\n",(int)main_window_xres,(int)main_window_yres,16);
    w = glSeconds(); while(fabs(w-glSeconds())<1) {;}
    if (!glVesa(main_window_xres,main_window_yres,16)) {
      printf("Resolution %d by %d seems not supported\n",(int)main_window_xres,(int)main_window_yres);
      exit(0);
    }
  }
#else
  if (!initOtherPlatform()) {printf("Could not initialize.\n"); exit(0);}
#endif

  imgWritePNGGamma = 1.0/2.2; // standard SRGB Gamma for PNG writes

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);

  InitImGui(glFrameBufferWidth,glFrameBufferHeight);
  ImGui::GetStyle().WindowRounding = 0.f;
  glFastTexturing = true;
  screenSaverStartTime = glSeconds();

  palette.push_back(getImVec4Color(0xff000000));
  palette.push_back(getImVec4Color(0xffaa0000));
  palette.push_back(getImVec4Color(0xff00aa00));
  palette.push_back(getImVec4Color(0xffaaaa00));
  palette.push_back(getImVec4Color(0xff0000aa));
  palette.push_back(getImVec4Color(0xffaa00aa));
  palette.push_back(getImVec4Color(0xff0055aa));
  palette.push_back(getImVec4Color(0xffaaaaaa));
  palette.push_back(getImVec4Color(0xff555555));
  palette.push_back(getImVec4Color(0xffff5555));
  palette.push_back(getImVec4Color( 0xff55ff55));
  palette.push_back(getImVec4Color(0xffffff55));
  palette.push_back(getImVec4Color(0xff5555ff));
  palette.push_back(getImVec4Color(0xffff55ff));
  palette.push_back(getImVec4Color(0xff55ffff));
  palette.push_back(getImVec4Color(0xffffffff));

  Brush *b;
  b = new Brush(); b->size = 1; brushes.push_back(b);
  b = new Brush(); b->size = 4; brushes.push_back(b);
  b = new Brush(); b->size = 8; brushes.push_back(b);
  b = new Brush(); b->size = 8; b->pressure = 0.3f; brushes.push_back(b);
  b = new Brush(); b->size = 16; brushes.push_back(b);
  b = new Brush(); b->size = 16; b->pressure = 0.3f; brushes.push_back(b);
  b = new Brush(); b->size = 32; brushes.push_back(b);
  b = new Brush(); b->size = 32; b->pressure = 0.3f; brushes.push_back(b);
  b = new Brush(); b->size = 64; brushes.push_back(b);
  b = new Brush(); b->size = 64; b->pressure = 0.3f; brushes.push_back(b);
  b = new Brush(); b->size = 128; brushes.push_back(b);
  b = new Brush(); b->size = 128; b->pressure = 0.3f; brushes.push_back(b);
  b = new Brush(); b->size = 8; b->aspect = 0.5f; b->rotation = 45.f; brushes.push_back(b);
  b = new Brush(); b->size = 16; b->aspect = 0.5f; b->rotation = 45.f; brushes.push_back(b);
  b = new Brush(); b->size = 32; b->aspect = 0.5f; b->rotation = 45.f; brushes.push_back(b);
  b = new Brush(); b->size = 64; b->aspect = 0.5f; b->rotation = 45.f; brushes.push_back(b);
  b = new Brush(); b->size = 128; b->aspect = 0.5f; b->rotation = 45.f; brushes.push_back(b);
  b = new Brush(); b->size = 8; b->edginess = 1.0f; b->pressure = 1.5f; brushes.push_back(b);
  b = new Brush(); b->size = 16; b->edginess = 1.0f; b->pressure = 1.5f; brushes.push_back(b);
  b = new Brush(); b->size = 32; b->edginess = 1.0f; b->pressure = 1.5f; brushes.push_back(b);
  b = new Brush(); b->size = 64; b->edginess = 1.0f; b->pressure = 1.5f; brushes.push_back(b);
  b = new Brush(); b->size = 128; b->edginess = 1.0f; b->pressure = 1.5f; brushes.push_back(b);

  if (argc == 2) {
    loadImage("",argv[1]);
    if (canvas.empty()) {
      addTempPicture();
      Layer *l = loadImage(String(""),String(argv[1]));
      if (l != NULL) {
        currentCanvas->addLayer(l);
      }
    }
    if (refreshMasked) glRefreshMasked(); else glRefresh();
  }

  if (canvas.empty())
    addTempPicture();

  ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(1,1,1,1);
  ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(0,0,0,1);
  ImGui::GetStyle().Colors[ImGuiCol_Button] = ImVec4(0.75f,0.75f,0.75f,1);
  ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = ImVec4(0.5f,0.5f,1.0,1);
  ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = ImVec4(1,1,1,1);
  ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = ImVec4(0.9f,0.9f,0.9f,1);
  ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = ImVec4(0,0,0,1.f);
  ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f,0.2f,0.2f,1.f);
  ImGui::GetStyle().Colors[ImGuiCol_SliderGrab] = ImVec4(0.2f,0.2f,0.2f,1.f);
  ImGui::GetStyle().Colors[ImGuiCol_TooltipBg] = ImVec4(1,1,1,0.8f);
  ImGui::GetStyle().Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0,0,0,0);
  ImGui::GetStyle().Colors[ImGuiCol_CloseButton] = ImVec4(0.25f,0.25f,0.25f,1.f);

  ImGui::GetIO().MouseDoubleClickTime = 1;

  loadBaseSVGs();
  screenSaverStartTime = glSeconds();
  lastTime = glSeconds();

  currentCanvas->control.setCanvasTransform(currentCanvas);
  shouldUpdateAll = true;
  SETUPENTITIES(init);

  beforeUndoPoint();

  while(!quit) {

    if (currentKey == GL_VK_ESCAPE && ctrl) quit = true;
    perFrame();


    if (isPaintAreaActive()) {
      if (currentKey == '-') {currentCanvas->control.zoomBy(currentCanvas, mouseX, mouseY, 1.0/zoomStep);}
      if (currentKey == '+') {currentCanvas->control.zoomBy(currentCanvas, mouseX, mouseY, zoomStep);}
      if (currentKey == 'x') {lockedMouseX = !lockedMouseX; lockedMouseY = false; lockedMousePosX = (int)mouseX; lockedMousePosY = (int)mouseY;}
      if (currentKey == 'y') {lockedMouseY = !lockedMouseY; lockedMouseX = false;  lockedMousePosX = (int)mouseX; lockedMousePosY = (int)mouseY;}
  
      if (controlKeyId == CONTROLKEY_ALT) {currentCanvas->control.moveByPixels(currentCanvas,mouseDX,mouseDY);}

      if (currentKey == GL_VK_LEFT) currentCanvas->control.moveByPixels(currentCanvas,scrollKeyPixels,0);
      if (currentKey == GL_VK_RIGHT) currentCanvas->control.moveByPixels(currentCanvas,-scrollKeyPixels,0);
      if (currentKey == GL_VK_UP) currentCanvas->control.moveByPixels(currentCanvas,0,scrollKeyPixels);
      if (currentKey == GL_VK_DOWN) currentCanvas->control.moveByPixels(currentCanvas,0,-scrollKeyPixels);

      bool mouseMoveKeysEnabled = tool != TOOL_MOVE && tool != TOOL_SELECTION;
      if (mouseMoveKeysEnabled) {
        Layer *l = currentCanvas->getActiveLayer();
        if (l != NULL) {
          double lx0 = mouseX, ly0 = mouseY; currentCanvas->getLayerPos(l,lx0,ly0,lx0,ly0);
          double x0 = lx0+1, y0 = ly0; currentCanvas->getScreenPos(l,x0,y0,x0,y0);
          double x1 = lx0, y1 = ly0+1; currentCanvas->getScreenPos(l,x1,y1,x1,y1);
          double x = 0, y = 0, moveX = 1, moveY = 1;
          switch(currentKey) {
            case '4': {x=-moveX;} break;
            case '6': {x=+moveX;} break;
            case '8': {y=-moveY;} break;
            case '2': {y=+moveY;} break;
            case '7': {x=-moveX;y=-moveY;} break;
            case '1': {x=-moveX;y=+moveY;} break;
            case '9': {x=+moveX;y=-moveY;} break;
            case '3': {x=+moveX;y=+moveY;} break;
            case '5': {ImGui::GetIO().MouseDown[0] = true;};
          }
          if (x != 0 || y != 0) {mouseX += x * (x0-mouseX); mouseY += x * (y0 - mouseY); mouseX += y * (x1-mouseX); mouseY += y * (y1 - mouseY);}
        }
      }
    }

    applyHotKeys();
    useTool();

    if (shouldUpdateAll) {
      currentCanvas->control.setCanvasTransform(currentCanvas);
    }

    if (fullScreen) glClear(GL_COLOR_BUFFER_BIT);

    if (!galleryVisible) {
      blitCanvas();
      toolCanvasStuff();
    } else {
      blitGallery();
    }
    
    ImGuiNewFrame();
    drawMenuBar();
    drawNavigator();
    drawColorField();
    drawLayerView();
    drawToolBar();
    drawToolView();
    drawDialogs();
    showDebugWindow();
    splashScreen();
    ImGui::Render();

    screenSaver();
    whiteFade();
    
    lastActiveLayerSwitchIndex = activeLayerSwitchIndex;
      
    if (refreshMasked) glRefreshMasked(); else glRefresh();

    if (waitTiny>0) {
      waitTiny--;
      double a = glSeconds(); while(fabs(glSeconds()-a)<1.0/30.0) {;}
    }
  }

  glDone();

  printf("Wypa version %d.%02d! (Why you polish abstracts)\n",version / 100, version % 100);
  printf("-----------------------\n");
  printf("A tiny tool by Puls4r productions.\n");
  printf("This experimental DOS application\n");
  printf("is by far not a professional tool.\n");
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
