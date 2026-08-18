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
#include "CONFIG.HPP"
#include "IMGUI/IMGUI.H"
#ifdef OTHERPLATFORM
#define WATCOMGLAPI
#endif //OTHERPLATFORM
#include "STL/GL.H"
#include "STL/STRING.HPP"
#include "LAYERS.HPP"

double fps = 30.0;
double timeDelta = 0;
double mouseDX = 0, mouseDY = 0;
unsigned int mouseB = 0;
bool shift = false, ctrl = false, alt = false;
unsigned int controlKeyId = 0;
const char *appName = "WYPA";
uint32_t main_window_xres = 1024;
uint32_t main_window_yres = 768;
double aspectRatio = 16.0/9.0;
//double aspectRatio = 4.0/3.0;
const char *logFileName = "WYPA.LOG";
String appFolder;
bool refreshMasked = true;
int version = 95;
double zoomStep = 1.25;
double scrollKeyPixels = main_window_xres/20;
unsigned int undoStageCount = 10;
bool useUndo = true;

bool debugWindowOn = false;
bool showToolTips = true;

extern int undoStackBytes;

#ifdef __WATCOMC__
#include <i86.h>
struct meminfo {
    unsigned LargestBlockAvail;
    unsigned MaxUnlockedPage;
    unsigned LargestLockablePage;
    unsigned LinAddrSpace;
    unsigned NumFreePagesAvail;
    unsigned NumPhysicalPagesFree;
    unsigned TotalPhysicalPages;
    unsigned FreeLinAddrSpace;
    unsigned SizeOfPageFile;
    unsigned Reserved[3];
} MemInfo;
#endif
#ifdef __DJGPP__
#include <dpmi.h>
//typedef struct {
//  unsigned long largest_available_free_block_in_bytes;
//  unsigned long maximum_unlocked_page_allocation_in_pages;
//  unsigned long maximum_locked_page_allocation_in_pages;
//  unsigned long linear_address_space_size_in_pages;
//  unsigned long total_number_of_unlocked_pages;
//  unsigned long total_number_of_free_pages;
//  unsigned long total_number_of_physical_pages;
//  unsigned long free_linear_address_space_in_pages;
//  unsigned long size_of_paging_file_partition_in_pages;
//  unsigned long reserved[3];
//} __dpmi_free_mem_info;
#endif

int getAdditionalMemoryFreeBytes() {
#ifdef __WATCOMC__
  union REGS regs;
  struct SREGS sregs;
  memset( &regs, 0, sizeof(regs) );
  memset( &sregs, 0, sizeof(sregs) );
  regs.w.ax = 0x500;
  sregs.es = FP_SEG( &MemInfo );
  regs.x.edi = FP_OFF( &MemInfo );
  int386x( 0x31, &regs, &regs, &sregs );
  int pageSize = 4096;
  int mem = 0;
  if (mem == 0 && MemInfo.LargestBlockAvail != 0xffffffff) mem = MemInfo.LargestBlockAvail;
  return mem;
#else
  __dpmi_free_mem_info memInfo;
  __dpmi_get_free_memory_information(&memInfo);
  int mem = 0;
  if (memInfo.largest_available_free_block_in_bytes  != 0xffffffff) mem = memInfo.largest_available_free_block_in_bytes;
  return mem;
#endif
}

float getFreeMemoryInKb() {
  return (float)getAdditionalMemoryFreeBytes()/1024.f;
}

int getLayersByteCount() {
  int ret = 0;
  for (int i = 0; i < (int)canvas.size(); i++) {
    for (int j = 0; j < (int)canvas[i]->layers.size(); j++) {
      ret += canvas[i]->layers[j]->byteCount();
    }
    ret += canvas[i]->byteCount();
  }
  return ret;
}

extern Array<float> paintValues;
extern Array<unsigned int> paintBackground;
extern Array<unsigned short> quantization;
extern "C" {extern GLint glFrameBufferMultiSample;}

int getWatcomGLBytes() {
  int ret = 0;
  int multiSample = glFrameBufferMultiSample;
  if (glFrameBuffer != NULL) {
    ret += glFrameBufferWidth * glFrameBufferHeight * sizeof(unsigned int) * multiSample;
    if (refreshMasked) ret += glFrameBufferWidth * glFrameBufferHeight * sizeof(unsigned int) * multiSample;
  }
  if (glDepthBuffer != NULL) ret += glFrameBufferWidth * glFrameBufferHeight * sizeof(float) * multiSample;
  if (glStencilBuffer != NULL) ret += glFrameBufferWidth * glFrameBufferHeight * sizeof(unsigned char) * multiSample;
  return ret;
}

