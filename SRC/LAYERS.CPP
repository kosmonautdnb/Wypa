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
#include "LAYERS.HPP"
#include "CONFIG.HPP"
#include "STL/VECTOR.HPP"
#include "COLORFLD.HPP"
#include "GL.H"
#include "MENUBAR.HPP"
#include "NAVIGTOR.HPP"
#include "SERIALZE.HPP"
#include "TOOLBAR.HPP"
#include "TOOLVIEW.HPP"
#include <math.h>

Layer *copyLayer = NULL;
Canvas *currentCanvas = NULL;
Array<Canvas*> canvas;
bool shouldUpdateAll = false;
bool shouldUpdate = false;

void initLayers() {
}

void freeLayers() {
  for (int i = 0; i < canvas.size(); i++) delete canvas[i];
  canvas.clear();
}

Matrix inverse2(const Matrix &a) {
  return inverse(a); // maybe this inverse is too simple
}

const char *layerModeNames[LAYERMODE_COUNT] = {"UNDEFINED","Normal","Add","Sub","Mul","Mul2X","Premultiplied","Max","Min","Screen","Color","Luma","SoftLight","HardLight","LinearLight","PinLight","Diff"};
const char *interpolationNames[INTERPOLATION_COUNT] = {"Pixelated","Averaging","Linear"};
void LayerState::clear() {
  active = false;
  selected = false;
  visible = true;
}

void TransformationState::transformForward(double x, double y, double &rx, double &ry) {
  rx = x*transformation.m[0+0*4]+y*transformation.m[0+1*4]+transformation.m[0+3*4];
  ry = x*transformation.m[1+0*4]+y*transformation.m[1+1*4]+transformation.m[1+3*4];
}

void TransformationState::transformBackward(double x, double y, double &rx, double &ry) {
  rx = x*inverseTransformation.m[0+0*4]+y*inverseTransformation.m[0+1*4]+inverseTransformation.m[0+3*4];
  ry = x*inverseTransformation.m[1+0*4]+y*inverseTransformation.m[1+1*4]+inverseTransformation.m[1+3*4];
}

void TransformationState::clear() {
  transformation.identity();
  inverseTransformation.identity();
}

void TransformationState::update() {
  inverseTransformation = inverse2(transformation);
}

void TransformationState::translate(double x, double y) {
  transformation.translate(Vector(x,y));
  update();
}

void TransformationState::scale(double x, double y) {
  transformation.scale3(Vector(x,y,1));
  update();
}

void TransformationState::scaleAround(double scaleX, double scaleY, double centerX, double centerY) {
  transformation.translate(Vector(-centerX, -centerY,0));
  Matrix sc; sc.scale3(Vector(scaleX,scaleY,1));
  transformation = sc * transformation;
  transformation.translate(Vector(centerX, centerY,0));
  update();
}

void TransformationState::fresh() {
  clear();
}

void TransformationState::zoom(double zoom) {
  zoom = zoom/100.0;
  if (zoom == 0) return;
  scaleAround(1.0/zoom,1.0/zoom,0,0);
}

void TransformationState::aspect(double aspect) {
  scaleAround(aspect,1,0,0);
}

void TransformationState::pixelSize(double x, double y) {
  if (x == 0 || y == 0) return;
  scaleAround(1.0/x,1.0/y,0,0);
}


unsigned int Layer::getPixel(int x, int y) {
  if ((unsigned int)x<width&&(unsigned int)y<height) return pixels[x+y*width];
  return 0x00000000;
}

void Layer::setPixel(int x, int y, unsigned int rgba) {
  if ((unsigned int)x<width&&(unsigned int)y<height) pixels[x+y*width] = rgba;
}

void Layer::init(int width, int height) {
  transform.clear();
  state.clear();
  mode = LAYERMODE_NORMAL;
  if (pixels != NULL) {delete[] pixels; pixels = NULL;}
  if (pixelStates != NULL) {delete[] pixelStates; pixelStates = NULL;}
  pixels = new unsigned int[width*height];
  if (pixels==NULL) {
    this->width = 0;
    this->height = 0;
    return;
  }
  memset(pixels,0,width*height*sizeof(unsigned int));
  pixelStates = new unsigned char[width*height];
  if (pixelStates==NULL) {
    this->width = 0;
    this->height = 0;
    return;
  }
  memset(pixelStates,0,width*height*sizeof(unsigned char));
  this->width = width;
  this->height = height;
}

void Layer::mirrorHorizontally() {
  Matrix center; center.translate(Vector((double)width/2,0,0));
  Matrix modify; modify.scale3(Vector(-1,1,1));
  transform.transformation = transform.transformation * center * modify * inverse2(center);
  transform.update();
  shouldUpdateAll = true;
}

void Layer::mirrorVertically() {
  Matrix center; center.translate(Vector(0,(double)height/2,0));
  Matrix modify; modify.scale3(Vector(1,-1,1));
  transform.transformation = transform.transformation * center * modify * inverse2(center);
  transform.update();
  shouldUpdateAll = true;
}

Matrix straighten(const Matrix &m) {
  Matrix ret;
  for (int i = 0; i < 16; i++) {
    double a = m.m[i];
    if (a < -0.5) a = -1; else
    if (a > 0.5) a = 1; else
    a = 0;
    ret.m[i] = a;
  }
  return ret;
}

void Layer::rotate90DegreesForward() {
  Vector c((double)width/2.0,(double)height/2.0,0);
  c = inverse2(transform.transformation)*c;
  Matrix center; center.translate(Vector(c.x,c.y,0));
  Matrix modify; modify = rotationMatrixAngleAxis(PI*-0.5,0,0,1);
  modify = straighten(modify);
  transform.transformation = transform.transformation * center * modify * inverse2(center);
  transform.update();
  shouldUpdateAll = true;
}

void Layer::rotate90DegreesBackward() {
  Vector c((double)width/2.0,(double)height/2.0,0);
  c = inverse2(transform.transformation)*c;
  Matrix center; center.translate(Vector(c.x,c.y,0));
  Matrix modify; modify = rotationMatrixAngleAxis(PI*0.5,0,0,1);
  modify = straighten(modify);
  transform.transformation = transform.transformation * center * modify * inverse2(center);
  transform.update();
  shouldUpdateAll = true;
}

void Layer::rotateByDegrees(double degrees) {
  Vector c((double)width/2.0,(double)height/2.0,0);
  c = inverse2(transform.transformation)*c;
  Matrix center; center.translate(Vector(c.x,c.y,0));
  Matrix modify; modify = rotationMatrixAngleAxis(-degrees*PI/180.0,0,0,1);
  transform.transformation = transform.transformation * center * modify * inverse2(center);
  transform.update();
  shouldUpdateAll = true;
}

void Layer::selectAll() {
  for (int i = 0; i < width * height; i++) pixelStates[i] |= PIXELSTATE_SELECTED;
  shouldUpdateAll = true;
}

void Layer::selectNone() {
  for (int i = 0; i < width * height; i++) pixelStates[i] &= ~PIXELSTATE_SELECTED;
  shouldUpdateAll = true;
}

void Layer::invertSelection() {
  for (int i = 0; i < width * height; i++) pixelStates[i] ^= PIXELSTATE_SELECTED;
  shouldUpdateAll = true;
}

void Layer::fillSelectionWithColor(unsigned int color) {
  for (int i = 0; i < width * height; i++) if (pixelStates[i] & PIXELSTATE_SELECTED) pixels[i] = color;
  shouldUpdateAll = true;
  addUsedColor(getImVec4Color(color));
}

void Layer::expandSelection(int pixels) {
  if (pixels == 0) return;
  unsigned char *before = new unsigned char[width*height];
  bool forward = pixels > 0;
  for (int i = 0; i < abs(pixels); i++) {
    memcpy(before,pixelStates,width*height*sizeof(unsigned char));
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        unsigned char p00 = before[x+y*width] & PIXELSTATE_SELECTED;
        unsigned char pm0 = x == 0 ? 0 : before[x-1+y*width] & PIXELSTATE_SELECTED;
        unsigned char pp0 = x == width-1 ? 0 : before[x+1+y*width] & PIXELSTATE_SELECTED;
        unsigned char p0m = y == 0 ? 0 : before[x+(y-1)*width] & PIXELSTATE_SELECTED;
        unsigned char p0p = y == height-1 ? 0 : before[x+(y+1)*width] & PIXELSTATE_SELECTED;
        if (forward) {
          unsigned char k = pm0|pp0|p0m|p0p;
          if (!p00) {
            if (k) pixelStates[x+y*width] |= PIXELSTATE_SELECTED;
          }
        } else {
          unsigned char k = pm0&pp0&p0m&p0p;
          if (p00) {
            if (!k) pixelStates[x+y*width] &= ~PIXELSTATE_SELECTED;
          }
        }
      }
    }
  }
  delete[] before;
  shouldUpdateAll = true;
}

