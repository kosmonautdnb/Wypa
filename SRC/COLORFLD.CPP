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
#include "COLORFLD.HPP"
#include "NAVIGTOR.HPP"
#include "MENUBAR.HPP"
#include "CONFIG.HPP"
#include "LAYERS.HPP"
#include "STL/GL.H"
#include "IMGUI/IMGUI.H"
#include "IMGUI/IMGUIGL.HPP"
#include "DIALOGS.HPP"
#include "TOOLBAR.HPP"
#include "SERIALZE.HPP"
#include "TOOLS.HPP"
#include <math.h>

unsigned int colorField_height = 380;

void updatePalette() {
  shouldRecreateQuantization = true;
  shouldUpdateAll = true;
}

int paletteSort(const void *p, const void *q)
{
  const ImVec4* a = (const ImVec4*)p;
  const ImVec4* b = (const ImVec4*)q;

  double la = a->x+a->y+a->z;
  double lb = b->x+b->y+b->z;

  if (la > lb) return 1;
  if (la < lb) return -1;
  if (a->w > b->w) return 1;
  if (a->w < b->w) return -1;
  return 0;
}

void sortPalette(Array<ImVec4> &pal) {
  qsort(&pal[0], pal.size(), sizeof(ImVec4), paletteSort);
}

unsigned int getUIntColor(const ImVec4 &k) {
  int r = k.x * 255.f;
  int g = k.y * 255.f;
  int b = k.z * 255.f;
  int a = k.w * 255.f;
  if (r < 0) r = 0;
  if (g < 0) g = 0;
  if (b < 0) b = 0;
  if (a < 0) a = 0;
  if (r > 255) r = 255;
  if (g > 255) g = 255;
  if (b > 255) b = 255;
  if (a > 255) a = 255;
  return r|(g<<8)|(b<<16)|(a<<24);
}

ImVec4 getImVec4Color(unsigned int v) {
  ImVec4 r;
  r.x = (v & 255) / 255.f;
  r.y = ((v>>8) & 255) / 255.f;
  r.z = ((v>>16) & 255) / 255.f;
  r.w = ((v>>24) & 255) / 255.f;
  return r;
}

unsigned int colorTraceBacks[COLORTRACEBACKS] = {0};

void addUsedColor(const ImVec4 &color) {
  if (getUIntColor(color) != colorTraceBacks[0]) {
    for (int i = COLORTRACEBACKS-2; i >= 0; i--) {
      colorTraceBacks[i+1] = colorTraceBacks[i];
    }
    colorTraceBacks[0] = getUIntColor(color);
  }
}

ImVec4 foregroundColor(0,1,1,1);

GLuint colorSliderTexture = 0;
const unsigned int colorSliderTextureWidth = 256;
const unsigned int colorSliderTextureHeight = 6;
unsigned int colorSliderTextureContent[colorSliderTextureWidth*colorSliderTextureHeight];

void updateColorSlidersTexture() {
  ImVec4 _hsl; getHsl(foregroundColor,&_hsl.x,&_hsl.y,&_hsl.z);
  for (int y = 0; y < 6; y++) {
    for (int x = 0; x < colorSliderTextureWidth; x++) {
      float fx = (float)x /colorSliderTextureWidth;
      float h,s,l;
      ImVec4 rgb;
      switch(y) {
      case 0: {rgb.x=fx;rgb.y=foregroundColor.y;rgb.z=foregroundColor.z;} break;
      case 1: {rgb.x=foregroundColor.x;rgb.y=fx;rgb.z=foregroundColor.z;} break;
      case 2: {rgb.x=foregroundColor.x;rgb.y=foregroundColor.y;rgb.z=fx;} break;
      case 3: {h = fx; s = 1; l = 0.5; rgb = hsl(h,s,l);} break;
      case 4: {h = _hsl.x; s = fx; l = _hsl.z; rgb = hsl(h,s,l);} break;
      case 5: {h = _hsl.x; s = _hsl.y; l = fx; rgb = hsl(h,s,l);} break;
      }
      int ri = (int)floor(rgb.x*255.0);
      int gi = (int)floor(rgb.y*255.0);
      int bi = (int)floor(rgb.z*255.0);
      if (ri < 0) ri = 0;
      if (gi < 0) gi = 0;
      if (bi < 0) bi = 0;
      if (ri > 255) ri = 255;
      if (gi > 255) gi = 255;
      if (bi > 255) bi = 255;
      colorSliderTextureContent[x+y*colorSliderTextureWidth] = ri|(gi<<8)|(bi<<16)|0xff000000;
    }
  }
  glBindTexture(GL_TEXTURE_2D,colorSliderTexture);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, colorSliderTextureWidth, colorSliderTextureHeight, 0, GL_RGBA, GL_BYTE, colorSliderTextureContent);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);
}

