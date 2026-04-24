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
#ifndef __LAYERS_HPP__
#define __LAYERS_HPP__

#include "STL/ARRAY.HPP"
#include "STL/PAIR.HPP"
#include "STL/MATRIX.HPP"
#include "STL/VECTOR.HPP"

void initLayers();
void freeLayers();

enum LAYERMODES {
  LAYERMODE_UNDEFINED = 0,
  LAYERMODE_NORMAL = 1,
  LAYERMODE_ADD = 2,
  LAYERMODE_SUB = 3,
  LAYERMODE_MUL = 4,
  LAYERMODE_MUL2X = 5,
  LAYERMODE_PREMULTIPLIEDALPHA = 6,
  LAYERMODE_MAX = 7,
  LAYERMODE_MIN = 8,
  LAYERMODE_SCREEN = 9,
  LAYERMODE_COLOR = 10,
  LAYERMODE_LUMINANCE = 11,
  LAYERMODE_SOFTLIGHT = 12,
  LAYERMODE_HARDLIGHT = 13,
  LAYERMODE_LINEARLIGHT = 14,
  LAYERMODE_PINLIGHT = 15,
  LAYERMODE_DIFFERENCE = 16,
  LAYERMODE_COUNT = 17
};

extern const char *layerModeNames[LAYERMODE_COUNT];

class LayerState {
public:

  bool active;
  bool selected;
  bool visible;
  float alpha;
  bool imGuiActive;

  LayerState() {
    active = false;
    selected = false;
    visible = true;
    alpha = 1;
    imGuiActive = false;
  }

  void clear();
};

class TransformationState {
public:
  Matrix transformation;
  Matrix inverseTransformation;

  void transformForward(double x, double y, double &rx, double &ry);
  void transformBackward(double x, double y, double &rx, double &ry);

  void clear();
  void update();

  void translate(double x, double y);
  void scale(double x, double y);
  void scaleAround(double scaleX, double scaleY, double centerX, double centerY);

  void fresh();
  void zoom(double zoom);
  void aspect(double aspect);
  void pixelSize(double x, double y);
};

#define LAYERNAMEMAX (16)

enum PIXELSTATES {
  PIXELSTATE_NONE = 0,
  PIXELSTATE_UPDATED = 1,
  PIXELSTATE_SELECTED = 2,
  PIXELSTATE_TEMP = 4,
  PIXELSTATE_OUTLINE = 8
};

enum REPAINTSTATES {
  REPAINTSTATE_NONE = 0,
  REPAINTSTATE_FULLUPDATE = 1,
  REPAINTSTATE_PIXELUPDATE = 2
};

enum INTERPOLATION {
  INTERPOLATION_NONE = 0,
  INTERPOLATION_INTERAREA = 1,
  INTERPOLATION_LINEAR = 2,
  INTERPOLATION_COUNT = 3
};
extern const char *interpolationNames[INTERPOLATION_COUNT];

class ColorTransform {
public:
  ColorTransform() {
    reset();
  }
  float brightness;
  float contrast;
  float gamma;
  bool gammaInvert;
  float saturation;
  float addRed;
  float addGreen;
  float addBlue;
  float addAlpha;
  float scaleRed;
  float scaleGreen;
  float scaleBlue;
  float scaleAlpha;
  float hue;
  unsigned int r[256];
  unsigned int g[256];
  unsigned int b[256];
  unsigned int a[256];
  unsigned int grey[256];
  bool shouldUpdate;
  void reset();
  void update();
  unsigned int applyColorTransform(unsigned int pixel);
};

class Layer {
public:

  int mode;
  int width;
  int height;
  unsigned int *pixels;
  unsigned char *pixelStates;
  LayerState state;
  TransformationState transform;
  ColorTransform colorTransform;
  char name[LAYERNAMEMAX];

  int repaintState;                    
  bool justSelection;
  bool hasOutline;
  bool useColorTransform;

  Layer() {
    mode = LAYERMODE_NORMAL;
    width = 0;
    height = 0;
    pixels = NULL;
    pixelStates = NULL;
    repaintState = REPAINTSTATE_NONE;
    justSelection = false;
    hasOutline = false;
    useColorTransform = false;
    static int layerCounter = 0; layerCounter++;
    sprintf(name,"Layer %d", layerCounter);
  }

