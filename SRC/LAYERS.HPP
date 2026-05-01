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

#include "CONFIG.HPP"
#include "STL/ARRAY.HPP"
#include "STL/PAIR.HPP"
#include "STL/MATRIX.HPP"
#include "STL/VECTOR.HPP"

extern unsigned int pixelsUpdateCounter;
extern unsigned int pixelStatesUpdateCounter;
void updatedPixels(class Layer *l);
void updatedPixelStates(class Layer *l);
void reverseLastUpdatedPixels(class Layer *l);
void reverseLastUpdatedPixelStates(class Layer *l);
void signSave(class Layer *l);

void initLayers();
void freeLayers();

enum LAYERMODES {
  LAYERMODE_UNDEFINED = 0,
  LAYERMODE_NORMAL = 1,
  LAYERMODE_PREMULTIPLIEDALPHA = 2,
  LAYERMODE_SCREEN = 3,
  LAYERMODE_ADD = 4,
  LAYERMODE_SUB = 5,
  LAYERMODE_MUL = 6,
  LAYERMODE_MUL2X = 7,
  LAYERMODE_MAX = 8,
  LAYERMODE_MIN = 9,
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
#define S_LAYERSTATE(__name__,__o__)\
  S_OBJECT_OPEN(__name__);\
  S_BOOL("active",(__o__)->active);\
  S_BOOL("selected",(__o__)->selected);\
  S_BOOL("visible",(__o__)->visible);\
  S_FLOAT("alpha",(__o__)->alpha);\
  S_OBJECT_CLOSE();

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
#define S_TRANSFORMATIONSTATE(__name__,__o__)\
  S_OBJECT_OPEN(__name__);\
  S_MATRIX("transformation",(__o__)->transformation);\
  (__o__)->inverseTransformation = inverse2((__o__)->transformation);\
  S_OBJECT_CLOSE();


#define LAYERNAMEMAX (16)

enum PIXELSTATES {
  PIXELSTATE_NONE = 0,
  PIXELSTATE_UPDATED = 1,
  PIXELSTATE_SELECTED = 2,
  PIXELSTATE_TEMP = 4,
  PIXELSTATE_OUTLINE = 8,
  PIXELSTATE_GRID = 16,
  PIXELSTATE_CANVAS_UPDATED = 32,
  PIXELSTATE_CANVAS_GRID = 64
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
    useColorTransform = false;
    useQuantization = false;
    usePosterize = false;
    reset();
  }
  float hue;
  float saturation;
  float lightness;
  float brightness;
  float contrast;
  float gamma;
  bool gammaInvert;
  float pictureGamma;
  bool pictureGammaInvert;
  float addRed;
  float addGreen;
  float addBlue;
  float addAlpha;
  float scaleRed;
  float scaleGreen;
  float scaleBlue;
  float scaleAlpha;
  bool invertR;
  bool invertG;
  bool invertB;
  bool invertA;
  float posterizeRed;
  float posterizeGreen;
  float posterizeBlue;
  float posterizeAlpha;
  float ditherStrength;
  float middleRedAdjust;
  float middleGreenAdjust;
  float middleBlueAdjust;
  bool perceptional;
  bool useColorTransform;
  bool useQuantization;
  bool usePosterize;
  float *processingGet();
  void fromProcessing(float *processing);
  unsigned int r[256];
  unsigned int g[256];
  unsigned int b[256];
  unsigned int a[256];
  bool shouldUpdate;
  void reset();
  void update();
  unsigned int applyColorTransform(unsigned int pixel);
  unsigned int applyPosterize(unsigned int pixel);
  int byteCount();
};
#define S_COLORTRANSFORM(__name__,__o__)\
  S_OBJECT_OPEN(__name__);\
  S_FLOAT("hue",(__o__)->hue);\
  S_FLOAT("saturation",(__o__)->saturation);\
  S_FLOAT("lightness",(__o__)->lightness);\
  S_FLOAT("brightness",(__o__)->brightness);\
  S_FLOAT("contrast",(__o__)->contrast);\
  S_FLOAT("gamma",(__o__)->gamma);\
  S_BOOL("gammaInvert",(__o__)->gammaInvert);\
  S_FLOAT("pictureGamma",(__o__)->pictureGamma);\
  S_BOOL("pictureGammaInvert",(__o__)->pictureGammaInvert);\
  S_FLOAT("addRed",(__o__)->addRed);\
  S_FLOAT("addGreen",(__o__)->addGreen);\
  S_FLOAT("addBlue",(__o__)->addBlue);\
  S_FLOAT("addAlpha",(__o__)->addAlpha);\
  S_FLOAT("scaleRed",(__o__)->scaleRed);\
  S_FLOAT("scaleGreen",(__o__)->scaleGreen);\
  S_FLOAT("scaleBlue",(__o__)->scaleBlue);\
  S_FLOAT("scaleAlpha",(__o__)->scaleAlpha);\
  S_BOOL("invertR",(__o__)->invertR);\
  S_BOOL("invertG",(__o__)->invertG);\
  S_BOOL("invertB",(__o__)->invertB);\
  S_BOOL("invertA",(__o__)->invertA);\
  S_FLOAT("posterizeRed",(__o__)->posterizeRed);\
  S_FLOAT("posterizeGreen",(__o__)->posterizeGreen);\
  S_FLOAT("posterizeBlue",(__o__)->posterizeBlue);\
  S_FLOAT("posterizeAlpha",(__o__)->posterizeAlpha);\
  S_FLOAT("ditherStrength",(__o__)->ditherStrength);\
  S_FLOAT("middleRedAdjust",(__o__)->middleRedAdjust);\
  S_FLOAT("middleGreenAdjust",(__o__)->middleGreenAdjust);\
  S_FLOAT("middleBlueAdjust",(__o__)->middleBlueAdjust);\
  S_BOOL("perceptional",(__o__)->perceptional);\
  S_BOOL("useColorTransform",(__o__)->useColorTransform);\
  S_BOOL("usePosterize",(__o__)->usePosterize);\
  if (!save) (__o__)->shouldUpdate = true;\
  S_OBJECT_CLOSE();