void Layer::selectByAlpha(int alphaThreshold) {
  for (int i = 0; i < width * height; i++) {
    pixelStates[i] = pixelStates[i] & (~PIXELSTATE_SELECTED);
    if ((pixels[i]>>24) >= alphaThreshold) pixelStates[i] |= PIXELSTATE_SELECTED;
  }
  shouldUpdateAll = true;
}

bool Layer::hasSelection() {
  for (int i = 0; i < width * height; i++) if (pixelStates[i] & PIXELSTATE_SELECTED) return true;
  return false;
}

Vector Layer::boundingBox() {
  Vector ret;
  Matrix invM = inverse2(transform.transformation);
  for (int i = 0; i < 4; i++) {
    Vector k(i & 1 ? 0 : width, i & 2 ? 0 : height, 0);
    k = invM * k;
    if (i == 0 || k.x < ret.x) ret.x = k.x;
    if (i == 0 || k.y < ret.y) ret.y = k.y;
    if (i == 0 || k.x > ret.z) ret.z = k.x;
    if (i == 0 || k.y > ret.w) ret.w = k.y;
  }
  return ret;
}

void Layer::quantizeBoundingBox() {
  for (int i = 0; i < 10; i++) { // this doesn't work reliably
    Vector b = boundingBox();
    b.x = floor(b.x);
    b.y = floor(b.y);
    b.z = floor(b.z);
    b.w = floor(b.w);
    setLayerPos(b.x,b.y);
    setLayerSize(b.z-b.x,b.w-b.y);
  }
}

void layerNewContent(Layer *l, int w, int h) {
  if (w < 1) return;
  if (h < 1) return;
  if (l->pixels != NULL) {delete[] l->pixels; l->pixels = NULL;}
  if (l->pixelStates != NULL) {delete[] l->pixelStates; l->pixelStates = NULL;}
  l->width = w;
  l->height = h;
  l->pixels = new unsigned int[w*h];
  l->pixelStates = new unsigned char[w*h];
  memset(l->pixels,0,w*h*sizeof(unsigned int));
  memset(l->pixelStates,0,w*h*sizeof(unsigned char));
}

void layerGetContent(Layer *to, Layer *from) {
  int w = from->width;
  int h = from->height;
  layerNewContent(to,w,h);
  memcpy(to->pixels,from->pixels,w*h*sizeof(unsigned int));
  memcpy(to->pixelStates,from->pixelStates,w*h*sizeof(unsigned char));
}

void Layer::resizeLayerContent(int interpolation, int newWidth, int newHeight) {
  if (newWidth <= 0) return;
  if (newHeight <= 0) return;
  TransformationState before = transform;
  transform.fresh();
  transform.scale((double)width/newWidth,(double)height/newHeight);
  bakeLayer(interpolation);
  transform = before;
}

void Layer::bakeLayer(int interpolation) {

  Vector b = boundingBox();
  int x0 = (int)floor(b.x);
  int y0 = (int)floor(b.y);
  int x1 = (int)floor(b.z);
  int y1 = (int)floor(b.w);
  int newWidth = x1-x0;
  int newHeight = y1-y0;
  if (newWidth<1) return;
  if (newHeight<1) return;

  unsigned char *beforeS = new unsigned char[width*height];
  unsigned int *beforeP = new unsigned int[width*height];
  memcpy(beforeS,pixelStates,width*height*sizeof(unsigned char));
  memcpy(beforeP,pixels,width*height*sizeof(unsigned int));
  int ow = this->width;
  int oh = this->height;

  layerNewContent(this,newWidth,newHeight);

  Matrix m = transform.transformation;

  double rounding = 0.5;
  if (interpolation == INTERPOLATION_LINEAR) rounding = 0;

  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      double px = x;
      double py = y;
      double lx3 = px*m.m[0+0*4]+py*m.m[0+1*4]+m.m[0+3*4]+rounding;
      double ly3 = px*m.m[1+0*4]+py*m.m[1+1*4]+m.m[1+3*4]+rounding;
      int lx = (int)lx3;
      int ly = (int)ly3;
      if (lx == 0) lx = (int)floor(px*m.m[0+0*4]+py*m.m[0+1*4]+m.m[0+3*4]+rounding);
      if (ly == 0) ly = (int)floor(px*m.m[1+0*4]+py*m.m[1+1*4]+m.m[1+3*4]+rounding);
      if ((unsigned int)lx >= ow || (unsigned int)ly >= oh) continue;
      pixelStates[(x-x0)+(y-y0)*this->width] = beforeS[lx+ly*ow];
      switch(interpolation) {
        case INTERPOLATION_NONE: {
          pixels[(x-x0)+(y-y0)*this->width] = beforeP[lx+ly*ow];
        } break;
        case INTERPOLATION_LINEAR: {
            double fx = lx3-floor(lx3);
            double fy = ly3-floor(ly3);
            unsigned int p00 = beforeP[lx+ly*ow];
            unsigned int p10 = (lx+1) < ow ? beforeP[(lx+1)+ly*ow] : p00;
            unsigned int p01 = (ly+1) < oh ? beforeP[lx+(ly+1)*ow] : p00;
            unsigned int p11 = (ly+1) < oh && (lx+1) < ow ? beforeP[(lx+1)+(ly+1)*ow] : p00;
            double rt = ((int)(p10 & 255) - (int)(p00 & 255))*fx + (int)(p00 & 255);
            double gt = ((int)((p10>>8) & 255) - (int)((p00>>8) & 255))*fx + (int)((p00>>8) & 255);
            double bt = ((int)((p10>>16) & 255) - (int)((p00>>16) & 255))*fx + (int)((p00>>16) & 255);
            double at = ((int)((p10>>24) & 255) - (int)((p00>>24) & 255))*fx + (int)((p00>>24) & 255);
            double rb = ((int)(p11 & 255) - (int)(p01 & 255))*fx + (int)(p01 & 255);
            double gb = ((int)((p11>>8) & 255) - (int)((p01>>8) & 255))*fx + (int)((p01>>8) & 255);
            double bb = ((int)((p11>>16) & 255) - (int)((p01>>16) & 255))*fx + (int)((p01>>16) & 255);
            double ab = ((int)((p11>>24) & 255) - (int)((p01>>24) & 255))*fx + (int)((p01>>24) & 255);
            int r = floor((rb - rt) * fy + rt);
            int g = floor((gb - gt) * fy + gt);
            int b = floor((bb - bt) * fy + bt);
            int a = floor((ab - at) * fy + at);
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            if (a > 255) a = 255;
            if (r < 0) r = 0;
            if (g < 0) g = 0;
            if (b < 0) b = 0;
            if (a < 0) a = 0;
            pixels[(x-x0)+(y-y0)*this->width] = (r)|(g<<8)|(b<<16)|(a<<24);
        } break;
      }
    }
  }

  transform.fresh();
  transform.translate(-x0,-y0);
  transform.update();

  switch(interpolation) {
    case INTERPOLATION_INTERAREA: {
      unsigned int *accumulationBuffer = new unsigned int[width*height*5]; 
      memset(accumulationBuffer,0,width*height*5*sizeof(unsigned int));
      Matrix m2 = transform.transformation * inverse2(m);
      for (int y = 0; y < oh; y++) {
        for (int x = 0; x < ow; x++) {
          double lx4 = x*m2.m[0+0*4]+y*m2.m[0+1*4]+m2.m[0+3*4]+rounding;
          double ly4 = x*m2.m[1+0*4]+y*m2.m[1+1*4]+m2.m[1+3*4]+rounding;
          int lx2 = (int)lx4;
          int ly2 = (int)ly4;
          if (lx2 == 0) lx2 = (int)floor(x*m2.m[0+0*4]+y*m2.m[0+1*4]+m2.m[0+3*4]+rounding);
          if (ly2 == 0) ly2 = (int)floor(x*m2.m[1+0*4]+y*m2.m[1+1*4]+m2.m[1+3*4]+rounding);
          if ((unsigned int)lx2 >= width || (unsigned int)ly2 >= height) continue;
          unsigned int ps = beforeP[x+y*ow];
          int r = (ps) & 255;
          int g = (ps>>8) & 255;
          int b = (ps>>16) & 255;
          int a = (ps>>24) & 255;
          int pos = (lx2 + ly2 * width)*5;
          accumulationBuffer[pos+0]+=r;
          accumulationBuffer[pos+1]+=g;
          accumulationBuffer[pos+2]+=b;
          accumulationBuffer[pos+3]+=a;
          accumulationBuffer[pos+4]+=1;
        }
      }
      Matrix m3 = inverse2(m2);
      for (int y2 = 0; y2 < height; y2++) {
        for (int x2 = 0; x2 < width; x2++) {
          int pos = (x2 + y2 * width)*5;
          int w = accumulationBuffer[pos+4];
          if (w > 0) {
            int r = accumulationBuffer[pos+0]/w;
            int g = accumulationBuffer[pos+1]/w;
            int b = accumulationBuffer[pos+2]/w;
            int a = accumulationBuffer[pos+3]/w;
            pixels[x2+y2*width] = (r)|(g<<8)|(b<<16)|(a<<24);
          } else {
            double x = x2;
            double y = y2;
            double lx4 = x*m3.m[0+0*4]+y*m3.m[0+1*4]+m3.m[0+3*4]+rounding;
            double ly4 = x*m3.m[1+0*4]+y*m3.m[1+1*4]+m3.m[1+3*4]+rounding;
            int lx2 = (int)lx4;
            int ly2 = (int)ly4;
            if (lx2 == 0) lx2 = (int)floor(x*m3.m[0+0*4]+y*m3.m[0+1*4]+m3.m[0+3*4]+rounding);
            if (ly2 == 0) ly2 = (int)floor(x*m3.m[1+0*4]+y*m3.m[1+1*4]+m3.m[1+3*4]+rounding);
            unsigned int ps = 0;
            if ((unsigned int)lx2 < ow && (unsigned int)ly2 < oh) ps = beforeP[lx2+ly2*ow];
            pixels[x2+y2*width] = ps;
          }
        }
      }
      delete[] accumulationBuffer;
    } break;
  }

  delete[] beforeP;
  delete[] beforeS;
}

