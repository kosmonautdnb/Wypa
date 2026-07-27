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
#include "GALLERY.HPP"
#include "STL/GL.H"
#include "IMGUI/IMGUI.H"
#include "IMGUI/IMGUIGL.HPP"
#include "SERIALZE.HPP"
#include "LAYERS.HPP"
#include "TOOLBAR.HPP"
#include "NAVIGTOR.HPP"
#include "TOOLVIEW.HPP"
#include "MENUBAR.HPP"
#include "STL/DOS.HPP"
#include "TOOLS.HPP"
#include "DIALOGS.HPP"
#include "COLORFLD.HPP"
#include <math.h>

bool roundButton(const char *name, bool enabled);

bool galleryVisible = false;
float galleryScale = 1.f;
int galleryScrollTo = -1;

const int GALLERYTEXTURECOUNT = 4;

class GalleryEntry {
public:
  GLuint texHandle;
  String fileName;
  File file;
  int width,height;
  bool hasAlpha;

  GalleryEntry() {
    fileName = "";
    width = height = 1;
    hasAlpha = false;
    glGenTextures(1,&texHandle);
    glBindTexture(GL_TEXTURE_2D,texHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D,0);
  }

  ~GalleryEntry() {
    glDeleteTextures(1,&texHandle); texHandle = 0;
    width = height = 1;
  }

  void load(const String &fileName) {
    hasAlpha = false;
    this->fileName = fileName;
    Layer *l = loadImage(String(""), fileName);
    if (l == NULL) {
      l = loadThumb(String(""), fileName);
    }
    if (l == NULL || l->width <= 1 || l->height <= 1) {
      if (l != NULL) delete l;
      width = 1; height = 1;
      glBindTexture(GL_TEXTURE_2D,texHandle);
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, width, height, 0, GL_RGBA, GL_BYTE, NULL);
      glBindTexture(GL_TEXTURE_2D,0);
    } else {
      reverseLastLayerIdCountUp();
      width = l->width; height = l->height;
      glBindTexture(GL_TEXTURE_2D,texHandle);
      for (int i = 0; i < width*height; i++) {if (((l->pixels[i]>>24)&255)!=255) {hasAlpha = true; break;}}
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, width, height, 0, GL_RGBA, GL_BYTE, l->pixels);
      glBindTexture(GL_TEXTURE_2D,0);
      delete l;
    }
  }

  bool isInvalid() {
    return width <= 1 || height <= 1;
  }

  void update(const File &file) {
    String fileName = dosFileName(file.name.full);
    if (fileName != this->fileName) {
      load(fileName);
      this->file = file;
    }
  }

  void blit(float index) {
    double sc2 = galleryScale;
    if (sc2 < 0.001) sc2 = 0.001;
    int GALLERYTEXTUREWIDTH = (glFrameBufferWidth-sidebar_width-toolbar_width-toolview_width)*2/3*sc2;
    int baseLine = 100;
    int bottom = glFrameBufferHeight-baseLine;
    if (sc2 > 1) sc2 = 1;
    int left = toolbar_width+toolview_width + GALLERYTEXTUREWIDTH*(index-1)/sc2;
    left += ((glFrameBufferWidth-sidebar_width-toolbar_width-toolview_width)-GALLERYTEXTUREWIDTH)*0.5;
    int top = bottom - GALLERYTEXTUREWIDTH * height / width * aspectRatio * main_window_yres/main_window_xres;
    int right = left + GALLERYTEXTUREWIDTH;

    int wallX = 10;
    int wallY = wallX * aspectRatio * main_window_yres/main_window_xres;
    int top2 = top - wallY;
    int right3 = right + wallX;
    //float k2 = 0.0;
    glBegin(GL_QUADS);

    glColor4f(0,0,0,1);

    glVertex4f(right,top2,0,1);
    glVertex4f(left+wallX,top2,0,1);
    glVertex4f(left,top,0,1);
    glVertex4f(right,top,0,1);

    glVertex4f(right3,top2,0,1);
    glVertex4f(right,top2,0,1);
    glVertex4f(right,bottom,0,1);
    glVertex4f(right3,bottom-wallY,0,1);

    glEnd();

    if (hasAlpha) {
      glEnable(GL_BLEND);
      //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ALPHA
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // PREMUL
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,texHandle);
    glBegin(GL_QUADS);
    glColor4f(1,1,1,1);
    glTexCoord2f(1,0); glVertex4f(right,top,0,1);
    glTexCoord2f(0,0); glVertex4f(left,top,0,1);
    glTexCoord2f(0,1); glVertex4f(left,bottom,0,1);
    glTexCoord2f(1,1); glVertex4f(right,bottom,0,1);
    glEnd();

    if (hasAlpha) glDisable(GL_BLEND);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float k = 1.2;
    int left2 = (left - glFrameBufferWidth*0.5) * k + glFrameBufferWidth*0.5;
    int right2 = (right - glFrameBufferWidth*0.5) * k + glFrameBufferWidth*0.5;
    int bottom2 = bottom+baseLine*0.7;

    glBegin(GL_QUADS);
    glColor4f(1,1,1,1);
    glTexCoord2f(1,1); glVertex4f(right,bottom,0,1);
    glTexCoord2f(0,1); glVertex4f(left,bottom,0,1);
    glColor4f(1,1,1,0);
    glTexCoord2f(0,0.5f); glVertex4f(left2,bottom2,0,1);
    glTexCoord2f(1,0.5f); glVertex4f(right2,bottom2,0,1);
    glEnd();

    glDisable(GL_BLEND);

    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);

    String s = String(longFileName(fileName)).last(20).c_str();

    int bottom3 = bottom + 12;
    for (int y = -1; y <= 1; y++) {
      for (int x = -1; x <= 1; x++) {
        drawText(3,left+x*3,bottom3+y*3,s.c_str(),0xff000000,0,0);
      } 
    }
    drawText(3,left,bottom3,s.c_str(),0xff0000ff,0,0);
  }
};

