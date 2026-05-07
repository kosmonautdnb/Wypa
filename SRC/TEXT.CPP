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
#include "TEXT.HPP"
#define STB_TRUETYPE_IMPLEMENTATION
#include "STL/STBTTF.HPP"
#include "LAYERS.HPP"

unsigned int fontIdCounter = 0;

void initText() {
}

void freeText() {
  for (int i = 0; i < fonts.size(); i++) delete fonts[i];
  fonts.clear();
}

float FontSource::getScaleForPixelHeight(float pixels) {
  return stbtt_ScaleForPixelHeight(&font, pixels);
}
void FontSource::getBoundsForChar(int c, float scaleX, float scaleY, float shiftX, int &x0, int &y0, int &x1, int &y1) {
  x0=y0=x1=y1=0;
  if (data == NULL) return;
  stbtt_GetCodepointBitmapBoxSubpixel(&font,c,scaleX,scaleY,shiftX,0,&x0,&y0,&x1,&y1);
}

void FontSource::getBounds(const String &text, float scaleX, float scaleY, int &x0, int &y0, int &x1, int &y1) {
  x0 = y0 = x1 = y1 = 0;
  if (data == NULL) return;
  float xpos = 0;  
  float ypos = 0;  

  int ascent; stbtt_GetFontVMetrics(&font, &ascent,0,0);
  int baseLine = (int)(ascent*scaleY);

  int ch = 0;
  while (text[ch]) {
    int advance,lsb,cx0=0,cy0=0,cx1=0,cy1=0; float shiftX = xpos - (float)floor(xpos);
    stbtt_GetCodepointHMetrics(&font, text[ch], &advance, &lsb); getBoundsForChar(text[ch],scaleX,scaleY,shiftX,cx0,cy0,cx1,cy1);
    cx0 += xpos;
    cy0 += ypos+baseLine;
    cx1 += xpos;
    cy1 += ypos+baseLine;
    if (cx0 < x0) x0 = cx0;
    if (cy0 < y0) y0 = cy0;
    if (cx1 > x1) x1 = cx1;
    if (cy1 > y1) y1 = cy1;
    xpos += advance * scaleX;
    if (text[ch+1]) xpos += scaleX*stbtt_GetCodepointKernAdvance(&font, text[ch],text[ch+1]);
    ++ch;
  }
}

GLuint FontSource::createTexture(const String &text, int fontSize, float aspect, int &width, int &height, unsigned int col) {
  width = 0;
  height = 0;

  if (data == NULL) return 0;

  int x0,y0,x1,y1;
  float scale = getScaleForPixelHeight(fontSize); float scaleX = scale, scaleY = scale;
  if (aspect != 0) scaleX /= aspect;
  getBounds(text,scaleX,scaleY,x0,y0,x1,y1);
  width = x1 - x0;
  height = y1 - y0;
  float xpos = 0;  
  float ypos = 0;  

  int ascent; stbtt_GetFontVMetrics(&font, &ascent,0,0);
  int baseLine = (int)(ascent*scaleY);

  Array<unsigned int> texture; texture.resize(width*height); for (int i = 0; i < width*height; i++) texture[i] = col & 0x00ffffff;

  int ch = 0;
  static Array<unsigned char> buffer;
  while (text[ch]) {
    int advance,lsb,cx0=0,cy0=0,cx1=0,cy1=0; float shiftX = xpos - (float)floor(xpos);
    stbtt_GetCodepointHMetrics(&font, text[ch], &advance, &lsb); getBoundsForChar(text[ch],scaleX,scaleY,shiftX,cx0,cy0,cx1,cy1);
    int w = cx1-cx0;
    int h = cy1-cy0;
    int pitch = w;
    if (buffer.size()<w*h) buffer.resize(w*h);
    stbtt_MakeCodepointBitmapSubpixel(&font,&buffer[0],w,h,pitch,scaleX,scaleY,shiftX,0,text[ch]);
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        unsigned char val = buffer[x+y*pitch];
        int xd = x + (int)floor(xpos)+cx0;
        int yd = y + (int)floor(ypos)+cy0+baseLine;
        if ((unsigned int)xd < width && (unsigned int)yd < height) {
          unsigned int col = texture[xd+yd*width];
          unsigned int a = (col >> 24) & 255;
          a = 255-(255-a)*(255-val)/255;
          col = (col & 0x00ffffff) | (a <<24);
          texture[xd+yd*width] = col;
        }
      }
    }
    xpos += advance * scaleX;
    if (text[ch+1]) xpos += scaleX*stbtt_GetCodepointKernAdvance(&font, text[ch],text[ch+1]);
    ++ch;
  }
  
  GLuint ret = 0;
  glGenTextures(1,&ret);
  glBindTexture(GL_TEXTURE_2D,ret);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, width, height, 0, GL_RGBA, GL_BYTE, &texture[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);

  return ret;
}

void addFontSource(Array<unsigned char> *data, const String &fileName) {
  if (data == NULL) return;
  FontSource *f = new FontSource(data,fileName);
  fonts.push_back(f);
  Layer *l = currentCanvas->getActiveLayer();
  if (l != NULL) {
    l->toolInfo.fontId = f->id;
  }
}

void removeFont(unsigned int fontId) {
  for (int i = 0; i < fonts.size(); i++) {
    FontSource *f = fonts[i];
    if (f->id == fontId) {
      delete f;
      fonts.erase(i,1);
      break;
    }
  }
}

Array<FontSource *> fonts;

void reScaleTexts() {
  for (int i = 0; i < fonts.size(); i++) {
    FontSource *f = fonts[i];
    if (f->glHandle != 0) {glDeleteTextures(1,&f->glHandle); f->glHandle = 0;}
  }
}

