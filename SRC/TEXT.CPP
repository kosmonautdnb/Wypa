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
  for (int i = 0; i < (int)fonts.size(); i++) delete fonts[i];
  fonts.clear();
}

float FontSource::getScaleForPixelHeight(float pixels) {
  if (data == NULL) return pixels;
  return stbtt_ScaleForPixelHeight(&font, pixels);
}
void FontSource::getBoundsForChar(int c, float scaleX, float scaleY, float shiftX, int &x0, int &y0, int &x1, int &y1) {
  x0=y0=x1=y1=0;
  if (data == NULL) return;
  stbtt_GetCodepointBitmapBoxSubpixel(&font,c,scaleX,scaleY,shiftX,0,&x0,&y0,&x1,&y1);
}

void FontSource::getBounds(const unsigned short *text, float scaleX, float scaleY, int &x0, int &y0, int &x1, int &y1) {
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

bool isHex(char c) {
  if (c >= '0' && c <= '9') return true;
  if (c >= 'a' && c <= 'f') return true;
  if (c >= 'A' && c <= 'F') return true;
  return false;
}

int fromHex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0;
}

#define NOFONTCOLOR 0x00010302

unsigned short *unicodeTextLine(const String &text, Array<unsigned int> *cols = NULL) {
  unsigned short *t = new unsigned short[text.length()+1]; if (cols != NULL) {cols->resize(text.length()+1); cols->fill(NOFONTCOLOR);}
  int i = 0;
  int j = 0;
  for (; i < text.length(); i++) {
    unsigned char c = text[i];
    if (c == 0) break;
    if (c == '^' && i < text.length()-4 && isHex(text[i+1]) && isHex(text[i+2]) && isHex(text[i+3]) && isHex(text[i+4])) {
      unsigned int h = fromHex(text[i+1])*0x1000+fromHex(text[i+2])*0x100+fromHex(text[i+3])*0x10+fromHex(text[i+4]);
      t[j] = (unsigned short)h;  
      i += 4;
    } else {
      if (c == '^' && i < text.length()-9 && text[i+1] == '#')  {
        unsigned int col = 0;
        i += 2;
        for (int k = 0; k < 4; k++) {
          col <<= 8;
          col |= fromHex(text[i+0])*16 + fromHex(text[i+1]);
          i += 2;
        }
        col = ((col & 0x00ff0000)>>16)|((col & 0x000000ff)<<16)|(col & 0xff00ff00);
        if (cols != NULL) (*cols)[j] = col;
        i--;
        j--;
      } else {
        t[j] = c;
      }
    }
    j++;
  }
  t[j] = 0;
  return t;
}

void FontSource::getBounds(const String &text, float scaleX, float scaleY, int &x0, int &y0, int &x1, int &y1) {
  unsigned short *t = unicodeTextLine(text);
  getBounds(t,scaleX,scaleY,x0,y0,x1,y1);
  delete[] t;
}

bool FontSource::hasGlyph(unsigned short uniCodeCodePoint) {
  if (data == NULL) return false;
  int glyph = stbtt_FindGlyphIndex(&font, uniCodeCodePoint);
  return glyph != 0;
}

