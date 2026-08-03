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
  LAYERMODE_ALPHA = 1,
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
  LAYERMODE_COPY = 17,
  LAYERMODE_DEFAULT = LAYERMODE_PREMULTIPLIEDALPHA,
  LAYERMODE_COUNT = 18
};
extern const char *layerModeNames[LAYERMODE_COUNT];

enum LAYERTYPES {
  LAYERTYPE_BASIC = 0,
  LAYERTYPE_TEXT = 1,
  LAYERTYPE_ADJUSTMENT = 2,
  LAYERTYPE_TILEMAP = 3
};

enum ALIGNMENTS {
  ALIGN_LEFT = 0,
  ALIGN_RIGHT = 1,
  ALIGN_CENTER = 2,
};

enum LOCKS {
  LOCK_NONE = 0,
  // querying
  LOCK_ANY = 1,
  LOCK_PERMANENT = 2,
  LOCK_PIXELS = 3,
  LOCK_PIXELS_AND_TILEMAP = 4,
  LOCK_SELECTION = 5,
  LOCK_UNDO = 6,
  // initializing
  LOCK_TILEMAP = 1000,
  LOCK_TEXT = 1001,
  LOCK_REFERENCE = 1002,
  // user defined
  LOCK_USER = 0x40000000
};

#define NOTILEMAPPIXEL 0x77777777

class LayerState {
public:

  bool active;
  bool selected;
  bool visible;
  unsigned int lockBits;
  float alpha;
  bool imGuiActive;