unsigned int colorWheelX = 160;
double colorWheelAspectRatio = aspectRatio;
unsigned int colorWheelY = -1;
unsigned int colorWheelTexture = 0;

bool roundButton(const char *name, bool enabled);

bool showPalette = false;
int paletteColorsPerRow = 16;
Array<ImVec4> palette;
Array<ImVec4> *paletteToSave;

float hueToRgb(float p, float q, float t) {
  if (t < 0) t += 1;
  if (t > 1) t -= 1;
  if (t < 1/6.0) return p + (q-p)*6*t;
  if (t < 1/2.0) return q;
  if (t < 2.0/3.0) return p + (q-p) * (2/3.0-t)*6;
  return p;
}

ImVec4 hsl(double h,double s,double l) {
  if (h<0.00001) h = 0.00001; // hotfix
  double r,g,b;
  if (s == 0) {
    r=g=b=l;
  } else {
    const double q = l < 0.5 ? l * (1+s) : l + s - l * s;
    const double p = 2 * l - q;
    r = hueToRgb(p, q, h + 1 / 3.0);
    g = hueToRgb(p, q, h);
    b = hueToRgb(p, q, h - 1 / 3.0);
  }
  return ImVec4(r,g,b,1);
}

void getHsl(const ImVec4 &rgb, float *hv, float *sv, float *lv) {
  double r = rgb.x;
  double g = rgb.y;
  double b = rgb.z;
  double m = r; if (g > m) m = g; if (b > m) m = b;
  double n = r; if (g < n) n = g; if (b < n) n = b;
  double h = 0, s = 0, l = (m+n)*0.5;
  if (m == n) {
    *hv = 0;
    *sv = 0;
    *lv = l;
    return;
  }
  double d = m - n;
  s = l > 0.5 ? d / (2-m-n) : d / (m+n);
  if (s < 0) s = 0;
  if (m == r) h = (g-b)/d+(g<b?6:0);
  if (m == g) h = (b-r)/d+2;
  if (m == b) h = (r-g)/d+4;
  h /= 6.0;
  *hv = (float)h;
  *sv = (float)s;
  *lv = (float)l;
}

bool updateColorWheel(float h, int mx, int my, float *hv, float *sv, float *lv) {
  bool ret = false;
  const double a = -h * 2 * PI;
  const double wheelPixels = 16.0;
  unsigned int *rgba = glGetTexturePointer(colorWheelTexture);
  double s = 1.0/(1.0/sqrt(2.0));
  for (int y = 0; y < colorWheelY; y++) {
    double fy = (double)y / (colorWheelY-1) * 2.0 - 1.0;
    for (int x = 0; x < colorWheelX; x++) {
      double fx = (double)x / (colorWheelX-1) * 2.0 - 1.0;
      double d = sqrt(fx*fx+fy*fy);
      unsigned int c = 0;
      if (d < 1.0 && d > 1.0 - wheelPixels/colorWheelX) {
        float h = atan2(fx,fy)/PI*0.5+0.5;
        ImVec4 c2 = hsl(h,1,0.5);
        int ri = c2.x * 255.0;
        int gi = c2.y * 255.0;
        int bi = c2.z * 255.0;
        c = ri|(gi<<8)|(bi<<16)|0xff000000;
        if ((hv != NULL) && ((int)mx==x) && ((int)my==y)) {*hv = h; ret = true;}
      }
      double fx2 = (-sin(a)*fx+cos(a)*fy)*s;
      double fy2 = (cos(a)*fx+sin(a)*fy)*s;
      fx2 += 0.25;
      if (fx2 > -1 && fx2 < 1 && fy2 > -1 && fy2 < 1) {
         if(fabs(fy2)<fx2*0.5+0.5) {
          double hx = fx2 * 0.5 + 0.5;
          double hy = fy2 * 0.5 + 0.5;
          ImVec4 c2 = hsl(h,1.0-hx,hy-0.5+0.5);
          int ri = c2.x * 255.0;
          int gi = c2.y * 255.0;
          int bi = c2.z * 255.0;
          c = ri|(gi<<8)|(bi<<16)|0xff000000;
          if ((sv != NULL) && ((int)mx==x) && ((int)my==y)) {*sv = (float)(1.0-hx); ret = true;}
          if ((lv != NULL) && ((int)mx==x) && ((int)my==y)) {*lv = (float)(hy-0.5+0.5); ret = true;}
        }
      }
      rgba[x+y*colorWheelX] = c;
    }
  }
  return ret;
}