unsigned int quantizeFast(unsigned int col, float ditherStrength, int x, int y);

class LayerUndoData {
public:
  LayerUndoData() {
    rleDataPixels_forward = NULL;
    rleDataPixelStates_forward = NULL;
    rleDataPixels_backward = NULL;
    rleDataPixelStates_backward = NULL;
    width_forward = 0;
    height_forward = 0;
    width_backward = 0;
    height_backward = 0;
    completeByteCount = 0;
    updatePixelsCount = 0;
    updatePixelStatesCount = 0;
    updatePixelsCountOfLastUndo = -1;
    updatePixelStatesCountOfLastUndo = -1;
    updatePixelsCountOfLastSave = -1;
    updatePixelStatesCountOfLastSave = -1;
    undoLayer = NULL;
  }
  ~LayerUndoData() {
    freeIt();
  }

  void freeIt();
  void signUndo();
  void signSave();
  void signWith(LayerUndoData *source);
  int byteCount();

  int width_forward,height_forward;
  int width_backward,height_backward;
  unsigned char *rleDataPixels_forward;
  unsigned char *rleDataPixelStates_forward;
  unsigned char *rleDataPixels_backward;
  unsigned char *rleDataPixelStates_backward;
  unsigned int completeByteCount;
  unsigned int updatePixelsCount;
  unsigned int updatePixelStatesCount;
  int updatePixelsCountOfLastUndo;
  int updatePixelStatesCountOfLastUndo;
  int updatePixelsCountOfLastSave;
  int updatePixelStatesCountOfLastSave;
  class Layer *undoLayer;
};