  LayerState() {
    active = false;
    selected = false;
    visible = true;
    lockBits = LOCK_NONE;
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
  S_UINT("lockBits",(__o__)->lockBits);\
  S_FLOAT("alpha",(__o__)->alpha);\
  S_OBJECT_CLOSE();

bool locked(const class Layer *l, unsigned int lockFlags);

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
  PIXELSTATE_CANVAS_GRID = 64,
  PIXELSTATE_CANVAS_OVERLAY = 128
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
    useThreshold = false;
    useLut = false;
    lut = NULL;
    reset();
  }
  float colorMatrix[4*5];
  float hue;
  float saturation;
  float lightness;
  float brightness;
  bool removeGreen;
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
  float warmth;
  float posterizeGrey;
  float posterizeRed;
  float posterizeGreen;
  float posterizeBlue;
  float posterizeAlpha;
  float posterizeGreyBase;
  float posterizeRedBase;
  float posterizeGreenBase;
  float posterizeBlueBase;
  float posterizeAlphaBase;
  bool posterizeUseGrey;
  float thresholdRed;
  float thresholdGreen;
  float thresholdBlue;
  float thresholdAlpha;
  float thresholdLuma;
  bool thresholdInvert;
  bool thresholdBlackWhite;
  float ditherStrength;
  int paletteShift;
  float middleRedAdjust;
  float middleGreenAdjust;
  float middleBlueAdjust;
  bool perceptional;
  bool useColorTransform;
  bool useQuantization;
  bool usePosterize;
  bool useThreshold;
  String gradientCurves;
  Array<float> *lut;
  bool useLut;
  float lutInfluence;
  int lutRSize;
  int lutGSize;
  int lutBSize;
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
  unsigned int applyLut(unsigned int pixel);
  unsigned int applyPosterize(unsigned int pixel);
  unsigned int applyThreshold(unsigned int pixel);
  int byteCount();
};
#define S_COLORTRANSFORM(__name__,__o__)\
  S_OBJECT_OPEN(__name__);\
  ColorTransform *ctr = (__o__); S_CARRAY("colorMatrix",ctr->colorMatrix,S_FLOAT(NULL,ctr->colorMatrix[i]),4*5);\
  S_FLOAT("hue",(__o__)->hue);\
  S_FLOAT("saturation",(__o__)->saturation);\
  S_FLOAT("lightness",(__o__)->lightness);\
  S_FLOAT("brightness",(__o__)->brightness);\
  S_FLOAT("contrast",(__o__)->contrast);\
  S_BOOL("removeGreen",(__o__)->removeGreen);\
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
  S_FLOAT("warmth",(__o__)->warmth);\
  S_FLOAT("posterizeGrey",(__o__)->posterizeGrey);\
  S_FLOAT("posterizeRed",(__o__)->posterizeRed);\
  S_FLOAT("posterizeGreen",(__o__)->posterizeGreen);\
  S_FLOAT("posterizeBlue",(__o__)->posterizeBlue);\
  S_FLOAT("posterizeAlpha",(__o__)->posterizeAlpha);\
  S_FLOAT("posterizeGreyBase",(__o__)->posterizeGreyBase);\
  S_FLOAT("posterizeRedBase",(__o__)->posterizeRedBase);\
  S_FLOAT("posterizeGreenBase",(__o__)->posterizeGreenBase);\
  S_FLOAT("posterizeBlueBase",(__o__)->posterizeBlueBase);\
  S_FLOAT("posterizeAlphaBase",(__o__)->posterizeAlphaBase);\
  S_BOOL("posterizeUseGrey",(__o__)->posterizeUseGrey);\
  S_FLOAT("thresholdRed",(__o__)->thresholdRed);\
  S_FLOAT("thresholdGreen",(__o__)->thresholdGreen);\
  S_FLOAT("thresholdBlue",(__o__)->thresholdBlue);\
  S_FLOAT("thresholdAlpha",(__o__)->thresholdAlpha);\
  S_BOOL("thresholdInvert",(__o__)->thresholdInvert);\
  S_BOOL("thresholdBlackWhite",(__o__)->thresholdBlackWhite);\
  S_FLOAT("thresholdLuma",(__o__)->thresholdLuma);\
  S_FLOAT("ditherStrength",(__o__)->ditherStrength);\
  S_INT("paletteShift",(__o__)->paletteShift);\
  S_FLOAT("middleRedAdjust",(__o__)->middleRedAdjust);\
  S_FLOAT("middleGreenAdjust",(__o__)->middleGreenAdjust);\
  S_FLOAT("middleBlueAdjust",(__o__)->middleBlueAdjust);\
  S_BOOL("perceptional",(__o__)->perceptional);\
  S_BOOL("useColorTransform",(__o__)->useColorTransform);\
  S_BOOL("usePosterize",(__o__)->usePosterize);\
  S_BOOL("useThreshold",(__o__)->useThreshold);\
  S_STRING("gradientCurves",(__o__)->gradientCurves);\
  if (!save) {if ((__o__)->lut == NULL) (__o__)->lut = new Array<float>(); (__o__)->lut->clear(); Array<float> *helper = (__o__)->lut; S_ZIPARRAY(combined(String("lut")+String::fromInt(i)+".rgb"),(*helper),sizeof(float)); if ((__o__)->lut->size()==0) {delete (__o__)->lut; (__o__)->lut = NULL;}}\
  if (save && ((__o__)->lut != NULL)) {Array<float> *helper = (__o__)->lut; S_ZIPARRAY(combined(String("lut")+String::fromInt(i)+".rgb"),(*helper),sizeof(float));}\
  S_BOOL("useLut",(__o__)->useLut);\
  S_FLOAT("lutInfluence",(__o__)->lutInfluence);\
  S_INT("lutRSize",(__o__)->lutRSize);\
  S_INT("lutGSize",(__o__)->lutGSize);\
  S_INT("lutBSize",(__o__)->lutBSize);\
  if (!save) (__o__)->shouldUpdate = true;\
  S_OBJECT_CLOSE();