void initColorField() {
  colorWheelAspectRatio = aspectRatio;
  colorWheelY = colorWheelX*colorWheelAspectRatio*main_window_yres/main_window_xres;
  if (colorWheelTexture != 0) {glDeleteTextures(1,&colorWheelTexture); colorWheelTexture = 0;}
  glGenTextures(1,&colorWheelTexture);
  glBindTexture(GL_TEXTURE_2D,colorWheelTexture);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, colorWheelX, colorWheelY, 0, GL_RGBA, GL_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);
  float h,s,l; getHsl(foregroundColor,&h,&s,&l); updateColorWheel(h,0,0,NULL,NULL,NULL);
  updatePalette();

  if (colorSliderTexture == 0) {
    glGenTextures(1,&colorSliderTexture);
  }
  updateColorSlidersTexture();
}

void freeColorField() {
  glDeleteTextures(1,&colorWheelTexture); colorWheelTexture = 0;
  glDeleteTextures(1,&colorSliderTexture); colorSliderTexture = 0;
}

unsigned int swapRB(unsigned int color) {
  return (color & 0xff00ff00)|((color>>16)&0x000000ff)|((color<<16)&0x00ff0000);
}

bool colorItem(const String &name, const String &id, ImVec4 *color) {
  bool ret = false;
  ImGui::PushID("##DRAWCOLORITEMTEXBUTTON");
  ImGui::Image((ImTextureID)0, ImVec2(16/aspectRatio*main_window_xres/main_window_yres,16), ImVec2(0,0), ImVec2(1,1), *color);
  ImGui::PopID();
  ImGui::SameLine();
  ImGui::Text("%s#",name.c_str());
  ImGui::SameLine();
  static char colorText[11];
  sprintf(colorText,"%08x",swapRB(getUIntColor(*color)));
  for (int i = 0; i < strlen(colorText); i++) if (colorText[i]>='a'&&colorText[i]<='f') colorText[i]=(char)((colorText[i]-'a')+'A');
  ImGui::PushItemWidth(7*8);
  if (ImGui::InputText(id.c_str(),colorText,9)) {
    unsigned int value = 0;
    for (int i = 0; i < 8; i++) {
      value <<= 4;
      char c = colorText[i];
      if (c>='0'&&c<='9') value += c-'0';
      if (c>='a'&&c<='f') value += c-'a'+10;
      if (c>='A'&&c<='F') value += c-'A'+10;
    }
    *color = getImVec4Color(swapRB(value));
    ret = true;
  }
  ImGui::PopItemWidth();
  return ret;
}

void colorBorder() {
  ImVec2 c = ImGui::GetItemRectMax();
  c.x -= 1;
  c.y -= 1;
  ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(),c,0xff000000);
}

void blitColorSliderBackground(const ImVec2 &before, int slider, int sliderWidth) {
  ImVec2 a = ImGui::GetCursorScreenPos();
  ImVec2 b = before;
  b.x += 2;
  b.y += 2;
  ImGui::SetCursorScreenPos(b);
  float ty = slider / 6.0;
  ImGui::Image((ImTextureID)colorSliderTexture,ImVec2(sliderWidth,13),ImVec2(0,ty),ImVec2(1,ty));
  colorBorder();
  ImGui::SetCursorScreenPos(a);
}