class LayerAdditionalToolInfo {
public:
  LayerAdditionalToolInfo() {
    tilingOffsetX = 0;
    tilingOffsetY = 0;
    tilingWidth = 8;
    tilingHeight = 8;

    tilingAnimFPS = 30;
    tilingAnimFirstFrame = 0;
    tilingAnimLastFrame = 0;
    tilingAnimPlaying = false;
    tilingAnimReverse = false;
    tilingAnimTime = 0;
    tilingAnimFrame = 0;

    showTilingGrid = true;
    showTilingGridIndices = true;
  }
  int tilingOffsetX;
  int tilingOffsetY;
  int tilingWidth;
  int tilingHeight;

  float tilingAnimFPS;
  int tilingAnimFirstFrame;
  int tilingAnimLastFrame;
  int tilingAnimFrame;
  bool tilingAnimPlaying;
  bool tilingAnimReverse;
  float tilingAnimTime;

  bool showTilingGrid;
  bool showTilingGridIndices;
};
#define S_LAYERADDITIONALTOOLINFO(__name__,__o__)\
  S_OBJECT_OPEN(__name__);\
  S_INT("tilingOffsetX",(__o__)->tilingOffsetX);\
  S_INT("tilingOffsetY",(__o__)->tilingOffsetY);\
  S_INT("tilingWidth",(__o__)->tilingWidth);\
  S_INT("tilingHeight",(__o__)->tilingHeight);\
  S_FLOAT("tilingAnimFPS",(__o__)->tilingAnimFPS);\
  S_INT("tilingAnimFirstFrame",(__o__)->tilingAnimFirstFrame);\
  S_INT("tilingAnimLastFrame",(__o__)->tilingAnimLastFrame);\
  S_BOOL("tilingAnimPlaying",(__o__)->tilingAnimPlaying);\
  S_BOOL("tilingAnimReverse",(__o__)->tilingAnimReverse);\
  S_BOOL("showTilingGrid",(__o__)->showTilingGrid);\
  S_BOOL("showTilingGridIndices",(__o__)->showTilingGridIndices);\
  S_OBJECT_CLOSE();

class Layer {
public:

  int mode;
  int width;
  int height;
  unsigned int id;
  unsigned int *pixels;
  unsigned char *pixelStates;
  LayerState state;
  TransformationState transform;
  ColorTransform colorTransform;
  LayerUndoData undoData;
  LayerAdditionalToolInfo toolInfo;
  char name[LAYERNAMEMAX];

  int repaintState;                    
  bool justSelection;
  bool hasOutline;
  bool showColorTransformOnlyInSelection;