unsigned int quantizeFast(unsigned int col, float ditherStrength, int x, int y, int shift);

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
    tilingMinColorCount = 0;

    tilingAnimFPS = 30;
    tilingAnimFirstFrame = 0;
    tilingAnimLastFrame = 0;
    tilingAnimPlaying = false;
    tilingAnimReverse = false;
    tilingAnimDoubleSided = false;
    tilingAnimTime = 0;
    tilingAnimFrame = 0;

    tileDrawX0 = 0;
    tileDrawY0 = 0;
    tileDrawX1 = 0;
    tileDrawY1 = 0;

    tileMapSelectedTile = 0;

    showTilingGrid = true;
    showTilingGridIndices = true;

    sprintf(textEditText,"Some text");
    fontSize = 50;
    fontScaleX = 1;
    fontScaleY = 1;
    fontSpaceX = 0;
    fontSpaceY = 0;
    fontAlignment = ALIGN_LEFT;
    fontColor = 0xff202020;
    fontId = 0;
    blink = false;
  }
  int tilingOffsetX;
  int tilingOffsetY;
  int tilingWidth;
  int tilingHeight;
  int tilingMinColorCount;

  float tilingAnimFPS;
  int tilingAnimFirstFrame;
  int tilingAnimLastFrame;
  int tilingAnimFrame;
  bool tilingAnimPlaying;
  bool tilingAnimReverse;
  bool tilingAnimDoubleSided;
  float tilingAnimTime;

  int tileDrawX0;
  int tileDrawY0;
  int tileDrawX1;
  int tileDrawY1;

  int tileMapSelectedTile;

  bool showTilingGrid;
  bool showTilingGridIndices;

  char textEditText[MAXTEXTEDITTEXT];
  float fontSize;
  float fontScaleX;
  float fontScaleY;
  float fontSpaceX;
  float fontSpaceY;
  int fontAlignment;
  unsigned int fontColor;
  unsigned int fontId;

  bool blink;
};
#define S_LAYERADDITIONALTOOLINFO(__name__,__o__)\
  S_OBJECT_OPEN(__name__);\
  S_INT("tilingOffsetX",(__o__)->tilingOffsetX);\
  S_INT("tilingOffsetY",(__o__)->tilingOffsetY);\
  S_INT("tilingWidth",(__o__)->tilingWidth);\
  S_INT("tilingHeight",(__o__)->tilingHeight);\
  S_INT("tilingMinColorCount",(__o__)->tilingMinColorCount);\
  S_FLOAT("tilingAnimFPS",(__o__)->tilingAnimFPS);\
  S_INT("tilingAnimFirstFrame",(__o__)->tilingAnimFirstFrame);\
  S_INT("tilingAnimLastFrame",(__o__)->tilingAnimLastFrame);\
  S_BOOL("tilingAnimPlaying",(__o__)->tilingAnimPlaying);\
  S_BOOL("tilingAnimReverse",(__o__)->tilingAnimReverse);\
  S_BOOL("tilingAnimDoubleSided",(__o__)->tilingAnimDoubleSided);\
  S_INT("tileDrawX0",(__o__)->tileDrawX0);\
  S_INT("tileDrawY0",(__o__)->tileDrawY0);\
  S_INT("tileDrawX1",(__o__)->tileDrawX1);\
  S_INT("tileDrawY1",(__o__)->tileDrawY1);\
  S_INT("tileMapSelectedTile",(__o__)->tileMapSelectedTile);\
  S_BOOL("showTilingGrid",(__o__)->showTilingGrid);\
  S_BOOL("showTilingGridIndices",(__o__)->showTilingGridIndices);\
  S_BASE64("textEditText",(__o__)->textEditText,MAXTEXTEDITTEXT);\
  S_FLOAT("fontSize",(__o__)->fontSize);\
  S_FLOAT("fontScaleX",(__o__)->fontScaleX);\
  S_FLOAT("fontScaleY",(__o__)->fontScaleY);\
  S_FLOAT("fontSpaceX",(__o__)->fontSpaceX);\
  S_FLOAT("fontSpaceY",(__o__)->fontSpaceY);\
  S_INT("fontAlignment",(__o__)->fontAlignment);\
  S_UINT("fontId",(__o__)->fontId);\
  S_UINT("fontColor",(__o__)->fontColor);\
  S_OBJECT_CLOSE();

extern unsigned int layerIdCounter;
void reverseLastLayerIdCountUp();

class Layer {
public:

  int type;
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

  unsigned int sourceLayerId;
  unsigned int tileSourceLayerId;