void colorDisplay(ImVec4 *c) {
  float k;
  ImVec2 before;
  int sw = sidebar_width - 60;
  ImGui::PushItemWidth(sw);
  k = c->w * 255.0; if (ImGui::SliderFloat("Alpha##DIALOGCOLORCOLORALPHA",&k,0,255)) {c->w = k/255.0;}
  ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0,0,0,0));
  before = ImGui::GetCursorScreenPos(); blitColorSliderBackground(before,0,sw); k = c->x * 255.0; if (ImGui::SliderFloat("Red##DIALOGCOLORCOLORRED",&k,0,255)) {c->x = k/255.0;updateColorSlidersTexture();}
  before = ImGui::GetCursorScreenPos(); blitColorSliderBackground(before,1,sw); k = c->y * 255.0; if (ImGui::SliderFloat("Green##DIALOGCOLORCOLORGREEN",&k,0,255)) {c->y = k/255.0;updateColorSlidersTexture();}
  before = ImGui::GetCursorScreenPos(); blitColorSliderBackground(before,2,sw); k = c->z * 255.0; if (ImGui::SliderFloat("Blue##DIALOGCOLORCOLORBLUE",&k,0,255)) {c->z = k/255.0;updateColorSlidersTexture();}
  float h,s,l; 
  getHsl(*c,&h,&s,&l);
  before = ImGui::GetCursorScreenPos(); blitColorSliderBackground(before,3,sw); k = h*360.0;  if (ImGui::SliderFloat("Hue##DIALOGCOLORCOLORHUE",&k,0,360)) {double w = c->w; h = k/360.0; updateColorWheel(h,0,0,&h,&s,&l); *c = hsl(h,s,l); c->w = w; getHsl(*c,&h,&s,&l); updateColorWheel(h,0,0,NULL,NULL,NULL);updateColorSlidersTexture();}
  before = ImGui::GetCursorScreenPos(); blitColorSliderBackground(before,4,sw); k = s;  if (ImGui::SliderFloat("Sat.##DIALOGCOLORCOLORSAT",&k,0,1)) {double w = c->w; s = k; *c = hsl(h,s,l); c->w = w;updateColorSlidersTexture();}
  before = ImGui::GetCursorScreenPos(); blitColorSliderBackground(before,5,sw); k = l;  if (ImGui::SliderFloat("Light.##DIALOGCOLORCOLORLIT",&k,0,1)) {double w = c->w; l = k; *c = hsl(h,s,l); c->w = w;updateColorSlidersTexture();}
  ImGui::PopStyleColor();
  ImGui::PopItemWidth();
  ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0,0,0,0));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0,0,0,0));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0,0,0,0));
  ImVec2 cursorHere = ImGui::GetCursorScreenPos();
  if (ImGui::ImageButton((ImTextureID)colorWheelTexture,ImVec2(colorWheelX,colorWheelY)) || (ImGui::IsItemHovered()&&ImGui::IsMouseDown(0))) {
    int mx = ImGui::GetMousePos().x-cursorHere.x;
    int my = ImGui::GetMousePos().y-cursorHere.y;
    bool update = updateColorWheel(h,mx,my,&h,&s,&l);
    if (update) {double w = c->w; *c = hsl(h,s,l); c->w = w; getHsl(*c,&h,&s,&l); updateColorWheel(h,0,0,NULL,NULL,NULL); updateColorSlidersTexture();}
  }
  ImGui::PopStyleColor();
  ImGui::PopStyleColor();
  ImGui::PopStyleColor();
  ImGui::SameLine();
  ImGui::BeginChild("COLORCHILD1",ImVec2(16/aspectRatio*main_window_xres/main_window_yres,20*(COLORTRACEBACKS+1)+20));
  ImGui::Image((ImTextureID)0, ImVec2(16/aspectRatio*main_window_xres/main_window_yres,16), ImVec2(0,0), ImVec2(1,1), *c);
  ImGui::Text("");
  for (int i = 0; i < COLORTRACEBACKS; i++) {
    unsigned int c2 = colorTraceBacks[i];
    ImVec4 c3 = ImVec4((c2&255)/255.f,((c2>>8)&255)/255.f,((c2>>16)&255)/255.f,((c2>>24)&255)/255.f);
    ImGui::PushID(combined(String("##COLORDISPLAYTRACEBACK")+String::fromInt(i)).c_str());
    if (ImGui::ImageButton((ImTextureID)0, ImVec2(16/aspectRatio*main_window_xres/main_window_yres,16), ImVec2(0,0), ImVec2(1,1), 0, c3, c3)) {
      *c = ImVec4(c3.x,c3.y,c3.z,c3.w);
      getHsl(*c,&h,&s,&l); updateColorWheel(h,s,l,NULL,NULL,NULL);updateColorSlidersTexture();
    }
    ImGui::PopID();
  }
  ImGui::EndChild();
}