  ~Layer() {
    if (pixels != NULL) {delete[] pixels; pixels = NULL;}
    if (pixelStates != NULL) {delete[] pixelStates; pixelStates = NULL;}
  }

  unsigned int getPixel(int x, int y);
  void setPixel(int x, int y, unsigned int rgba);
  void pixelLine(int x0, int y0, int x1, int y1, unsigned int color);
  void pixelLineOr(int x0, int y0, int x1, int y1, unsigned int or);
  void pixelLineEor(int x0, int y0, int x1, int y1, unsigned int eor);
  void pixelXLineEor(int x0, int y0, int x1, int y1, unsigned int eor);

  void init(int width, int height);
  void clear();
  void updatePixelStates(unsigned char and, unsigned char or, unsigned char eor);

  void mirrorHorizontally();
  void mirrorVertically();
  void rotate90DegreesForward();
  void rotate90DegreesBackward();
  void rotateByDegrees(double degrees);

  void selectAll();
  void selectNone();
  void invertSelection();
  void fillSelectionWithColor(unsigned int color);
  void fillSelectionWithAlpha(unsigned char alpha);
  void invertSelectionColor();
  void invertSelectionAlpha();
  void expandSelection(int pixels);
  void selectByAlpha(int alphaThreshold);
  void selectionAsOutline();

  bool hasSelection();
  Vector boundingBox();
  Vector selectionBounding();
  Vector boundingByAlpha(double alphaThreshold);
  void quantizeBoundingBox();

  void resizeLayerContent(int interpolation, int newWidth, int newHeight);
  void bakeLayer(int interpolation);
  Layer *clone();

  void setLayerPos(double x, double y);
  void setLayerSize(double width, double height);
  double getAngleDegrees();
  void setAngleDegrees(double degrees);
  void snapToGrid(class Canvas *canvas);

  void enlargenLayerBy(int left, int top, int right, int bottom);
  void cropByAlpha(double alphaThreshold);
  void antialiasSelected();
  void colorFromAlpha();
  void applyAlphaMask(Layer *source);
  void quantize(const Array<unsigned int> &colors);

  void copyColorTransform();
  void pasteColorTransform();
  void bakeColorTransform(bool onlyInSelection);

  void addSelectionFromLayer(Layer *source);
};

class CanvasControl {
public:

  double zoom;
  double pixelSizeX;
  double pixelSizeY;
  double scrollX;
  double scrollY;

  CanvasControl() {
    zoom = 100.0;
    pixelSizeX = 1;
    pixelSizeY = 1;
    scrollX = 0;
    scrollY = 0;
  }

  void setCanvasTransform(class Canvas *canvas);
  void moveByPixels(class Canvas *canvas,double x, double y);
  void zoomTo(Canvas *canvas, double screenPixelCenterX, double screenPixelCenterY,double zoom);
  void zoomBy(Canvas *canvas, double screenPixelCenterX, double screenPixelCenterY,double step);
};

#define PICTURENAMEMAX 16

extern Layer *copyLayer;

class Canvas {
public:

  int screenPosX;
  int screenPosY;
  int width;
  int height;
  unsigned int *pixels;
  unsigned char *pixelStates;
  bool gridOn;
  int gridStepX;
  int gridStepY;
  int gridOffsetX;
  int gridOffsetY;

  String fileName;
  char pictureName[PICTURENAMEMAX];

  int canvasX0, canvasY0, canvasX1, canvasY1;

  CanvasControl control;
  TransformationState transform;
  Array<Layer*> layers;

  bool hilightSelection;
  bool justSingle;

  Canvas() {
    width = 0;
    height = 0;
    screenPosX = 0;
    screenPosY = 0;
    canvasX0 = 0;
    canvasY0 = 0;
    canvasX1 = 0;
    canvasY1 = 0;
    pixels = NULL;
    pixelStates = NULL;
    static int counter = 0; counter++;
    sprintf(pictureName,"Picture %d",counter);
    gridOn = false;
    gridStepX = 8;
    gridStepY = 8;
    gridOffsetX = 0;
    gridOffsetY = 0;
    hilightSelection = false;
    justSingle = false;
  }

  ~Canvas() {
    if (pixels != NULL) {delete[] pixels; pixels = NULL;}
    if (pixelStates != NULL) {delete[] pixelStates; pixelStates = NULL;}
    for (int i = 0; i < layers.size(); i++) {
      if (layers[i] != NULL) {delete layers[i]; layers[i] = NULL;}
    }
    layers.clear();
  }