void showDebugWindow() {
  if (!debugWindowOn) return;
  int additionalSize = 0;
  additionalSize += paintValues.allocatedSize * sizeof(float);
  additionalSize += paintBackground.allocatedSize * sizeof(unsigned int);
  additionalSize += quantization.allocatedSize * sizeof(unsigned short);

  ImGui::PushStyleColor(ImGuiCol_TitleBg,ImVec4(0,0.75,0.75,0.5));
  ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1,1,1,1));
  ImGui::Begin("Status window", &debugWindowOn, ImVec2(0,0),0.25,ImGuiWindowFlags_NoCollapse);
  ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0,0,0,1));
  ImGui::Text("Undo stack %.02fkb", undoStackBytes/1024.f);
  ImGui::Text("Layers %.02fkb", getLayersByteCount()/1024.f);
  ImGui::Text("Framebuffer %.02fkb",getWatcomGLBytes()/1024.f);
  ImGui::Text("Additional memory %.02fkb",additionalSize/1024.f);
  ImGui::Text("Maybe free memory %.04fmb", getAdditionalMemoryFreeBytes()/1024.f/1024.f);
  ImGui::Text("FPS %.02f (please use the newest DJGPP if that's a problem)",fps);
  ImGui::PopStyleColor();
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleColor();
}

String combined(const String &c) {
  return c;
}

char tempBuffer[256];

void toolTip(const char *fmt, ...) {
  if (!showToolTips) return;
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.0,0.0,0.0,1));
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
    ImGui::PopStyleColor();
    ImGui::EndTooltip();
  }
}

void toolTip2(const char *fmt, ...) {
  if (!showToolTips) return;
  ImGui::BeginTooltip();
  ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.0,0.0,0.0,1));
  va_list args;
  va_start(args, fmt);
  ImGui::TextV(fmt, args);
  va_end(args);
  ImGui::PopStyleColor();
  ImGui::EndTooltip();
}

float luminanceValues[3] = {0.2126,0.7152,0.0722}; //{0.25,0.6,.15};

