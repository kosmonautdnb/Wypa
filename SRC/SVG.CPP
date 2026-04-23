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
#include "SVG.HPP"

HashMap<String, SVG> svgs;

#define NANOSVG_IMPLEMENTATION
#include "STL\NANOSVG.HPP"
#define NANOSVGRAST_IMPLEMENTATION
#include "STL\NSVGRAST.HPP"
#include "STL\IMAGE.HPP"
#include "CONFIG.HPP"
#include "TOOLBAR.HPP"

#define APPICONSCALE (25.0/17.0)
#define ADDAPPICON(__a__,__b__) addSVG(__a__,"DATA/ICONS/" __b__,17*APPICONSCALE,17*APPICONSCALE,1*APPICONSCALE,aspectRatio)

#define ICONSCALE (16.0/11.0)
#define ADDICON(__a__,__b__) addSVG(__a__,"DATA/ICONS/" __b__,11*ICONSCALE,11*ICONSCALE,1*ICONSCALE,aspectRatio)

#define ICON2SCALE (16.0/24.0)
#define ADDICON2(__a__,__b__) addSVG(__a__,"DATA/ICONS/" __b__,24*ICON2SCALE,24*ICON2SCALE,1*ICON2SCALE,aspectRatio)

#define ADDTOOLBARICON(__a__,__b__) addSVG(__a__,"DATA/ICONS/" __b__,toolbar_width2,toolbar_width2,((double)toolbar_width2/24.0),aspectRatio)

SVG *SVG::free() {
  if (rgba != NULL) {::free(rgba); rgba = NULL;}
  if (glHandle != 0) {glDeleteTextures(1,&glHandle); glHandle = 0;}
  w = -1;
  h = -1;
  fileName = NULL;
  return this;
}

SVG *SVG::set(const char *fileName, unsigned int *rgba, int w, int h) {
  SVG::free();
  this->fileName = fileName;
  this->rgba = rgba;
  this->w = w;
  this->h = h;
  glGenTextures(1,&glHandle);
  glBindTexture(GL_TEXTURE_2D,glHandle);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, w, h, 0, GL_RGBA, GL_BYTE, rgba);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  return this;
}

void addSVG(const char *id, const char *fileName, int width, int height,double scale, double aspect) {
  NSVGimage* image;
  NSVGrasterizer* rast = nsvgCreateRasterizer();
  image = nsvgParseFromFile(fileName, "px", 96);
  if (image == NULL) {
    printf("some problem with loading the svg\n");
    exit(0);
  }
  int w = width;
  int h = height;
  unsigned int* img = (unsigned int*)malloc(w*h*4);
  nsvgRasterize(rast, image, 0,0,scale, (unsigned char*)img, w, h, w*4);
  nsvgDelete(image);
  nsvgDeleteRasterizer(rast);
  RGBAImage img2;
  img2.data = img;
  img2.width = w;
  img2.height = h;
  RGBAImage img3;
  img3 = img2.getResized(w/aspect*main_window_xres/main_window_yres,h); // this here somehow "blurs" the images, but is needed on FreeDOS
  img2.free();
  svgs[id].set(fileName,img3.data,img3.width,img3.height);
}

void reloadIcons() {
  svgs.clear();
  loadBaseSVGs();
}

void loadBaseSVGs() {
  ADDAPPICON("appicon","WYPA.SVG");
  ADDICON("search","SEARCH.SVG");
  ADDICON("delete","DELETE.SVG");
  double toolbar_width2 = toolbar_width-5;
  ADDTOOLBARICON("moveTool","MOVE.SVG");
  ADDTOOLBARICON("selectionTool","SELECT.SVG");
  ADDTOOLBARICON("pencilTool","PENCIL.SVG");
  ADDTOOLBARICON("paintBrushTool","BRUSH.SVG");
  ADDTOOLBARICON("eraserTool","ERASER.SVG");
  ADDTOOLBARICON("wandTool","WAND.SVG");
  ADDTOOLBARICON("gradientTool","GRADIENT.SVG");
  ADDTOOLBARICON("pipetteTool","PIPETTE.SVG");
  ADDTOOLBARICON("magicTool","BLUR.SVG");
  ADDTOOLBARICON("fillTool","FILL.SVG");
  ADDTOOLBARICON("lineTool","LINE.SVG");
  ADDTOOLBARICON("textTool","TEXT.SVG");
  ADDICON2("visible","VISIBLE.SVG");
}
