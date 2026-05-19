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
#include "NAVIGTOR.HPP"
#include "MENUBAR.HPP"
#include "CONFIG.HPP"
#include "LAYERS.HPP"
#include "IMGUI/IMGUI.H"
#include "IMGUI/IMGUIGL.HPP"
#include "TOOLS.HPP"
#include "TOOLBAR.HPP"
#include "COLORFLD.HPP"
#include <math.h>

int navigator_height = 68;
int sidebar_width = 200;

extern double canvasScrollX;
extern double canvasScrollY;
extern double canvasZoom;

GLuint navigatorTexture = 0;
int navigatorX = 11;
int navigatorY = 11;
int navigatorWidth = navigator_height - 10;
int navigatorHeight = navigatorWidth;
Canvas *navigatorCanvas = NULL;

GLuint layerTexture = 0;
int layerTextureWidth = 32;
int layerTextureHeight = 32;
int layerTextureSizeX = 32;
int layerTextureSizeY = 32;

void initNavigator() {
  glGenTextures(1,&navigatorTexture);
  glBindTexture(GL_TEXTURE_2D,navigatorTexture);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, navigatorX, navigatorY, 0, GL_RGBA, GL_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);

  glGenTextures(1,&layerTexture);
  glBindTexture(GL_TEXTURE_2D,layerTexture);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, layerTextureWidth, layerTextureHeight, 0, GL_RGBA, GL_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);

  navigatorCanvas = new Canvas();
  navigatorCanvas->init(navigatorX, navigatorY);
}

void freeNavigator() {
  glDeleteTextures(1,&navigatorTexture);
  delete navigatorCanvas; navigatorCanvas = NULL;
}

void updateNavigatorImage() {
  Vector m(mouseX,mouseY,0);
  currentCanvas->getCanvasPos(m.x,m.y,m.x,m.y);
  m.x -= navigatorX/2;
  m.y -= navigatorY/2;
  Matrix t; t.translate(m);
  navigatorCanvas->transform.fresh(); 
  navigatorCanvas->transform.transformation = t * navigatorCanvas->transform.transformation;
  navigatorCanvas->transform.update(); 


  navigatorCanvas->layers = currentCanvas->layers;
  navigatorCanvas->update(true);
  navigatorCanvas->layers.clear();
  unsigned int *dest = glGetTexturePointer(navigatorTexture);
  unsigned int width = glGetTextureWidth(navigatorTexture);
  unsigned int height = glGetTextureHeight(navigatorTexture);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      unsigned int col = 0xff000000;
      int sx = x;
      int sy = y;
      if (sx >= 0 && sx < navigatorCanvas->width && sy >= 0 && sy < navigatorCanvas->height) 
        col = navigatorCanvas->pixels[sx+sy*navigatorCanvas->width];
      dest[x+y*width] = gammaCorrect(col);
    }
  }
}

void updateLayerImage() {
  Layer *l = currentCanvas->getActiveLayer();
  unsigned int *dest = glGetTexturePointer(layerTexture);
  unsigned int width = glGetTextureWidth(layerTexture);
  unsigned int height = glGetTextureHeight(layerTexture);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      unsigned int col = 0xff000000;
      if (l != NULL) {
        int sx = x * l->width / width;
        int sy = y * l->height / height;
        if (sx >= 0 && sx < l->width && sy >= 0 && sy < l->height) 
          col = l->pixels[sx+sy*l->width];
      }
      dest[x+y*width] = gammaCorrect(col);
    }
  }
}

void drawNavigator() {
  if (fullScreen) return;
  ImGui::SetNextWindowPos(ImVec2(main_window_xres-sidebar_width,menuBar_height));
  ImGui::Begin("NAVIGATOR", NULL, ImVec2(sidebar_width,navigator_height),1.0,FROZENWINDOW);

  ImVec2 cursor = ImGui::GetCursorPos();
  ImVec2 cursor3 = ImGui::GetCursorScreenPos();
  int nameSize = 7*(PICTURENAMEMAX);
  ImGui::PushItemWidth(nameSize);
  ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0.9,0.9,0.9,1));
  ImGui::InputText("##DRAWNAVIGATORPICTURENAME",currentCanvas->pictureName, PICTURENAMEMAX);
  ImGui::PopStyleColor();
  ImGui::PopItemWidth();

  double x,y; x = mouseX - currentCanvas->screenPosX; y = mouseY - currentCanvas->screenPosY; currentCanvas->transform.transformForward(x,y,x,y);
  int xp = (int)floor(x);
  int yp = (int)floor(y);
  if (!currentCanvas->gridOn) {
    ImGui::Text("%d %d",xp,yp);
  } else {
    int gridX = (xp - currentCanvas->gridOffsetX+currentCanvas->gridStepX*10000) / currentCanvas->gridStepX-10000;
    int gridY = (yp - currentCanvas->gridOffsetY+currentCanvas->gridStepY*10000) / currentCanvas->gridStepY-10000;
    ImGui::Text("%d[%d] %d[%d]",xp,gridX,yp,gridY);
  }
  if (toolW >= 0 || toolH >= 0) {
    ImGui::Text("%d %d",(int)ceil(toolW),(int)ceil(toolH));
  }

  if (getFreeMemoryInKb()<MEMWARNINGMB*1024) {
    ImGui::TextColored(ImVec4(1,0,0,1),"Less than %dmb",(int)MEMWARNINGMB);
  }
  ImGui::SetCursorPos(ImVec2(cursor.x+sidebar_width - navigatorWidth - 10,cursor.y));
  updateNavigatorImage();
  ImVec2 cursor2 = ImGui::GetCursorScreenPos();
  if (aspectRatio != 0) {
    navigatorWidth = navigatorHeight / aspectRatio * main_window_xres / main_window_yres;
  }
  if (ImGui::ImageButton((ImTextureID)navigatorTexture,ImVec2(navigatorWidth,navigatorHeight),ImVec2(0,0),ImVec2(1,1),0)) {
  }
  ImVec2 c0 = ImVec2((double)navigatorWidth*(navigatorX/2)/navigatorX,(double)navigatorHeight*(navigatorY/2)/navigatorY);
  ImVec2 c1 = ImVec2((double)navigatorWidth*(navigatorX/2+1)/navigatorX,(double)navigatorHeight*(navigatorY/2+1)/navigatorY);
  c0.x += cursor2.x - 1;
  c0.y += cursor2.y - 1;
  c1.x += cursor2.x + 1;
  c1.y += cursor2.y + 1;
  c0.x = floor(c0.x); c0.y = floor(c0.y); c1.x = floor(c1.x); c1.y = floor(c1.y);
  ImGui::GetWindowDrawList()->AddRect(c0,c1,0xff0000ff);

  c0.x = cursor3.x + sidebar_width - navigatorWidth - 10 - layerTextureSizeX;
  c0.y = cursor3.y + navigatorHeight - layerTextureSizeY - 8;
  ImGui::SetCursorScreenPos(c0);
  ImGui::Image((ImTextureID)layerTexture,ImVec2(layerTextureSizeX,layerTextureSizeY));

  updateLayerImage();
  updateNavigatorImage();

  ImGui::End();
}