  void init(int width, int height);
  void clearUpdateFlag();
  void updatePixelStates(unsigned char and, unsigned char or, unsigned char eor);

  void getLayerPos(Layer *layer, double screenX, double screenY, double &layerX, double &layerY);
  void getLayerPosFromCanvasPos(Layer *layer, double canvasX, double canvasY, double &layerX, double &layerY);
  void getCanvasPosFromLayerPos(Layer *layer, double layerX, double layerY, double &canvasX, double &canvasY);
  void getCanvasPos(double screenX, double screenY, double &canvasX, double &canvasY);
  void getScreenPos(double canvasX, double canvasY, double &screeenX, double &screenY);
  void getScreenPos(Layer *layer, double layerX, double layerY, double &screenX, double &screenY);

  unsigned int getCanvasColor(double screenX, double screenY);
  unsigned int getLayerColor(double screenX, double screenY, bool useColorTransform);

  void updateRegion(int screenX0, int screenY0, int screenX1, int screenY1);
  void update(bool forceUpdate);
  void updateLayer(Layer *layer,bool forceUpdate);

  void addLayer(Layer *layer);
  void insertLayer(Layer *layer, int position);
  void eraseLayer(Layer *layer);

  void mirrorHorizontally();
  void mirrorVertically();
  void rotate90DegreesForward();
  void rotate90DegreesBackward();
  void resizeCanvas(double w, double h, bool quantizeLayers, int interpolation);
  void fitCanvasToLayers();
  void fitCanvasToSelection();
  void cropCanvasByAlpha(double alpha);

  void mirrorHorizontallyLayers();
  void mirrorVerticallyLayers();
  void rotate90DegreesForwardLayers();
  void rotate90DegreesBackwardLayers();
  Layer *getMergedLayer(const Array<Layer*> &layers,bool pixelSelection, bool keepCanvasSize);
  void mergeDownLayers();
  void insertMergeOfLayers();
  void insertAlphaMasks();
  void applyAlphaMaskToBelow();
  void quantizeLayers();

  void selectAll();
  void selectNone(bool all);
  void invertSelection();
  bool hasSelection();
  void copyActiveSelectionToLayer(int layerIndexSource,int layerIndexDest);
  void selectByAlpha(int alphaThreshold);
  void selectionAsOutline();
  void selectAllLayers();
  void showAllLayers(bool show);

  void moveLayersUp();
  void moveLayersDown();
  void compactLayers();

  void fillSelectionWithColor(unsigned int color);
  void fillSelectionWithAlpha(unsigned char alpha);
  void invertSelectionColor();
  void invertSelectionAlpha();
  void expandSelection(int pixels);
  void antialiasSelected();
  
  Layer *getSelectionAsLayer(bool pixelSelection, bool selectAllLayers, bool keepCanvasSize);
  void duplicateLayers();
  void insertLayerLikeThisLayer();
  void copy();
  void paste();

  int getActiveLayerIndex();
  Layer *getActiveLayer();
  void setActiveLayer(int layer);
  void noSelectedLayers();

  Vector boundingBox();
  Vector getLayerPixelBounding(Layer *l, double canvasX0, double canvasY0, double canvasX1, double canvasY1);
  void setBoundingPos(double x, double y);
  void setBoundingSize(double width, double height);
  double getBoundingAngleDegrees();
  void setBoundingAngleDegrees(double degrees);
  void snapToGrid();

  void cropLayersByAlpha(double alpha);
  bool isOnlyActiveLayerActive();
  void getLayerSelectionFromLayers(Layer *l);
  void clearAllOtherLayerSelections(Layer *l);

};

extern Canvas *currentCanvas;
extern Array<Canvas*> canvas;
extern bool shouldUpdateAll;
extern bool shouldUpdate;
extern Layer *selectionPlane;
extern bool showSelectionPlane;
extern bool showSelectionPlaneBounding;
extern bool clearOutlineAfterNextPaint;

void repaintLayer(Layer *layer, bool fullUpdate);
void blitCanvas();

void addPicture(String fileName);
void insertPicture(String fileName);
void addTempPicture();
void newPicture(int w, int h,const String &pictureName);

#endif //__LAYERS_HPP__
