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
#include "IMGUI/IMGUI.H"
#include "IMGUI/IMGUIGL.HPP"
#include "UNDO.HPP"
#include "LAYERS.HPP"
#include "CONFIG.HPP"
#include "SERIALZE.HPP"


bool isForcedUndoPoint = false;

void calcUndoStackByteCount();

class UndoPacket {
public:
  UndoPacket() {
    undoCanvas = NULL;
  }
  ~UndoPacket() {
    freeIt();
  }

  Array<Layer *> undoLayers;
  Canvas *undoCanvas;

  void get() {
    freeIt();
    undoCanvas = currentCanvas;
    for (int i = 0; i < undoCanvas->layers.size(); i++) {
      if (undoCanvas->layers[i]->state.active || undoCanvas->layers[i]->state.selected) {
        Layer *l2 = undoCanvas->layers[i];
        int id = l2->id; 
        Layer *l = l2->clone(); l->id = id; memcpy(l->pixelStates,l2->pixelStates,l->width*l->height*sizeof(unsigned char)); // sorry
        undoLayers.push_back(l);
      }
    }
  }

  void freeIt() {
    for (int i = 0; i < undoLayers.size(); i++) {delete undoLayers[i];}
    undoLayers.clear();
  }

  bool doWeHaveTheUndoCanvas() {
    if (undoCanvas == NULL) return false;
    for (int i = 0; i < canvas.size(); i++) if (canvas[i] == undoCanvas) return true;
    return false;
  }

  bool isLayerSame(Layer *a, Layer *b) {
    if (a->width != b->width) return false;
    if (a->height != b->height) return false;
    if (memcmp(a->pixels,b->pixels,a->width*a->height*sizeof(unsigned int))!=0) return false;
    if (memcmp(a->pixelStates,b->pixelStates,a->width*a->height*sizeof(unsigned char))!=0) return false;
    return true;
  }

  bool isSame(const UndoPacket &source) {
    if (undoLayers.size() != source.undoLayers.size()) return false;
    for (int i = 0; i < undoLayers.size(); i++) {
      int foundSourceLayer = -1;
      for (int j = 0; j < source.undoLayers.size(); j++) {
        if (source.undoLayers[j]->id == undoLayers[i]->id) {
          foundSourceLayer = j;
          break;
        }
      }
      if (foundSourceLayer < 0) return false;
      if (!isLayerSame(undoLayers[i],source.undoLayers[foundSourceLayer])) return false;
    }
    return true;
  }


  unsigned char *getNoRle(int &byteCount) {
#define WRITEDWORDHERE(__a__) {dest[writePos] = (__a__); writePos++;}
    unsigned char *ret = new unsigned char[2*sizeof(unsigned int)];
    unsigned int *dest = (unsigned int *)ret; unsigned int writePos = 0;
    WRITEDWORDHERE(0x77665544)
    WRITEDWORDHERE(0xffffffff)
    byteCount += writePos*4;
    return ret;
  }
#undef WRITEDWORDHERE

  unsigned char *getRleFromDiff(void *a2, void *b2, int bytes, bool doRLE, int &byteCount) {
#define WRITEDWORDHERE(__a__) {if (writePos>=data.size()) {data.resize(data.size()+ints); writeTo = &data[0];} writeTo[writePos] = (__a__); writePos++;}
    unsigned int *a = (unsigned int *)a2;
    unsigned int *b = (unsigned int *)b2;
    int ints = bytes / 4; // some elements are missing here
    static Array<unsigned int> data; data.resize(ints); unsigned int *writeTo = &data[0]; int writePos = 0;
    bool inColorData = true; int pixelCount = 0;
    WRITEDWORDHERE(0x77665544)
    if (doRLE) {
      for (int i = 0; i < ints; i++) {
        bool inColorDataHere = a[i] != b[i];
        if (inColorDataHere != inColorData) {
          WRITEDWORDHERE(pixelCount);
          if (inColorData) {
            for (int j = i-pixelCount; j < i; j++) WRITEDWORDHERE(a[j]);
          }
          inColorData = inColorDataHere;
          pixelCount = 0;
        } 
        pixelCount++;
      }
    } else {
      pixelCount = ints;
    }
    WRITEDWORDHERE(pixelCount);
    if (inColorData) {
      for (int j = ints-pixelCount; j < ints; j++) WRITEDWORDHERE(a[j]);
    }
    WRITEDWORDHERE(0xffffffff);
    unsigned char *ret = new unsigned char[writePos*4];
    byteCount += writePos*4;
    memcpy(ret,&data[0],writePos*4);
    return ret;
#undef WRITEDWORDHERE
  }

