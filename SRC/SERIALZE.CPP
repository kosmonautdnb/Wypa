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
#include "SERIALZE.HPP"
#include "LAYERS.HPP"
#include "COLORFLD.HPP"
#include "CONFIG.HPP"
#include "STL/IMAGE.HPP"
#include "STL/ARRAY.HPP"
#include "STL/DOS.HPP"
#include "STL/PSDIMAGE.HPP"
#include "TOOLVIEW.HPP"
#include "TOOLS.HPP"
#include "STL\NANOSVG.HPP"
#include "STL\NSVGRAST.HPP"
#include "DIALOGS.HPP"
#include <math.h>

#define RGBAIMAGEFROMFILE(__image__,__path__,__fileName__) String fullFileName = String(__path__) + String(__fileName__); Array<unsigned char> arr; getFileArray(&arr,fullFileName); RGBAImage __image__ = RGBAImage::fromMemory(&(arr[0]), arr.size());

#define LONGFILENAME(__str__) (longFileNames ? longFileName(__str__) : unixFileName(__str__))
#define SHORTFILENAME(__str__) (longFileNames ? shortFileName(__str__) : unixFileName(__str__))
bool longFileNames = false;
char longFileNameBuffer[MAX_LFN_PATH];

String fromLocalFileName(const String &str) {
  String ret = unixFileName(str);
  if (ret.startsWith("./")) ret = appFolder + ret.substr(2);
  return ret;
}

int statusBarProgress = 0;

void updateStatusBar() {
  int statusBarHeight = 16;
  int statusBarWidth = main_window_xres / 4;
  int statusBarX0 = glFrameBufferWidth/2-statusBarWidth/2;
  int statusBarY0 = glFrameBufferHeight/2-statusBarHeight/2;
  int statusBarX1 = statusBarX0 + statusBarWidth;
  int statusBarY1 = statusBarY0 + statusBarHeight;
  int statusBarParts = 16;
  int cell = statusBarProgress % statusBarParts;
  for (int x = statusBarX0; x < statusBarX1; x++) {
    int c = (x-statusBarX0)*statusBarParts/(statusBarX1-statusBarX0);
    unsigned int col = 0xff000000;
    if (c == cell) col = 0xff00ff00;
    for (int y = statusBarY0; y < statusBarY1; y++) {
      glFrameBuffer[x+y*glFrameBufferWidth] = col;
    }
  }
  statusBarProgress++;
  if (refreshMasked) glRefreshMasked(); else glRefresh();
}

void initStatusBar() {
  statusBarProgress = 0;
  updateStatusBar();
}

String longFileName(const String &a) {
  if (a[0]>='A'&&a[0]<='B') return unixFileName(a);
  if (!doslfnLongPath(longFileNameBuffer,dosFileName(a).c_str())) return unixFileName(a);
  return unixFileName(String(longFileNameBuffer));
}

String shortFileName(const String &a) {
  if (a[0]>='A'&&a[0]<='B') return unixFileName(a);
  if (!doslfnShortPath(longFileNameBuffer,dosFileName(a).c_str())) return unixFileName(a);
  return unixFileName(String(longFileNameBuffer));
}

String tempName;

String getTempFileName(const char *path) {
  String name = String(path)+"TMP.TMP";
  dosRemoveFile(name.c_str());
  return name;
}

FILE *createTempFile(const char *path, const char *writeMode) {
  tempName = getTempFileName(path);
  return fopen(dosFileName(tempName).c_str(),writeMode);
}

void renameTempFile(const char *path, const char *fileName) {
  String tempName = String(path)+"TMP.TMP";
  String fullName = String(path) + fileName;
  if (!doslfnDeleteFiles(fullName.c_str(),false)) {
    dosRemoveFile(fullName.c_str());
  }
  if (!doslfnRename(tempName.c_str(),fullName.c_str())) {
    rename(tempName.c_str(),fullName.c_str());
  }
}

void getFileArray(Array<unsigned char> *arr,const String &maybeExtendedFileName) {
  arr->resize(0);
  String fullFileName = shortFileName(dosFileName(maybeExtendedFileName));
  FILE *in = fopen(dosFileName(fullFileName).c_str(),"rb");
  if (in == NULL) return;
  fseek(in,0,SEEK_END);
  unsigned int fileLen = ftell(in);
  fseek(in,0,SEEK_SET);
  arr->resize(fileLen);
  fread(&(*arr)[0],1,fileLen,in);
  fclose(in);
}

