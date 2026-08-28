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
#define _CRT_SECURE_NO_WARNINGS
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
bool captureMouse = true;
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
#endif // __WATCOMC__
#ifdef __DJGPP__        
  __dpmi_free_mem_info memInfo;
  __dpmi_get_free_memory_information(&memInfo);
  int mem = 0;
  if (memInfo.largest_available_free_block_in_bytes  != 0xffffffff) mem = memInfo.largest_available_free_block_in_bytes;
  return mem;
#endif // __DJGPP__
#ifdef NOT_DOS
  return 256*1024*1024;
#endif // NOT_DOS
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

int getUsedTextureCount() {
  int count = 0; for (int i = 0; i < GLMAXTEXTURES; i++) if (glGetTexturePointer(i) != NULL) count++;
  return count;
}

int getUsedTextureBytes() {
  int bytes = 0; for (int i = 0; i < GLMAXTEXTURES; i++) if (glGetTexturePointer(i) != NULL) bytes += glGetTextureWidth(i)*glGetTextureHeight(i)*4;
  return bytes;
}

void showDebugWindow() {
  if (!debugWindowOn) return;
  int additionalSize = 0;
  additionalSize += (int)(paintValues.allocatedSize * sizeof(float));
  additionalSize += (int)(paintBackground.allocatedSize * sizeof(unsigned int));
  additionalSize += (int)(quantization.allocatedSize * sizeof(unsigned short));

  ImGui::PushStyleColor(ImGuiCol_TitleBg,ImVec4(0,0.75,0.75,0.5));
  ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1,1,1,1));
  ImGui::Begin("Status window", &debugWindowOn, ImVec2(0,0),0.25,ImGuiWindowFlags_NoCollapse);
  ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0,0,0,1));
  ImGui::Text("Undo stack %.02fkb", undoStackBytes/1024.f);
  ImGui::Text("Layers %.02fkb", getLayersByteCount()/1024.f);
  ImGui::Text("Framebuffer %.02fkb",getWatcomGLBytes()/1024.f);
  ImGui::Text("Additional memory %.02fkb",additionalSize/1024.f);
  ImGui::Text("Maybe free memory %.04fmb", getAdditionalMemoryFreeBytes()/1024.f/1024.f);
  ImGui::Text("FPS %.02f (please compile with the newest DJGPP if that's a problem)",fps);
  ImGui::Text("Used WatcomGL textures: %d of %d\n", getUsedTextureCount(), GLMAXTEXTURES);
  ImGui::Text("WatcomGL texture memory: %.02fmb\n", (float)getUsedTextureBytes()/1024.f/1024.f);
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

float luminanceValues[3] = {0.2126f,0.7152f,0.0722f}; //{0.25,0.6,.15};