void getTextBounds(FontSource *f, Layer *l, int &x0, int &y0, int &x1, int &y1, bool andDraw, Array<float> &lineWidths) {
  if (!andDraw) {x0 = y0 = x1 = y1 = 0;}
  if (f->data == NULL) return;
  float xpos = 0;  
  float ypos = 0;  

  float xposStart = xpos;

  float scaleOne = f->getScaleForPixelHeight(1);
  float scale = f->getScaleForPixelHeight(l->toolInfo.fontSize);
  float scaleX = l->toolInfo.fontScaleX * scale;
  float scaleY = l->toolInfo.fontScaleY * scale;
  if (scaleX > scaleOne * MAXGLYPHSIZE) scaleX = scaleOne * MAXGLYPHSIZE;
  if (scaleY > scaleOne * MAXGLYPHSIZE) scaleY = scaleOne * MAXGLYPHSIZE;

  int ascent; stbtt_GetFontVMetrics(&f->font, &ascent,0,0);
  int baseLine = (int)(ascent*scaleY);

  int ch = 0;
  int lw = 0;
  float lastLineWidth = 0;
  while (l->toolInfo.textEditText[ch]) {
    int advance,lsb,cx0=0,cy0=0,cx1=0,cy1=0; float shiftX = xpos - (float)floor(xpos);
    if (l->toolInfo.textEditText[ch] == '\n') {
      if (!andDraw) {lineWidths.push_back(lastLineWidth);} else {lw++;}
      stbtt_GetCodepointHMetrics(&f->font, ' ', &advance, &lsb); f->getBoundsForChar(' ',scaleX,scaleY,shiftX,cx0,cy0,cx1,cy1);
      xpos = xposStart;
      ypos += baseLine + l->toolInfo.fontSpaceY*l->toolInfo.fontScaleY;
      cx0 += xpos;
      cy0 += ypos+baseLine;
      cx1 += xpos;
      cy1 += ypos+baseLine;
      if (!andDraw) {
        if (cx0 < x0) x0 = cx0;
        if (cy0 < y0) y0 = cy0;
        if (cx1 > x1) x1 = cx1;
        if (cy1 > y1) y1 = cy1;
      }
      lastLineWidth = 0;
    } else {
      stbtt_GetCodepointHMetrics(&f->font, l->toolInfo.textEditText[ch], &advance, &lsb); f->getBoundsForChar(l->toolInfo.textEditText[ch],scaleX,scaleY,shiftX,cx0,cy0,cx1,cy1);
      cx0 += xpos;
      cy0 += ypos+baseLine;
      cx1 += xpos;
      cy1 += ypos+baseLine;
      if (!andDraw) {
        if (cx0 < x0) x0 = cx0;
        if (cy0 < y0) y0 = cy0;
        if (cx1 > x1) x1 = cx1;
        if (cy1 > y1) y1 = cy1;
      }
      lastLineWidth = cx1 - xposStart;

      if (andDraw) {
        float lineWidth = lw<lineWidths.size() ? lineWidths[lw] : 100;
        static Array<unsigned char> buffer;
        int w = cx1-cx0;
        int h = cy1-cy0;
        int px = cx0;
        int py = cy0;
        switch (l->toolInfo.fontAlignment) {
        case ALIGN_LEFT: {} break;
        case ALIGN_RIGHT: {px = x1-lineWidth+(px-xposStart);} break;
        case ALIGN_CENTER: {px = px + ((x1-x0)-lineWidth)*0.5;} break;
        }
        int pitch = w;
        if (buffer.size()<w*h) buffer.resize(w*h);
        stbtt_MakeCodepointBitmapSubpixel(&f->font,&buffer[0],w,h,pitch,scaleX,scaleY,shiftX,0,l->toolInfo.textEditText[ch]);
        for (int y = 0; y < h; y++) {
          for (int x = 0; x < w; x++) {
            unsigned char val = buffer[x+y*pitch];
            int xd = x + px;
            int yd = y + py;
            if ((unsigned int)xd < l->width && (unsigned int)yd < l->height) {
              unsigned int col = l->pixels[xd+yd*l->width];
              unsigned int a = (col >> 24) & 255;
              a = 255-(255-a)*(255-val)/255;
              col = (col & 0x00ffffff) | (a <<24);
              l->pixels[xd+yd*l->width] = col;
            }
          }
        }
      }
      xpos += advance * scaleX;
      if (l->toolInfo.textEditText[ch+1]) {
        if (l->toolInfo.textEditText[ch+1] != '\n') {xpos += l->toolInfo.fontSpaceX*l->toolInfo.fontScaleX;}
        xpos += scaleX*stbtt_GetCodepointKernAdvance(&f->font, l->toolInfo.textEditText[ch],l->toolInfo.textEditText[ch+1]);
      }
    }
    ++ch;
  }
  if (!andDraw) lineWidths.push_back(lastLineWidth);
}

void paintTextIntoLayer(class Layer *l) {
  FontSource *f = NULL;
  for (int i = 0; i < fonts.size(); i++) {
    if (fonts[i]->id == l->toolInfo.fontId) {
      f = fonts[i];
      break;
    }
  }
  if (f == NULL) return;
  shouldUpdateAll = true;
  int x0=0,y0=0,x1=0,y1=0;
  Array<float> lineWidths;
  getTextBounds(f, l, x0, y0, x1, y1, false, lineWidths);
  int width = x1 - x0;
  int height = y1 - y0;
  if (width*height==0) return;
  l->setContentSize(width,height);
  unsigned int col = l->toolInfo.fontColor;
  for (int i2 = 0; i2 < width*height; i2++) l->pixels[i2] = col & 0x00ffffff;
  getTextBounds(f, l, x0, y0, x1, y1, true, lineWidths);
}