  void unpackRleStream(void *dest, unsigned char *rle) {
    unsigned int *read = (unsigned int *)rle;
    unsigned int *write = (unsigned int *)dest;
    read++; // kennung
    bool inColorData = true; unsigned int pixelCount = 0;
    while((pixelCount=*read)!=0xffffffff) {
      read++;
      if (inColorData) {
        for (int i = 0; i < pixelCount; i++) {
          *write++ = *read++;
        }
      } else {
        write += pixelCount;
      }
      inColorData = !inColorData;
    }
  }

  void tempDebugOutput() {
    initStatusBar();
    for (int i = 0; i < 100; i++) updateStatusBar();
  }

  void generateDeltas(Layer *dest, Layer *source) {
    // source is undopacket "before"
    // dest is current undopacket
    dest->undoData.width_forward = dest->width;
    dest->undoData.height_forward = dest->height;
    dest->undoData.width_backward = source->width;
    dest->undoData.height_backward = source->height;
    bool doRLE = true;
    if (dest->width != source->width || dest->height != source->height) doRLE = false;

    int lastUndoPixels = source->undoData.updatePixelsCountOfLastUndo;
    int lastUndoPixelStates = source->undoData.updatePixelStatesCountOfLastUndo;

    bool needsFullSourcePixels = source->undoData.updatePixelsCount != lastUndoPixels && (lastUndoPixels >= 0);
    bool needsFullSourcePixelStates = source->undoData.updatePixelStatesCount != lastUndoPixelStates && (lastUndoPixelStates >= 0);

    //if (needsFullSourcePixels) {tempDebugOutput();}
    //if (needsFullSourcePixelStates) {tempDebugOutput();}

    if (isForcedUndoPoint) {
      needsFullSourcePixels = true;
      needsFullSourcePixelStates = true;
    }

    bool emptyPixels = source->undoData.updatePixelsCount == dest->undoData.updatePixelsCount;
    bool emptyPixelStates = source->undoData.updatePixelStatesCount == dest->undoData.updatePixelStatesCount;

    int byteCount = 0;
    dest->undoData.freeIt();
    dest->undoData.rleDataPixels_forward = emptyPixels ? getNoRle(byteCount) : getRleFromDiff(dest->pixels,source->pixels,dest->width*dest->height*sizeof(unsigned int),doRLE,byteCount);
    dest->undoData.rleDataPixelStates_forward = emptyPixelStates ? getNoRle(byteCount) : getRleFromDiff(dest->pixelStates,source->pixelStates,dest->width*dest->height*sizeof(unsigned char),doRLE,byteCount);
    dest->undoData.rleDataPixels_backward = emptyPixels&&(!needsFullSourcePixels) ? getNoRle(byteCount) : getRleFromDiff(source->pixels,dest->pixels,source->width*source->height*sizeof(unsigned int),doRLE&&(!needsFullSourcePixels),byteCount);
    dest->undoData.rleDataPixelStates_backward = emptyPixelStates&&(!needsFullSourcePixels) ? getNoRle(byteCount) : getRleFromDiff(source->pixelStates,dest->pixelStates,source->width*source->height*sizeof(unsigned char),doRLE&&(!needsFullSourcePixelStates),byteCount);
    dest->undoData.completeByteCount = byteCount;

    dest->undoData.updatePixelsCountOfLastUndo2 = source->undoData.updatePixelsCountOfLastUndo;
    dest->undoData.updatePixelStatesCountOfLastUndo2 = source->undoData.updatePixelStatesCountOfLastUndo;
    dest->undoData.updatePixelsCount2 = source->undoData.updatePixelsCount;
    dest->undoData.updatePixelStatesCount2 = source->undoData.updatePixelStatesCount;
  }

  int byteCount() {
    int ret = 0;
    for (int i = 0; i < undoLayers.size(); i++) {
      ret += undoLayers[i]->byteCount();
    }
    return ret;
  }

  void deltaPackFrom(const UndoPacket &source) {
    for (int i = 0; i < undoLayers.size(); i++) {
      int foundSourceLayer = -1;
      for (int j = 0; j < source.undoLayers.size(); j++) {
        if (source.undoLayers[j]->id == undoLayers[i]->id) {
          foundSourceLayer = j;
          break;
        }
      }
      if (foundSourceLayer < 0) {
        undoLayers.erase(i,1);
        i--;
        continue;
      }
      bool hasChanged = !isLayerSame(undoLayers[i],source.undoLayers[foundSourceLayer]);
      if ((!hasChanged)&&(!isForcedUndoPoint)) {
        undoLayers.erase(i,1);
        i--;
        continue;
      }
      generateDeltas(undoLayers[i],source.undoLayers[foundSourceLayer]);
      undoLayers[i]->init(1,1);
    }
    calcUndoStackByteCount();
  }