void removeColor() {
  for (int i = 0; i < palette.size(); i++) {
    if (getUIntColor(palette[i]) == getUIntColor(foregroundColor)) {
      palette.erase(i,1); 
      int pos = i; 
      if (pos>=palette.size()) pos = palette.size()-1; 
      if ((unsigned int)pos<palette.size()) foregroundColor = palette[pos];
      return;
    }
  }
}

ImVec4 getUnusedColor() {
  for (int i = 0; i < 256; i++) {
    int r = rand() & 255;
    int g = rand() & 255;
    int b = rand() & 255;
    int a = (rand() & 127)+128;
    unsigned int col = r|(g<<8)|(b<<16)|(a<<24);
    bool found = false;
    for (int j = 0; j < palette.size(); j++) {
      if (getUIntColor(palette[i])==col) {found = true; break;}
    }
    if (!found) return ImVec4(r/255.f,g/255.f,b/255.f,a/255.f);
  }
  return ImVec4(0,0,0,0);
}

void insertColor() {
  for (int i = 0; i < palette.size(); i++) {
    if (getUIntColor(palette[i]) == getUIntColor(foregroundColor)) {
      palette.insert(getUnusedColor(),i); 
      foregroundColor = palette[i];
      return;
    }
  }
  palette.push_back(getUnusedColor()); 
  foregroundColor = palette[palette.size()-1];
}