// if you want to port Wypa to another platform than DOS, you could start here
// you need the #define __GLDISABLEDOSFUNCTIONS__ 1 at compiletime like gcc -D__GLDISABLEDOSFUNCTIONS__
// and maybe you need to update STL/DOS.HPP for dos functions.
// maybe you need to update case sensitivity for includes, maybe #include "STL/DOS.HPP" has to get #include "stl/dos.hpp"
//--
#ifdef OTHERPLATFORM
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <mmsystem.h>
static BITMAPINFO bmi = { {sizeof(BITMAPINFOHEADER),(LONG)main_window_xres,-(LONG)main_window_yres,1,32,BI_RGB,0,0,0,0,0},{0,0,0,0} };
static DEVMODE screenSettings = { {0},
    #if _MSC_VER < 1400
    0,0,148,0,0x001c0000,{0},0,0,0,0,0,0,0,0,0,{0},0,32,XRES,YRES,0,0,      // Visual C++ 6.0
    #else
    0,0,156,0,0x001c0000,{0},0,0,0,0,0,{0},0,32,main_window_xres,main_window_yres,{0}, 0,           // Visuatl Studio 2005
    #endif
    #if(WINVER >= 0x0400)
    0,0,0,0,0,0,
    #if (WINVER >= 0x0500) || (_WIN32_WINNT >= 0x0400)
    0,0
    #endif
    #endif
};
#include <time.h>
#include <conio.h>
unsigned int* frameBufferX = NULL;
float* depthBufferX = NULL;
GLdouble glStartClockX;
GLdouble glSecondsX() { return (GLdouble)(clock() - glStartClockX) / CLOCKS_PER_SEC; }
GLvoid glSetTimeX(GLdouble seconds) { glStartClockX = clock() - seconds * CLOCKS_PER_SEC; }
#define KEYBOARDBUFFERSIZE 100
unsigned int keyboardBuffer[KEYBOARDBUFFERSIZE] = { { 0 } };
unsigned int keyboardBufferWrite = 0;
unsigned int keyboardBufferRead = 0;
GLushort glNextKeyX() {
    if (keyboardBufferWrite != keyboardBufferRead) {
        unsigned int key = keyboardBuffer[keyboardBufferRead % KEYBOARDBUFFERSIZE];
        keyboardBufferRead = (keyboardBufferRead + 1) % KEYBOARDBUFFERSIZE;
        return (unsigned short)key;
    }
    return 0;
}
double mouseDeltaX_X = 0; double mouseDeltaY_X = 0;
GLvoid glNextMouseDeltaX(GLdouble* deltaX, GLdouble* deltaY) { 
    *deltaX = mouseDeltaX_X; *deltaY = mouseDeltaY_X;
    mouseDeltaX_X = mouseDeltaY_X = 0;
}
GLushort glMouseButtonsX() {
    GLushort ret = GetAsyncKeyState(VK_LBUTTON) & 0x8000 ? GL_MOUSE_BUTTON_LEFT : 0;
    ret |= GetAsyncKeyState(VK_MBUTTON) & 0x8000 ? GL_MOUSE_BUTTON_MIDDLE : 0;
    ret |= GetAsyncKeyState(VK_RBUTTON) & 0x8000 ? GL_MOUSE_BUTTON_RIGHT : 0;
    return ret;
}
GLvoid glSpecialKeysX(GLboolean* shiftKey, GLboolean* ctrlKey, GLboolean* altKey) {
if (shiftKey != NULL) { *shiftKey = (GetAsyncKeyState(VK_LSHIFT) | GetAsyncKeyState(VK_RSHIFT)) & 0x8000 ? GL_TRUE : GL_FALSE; }
if (ctrlKey != NULL) { *ctrlKey = (GetAsyncKeyState(VK_LCONTROL) | GetAsyncKeyState(VK_RCONTROL)) & 0x8000 ? GL_TRUE : GL_FALSE; }
if (altKey != NULL) { *altKey = (GetAsyncKeyState(VK_LMENU) | GetAsyncKeyState(VK_RMENU)) & 0x8000 ? GL_TRUE : GL_FALSE; }
}
HDC hDC;
__inline unsigned int ___swapRB(unsigned int k) {
    return ((k & 0x00ff0000) >> 16) | ((k & 0x000000ff) << 16) | (k & 0xff00ff00);
}
bool lastMX = captureMouse;
GLvoid glRefreshX() {
    unsigned int* blitti = new unsigned int[glFrameBufferWidth * glFrameBufferHeight];
    for (int i = 0; i < glFrameBufferWidth * glFrameBufferHeight; i++) blitti[i] = ___swapRB(glFrameBuffer[i]);
    StretchDIBits(hDC, 0, 0, glFrameBufferWidth, glFrameBufferHeight, 0, 0, glFrameBufferWidth, glFrameBufferHeight, blitti, &bmi, DIB_RGB_COLORS, SRCCOPY);
    delete[] blitti;
    MSG msg;
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        //done |= (GetAsyncKeyState(VK_ESCAPE) != 0);
        //if (msg.message == WM_QUIT) ..
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    Sleep(10);
    if (lastMX != captureMouse) { lastMX = captureMouse; ShowCursor(!lastMX); }
}
GLvoid glDoneX() {
    if (frameBufferX != NULL) { free(frameBufferX); frameBufferX = NULL; }
    if (depthBufferX != NULL) { free(depthBufferX); depthBufferX = NULL; }
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CHAR) {
        keyboardBuffer[keyboardBufferWrite % KEYBOARDBUFFERSIZE] = (unsigned int)wParam;
        keyboardBufferWrite = (keyboardBufferWrite + 1) % KEYBOARDBUFFERSIZE;
    }
    if (msg == WM_INPUT) {
        RAWINPUT raw;
        UINT size = sizeof(raw);
        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) == size) {
            if (raw.header.dwType == RIM_TYPEMOUSE) {
                LONG dx = raw.data.mouse.lLastX;
                LONG dy = raw.data.mouse.lLastY;
                mouseDeltaX_X = dx;
                mouseDeltaY_X = dy;
                if (IsWindow(hwnd)) { // not reliable enough the cursor could move out of the window anyways
                    RECT rect;
                    if (GetClientRect(hwnd, &rect)) {
                        int centerX = (rect.right - rect.left) / 2;
                        int centerY = (rect.bottom - rect.top) / 2;
                        POINT pt = { centerX, centerY };
                        if (ClientToScreen(hwnd, &pt)) {
                            if (captureMouse) SetCursorPos(pt.x, pt.y);
                        }
                    }
                }
            }
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
bool initOtherPlatform() {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"WypaWNDClass";
    RegisterClass(&wc);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN)-100; int screenHeight = GetSystemMetrics(SM_CYSCREEN)-100;
    main_window_xres = screenWidth; main_window_yres = screenHeight;
    bmi.bmiHeader.biHeight = -screenHeight; bmi.bmiHeader.biWidth = screenWidth;
    screenSettings.dmPelsWidth = screenWidth;screenSettings.dmPelsHeight= screenHeight;
    aspectRatio = (double)main_window_xres / main_window_yres;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
    HWND hwnd = CreateWindowEx(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, L"WypaWNDClass", L"Wypa", WS_VISIBLE | WS_POPUP | WS_BORDER, 0, 0, main_window_xres, main_window_yres, NULL, NULL, wc.hInstance, NULL);
    hDC = GetDC(hwnd);
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hwnd;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
    lastMX = captureMouse; ShowCursor(!lastMX);
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    glSetSecondsCallback(glSecondsX);
    glSetSetTimeCallback(glSetTimeX);
    glSetNextKeyCallback(glNextKeyX);
    glSetNextMouseDeltaCallback(glNextMouseDeltaX);
    glSetMouseButtonsCallback(glMouseButtonsX);
    glSetSpecialKeysCallback(glSpecialKeysX);
    glSetRefreshNotification(glRefreshX);
    glSetDoneNotification(glDoneX);
    int w = main_window_xres; int h = main_window_yres;
    frameBufferX = (unsigned int*)malloc(w * h * sizeof(unsigned int)); depthBufferX = (float*)malloc(w * h * sizeof(float));
    glDirect(frameBufferX, depthBufferX, NULL, w, h);
    return true;
}

#else // _WIN32

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
#endif //_WIN32
#endif //OTHERPLATFORM
//--