Layer *Layer::clone() {
  Layer *ret = new Layer();
  *ret = *this;
  ret->pixels = NULL;
  ret->pixelStates = NULL;
  ret->init(width,height);
  memcpy(ret->pixels,pixels,ret->width*ret->height*sizeof(unsigned int));
  return ret;
}

void Layer::setLayerPos(double x, double y) {
  for (int i = 0; i < 5; i++) { // maybe it's not perfect?
    Vector b = boundingBox();
    double moveX = x-b.x;
    double moveY = y-b.y;
    Vector k2 = Vector(moveX,moveY,0);
    transform.translate(-k2.x,-k2.y);
    transform.update();
  }
}

void Layer::setLayerSize(double width, double height) {
  if (width == 0 || height == 0) return;
  for (int i = 0; i < 5; i++) { // maybe it's not perfect?
    Vector b = boundingBox();
    double oldWidth = b.z-b.x;
    double oldHeight = b.w-b.y;
    if (oldWidth == 0 || oldHeight == 0) return;
    double scaleX = width / oldWidth;
    double scaleY = height / oldHeight;
    if (scaleX == 0 || scaleY == 0) return;
    Matrix modify;modify.scale3(Vector(1.0/scaleX,1.0/scaleY,1));
  
    //Vector c((double)width/2.0*0,(double)height/2.0*0,0);
    //c = inverse2(transform.transformation)*c;
    Vector c = b.xy();
    Matrix center; center.translate(Vector(c.x,c.y,0));
    transform.transformation = transform.transformation * center * modify * inverse2(center);
    transform.update();
    shouldUpdateAll = true;
  }
}

double Layer::getAngleDegrees() {
  Vector c((double)width/2.0,(double)height/2.0,0);
  Vector n((double)width/2.0,(double)height/2.0*0,0);
  c = inverse2(transform.transformation)*c;
  n = inverse2(transform.transformation)*n;
  c -= n;
  if (c.x==0&&c.y==0) return 0;
  return fmod((atan2(c.y,c.x)*180.0/PI+360-90),360.0);
}

void Layer::setAngleDegrees(double degrees) {
  if (degrees < 0 || degrees > 360.0) return;
  double currentAngle = getAngleDegrees();
  double modify = degrees - currentAngle;
  rotateByDegrees(modify);
}

void Layer::enlargenLayerBy(int left, int top, int right, int bottom) {
  int newWidth = width + left + right;
  int newHeight = height + top + bottom;
  if (newWidth < 1 || newHeight < 1) return;
  unsigned int *pixels2 = new unsigned int[newWidth*newHeight];
  unsigned char *pixelStates2 = new unsigned char[newWidth*newHeight];

  for (int y = 0; y < newHeight; y++) {
    for (int x = 0; x < newWidth; x++) {
      int sx = x - left;
      int sy = y - top;
      unsigned int col = 0x00000000;
      unsigned char state = 0;
      if ((unsigned int)sx < width && (unsigned int)sy < height) {
        col = pixels[sx+sy*width];
        state = pixelStates[sx+sy*width];
      }
      pixels2[x+y*newWidth] = col;
      pixelStates2[x+y*newWidth] = state;
    }
  }

  if (pixels != NULL) {delete[] pixels; pixels = NULL;};
  if (pixelStates != NULL) {delete[] pixelStates; pixelStates = NULL;};
  pixels = pixels2;
  pixelStates = pixelStates2;
  width = newWidth;
  height = newHeight;
  Matrix modify; modify.translate(Vector(left,top,0));
  transform.transformation = modify * transform.transformation;
  transform.update();
  shouldUpdateAll = true;
}

void CanvasControl::setCanvasTransform(Canvas *canvas) {
  canvas->transform.fresh();
  canvas->transform.translate(scrollX,scrollY);
  canvas->transform.zoom(zoom);
  canvas->transform.aspect(1*aspectRatio*glFrameBufferHeight/glFrameBufferWidth);
  canvas->transform.pixelSize(pixelSizeX,pixelSizeY);
  canvas->transform.update();
  shouldUpdateAll = true;
}

void CanvasControl::moveByPixels(Canvas *canvas, double x, double y) {
  scrollX -= x;
  scrollY -= y;
  setCanvasTransform(canvas);
}

void CanvasControl::zoomTo(Canvas *canvas, double screenPixelCenterX, double screenPixelCenterY,double zoom) {
  if (zoom < 0.01) zoom = 0.01;
  double x0,y0; x0 = screenPixelCenterX - canvas->screenPosX; y0 = screenPixelCenterY - canvas->screenPosY; canvas->transform.transformForward(x0,y0,x0,y0);
  double x1,y1; x1 = screenPixelCenterX - canvas->screenPosX; y1 = screenPixelCenterY - canvas->screenPosY; this->zoom = zoom; setCanvasTransform(canvas); canvas->transform.transformForward(x1,y1,x1,y1);
  canvas->transform.transformBackward(x0,y0,x0,y0);                                                                                                                                                                                                                                                                                  
  canvas->transform.transformBackward(x1,y1,x1,y1);                                                                                                                                                                                                                                                                                  
  scrollX -= x1 - x0;
  scrollY -= y1 - y0;
  setCanvasTransform(canvas);
}

void CanvasControl::zoomBy(Canvas *canvas, double screenPixelCenterX, double screenPixelCenterY,double step) {
  zoomTo(canvas,screenPixelCenterX,screenPixelCenterY,zoom * step);
}