void drawColorField() {
  if (fullScreen) return;
  if (colorWheelAspectRatio != aspectRatio) initColorField();
  ImGui::SetNextWindowPos(ImVec2(main_window_xres-sidebar_width,menuBar_height+navigator_height));
  ImGui::Begin("COLORFIELD", NULL, ImVec2(sidebar_width,colorField_height),1.0,FROZENWINDOW);
  if (showPalette) {
    if (roundButton("Color wheel##DRAWPALETTECOLORFIELDCOLORWHEEL",true)) {
      showPalette = false;
    }
    ImGui::PushItemWidth(7*4+16*2);
    ImGui::SameLine();colorItem("","##DRAWCOLORFIELDPALETTEPALETTEHEX",&foregroundColor);
    if (paletteColorsPerRow<1) paletteColorsPerRow = 1;
    if (paletteColorsPerRow>32) paletteColorsPerRow = 32;
    if (roundButton("Load palette##DRAWCOLORFIELDPALETTELOAD",true)) {
      dialogLoadSave(true,DIALOG_LOADPALETTE);
    }
    ImGui::SameLine();
    if (roundButton("Save palette##DRAWCOLORFIELDPALETTESAVE",!palette.empty())) {
      paletteToSave = &palette; dialogLoadSave(true,DIALOG_SAVEPALETTE);
    }
    ImGui::Text("Palette colors"); int cols = palette.size(); ImGui::SameLine(); if (ImGui::InputInt("##DRAWCOLORFIELDCOLORCOUNT",&cols,1,1)) {if (cols>2048) cols = 2048; if (cols < 0) cols = 0; palette.resize(cols); updatePalette();}
    ImGui::Text("Colors per row"); ImGui::SameLine(); ImGui::InputInt("##DRAWPALETTECOLORFIELDROWCOLS",&paletteColorsPerRow, 1, 1);
    if (paletteColorsPerRow > 16) paletteColorsPerRow = 16;
    if (paletteColorsPerRow < 1) paletteColorsPerRow = 1;
    if (roundButton("Insert color##DRAWCOLORFIELDPALETTEINSERT",true)) {
      insertColor();
      updatePalette();
    }
    ImGui::SameLine();
    if (roundButton("Remove color##DRAWCOLORFIELDPALETTEREMOVE",!palette.empty())) {
      removeColor();
      updatePalette();
    }
    ImGui::BeginChild("DRAWCOLORFIELDCHILD",ImVec2(sidebar_width,ImGui::GetWindowSize().y-(ImGui::GetCursorScreenPos().y-ImGui::GetWindowPos().y)));
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    cursor.y += 3;
    ImGui::SetCursorScreenPos(cursor);
    int b = (sidebar_width-40-16) / (paletteColorsPerRow);
    b -= 1; // rounding
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(0,0));
    for (int i = 0; i < palette.size(); i++) {
      ImGui::PushID(combined(String("##DRAWCOLORFIELDPALETTEPALETTEBUTTON")+String::fromInt(i)).c_str());
      if ((i % paletteColorsPerRow) != 0) {
        ImGui::SameLine();
      } else {
        ImGui::Text("");
        ImVec2 cursor2 = ImGui::GetCursorScreenPos();
        cursor2.x += 2;
        ImGui::SetCursorScreenPos(cursor2);
        ImGui::SameLine();
      }
      ImVec4 c3 = palette[i];
      bool a = ImGui::GetIO().MouseClicked[0];
      ImGui::GetIO().MouseClicked[0] = ImGui::GetIO().MouseClicked[0] || ImGui::GetIO().MouseClicked[1];
      if (ImGui::ImageButton((ImTextureID)0, ImVec2(b,14), ImVec2(0,0), ImVec2(1,1), -1, c3, c3)) {
        if (ImGui::IsMouseDown(1)) {
          palette[i] = foregroundColor;
          updatePalette();
        } else {
          foregroundColor = c3;
        }
      }
      if (getUIntColor(palette[i]) == getUIntColor(foregroundColor)) {
        ImVec2 p00 = ImGui::GetItemRectMin();
        ImVec2 p11 = ImGui::GetItemRectMax();
        p00.x -= 1;
        p00.y -= 1;
        ImGui::GetWindowDrawList()->AddRect(p00,p11,0xff0000ff);
        p00.x -= 1;
        p00.y -= 1;
        p11.x += 1;
        p11.y += 1;
        ImGui::GetWindowDrawList()->AddRect(p00,p11,0xff000000);
      }
      ImGui::GetIO().MouseClicked[0] = a;
      ImGui::PopID();
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::PopItemWidth();
  } else {
    if (roundButton("Palette##DRAWCOLORFIELDPALETTE",true)) {
      showPalette = true;
    }

    Layer *l = currentCanvas->getActiveLayer();
    if (l != NULL && l->colorTransform.useColorTransform) {//||l->colorTransform.useQuantization||l->colorTransform.usePosterize||l->colorTransform.useThreshold)) {
      ImGui::SameLine();
      ImGui::PushID("##DRAWCOLORITEMPOSTPROCESSCOLOR");
      bool quantize = l->colorTransform.useQuantization && (quantization.size() == 64*64*64);
      unsigned int col = getUIntColor(foregroundColor);
      if (l->colorTransform.useColorTransform) col = l->colorTransform.applyColorTransform(col);
      //if (l->colorTransform.usePosterize) col = l->colorTransform.applyPosterize(col);
      //if (l->colorTransform.useThreshold) col = l->colorTransform.applyThreshold(col);
      //if (quantize) col = quantizeFast(col, l->colorTransform.ditherStrength, 0, 0);
      ImVec4 c2 = getImVec4Color(col);
      ImGui::Image((ImTextureID)0, ImVec2(16/aspectRatio*main_window_xres/main_window_yres,16), ImVec2(0,0), ImVec2(1,1), c2);
      ImGui::PopID();
    }

    ImGui::SameLine();
    ImVec4 *c = &foregroundColor;
    if (colorItem("","##DIALOGCOLORCOLORHEX",c)) {
      float h,s,l;
      getHsl(*c,&h,&s,&l); updateColorWheel(h,s,l,NULL,NULL,NULL);
    }
    colorDisplay(&foregroundColor);
  }
  ImGui::End();
}