  void apply(bool forward) {
    if (!doWeHaveTheUndoCanvas()) return;
    for (int i = 0; i < undoLayers.size(); i++) {
      int foundLayer = -1;
      for (int j = 0; j < undoCanvas->layers.size(); j++) {
        if (undoCanvas->layers[j]->id == undoLayers[i]->id) {
          foundLayer = j;
          break;
        }
      }
      if (foundLayer < 0) continue;
      Layer *source = undoLayers[i];
      Layer *dest = undoCanvas->layers[foundLayer];
      int newWidth = forward ? source->undoData.width_forward : source->undoData.width_backward;
      int newHeight = forward ? source->undoData.height_forward : source->undoData.height_backward;
      unsigned char *rleStreamPixels = forward ? source->undoData.rleDataPixels_forward : source->undoData.rleDataPixels_backward;
      unsigned char *rleStreamPixelStates = forward ? source->undoData.rleDataPixelStates_forward : source->undoData.rleDataPixelStates_backward;
      if (newHeight != dest->height || newWidth != dest->width) {
        dest->init(newWidth,newHeight);
      }
      unpackRleStream(dest->pixels,rleStreamPixels);
      unpackRleStream(dest->pixelStates,rleStreamPixelStates);

      if (!forward)
      {
        // undo
        dest->undoData.updatePixelsCountOfLastUndo = source->undoData.updatePixelsCountOfLastUndo2;
        dest->undoData.updatePixelStatesCountOfLastUndo = source->undoData.updatePixelStatesCountOfLastUndo2;
        dest->undoData.updatePixelsCount = source->undoData.updatePixelsCount2;
        dest->undoData.updatePixelStatesCount = source->undoData.updatePixelStatesCount2;
      } else {
        // redo
        dest->undoData.updatePixelsCountOfLastUndo = source->undoData.updatePixelsCountOfLastUndo;
        dest->undoData.updatePixelStatesCountOfLastUndo = source->undoData.updatePixelStatesCountOfLastUndo;
        dest->undoData.updatePixelsCount = source->undoData.updatePixelsCount;
        dest->undoData.updatePixelStatesCount = source->undoData.updatePixelStatesCount;
      }
    }
    shouldUpdateAll = true;
  }

  void signSourceLayersAfterUndoPoint() {
    if (!doWeHaveTheUndoCanvas()) return;
    for (int i = 0; i < undoLayers.size(); i++) {
      int foundLayer = -1;
      for (int j = 0; j < undoCanvas->layers.size(); j++) {
        if (undoCanvas->layers[j]->id == undoLayers[i]->id) {
          foundLayer = j;
          break;
        }
      }
      if (foundLayer >= 0) undoCanvas->layers[foundLayer]->undoData.signUndo();
    }
  }
};

UndoPacket packetBeforeUndo;
Array<UndoPacket *> undoStages;

int undoStackBytes = 0;
void calcUndoStackByteCount() {
  undoStackBytes = 0;
  for (int i = 0; i < undoStages.size(); i++) {
    undoStackBytes += undoStages[i]->byteCount();
  }
}

int availableUndos = 0;
int availableRedos = 0;

void beforeUndoPoint() {
  if (useUndo) {
    packetBeforeUndo.get();
  }
}

void afterUndoPoint() {
  if (useUndo) {
    UndoPacket *packetAfterUndo = new UndoPacket();
    packetAfterUndo->get();
    if (packetAfterUndo->isSame(packetBeforeUndo) && (!isForcedUndoPoint)) {
      delete packetAfterUndo;
    } else {
      for (int i2 = undoStages.size()-availableUndos-1; i2 >= 0 ; i2--) {
        if (!undoStages.empty()) {
          delete undoStages.back();
          undoStages.resize(undoStages.size()-1);
        }
      }
      availableRedos = 0;
      packetAfterUndo->deltaPackFrom(packetBeforeUndo);
      packetAfterUndo->signSourceLayersAfterUndoPoint();
      undoStages.push_back(packetAfterUndo);
      if (undoStages.size()>undoStageCount) {
        delete undoStages[0];
        undoStages.erase(0,1);
      }
      availableUndos = undoStages.size();
    }
  }
}

void forceUndoPoint() {
  if (useUndo) {
    beforeUndoPoint();
    isForcedUndoPoint = true;
    afterUndoPoint();
    isForcedUndoPoint = false;
  }
}

void undo() {
  if (useUndo) {
    if (undoStages.size()>0) {
      if (availableRedos > undoStages.size()) return;
      UndoPacket *a = undoStages[undoStages.size()-1-availableRedos];
      a->apply(false);
      availableRedos++;
      if (availableRedos > undoStages.size()) availableRedos = undoStages.size();
      availableUndos = undoStages.size() - availableRedos;
    }
  }
}

void redo() {
  if (useUndo) {
    if (undoStages.size()>0) {
      availableUndos++;
      if (availableUndos > undoStages.size()) availableUndos = undoStages.size();
      availableRedos = undoStages.size() - availableUndos;
      if (availableRedos > undoStages.size()) return;
      UndoPacket *a = undoStages[undoStages.size()-1-availableRedos];
      a->apply(true);
    }
  }
}