void Canvas::init(int width, int height) {
  transform.clear();
  if (pixels != NULL) {delete[] pixels; pixels = NULL;}
  if (pixelStates != NULL) {delete[] pixelStates; pixelStates = NULL;}
  pixels = new unsigned int[width*height];
  if (pixels==NULL) {
    this->width = 0;
    this->height = 0;
    return;
  }
  memset(pixels,0,width*height*sizeof(unsigned int));
  pixelStates = new unsigned char[width*height];
  if (pixelStates==NULL) {
    this->width = 0;
    this->height = 0;
    return;
  }
  memset(pixelStates,0,width*height*sizeof(unsigned char));
  this->width = width;
  this->height = height;
  this->canvasX0 = 0;
  this->canvasY0 = 0;
  this->canvasX1 = width*aspectRatio*glFrameBufferHeight/glFrameBufferWidth;
  this->canvasY1 = height;
}

void Canvas::getLayerPos(Layer *layer, double screenX, double screenY, double &layerX, double &layerY) {
  double x,y;
  x = (double)screenX-screenPosX;
  y = (double)screenY-screenPosY;
  transform.transformForward(x,y,x,y);
  layer->transform.transformForward(x,y,x,y);
  layerX = x;
  layerY = y;
}

void Canvas::getCanvasPos(double screenX, double screenY, double &canvasX, double &canvasY) {
  double x,y;
  x = (double)screenX-screenPosX;
  y = (double)screenY-screenPosY;
  transform.transformForward(x,y,x,y);
  canvasX = x;
  canvasY = y;
}

void Canvas::getScreenPos(double canvasX, double canvasY, double &screenX, double &screenY) {
  double x,y;
  x = (double)canvasX;
  y = (double)canvasY;
  transform.transformBackward(x,y,x,y);
  screenX = x+screenPosX;
  screenY = y+screenPosY;
}

void Canvas::getScreenPos(Layer *layer, double layerX, double layerY, double &screenX, double &screenY) {
  double x,y;
  x = (double)layerX;
  y = (double)layerY;
  layer->transform.transformBackward(x,y,x,y);
  transform.transformBackward(x,y,x,y);
  screenX = x+screenPosX;
  screenY = y+screenPosY;
}

bool justSelection = false;

void fillCanvas(Canvas *canvas, int x0, int y0, int x1, int y1, unsigned int color, unsigned char additionalPixelState) {
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > canvas->width) x1 = canvas->width;
  if (y1 > canvas->height) y1 = canvas->height;
  if (x0 >= x1) return;
  if (y0 >= y1) return;
  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      if (!(canvas->pixelStates[x+y*canvas->width] & PIXELSTATE_UPDATED)) continue;
      canvas->pixels[x+y*canvas->width] = color;
      unsigned char a = canvas->pixelStates[x+y*canvas->width];
      a = a & (~PIXELSTATE_SELECTED);
      a |= additionalPixelState & PIXELSTATE_SELECTED;
      canvas->pixelStates[x+y*canvas->width] = a;
    }
  }
}

void updateCanvas(Canvas *canvas, int x0, int y0, int x1, int y1, Layer *layer) {
  if ((!canvas->justSingle) && (!layer->state.visible)) return;
  if (canvas->justSingle && (!layer->state.active)) return;

  bool darken = !(layer->state.selected || layer->state.active);
  if (!canvas->hilightSelection) darken = false;

  Vector b = layer->boundingBox();
  Vector b1 = b.xy();
  Vector b2; b2.x = b.z; b2.y = b.w;
  b1 = inverse2(canvas->transform.transformation) * b1;
  b2 = inverse2(canvas->transform.transformation) * b2;
  b2.x += 1; // no better solution, yet
  b2.y += 1; // no better solution, yet
  if (b2.x < x0) return;
  if (b2.y < y0) return;
  if (b1.x > x1) return;
  if (b1.y > y1) return;
  if (x0 < b1.x) x0 = b1.x;
  if (y0 < b1.y) y0 = b1.y;
  if (x1 > b2.x) x1 = b2.x;
  if (y1 > b2.y) y1 = b2.y;

  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > canvas->width) x1 = canvas->width;
  if (y1 > canvas->height) y1 = canvas->height;
  if (x0 >= x1) return;
  if (y0 >= y1) return;
  float alpha = layer->state.alpha;
  Matrix cm = canvas->transform.transformation;
  Matrix lm = layer->transform.transformation;
  Matrix m = lm * cm;

  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      int lx = (int)(x*m.m[0+0*4]+y*m.m[0+1*4]+m.m[0+3*4]);
      int ly = (int)(x*m.m[1+0*4]+y*m.m[1+1*4]+m.m[1+3*4]);
      if (lx == 0) lx = (int)floor(x*m.m[0+0*4]+y*m.m[0+1*4]+m.m[0+3*4]);
      if (ly == 0) ly = (int)floor(x*m.m[1+0*4]+y*m.m[1+1*4]+m.m[1+3*4]);
      if ((unsigned int)lx >= layer->width || (unsigned int)ly >= layer->height) continue;

      unsigned char a = canvas->pixelStates[x+y*canvas->width];
      if (!(a & PIXELSTATE_UPDATED)) continue;
      if (justSelection && (!(layer->pixelStates[lx+ly*layer->width] & PIXELSTATE_SELECTED))) continue;
      canvas->pixelStates[x+y*canvas->width] = a | (layer->pixelStates[lx+ly*layer->width] & PIXELSTATE_SELECTED);

      unsigned int col = 0xffff00ff;
      switch(layer->mode) {
      case LAYERMODE_NORMAL: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int a = (rgb1 >> 24)*alpha;
          int r = ((int)(rgb1 & 255)-(int)(rgb0 & 255))*a/255+(int)(rgb0 & 255);
          int g = ((int)((rgb1>>8) & 255)-(int)((rgb0>>8) & 255))*a/255+(int)((rgb0>>8) & 255);
          int b = ((int)((rgb1>>16) & 255)-(int)((rgb0>>16) & 255))*a/255+(int)((rgb0>>16) & 255);
          if ((rgb0>>24)>a) a = rgb0>>24;
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_ADD: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          int r = (int)(rgb0 & 255)+(int)(rgb1 & 255)*alpha2;
          int g = (int)((rgb0>>8) & 255)+(int)((rgb1>>8) & 255)*alpha2;
          int b = (int)((rgb0>>16) & 255)+(int)((rgb1>>16) & 255)*alpha2;
          if (r > 255) r = 255;       
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_SUB: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          int r = (int)(rgb0 & 255)-(int)(rgb1 & 255)*alpha2;
          int g = (int)((rgb0>>8) & 255)-(int)((rgb1>>8) & 255)*alpha2;
          int b = (int)((rgb0>>16) & 255)-(int)((rgb1>>16) & 255)*alpha2;
          if (r < 0) r = 0;
          if (g < 0) g = 0;
          if (b < 0) b = 0;
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_MUL: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int r = (int)(rgb0 & 255)*(int)(rgb1 & 255);
          int g = (int)((rgb0>>8) & 255)*(int)((rgb1>>8) & 255);
          int b = (int)((rgb0>>16) & 255)*(int)((rgb1>>16) & 255);
          r /= 255;
          g /= 255;
          b /= 255;
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_MUL2X: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int r = (int)(rgb0 & 255)*(int)(rgb1 & 255);
          int g = (int)((rgb0>>8) & 255)*(int)((rgb1>>8) & 255);
          int b = (int)((rgb0>>16) & 255)*(int)((rgb1>>16) & 255);
          r *= 2;
          g *= 2;
          b *= 2;
          r /= 255;
          g /= 255;
          b /= 255;
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_PREMULTIPLIEDALPHA: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int a = (rgb1 >> 24)*alpha;
          int a2 = 255-a;
          int r = (int)(rgb0 & 255)*a2/255+(int)(rgb1 & 255)*alpha;
          int g = (int)((rgb0>>8) & 255)*a2/255+(int)((rgb1>>8) & 255)*alpha;
          int b = (int)((rgb0>>16) & 255)*a2/255+(int)((rgb1>>16) & 255)*alpha;
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          if ((rgb0>>24)>a) a = rgb0>>24;
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_MAX: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int r = maxi((int)(rgb0 & 255),(int)(rgb1 & 255));
          int g = maxi((int)((rgb0>>8) & 255),(int)((rgb1>>8) & 255));
          int b = maxi((int)((rgb0>>16) & 255),(int)((rgb1>>16) & 255));
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_MIN: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int r = mini((int)(rgb0 & 255),(int)(rgb1 & 255));
          int g = mini((int)((rgb0>>8) & 255),(int)((rgb1>>8) & 255));
          int b = mini((int)((rgb0>>16) & 255),(int)((rgb1>>16) & 255));
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_SCREEN: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int r = 255-(255-(int)(rgb0 & 255))*(255-(int)(rgb1 & 255))/255;
          int g = 255-(255-(int)((rgb0>>8) & 255))*(255-(int)((rgb1>>8) & 255))/255;
          int b = 255-(255-(int)((rgb0>>16) & 255))*(255-(int)((rgb1>>16) & 255))/255;
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_COLOR: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int rd = (rgb0&255);
          int gd = ((rgb0>>8)&255);
          int bd = ((rgb0>>16)&255);
          int rs = (rgb1&255);
          int gs = ((rgb1>>8)&255);
          int bs = ((rgb1>>16)&255);
          int lumas = rs+gs+bs;
          int lumad = rd+gd+bd;
          bool zero = lumas == 0; // source has no color/black
          int r = zero ? 0 : lumad*rs/lumas;
          int g = zero ? 0 : lumad*gs/lumas;
          int b = zero ? 0 : lumad*bs/lumas;
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_LUMINANCE: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int rd = (rgb0&255);
          int gd = ((rgb0>>8)&255);
          int bd = ((rgb0>>16)&255);
          int rs = (rgb1&255);
          int gs = ((rgb1>>8)&255);
          int bs = ((rgb1>>16)&255);
          int lumas = rs+gs+bs;
          int lumad = rd+gd+bd;
          bool zero = lumad == 0; // dest has no color/black
          int r = zero ? 0 : lumas*rd/lumad;
          int g = zero ? 0 : lumas*gd/lumad;
          int b = zero ? 0 : lumas*bd/lumad;
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_SOFTLIGHT: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int rd = (rgb0&255);
          int gd = ((rgb0>>8)&255);
          int bd = ((rgb0>>16)&255);
          int rs = (rgb1&255);
          int gs = ((rgb1>>8)&255);
          int bs = ((rgb1>>16)&255);
          int r = rs > 128 ? (255-(255-rd)*(255-(rs-128))/255) : (rd*(rs+128)/255);
          int g = gs > 128 ? (255-(255-gd)*(255-(gs-128))/255) : (gd*(gs+128)/255);
          int b = bs > 128 ? (255-(255-bd)*(255-(bs-128))/255) : (bd*(bs+128)/255);
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_HARDLIGHT: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int rd = (rgb0&255);
          int gd = ((rgb0>>8)&255);
          int bd = ((rgb0>>16)&255);
          int rs = (rgb1&255);
          int gs = ((rgb1>>8)&255);
          int bs = ((rgb1>>16)&255);
          int r = rs > 128 ? (255-(255-rd)*(255-2*(rs-128))/255) : (rd*(2*rs)/255);
          int g = gs > 128 ? (255-(255-gd)*(255-2*(gs-128))/255) : (gd*(2*gs)/255);
          int b = bs > 128 ? (255-(255-bd)*(255-2*(bs-128))/255) : (bd*(2*bs)/255);
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_LINEARLIGHT: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int rd = (rgb0&255);
          int gd = ((rgb0>>8)&255);
          int bd = ((rgb0>>16)&255);
          int rs = (rgb1&255);
          int gs = ((rgb1>>8)&255);
          int bs = ((rgb1>>16)&255);
          int r = rs > 128 ? (rd+2*(rs-128)) : (rd+2*rs-255);
          int g = gs > 128 ? (gd+2*(gs-128)) : (gd+2*gs-255);
          int b = bs > 128 ? (bd+2*(bs-128)) : (bd+2*bs-255);
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          if (r < 0) r = 0;
          if (g < 0) g = 0;
          if (b < 0) b = 0;
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_PINLIGHT: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int rd = (rgb0&255);
          int gd = ((rgb0>>8)&255);
          int bd = ((rgb0>>16)&255);
          int rs = (rgb1&255);
          int gs = ((rgb1>>8)&255);
          int bs = ((rgb1>>16)&255);
          int r = rs > 128 ? maxi(rd,2*(rs-128)) : mini(rd,2*rs);
          int g = gs > 128 ? maxi(gd,2*(gs-128)) : mini(gd,2*gs);
          int b = bs > 128 ? maxi(bd,2*(bs-128)) : mini(bd,2*bs);
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      case LAYERMODE_DIFFERENCE: {
          unsigned int rgb0 = canvas->pixels[x+y*canvas->width];
          unsigned int rgb1 = layer->pixels[lx+ly*layer->width];
          int rd = (rgb0&255);
          int gd = ((rgb0>>8)&255);
          int bd = ((rgb0>>16)&255);
          int rs = (rgb1&255);
          int gs = ((rgb1>>8)&255);
          int bs = ((rgb1>>16)&255);
          rd -= rs;
          gd -= gs;
          bd -= bs;
          int r = rd < 0 ? -rd : rd;
          int g = gd < 0 ? -gd : gd;
          int b = bd < 0 ? -bd : bd;
          int a = alpha * (int)(rgb1>>24); float alpha2 = a/255.f;
          r = (r - (int)(rgb0 & 255)) * alpha2 + (int)(rgb0 & 255);
          g = (g - (int)((rgb0>>8) & 255)) * alpha2 + (int)((rgb0>>8) & 255);
          b = (b - (int)((rgb0>>16) & 255)) * alpha2 + (int)((rgb0>>16) & 255);
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          if (a>0) a = 255; else a = (rgb0>>24);
          col = r|(g<<8)|(b<<16)|(a<<24);
        } break;
      }
      if (darken) {col = ((col>>3) & 0x001f1f1f)|(col & 0xff000000);}
      canvas->pixels[x+y*canvas->width] = col;
    }
  }
}