  Layer() {
    mode = LAYERMODE_NORMAL;
    width = 0;
    height = 0;
    pixels = NULL;
    pixelStates = NULL;
    repaintState = REPAINTSTATE_NONE;
    justSelection = false;
    hasOutline = false;
    showColorTransformOnlyInSelection = false;
    static int layerCounter = 0; layerCounter++;
    sprintf(name,"Layer %d", layerCounter);
    static unsigned int idCounter = 0; idCounter++;
    id = idCounter;
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
  void invalidateContent();
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
  void addSelectionRectangle(int lx0, int ly0, int lx1, int ly1);

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
  void updateTilingGrid(bool visible);

  void copyColorTransform();
  void pasteColorTransform();
  void bakeMagic(bool onlyInSelection);

  void addSelectionFromLayer(Layer *source);

  unsigned int byteCount();
  bool hasUpdates();
};

#define S_LAYER(__name__,__o__)\
  S_OBJECT_OPEN(__name__);\
  S_INT("mode",(__o__)->mode);\
  S_INT("width",(__o__)->width);\
  S_INT("height",(__o__)->height);\
  S_NAME("name",(__o__)->name,LAYERNAMEMAX);\
  if (!save) (__o__)->init(1,1);\
  S_ZIPLAYERDATA(i,__o__);\
  S_UINT("id",(__o__)->id);\
  S_LAYERSTATE("state",&(__o__)->state);\
  S_TRANSFORMATIONSTATE("transform",&(__o__)->transform);\
  S_COLORTRANSFORM("colorTransform",&(__o__)->colorTransform);\
  S_LAYERADDITIONALTOOLINFO("toolInfo",&(__o__)->toolInfo);\
  S_BOOL("showColorTransformOnlyInSelection",(__o__)->showColorTransformOnlyInSelection);\
  S_OBJECT_CLOSE();

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
#define S_CANVASCONTROL(__name__,__o__)\
  S_OBJECT_OPEN(__name__);\
  S_DOUBLE("zoom",(__o__)->zoom);\
  S_DOUBLE("pixelSizeX",(__o__)->pixelSizeX);\
  S_DOUBLE("pixelSizeY",(__o__)->pixelSizeY);\
  S_DOUBLE("scrollX",(__o__)->scrollX);\
  S_DOUBLE("scrollY",(__o__)->scrollY);\
  S_OBJECT_CLOSE();


#define PICTURENAMEMAX 16

extern Layer *copyLayer;
String timeDescString();

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
  String timeStamp;
  String comment;
  String version;
  char pictureName[PICTURENAMEMAX];
  char author[PICTURENAMEMAX];

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
    sprintf(author,"");
    timeStamp = timeDescString();
    version = String::fromInt(::version);
    comment = "";
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
  void centerCanvas();

  void cropLayersByAlpha(double alpha);
  bool isOnlyActiveLayerActive();
  void getLayerSelectionFromLayers(Layer *l);
  void clearAllOtherLayerSelections(Layer *l);

  bool hasUpdates();
  void signSave();
};
#define S_CANVAS(__name__,__o__)\
  S_OBJECT_OPEN(__name__);\
  if (!save) canvasDefault(__o__);\
  S_STRING("timeStamp",(__o__)->timeStamp);\
  S_STRING("comment",(__o__)->comment);\
  S_STRING("version",(__o__)->version);\
  S_STRING("fileName",(__o__)->fileName);\
  S_NAME("pictureName", (__o__)->pictureName,PICTURENAMEMAX);\
  S_NAME("author", (__o__)->author,PICTURENAMEMAX);\
  S_BOOL("gridOn",(__o__)->gridOn);\
  S_INT("gridStepX",(__o__)->gridStepX);\
  S_INT("gridStepY",(__o__)->gridStepY);\
  S_INT("gridOffsetX",(__o__)->gridOffsetX);\
  S_INT("gridOffsetY",(__o__)->gridOffsetY);\
  S_INT("canvasX0",(__o__)->canvasX0);\
  S_INT("canvasY0",(__o__)->canvasY0);\
  S_INT("canvasX1",(__o__)->canvasX1);\
  S_INT("canvasY1",(__o__)->canvasY1);\
  S_CANVASCONTROL("control",&(__o__)->control);\
  Canvas *helperCanvas = __o__; S_POINTER_ARRAY("layers",helperCanvas->layers,Layer,S_LAYER(NULL,helperCanvas->layers[i]));\
  S_OBJECT_CLOSE();



extern Canvas *currentCanvas;
extern Array<Canvas*> canvas;
extern bool shouldUpdateAll;
extern bool shouldUpdate;
extern Layer *selectionPlane;
extern bool showSelectionPlane;
extern bool showSelectionPlaneBounding;
extern bool clearOutlineAfterNextPaint;

extern bool shouldRecreateQuantization;

void repaintLayer(Layer *layer, bool fullUpdate);
void blitCanvas();

void canvasDefault(Canvas *canvas);
void addPicture(String fileName);
void addCanvas(Canvas *canvas);
void insertPicture(String fileName);
void addTempPicture();
void newPicture(int w, int h,const String &pictureName);

#define S_LAYERS_CPP()

#endif //__LAYERS_HPP__