void loadImage(const char *shortPath, const char *longFileName) {
  addPicture(String(shortPath)+String(longFileName));
}

void insertImageLayers(const char *shortPath, const char *longFileName) {
  insertPicture(String(shortPath)+String(longFileName));
}

void exportLayers(const char *shortPath, const char *longFileName) {
  Layer *l = currentCanvas->getSelectionAsLayer(false,false,true);
  if (l == NULL) return;
  tempName = getTempFileName(shortPath);
  RGBAImage img;
  img.data = l->pixels;
  img.width = l->width;
  img.height = l->height;
  img.savePNG(tempName.c_str());
  renameTempFile(shortPath, longFileName);
  delete l;
}

void exportPNG(const char *shortPath, const char *longFileName) {
  Layer *l = currentCanvas->getSelectionAsLayer(false,true,true);
  if (l == NULL) return;
  tempName = getTempFileName(shortPath);
  RGBAImage img;
  img.data = l->pixels;
  img.width = l->width;
  img.height = l->height;
  img.savePNG(tempName.c_str());
  renameTempFile(shortPath, longFileName);
  delete l;
}

void exportLayersRaw(const char *shortPath, const char *longFileName, bool applyColorTransform) {
  int k = 0;
  initStatusBar();
  for (int i = 0; i < currentCanvas->layers.size(); i++) {
    Layer *l = currentCanvas->layers[i];
    if ((l->state.selected||l->state.active) && l->width*l->height>0) {
      tempName = getTempFileName(shortPath);
      RGBAImage img;
      img.data = new unsigned int[l->width*l->height];
      memcpy(img.data,l->pixels,l->width*l->height*sizeof(unsigned int));
      if (applyColorTransform && l->useColorTransform) {
        for (int j = 0; j < l->width*l->height; j++) img.data[j] = l->colorTransform.applyColorTransform(img.data[j]);
      }
      img.width = l->width;
      img.height = l->height;
      img.savePNG(tempName.c_str());
      delete[] img.data;
      String longName = String(longFileName);
      String name = getName(longName);
      String ext = getExtension(longName);
      longName = name + String::fromInt(k) + ext; k++;
      renameTempFile(shortPath, longName.c_str());
      updateStatusBar();
    }
  }
}

void insertPSD(const char *shortPath, const char *longFileName) {
  String fullFileName = shortFileName(dosFileName(String(shortPath)+String(longFileName)));
  PSDImage *img = new PSDImage();
  if (img->Load(dosFileName(fullFileName).c_str())) {
    int pos = 0;
    for (int i = 0; i < currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.active) {pos = i; break;}
    int w = img->header.Columns;
    int h = img->header.Rows;
    int l = img->NumLayers();
    if (h > 0 && w > 0 && l > 1) {
      for (int i = 0; i < l; i++) {
        int width,height,offset_x,offset_y;
        unsigned int *imageData = img->GetLayerData(i, width, height, offset_x, offset_y);
        if (imageData==NULL) continue;
        if (width == 0) continue;
        if (height == 0) continue;
        Layer *l = new Layer();
        l->init(width,height);
        for (int j = 0; j < width * height; j++) {
          l->pixels[j] = swapRB(imageData[j]);
        }
        l->transform.translate(-offset_x,-offset_y);
        const PSDImage::psd_layer *k = img->GetLayer(i);
        memcpy(l->name,k->name,LAYERNAMEMAX);
        l->name[LAYERNAMEMAX-1] = 0;
        l->state.visible = k->hidden ? false : true;
        bool has_opacity = k->has_opacity ? true : false;
        l->state.alpha = has_opacity ? k->opacity/255.0 : 1;
        currentCanvas->insertLayer(l,pos); pos++;
      }
    }
  }
  delete img;
}

NSVGimage* image = NULL;