void Canvas::updateRegion(int screenX0, int screenY0, int screenX1, int screenY1) {
  int x0 = screenX0 - screenPosX;
  int y0 = screenY0 - screenPosY;
  int x1 = screenX1 - screenPosX;
  int y1 = screenY1 - screenPosY;
  fillCanvas(this,x0,y0,x1,y1,0x00000000,0);
  for (int i = layers.size()-1; i >= 0; i--) {
    updateCanvas(this,x0,y0,x1,y1,layers[i]);
  }
}

void Canvas::update(bool forceUpdate) {
  if (forceUpdate) for (int i = 0; i < width*height; i++) pixelStates[i] |= PIXELSTATE_UPDATED;
  updateRegion(screenPosX,screenPosY,screenPosX+width,screenPosY+height);
}

void Canvas::updateLayer(Layer *layer, bool forceUpdate) {
  int x0 = 0;
  int y0 = 0;
  int x1 = width;
  int y1 = height;
  Vector b = layer->boundingBox();
  Vector b1 = b.xy();
  Vector b2; b2.x = b.z; b2.y = b.w;
  b1 = inverse2(transform.transformation) * b1;
  b2 = inverse2(transform.transformation) * b2;
  if (b2.x < x0) return;
  if (b2.y < y0) return;
  if (b1.x > x1) return;
  if (b1.y > y1) return;
  if (x0 < b1.x) x0 = b1.x;
  if (y0 < b1.y) y0 = b1.y;
  if (x1 > b2.x) x1 = b2.x;
  if (y1 > b2.y) y1 = b2.y;
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > width) x1 = width;
  if (y1 > height) y1 = height;
  if (x0 >= x1) return;
  if (y0 >= y1) return;
  if (forceUpdate) {
    for (int y = y0; y < y1; y++) {
      for (int x = x0; x < x1; x++) {
        pixelStates[x+y*width] |= PIXELSTATE_UPDATED;
      }
    }
  }
  updateRegion(x0+screenPosX,y0+screenPosY,x1+screenPosX,y1+screenPosY);
}

void Canvas::addLayer(Layer *layer) {
  if (layer == NULL) return;
  layers.push_back(layer);
  shouldUpdateAll = true;
}

void Canvas::insertLayer(Layer *layer, int position) {
  if (layer == NULL) return;
  if (position < 0) position = 0;
  if (position > layers.size()) position = layers.size();
  layers.insert(layer,position);
  shouldUpdateAll = true;
}

void Canvas::eraseLayer(Layer *layer) {
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i] == layer) {
      layers.erase(i,1);
      i--;
      shouldUpdateAll = true;
    }
  }
}

extern double canvasScrollX;
extern double canvasScrollY;

