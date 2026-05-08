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
#ifndef __TEXT_HPP__
#define __TEXT_HPP__

#include "IMGUI/IMGUI.H"
#include "STL/GL.H"
#include "STL/ARRAY.HPP"
#include "STL/STRING.HPP"
#include "STL/STBTTF.HPP"

void initText();
void freeText();

extern unsigned int fontIdCounter;

class FontSource {
public:
  FontSource() {
    newOne();
  }
  FontSource(Array<unsigned char> *data, const String &fileName) {
    newOne();
    this->data = data;
    this->fileName = fileName;
    prepareFont();
  }
  void newOne() {
    fontIdCounter++; id = fontIdCounter;
    data = NULL;
    fileName = "";
    glHandle = 0;
    glWidth = 0;
    glHeight = 0;
  }
  void prepareFont() {
    stbtt_InitFont(&font, &((*data)[0]), stbtt_GetFontOffsetForIndex(&((*data)[0]),0));
  }
  ~FontSource() {
    if (data != NULL) {delete data; data = NULL;}
    if (glHandle != 0) {glDeleteTextures(1,&glHandle); glHandle = 0;}
  }

  unsigned int id;

  Array<unsigned char> *data;
  stbtt_fontinfo font;
  String fileName;

  GLuint glHandle;
  int glWidth;
  int glHeight;

  float getScaleForPixelHeight(float pixels);
  void getBoundsForChar(int c, float scaleX, float scaleY, float shiftX, int &x0, int &y0, int &x1, int &y1);
  void getBounds(const String &text, float scaleX, float scaleY, int &x0, int &y0, int &x1, int &y1);

  GLuint createTexture(const String &text, int fontSize, float aspect, int &width, int &height, unsigned int col);
};
#define S_FONT(__name__,__o__)\
  S_OBJECT_OPEN(__name__);\
  S_UINT("id",(__o__)->id);\
  if ((__o__)->id>maxFontId) maxFontId = (__o__)->id;\
  S_STRING("fileName",(__o__)->fileName);\
  {if (!save) {if ((__o__)->data != NULL) {delete (__o__)->data; (__o__)->data = NULL;} (__o__)->data = new Array<unsigned char>();} Array<unsigned char> *helper = (__o__)->data; S_ZIPARRAY(combined(String("font")+String::fromInt(i)+".ttf"),(*helper),sizeof(unsigned char));}\
  if (!save) {(__o__)->glHandle = 0; (__o__)->prepareFont();}\
  S_OBJECT_CLOSE();

void addFontSource(Array<unsigned char> *data, const String &fileName);
void removeFont(unsigned int id);
class Layer *getFontLayer(const char *text, float pixelHeight, Array<unsigned char> *fontData);

extern Array<FontSource*> fonts;

void reScaleTexts();
void paintTextIntoLayer(class Layer *l);

#define S_TEXT_CPP()\
  unsigned int maxFontId = 0;\
  S_POINTER_ARRAY("fonts",fonts,FontSource,S_FONT(NULL,fonts[i]));\
  if (!save) fontIdCounter = maxFontId+1;


#endif //__TEXT_HPP__