RGBAImage getSVGImage(const String &fullFileName, float scale) {
  RGBAImage ret;
  NSVGimage *svg = nsvgParseFromFile(dosFileName(shortFileName(dosFileName(fullFileName))).c_str(), "px", 96);
  if (svg == NULL) {
    return ret;
  }
  NSVGrasterizer* rast = nsvgCreateRasterizer();
  int w = ceil(svg->width*scale);
  int h = ceil(svg->height*scale);
  unsigned int* img = (unsigned int*)malloc(w*h*4);
  nsvgRasterize(rast, svg, 0,0,scale, (unsigned char*)img, w, h, w*4);
  nsvgDelete(svg);
  nsvgDeleteRasterizer(rast);
  for (int j = 0; j < w * h; j++) {
    img[j] = swapRB(img[j]);
  }
  ret.data = img;
  ret.width = w;
  ret.height = h;
  return ret;
}

float getSVGImageScaleForWidth(const String &fullFileName, float width) {
  NSVGimage *svg = nsvgParseFromFile(dosFileName(shortFileName(dosFileName(fullFileName))).c_str(), "px", 96);
  if (svg == NULL) {
    return 0;
  }
  float scale = (float)width/svg->width;
  nsvgDelete(svg);
  return scale;
}

void insertSVGResized(int layerWidth, int layerHeight, float scale)
{
  if (image == NULL) return;
  NSVGrasterizer* rast = nsvgCreateRasterizer();
  int w = ceil(layerWidth*scale);
  int h = ceil(layerHeight*scale);
  unsigned int* img = (unsigned int*)malloc(w*h*4);
  nsvgRasterize(rast, image, 0,0,scale, (unsigned char*)img, w, h, w*4);
  nsvgDelete(image);
  nsvgDeleteRasterizer(rast);
  Layer *l = new Layer();
  l->init(w,h);
  for (int j = 0; j < w * h; j++) {
    l->pixels[j] = swapRB(img[j]);
  }
  free(img);
  int pos = 0;
  for (int i = 0; i < currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.active) {pos = i; break;}
  currentCanvas->insertLayer(l,pos);
}

void insertSVG(const char *shortPath, const char *longFileName) {
  String fullFileName = shortFileName(dosFileName(String(shortPath)+String(longFileName)));
  image = nsvgParseFromFile(dosFileName(fullFileName).c_str(), "px", 96);
  if (image == NULL) {
    return;
  }
  dialogInsertSVG(true,image->width,image->height);
}

Array<String> getLines(const char *string) {
  Array<String> ret;
  int len = strlen(string);
  String k="";
  for (int i = 0; i < len; i++) {
    char c = string[i];
    if (c == 0x0d || c == 0x0a) {
      ret.push_back(k);
      k = "";      
      if (i < len-1 && (c == 0x0d && string[i+1]==0x0a)) {i++;}
      if (i < len-1 && (c == 0x0a && string[i+1]==0x0d)) {i++;}
      continue;
    }
    k += String::fromChar(c);
  }
  return ret;
}

void loadPalette(const char *shortPath, const char *longFileName) {
  Array<unsigned char> arr;
  getFileArray(&arr,String(shortPath)+String(longFileName));
  if (arr.empty()) return;
  const char *str = (char*)&arr[0];
  Array<String> lines = getLines(str);
  if (lines.size()<1) return;
  int a[4];
  if (sscanf(lines[0].c_str(),"%d",&a[0])==1) paletteColorsPerRow = a[0];
  palette.clear();
  for (int i = 1; i < lines.size(); i++) {
    if (sscanf(lines[i].c_str(),"%d %d %d %d",&a[0],&a[1],&a[2],&a[3])==4) {
      if (a[0]<0) a[0] = 0;
      if (a[1]<0) a[1] = 0;
      if (a[2]<0) a[2] = 0;
      if (a[3]<0) a[3] = 0;
      if (a[0]>255) a[0] = 255;
      if (a[1]>255) a[1] = 255;
      if (a[2]>255) a[2] = 255;
      if (a[3]>255) a[3] = 255;
      palette.push_back(getImVec4Color(a[0]+(a[1]<<8)+(a[2]<<16)+(a[3]<<24)));
    }
  }
  updatePalette();
}