void applyFlip(Layer *l, const Matrix &scale, const Matrix &center, const Matrix &screenToCanvas) {
  //l->transform.transformation = l->transform.transformation * center * scale * inverse2(center) * inverse2(l->transform.transformation) * l->transform.transformation;
  l->transform.transformation = l->transform.transformation * screenToCanvas * center * scale * inverse2(center) * inverse2(screenToCanvas) * inverse2(l->transform.transformation) * l->transform.transformation;
  l->transform.update();
}

void Canvas::mirrorHorizontally() {
  Matrix screenToCanvas = transform.transformation; // from screen coordinates to canvas space
  Vector ltCanvas = inverse2(screenToCanvas) * Vector(canvasX0,canvasY0,0);
  Vector rbCanvas = inverse2(screenToCanvas) * Vector(canvasX1,canvasY1,0);
  Matrix center; center.translate(Vector((ltCanvas.x+rbCanvas.x)*0.5,0,0));
  Matrix scale; scale.scale3(Vector(-1,1,1));
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    //Matrix centerInLayerSpace = screenToCanvas * center;
    applyFlip(l,scale,center,screenToCanvas);  
  }
  shouldUpdateAll = true;
}

void Canvas::mirrorVertically() {
  Matrix screenToCanvas = transform.transformation; // from screen coordinates to canvas space
  Vector ltCanvas = inverse2(screenToCanvas) * Vector(canvasX0,canvasY0,0);
  Vector rbCanvas = inverse2(screenToCanvas) * Vector(canvasX1,canvasY1,0);
  Matrix center; center.translate(Vector(0,(ltCanvas.y+rbCanvas.y)*0.5,0));
  Matrix scale; scale.scale3(Vector(1,-1,1));
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    //Matrix centerInLayerSpace = screenToCanvas * center;
    applyFlip(l,scale,center,screenToCanvas);  
  }
  shouldUpdateAll = true;
}

void Canvas::rotate90DegreesForward() {
  Matrix screenToCanvas = transform.transformation; // from screen coordinates to canvas space
  Vector ltCanvas = inverse2(screenToCanvas) * Vector(canvasX0,canvasY0,0);
  Vector rbCanvas = inverse2(screenToCanvas) * Vector(canvasX1,canvasY1,0);
  Matrix center; center.translate(Vector((ltCanvas.x+rbCanvas.x)*0.5,(ltCanvas.y+rbCanvas.y)*0.5,0));
  Matrix rotate = rotationMatrixAngleAxis(PI*-0.5,0,0,1);
  rotate = straighten(rotate);
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    Matrix centerInLayerSpace; centerInLayerSpace.translate(screenToCanvas * center * Vector(0,0,0));
    l->transform.transformation = l->transform.transformation * centerInLayerSpace * rotate * inverse2(centerInLayerSpace);
    l->transform.update();
  }

  double cWidth = canvasX1 - canvasX0;
  double cHeight = canvasY1 - canvasY0;
  double cMX = (canvasX1 + canvasX0) * 0.5;
  double cMY = (canvasY1 + canvasY0) * 0.5;
  canvasX0 = cMX - cHeight * 0.5;
  canvasX1 = cMX + cHeight * 0.5;
  canvasY0 = cMY - cWidth * 0.5;
  canvasY1 = cMY + cWidth * 0.5;

  shouldUpdateAll = true;
}

void Canvas::rotate90DegreesBackward() {
  Matrix screenToCanvas = transform.transformation; // from screen coordinates to canvas space
  Vector ltCanvas = inverse2(screenToCanvas) * Vector(canvasX0,canvasY0,0);
  Vector rbCanvas = inverse2(screenToCanvas) * Vector(canvasX1,canvasY1,0);
  Matrix center; center.translate(Vector((ltCanvas.x+rbCanvas.x)*0.5,(ltCanvas.y+rbCanvas.y)*0.5,0));
  Matrix rotate = rotationMatrixAngleAxis(PI*0.5,0,0,1);
  rotate = straighten(rotate);
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    Matrix centerInLayerSpace; centerInLayerSpace.translate(screenToCanvas * center * Vector(0,0,0));
    l->transform.transformation = l->transform.transformation * centerInLayerSpace * rotate * inverse2(centerInLayerSpace);
    l->transform.update();
  }

  double cWidth = canvasX1 - canvasX0;
  double cHeight = canvasY1 - canvasY0;
  double cMX = (canvasX1 + canvasX0) * 0.5;
  double cMY = (canvasY1 + canvasY0) * 0.5;
  canvasX0 = cMX - cHeight * 0.5;
  canvasX1 = cMX + cHeight * 0.5;
  canvasY0 = cMY - cWidth * 0.5;
  canvasY1 = cMY + cWidth * 0.5;

  shouldUpdateAll = true;
}

void Canvas::resizeCanvas(double w, double h, bool quantizeLayers, int interpolation) {
  double w2 = canvasX1 - canvasX0;
  double h2 = canvasY1 - canvasY0;
  if (w == 0) w = 1;
  if (h == 0) h = 1;
  double scaleX = w2 / w;
  double scaleY = h2 / h;
  Matrix m; m.scale3(Vector(scaleX,scaleY,1));
  for (int i = 0; i < layers.size(); i++) {
    Layer *l = layers[i];
    if (quantizeLayers) l->bakeLayer(interpolation);
    l->transform.transformation = l->transform.transformation * m;
    if (quantizeLayers) {
      int wBefore = l->width;
      int hBefore = l->height;
      l->resizeLayerContent(interpolation,wBefore/scaleX,hBefore/scaleY);
      int wAfter = l->width;
      int hAfter = l->height;
      if (wBefore == 0) wBefore = 1;
      if (hBefore == 0) hBefore = 1;
      double scaleX2 = (double)wAfter/wBefore;
      double scaleY2 = (double)hAfter/hBefore;
      Matrix m2; m2.scale3(Vector(scaleX2,scaleY2,1));
      l->transform.transformation = m2 * l->transform.transformation;
    }
    l->transform.update();
  }
  canvasX1 = canvasX0 + w;
  canvasY1 = canvasY0 + h;
  shouldUpdateAll = true;
}

void Canvas::fitCanvasToLayers() {
  if (layers.empty()) return;
  Vector boundingBox;
  for (int i3 = 0; i3 < layers.size(); i3++) {
    Vector here = layers[i3]->boundingBox();
    if (i3 == 0 || here.x < boundingBox.x) boundingBox.x = here.x;
    if (i3 == 0 || here.y < boundingBox.y) boundingBox.y = here.y;
    if (i3 == 0 || here.z > boundingBox.z) boundingBox.z = here.z;
    if (i3 == 0 || here.w > boundingBox.w) boundingBox.w = here.w;
  }
  double x0 = floor(boundingBox.x); // the inverse is too bad here
  double y0 = floor(boundingBox.y);
  double x1 = floor(boundingBox.z);
  double y1 = floor(boundingBox.w);
  if (floor(x1)-floor(x0)<1) x1 = x0 + 1;
  if (floor(y1)-floor(y0)<1) y1 = y0 + 1;
  canvasX0 = x0;
  canvasY0 = y0;
  canvasX1 = x1;
  canvasY1 = y1;
  shouldUpdateAll = true;
}

void Canvas::mirrorHorizontallyLayers() {
  bool one = false; for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active)) l->mirrorHorizontally();
  }
}

void Canvas::mirrorVerticallyLayers() {
  bool one = false; for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active)) l->mirrorVertically();
  }
}

void Canvas::rotate90DegreesForwardLayers() {
  bool one = false; for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active)) l->rotate90DegreesForward();
  }
}

void Canvas::rotate90DegreesBackwardLayers() {
  bool one = false; for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active)) l->rotate90DegreesBackward();
  }
}

void Canvas::selectAll() {
  bool one = false; 
  bool two = false; 
  for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true; if (l->state.active) two = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active) || ((!one)&&(!two))) l->selectAll();
  }
}

void Canvas::selectNone(bool all) {
  bool one = false; 
  bool two = false; 
  for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true; if (l->state.active) two = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active) || ((!one)&&(!two)) || (all)) l->selectNone();
  }
}

void Canvas::invertSelection() {
  bool one = false; 
  bool two = false; 
  for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true; if (l->state.active) two = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active) || ((!one)&&(!two))) l->invertSelection();
  }
}

