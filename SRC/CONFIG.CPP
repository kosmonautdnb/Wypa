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
int version = 50;
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
  for (int i = 0; i < canvas.size(); i++) {
    for (int j = 0; j < canvas[i]->layers.size(); j++) {
      ret += canvas[i]->layers[j]->byteCount();
    }
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
  ImGui::Begin("Debug window", NULL, ImVec2(main_window_xres/3,main_window_yres/3),0.25);
  ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0,0,0,1));
  ImGui::Text("Undo stack %.02fkb", undoStackBytes/1024.f);
  ImGui::Text("Layers %.02fkb", getLayersByteCount()/1024.f);
  ImGui::Text("Framebuffer %.02fkb",getWatcomGLBytes()/1024.f);
  ImGui::Text("Additional memory %.02fkb",additionalSize/1024.f);
  ImGui::Text("Maybe free memory %.02fkb", getAdditionalMemoryFreeBytes()/1024.f);
  ImGui::Text("FPS %.02f",fps);
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