void savePalette(const char *shortPath, const char *longFileName) {
  FILE *out = createTempFile(shortPath,"w");

  fprintf(out,"%d\n",paletteColorsPerRow);
  for (int i = 0; i < palette.size(); i++) {
    int r = int(palette[i].x*255.f);
    int g = int(palette[i].y*255.f);
    int b = int(palette[i].z*255.f);
    int a = int(palette[i].w*255.f);
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (a < 0) a = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    if (a > 255) a = 255;
    fprintf(out,"%d %d %d %d\n",r,g,b,a);
  }
  fclose(out);
  renameTempFile(shortPath, longFileName);
}

void loadGradient(const char *shortPath, const char *longFileName) {
  Array<unsigned char> arr;
  getFileArray(&arr,String(shortPath)+String(longFileName));
  if (arr.empty()) return;
  const char *str = (char*)&arr[0];
  Array<String> lines = getLines(str);
  if (lines.size()<1) return;
  gradient.elements.clear();
  float pos;
  int a[4]; 
  char step;
  for (int i = 0; i < lines.size(); i++) {
    if (sscanf(lines[i].c_str(),"%f %d %d %d %d %c",&pos,&a[0],&a[1],&a[2],&a[3],&step)==6) {
      if (a[0]<0) a[0] = 0;
      if (a[1]<0) a[1] = 0;
      if (a[2]<0) a[2] = 0;
      if (a[3]<0) a[3] = 0;
      if (a[0]>255) a[0] = 255;
      if (a[1]>255) a[1] = 255;
      if (a[2]>255) a[2] = 255;
      if (a[3]>255) a[3] = 255;
      GradientElement e;
      e.color = a[0]+(a[1]<<8)+(a[2]<<16)+(a[3]<<24);
      e.isStep = step == 's' ? true : false;
      if (pos < 0) pos = 0;
      if (pos > 100000) pos = 100000;
      e.position = pos;
      gradient.elements.push_back(e);
    }
  }
  updateGradient();
}

void saveGradient(const char *shortPath, const char *longFileName) {
  FILE *out = createTempFile(shortPath,"w");

  for (int i = 0; i < gradient.elements.size(); i++) {
    GradientElement *e = &gradient.elements[i];
    float pos = e->position;
    int r = e->color & 255;
    int g = (e->color>>8) & 255;
    int b = (e->color>>16) & 255;
    int a = (e->color>>24) & 255;
    bool step = e->isStep;
    fprintf(out,"%f %d %d %d %d %c\n",pos,r,g,b,a,step ? 's' : 'l');
  }
  fclose(out);
  renameTempFile(shortPath, longFileName);
}

#define SVGSAMPLEBRUSHWIDTH 256

void loadBrushShape(const char *shortPath, const char *longFileName) {
  if ((unsigned int)currentBrush>=brushes.size()) return;
  Brush *brush = brushes[currentBrush];
  String fullFileName = shortFileName(dosFileName(String(shortPath)+String(longFileName)));
  float scale = getSVGImageScaleForWidth(fullFileName, SVGSAMPLEBRUSHWIDTH);
  if (scale == 0) {brush->isDistanceField = false; brush->update(); return;}
  RGBAImage rgba = getSVGImage(fullFileName,scale);
  if (rgba.data == NULL) {brush->isDistanceField = false; brush->update(); return;}
  scanSampleDistanceField(brush, rgba);
  brush->isDistanceField = true;
  brush->update();
  rgba.free();
}

Layer *loadImage(const String &filePath, const String &fileName) {
  Layer *ret = NULL;
  RGBAIMAGEFROMFILE(img,filePath,fileName);
  if (img.data == NULL) {
    if (toLower(getExtension(fileName)) == ".svg") {
      String fullFileName = shortFileName(dosFileName(String(filePath)+String(fileName)));
      img = getSVGImage(fullFileName,1);
    }
  }

  if (img.data != NULL) {
    ret = new Layer();
    ret->init(img.width,img.height);
    ret->pixels = img.data;
    // no free of img needed here
  }
  return ret;
}

double updateToolTip(unsigned int *rgba, int width, int height, const char *path, const char *fileName) {
  RGBAIMAGEFROMFILE(image, path, fileName);
  if (image.data != NULL && image.width > 0 && image.height > 0) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        rgba[x+y*width] = image.data[x*image.width/width + y*image.height/height * image.width];
      }
    }
    double aspect = (double)image.width/image.height;
    image.free();
    return aspect;
  }
  return 0;
}