bool Canvas::hasSelection() {
  for (int i = 0; i < layers.size(); i++) {if (layers[i]->hasSelection()) return true;}
  return false;
}

void Canvas::selectByAlpha(int alphaThreshold) {
  bool one = false; 
  bool two = false; 
  for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true; if (l->state.active) two = true;}
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active) || ((!one)&&(!two))) l->selectByAlpha(alphaThreshold);
  }
}

void Canvas::selectAllLayers() {
  for (int i = 0; i < layers.size(); i++) {layers[i]->state.selected = true;layers[i]->state.active = false;}
  shouldUpdateAll = true;
}

void Canvas::showAllLayers(bool show) {
  for (int i = 0; i < layers.size(); i++) {layers[i]->state.visible = show;}
  shouldUpdateAll = true;
}

void Canvas::moveLayersUp() {
  if ((!layers.empty()) && (layers[0]->state.selected || layers[0]->state.active)) return;
  for (int i = 1; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Layer *t = layers[i-1];
      layers[i-1] = layers[i];
      layers[i] = t;
    }
  }
  shouldUpdateAll = true;
}

void Canvas::moveLayersDown() {
  if ((!layers.empty()) && (layers.back()->state.selected || layers.back()->state.active)) return;
  for (int i = layers.size()-2; i >= 0; i--) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Layer *t = layers[i+1];
      layers[i+1] = layers[i];
      layers[i] = t;
    }
  }
  shouldUpdateAll = true;
}

void Canvas::compactLayers() {
  int firstIndex = -1;
  for (int i2 = 0; i2 < layers.size(); i2++) {
    if (layers[i2]->state.selected||layers[i2]->state.active) {firstIndex = i2; break;}
  }
  for (int i = firstIndex+1; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      firstIndex++; 
      Layer *t = layers[i];
      layers.erase(i,1);
      layers.insert(t,firstIndex);

    }
  }
  shouldUpdateAll = true;
}

void Canvas::fillSelectionWithColor(unsigned int color) {
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->hasSelection()) layers[i]->fillSelectionWithColor(color);
  }
}

void Canvas::expandSelection(int pixels) {
  if (pixels == 0) return;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->hasSelection()) layers[i]->expandSelection(pixels);
  }
}

Layer *layerFromCanvas(Canvas *canvas) {
  Layer *ret = NULL;
  ret = new Layer();
  int w = (int)canvas->width;
  int h = (int)canvas->height;
  if (w < 0) w = 1;
  if (h < 0) h = 1;
  ret->init(w,h);
  for (int i = 0; i < ret->width*ret->height; i++) ret->pixels[i] = canvas->pixels[i];
  ret->transform.transformation = inverse2(canvas->transform.transformation);
  ret->transform.update();
  return ret;
}

Layer *Canvas::getSelectionAsLayer(bool pixelSelection,bool selectAllLayers, bool keepCanvasSize) {
  Layer *l = NULL;
  bool one = false; for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true;}
  Array<Layer*> selection;
  for (int i2 = 0; i2 < layers.size(); i2++) {
    Layer *l = layers[i2];
    if ((one && l->state.selected) || ((!one)&&l->state.active) || (selectAllLayers)) if (l->hasSelection()||selectAllLayers||(!pixelSelection)) selection.push_back(l);
  }
  if (selection.empty()) return l;
  Vector boundingBox;
  for (int i3 = 0; i3 < selection.size(); i3++) {
    Vector here = selection[i3]->boundingBox();
    if (i3 == 0 || here.x < boundingBox.x) boundingBox.x = here.x;
    if (i3 == 0 || here.y < boundingBox.y) boundingBox.y = here.y;
    if (i3 == 0 || here.z > boundingBox.z) boundingBox.z = here.z;
    if (i3 == 0 || here.w > boundingBox.w) boundingBox.w = here.w;
  }
  int x0 = floor(boundingBox.x);
  int y0 = floor(boundingBox.y);
  int x1 = floor(boundingBox.z);
  int y1 = floor(boundingBox.w);
  int w = x1-x0;
  int h = y1-y0;
  Canvas *copyCanvas = new Canvas();
  if (!keepCanvasSize) {
    if (w<1||h<1) return l;
    copyCanvas->init(w,h);
    copyCanvas->canvasX0 = 0;
    copyCanvas->canvasY0 = 0;
    copyCanvas->canvasX1 = x1-x0;
    copyCanvas->canvasY1 = y1-y0;
    copyCanvas->transform.fresh();
    copyCanvas->transform.translate(x0,y0);
  } else {
    w = canvasX1-canvasX0;
    h = canvasY1-canvasY0;
    if (w<1||h<1) return l;
    copyCanvas->init(w,h);
    copyCanvas->canvasX0 = 0;
    copyCanvas->canvasY0 = 0;
    copyCanvas->canvasX1 = w;
    copyCanvas->canvasY1 = h;
    copyCanvas->transform.fresh();
    copyCanvas->transform.translate(canvasX0,canvasY0);
  }
  copyCanvas->transform.update();
  copyCanvas->layers = selection;
  hilightSelection = false;
  justSelection = pixelSelection;
  copyCanvas->update(true);
  justSelection = false;
  copyCanvas->layers.clear();
  l = layerFromCanvas(copyCanvas);
  sprintf(l->name,"paste");
  delete copyCanvas;
  return l;
}


void Canvas::duplicateLayers() {
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected || layers[i]->state.active) {
      layers[i]->state.selected = false;
      Layer *l = layers[i]->clone();
      sprintf(l->name,"duplicate");
      insertLayer(l,i);
      i++;
    }
  }
  shouldUpdateAll = true;
}

void Canvas::copy() {
  if (copyLayer != NULL) {delete copyLayer; copyLayer = NULL;}
  copyLayer = getSelectionAsLayer(true,false,false);
  selectNone(true);
}

void Canvas::paste() {
  if (copyLayer == NULL) return;
  bool one = false; for (int i = 0; i < layers.size(); i++) {Layer *l = layers[i]; if (l->state.selected) one = true;}
  int layerPos = 0;
  for (int i2 = 0; i2 < layers.size(); i2++) {Layer *l = layers[i2]; if ((l->state.selected && one)||(l->state.active && (!one))) {layerPos = i2; break;}}
  insertLayer(copyLayer,layerPos);
  setActiveLayer(layerPos);
  copyLayer = NULL;
}

Layer *Canvas::getActiveLayer() {
  Layer *ret = NULL;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.active) {
      ret = layers[i];
      break;
    }
  }
  return ret;
}

void Canvas::setActiveLayer(int layer) {
  for (int i = 0; i < currentCanvas->layers.size(); i++) {
    Layer *l = currentCanvas->layers[i];
    l->state.active = i == layer ? true : false;
  }
}

void Canvas::noSelectedLayers() {
  for (int i = 0; i < currentCanvas->layers.size(); i++) {
    Layer *l = currentCanvas->layers[i];
    l->state.selected = false;
  }
}

Vector Canvas::boundingBox() {
  Vector ret;
  bool first = true;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Vector k = layers[i]->boundingBox();
      if (first || k.x < ret.x) ret.x = k.x;
      if (first || k.y < ret.y) ret.y = k.y;
      if (first || k.z > ret.z) ret.z = k.z;
      if (first || k.w > ret.w) ret.w = k.w;
      first = false;
    }
  }
  return ret;
}

void Canvas::setBoundingPos(double x, double y) {
  Vector k = boundingBox();
  if (k.z-k.x<=0) return;
  if (k.w-k.y<=0) return;
  double updateX = x - k.x;
  double updateY = y - k.y;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Layer *l = layers[i]; Vector b2 = l->boundingBox(); l->setLayerPos(b2.x+updateX, b2.y+updateY);
    }
  }
}

void Canvas::setBoundingSize(double width, double height) {
  Vector k = boundingBox();
  if (k.z-k.x<=0) return;
  if (k.w-k.y<=0) return;
  double updateX = width / (k.z-k.x);
  double updateY = height / (k.w-k.y);
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Layer *l = layers[i]; Vector b2 = l->boundingBox(); 
      l->setLayerPos((b2.x-k.x)*updateX+k.x,(b2.y-k.y)*updateY+k.y);
      l->setLayerSize((b2.z-b2.x)*updateX, (b2.w-b2.y)*updateY);
    }
  }
}