// if you want to port Wypa to another platform than DOS, you could start here
//--
#ifdef OTHERPLATFORM
#include <time.h>
#ifdef __DJGPP__
#include <sys/nearptr.h>
#endif // __DJGPP__
#include <dos.h>
#include <string.h>
unsigned short oldMode;
void setDosBiosGraphicsMode(unsigned short mode) {union REGS regs; memset(&regs,0,sizeof(regs)); regs.w.ax = mode; int386(0x10, &regs, &regs);}
unsigned short getDosBiosGraphicsMode() {union REGS regs; memset(&regs,0,sizeof(regs)); regs.w.ax = 0x0f00; int386(0x10, &regs, &regs); return regs.h.al;}
GLdouble glStartClockX;
GLdouble glSecondsX() {return (GLdouble)(clock() - glStartClockX) / CLOCKS_PER_SEC;}
GLvoid glSetTimeX(GLdouble seconds) {glStartClockX = clock() - seconds * CLOCKS_PER_SEC;}
#ifdef __WATCOMC__
GLuint int16_1x();
#pragma aux int16_1x="mov ah,0x01""int 0x16""jz do""mov eax,0""jmp dont""do:""mov eax,1""dont:" value[eax] modify[eax ebx ecx edx]
#endif
#ifdef __DJGPP__
GLuint int16_1x() {
  GLuint ret;
  __asm__ __volatile__("\tmovb $1,%%ah\n""\tint $0x16\n""\tjz 1f\n""\tmovl $0,%%eax\n""\tjmp 2f\n""1:\n""\tmovl $1,%%eax\n""2:\n" : "=a" (ret));
  return ret;
}
#endif
// some special keys like GL_VK_UP and so on must also be supported (look into hotkeys.hpp for instance)
GLushort glNextKeyX() {if (int16_1x()) return 0; union REGS regs; memset(&regs,0,sizeof(regs)); regs.w.ax = 0x0000; int386(0x16, &regs, &regs); int k = regs.w.ax; if ((k & 0xff) != 0x00) k &= 0xff; return (GLushort)k;}
GLvoid glSetMousePosX(GLint x, GLint y) {union REGS regs; memset(&regs,0,sizeof(regs)); regs.w.ax = 0x04; regs.w.cx = (uint16_t)(x & 0xffff); regs.w.dx = (uint16_t)(y & 0xffff); int386(0x33, &regs, &regs);}
int glMouseX_ = 0, glMouseY_ = 0, glLastMouseX_ = 0, glLastMouseY_ = 0;
GLvoid glNextMouseDeltaX(GLdouble *deltaX, GLdouble *deltaY) {
  union REGS regs; memset(&regs,0,sizeof(regs)); regs.w.ax = 0x03; int386(0x33, &regs, &regs); glLastMouseX_ = glMouseX_; glLastMouseY_ = glMouseY_;
  glMouseX_ = regs.w.cx & 0xffff; glMouseY_ = regs.w.dx & 0xffff; *deltaX = glMouseX_ - glLastMouseX_; *deltaY = glMouseY_ - glLastMouseY_;
  glSetMousePosX(320/2, 200/2); glMouseX_ = 320/2; glMouseY_ = 200/2;
}
GLushort glMouseButtonsX() {union REGS regs; memset(&regs,0,sizeof(regs)); regs.w.ax = 0x03; int386(0x33, &regs, &regs); return (GLushort)(regs.w.bx & 0xffff);}
GLvoid glSpecialKeysX(GLboolean *shiftKey, GLboolean *ctrlKey, GLboolean *altKey) {
  union REGS regs; memset(&regs,0,sizeof(regs)); regs.w.ax = 0x0200; int386(0x16, &regs, &regs); uint8_t keyState = (uint8_t)(regs.w.ax & 0xff);
  *shiftKey = ((keyState & 0x03) != 0) ? GL_TRUE : GL_FALSE; *ctrlKey = ((keyState & 0x04) != 0) ? GL_TRUE : GL_FALSE; *altKey = ((keyState & 0x08) != 0) ? GL_TRUE : GL_FALSE;
}
GLvoid glRefreshX() {
#ifdef __DJGPP__
    __djgpp_nearptr_enable(); for (int y = 0; y < 200; y++) { for (int x = 0; x < 320; x++) {
      unsigned int col = glFrameBuffer[x*glFrameBufferWidth/320+(y*glFrameBufferHeight/200)*glFrameBufferWidth]; unsigned int r = col & 255; unsigned int g = (col>>8) & 255; unsigned int b = (col >>16) & 255;
      ((unsigned char*)(0xa0000+__djgpp_conventional_base))[x+y*320] = (unsigned char)((r+g+b)/3/16+16);
    }} __djgpp_nearptr_disable();
#endif // __DJGPP__
#ifdef __WATCOMC__
    for (int y = 0; y < 200; y++) { for (int x = 0; x < 320; x++) {
      unsigned int col = glFrameBuffer[x*glFrameBufferWidth/320+(y*glFrameBufferHeight/200)*glFrameBufferWidth]; unsigned int r = col & 255; unsigned int g = (col>>8) & 255; unsigned int b = (col >>16) & 255;
      ((unsigned char*)0xa0000)[x+y*320] = (unsigned char)((r+g+b)/3/16+16);
    }}
#endif // __WATCOMC__
}
unsigned int *frameBufferX = NULL;
float *depthBufferX = NULL;
GLvoid glDoneX() {
  setDosBiosGraphicsMode(oldMode); 
  if (frameBufferX != NULL) {free(frameBufferX); frameBufferX = NULL;}
  if (depthBufferX != NULL) {free(depthBufferX); depthBufferX = NULL;}
}
//
bool initOtherPlatform() {
  glSetSecondsCallback(glSecondsX);
  glSetSetTimeCallback(glSetTimeX);
  glSetNextKeyCallback(glNextKeyX);
  glSetNextMouseDeltaCallback(glNextMouseDeltaX);
  glSetMouseButtonsCallback(glMouseButtonsX);
  glSetSpecialKeysCallback(glSpecialKeysX);
  glSetRefreshNotification(glRefreshX);
  glSetDoneNotification(glDoneX);
  int w = main_window_xres; int h = main_window_yres;
  frameBufferX = (unsigned int*)malloc(w*h*sizeof(unsigned int)); depthBufferX = (float*)malloc(w*h*sizeof(float));
  glDirect(frameBufferX, depthBufferX, NULL, w, h); oldMode = getDosBiosGraphicsMode(); setDosBiosGraphicsMode(0x13);
  return true;
}
#endif //OTHERPLATFORM
//--