Array<File> galleryFiles;
int destIndex = 0;
float currentIndex = 0;
String galleryFolder = "./DATA/IMAGES/";
GalleryEntry *gallery[GALLERYTEXTURECOUNT] = {NULL};

void getFolder(const String &folder) {
  String f = dosFileName(fromLocalFileName(folder)); galleryFiles = dosGetFiles(f);
  for (int i = 0; i < (int)galleryFiles.size(); i++) {
    File &k = galleryFiles[i];
    bool invalid = (k.attributes & (DOS_ATTR_VOLID|DOS_ATTR_SUBDIR))!=0;
    if (k.name.name == "." || k.name.name == "..") invalid = true;
    if (invalid) {
      galleryFiles.erase(i,1);
      i--;
    }
  }
  sortFileNames(galleryFiles,3,false);
  destIndex = 0;
  currentIndex = 0;
}

void initGallery() {
  for (int i = 0; i < GALLERYTEXTURECOUNT; i++) {
    gallery[i] = new GalleryEntry();
  }
  getFolder(galleryFolder);
}

void freeGallery() {
  for (int i = 0; i < GALLERYTEXTURECOUNT; i++) {
    if (gallery[i] != NULL) {delete gallery[i]; gallery[i] = NULL;}
  }
}

void selectGalleryFolder(const char *shortPath, const char *longFileName) {
  UNUSED(longFileName);
  getFolder(shortPath);
}