double Canvas::getBoundingAngleDegrees() {
  double angle = 0;
  int k = 0;
  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Layer *l = layers[i]; angle += l->getAngleDegrees(); k++;
    }
  }
  if (k > 0) angle = angle / k;
  return angle;
}

void Canvas::setBoundingAngleDegrees(double degrees) {
  double angleDelta = degrees - getBoundingAngleDegrees();
  angleDelta += 360;
  if (angleDelta < 0) return;
  angleDelta = fmod(angleDelta,360);

  Vector center;
  for (int i2 = 0; i2 < layers.size(); i2++) {
    if (layers[i2]->state.selected||layers[i2]->state.active) {
      Layer *l = layers[i2];
      Vector c = l->boundingBox();
      center += Vector((c.z-c.x)*0.5+c.x,(c.w-c.y)*0.5+c.y,1);
    }
  }
  if (center.z != 0) center /= center.z;

  Matrix modify; modify = rotationMatrixAngleAxis(-angleDelta*PI/180.0,0,0,1);

  for (int i = 0; i < layers.size(); i++) {
    if (layers[i]->state.selected||layers[i]->state.active) {
      Layer *l = layers[i];
      Matrix t; t.translate(Vector(center.x,center.y,0));
      l->transform.transformation = l->transform.transformation * t * modify * inverse2(t);
      l->transform.update();
    }
  }
}

void repaintLayer(Layer *layer, bool fullUpdate) {
  layer->repaintState = fullUpdate ? REPAINTSTATE_FULLUPDATE : REPAINTSTATE_PIXELUPDATE;
  shouldUpdate = true;
}

void blitCanvas() {
  int count = 0;
  Layer *repaintLayer = NULL;
  int repaintLayerRepaintState = REPAINTSTATE_NONE;
  for (int i = 0; i < currentCanvas->layers.size(); i++) {
    if (currentCanvas->layers[i]->repaintState != REPAINTSTATE_NONE) {
      repaintLayer = currentCanvas->layers[i];
      repaintLayerRepaintState = repaintLayer->repaintState;
      repaintLayer->repaintState = REPAINTSTATE_NONE;
      count++;
    }
  }
  if (count>1) repaintLayer = NULL;

  if (shouldUpdateAll||shouldUpdate) {
    if (shouldUpdateAll) repaintLayer = NULL;
    if (repaintLayer != NULL) {
      currentCanvas->updateLayer(repaintLayer,repaintLayerRepaintState == REPAINTSTATE_FULLUPDATE);
    } else {
      currentCanvas->update(shouldUpdateAll);
    }
    shouldUpdateAll = false;
    shouldUpdate = false;
  }
  int time = (int)(glSeconds()*8.0);
  Matrix m = currentCanvas->transform.transformation;
  int gridValX = -currentCanvas->gridOffsetX+currentCanvas->gridStepX * 10000;
  int gridValY = -currentCanvas->gridOffsetY+currentCanvas->gridStepY * 10000;
  for (int y = 0; y < currentCanvas->height; y++) {
    int dy = y + currentCanvas->screenPosY;
    if ((unsigned int)dy >= glFrameBufferHeight) continue;
    for (int x = 0; x < currentCanvas->width; x++) {
      int dx = x + currentCanvas->screenPosX;
      if ((unsigned int)dx >= glFrameBufferWidth) continue;
      unsigned int col = currentCanvas->pixels[x+y*currentCanvas->width];
      double lx3 = x*m.m[0+0*4]+y*m.m[0+1*4]+m.m[0+3*4];
      double ly3 = x*m.m[1+0*4]+y*m.m[1+1*4]+m.m[1+3*4];
      int lx2 = (int)(lx3+4*100000)-4*100000;
      int ly2 = (int)(ly3+4*100000)-4*100000;
      if ((col & 0xff000000) != 0xff000000) {
        int lx = lx2 + 4*100000;
        int ly = ly2 + 4*100000;
        lx /= 4;
        ly /= 4;
        int k = (lx + ly) & 1;
        if (k) {
          int r = col & 0xff;
          int g = (col>>8) & 0xff;
          int b = (col>>16) & 0xff;
          int a = (col>>24) & 0xff;
          int aadd = 64*(255-a);
          r = (r*a+aadd)/255;
          //g = (g*a+aadd)/255;
          //b = (b*a+aadd)/255;
          col = r|(g<<8)|(b<<16)|0xff000000;
        }
      }
      if (lx3 < currentCanvas->canvasX0 || ly3 < currentCanvas->canvasY0 || lx3 >= currentCanvas->canvasX1 || ly3 >= currentCanvas->canvasY1) {
        col >>= 2;
        col &= 0x3f3f3f3f;
        col |= 0x40404040;
      }
      if (currentCanvas->gridOn) {
        int lx2x = (int)((x+1)*m.m[0+0*4]+y*m.m[0+1*4]+m.m[0+3*4]+4*100000)-4*100000;
        int ly2y = (int)(x*m.m[1+0*4]+(y+1)*m.m[1+1*4]+m.m[1+3*4]+4*100000)-4*100000;
        lx2x =  (lx2x + gridValX) % currentCanvas->gridStepX;
        ly2y =  (ly2y + gridValY) % currentCanvas->gridStepY;
        int lx2a = (lx2 + gridValX) % currentCanvas->gridStepX;
        int ly2a =  (ly2 + gridValY) % currentCanvas->gridStepY;
        if (lx2a > lx2x || ly2a > ly2y) {
          int g = (col>>8) & 255;
          col = g > 128 ? 0xff000000 : 0xff00ff00;
        }
      }
      unsigned char state = currentCanvas->pixelStates[x+y*currentCanvas->width];
      if (state & PIXELSTATE_SELECTED) {
        if (x != 0 && x != currentCanvas->width-1 && y != 0 && y != currentCanvas->height-1) {
          state &= currentCanvas->pixelStates[x+1+y*currentCanvas->width];
          state &= currentCanvas->pixelStates[x-1+y*currentCanvas->width];
          state &= currentCanvas->pixelStates[x+(y+1)*currentCanvas->width];
          state &= currentCanvas->pixelStates[x+(y-1)*currentCanvas->width];
          if (!(state & PIXELSTATE_SELECTED)) {
            col = (((x+y+time)/4) & 1) ? 0xffffffff : 0xff0000ff;
          }
        }
      }
      glFrameBuffer[dx+dy*glFrameBufferWidth] = col;
    }
  }
}

void canvasDefault(Canvas *canvas) {
  canvas->init(glFrameBufferWidth-sidebar_width-toolbar_width-toolview_width,glFrameBufferHeight-menuBar_height);
  canvas->screenPosX = toolbar_width+toolview_width;
  canvas->screenPosY = menuBar_height;
}

void addPicture(String fileName) {
  currentCanvas = new Canvas();
  canvas.push_back(currentCanvas);
  canvasDefault(currentCanvas);
  currentCanvas->addLayer(loadImage("",fileName));
  currentCanvas->setActiveLayer(0);
  currentCanvas->fitCanvasToLayers();
  shouldUpdateAll = true;
}

void insertPicture(String fileName) {
  Layer *l = loadImage("",fileName);
  if (l == NULL) return;
  int pos = 0;
  for (int i = 0; i < currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.active) {pos = i; break;}
  currentCanvas->layers.insert(l,pos);
  currentCanvas->setActiveLayer(pos);
  shouldUpdateAll = true;
}

void addTempPicture() {
  currentCanvas = new Canvas();
  sprintf(currentCanvas->pictureName,"empty");
  canvas.push_back(currentCanvas);
  canvasDefault(currentCanvas);
  shouldUpdateAll = true;
}

void newPicture(int w, int h,const String &pictureName) {
  currentCanvas = new Canvas();
  sprintf(currentCanvas->pictureName,pictureName.c_str());
  canvas.push_back(currentCanvas);
  canvasDefault(currentCanvas);
  Layer *l = new Layer();
  l->init(w,h);
  l->selectAll();
  l->fillSelectionWithColor(0xff000000);
  l->selectNone();
  sprintf(l->name,"unnamed");
  currentCanvas->addLayer(l);
  currentCanvas->fitCanvasToLayers();
  currentCanvas->setActiveLayer(0);
  shouldUpdateAll = true;
}