  Layer() {
    mode = LAYERMODE_DEFAULT;
    type = LAYERTYPE_BASIC;
    width = 0;
    height = 0;
    pixels = NULL;
    pixelStates = NULL;
    sourceLayerId = 0;
    tileSourceLayerId = 0;
    repaintState = REPAINTSTATE_NONE;
    justSelection = false;
    hasOutline = false;
    showColorTransformOnlyInSelection = false;
    layerIdCounter++; id = layerIdCounter;
    sprintf(name,"Layer %d", layerIdCounter);
  }

  ~Layer() {
    if (pixels != NULL) {delete[] pixels; pixels = NULL;}
    if (pixelStates != NULL) {delete[] pixelStates; pixelStates = NULL;}
  }

  unsigned int getPixel(int x, int y);
  void setPixel(int x, int y, unsigned int rgba);
  void pixelLine(int x0, int y0, int x1, int y1, unsigned int color);
  void pixelLineOr(int x0, int y0, int x1, int y1, unsigned int _or, bool clampOnLayer);
  void pixelLineEor(int x0, int y0, int x1, int y1, unsigned int _eor, bool clampOnLayer);
  void pixelXLineEor(int x0, int y0, int x1, int y1, unsigned int _eor, bool clampOnLayer);

  void init(int width, int height);
  void setContentSize(int width, int height);
  void invalidateContent();
  void clear();
  void updatePixelStates(unsigned char _and, unsigned char _or, unsigned char _eor);

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
  void greyScale();
  void normalize();
  void whiteBalance();
  void invertColor();
  void invertAlpha();
  void expandSelection(int pixels);
  void selectByAlpha(int alphaThresholdMin,int alphaThresholdMax);
  void selectionAsOutline();
  void addSelectionRectangle(int lx0, int ly0, int lx1, int ly1);

  bool hasSelection();
  Vector boundingBox();
  Vector selectionBounding();
  Vector boundingByAlpha(double alphaThreshold);
  void quantizeBoundingBox();
  void quantizePosition();

  void resizeLayerContent(int interpolation, int newWidth, int newHeight);
  void bakeLayer(int interpolation);
  Layer *clone();
  void convertToPremul();
  void convertToBitmap(class Canvas *canvas);

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
  void bakeMagic(bool onlyInSelection);

  void addSelectionFromLayer(Layer *source);

  unsigned int byteCount();
  bool hasUpdates();
};

#define S_LAYER(__name__,__o__)\
  S_OBJECT_OPEN(__name__);\
  S_INT("width",(__o__)->width);\
  S_INT("height",(__o__)->height);\
  S_NAME("name",(__o__)->name,LAYERNAMEMAX);\
  if (!save) (__o__)->init(1,1);\
  S_ZIPLAYERDATA(i,__o__);\
  S_INT("mode",(__o__)->mode);\
  S_INT("type",(__o__)->type);\
  S_ID("id",(__o__)->id);\
  S_IDREFERENCE("sourceLayerId",(__o__)->sourceLayerId);\
  S_IDREFERENCE("tileSourceLayerId",(__o__)->tileSourceLayerId);\
  S_LAYERSTATE("state",&(__o__)->state);\
  S_TRANSFORMATIONSTATE("transform",&(__o__)->transform);\
  S_COLORTRANSFORM("colorTransform",&(__o__)->colorTransform);\
  S_LAYERADDITIONALTOOLINFO("toolInfo",&(__o__)->toolInfo);\
  S_BOOL("showColorTransformOnlyInSelection",(__o__)->showColorTransformOnlyInSelection);\
  S_OBJECT_CLOSE();

#define ANNOTATIONTEXTMAX 16

enum OVERLAYTYPES {
  OVERLAYTYPE_NORMAL = 0,
  OVERLAYTYPE_ANNOTATION = 1,
  OVERLAYTYPE_SEPERATINGLINEX = 2,
  OVERLAYTYPE_SEPERATINGLINEY = 3,
  OVERLAYTYPE_CANVASSPRITE = 4
};