void blitGallery() {
  glPushAttrib(0xffffffff);

  int bottom = glFrameBufferHeight;
  int left = toolbar_width+toolview_width;
  int top = menuBar_height;
  int right = glFrameBufferWidth -  sidebar_width;

  glViewport(left, glFrameBufferHeight - bottom, right-left, bottom-top);
  glScissor(left, glFrameBufferHeight - bottom, right-left, bottom-top);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(left,right,bottom,top,-1,1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glClearColor(0,0,0,1);
  glClear(GL_COLOR_BUFFER_BIT);


  glBegin(GL_QUADS);
  glColor4f(0.1,0.1,0.1,1);
  glVertex4f(glFrameBufferWidth,0,0,1);
  glVertex4f(0,0,0,1);
  float k = 0.4;
  glColor4f(0.9*k,0.9*k,1.0*k,1);
  glVertex4f(0,glFrameBufferHeight,0,1);
  glVertex4f(glFrameBufferWidth,glFrameBufferHeight,0,1);
  glEnd();

  int baseLine = 100;
  int bottom2 = glFrameBufferHeight-baseLine;
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  glColor4f(1,1,1,0);
  glVertex4f(glFrameBufferWidth,bottom2-40,0,1);
  glVertex4f(0,bottom2-40,0,1);
  glColor4f(1,1,1,1);
  glVertex4f(0,bottom,0,1);
  glVertex4f(glFrameBufferWidth,bottom,0,1);
  glEnd();
  glDisable(GL_BLEND);

  if (destIndex > (int)galleryFiles.size()-1) destIndex = galleryFiles.size()-1;
  if (destIndex < 0) destIndex = 0;
  currentIndex = (destIndex - currentIndex)*0.95+currentIndex;
  if (currentIndex < 0) currentIndex = 0;
  if (currentIndex > (int)galleryFiles.size()-1) currentIndex = galleryFiles.size()-1;

  for (int i = 0; i < GALLERYTEXTURECOUNT; i++) {
    int k = (int)floor(i-1+currentIndex);
    if ((unsigned int)k < (unsigned int)galleryFiles.size()) {
      int mapIndex = (k + GALLERYTEXTURECOUNT) % GALLERYTEXTURECOUNT;
      gallery[mapIndex]->update(galleryFiles[k]);
      gallery[mapIndex]->blit(i-(currentIndex-floor(currentIndex)));
    }
  }

  if (monitorGamma != 1) {
    for (int y = top; y < bottom; y++) {
      for (int x = left; x < right; x++) {
        unsigned int col = glFrameBuffer[x+y*glFrameBufferWidth];
        col = gammaTable[col & 255]|(gammaTable[((col>>8) & 255)+256*1]<<8)|(gammaTable[((col>>16) & 255)+256*2]<<16)|(col & 0xff000000);
        glFrameBuffer[x+y*glFrameBufferWidth] = col;
      }
    }
  }

  if (!ImGui::GetIO().WantCaptureKeyboard) {
    if (currentKey == GL_VK_RIGHT || currentKey == '6') {destIndex++; galleryScrollTo = destIndex;}
    if (currentKey == GL_VK_LEFT || currentKey == '4') {destIndex--; galleryScrollTo = destIndex;}
    if (currentKey == GL_VK_ESCAPE) {galleryVisible = !galleryVisible;}
  }
  if (!ImGui::GetIO().WantCaptureMouse) {
    if (ImGui::IsMouseClicked(0)) {
      if (mouseX < glFrameBufferWidth*0.5) {destIndex--; galleryScrollTo = destIndex;}
      if (mouseX > glFrameBufferWidth*0.5) {destIndex++; galleryScrollTo = destIndex;}
    }
  }

  glPopAttrib();
}

void drawGalleryView() {
  ImGui::Text("Scale"); ImGui::SameLine(); if (ImGui::SliderFloat("##DRAWGALLERYVIEWSCALE",&galleryScale,0.001,2)) {} else toolTip("Scale the gallery images view size by this value");
  if (galleryScale < 0.001) galleryScale = 0.001;
  if (roundButton("Select folder",true)) {
    dialogLoadSave(true, DIALOG_SELECTGALLERYFOLDER);
  } else toolTip("Select the folder to display the images from");
  if (roundButton("<<",true)) {destIndex--; galleryScrollTo = destIndex;} else toolTip("Scroll to the image before");
  ImGui::SameLine();
  if (roundButton(">>",true)) {destIndex++; galleryScrollTo = destIndex;} else toolTip("Scroll to the image beyond");
  ImGui::Separator();

  GalleryEntry *a = gallery[0];
  if (currentIndex < 0) currentIndex = 0;
  if (currentIndex > (int)galleryFiles.size()-1) currentIndex = galleryFiles.size()-1;
  int k2 = (int)currentIndex;
  if ((unsigned int)k2 < galleryFiles.size()) {
    int mapIndex = (k2 + GALLERYTEXTURECOUNT) % GALLERYTEXTURECOUNT;
    a = gallery[mapIndex];
  }

  ImGui::TextWrapped("%s",a->fileName.c_str()); toolTip("The current image filename");
  ImGui::Separator();
  ImGui::TextWrapped("%s",dosFileName(longFileName(a->fileName)).c_str()); toolTip("The current image filename as long filename");
  ImGui::Separator();
  ImGui::Text("Width %d",a->width); toolTip("The current image width in pixels");
  ImGui::Text("Height %d",a->height); toolTip("The current image height in pixels");
  ImGui::Text("%d.%d.%d",a->file.date.day,a->file.date.month,a->file.date.year); toolTip("The date of creation of the current image");
  ImGui::Text("%d:%d.%d",a->file.date.hour,a->file.date.minute,a->file.date.second); toolTip("The time of creation of the current image");
  ImGui::Separator();
  if (roundButton("Load image",true)) {
    loadImage("",a->fileName.c_str());
    galleryVisible = false;
  } else toolTip("Load this image");
  ImGui::Separator();
  ImGui::Text("Pictures %d", galleryFiles.size()); toolTip("The count of viewable pictures in this folder");
  ImGui::PushItemWidth(7*4);
  ImGui::Text("Selected"); ImGui::SameLine(); int k = (int)currentIndex; if (ImGui::InputInt("##DRAWGALLERYVIEWSELECTED",&k,0,0)) {currentIndex = k; destIndex = (int)currentIndex;} else toolTip("The current selected image");
  ImGui::PopItemWidth();
  ImGui::Separator();
  ImGui::BeginChild("DRAWGALLERYVIEWCHILD",ImVec2(toolview_width,main_window_yres-ImGui::GetCursorScreenPos().y));
  for (int i = 0; i < (int)galleryFiles.size(); i++) {
    ImGui::Text("%s",galleryFiles[i].name.full.last(14).c_str());
    ImVec2 minB = ImGui::GetItemRectMin();
    ImVec2 maxB = ImGui::GetItemRectMax();
    if (galleryScrollTo == i) ImGui::SetScrollPosHere();
    if (i == (int)currentIndex) ImGui::GetWindowDrawList()->AddRectFilled(minB,maxB,0x40ffff00);
    if (ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(minB,maxB)) {currentIndex = i; destIndex = (int)currentIndex;}
  }
  ImGui::EndChild();
  galleryScrollTo = -1;
}