GLuint FontSource::createTexture(const unsigned short *text, int fontSize, float aspect, int &width, int &height, unsigned int col) {
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
    if ((int)buffer.size()<w*h) buffer.resize(w*h);
    stbtt_MakeCodepointBitmapSubpixel(&font,&buffer[0],w,h,pitch,scaleX,scaleY,shiftX,0,text[ch]);
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        unsigned char val = buffer[x+y*pitch];
        int xd = x + (int)floor(xpos)+cx0;
        int yd = y + (int)floor(ypos)+cy0+baseLine;
        if ((unsigned int)xd < (unsigned int)width && (unsigned int)yd < (unsigned int)height) {
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

GLuint FontSource::createTexture(const String &text, int fontSize, float aspect, int &width, int &height, unsigned int col) {
  unsigned short *t = unicodeTextLine(text);
  GLuint ret = createTexture(t,fontSize,aspect,width,height,col);
  delete[] t;
  return ret;
}

void addFontSource(Array<unsigned char> *data, const String &fileName) {
  if (data == NULL) return;
  FontSource *f = new FontSource(data,fileName);
  if (f->data != NULL) {
    fonts.push_back(f);
    Layer *l = currentCanvas->getActiveLayer();
    if (l != NULL) {
      l->toolInfo.fontId = f->id;
    }
  }
}

void removeFont(unsigned int fontId) {
  for (int i = 0; i < (int)fonts.size(); i++) {
    FontSource *f = fonts[i];
    if (f->id == fontId) {
      delete f;
      fonts.erase(i,1);
      break;
    }
  }
}

Layer *getFontLayer(const char *text, float pixelHeight, Array<unsigned char> *fontData) {
  Layer *ret = NULL;
  FontSource *f = new FontSource(fontData,"");
  ret = new Layer(); // idCountup is reversed outside
  sprintf(ret->toolInfo.textEditText,"%s",text);
  ret->toolInfo.fontSize = pixelHeight;
  ret->toolInfo.fontColor = 0xff000000;
  ret->toolInfo.fontId = f->id;
  fonts.push_back(f);
  paintTextIntoLayer(ret);
  fonts.resize(fonts.size()-1);
  delete f;
  return ret;
}


Array<FontSource *> fonts;

void reScaleTexts() {
  for (int i = 0; i < (int)fonts.size(); i++) {
    FontSource *f = fonts[i];
    if (f->glHandle != 0) {glDeleteTextures(1,&f->glHandle); f->glHandle = 0;}
  }
}

void getTextBounds(FontSource *f, Layer *l, int &x0, int &y0, int &x1, int &y1, bool andDraw, Array<float> &lineWidths) {
  if (!andDraw) {x0 = y0 = x1 = y1 = 0;}
  if (f->data == NULL) return;
  float xpos = 0;  
  float ypos = 0;  

  unsigned int col2 = l->toolInfo.fontColor;

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

  Array<unsigned int> cols; unsigned short *textEditText2 = unicodeTextLine(l->toolInfo.textEditText, &cols);

  unsigned int col3 = col2;
  while (textEditText2[ch]) {
    if (cols[ch] != NOFONTCOLOR) {col2 = cols[ch]; if (col2 == 0x00000000) col2 = col3;}
    int advance,lsb,cx0=0,cy0=0,cx1=0,cy1=0; float shiftX = xpos - (float)floor(xpos);
    if (textEditText2[ch] == '\n') {
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
      stbtt_GetCodepointHMetrics(&f->font, textEditText2[ch], &advance, &lsb); f->getBoundsForChar(textEditText2[ch],scaleX,scaleY,shiftX,cx0,cy0,cx1,cy1);
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
        float lineWidth = lw<(int)lineWidths.size() ? lineWidths[lw] : 100;
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
        if ((int)buffer.size()<w*h) buffer.resize(w*h);
        stbtt_MakeCodepointBitmapSubpixel(&f->font,&buffer[0],w,h,pitch,scaleX,scaleY,shiftX,0,textEditText2[ch]);
        for (int y = 0; y < h; y++) {
          for (int x = 0; x < w; x++) {
            unsigned char val = buffer[x+y*pitch];
            int xd = x + px;
            int yd = y + py;
            if ((unsigned int)xd < (unsigned int)l->width && (unsigned int)yd < (unsigned int)l->height) {
              int val2 = (int)val * ((int)(col2>>24) & 255) / 255;
              unsigned int col = l->pixels[xd+yd*l->width];
              unsigned int a = (col >> 24) & 255;
              a = 255-(255-a)*(255-val2)/255;
              col = 0x00000000; if (a != 0x00) {
                int r = (col2 & 0xff)*a/255;
                int g = ((col2>>8) & 0xff)*a/255;
                int b = ((col2>>16) & 0xff)*a/255;
                col = r|(g<<8)|(b<<16)|(a <<24);
              }
              l->pixels[xd+yd*l->width] = col;
            }
          }
        }
      }
      xpos += advance * scaleX;
      if (textEditText2[ch+1]) {
        if (textEditText2[ch+1] != '\n') {xpos += l->toolInfo.fontSpaceX*l->toolInfo.fontScaleX;}
        xpos += scaleX*stbtt_GetCodepointKernAdvance(&f->font, textEditText2[ch],textEditText2[ch+1]);
      }
    }
    ++ch;
  }
  delete[] textEditText2;
  if (!andDraw) lineWidths.push_back(lastLineWidth);
}

void paintTextIntoLayer(class Layer *l) {
  FontSource *f = NULL;
  for (int i = 0; i < (int)fonts.size(); i++) {
    if (fonts[i]->id == l->toolInfo.fontId) {
      f = fonts[i];
      break;
    }
  }
  if (f == NULL) return;
  if (f->data == NULL) return;
  shouldUpdateAll = true;
  int x0=0,y0=0,x1=0,y1=0;
  Array<float> lineWidths;
  getTextBounds(f, l, x0, y0, x1, y1, false, lineWidths);
  int width = x1 - x0;
  int height = y1 - y0;
  if (width*height==0) return;
  l->setContentSize(width,height);
  //unsigned int col = l->toolInfo.fontColor;
  for (int i2 = 0; i2 < width*height; i2++) l->pixels[i2] = 0x00000000;
  getTextBounds(f, l, x0, y0, x1, y1, true, lineWidths);
}