class Overlay {
public:
  Overlay() {
    layerId = 0;
    lx0 = ly0 = lx1 = ly1 = 0;
    cx0 = cy0 = cx1 = cy1 = 0;
    visible = true;
    type = OVERLAYTYPE_NORMAL;
    annotationColor = 0xffffffff;
    annotationFontSize = 3.f;
    sprintf(annotationText,"annotation");
    static int idCounter = 0; idCounter++;
    id = idCounter;
    sourceCounter = 0;
  }
  bool visible;
  int id;
  int type;
  unsigned int sourceCounter;
  unsigned int layerId;
  int lx0,ly0,lx1,ly1;
  int cx0,cy0,cx1,cy1;
  unsigned int annotationColor;
  float annotationFontSize;
  char annotationText[ANNOTATIONTEXTMAX];
};
#define S_OVERLAY(__name__,__o__)\
  S_OBJECT_OPEN(__name__);\
  S_IDREFERENCE("layerId",(__o__)->layerId);\
  S_INT("id",(__o__)->id);\
  S_INT("type",(__o__)->type);\
  S_BOOL("visible",(__o__)->visible);\
  S_INT("lx0",(__o__)->lx0);\
  S_INT("ly0",(__o__)->ly0);\
  S_INT("lx1",(__o__)->lx1);\
  S_INT("ly1",(__o__)->ly1);\
  S_INT("cx0",(__o__)->cx0);\
  S_INT("cy0",(__o__)->cy0);\
  S_INT("cx1",(__o__)->cx1);\
  S_INT("cy1",(__o__)->cy1);\
  S_UINT("annotationColor",(__o__)->annotationColor);\
  S_NAME("annotationText",(__o__)->annotationText,ANNOTATIONTEXTMAX);\
  S_FLOAT("annotationFontSize",(__o__)->annotationFontSize);\
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
extern unsigned int activeLayerSwitchIndex;
extern unsigned int lastActiveLayerSwitchIndex;
bool wasActiveLayerSwitched();

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
  bool layerShapesOn;

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
  Array<Overlay*> overlays;

  bool hilightSelection;
  bool justSingle;

  bool showOverlays;
  bool showAnnotations;
  bool showLines;
  bool showSprites;

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
    static unsigned short counter = 0; counter++;
    sprintf(pictureName,"Picture %d",counter);
    sprintf(author,"unknown");
    timeStamp = timeDescString();
    version = String::fromInt(::version);
    comment = "";
    gridOn = false;
    gridStepX = 8;
    gridStepY = 8;
    gridOffsetX = 0;
    gridOffsetY = 0;
    layerShapesOn = false;
    hilightSelection = false;
    justSingle = false;
    showOverlays = true;
    showAnnotations = true;
    showLines = true;
    showSprites = true;
  }

  ~Canvas() {
    if (pixels != NULL) {delete[] pixels; pixels = NULL;}
    if (pixelStates != NULL) {delete[] pixelStates; pixelStates = NULL;}
    for (int i = 0; i < (int)layers.size(); i++) {
      if (layers[i] != NULL) {delete layers[i]; layers[i] = NULL;}
    }
    layers.clear();
    for (int i2 = 0; i2 < (int)overlays.size(); i2++) {
      if (overlays[i2] != NULL) {delete overlays[i2]; overlays[i2] = NULL;}
    }
    overlays.clear();
  }

  void init(int width, int height);
  void resize(int newWidth, int newHeight);
  Canvas *clone();
  void clearUpdateFlag();
  void updatePixelStates(unsigned char _and, unsigned char _or, unsigned char _eor);

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
  void insertTextLayer();
  void insertAdjustmentLayer();

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
  void convertFromAlphaToPremul();
  void convertFromTilemapToBitmap();

  void selectAll();
  void selectNone(bool all);
  void invertSelection();
  bool hasSelection();
  void copyActiveSelectionToLayer(int layerIndexSource,int layerIndexDest);
  void selectByAlpha(int alphaThresholdMin, int alphaThresholdMax);
  void selectionAsOutline();
  void selectAllLayers();
  void showAllLayers(bool show);

  unsigned int moveLayersUp();
  unsigned int moveLayersDown();
  void compactLayers();

  void fillSelectionWithColor(unsigned int color);
  void fillSelectionWithAlpha(unsigned char alpha);
  void invertColor();
  void invertAlpha();
  void expandSelection(int pixels);
  void antialiasSelected();
  void greyScale();
  void normalize();
  void whiteBalance();
  void setPixel(double x, double y, unsigned int color);
  void pixelLine(double x0,double y0,double x1,double y1,unsigned int color);
  
  Layer *getSelectionAsLayer(bool pixelSelection, bool selectAllLayers, bool keepCanvasSize);
  void duplicateLayers();
  void duplicateLayerAsReference(Layer *l);
  void insertLayerLikeThisLayer();
  void copy();
  void paste();

  int getActiveLayerIndex();
  Layer *getActiveLayer();
  void setActiveLayer(int layer);
  void noSelectedLayers();
  void setActiveLayerByScreenPos(double screenX, double screenY);

  Vector boundingBox();
  Vector getLayerPixelBounding(Layer *l, double canvasX0, double canvasY0, double canvasX1, double canvasY1);
  void setBoundingPos(double x, double y);
  void setBoundingSize(double width, double height);
  double getBoundingAngleDegrees();
  void setBoundingAngleDegrees(double degrees);
  void snapToGrid();
  void centerCanvas();
  void fitCanvas();

  void cropLayersByAlpha(double alpha);
  bool isOnlyActiveLayerActive();
  void getLayerSelectionFromLayers(Layer *l);
  void clearAllOtherLayerSelections(Layer *l);

  bool hasUpdates();
  void signSave();

  void lockLayers(bool lockOrUnlock);
  void getContentFromOtherLayer(Layer *dest, unsigned int sourceId);

  Layer *getLayerById(unsigned int id);
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
  S_BOOL("layerShapesOn",(__o__)->layerShapesOn);\
  S_INT("canvasX0",(__o__)->canvasX0);\
  S_INT("canvasY0",(__o__)->canvasY0);\
  S_INT("canvasX1",(__o__)->canvasX1);\
  S_INT("canvasY1",(__o__)->canvasY1);\
  S_BOOL("showAnnotations",(__o__)->showAnnotations);\
  S_BOOL("showOverlays",(__o__)->showOverlays);\
  S_BOOL("showLines",(__o__)->showLines);\
  S_BOOL("showSprites",(__o__)->showSprites);\
  S_CANVASCONTROL("control",&(__o__)->control);\
  (__o__)->control.setCanvasTransform(__o__);\
  Canvas *helperCanvas = __o__; S_POINTER_ARRAY("layers",helperCanvas->layers,Layer,S_LAYER(NULL,helperCanvas->layers[i]));\
  S_POINTER_ARRAY("overlays",helperCanvas->overlays,Overlay,S_OVERLAY(NULL,helperCanvas->overlays[i]));\
  S_RESOLVEIDS();\
  S_OBJECT_CLOSE();

extern Canvas *currentCanvas;
extern Array<Canvas*> canvas;
extern bool shouldUpdateAll;
extern bool shouldUpdate;
extern Layer *selectionPlane;
extern bool showSelectionPlane;
extern bool showSelectionPlaneBounding;
extern bool clearOutlineAfterNextPaint;
extern float monitorGamma;
extern bool shouldRecreateQuantization;
extern bool showOnlyActiveLayersSelection;
extern bool blitOutlineSolidOneFrame;

Layer *layerFromCanvas(Canvas *canvas);
void repaintLayer(Layer *layer, bool fullUpdate);
void blitCanvas();

void canvasDefault(Canvas *canvas);
void addPicture(String fileName);
void addCanvas(Canvas *canvas);
void insertPicture(String fileName);
void addTempPicture();
void newPicture(int w, int h,const String &pictureName);
void duplicatePicture(bool mergeLayers);

#define S_LAYERS_CPP()\
  S_BOOL("showOnlyActiveLayersSelection", showOnlyActiveLayersSelection);

#endif //__LAYERS_HPP__
