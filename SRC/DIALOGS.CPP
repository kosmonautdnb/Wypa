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
#include "IMGUI/IMGUIGL.HPP"
#include "CONFIG.HPP"
#include "DIALOGS.HPP"
#include "LAYERS.HPP"
#include "SVG.HPP"
#include "SERIALZE.HPP"
#include "HOTKEYS.HPP"
#include "UNDO.HPP"
#include "FILTER.HPP"
#include "COLORFLD.HPP"
#include "TOOLBAR.HPP"
#include "GALLERY.HPP"
#include "TEXT.HPP"
#include <math.h>

bool reOpen = false;
bool dialogNoMove = false;

#define DIALOGSTART(__name__) static int keyboardFocusNew = 0; UNUSED(keyboardFocusNew); static bool openItNextTime = false;  if (openIt) {openItNextTime = true; keyboardFocusNew = 2; return;}  if (openItNextTime) {openItNextTime = false; ImGui::OpenPopup(__name__); } ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0,0,0,1)); ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(1,1,1,1); if (ImGui::BeginPopupModal(__name__,NULL,ImGuiWindowFlags_AlwaysAutoResize|(dialogNoMove?ImGuiWindowFlags_NoMove:0))) { ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(0,0,0,1); ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.4,0.4,0.4,1));disableNativeInput();
#define DIALOGEND() ImGui::PopStyleColor(); ImGui::EndPopup();} ImGui::PopStyleColor();
#define DIALOGKEYSHERE() {if (keyboardFocusNew>0) {keyboardFocusNew--;ImGui::SetKeyboardFocusHere();}}

bool roundButton(const char *name, bool enabled);

unsigned int previewTexture = 0;
const unsigned int previewWidth = 64;
const unsigned int previewHeight = 64;
unsigned int preview[previewWidth*previewHeight];
double previewAspect = 1;

String dialogShortPath;
String dialogLongFileName;


void updatePreview() {
  glBindTexture(GL_TEXTURE_2D,previewTexture);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, previewWidth, previewHeight, 0, GL_RGBA, GL_BYTE, preview);
  glBindTexture(GL_TEXTURE_2D,0);
}

void clearPreview() {
  for (int y = 0; y < (int)previewHeight; y++) {
    for (int x = 0; x < (int)previewWidth; x++) {
      preview[x+y*previewWidth] = 0x40000000;
    }
  }
  previewAspect = 1;
  updatePreview();
}

void noUndoMessage() {
  static double k = 0; k += timeDelta*2; k = fmod(k,100);
  int t = k;
  ImGui::TextColored(ImVec4(1,0,0,(t & 1)*0.5+0.5),"Be aware that there is no <undo> for this!!");
}

GLuint dialogPreviewTexture[2] = {0,0};
int dialogPreviewTextureWidth = 100;
int dialogPreviewTextureHeight = 100;

GLuint dotTextures16x16[4] = {0,0,0,0};

void initDialogs() {
  glGenTextures(1,&previewTexture);
  glBindTexture(GL_TEXTURE_2D,previewTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  clearPreview();
  glBindTexture(GL_TEXTURE_2D,0);

  for (int i = 0; i < 2; i++) {
    glGenTextures(1,&dialogPreviewTexture[i]);
    glBindTexture(GL_TEXTURE_2D,dialogPreviewTexture[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D,0);
  }

  unsigned int *dots = new unsigned int[16*16];
  for (int i2 = 0; i2 < 4; i2++) {
    glGenTextures(1,&dotTextures16x16[i2]);
    glBindTexture(GL_TEXTURE_2D,dotTextures16x16[i2]);
    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < 16; x++) {
        bool on = ((x & 1) + (y & 1)*2) == i2;
        dots[x + y*16] = on ? 0xffffffff : 0x00000000;
      }
    }
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, 16, 16, 0, GL_RGBA, GL_BYTE, dots);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D,0);
  }
  delete[] dots;
}

void freeDialogs() {
  glDeleteTextures(1, &previewTexture);
  for (int i = 0; i < 2; i++) {
    glDeleteTextures(1, &dialogPreviewTexture[i]);
  }
  for (int i2 = 0; i2 < 4; i2++) {
    glDeleteTextures(1,&dotTextures16x16[i2]);
  }
}

bool dialogLoadSaveOff = false;

void dialogs() {
  dialogCanvas(false);
  dialogExpandShrinkSelection(false);
  dialogImageResize(false);
  if (!dialogLoadSaveOff) {dialogLoadSave(false,DIALOG_NONE);} dialogLoadSaveOff = false;
  dialogDeletePicture(false);
  dialogAddLayer(false);
  dialogDeleteLayers(false);
  dialogSelectByAlpha(false);
  dialogBakeLayers(false);
  dialogNewImage(false);
  dialogGetLayersFromPicture(false);
  dialogUpdateLayer(false);
  dialogAbout(false);
  dialogDocumentation(false);
  dialogNewHere(false);
  dialogWhatFor(false);
  dialogSuite(false);
  dialogExit(false);
  dialogDuplicate(false);
  dialogMergeDown(false);
  dialogInsertSVG(false,0,0);
  dialogExportLayersRaw(false);
  dialogPictureInfo(false);
  dialogGradiationCurves(false);
  dialogColorBalance(false);
  dialogBlur(false);
  dialogGlow(false);
  dialogEmboss(false);
  dialogHotkeys(false);
  dialogColorMatrix(false);
  dialogQuantizePalette(false);
  dialogExtrudeSelection(false);
  dialogKeying(false);
  dialogMonitorGamma(false);
  dialogInsertTileMapLayer(false);
  dialogImportFolder(false);
  dialogPixelArtInflate(false);
  dialogClahe(false);
  dialogCheckerboard(false);
  dialogFontGlyphs(false, NULL);
  dialogCopyPasteLoadedText(false);
  dialogEditText(false);
  dialogDisplacementMap(false);
}

void drawDialogs() {
  ImGui::SetNextWindowPos(ImVec2(main_window_xres-64,0));
  ImGui::Begin("DRAWDIALOGS", NULL, ImVec2(0,0),0.0,FROZENWINDOW);
  dialogs();
  ImGui::End();
  errorWindow(false);
}

void dialogAbout(bool openIt) {
  String buildType = "Unknown";
#ifdef __WATCOMC__
  buildType = "WATCOMC"+String::fromInt(__WATCOMC__);
#endif
#ifdef __DJGPP__
  buildType = "DJGPP"+String::fromInt(__DJGPP__);
#endif
  DIALOGSTART("About")
  ImGui::Text(
    "WYPA - A SIMPLE PAINT PROGRAM FOR DOS\n"
    "------------------------------------\n"
    "Coded in early 2026 including\n"
    "all design by\n"
    "Stefan Mader of Puls4r\n"
    "------------------------------------\n"
    "Icons (license CC0) by Rainer Muehr\n"
    "------------------------------------\n"
    "Source is released under the most\n"
    "common: MIT License. (open source)\n"
    "------------------------------------\n"
    "Build date: " __DATE__ "\n"
    "Build time: " __TIME__ "\n"
    "Build type: %s\n"
    "Version: %d.%02d\n"
    "------------------------------------\n"
    "Wypa is not coded very \"good\".\n"
    "It's just a proof of concept, since there\n"
    "where almost no other options to paint\n"
    "in high resolutions with true color on DOS.\n"
    "------------------------------------\n"
    "This program is freeware.\n"
  , buildType.c_str(), version / 100, version % 100
  );
  ImGui::Separator();
  if (ImGui::Button("Close##DIALOGABOUTCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

#ifdef INTERNALDOCU
#define _N_ "_"
const char *docuString = 
#include "../DATA/DOCU.DC2"
;
#endif

void dialogDocumentation(bool openIt) {
#define FILTERNAMEMAX2 16
  static Array<unsigned char> docu;
  static char filter[FILTERNAMEMAX2];
  static int index = 0;
  static Array<bool> lineIsOpen;
  if (openIt) {
    sprintf(filter,"%s","");
    lineIsOpen.clear();
#ifndef INTERNALDOCU
    if (docu.empty()) {
      getFileArray(&docu,fromLocalFileName("./DATA/DOCU.DC2"));
      char lc = 0;
      bool inLine = false;
      for (int i = 0; i < (int)docu.size(); i++) {
        char c = (char)docu[i];
        if (c == '"' && lc != '\\') {
          inLine = !inLine;
          docu.erase(i,1);
          i--;
        } else {
          if (c == '"' && lc == '\\') {
            if (i > 0) {i--; docu.erase(i,1);}
          } else {
            if (c == '_' && lc == 'N') {
            } else {
              if (!inLine) {
                docu.erase(i,1);
                i--;
              }
            }
          }
        }
        lc = c;
      }
    }
#endif
    index = 0;
  }
#ifndef INTERNALDOCU
  if (docu.empty()) {
    DIALOGSTART("Tiny documentation not found")
    ImGui::TextColored(ImVec4(1,0,0,1),"The documentation was not found in path");
    ImGui::TextColored(ImVec4(1,0,0,1),"%s",fromLocalFileName("./DATA/DOCU.DC2").c_str());
    DIALOGEND()
    return;
  }
  const char *docuString = (char*)(&(docu[0]));
#endif

  DIALOGSTART("Tiny documentation")
  bool toFirst = false;
  SVG *svg = &svgs["search"];ImGui::Image((ImTextureID)svg->glHandle,ImVec2(svg->w,svg->h));ImGui::SameLine(); DIALOGKEYSHERE(); if (ImGui::InputText("##DIALOGDOCUMENTATIONFILTER",filter,FILTERNAMEMAX2)) {toFirst = true; index = 0;}

  ImGui::SameLine();
  if (ImGui::Button("Collapse all##DIALOGDOCUMENTATIONCOLLAPSEALL")) {
    for (int i = 0; i < (int)lineIsOpen.size(); i++) lineIsOpen[i] = false;
  }
  ImGui::SameLine();
  if (ImGui::Button("Open all##DIALOGDOCUMENTATIONOPENALL") || strlen(filter) != 0) {
    for (int i = 0; i < (int)lineIsOpen.size(); i++) lineIsOpen[i] = true;
  }

  int width = 101*7;
  int height = main_window_yres/2;
  ImGui::Text("The documentation maybe, or is more or less incorrect, but wth.");
  ImGui::Text("This docu is done by a german, so don't be too angry");
  ImGui::Text("about the style/content.");
  ImGui::Separator();
  ImGui::BeginChild("DIALOGDOCUMENTATIONCHILD",ImVec2(width,height));
  if (currentKey == GL_VK_PAGEDOWN) ImGui::SetScrollY(ImGui::GetScrollY()+height*0.75);
  if (currentKey == GL_VK_PAGEUP) ImGui::SetScrollY(ImGui::GetScrollY()-height*0.75);
  if (currentKey == GL_VK_F3) {index++; toFirst = true;}
  const char *s = docuString;
  String currentLine = "";
  int indent = 0;
  int line = 0;
  Array<bool> openStack; 
  openStack.push_back(true);
  String filterLook = toLower(filter);
  int index2 = 0;
  while(*s != 0) {
    char c = *s;
    s++;
    if (c == '_') {
      if (line+1>(int)lineIsOpen.size()) {lineIsOpen.resize(line+1); lineIsOpen[line] = false;}
      bool thisIsOpen = openStack.back();
      if (currentLine.empty()) currentLine = " "; // for cursor
      if (!currentLine.empty()) {
        bool found = false; if ((!filterLook.empty()) && toLower(currentLine).find(filterLook)>=0) found = true;
        char d = currentLine.last(1)[0];
        bool isExpanding = false;
        ImVec2 cursor0 = ImGui::GetCursorScreenPos(); ImVec2 cursor = cursor0;
        ImGui::Text(""); ImGui::SetCursorScreenPos(cursor0); // for scrolling;
        cursor.x += indent * 10;
        //ImVec2 cursor1 = cursor;
        if (d == '{') {
          currentLine = currentLine.substr(0,currentLine.length()-1);
          indent++; openStack.push_back(lineIsOpen[line]);
          isExpanding = true;
        }
        if (d == '}') {
          currentLine = currentLine.substr(0,currentLine.length()-1);
          indent--; openStack.pop_back();
          if (currentLine.empty()) continue;
        }
        float expandSize = 1;
        int siz = ImGui::GetWindowFontSize()*(isExpanding?expandSize:1);
        ImVec2 a = ImGui::CalcTextSize(currentLine.c_str());
        a.x *= expandSize; a.y *= expandSize; a.x += cursor.x; a.y += cursor.y;

        if (thisIsOpen) {
          if (isExpanding) {
            ImGui::GetWindowDrawList()->AddRectFilled(cursor,a,0xff808080);
          }
          if (found) {
            ImGui::GetWindowDrawList()->AddRectFilled(cursor,a,index2==index?0xff8080ff:0xff0000ff);
          }
          ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowFont(),siz,ImVec2(cursor.x,cursor.y),0xff000000, currentLine.c_str());
          if (isExpanding) {
            if (ImGui::IsMouseHoveringRect(cursor,a)) {
              ImGui::GetWindowDrawList()->AddRectFilled(cursor,a,0x20ffffff);
              if (ImGui::IsMouseClicked(0)) {
                lineIsOpen[line] = !lineIsOpen[line];
              }
            }
          }
          cursor.x = cursor0.x; cursor.y = cursor0.y + siz; ImGui::SetCursorScreenPos(cursor);
        }
        if (found) {
          if (index2==index) {
            if (toFirst) {toFirst = false; ImGui::SetScrollPosHere();} 
          } 
          index2++;
        }
      }
      currentLine = "";
      line++;
    } else {
      currentLine += String::fromChar(c);
    }
  }
  ImGui::EndChild();
  ImGui::Separator();
  if (ImGui::Button("Close##DIALOGDOCUMENTATIONCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogNewHere(bool openIt) {
  DIALOGSTART("If you are new here")
  ImGui::Text(
    "Wypa is pretty different to other\n"
    "paint programs. (e.g. undo is not possible anywhere)\n"
    "\n"
    "The most distinctive feature is,\n"
    "that layers mostly keep their\n"
    "original width and height.\n"
    "\n"
    "And that layers have each their\n"
    "own selection mask.\n"
    "\n"
    "So the most important thing to know is:\n"
    "You can press <ctrl> at switching of the active\n"
    "layer to copy the current layers selection to\n"
    "the new layer.\n"
    "\n"
    "Secondly use the '!' button to fit the layers to the picture (e.g. before merging).\n"
    "Merged layers have no undo.\n"
    "\n"
    "In the move tool you can use the right\n"
    "mouse button to rotate the layers.\n"
    "And the ctrl key to change the scaling of the layers.\n"
    "\n"
    "You have to load a font in order to bake texts.\n"
    "\n"
    "When selecting a polygon you can use the right mouse\n"
    "button to add a point (whilst having the left button pressed).\n"
    "\n"
    "If layers are baked, they lose their rotation and scaling.\n"
    "\n"
    "The alpha mode may get outlines/seams around the pixels.\n"
    "\n"
    "Please use a good DPMI host like the one from\n"
    "SBEMU 1.0.0-beta.5(2024-08-18) hdpmi32i.exe\n"
    "it provides around 4GB in WatcomC and DJGPP.\n"
    "\n"
    "Currently there is no forum for pictures that\n"
    "are done with Wypa or providing help on painting.\n"
    "\n"
    "Most color operations are done with the Postprocessing tool.\n"
    "\n"
    "When painting you may use the right mouse to get the current layer color.\n"
    "When painting with a pen you can press <ctrl> to erase instead of paint.\n"
    "\n"
    "You can use <ctrl> and <shift> (or both) when selecting things.\n"
    "In selection mode you can also use the right mouse to move the selection.\n"
    "\n"
    "You may have to use \"convert alpha to premul\" after loading external images.\n"
    "\n"
    "You can move the mouse by one pixel with the Numblock.\n"
    "In move mode you can also move with the Numblock.\n"
    "\n"
    "<Alt> and mouse pans the image plane.\n"
  );
  ImGui::Separator();
  if (ImGui::Button("Close##DIALOGNEWHERECLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogWhatFor(bool openIt) {
  DIALOGSTART("What's Wypa for?")
  ImGui::Text(
    "Wypa is just a simple paint program\n"
    "intended to create games on DOS.\n"
    "Mainly it's destination was pixelling for\n"
    "games, like doing tilemaps and simple\n"
    "graphics manipulations.\n"
    "On DOS there aren't other paint programs\n"
    "that do all of this kind of stuff currently.\n"
    "So be aware that Wypa is not a replacement\n"
    "for a propper graphics suite on other operating\n"
    "systems.\n"
  );
  ImGui::Separator();
  if (ImGui::Button("Close##DIALOGWHATFORCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogSuite(bool openIt) {
  DIALOGSTART("Puls4r game creation suite")
  ImGui::Text(
    "This little freeware program is brought to you by\n"
    "Puls4r\n"
    "\n"
    "Puls4r started originally as C16 and C64 and Plus/4\n"
    "game creation company.\n"
    "\n"
    "With the need to migrate to FreeDOS from Windows 10\n"
    "we where missing the tools available on Windows.\n"
    "Mainly the tools to create games.\n"
    "\n"
    "Currently there are two tools for the creation of games\n"
    "available for DOS from Puls4r.\n"
    "\n"
    "The first one is \"SWIFE\" a 3D object placement tool.\n"
    "With \"SWIFE\" you can take 3D objects from the internet\n"
    "and create a 3D scene by placing and manipulating these\n"
    "objects. By this you can create 3D worlds for your DOS\n"
    "3D games.\n"
    "\n"
    "The second one is \"WYPA\" a multi feature paint program\n"
    "for DOS. It's the tool you are currently running on your\n"
    "computer.\n"
    "\n"
    "It's highly likely that more DOS based game creation tools\n"
    "will follow in this \"Suite\".\n"
    "\n"
    "Puls4r (coder: Stefan Mader) also provided WatcomGL.\n"
    "A basic 3D graphics library for DJGPP/WATCOMC which let's\n"
    "you do most of the stuff early OpenGLs on other platforms\n"
    "where able to do. It's performant and is used as base for\n"
    "our programs like \"WYPA\"\n"
    "\n"
    "There is also a WatcomLibs project on github where you can\n"
    "find many libraries to do graphics/sound/fileformat etc..\n"
    "stuff on DOS/FreeDOS.\n"
  );
  ImGui::Separator();
  if (ImGui::Button("Close##DIALOGSUITECLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogMonitorGamma2(bool openIt) {
  DIALOGSTART("Monitor calibration")

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1,1,1,1)); 
  dialogLoadSaveOff = true; dialogLoadSave(false,DIALOG_NONE);
  ImGui::PopStyleColor();

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,1));
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2,0.2,0.2,1));
  ImGui::PushItemWidth(7*9);

  bool updateIt = false;

  ImGui::Separator();
  ImGui::Text("Monitor gamma"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGMONITORGAMMAGAMMA",&monitorGamma)) {updateIt = true;} else toolTip("The monitors luminance curve"); if (monitorGamma<0.0001) monitorGamma = 0.0001; if (monitorGamma>400) monitorGamma = 400;
  ImGui::Separator();
  ImGui::Text("Min red"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGMONITORGAMMAMINR",&monitorMin[0])) {updateIt = true;} else toolTip("The value where to start with red (so to say black)");
  ImGui::SameLine();ImGui::Text("Max red"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGMONITORGAMMAMAXR",&monitorMax[0])) {updateIt = true;} else toolTip("The value of full red visibility");
  ImGui::Text("Min green"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGMONITORGAMMAMING",&monitorMin[1])) {updateIt = true;} else toolTip("The value where to start with green (so to say black)");
  ImGui::SameLine();ImGui::Text("Max green"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGMONITORGAMMAMAXG",&monitorMax[1])) {updateIt = true;} else toolTip("The value of full green visibility");
  ImGui::Text("Min blue"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGMONITORGAMMAMINB",&monitorMin[2])) {updateIt = true;} else toolTip("The value where to start with blue (so to say black)");
  ImGui::SameLine();ImGui::Text("Max blue"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGMONITORGAMMAMAXB",&monitorMax[2])) {updateIt = true;} else toolTip("The value of full blue visibility");
  for (int k = 0; k < 3; k++) {
    if (monitorMin[k] < -512) monitorMin[k] = -512;
    if (monitorMin[k] > 256) monitorMin[k] = 256;
    if (monitorMax[k] < 0) monitorMax[k] = 0;
    if (monitorMax[k] > 512) monitorMax[k] = 512;
  }
  ImGui::Separator();
  if (ImGui::Button("Reset##DIALOGMONITORGAMMARESET")) {
    monitorMin[0] = 0; monitorMin[1] = 0; monitorMin[2] = 0;
    monitorMax[0] = 255; monitorMax[1] = 255; monitorMax[2] = 255;
    monitorGamma = 1;
    updateIt = true;
  }
  ImGui::SameLine();  
  if (ImGui::Button("Load##DIALOGMONITORGAMMALOAD")) {
    dialogLoadSave(true,DIALOG_LOADCALIBRATION);
  }
  ImGui::SameLine();  
  if (ImGui::Button("Save##DIALOGMONITORGAMMASAVE")) {
    dialogLoadSave(true,DIALOG_SAVECALIBRATION);
  }
  ImGui::Separator();

  if (updateIt) {
    updateIt = false;
    reGenMonitorGamma();
  }

  int border = 32;
  ImGui::BeginChild("DIALOGMONITORGAMMACHILD", ImVec2(16*16+border*2,7*16+border*2));
  ImVec2 cursor = ImGui::GetCursorScreenPos();
  cursor.x += border;
  cursor.y += border;
  for (int y = 0; y < 7; y++) {
    for (int x = 0; x < 16; x++) {
      if (x != 0) ImGui::SameLine();
      float k = (float)x / 15.f;
      float r = y == 1 ? 1 : 0;
      float g = y == 2 ? 1 : 0;
      float b = y == 0 ? 1 : 0;
      r = (r * (1-k)) + k;
      g = (g * (1-k)) + k;
      b = (b * (1-k)) + k;
      if (y >= 3) {
        r = y == 4 ? k : 0;
        g = y == 5 ? k : 0;
        b = y == 3 ? k : 0;
      }
      if (y == 6) {r = g = b = k;}
      ImVec2 cursor2 = cursor;
      cursor2.x += x * 16;
      cursor2.y += y * 16;
      for (int j = 0; j < 4; j++) {
        ImGui::SetCursorScreenPos(cursor2);
        unsigned int col0 = getUIntColor(ImVec4(r,g,b,1));
        unsigned int col = gammaCorrect(col0,j & 1, (j / 2) & 1);
        ImGui::Image((ImTextureID)dotTextures16x16[j], ImVec2(16,16), ImVec2(0,0), ImVec2(1,1), getImVec4Color(col));
        if ((j == 0) && ImGui::IsItemHovered()) toolTip("#%02x%02x%02x", col0 & 255, (col0>>8) & 255, (col0>>16) & 255);
      }
    }
  }
  ImGui::EndChild();

  if (ImGui::Button("Close##DIALOGMONITORGAMMACLOSE")) {
    ImGui::CloseCurrentPopup();
  }

  ImGui::PopItemWidth();
  ImGui::PopStyleColor();
  ImGui::PopStyleColor();
  DIALOGEND()
}

void dialogMonitorGamma(bool openIt) {
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,1)); 
  float before = ImGui::GetStyle().WindowFillAlphaDefault; ImGui::GetStyle().WindowFillAlphaDefault = 1.f;
  dialogMonitorGamma2(openIt);
  ImGui::GetStyle().WindowFillAlphaDefault = before; 
  ImGui::PopStyleColor();
}
void dialogInsertTileMapLayer(bool openIt) {

  static char layerName[LAYERNAMEMAX];
  static int w = 128, h = 128;
  if (openIt) {
    static unsigned short counter = 0; counter++;
    sprintf(layerName,"Tilemap %d",counter);
  }

  DIALOGSTART("Insert tilemap layer")
  noUndoMessage();
  ImGui::Text("Layer name"); ImGui::SameLine(); DIALOGKEYSHERE(); ImGui::InputText("##DIALOGINSERTTILEMAPLAYERNAME",layerName,LAYERNAMEMAX); toolTip("The name of the tilemap layer");
  ImGui::PushItemWidth(64+16*2);
  ImGui::Text("Tilemap width"); ImGui::SameLine(); ImGui::InputInt("##DIALOGINSERTTILEMAPWIDTH",&w); toolTip("The width in tiles of the tilemap layer");
  ImGui::Text("Tilemap height"); ImGui::SameLine(); ImGui::InputInt("##DIALOGINSERTTILEMAPHEIGHT",&h); toolTip("The height in tiles of the tilemap layer");
  ImGui::PopItemWidth();
  if (w < 1) w = 1;
  if (h < 1) h = 1;
  if (ImGui::Button("Add tilemap##DIALOGINSERTTILEMAPADD")) {
    int pos = 0;
    for (int i = 0; i < (int)currentCanvas->layers.size(); i++) {
      if (currentCanvas->layers[i]->state.active || currentCanvas->layers[i]->state.selected) {pos = i; break;}
    }
    Layer *l = new Layer();
    l->init(w,h);
    l->setLayerPos(currentCanvas->canvasX0, currentCanvas->canvasY0);
    l->state.lockBits = LOCK_TILEMAP;
    l->type = LAYERTYPE_TILEMAP;
    l->tileSourceLayerId = currentCanvas->getActiveLayer() != NULL ? currentCanvas->getActiveLayer()->id : 0;
    sprintf(l->name,layerName);
    currentCanvas->insertLayer(l,pos);
    currentCanvas->setActiveLayer(pos);
    shouldUpdateAll = true;
    switchTool(TOOL_TILEMAP);
    ImGui::CloseCurrentPopup();
  } else toolTip("Create the tilemap and insert it above the first selected layer");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGINSERTTILEMAPLAYERCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't create a tilemap here");
  DIALOGEND()
}

int newWidth = -1;
int newHeight = -1;
char newLayerName[LAYERNAMEMAX];

void dialogImportFolder(bool openIt) {

  static char layerName[LAYERNAMEMAX];
  static int w = 512, h = 512;
  if (openIt) {
    static unsigned short counter = 0; counter++;
    newWidth = -1;
    newHeight = -1;
    sprintf(layerName,"Folder %d",counter);
  }

  DIALOGSTART("Insert folder layer")
  noUndoMessage();
  ImGui::Text("Layer name"); ImGui::SameLine();  DIALOGKEYSHERE(); ImGui::InputText("##DIALOGIMPORTFOLDERLAYERNAME",layerName,LAYERNAMEMAX); toolTip("The name of the layer");
  ImGui::PushItemWidth(64+16*2);
  ImGui::Text("Layer width"); ImGui::SameLine(); ImGui::InputInt("##DIALOGIMPORTFOLDERWIDTH",&w); toolTip("The width of the layer where the imported pictures are on");
  ImGui::Text("Layer height"); ImGui::SameLine(); ImGui::InputInt("##DIALOGIMPORTFOLDERHEIGHT",&h); toolTip("The height of the layer where the imported pictures are on");
  ImGui::PopItemWidth();
  if (w < 1) w = 1;
  if (h < 1) h = 1;
  if (ImGui::Button("Insert folder layer##DIALOGIMPORTFOLDERINSERT")) {
    newWidth = w; newHeight = h; sprintf(newLayerName,"%s",layerName);
    ImGui::CloseCurrentPopup();
    dialogLoadSave(true,DIALOG_IMPORTFOLDER);
  } else toolTip("Next select a folder for importing");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGIMPORTFOLDERCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't create a new layer here");
  DIALOGEND()
}

void dialogPixelArtInflate(bool openIt) {
  const int SCALING_TYPE_COUNT = 3;
  const char *scalingTypeNames[SCALING_TYPE_COUNT] = {"Eagle 2x","Epx 2x","SuperXBR 2x"};
  const char *scalingTypeDescs[SCALING_TYPE_COUNT] = {"Color preserving","Color preserving","Antialiasing"};
  static int scalingType = 0;
  if (openIt) {
  }
  DIALOGSTART("Pixelart inflate")
  ImGui::PushItemWidth(7*15);
  ImGui::Text("Type");ImGui::SameLine();ImGui::Combo("##DIALOGPIXELARTINFLATETYPE",&scalingType, scalingTypeNames, SCALING_TYPE_COUNT); toolTip("The type of pixel art scaling to apply");
  ImGui::PopItemWidth();
  ImGui::Text("%s",scalingTypeDescs[scalingType]);

  if (ImGui::Button("Apply##DIALOGPIXELARTINFLATEAPPLY")) {
    switch(scalingType) {
      case 0: {filterEagle();} break;
      case 1: {filterEpx();} break;
      case 2: {filterSuperXBR();} break;
    }
    ImGui::CloseCurrentPopup();
  } else toolTip("Inflate the picture");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGPIXELARTINFLATECLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't scale the image");
  DIALOGEND()
}

void dialogClahe(bool openIt) {
  static float clahe = 1.5;
  static int cellSize = 16;
  if (openIt) {
  }
  DIALOGSTART("Clahe lighting")
  ImGui::PushItemWidth(7*9);
  ImGui::Text("Clahe damping"); ImGui::SameLine(); ImGui::InputFloat("##DIALOGCLAHEVALUE",&clahe,0); toolTip("Lesser values add more light"); if (clahe < 0.05) clahe = 0.05; if (clahe > 4) clahe = 4;
  ImGui::Text("Clahe cell size"); ImGui::SameLine(); ImGui::InputInt("##DIALOGCLAHECELLSIZE",&cellSize,0,0); toolTip("The less the more positional difference in lighting"); if (cellSize < 8) cellSize = 8;
  ImGui::PopItemWidth();
  if (ImGui::Button("Apply##DIALOGCLAHEAPPLY")) {
    filterClahe(clahe, cellSize);
    ImGui::CloseCurrentPopup();
  } else toolTip("Apply the Clahe lighting effect");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGCLAHECLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't apply Clahe");
  DIALOGEND()
}

void dialogCheckerboard(bool openIt) {
  static int cellWidth = 8;
  static int cellHeight = 8;
  static int offsetX = 0;
  static int offsetY = 0;
  if (openIt) {
  }
  DIALOGSTART("Checkerboard")
  ImGui::PushItemWidth(7*9);
  ImGui::Text("Cell width"); ImGui::SameLine(); ImGui::InputInt("##DIALOGCHECKERBOARDCELLWIDTH",&cellWidth,0,0); toolTip("One grid cell size in x"); if (cellWidth<1) cellWidth = 1;
  ImGui::SameLine();
  ImGui::Text("Cell height"); ImGui::SameLine(); ImGui::InputInt("##DIALOGCHECKERBOARDCELLHEIGHT",&cellHeight,0,0); toolTip("One grid cell size in y"); if (cellHeight<1) cellHeight = 1;
  ImGui::Text("Offset x"); ImGui::SameLine(); ImGui::InputInt("##DIALOGCHECKERBOARDCELLOFSX",&offsetX,0,0); toolTip("Grid position in x");
  ImGui::SameLine();
  ImGui::Text("Offset y"); ImGui::SameLine(); ImGui::InputInt("##DIALOGCHECKERBOARDCELLOFSY",&offsetY,0,0); toolTip("Grid position in y");
  ImGui::PopItemWidth();
  if (ImGui::Button("Apply##DIALOGCHECKERBOARDAPPLY")) {
    filterCheckerboard(cellWidth, cellHeight, offsetX, offsetY);
    ImGui::CloseCurrentPopup();
  } else toolTip("Create a checkerboard");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGCHECKERBOARDCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't create a checkerboard");
  DIALOGEND()
}

void drawGlyph(FontSource *fontSource, Array<unsigned int> &img,int w, int h, int glyph, int gx, int gy, int gw, int gh) {
  if (fontSource == NULL) return;
  unsigned short p[2]; p[0] = (unsigned short)glyph; p[1] = 0; int w2,h2;
  GLuint texId = fontSource->createTexture(p, gh, 1, w2, h2, 0xff000000);
  if (texId != 0) {
    for (int y = 0; y < gh; y++) {
      for (int x = 0; x < gw; x++) {
        int px = gx + x;
        int py = gy + y;
        if ((unsigned int)px < (unsigned int)w && (unsigned int)py < (unsigned int)h) {
          if (x < w2 && y < h2) {
            unsigned int col = *(glGetTexturePointer(texId)+x+y*w2);
            int grey = (col>>24) & 255;
            grey = 255-grey;
            img[px+py*w] = grey|(grey<<8)|(grey<<16)|0xff000000;
          }
          if (x == gw-1 || y == gh-1) {
            img[px+py*w] = 0xff808080;
          }
        }
      }
    }
    glDeleteTextures(1,&texId);
  }
}

void dialogFontGlyphs(bool openIt, const char *fontFileName) {
  static FontSource *fontSource = NULL;
  static int countX = 16;
  static bool rebuildGlyphs = false;
  static unsigned int glyphTexture = 0;
  static int glyphTextureWidth = 0;
  static int glyphTextureHeight = 0;
  if (openIt) {
    if (fontSource != NULL) {delete fontSource; fontSource = NULL;}
    if (glyphTexture != 0) {glDeleteTextures(1,&glyphTexture); glyphTexture = 0;}
    Array<unsigned char> *fontData = new Array<unsigned char>();
    getFileArray(fontData, fontFileName);
    if (!fontData->empty()) {
      fontSource = new FontSource(fontData, fontFileName);
    } else {
      delete fontData;
    }
    rebuildGlyphs = true;
  }
  DIALOGSTART("Font Glyphs")
  int w = main_window_xres*2/3;
  int h = main_window_yres*2/3;
  int glyphXRes = (w-16) / (countX > 0 ? countX : 1);
  int glyphYRes = glyphXRes;
  if (fontSource != NULL) {
    ImGui::BeginChild("DIALOGFONTGLYPHSCHILD",ImVec2(w,h));
    double aspect = 1.0/aspectRatio * main_window_xres / main_window_yres;
    ImVec2 c = ImGui::GetCursorScreenPos();
    ImGui::Image((ImTextureID)glyphTexture, ImVec2(glyphTextureWidth, glyphTextureHeight/aspect));
    int count = 0;
    for (int k = 0; k < 0x10000; k++) {
      if (fontSource->hasGlyph((unsigned short)k)) {
        int cx = count % countX;
        int cy = count / countX;
        int sx = cx * glyphXRes;
        int sy = ((cy+1) * glyphYRes)/aspect-13;
        sx += c.x; sy += c.y;
        if (sx >= 0 && sy >= 0 && sx < (int)main_window_xres && sy < (int)main_window_yres) {
          ImGui::SetCursorScreenPos(ImVec2(sx,sy));
          ImGui::TextColored(ImVec4(1,0,0,0.75),"^%04x",k);
        }
        count++;
      }
    }
    ImGui::EndChild();
  } else {
    ImGui::TextColored(ImVec4(1,0,0,1),"Couldn't find font");
  }
  if (rebuildGlyphs) {
    rebuildGlyphs = false;
    int glyphCount = 0;
    for (int i = 0; i < 0x10000; i++) {if (fontSource->hasGlyph((unsigned short)i)) glyphCount++;}
    glyphTextureWidth = glyphXRes * countX;
    int countY = (glyphCount + countX - 1) / countX;
    glyphTextureHeight = glyphYRes * countY;
    Array<unsigned int> graphic; graphic.resize(glyphTextureWidth * glyphTextureHeight);
    for (int j = 0; j < glyphTextureWidth*glyphTextureHeight; j++) {
      graphic[j] = 0xffffffff;
    }
    int count = 0;
    for (int k = 0; k < 0x10000; k++) {
      if (fontSource->hasGlyph((unsigned short)k)) {
        int cx = count % countX;
        int cy = count / countX;
        drawGlyph(fontSource,graphic,glyphTextureWidth,glyphTextureHeight,k,cx*glyphXRes,cy*glyphYRes,glyphXRes,glyphYRes);
        count++;
      }
    }
    if (glyphTexture != 0) {glDeleteTextures(1,&glyphTexture); glyphTexture = 0;}
    glGenTextures(1,&glyphTexture);
    glBindTexture(GL_TEXTURE_2D,glyphTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, glyphTextureWidth, glyphTextureHeight, 0, GL_RGBA, GL_BYTE, &graphic[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D,0);
  }
  if (ImGui::Button("Close##DIALOGFONTGLYPHSCLOSE")) {
    if (fontSource != NULL) {delete fontSource; fontSource = NULL;}
    if (glyphTexture != 0) {glDeleteTextures(1,&glyphTexture); glyphTexture = 0;}
    ImGui::CloseCurrentPopup();
  } else toolTip("Just close this view");
  DIALOGEND()
}

void dialogCopyPasteLoadedText(bool openIt) {
  static int loadedTextLength = 0;
  if (openIt) {
    loadedTextLength = loadedText.length()+1; // it's safe this way with our string class
  }
  DIALOGSTART("Copy paste text")
  ImGui::Text("Text from loaded file");
  ImVec2 boxSize(main_window_xres*5/6,main_window_yres/3);
  DIALOGKEYSHERE();
  ImGui::InputTextMultiline("##DIALOGCOPYPASTELOADEDTEXTFROMFILE", &loadedText[0], loadedTextLength, boxSize); toolTip("The file you have loaded");
  ImGui::Text("Text to be pasted to");
  ImGui::InputTextMultiline("##DIALOGCOPYPASTELOADEDTEXTTOEDIT", textToSave, textToSaveLength, boxSize); toolTip("The text to be edited copy/paste etc..");
  if (ImGui::Button("Close##DIALOGCOPYPASTELOADEDTEXTCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Just close this view");
  DIALOGEND()
}

void dialogEditText(bool openIt) {
  DIALOGSTART("Edit text")
  ImVec2 boxSize(main_window_xres*5/6,main_window_yres*2/3);
  DIALOGKEYSHERE();
  ImGui::InputTextMultiline("##DIALOGEDITTEXTTEXTTOEDIT", textToSave, textToSaveLength, boxSize); toolTip("The text to be edited");
  if (ImGui::Button("Close##DIALOGEDITTEXTCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Just close this view");
  DIALOGEND()
}

void dialogDisplacementMap(bool openIt) {

  static Layer *clone = NULL;
  static int layer = -1;
  static float displacementX = 1;
  static float displacementY = 1;
  static float scale = 10;
  static bool updateLayer = false;
  static int type = 1;
#define DISPLACEMENT_TYPE_COUNT 3
  static const char *displacementTypeNames[DISPLACEMENT_TYPE_COUNT] = {"Steepness","Grey(128) as base","Absolute"};

  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;

  if (openIt) {
    beforeUndoPoint();
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    layer = -1;
    updateLayer = true;
  }
  DIALOGSTART("Displacement mapping")
  ImGui::Text("The red channel is for x displacement");
  ImGui::Text("The green channel is for y displacement");
  ImGui::Text("");
  ImGui::Text("Select a layer as displacement source"); toolTip("This will be the graphical source for the displacement per pixel");
  ImGui::BeginChild("DIALOGDISPLACEMENTMAPCHILD", ImVec2(main_window_xres/2,main_window_yres/3));
  for (int i = 0; i < (int)currentCanvas->layers.size(); i++) {
    if (currentCanvas->layers[i] != l) {
      String buttonName = (layer == i ? ">" : " ") + String("Layer") + String::fromInt(i) + ":\"" + currentCanvas->layers[i]->name + "\"##" + String::fromInt(i);
      if (ImGui::Button(buttonName.c_str())) {
        updateLayer = true;
        layer = i;
      }
    }
  }
  ImGui::EndChild();

  if ((unsigned int)layer < (unsigned int)currentCanvas->layers.size()) {
    ImGui::Text("Type");ImGui::SameLine();if(ImGui::Combo("##DIALOGDISPLACEMENTMAPTYPE",&type, displacementTypeNames, DISPLACEMENT_TYPE_COUNT)) {updateLayer = true;} else toolTip("The type of pixel art scaling to apply");
    ImGui::Text("Displacment Scale"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGDISPLACEMENTMAPSCALE",&scale)) {updateLayer = true;} else toolTip("The magnitude of the pixel movement.");
    ImGui::Text("Displacement in X"); ImGui::SameLine(); if (ImGui::SliderFloat("##DIALOGDISPLACEMENTMAPSLIDERX",&displacementX,-1,1)) {updateLayer = true;} else toolTip("How much the pixels should be moved in x*scale by the displacement map");
    ImGui::Text("Displacement in Y"); ImGui::SameLine(); if (ImGui::SliderFloat("##DIALOGDISPLACEMENTMAPSLIDERY",&displacementY,-1,1)) {updateLayer = true;} else toolTip("How much the pixels should be moved in y*scale by the displacement map");
  }

  if (updateLayer) {
    if (!ImGui::IsAnyItemActive()) {
      updateLayer = false;
      if ((unsigned int)layer < (unsigned int)currentCanvas->layers.size()) {
        filterDisplacementMap(l->pixels, clone->pixels, l->width, l->height, displacementX*scale, displacementY*scale, type, l->pixelStates, l->hasSelection(), currentCanvas->layers[layer]);
        repaintLayer(l,true);
      }
    }
  }

  if (ImGui::Button("Apply##DIALOGDISPLCEMEMNTMAPAPPLY")) {
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    updatedPixels(l);
    afterUndoPoint();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGDISPLCEMEMNTMAPCLOSE")) {
    if ((!locked(l,LOCK_ANY)) && clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
  } else toolTip("Just close this view");
  DIALOGEND()
}

#define HOTKEY(__char__,__ctrl__,__alt__,__text__) ImGui::Text("%s",__text__);

void dialogHotkeys(bool openIt) {
  DIALOGSTART("Hotkeys")
  HOTKEY_ZOOMPAN
  HOTKEY_ZOOMIN
  HOTKEY_ZOOMOUT
  HOTKEY_TOOL_SELECTION
  HOTKEY_TOOL_MOVE
  HOTKEY_TOOL_WAND
  HOTKEY_TOOL_PIPETTE
  HOTKEY_TOOL_PENCIL
  HOTKEY_TOOL_PAINTBRUSH
  HOTKEY_TOOL_ERASER
  HOTKEY_TOOL_FILL
  HOTKEY_TOOL_LINE
  HOTKEY_TOOL_GRADIENT
  HOTKEY_TOOL_TEXT
  HOTKEY_TOOL_MAGIC
  HOTKEY_TOOL_BLUR
  HOTKEY_TOOL_MEASURE
  HOTKEY_TOOL_TILING
  HOTKEY_TOOL_OVERLAYS
  HOTKEY_TOGGLE_PALETTE
  HOTKEY_LAYERUNDERMOUSE
  HOTKEY_SELECTNONE
  HOTKEY_SELECTALL
  HOTKEY_INVERTSELECTION
  HOTKEY_COPY
  HOTKEY_PASTE
  HOTKEY_FITCANVAS
  HOTKEY_MOUSELOCKX
  HOTKEY_MOUSELOCKY
  HOTKEY_FULLSCREEN
  HOTKEY_ERASE
  HOTKEY_FILLCOLOR
  HOTKEY_ZOOM100
  HOTKEY_FITLAYER
  HOTKEY_MOUSETOGRID
  HOTKEY_SAVEIMAGE
  HOTKEY_RESTORESCROLL
  HOTKEY_LAYERORIGIN
  HOTKEY_REFRESH
  HOTKEY_LAYERVISIBLE
  ImGui::Text("You can press <CTRL> on sliders to enter a value");
  ImGui::Separator();
  if (ImGui::Button("Close##DIALOGABOUTCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

extern bool quit;

void dialogExit(bool openIt) {
  DIALOGSTART("Exit")
  ImGui::Text("Do you really want to exit?");
  ImGui::Text("Did you save all your work?");
  if (ImGui::Button("Quit##DIALOGEXITEXIT")) {
    quit = true;
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Continue##DIALOGEXITCLOSE")) {
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

void dialogCanvas(bool openIt) {
  static double x0=0,y0=0,x1=0,y1=0;
  if (openIt) {
    x0 = currentCanvas->canvasX0;
    y0 = currentCanvas->canvasY0;
    x1 = currentCanvas->canvasX1;
    y1 = currentCanvas->canvasY1;
  }
  DIALOGSTART("Configure canvas")
  int k;
  ImGui::PushItemWidth(64+16*2);
  k = (int)(currentCanvas->canvasX1-currentCanvas->canvasX0);ImGui::Text("Canvas width");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASVANVASWIDTH",&k)) {currentCanvas->canvasX1 = currentCanvas->canvasX0 + k;} else toolTip("The width of the picture in pixels");
  k = (int)(currentCanvas->canvasY1-currentCanvas->canvasY0);ImGui::Text("Canvas height");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASVANVASHEIGHT",&k)) {currentCanvas->canvasY1 = currentCanvas->canvasY0 + k;} else toolTip("The height of the picture in pixels");
  k = (int)(currentCanvas->canvasX0);ImGui::Text("Canvas x");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASVANVASX",&k)) {int v = currentCanvas->canvasX1-currentCanvas->canvasX0;currentCanvas->canvasX0 = k;currentCanvas->canvasX1 = currentCanvas->canvasX0+v;} else toolTip("The x position of the picture in pixels");
  k = (int)(currentCanvas->canvasY0);ImGui::Text("Canvas y");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASVANVASY",&k)) {int v = currentCanvas->canvasY1-currentCanvas->canvasY0;currentCanvas->canvasY0 = k;currentCanvas->canvasY1 = currentCanvas->canvasY0+v;} else toolTip("The y position of the picture in pixels");
  ImGui::PopItemWidth();
  double w = currentCanvas->canvasX1-currentCanvas->canvasX0;
  double h = currentCanvas->canvasY1-currentCanvas->canvasY0;
  if (w < 1) w = 1;
  if (h < 1) h = 1;
  if (ImGui::Button("tl##DIALOGCANVASTL")) {currentCanvas->canvasX0 = x0;currentCanvas->canvasY0 = y0;} else toolTip("Canvas is at the upper left of the current canvas");
  ImGui::SameLine();
  if (ImGui::Button("tc##DIALOGCANVASTC")) {currentCanvas->canvasX0 = (x0+x1)/2-w/2;currentCanvas->canvasY0 = y0;} else toolTip("Canvas is at the upper center of the current canvas");
  ImGui::SameLine();
  if (ImGui::Button("tr##DIALOGCANVASTR")) {currentCanvas->canvasX0 = x1-w;currentCanvas->canvasY0 = y0;} else toolTip("Canvas is at the upper right of the current canvas");

  if (ImGui::Button("cl##DIALOGCANVASCL")) {currentCanvas->canvasX0 = x0;currentCanvas->canvasY0 = (y0+y1)/2-h/2;} else toolTip("Canvas is at the mid left of the current canvas");
  ImGui::SameLine();
  if (ImGui::Button("cc##DIALOGCANVASCC")) {currentCanvas->canvasX0 = (x0+x1)/2-w/2;currentCanvas->canvasY0 = (y0+y1)/2-h/2;} else toolTip("Canvas is at the center of the current canvas");
  ImGui::SameLine();
  if (ImGui::Button("cr##DIALOGCANVASCR")) {currentCanvas->canvasX0 = x1-w;currentCanvas->canvasY0 = (y0+y1)/2-h/2;} else toolTip("Canvas is at the mid right of the current canvas");

  if (ImGui::Button("bl##DIALOGCANVASBL")) {currentCanvas->canvasX0 = x0;currentCanvas->canvasY0 = y1-h;} else toolTip("Canvas is at the bottom left of the current canvas");
  ImGui::SameLine();
  if (ImGui::Button("bc##DIALOGCANVASBC")) {currentCanvas->canvasX0 = (x0+x1)/2-w/2;currentCanvas->canvasY0 = y1-h;} else toolTip("Canvas is at the bottom center of the current canvas");
  ImGui::SameLine();
  if (ImGui::Button("br##DIALOGCANVASBR")) {currentCanvas->canvasX0 = x1-w;currentCanvas->canvasY0 = y1-h;} else toolTip("Canvas is at the bottom right of the current canvas");

  currentCanvas->canvasX1 = currentCanvas->canvasX0+w;
  currentCanvas->canvasY1 = currentCanvas->canvasY0+h;

  ImGui::PushItemWidth(64+16*2);

  ImGui::Separator();

  ImGui::Text("");
  float k2;
  k2 = (float)currentCanvas->control.pixelSizeX; ImGui::Text("Pixel size x");ImGui::SameLine();if (ImGui::InputFloat("##DIALOGCANVASPIXELSIZEX",&k2,0,0,2)) {currentCanvas->control.pixelSizeX = k2; shouldUpdateAll = true;}
  k2 = (float)currentCanvas->control.pixelSizeY; ImGui::Text("Pixel size y");ImGui::SameLine();if (ImGui::InputFloat("##DIALOGCANVASPIXELSIZEY",&k2,0,0,2)) {currentCanvas->control.pixelSizeY = k2; shouldUpdateAll = true;}

  ImGui::Text("");
  k = currentCanvas->gridStepX; ImGui::Text("Grid step x");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASGRIDSTEPX",&k)) {currentCanvas->gridStepX = k;} else toolTip("The size in x pixels of one grid cell");
  k = currentCanvas->gridStepY; ImGui::Text("Grid step y");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASGRIDSTEPY",&k)) {currentCanvas->gridStepY = k;} else toolTip("The size in y pixels of one grid cell");
  k = currentCanvas->gridOffsetX; ImGui::Text("Grid offset x");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASGRIDOFFSETX",&k)) {currentCanvas->gridOffsetX = k;} else toolTip("The position of the grid in pixels in x");
  k = currentCanvas->gridOffsetY; ImGui::Text("Grid offset y");ImGui::SameLine();if (ImGui::InputInt("##DIALOGCANVASGRIDOFFSETY",&k)) {currentCanvas->gridOffsetY = k;} else toolTip("The position of the grid in pixels in y");
  if (currentCanvas->gridStepX < 2) currentCanvas->gridStepX = 2;
  if (currentCanvas->gridStepY < 2) currentCanvas->gridStepY = 2;
  if (currentCanvas->gridStepX > 16384) currentCanvas->gridStepX = 16384;
  if (currentCanvas->gridStepY > 16384) currentCanvas->gridStepY = 16384;
  if (currentCanvas->gridOffsetX > 16384) currentCanvas->gridOffsetX = 16384;
  if (currentCanvas->gridOffsetY > 16384) currentCanvas->gridOffsetY = 16384;
  if (currentCanvas->gridOffsetX < -16384) currentCanvas->gridOffsetX = -16384;
  if (currentCanvas->gridOffsetY < -16384) currentCanvas->gridOffsetY = -16384;

  ImGui::PopItemWidth();

  if (ImGui::Button("Close##DIALOGCANVASCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Close with using the changes");
  DIALOGEND()
}

void dialogImageResize(bool openIt) {
  static double x0=0,y0=0,x1=0,y1=0,w=0,ow=0,h=0,oh=0;
  static bool keepAspect = true;
  static bool quantize = false;
  static double aspect = 1;
  static int interpolation = INTERPOLATION_NONE;
  if (openIt) {
    x0 = currentCanvas->canvasX0;
    y0 = currentCanvas->canvasY0;
    x1 = currentCanvas->canvasX1;
    y1 = currentCanvas->canvasY1;
    ow = x1 - x0;
    w = ow;
    oh = y1 - y0;
    h = oh;
    keepAspect = true;
    quantize = true;
    if (oh != 0) aspect = ow/oh;
  }
  DIALOGSTART("Resize image")
  noUndoMessage();
  int k;
  ImGui::Text("You may lose pixel information through resizing");
  ImGui::Text("and the layers may be rotated/resized when saving the layers!");
  ImGui::Text("");
  ImGui::PushItemWidth(64+16*2);
  k = (int)w;ImGui::Text("New image width");ImGui::SameLine();if (ImGui::InputInt("##DIALOGIMAGERESIZEWIDTH",&k)) {w = k; if (keepAspect) h = w/aspect;} else toolTip("Scale the image to this width in pixels");
  k = (int)h;ImGui::Text("New image height");ImGui::SameLine();if (ImGui::InputInt("##DIALOGIMAGERESIZEHEIGHT",&k)) {h = k; if (keepAspect) w = h*aspect;} else toolTip("Scale the image to this height in pixels");
  if (w < 1) w = 1;
  if (h < 1) h = 1;
  ImGui::PopItemWidth();
  ImGui::Text("Preserve proportions");ImGui::SameLine();ImGui::Checkbox("##DIALOGIMAGERESIZEKEEPASPECT",&keepAspect); toolTip("Keep the images x vs y ratio the same when entering new width or height");
  ImGui::Separator();
  ImGui::Text("Quantize/bake all");ImGui::SameLine();ImGui::Checkbox("##DIALOGIMAGERESIZEQUANTIZE",&quantize); toolTip("All layers loose their rotations and so on");
  ImGui::PushItemWidth(7*12);
  ImGui::Text("Interpolation");ImGui::SameLine();ImGui::Combo("##DIALOGIMAGERESIZEINTERPOLATION",&interpolation, interpolationNames, INTERPOLATION_COUNT); toolTip("The smoothing mode used at rescaling");
  ImGui::SameLine(); ImGui::Text("%s",interpolationDescs[interpolation]);
  ImGui::PopItemWidth();

  if (ImGui::Button("Resize##DIALOGIMAGERESIZEESIZE")) {
    currentCanvas->resizeCanvas(w,h,quantize,interpolation);
    ImGui::CloseCurrentPopup();
  } else toolTip("Apply the resizing");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGIMAGERESIZECLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't resize the image");
  DIALOGEND()
}

void dialogExpandShrinkSelection(bool openIt) {
  static int value = 1;
  static bool shrink = false;
  if (openIt) {
    shrink = false;
    value = 1;
  }
  DIALOGSTART("Expand or shrink selection")

  ImGui::PushItemWidth(64+16*2);
  int k;
  k = value; ImGui::Text("Expand or shrink selection by pixels");ImGui::SameLine();if (ImGui::InputInt("##DIALOGSELECTIONEXPANDORSHRINKVALUE",&k)) {value = k;} else toolTip("Increase or shrink of the selection area in pixels");
  ImGui::Text("Shrink?");ImGui::SameLine();if (ImGui::Checkbox("##DIALOGSELECTIONEXPANDORSHRINKSHRINK",&shrink)) {} else toolTip("Shrinks instead of expands the area");
  if (value > 100) value = 100;
  if (value < -100) value = -100;

  if (ImGui::Button("Apply##DIALOGCANVASAPPLY")) {
    beforeUndoPoint();
    currentCanvas->expandSelection(value * (shrink ? -1 : 1));
    afterUndoPoint();
    ImGui::CloseCurrentPopup();
  } else toolTip("Apply the shrink/expand");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGCANVASCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't do any expanding/shrinking");
  DIALOGEND()
}


void dialogDeletePicture(bool openIt) {
  DIALOGSTART("Delete current picture?");
  noUndoMessage();
  ImGui::Text("Do you want to delete the current picture \"%s\"?",currentCanvas->pictureName);
  if (ImGui::Button("Delete##DIALOGDELETEPICTUREDELETE")) {
    for (int i = 0; i < (int)canvas.size(); i++) {
      if (canvas[i] == currentCanvas) {
        delete canvas[i];
        canvas.erase(i,1);
        if (i < (int)canvas.size()) currentCanvas = canvas[i]; 
        else
        if (i-1 >= 0) currentCanvas = canvas[i-1];
      }
    }
    if (canvas.empty()) {
      addTempPicture();
      currentCanvas = canvas[0];
    }
    shouldUpdateAll = true;
    ImGui::CloseCurrentPopup();
  } else toolTip("Delete this whole picture from the program");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGDELETEPICTURECLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't delete this picture");
  DIALOGEND()
}

void dialogAddLayer(bool openIt) {
  static char layerName[LAYERNAMEMAX];
  static int w = 0, h = 0;
  if (openIt) {
    static unsigned short counter = 0; counter++;
    sprintf(layerName,"Added %d",counter);
    w = currentCanvas->canvasX1-currentCanvas->canvasX0;
    h = currentCanvas->canvasY1-currentCanvas->canvasY0;
  }
  DIALOGSTART("Add layer");
  noUndoMessage();
  ImGui::Text("Layer name"); ImGui::SameLine(); DIALOGKEYSHERE(); ImGui::InputText("##DIALOGADDLAYERNAME",layerName,LAYERNAMEMAX); toolTip("The new layers name");
  ImGui::PushItemWidth(64+16*2);
  ImGui::Text("Layer width"); ImGui::SameLine(); ImGui::InputInt("##DIALOGADDLAYERWIDTH",&w); toolTip("The new layers width in pixels");
  ImGui::Text("Layer height"); ImGui::SameLine(); ImGui::InputInt("##DIALOGADDLAYERHEIGHT",&h); toolTip("The new layers height in pixels");
  ImGui::PopItemWidth();
  if (w < 1) w = 1;
  if (h < 1) h = 1;
  if (ImGui::Button("Add layer##DIALOGADDLAYERADD")) {
    int pos = 0;
    for (int i = 0; i < (int)currentCanvas->layers.size(); i++) {
      if (currentCanvas->layers[i]->state.active || currentCanvas->layers[i]->state.selected) {pos = i; break;}
    }
    Layer *l = new Layer();
    l->init(w,h);
    l->setLayerPos(currentCanvas->canvasX0,currentCanvas->canvasY0);
    sprintf(l->name,layerName);
    currentCanvas->insertLayer(l,pos);
    currentCanvas->setActiveLayer(pos);
    shouldUpdateAll = true;
    ImGui::CloseCurrentPopup();
  } else toolTip("Add a new layer on top of the current selected ones");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGADDLAYERCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't add a new layer");
  DIALOGEND()
}

void dialogDeleteLayers(bool openIt) {
  DIALOGSTART("Delete layers");

  Array<Layer *> layers;
  bool one = false; for (int i = 0; i < (int)currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected) {one = true; layers.push_back(currentCanvas->layers[i]);}
  if (!one) for (int i2 = 0; i2 < (int)currentCanvas->layers.size(); i2++) if (currentCanvas->layers[i2]->state.active) {layers.push_back(currentCanvas->layers[i2]);}

  int l = (int)layers.size();
  for (int i3 = 0; i3 < l; i3++) {
    if (layers[i3]->sourceLayerId == 0) {
      for (int i4 = 0; i4 < (int)currentCanvas->layers.size(); i4++) {
        Layer *z = currentCanvas->layers[i4];
        if (z->sourceLayerId != 0 && z->sourceLayerId == layers[i3]->id) {
          layers.push_back(z);
        }
      }
    }
  }

  if (layers.empty()) {
    ImGui::Text("No layers selected or active");
  } else {
    noUndoMessage();
    ImGui::Text("Layers to delete");
    ImGui::BeginChild("DELETELAYERSCHILD",ImVec2(main_window_xres/2,main_window_yres/3));
    for (int i = 0; i < (int)layers.size(); i++) {
      ImGui::Text("\"%s\" %s",layers[i]->name,layers[i]->sourceLayerId != 0 ? "it's a REFERENCE":"");
    }
    ImGui::EndChild();
  }

  if (!layers.empty()) {
    if (ImGui::Button("Delete these layers##DIALOGDELETELAYERSDELETE")) {
      for (int i = 0; i < (int)layers.size(); i++) {
        currentCanvas->eraseLayer(layers[i]);
      }
      if (!currentCanvas->layers.empty()) currentCanvas->setActiveLayer(0);
      shouldUpdateAll = true;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
  } else toolTip("Delete these selected layers from the image");
  if (ImGui::Button("Close##DIALOGDELETELAYERSCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't delete these layers");
  DIALOGEND()
}

void dialogMergeDown(bool openIt) {
  DIALOGSTART("Merge down layers");

  Array<Layer *> layers;
  bool one = false; for (int i = 0; i < (int)currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected) {one = true; layers.push_back(currentCanvas->layers[i]);}
  if (!one) for (int i2 = 0; i2 < (int)currentCanvas->layers.size(); i2++) if (currentCanvas->layers[i2]->state.active) {layers.push_back(currentCanvas->layers[i2]);}

  if (layers.empty()) {
    ImGui::Text("No layers selected or active");
  } else {
    noUndoMessage();
    ImGui::Text("Layers to merge");
    ImGui::BeginChild("MERGEDOWNLAYERSCHILD",ImVec2(main_window_xres/2,main_window_yres/3));
    for (int i = 0; i < (int)layers.size(); i++) {
      ImGui::Text("\"%s\"",layers[i]->name);
    }
    ImGui::EndChild();
  }

  if (!layers.empty()) {
    if (ImGui::Button("Merge down these layers##DIALOGMERGEDOWNLAYERSMERGE")) {
      beforeUndoPoint();
      currentCanvas->mergeDownLayers();
      afterUndoPoint();
      ImGui::CloseCurrentPopup();
    } else toolTip("Insert one layer out of all the selected ones and delete the selected layers");
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGMERGEDOWNLAYERSCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Leave the layers as they are");
  DIALOGEND()
}

void dialogInsertSVG(bool openIt, int width, int height) {
  static int w = 0, h = 0;
  static float scale = 1;
  if (openIt) {
    w = width;
    h = height;
    scale = 1;
  }
  DIALOGSTART("Insert SVG");
  ImGui::PushItemWidth(64+16*2);
  ImGui::Text("Layer width %d",(int)ceil(w*scale));
  ImGui::Text("Layer height %d",(int)ceil(h*scale));
  ImGui::Text("Scale"); ImGui::SameLine(); ImGui::InputFloat("##DIALOGINSERTSVGSCALE",&scale,0,0,2); toolTip("Scale the vector graphic by this factor"); if (scale < 0) scale = 0;
  ImGui::PopItemWidth();
  if ((int)ceil(w*scale) > 0 && (int)ceil(h*scale) > 0) {
    if (ImGui::Button("Insert##DIALOGINSERTSVGINSERT")) {
      insertSVGResized(w, h, scale);
      ImGui::CloseCurrentPopup();
    } else toolTip("Insert the vector graphic as pixel layer");
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGINSERTSVGCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't insert the vector graphic");
  DIALOGEND()
}

void dialogExportLayersRaw(bool openIt) {
  static bool colorTransformOn = true;
  if (openIt) {
    colorTransformOn = true;
  }
  DIALOGSTART("Export layers raw");
  ImGui::Text("Apply postprocessing?"); ImGui::SameLine(); ImGui::Checkbox("##DIALOGEXPORTLAYERSRAWCOLORGRADINGON",&colorTransformOn); toolTip("Apply color transformations before exporting");
  if (ImGui::Button("Save##DIALOGEXPORTLAYERSRAWEXPORT")) {
    exportLayersRaw(dialogShortPath.c_str(),dialogLongFileName.c_str(),colorTransformOn);
    ImGui::CloseCurrentPopup();
  } else toolTip("Export all the layers as single files");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGEXPORTLAYERSRAWCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't export any of these layers");
  DIALOGEND()
}

#define PICTURECOMMENTMAX 256
char tempCommentBuffer[PICTURECOMMENTMAX];

void dialogPictureInfo(bool openIt) {
  if (openIt) {
    sprintf(tempCommentBuffer,"%s",currentCanvas->comment.c_str());
  }
  DIALOGSTART("Picture info");
  int nameSize = 7*(PICTURENAMEMAX);
  ImGui::PushItemWidth(nameSize);
  ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0.9,0.9,0.9,1));
  ImGui::Text("Name"); ImGui::SameLine(); DIALOGKEYSHERE(); ImGui::InputText("##DIALOGPICTUREINFOPICTURENAME",currentCanvas->pictureName, PICTURENAMEMAX); toolTip("The name of the picture");
  ImGui::Text("Author"); ImGui::SameLine(); ImGui::InputText("##DIALOGPICTUREINFOAUTHOR",currentCanvas->author, PICTURENAMEMAX); toolTip("The author of that picture");
  ImGui::PopStyleColor();
  ImGui::PopItemWidth();
  ImGui::Text("Date %s",currentCanvas->timeStamp.c_str()); toolTip("Time of the creation of this image");
  ImGui::Text("Width %d", currentCanvas->canvasX1-currentCanvas->canvasX0); toolTip("The width in pixels of the image");
  ImGui::Text("Height %d", currentCanvas->canvasY1-currentCanvas->canvasY0); toolTip("The height in pixels of the image");
  ImGui::Text("Comment");
  ImGui::InputTextMultiline("##DIALOGPICTUREINFOCOMMENT", tempCommentBuffer, PICTURECOMMENTMAX); toolTip("A user comment about this picture");
  if (ImGui::Button("Close##DIALOGPICTUREINFOCLOSE")) {
    currentCanvas->comment = String(tempCommentBuffer);
    ImGui::CloseCurrentPopup();
  } else toolTip("Close and use this info");
  DIALOGEND()
}

int gradiationTextureWidth = 256;
int gradiationTextureHeight = 256;
void updateGradiationTexture(GLuint gradiationTexture, Layer *l, int type, Array<int> &counts, bool squareRoots) {
  Array<unsigned int> gradiationTextureContent;
  gradiationTextureContent.resize(gradiationTextureWidth*gradiationTextureHeight);
  for (int i = 0; i < (int)gradiationTextureContent.size(); i++) gradiationTextureContent[i] = 0xff000000;
  counts.resize(256); for (int i5 = 0; i5 < 256; i5++) counts[i5] = 0;
  for (int i2 = 0; i2 < l->width*l->height; i2++) {
    unsigned int col = l->pixels[i2];
    int r = col & 255;
    int g = (col>>8) & 255;
    int b = (col>>16) & 255;
    int a = (col>>24) & 255;
    int grey = (r+g+b)/3;
    int count = 0;
    switch(type) {
    case 0: {count = grey;} break;
    case 1: {count = r;} break;
    case 2: {count = g;} break;
    case 3: {count = b;} break;
    case 4: {count = a;} break;
    }
    counts[count]++;
  }
  int max_ = 0;
  for (int i3 = 0; i3 < 256; i3++) {
    if (counts[i3]>max_) max_ = counts[i3];
  }
  if (max_ <= 0) max_ = 1;
  for (int y = 0; y < gradiationTextureHeight; y++) {
    for (int x = 0; x < gradiationTextureWidth; x++) {
      int heightHere = 255-y*256/gradiationTextureHeight;
      int xk = x * 256 / gradiationTextureWidth;
      unsigned int col = 0xff000000;
      double a = (double)counts[xk]/max_;
      if (a > 0 && squareRoots) a = sqrt(a);
      if (heightHere < a * 255) col = 0xff0000ff;
      gradiationTextureContent[x+y*gradiationTextureWidth] = col;
    }
  }
  glBindTexture(GL_TEXTURE_2D,gradiationTexture);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, gradiationTextureWidth, gradiationTextureHeight, 0, GL_RGBA, GL_BYTE, &gradiationTextureContent[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);
}

bool showCurveCVs = false;

class CurvePoint {
public:
  int x,y;
  float dx[2],dy[2];
  CurvePoint() {x=y=0; dx[0]=dy[0]=dx[1]=dy[1]=0;}
  CurvePoint(int x, int y) {this->x=x; this->y=y; dx[0]=dy[0]=dx[1]=dy[1]=0;}
};

double bezier(double t, double p0, double p1, double p2, double p3) {
  return (1-t)*(1-t)*(1-t)*p0 + 3*(1-t)*(1-t)*t*p1 + 3*(1-t)*t*t*p2 + t*t*t*p3;
}

class Curve {
public:
  Curve() {
    selectedPoint = -1;
    selectedCV = -1;
    dragPoint = -1;
  }
  int bezierY[256];
  Array<CurvePoint> points;
  void clear() {points.clear(); selectedPoint = -1; dragPoint = -1; selectedCV = -1;}
  int selectedPoint;
  int selectedCV;
  int dragPoint;

  int getInterpolation(int x) {
    int found = -1;
    for (int i = 0; i < (int)points.size()-1; i++) {
      if (x >= points[i].x && x <= points[i+1].x) {
        found = i;
        break;
      }
    }
    if (found < 0) return 0;
    float deltaX = x - points[found].x;
    float deltaXF = points[found+1].x - points[found].x;
    if (deltaXF == 0) return points[found].x;
    if (points[found].dx[1]==0&&points[found].dy[1]==0&&points[found+1].dx[0]==0&&points[found+1].dy[0]==0)
      return (points[found+1].y - points[found].y)*deltaX/deltaXF + points[found].y;
    return bezierY[x];
  } 

  void insertPoint(int x) {
    int found = -1;
    for (int i = 0; i < (int)points.size()-1; i++) {
      if (x > points[i].x && x <= points[i+1].x) {
        found = i;
        break;
      }
    }
    if (found < 0) return;
    int y = getInterpolation(x);
    points.insert(CurvePoint(x,y),found+1);
    selectedPoint = found + 1;
    dragPoint = -1;
  }

  void erasePoint(int x) {
    if (x < 1) return;
    if (x >= (int)points.size()-1) return;
    points.erase(x,1);
    selectedPoint = x-1;
    dragPoint = -1;
  }

  void generateBezier() {
#define BEZIER_TESSEL 128
    int lxi = points[0].x;
    int lyi = points[0].y;
    for (int i = 0; i < (int)points.size()-1; i++) {
      for (int j = 0; j < BEZIER_TESSEL+1; j++) {
        double bx = bezier((double)j/BEZIER_TESSEL,points[i].x,points[i].x+points[i].dx[1],points[i+1].x+points[i+1].dx[0],points[i+1].x);
        double by = bezier((double)j/BEZIER_TESSEL,points[i].y,points[i].y+points[i].dy[1],points[i+1].y+points[i+1].dy[0],points[i+1].y);
        int bxi = (int)floor(bx);
        int byi = (int)floor(by);
        int dx = bxi-lxi;
        int dy = byi-lyi;
        int d = abs(dx);
        float xp = lxi;
        float yp = lyi;
        float xa = 1;
        float ya = d>0?dy/d:0;
        for (int k = 0; k < d+1; k++) {
          int xpi = (int)floor(xp);
          int ypi = (int)floor(yp);
          if (xpi>=points[i].x&&xpi<=points[i+1].x&&xpi<256) {
            bezierY[xpi] = ypi;
          }
          xp += xa;
          yp += ya;
        }
        lxi = bxi;
        lyi = byi;
      }
    }
  }
};

Curve curves[5];

void drawCurve(const ImVec2 &minB,const ImVec2 &maxB, Curve &c, int histogramCount) {
  UNUSED(maxB);
  static bool lastMouseLeft = ImGui::IsMouseDown(0);
  bool mouseLeft = ImGui::IsMouseDown(0);
  bool toolTipShown = false;
  for (int i = 0; i < (int)c.points.size()-1; i++) {
    ImVec2 p0 = ImVec2(c.points[i].x,255-c.points[i].y);
    ImVec2 p1 = ImVec2(c.points[i+1].x,255-c.points[i+1].y);
    p0.x += minB.x;
    p0.y += minB.y;
    p1.x += minB.x;
    p1.y += minB.y;
    if (c.points[i].dx[1]==0&&c.points[i].dy[1]==0&&c.points[i+1].dx[0]==0&&c.points[i+1].dy[0]==0) {
      ImGui::GetWindowDrawList()->AddLine(p0,p1,0xffffffff);
      if (mouseX >= p0.x && mouseX <= p1.x && ImGui::IsMouseHoveringRect(minB,maxB)) {
        if (!toolTipShown) {
          float dx = p1.x - p0.x;
          float py = p0.y;
          if (dx != 0) py = (p1.y-p0.y)*(mouseX-p0.x)/dx+p0.y;
          py -= minB.y; py = 255 - py;
          toolTip("%d,%d\n%d", (int)(mouseX-minB.x),(int)py,histogramCount); toolTipShown = true;
        }
      }
    } else {
      ImVec2 last = p0;
      for (int x = c.points[i].x; x < c.points[i+1].x; x++) {
        ImVec2 p = ImVec2(x,255-c.bezierY[x]);
        p.x += minB.x;
        p.y += minB.y;
        ImGui::GetWindowDrawList()->AddLine(last,p,0xffffffff);
        if ((int)mouseX == (int)p.x && ImGui::IsMouseHoveringRect(minB,maxB)) {
          if (!toolTipShown) {
            float py = p.y;
            py -= minB.y; py = 255 - py;
            toolTip("%d,%d\n%d", (int)(mouseX-minB.x),(int)py,histogramCount); toolTipShown = true;
          }
        }
        last = p;
      }
      ImGui::GetWindowDrawList()->AddLine(last,p1,0xffffffff);
    }
  }
  float circleRadius = 8;
  if (!mouseLeft) {
    c.dragPoint = -1;
  }
  for (int i2 = 0; i2 < (int)c.points.size(); i2++) {
    ImVec2 p0 = ImVec2(c.points[i2].x,255-c.points[i2].y);
    p0.x += minB.x;
    p0.y += minB.y;
    double dx = mouseX - p0.x;
    double dy = mouseY - p0.y;
    double d = sqrt(dx*dx+dy*dy);
    if (!showCurveCVs) {
      unsigned int col = 0xff00ffff;
      if (d < circleRadius) {
        col = 0xff008080;
        if (mouseLeft) {
          if (!lastMouseLeft) {
            c.selectedPoint = i2;
            c.selectedCV = -1;
            c.dragPoint = i2;
          }
        }
      }
      if (c.selectedPoint == i2) col = 0xffffff00;
      if (c.dragPoint == i2) col = 0xffffff80;
      ImGui::GetWindowDrawList()->AddCircle(p0,circleRadius,col);
    }
    if (showCurveCVs) {
      unsigned int col;
      ImVec2 dp;
      col = 0xff00ffff;
      dp.x = p0.x + c.points[i2].dx[0];
      dp.y = p0.y - c.points[i2].dy[0];
      dx = mouseX - dp.x; dy = mouseY - dp.y; d = sqrt(dx*dx+dy*dy);
      if (d < circleRadius && i2 > 0) {
        col = 0xff008080;
        if (mouseLeft) {
          if (!lastMouseLeft) {
            c.selectedPoint = i2;
            c.dragPoint = i2;
            c.selectedCV = 0;
          }
        }
      }
      if (c.selectedPoint == i2 && c.selectedCV == 0) col = 0xffffff00;
      ImGui::GetWindowDrawList()->AddLine(p0,dp,0xff004040);
      ImGui::GetWindowDrawList()->AddCircle(dp,4,col);
      col = 0xff00ffff;
      dp.x = p0.x + c.points[i2].dx[1];
      dp.y = p0.y - c.points[i2].dy[1];
      dx = mouseX - dp.x; dy = mouseY - dp.y; d = sqrt(dx*dx+dy*dy);
      if (d < circleRadius && i2 < (int)c.points.size() - 1 ) {
        col = 0xff008080;
        if (mouseLeft) {
          if (!lastMouseLeft) {
            c.selectedPoint = i2;
            c.dragPoint = i2;
            c.selectedCV = 1;
          }
        }
      }
      if (c.selectedPoint == i2 && c.selectedCV == 1) col = 0xffffff00;
      ImGui::GetWindowDrawList()->AddLine(p0,dp,0xff004040);
      ImGui::GetWindowDrawList()->AddCircle(dp,4,col);
    }
  }
  lastMouseLeft = mouseLeft;
}

void getGradingArrayFromCurves(unsigned char *dest) {
  for (int j = 0; j < 5; j++) {
    curves[j].generateBezier();
    for (int x = 0; x < 256; x++) {
      int v = curves[j].getInterpolation(x);
      if (v < 0) v = 0;
      if (v > 255) v = 255;
      dest[j*256+x] = (unsigned char)v;
    }
  }
}

void stringToCurves(const String &string) {
  Array<String> l; getLines(l, &string[0]);
  int currentCurve = -1;
  for (int i = 0; i < (int)l.size(); i++) {
    const String &str = l[i];
    if (str == "C") {
      currentCurve++; 
      if (currentCurve >= 5) break;
      curves[currentCurve].points.clear();
      continue;
    }
    CurvePoint c; CurvePoint *p = &c;
    if (sscanf(&str[0],"P %d %d %f %f %f %f",&p->x,&p->y,&p->dx[0],&p->dy[0],&p->dx[1],&p->dy[1]) == 6) {
      if (currentCurve >= 0) curves[currentCurve].points.push_back(c);
    }
  }
}

String stringFromCurves() {
  String ret;
  for (int i = 0; i < 5; i++) {
    Curve *c = &curves[i];
    ret += "C\n";
    for (int j = 0; j < (int)c->points.size(); j++) {
      CurvePoint *p = &c->points[j];
      sprintf(tempBuffer,"P %d %d %.02f %.02f %.02f %.02f\n",p->x,p->y,p->dx[0],p->dy[0],p->dx[1],p->dy[1]);
      ret += String(tempBuffer);
    }
  }
  return ret;
}

void applyColorGradingCurves(Layer *dest, Layer *source) {
  if (locked(dest,LOCK_ANY)) return;
  if (dest->width != source->width || dest->height != source->height) return;
  Array<unsigned char> vals; vals.resize(5*256); getGradingArrayFromCurves(&vals[0]);
  bool hasSelection = dest->hasSelection();
  for (int i = 0; i < dest->width*dest->height; i++) {
    if ((!hasSelection)||(dest->pixelStates[i] & PIXELSTATE_SELECTED)) {
      unsigned int colS = source->pixels[i];
      int r = colS & 255;
      int g = (colS>>8) & 255;
      int b = (colS>>16) & 255;
      int a = (colS>>24) & 255;
      int rs = r;
      int gs = g;
      int bs = b;
      r = (int)vals[1*256+rs];
      g = (int)vals[2*256+gs];
      b = (int)vals[3*256+bs];
      a = (int)vals[4*256+a];
      r += (int)vals[0*256+rs]-rs;
      g += (int)vals[0*256+gs]-gs;
      b += (int)vals[0*256+bs]-bs;
      if (r < 0) r = 0;
      if (g < 0) g = 0;
      if (b < 0) b = 0;
      if (a < 0) a = 0;
      if (r > 255) r = 255;
      if (g > 255) g = 255;
      if (b > 255) b = 255;
      if (a > 255) a = 255;
      dest->pixels[i] = r | (g<<8) | (b<<16) | (a<<24);
    }
  }
}

void dialogPreview(Layer *l0, Layer *l1) {
  Layer *l2;
  Array<unsigned int> image;
  image.resize(dialogPreviewTextureWidth*dialogPreviewTextureHeight);
  glBindTexture(GL_TEXTURE_2D,dialogPreviewTexture[0]);
  l2 = l0;
  for (int y = 0; y < dialogPreviewTextureHeight; y++) {
    for (int x = 0; x < dialogPreviewTextureWidth; x++) {
      unsigned int col = 0xff000000;
      if (l2 != NULL) {
        int sx = x * l2->width / dialogPreviewTextureWidth;
        int sy = y * l2->height / dialogPreviewTextureHeight;
        if ((unsigned int)sx < (unsigned int)l2->width && (unsigned int)sy < (unsigned int)l2->height) col = l2->pixels[sx+sy*l2->width];
      }
      image[x+y*dialogPreviewTextureWidth] = col;
    }
  }
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, dialogPreviewTextureWidth, dialogPreviewTextureHeight, 0, GL_RGBA, GL_BYTE, &image[0]);
  glBindTexture(GL_TEXTURE_2D,dialogPreviewTexture[1]);
  l2 = l1;
  {for (int y = 0; y < dialogPreviewTextureHeight; y++) {
    for (int x = 0; x < dialogPreviewTextureWidth; x++) {
      unsigned int col = 0xff000000;
      if (l2 != NULL) {
        int sx = x * l2->width / dialogPreviewTextureWidth;
        int sy = y * l2->height / dialogPreviewTextureHeight;
        if ((unsigned int)sx < (unsigned int)l2->width && (unsigned int)sy < (unsigned int)l2->height) col = l2->pixels[sx+sy*l2->width];
      }
      image[x+y*dialogPreviewTextureWidth] = col;
    }
  }}
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, dialogPreviewTextureWidth, dialogPreviewTextureHeight, 0, GL_RGBA, GL_BYTE, &image[0]);
  glBindTexture(GL_TEXTURE_2D,0);
  double a = main_window_yres == 0 || aspectRatio == 0 ? 1 : 1.0/aspectRatio * main_window_xres / main_window_yres;
  int dialogPreviewTextureWidth2 = dialogPreviewTextureWidth * a;
  ImGui::Image((ImTextureID)dialogPreviewTexture[0],ImVec2(dialogPreviewTextureWidth2,dialogPreviewTextureHeight));
  ImGui::SameLine();
  ImGui::Image((ImTextureID)dialogPreviewTexture[1],ImVec2(dialogPreviewTextureWidth2,dialogPreviewTextureHeight));
}

#define DIALOGPREVIEW(__layerAfter__, __layerBefore__) dialogPreview(__layerAfter__,__layerBefore__);

ColorTransform *gradientCurvesToColorTransform = NULL;

void dialogGradiationCurves(bool openIt) {
  static String curveBefore = "";
  static GLuint gradiationTexture = 0;
  static int gradiationChannel = 0;
  static bool updateGradientCurvesPreview = false;
  static const char *channelNames[5] = {"All","Red","Green","Blue","Alpha"};
  static Layer *clone = NULL;
  static Array<int> histogram;
  static bool showCurveBetter = false;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  if (openIt) {
    showCurveCVs = false;
    beforeUndoPoint();
    gradiationChannel = 0;
    if (gradiationTexture != 0) {glDeleteTextures(1,&gradiationTexture); gradiationTexture = 0;}
    glGenTextures(1,&gradiationTexture);
    updateGradiationTexture(gradiationTexture,l,gradiationChannel,histogram,showCurveBetter);
    if (!reOpen) {
      for (int i = 0; i < 5; i++) {
        curves[i].clear();
        curves[i].points.push_back(CurvePoint(0,0));
        curves[i].points.push_back(CurvePoint(255,255));
      }
      if (gradientCurvesToColorTransform == NULL) {
        if (clone != NULL) {delete clone; clone = NULL;}
        clone = l->clone();
      } else {
        curveBefore = gradientCurvesToColorTransform->gradientCurves;
        stringToCurves(curveBefore);
      }
    }
    reOpen = false;
    updateGradientCurvesPreview = true;
  }
  DIALOGSTART("Gradiation curves");
  dialogLoadSaveOff = true; dialogLoadSave(false,DIALOG_NONE);

  if (roundButton("Reset##DIALOGGRADIATIONRESET",true)) {
    for (int i = 0; i < 5; i++) {
      curves[i].clear();
      curves[i].points.push_back(CurvePoint(0,0));
      curves[i].points.push_back(CurvePoint(255,255));
    }
    updateGradientCurvesPreview = true;
  } else toolTip("Clear the settings to the normal state");
  ImGui::SameLine(); 
  if (roundButton("Load##DIALOGGRADIATIONLOAD",true)) {
    dialogLoadSave(true,DIALOG_LOADGRADIENTCURVES);
  } else toolTip("Load a curve");
  ImGui::SameLine(); 
  if (roundButton("Save##DIALOGGRADIATIONLOAD",true)) {
    dialogLoadSave(true,DIALOG_SAVEGRADIENTCURVES);
  } else toolTip("Save this curve");
  ImGui::BeginChild("DIALOGGRADIENTCURVESCHILD",ImVec2(gradiationTextureWidth,gradiationTextureHeight));
  ImGui::Image((ImTextureID)gradiationTexture,ImVec2(gradiationTextureWidth,gradiationTextureHeight));
  ImVec2 minB = ImGui::GetItemRectMin();
  ImVec2 maxB = ImGui::GetItemRectMax();
  int histo = 0;
  if (maxB.x - minB.x > 0) {
    int mx2 = mouseX - minB.x;
    if (mx2 >= 0 && mx2 < (int)histogram.size()) histo = histogram[mx2];
  }
  drawCurve(minB,maxB,curves[gradiationChannel],histo);
  ImGui::EndChild();

  if (gradientCurvesToColorTransform == NULL) {ImGui::SameLine(); DIALOGPREVIEW(l,clone);}

  ImVec2 minB2 = minB;
  minB2.x -= 1;
  minB2.y -= 1;
  ImGui::GetWindowDrawList()->AddRect(minB2,maxB,0xff808080);
  ImGui::PushItemWidth(64+16*2);
  int colorIndex = (mouseX - minB.x)*256/(maxB.x-minB.x);
  int colorValue = 255-(mouseY - minB.y)*256/(maxB.y-minB.y);
  if (colorIndex < 0) colorIndex = 0;
  if (colorValue < 0) colorValue = 0;
  if (colorIndex > 255) colorIndex = 255;
  if (colorValue > 255) colorValue = 255;
  int colorIndex2 = colorIndex;
  int colorValue2 = colorValue;
  Curve &c = curves[gradiationChannel];
  if ((unsigned int)c.selectedPoint < c.points.size()) {
    CurvePoint &p = c.points[c.selectedPoint];
    colorIndex2 = p.x;
    colorValue2 = p.y;
  }
  bool pointUpdated = false;
  ImGui::Text("Index"); ImGui::SameLine(); if (ImGui::InputInt("##DIALOGGRADIATIONINDEX",&colorIndex2,1,1)) {colorIndex = colorIndex2; colorValue = colorValue2; pointUpdated = true;} else toolTip("The selected curve point");
  ImGui::SameLine(); ImGui::Text("Value"); ImGui::SameLine(); if (ImGui::InputInt("##DIALOGGRADIATIONVALUE",&colorValue2,1,1)) {colorIndex = colorIndex2; colorValue = colorValue2; pointUpdated = true;} else toolTip("The selected points height");
  if (colorIndex < 0) colorIndex = 0;
  if (colorValue < 0) colorValue = 0;
  if (colorIndex > 255) colorIndex = 255;
  if (colorValue > 255) colorValue = 255;
  if (pointUpdated) {
    updateGradientCurvesPreview = true;
    CurvePoint &p = c.points[c.selectedPoint];
    if (c.selectedPoint == 0 || c.selectedPoint == (int)c.points.size()-1) {
      p.y = colorValue;
    } else {
      CurvePoint &pm = c.points[c.selectedPoint-1];
      CurvePoint &pp = c.points[c.selectedPoint+1];
      if (colorIndex >= pp.x) colorIndex = pp.x-1;
      if (colorIndex <= pm.x) colorIndex = pm.x+1;
      p.x = colorIndex;
      p.y = colorValue;
    }
  }

  if ((unsigned int)c.dragPoint < c.points.size()) {
    CurvePoint &p = c.points[c.dragPoint];
    updateGradientCurvesPreview = true;
    if (c.selectedCV < 0) {
      if (c.dragPoint == 0 || c.dragPoint == (int)c.points.size()-1) {
        p.y = colorValue;
      } else {
        CurvePoint &pm = c.points[c.dragPoint-1];
        CurvePoint &pp = c.points[c.dragPoint+1];
        if (colorIndex >= pp.x) colorIndex = pp.x-1;
        if (colorIndex <= pm.x) colorIndex = pm.x+1;
        p.x = colorIndex;
        p.y = colorValue;
      }
    } else {
      p.dx[c.selectedCV] = colorIndex - p.x;
      p.dy[c.selectedCV] = colorValue - p.y;
    }
  }

  if (ImGui::IsMouseClicked(1)) {
    if (c.dragPoint<0&&(!showCurveCVs))
      c.insertPoint(mouseX-minB.x);
  }

  dialogNoMove = false;
  for (int i = 0; i < 5; i++) if (curves[i].dragPoint>=0) dialogNoMove = true;
  ImGui::Text("CVs"); ImGui::SameLine(); ImGui::Checkbox("##DIALOGGRADIATIONCVS",&showCurveCVs); toolTip("Show bezier curve handles");
  ImGui::SameLine(); ImGui::Text("Channel");ImGui::SameLine(); if (ImGui::Combo("##DIALOGGRADIATIONCHANNEL",&gradiationChannel, channelNames, 5)) {updateGradiationTexture(gradiationTexture,l,gradiationChannel,histogram,showCurveBetter);} else toolTip("Show the curve for a color channel");
  ImGui::PopItemWidth();
  ImGui::SameLine(); if (roundButton("Delete##DIALOGGRADIATIONDELETE",c.selectedPoint>0 && c.selectedPoint<(int)c.points.size()-1)) {c.erasePoint(c.selectedPoint); updateGradientCurvesPreview = true;} else toolTip("Delete this curve point");
  ImGui::SameLine(); ImGui::Text("Sqrt"); ImGui::SameLine(); if (ImGui::Checkbox("##DIALOGGRADIATIONBETTER",&showCurveBetter)) {updateGradiationTexture(gradiationTexture,l,gradiationChannel,histogram,showCurveBetter);} toolTip("Histogram shows squareroots as red columns");

  if (shouldUpdateAll) updateGradientCurvesPreview = true;

  if (updateGradientCurvesPreview) {
    if (gradientCurvesToColorTransform == NULL) {
      updateGradientCurvesPreview = false;
      applyColorGradingCurves(l,clone);
      repaintLayer(l,true);
    } else {
      String c = stringFromCurves();
      gradientCurvesToColorTransform->gradientCurves = c;
      gradientCurvesToColorTransform->shouldUpdate = true;
      repaintLayer(l,true);
    }
  }

  if (ImGui::Button("Apply##DIALOGGRADIATIONCURVEAPPLY")) {
    if (gradientCurvesToColorTransform == NULL) {
      if (clone != NULL) {delete clone; clone = NULL;}
    }
    dialogNoMove = false;
    if (gradiationTexture != 0) {glDeleteTextures(1,&gradiationTexture); gradiationTexture = 0;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    updatedPixels(l);
    afterUndoPoint();
    gradientCurvesToColorTransform = NULL;
  } else toolTip("Update the graphics with the curves");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGGRADIATIONCURVESCLOSE")) {
    if (gradientCurvesToColorTransform == NULL) {
      if ((!locked(l,LOCK_ANY)) && clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
      if (clone != NULL) {delete clone; clone = NULL;}
    } else {
      curveBefore = gradientCurvesToColorTransform->gradientCurves;
    }
    dialogNoMove = false;
    if (gradiationTexture != 0) {glDeleteTextures(1,&gradiationTexture); gradiationTexture = 0;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    gradientCurvesToColorTransform = NULL;
  } else toolTip("Don't update the graphics");

  DIALOGEND()
}

void dialogColorBalance(bool openIt) {
  static Layer *clone = NULL;
  static int channel = 0;
  static float colorBalanceR[3];
  static float colorBalanceG[3];
  static float colorBalanceB[3];
  static bool keepLuminance = true;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  static bool updateImage = false;
  if (openIt) {
    beforeUndoPoint();
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    channel = 1;
    for (int i = 0; i < 3; i++) {
      colorBalanceR[i] = 0;
      colorBalanceG[i] = 0;
      colorBalanceB[i] = 0;
    }
    keepLuminance = true;
    updateImage = true;
  }
  DIALOGSTART("Color balance");
  DIALOGPREVIEW(l,clone);

  float *k;
  ImGui::PushItemWidth(200);
  k = &colorBalanceR[channel]; ImGui::Text("purple - red - yellow"); if (ImGui::SliderFloat("##DIALOGCOLORBALANCEPRY",k,-1,1)) {updateImage = true;} else toolTip("Shift the colors in that direction"); if (*k<-1) *k = -1; if (*k>1) *k = 1;
  k = &colorBalanceG[channel]; ImGui::Text("yellow - green - cyan"); if (ImGui::SliderFloat("##DIALOGCOLORBALANCEGYC",k,-1,1)) {updateImage = true;} else toolTip("Shift the colors in that direction"); if (*k<-1) *k = -1; if (*k>1) *k = 1;
  k = &colorBalanceB[channel]; ImGui::Text("cyan - blue - purple"); if (ImGui::SliderFloat("##DIALOGCOLORBALANCECBP",k,-1,1)) {updateImage = true;} else toolTip("Shift the colors in that direction"); if (*k<-1) *k = -1; if (*k>1) *k = 1;
  ImGui::Text("");   
  ImGui::Text("Shadows"); ImGui::SameLine(); ImGui::RadioButton("##DIALOGCOLORBALANCECHANNEL0", &channel, 0); toolTip("Update the lower part of the colors by that");
  ImGui::Text("Mids"); ImGui::SameLine(); ImGui::RadioButton("##DIALOGCOLORBALANCECHANNEL1", &channel, 1); toolTip("Update the middle part of the colors by that");
  ImGui::Text("Lights"); ImGui::SameLine(); ImGui::RadioButton("##DIALOGCOLORBALANCECHANNEL2", &channel, 2); toolTip("Update the upper part of the colors by that");
  ImGui::Text("Keep luminance"); ImGui::SameLine(); if (ImGui::Checkbox("##DIALOGCOLORBALANCEKEEPLUMINANCE", &keepLuminance)) {updateImage = true;} else toolTip("Keep the luminance of the source color");
  ImGui::PopItemWidth();

  if (updateImage && (!ImGui::IsAnyItemActive())) {
    if ((!locked(l,LOCK_ANY)) && l->height == clone->height && l->width == clone->width) {
      bool hasSelection = l->hasSelection();
      for (int i = 0; i < l->width * l->height; i++) {
        if ((!hasSelection) || (l->pixelStates[i] & PIXELSTATE_SELECTED)) {
          unsigned int col = clone->pixels[i];
          float r = col & 255;
          float g = (col>>8) & 255;
          float b = (col>>16) & 255;
          float lumaStart = (r+g+b)/3.0;
          float rd = 0;
          float gd = 0;
          float bd = 0;
          float s;
          for (int j = 0; j < 3; j++) {
            float ra = 0, ga = 0, ba = 0;
            s = colorBalanceR[j]; ra += r; if (s>0) {ga += r * s;} else {ba += r * -s;}
            s = colorBalanceG[j]; ga += g; if (s>0) {ba += g * s;} else {ra += g * -s;}
            s = colorBalanceB[j]; ba += b; if (s>0) {ra += b * s;} else {ga += b * -s;}
            float f = lumaStart/255.f;
            switch(j) {
            case 0: {f = 1-f*2;} break;
            case 1: {f = 1-fabs(f-0.5)*2;} break;
            case 2: {f = 1-(1-f)*2;} break;
            }
            if (f < 0) f = 0;
            if (f > 1) f = 1;
            rd += ra * f;
            gd += ga * f;
            bd += ba * f;
          }
          r = rd;
          g = gd;
          b = bd;
          float lumaEnd =  (r+g+b)/3.0;
          if (keepLuminance && lumaEnd != 0) {
            r *= lumaStart/lumaEnd;
            g *= lumaStart/lumaEnd;
            b *= lumaStart/lumaEnd;
          }
          int a = (col >> 24) & 255;
          int ri = (int)r;
          int gi = (int)g;
          int bi = (int)b;
          if (ri < 0) ri = 0; 
          if (ri > 255) ri = 255;
          if (gi < 0) gi = 0; 
          if (gi > 255) gi = 255;
          if (bi < 0) bi = 0; 
          if (bi > 255) bi = 255;
          l->pixels[i] = ri|(gi<<8)|(bi<<16)|(a<<24);        
        }
      }
      repaintLayer(l,true);
    }
    updateImage = false;
  }

  if (ImGui::Button("Apply##DIALOGCOLORBALANCEAPPLY")) {
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    updatedPixels(l);
    afterUndoPoint();
  } else toolTip("Update the colors of the graphics");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGCOLORBALANCECLOSE")) {
    if ((!locked(l,LOCK_ANY)) && clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
  } else toolTip("Don't update the graphics");
  DIALOGEND()
}

enum BLURTYPES {
  BLURTYPE_GAUSS = 0,
  BLURTYPE_BOX = 1,
  BLURTYPE_RADIAL = 2,
  BLURTYPE_ITERATIVE = 3,
  BLURTYPE_MEDIAN = 4,
  BLURTYPE_COUNT = 5
};

void cropTwo(Layer *base, Layer *cropToo) {
  if (locked(base,LOCK_ANY)) return;
  if (locked(cropToo,LOCK_ANY)) return;
  if (base->width != cropToo->width || base->height != cropToo->height) return;
  float alphaThreshold = 0.5;
  bool first = true;
  int minX = 0, minY = 0, maxX = 0, maxY = 0;
  for (int y = 0; y < base->height; y++) {
    for (int x = 0; x < base->width; x++) {
      int a = base->pixels[x+y*base->width]>>24;
      if (a >= alphaThreshold) {
        if (first || x < minX) minX = x;
        if (first || y < minY) minY = y;
        if (first || x > maxX) maxX = x;
        if (first || y > maxY) maxY = y;
        first = false;
      }
    }
  }
  maxX++;
  maxY++;
  int w = base->width; int h = base->height;
  base->enlargenLayerBy(-minX,-minY,-(w-maxX),-(h-maxY));
  cropToo->enlargenLayerBy(-minX,-minY,-(w-maxX),-(h-maxY));
  shouldUpdateAll = true;
}

void dialogBlur(bool openIt) {
  static Layer *clone = NULL;
  static int boxWidth = 4;
  static int boxHeight = 4;
  static float gaussX = 2;
  static float  gaussY = 2;
  static float radialAngle = 0;
  static float radialSize = 4;
  static int iterativeSteps = 1;
  static int medianWidth = 4;
  static int medianHeight = 4;
  static int blurType = BLURTYPE_GAUSS;
  static const char *blurTypes[BLURTYPE_COUNT] = {"Gauss blur", "Box blur", "Radial blur","Iterative", "Median"};
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  static int maxBlurPixels = 100;
  static int maxMedianPixels = 30;
  static bool medianCircular = false;
  static bool medianContours = false;
  static bool highPass = false;
  static bool updatedValues = false;
  static int increaseLayerWidth = 0;
  static int increaseLayerHeight = 0;

  if (openIt) {
    beforeUndoPoint();
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    updatedValues = true;
  }
  DIALOGSTART("Blur");

  ImGui::PushItemWidth(7*15);
  ImGui::Text("Type");ImGui::SameLine(); if (ImGui::Combo("##DIALOGBLURTYPE",&blurType, blurTypes, BLURTYPE_COUNT)) {updatedValues = true;} else toolTip("The type of blur to apply");
  ImGui::PopItemWidth();

  ImGui::PushItemWidth(64);

  bool test = roundButton("Preview##DIALOGBLURTEST",true);

  switch(blurType) {
    case BLURTYPE_BOX: {
      ImGui::Text("Box blur settings");
      ImGui::Text("Radius X"); ImGui::SameLine(); if (ImGui::InputInt("##DIALOGBLURBOXWIDTH",&boxWidth,0,0)) {updatedValues = true;} else toolTip("The rectangular blur width/2"); if (boxWidth<0) boxWidth = 0; if (boxWidth>maxBlurPixels) boxWidth = maxBlurPixels;
      ImGui::Text("Radius Y"); ImGui::SameLine(); if (ImGui::InputInt("##DIALOGBLURBOXHEIGHT",&boxHeight,0,0)) {updatedValues = true;} else toolTip("The rectangular blur height/2"); if (boxHeight<0) boxHeight = 0; if (boxHeight>maxBlurPixels) boxHeight = maxBlurPixels;
      increaseLayerWidth = boxWidth;
      increaseLayerHeight = boxHeight;
    } break;
    case BLURTYPE_RADIAL: {
      ImGui::Text("Radial blur settings");
      ImGui::PushItemWidth(360);
      ImGui::Text("Angle"); ImGui::SameLine(); if (ImGui::SliderFloat("##DIALOGBLURRADIALANGLE",&radialAngle,0,360)) {updatedValues = true;} else toolTip("The direction in which to blur"); if (radialAngle<0) radialAngle = 0; if (radialAngle>360) radialAngle = 360;
      ImGui::PopItemWidth();
      ImGui::Text("Radius"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGBLURRADIALRADIUS",&radialSize,0,0,0)) {updatedValues = true;} else toolTip("The length of one streak of the blur"); if (radialSize<0) radialSize = 0; if (radialSize>maxBlurPixels) radialSize = maxBlurPixels;
      increaseLayerWidth = radialSize;
      increaseLayerHeight = radialSize;
    } break;
    case BLURTYPE_GAUSS: {
      ImGui::Text("Gauss blur settings");
      ImGui::Text("Radius X"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGBLURGAUSSX",&gaussX,0,0,2)) {updatedValues = true;} else toolTip("The width of the gaussian blur cell"); if (gaussX<0) gaussX = 0; if (gaussX>16384) gaussX = 16384;
      ImGui::Text("Radius Y"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGBLURGAUSSY",&gaussY,0,0,2)) {updatedValues = true;} else toolTip("The height of the gaussian blur cell"); if (gaussY<0) gaussY = 0; if (gaussY>16384) gaussY = 16384;
      increaseLayerWidth = gaussX;
      increaseLayerHeight = gaussY;
    } break;
    case BLURTYPE_ITERATIVE: {
      ImGui::Text("Iterative blur settings");
      ImGui::Text("Steps"); ImGui::SameLine(); if (ImGui::InputInt("##DIALOGBLURITERATIVE",&iterativeSteps,0,0)) {updatedValues = true;} else toolTip("How many steps of pixel merging"); if (iterativeSteps<0) iterativeSteps = 0; if (iterativeSteps>32) iterativeSteps = 32;
      increaseLayerWidth = 4;
      increaseLayerHeight = 4;
    } break;
    case BLURTYPE_MEDIAN: {
      ImGui::Text("Median blur settings");
      ImGui::Text("Circular"); ImGui::SameLine(); if (ImGui::Checkbox("##DIALOGBLURMEDIANCIRCULAR",&medianCircular)) {updatedValues = true;} else toolTip("This changes from box filter to some sort of round shape, maybe smoother.");
      ImGui::Text("Radius X"); ImGui::SameLine(); if (ImGui::InputInt("##DIALOGBLURMEDIANWIDTH",&medianWidth,0,0)) {updatedValues = true;} else toolTip("The rectangular blur width/2"); if (medianWidth<0) medianWidth = 0; if (medianWidth>maxMedianPixels) medianWidth = maxMedianPixels;
      ImGui::Text("Radius Y"); ImGui::SameLine(); if (ImGui::InputInt("##DIALOGBLURMEDIANHEIGHT",&medianHeight,0,0)) {updatedValues = true;} else toolTip("The rectangular blur height/2"); if (medianHeight<0) medianHeight = 0; if (medianHeight>maxMedianPixels) medianHeight = maxMedianPixels;
      ImGui::Text("Keep selection area contours"); ImGui::SameLine(); if (ImGui::Checkbox("##DIALOGBLURMEDIANCONTOURS",&medianContours)) {updatedValues = true;} else toolTip("Doesn't take pixels outside the selected area into account");
      increaseLayerWidth = medianWidth;
      increaseLayerHeight = medianHeight;
    } break;
  }
  ImGui::Separator();
  ImGui::Text("Just get the highpass"); ImGui::SameLine(); if (ImGui::Checkbox("##DIALOGBLURHIGHPASS",&highPass)) {updatedValues = true;} else toolTip("Gets the highpass (range -128..127) instead of the blur. Use layer mode \"Addsub\" to add it to the blur again.");
  ImGui::PopItemWidth();

  bool apply = false;
  if (ImGui::Button("Apply##DIALOGBLURAPPLY")) {
    apply = true;
  } else toolTip("Apply this blur to the layer");

  if (test||(apply && updatedValues)) {
    if (clone->width == l->width && clone->height == l->height && (!locked(l,LOCK_ANY))) {
      updatedValues = false;
      bool layerIncrease = !l->hasSelection();
      if (layerIncrease) {
        l->enlargenLayerBy(increaseLayerWidth, increaseLayerHeight, increaseLayerWidth, increaseLayerHeight);
        clone->enlargenLayerBy(increaseLayerWidth, increaseLayerHeight, increaseLayerWidth, increaseLayerHeight);
      }
      switch(blurType) {
        case BLURTYPE_BOX: {filterBoxBlur(l->pixels,clone->pixels, l->width, l->height, boxWidth, boxHeight,l->pixelStates,l->hasSelection());} break;
        case BLURTYPE_RADIAL: {filterRadialBlur(l->pixels,clone->pixels, l->width, l->height, radialAngle, radialSize,l->pixelStates,l->hasSelection());} break;
        case BLURTYPE_GAUSS: {filterGaussBlur(l->pixels,clone->pixels, l->width, l->height, gaussX, gaussY,l->pixelStates,l->hasSelection());} break;
        case BLURTYPE_ITERATIVE: {filterIterativeBlur(l->pixels,clone->pixels, l->width, l->height, iterativeSteps, l->pixelStates,l->hasSelection());} break;
        case BLURTYPE_MEDIAN: {filterMedianBlur(l->pixels,clone->pixels, l->width, l->height, medianWidth, medianHeight,l->pixelStates,l->hasSelection(), medianCircular, medianContours);} break;
      }
      if (highPass) {
        filterSubtract(l->pixels,clone->pixels,l->pixels,l->width,l->height);
        filterSetAlpha(l->pixels,l->width,l->height,255);
      }
      if (layerIncrease) {
        cropTwo(l, clone);
      }
      repaintLayer(l,true);
    }
  }

  if (apply) {
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    updatedPixels(l);
    afterUndoPoint();
  }
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGBLURCLOSE")) {
    if ((!locked(l,LOCK_ANY)) && clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
  } else toolTip("Don't apply a blur here");
  DIALOGEND()
}

void dialogGlow(bool openIt) {
  static Layer *clone = NULL;
  static float gaussX = 0;
  static float  gaussY = 0;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  static bool updateBlur = false;

  if (openIt) {
    beforeUndoPoint();
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    gaussX = 16;
    gaussY = 16;
    updateBlur = true;
  }
  DIALOGSTART("Turn to glow");

  ImGui::PushItemWidth(64);
  ImGui::Text("Radius X"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGGLOWGAUSSX",&gaussX,0,0,2)) {updateBlur = true;} else toolTip("The gauss cell width of the smoothing"); if (gaussX<0) gaussX = 0; if (gaussX>16384) gaussX = 16384;
  ImGui::Text("Radius Y"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGGLOWGAUSSY",&gaussY,0,0,2)) {updateBlur = true;} else toolTip("The gauss cell height of the smoothing"); if (gaussY<0) gaussY = 0; if (gaussY>16384) gaussY = 16384;
  ImGui::PopItemWidth();

  if (updateBlur) {
    if (!ImGui::IsAnyItemActive()) {
      updateBlur = false;
      if (clone->width == l->width && clone->height == l->height && (!locked(l,LOCK_ANY))) {
        filterGaussBlur(l->pixels,clone->pixels, l->width, l->height, gaussX, gaussY,l->pixelStates,l->hasSelection());
        l->greyScale();
        repaintLayer(l,true);
      }
    }
  }
  if (ImGui::Button("Apply##DIALOGGLOWAPPLY")) {
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    updatedPixels(l);
    afterUndoPoint();
  } else toolTip("Make this layer into a greyscale glow layer");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGGLOWCLOSE")) {
    if ((!locked(l,LOCK_ANY)) && clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
  } else toolTip("Don't change this layer");
  DIALOGEND()
}

void dialogEmboss(bool openIt) {
  static Layer *clone = NULL;
  static float angle = 0;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;

  static bool updateEmboss = false;

  if (openIt) {
    beforeUndoPoint();
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    angle = 0;
    updateEmboss = true;
  }
  DIALOGSTART("Emboss");

  ImGui::PushItemWidth(360);
  ImGui::Text("Angle"); ImGui::SameLine(); if (ImGui::SliderFloat("##DIALOGEMBOSSANGLE",&angle,0,360)) {updateEmboss = true;} else toolTip("The direction of light onto this relief"); if (angle<0) angle = 0; if (angle>360) angle = 360;
  ImGui::PopItemWidth();
  
  if (updateEmboss) {
    if (!ImGui::IsAnyItemActive()) {
      updateEmboss = false;
      if (clone->width == l->width && clone->height == l->height && (!locked(l,LOCK_ANY))) {
        filterEmboss(l->pixels,clone->pixels, l->width, l->height, angle, l->pixelStates,l->hasSelection());
        repaintLayer(l,true);
      }
    }
  }

  if (ImGui::Button("Apply##DIALOGEMBOSSAPPLY")) {
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    updatedPixels(l);
    afterUndoPoint();
  } else toolTip("Make this layer into a relief");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGEMBOSSCLOSE")) {
    if ((!locked(l,LOCK_ANY)) && clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
  } else toolTip("Don't update this layer");
  DIALOGEND()
}

void applyColorMatrix(Layer *dest, Layer *source) {
  if (locked(dest,LOCK_ANY)) return;
  if (dest == NULL || source == NULL || dest->width != source->width || dest->height != source->height) return;
  bool hasSelection = dest->hasSelection();
  float *colorMatrix = dest->colorTransform.colorMatrix;
  for (int i = 0; i < dest->width*dest->height; i++) {
    if ((!hasSelection)||(dest->pixelStates[i] & PIXELSTATE_SELECTED)) {
      unsigned int colS = source->pixels[i];
      int rf = colS & 255;
      int gf = (colS>>8) & 255;
      int bf = (colS>>16) & 255;
      int af = (colS>>24) & 255;
      int r = (int)(rf * colorMatrix[0*4+0] + gf * colorMatrix[1*4+0] + bf * colorMatrix[2*4+0] + colorMatrix[4*4+0]);
      int g = (int)(rf * colorMatrix[0*4+1] + gf * colorMatrix[1*4+1] + bf * colorMatrix[2*4+1] + colorMatrix[4*4+1]);
      int b = (int)(rf * colorMatrix[0*4+2] + gf * colorMatrix[1*4+2] + bf * colorMatrix[2*4+2] + colorMatrix[4*4+2]);
      int a = (int)(af * colorMatrix[3*4+3] + colorMatrix[4*4+3]);
      if (r < 0) r = 0;
      if (g < 0) g = 0;
      if (b < 0) b = 0;
      if (a < 0) a = 0;
      if (r > 255) r = 255;
      if (g > 255) g = 255;
      if (b > 255) b = 255;
      if (a > 255) a = 255;
      dest->pixels[i] = r | (g<<8) | (b<<16) | (a<<24);
    }
  }
}

bool colorMatrixDirect = false;
void dialogColorMatrixDirect(bool openIt) {
  colorMatrixDirect = true;
  dialogColorMatrix(openIt);
  colorMatrixDirect = false;
}

void dialogColorMatrix(bool openIt) {
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  static float redScale=1, blueScale=1, greenScale=1,allScale=1;
  static bool update = false;
  static float colorMatrixSaved[4*5];
  static bool applyDirect = false;
  static Layer *clone = NULL;

  if (openIt) {
    beforeUndoPoint();
    applyDirect = colorMatrixDirect;
    memcpy(colorMatrixSaved,l->colorTransform.colorMatrix,sizeof(colorMatrixSaved));
    memset(l->colorTransform.colorMatrix,0,sizeof(l->colorTransform.colorMatrix));
    l->colorTransform.colorMatrix[0*4+0] = 1;
    l->colorTransform.colorMatrix[1*4+1] = 1;
    l->colorTransform.colorMatrix[2*4+2] = 1;
    l->colorTransform.colorMatrix[3*4+3] = 1;
    if (applyDirect) {
      if (clone != NULL) {delete clone; clone = NULL;}
      clone = l->clone();
    }
  }

  DIALOGSTART("Color matrix");
  dialogLoadSaveOff = true; dialogLoadSave(false,DIALOG_NONE);

  if (roundButton("Greyscale##DIALOGCOLORMATRIXGREYSCALE", true)) {
    memset(l->colorTransform.colorMatrix,0,sizeof(l->colorTransform.colorMatrix));
    for (int y = 0; y < 3; y++) {
      for (int x = 0; x < 3; x++) {
        l->colorTransform.colorMatrix[x*4+y] = 1.0/3.0;
      }
    }
    l->colorTransform.colorMatrix[3*4+3] = 1;
    update = true;
  }
  if (roundButton("Greyscale with luminance##DIALOGCOLORMATRIXGREYSCALELUMINANCE", true)) {
    memset(l->colorTransform.colorMatrix,0,sizeof(l->colorTransform.colorMatrix));
    for (int y = 0; y < 3; y++) {
      for (int x = 0; x < 3; x++) {
        l->colorTransform.colorMatrix[x*4+y] = luminanceValues[x];
      }
    }
    l->colorTransform.colorMatrix[3*4+3] = 1;
    update = true;
  }

  ImGui::Separator();

  ImGui::PushItemWidth(7*7);

  float *v;
  ImGui::Text("Channel red"); 
  ImGui::SameLine(); ImGui::Text(""); ImGui::SameLine(); if (roundButton("Normalize####DIALOGCOLORMATRIXNORMALIZER",true)) {
    update = true;
    Vector k(l->colorTransform.colorMatrix[0*4+0],l->colorTransform.colorMatrix[0*4+1],l->colorTransform.colorMatrix[0*4+2]);
    k = normalize(k);
    l->colorTransform.colorMatrix[0*4+0] = k.x;
    l->colorTransform.colorMatrix[0*4+1] = k.y;
    l->colorTransform.colorMatrix[0*4+2] = k.z;
  } else toolTip("Final red of red,green,blue should sum up to one");
  v = &l->colorTransform.colorMatrix[0*4+0]; ImGui::Text("Into red"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXRR",v,0,0,4)) {update = true;} else toolTip("Transfer this amount of red into red");
  v = &l->colorTransform.colorMatrix[0*4+1]; ImGui::Text("Into green"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXRG",v,0,0,4)) {update = true;} else toolTip("Transfer this amount of red into green");
  v = &l->colorTransform.colorMatrix[0*4+2]; ImGui::Text("Into blue"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXRB",v,0,0,4)) {update = true;} else toolTip("Transfer this amount of red into blue");
  v = &redScale; ImGui::InputFloat("##DIALOGCOLORSCALEVALUER",v,0,0,4); ImGui::SameLine(); if (roundButton("Scale red by####DIALOGCOLORMATRIXSCALER",true)) {
    update = true;
    l->colorTransform.colorMatrix[0*4+0] *= *v;
    l->colorTransform.colorMatrix[0*4+1] *= *v;
    l->colorTransform.colorMatrix[0*4+2] *= *v;
  } else toolTip("Scale the red values");

  ImGui::Separator();

  ImGui::Text("");
  ImGui::Text("Channel green");
  ImGui::SameLine(); ImGui::Text(""); ImGui::SameLine(); if (roundButton("Normalize####DIALOGCOLORMATRIXNORMALIZEG",true)) {
    update = true;
    update = true;
    Vector k(l->colorTransform.colorMatrix[1*4+0],l->colorTransform.colorMatrix[1*4+1],l->colorTransform.colorMatrix[1*4+2]);
    k = normalize(k);
    l->colorTransform.colorMatrix[1*4+0] = k.x;
    l->colorTransform.colorMatrix[1*4+1] = k.y;
    l->colorTransform.colorMatrix[1*4+2] = k.z;
  } else toolTip("Final green of red,green,blue should sum up to one");
  v = &l->colorTransform.colorMatrix[1*4+0]; ImGui::Text("Into red"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXGR",v,0,0,4)) {update = true;} else toolTip("Transfer this amount of green into red");
  v = &l->colorTransform.colorMatrix[1*4+1]; ImGui::Text("Into green"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXGG",v,0,0,4)) {update = true;} else toolTip("Transfer this amount of green into green");
  v = &l->colorTransform.colorMatrix[1*4+2]; ImGui::Text("Into blue"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXGB",v,0,0,4)) {update = true;} else toolTip("Transfer this amount of green into blue");
  v = &greenScale; ImGui::InputFloat("##DIALOGCOLORSCALEVALUEG",v,0,0,4); ImGui::SameLine(); if (roundButton("Scale green by####DIALOGCOLORMATRIXSCALEG",true)) {
    update = true;
    l->colorTransform.colorMatrix[1*4+0] *= *v;
    l->colorTransform.colorMatrix[1*4+1] *= *v;
    l->colorTransform.colorMatrix[1*4+2] *= *v;
  } else toolTip("Scale the green values");

  ImGui::Separator();

  ImGui::Text("");
  ImGui::Text("Channel blue");
  ImGui::SameLine(); ImGui::Text(""); ImGui::SameLine(); if (roundButton("Normalize####DIALOGCOLORMATRIXNORMALIZEB",true)) {
    update = true;
    update = true;
    Vector k(l->colorTransform.colorMatrix[2*4+0],l->colorTransform.colorMatrix[2*4+1],l->colorTransform.colorMatrix[2*4+2]);
    k = normalize(k);
    l->colorTransform.colorMatrix[2*4+0] = k.x;
    l->colorTransform.colorMatrix[2*4+1] = k.y;
    l->colorTransform.colorMatrix[2*4+2] = k.z;
  } else toolTip("Final blue of red,green,blue should sum up to one");
  v = &l->colorTransform.colorMatrix[2*4+0]; ImGui::Text("Into red"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXBR",v,0,0,4)) {update = true;} else toolTip("Transfer this amount of blue into red");
  v = &l->colorTransform.colorMatrix[2*4+1]; ImGui::Text("Into green"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXBG",v,0,0,4)) {update = true;} else toolTip("Transfer this amount of blue into green");
  v = &l->colorTransform.colorMatrix[2*4+2]; ImGui::Text("Into blue"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXBB",v,0,0,4)) {update = true;} else toolTip("Transfer this amount of blue into blue");
  v = &blueScale; ImGui::InputFloat("##DIALOGCOLORSCALEVALUEB",v,0,0,4); ImGui::SameLine(); if (roundButton("Scale blue by####DIALOGCOLORMATRIXSCALEB",true)) {
    update = true;
    l->colorTransform.colorMatrix[2*4+0] *= *v;
    l->colorTransform.colorMatrix[2*4+1] *= *v;
    l->colorTransform.colorMatrix[2*4+2] *= *v;
  } else toolTip("Scale the blue values");

  ImGui::Separator();
  ImGui::Text("");

  v = &allScale; ImGui::InputFloat("##DIALOGCOLORSCALEVALUEALL",v,0,0,4); ImGui::SameLine(); if (roundButton("Scale all by####DIALOGCOLORMATRIXSCALEALL",true)) {
    update = true;
    l->colorTransform.colorMatrix[0*4+0] *= *v;
    l->colorTransform.colorMatrix[0*4+1] *= *v;
    l->colorTransform.colorMatrix[0*4+2] *= *v;
    l->colorTransform.colorMatrix[1*4+0] *= *v;
    l->colorTransform.colorMatrix[1*4+1] *= *v;
    l->colorTransform.colorMatrix[1*4+2] *= *v;
    l->colorTransform.colorMatrix[2*4+0] *= *v;
    l->colorTransform.colorMatrix[2*4+1] *= *v;
    l->colorTransform.colorMatrix[2*4+2] *= *v;
  } else toolTip("Scale red,green,blue values");

  ImGui::Separator();

  ImGui::Text("");
  ImGui::Text("Channel alpha");
  v = &l->colorTransform.colorMatrix[3*4+3]; ImGui::Text("Scale alpha"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXA",v,0,0,4)) {update = true;} else toolTip("Update alpha by this scaling");

  ImGui::Text("");
  v = &l->colorTransform.colorMatrix[4*4+0]; ImGui::Text("Add to red"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXADDR",v,0,0,2)) {update = true;} else toolTip("Add this amount to red (-256..256)");
  v = &l->colorTransform.colorMatrix[4*4+1]; ImGui::Text("Add to green"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXADDG",v,0,0,2)) {update = true;} else toolTip("Add this amount to green (-256..256)");
  v = &l->colorTransform.colorMatrix[4*4+2]; ImGui::Text("Add to blue"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXADDB",v,0,0,2)) {update = true;} else toolTip("Add this amount to blue (-256..256)");
  v = &l->colorTransform.colorMatrix[4*4+3]; ImGui::Text("Add to alpha"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGCOLORMATRIXADDA",v,0,0,2)) {update = true;} else toolTip("Add this amount to alpha (-256..256)");

  ImGui::PopItemWidth();

  if (roundButton("Save##DIALOGCOLORMATRIXSAVE", true)) {
    dialogLoadSave(true, DIALOG_SAVECOLORMATRIX);
  }
  ImGui::SameLine();
  if (roundButton("Load##DIALOGCOLORMATRIXLOAD", true)) {
    dialogLoadSave(true, DIALOG_LOADCOLORMATRIX);
  }
  ImGui::SameLine();
  if (roundButton("Reset##DIALOGCOLORMATRIXRESET", true)) {
    memset(l->colorTransform.colorMatrix,0,sizeof(l->colorTransform.colorMatrix));
    l->colorTransform.colorMatrix[0*4+0] = 1;
    l->colorTransform.colorMatrix[1*4+1] = 1;
    l->colorTransform.colorMatrix[2*4+2] = 1;
    l->colorTransform.colorMatrix[3*4+3] = 1;
    redScale=1;
    greenScale=1;
    blueScale=1;
    allScale=1;
    update = true;
  }
   
  if (shouldUpdateAll) update = true;
  if (update) {
    update = false;
    if (applyDirect) {
      applyColorMatrix(l,clone); // no update is sent to the layers colorgrading, so the old grading stays and the new grading gets "added" here
    } else {
      l->colorTransform.shouldUpdate = true;
    }
    repaintLayer(l,true);
  }

  if (applyDirect) {
    if (ImGui::Button("Apply##DIALOGCOLORMATRIXAPPLY")) {
      if (clone != NULL) {delete clone; clone = NULL;}
      if (applyDirect) memcpy(l->colorTransform.colorMatrix,colorMatrixSaved,sizeof(colorMatrixSaved));
      shouldUpdateAll = true;
      updatedPixels(l);
      afterUndoPoint();
      ImGui::CloseCurrentPopup();
    } else toolTip("Apply this color transformation. no preview");
    ImGui::SameLine();
  }
  
  if (ImGui::Button("Close##DIALOGCOLORMATRIXCLOSE")) {
    if (applyDirect && clone != NULL) {
      if ((!locked(l,LOCK_ANY)) && clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
      if (clone != NULL) {delete clone; clone = NULL;}
      memcpy(l->colorTransform.colorMatrix,colorMatrixSaved,sizeof(colorMatrixSaved));
      repaintLayer(l,true);
    }
    ImGui::CloseCurrentPopup();
  } else {if (!applyDirect) toolTip("Apply this color transformation"); else toolTip("Don't do anything");}
  DIALOGEND()
}

void getQuantizedPaletteKMeans(Array<unsigned int> &palette, int colorCount, Layer *l, bool luminance, int steps) {
  Layer *cloned = l->clone();
  Array<unsigned int> colorsWithClusterId;
  int sizeIncrease = 1024*2;
  int currentIndex = 0;
  int currentSize = 0;
  Array<unsigned char> alreadyFound;
  alreadyFound.resize(0x1000000/8); // 256*256*256/8 = 64kb*32 = 2mb
  memset(&alreadyFound[0],0,alreadyFound.size()*sizeof(unsigned char));
  for (int i = 0; i < cloned->width*cloned->height; i++) {
    unsigned int c = cloned->pixels[i];
    int k = c & 0xffffff;
    int pos8 = k / 8;
    int pos7 = k & 7;
    unsigned char b = (unsigned char)(alreadyFound[pos8] & (1<<pos7));
    if (!b) {   
      alreadyFound[pos8] |= (unsigned char)(1<<pos7);
      if (currentIndex+2 >= currentSize) {
        colorsWithClusterId.resize(colorsWithClusterId.size()+sizeIncrease);
        currentSize = (int)colorsWithClusterId.size();
      }
      colorsWithClusterId[currentIndex+0] = c;
      colorsWithClusterId[currentIndex+1] = 0;
      currentIndex += 2;
    }
  }
  colorsWithClusterId.resize(currentIndex);
  if ((int)colorsWithClusterId.size()/2<colorCount) colorCount = (int)colorsWithClusterId.size()/2;
  if (colorCount == 0) return;

#define KMEANSCHANNELS 4
  Array<unsigned int> clusterColorsRGBAW; clusterColorsRGBAW.resize(colorCount*KMEANSCHANNELS);
  srand(0); for (int i2 = 0; i2 < (int)clusterColorsRGBAW.size(); i2++) clusterColorsRGBAW[i2] = (i2/KMEANSCHANNELS)*255/colorCount;

  float sr = luminance ? luminanceValues[0] : 1;
  float sg = luminance ? luminanceValues[1] : 1;
  float sb = luminance ? luminanceValues[2] : 1;

  initStatusBar();
  for (int step = 0; step < steps; step++) {

    for (int i3 = 0; i3 < (int)colorsWithClusterId.size()/2; i3++) {
      unsigned int col = colorsWithClusterId[i3*2+0];
      float r = col & 255;
      float g = (col>>8) & 255;
      float b = (col>>16) & 255;
      r /= sr;
      g /= sg;
      b /= sb;
      float ld = 256*256*256;
      int cluster = 0;
      for (int i4 = 0; i4 < (int)clusterColorsRGBAW.size()/KMEANSCHANNELS; i4++) {
        float r2 = clusterColorsRGBAW[i4*KMEANSCHANNELS+0];
        float g2 = clusterColorsRGBAW[i4*KMEANSCHANNELS+1];
        float b2 = clusterColorsRGBAW[i4*KMEANSCHANNELS+2];
        r2 /= sr;
        g2 /= sg;
        b2 /= sb;
        float dr = r2 - r;
        float dg = g2 - g;
        float db = b2 - b;
        float d = dr * dr + dg * dg + db * db;
        if (d < ld) {
          ld = d;
          cluster = i4;
        }
      }
      colorsWithClusterId[i3*2+1] = cluster;
    }
  
    for (int i5 = 0; i5 < (int)clusterColorsRGBAW.size(); i5++) clusterColorsRGBAW[i5] = 0;
  
    for (int i6 = 0; i6 < (int)colorsWithClusterId.size()/2; i6++) {
      unsigned int col = colorsWithClusterId[i6*2+0];
      unsigned int c = colorsWithClusterId[i6*2+1];
      int r = col & 255;
      int g = (col>>8) & 255;
      int b = (col>>16) & 255;
      //int a = (col>>24) & 255;
      clusterColorsRGBAW[c*KMEANSCHANNELS+0] += r;
      clusterColorsRGBAW[c*KMEANSCHANNELS+1] += g;
      clusterColorsRGBAW[c*KMEANSCHANNELS+2] += b;
      clusterColorsRGBAW[c*KMEANSCHANNELS+3] += 1;
    }
  
    for (int i7 = 0; i7 < (int)clusterColorsRGBAW.size()/KMEANSCHANNELS; i7++) {
      int w = clusterColorsRGBAW[i7*KMEANSCHANNELS+3];
      if (w != 0) {
        clusterColorsRGBAW[i7*KMEANSCHANNELS+0] /= w;
        clusterColorsRGBAW[i7*KMEANSCHANNELS+1] /= w;
        clusterColorsRGBAW[i7*KMEANSCHANNELS+2] /= w;
      }
    }

    for (int i9 = 0; i9 < (int)clusterColorsRGBAW.size()/KMEANSCHANNELS; i9++) {
      int r0 = clusterColorsRGBAW[i9*KMEANSCHANNELS+0];
      int g0 = clusterColorsRGBAW[i9*KMEANSCHANNELS+1];
      int b0 = clusterColorsRGBAW[i9*KMEANSCHANNELS+2];
      for (int i10 = i9+1; i10 < (int)clusterColorsRGBAW.size()/KMEANSCHANNELS; i10++) {
        int r1 = clusterColorsRGBAW[i10*KMEANSCHANNELS+0];
        int g1 = clusterColorsRGBAW[i10*KMEANSCHANNELS+1];
        int b1 = clusterColorsRGBAW[i10*KMEANSCHANNELS+2];
        if (r1 != r0) continue;
        if (g1 != g0) continue;
        if (b1 != b0) continue;
        clusterColorsRGBAW[i10*KMEANSCHANNELS+0] = rand() & 255;
        clusterColorsRGBAW[i10*KMEANSCHANNELS+1] = rand() & 255;
        clusterColorsRGBAW[i10*KMEANSCHANNELS+2] = rand() & 255;
      }
    }
    updateStatusBar();
  }

  palette.resize(clusterColorsRGBAW.size()/KMEANSCHANNELS);
  for (int i8 = 0; i8 < (int)palette.size(); i8++) {
    int r = clusterColorsRGBAW[i8*KMEANSCHANNELS+0];
    int g = clusterColorsRGBAW[i8*KMEANSCHANNELS+1];
    int b = clusterColorsRGBAW[i8*KMEANSCHANNELS+2];
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    palette[i8] = r|(g<<8)|(b<<16)|0xff000000;
  }

  delete cloned;
}

Array<ImVec4> lastQuantizedPalette;

void dialogQuantizePalette(bool openIt) {
  static Layer *clone = NULL;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  static int colorCount = 0;
  static bool useLuminance = false;
  if (openIt) {
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    colorCount = 256;
    useLuminance = false;
  }
  DIALOGSTART("Quantized palette");

  ImGui::PushItemWidth(64);
  ImGui::Text("Quantization using KMeans (not precise)");
  ImGui::Text("Number of colors"); ImGui::SameLine(); ImGui::InputInt("##DIALOGQUANTIZEPALETTECOLORCOUNT",&colorCount,0,0); toolTip("The color count of the resulting image(1..256)"); if (colorCount < 1) colorCount = 1; if (colorCount > 256) colorCount = 256;
  ImGui::Text("Use luminance"); ImGui::SameLine(); ImGui::Checkbox("##DIALOGQUANTIZEPALETTELUMINANCE",&useLuminance); toolTip("Not just simple greyscale for comparisons");
  ImGui::PopItemWidth();

  if (roundButton("Preview",true) && (!locked(l,LOCK_ANY))) {
    Array<unsigned int> palette;
    getQuantizedPaletteKMeans(palette, colorCount, clone, useLuminance,32);
    if (l->width == clone->width && l->height == clone->height) {
      memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
      l->quantize(palette);
      repaintLayer(l,true);
    }
  } else toolTip("Generate a palette and show a preview in the layer");

  if (ImGui::Button("Save palette##DIALOGQUANTIZEPALETTESAVE")) {
    Array<unsigned int> palette;
    getQuantizedPaletteKMeans(palette, colorCount, clone, useLuminance,32);
    static Array<ImVec4> quantizedPalette;
    quantizedPalette.resize(palette.size());
    for (int i = 0; i < (int)quantizedPalette.size(); i++) {
      quantizedPalette[i] = getImVec4Color(palette[i]);
    }
    if ((!locked(l,LOCK_ANY)) && clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    shouldUpdateAll = true;
    ImGui::CloseCurrentPopup();
    sortPalette(quantizedPalette);
    paletteToSave = &quantizedPalette; dialogLoadSave(true,DIALOG_SAVEPALETTE);
  } else toolTip("Generate the palette and save it for later use");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGQUANTIZEPALETTECLOSE")) {
    if ((!locked(l,LOCK_ANY)) && clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    shouldUpdateAll = true;
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't do anything");
  DIALOGEND()
}

void dialogDuplicate(bool openIt) {
  DIALOGSTART("Duplicate layers");

  Array<Layer *> layers;
  bool one = false; for (int i = 0; i < (int)currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected) {one = true; layers.push_back(currentCanvas->layers[i]);}
  if (!one) for (int i2 = 0; i2 < (int)currentCanvas->layers.size(); i2++) if (currentCanvas->layers[i2]->state.active) {layers.push_back(currentCanvas->layers[i2]);}

  if (layers.empty()) {
    ImGui::Text("No layers selected or active");
  } else {
    noUndoMessage();
    ImGui::Text("Layers to duplicate");
    ImGui::BeginChild("DUPLICATELAYERSCHILD",ImVec2(main_window_xres/2,main_window_yres/3));
    for (int i = 0; i < (int)layers.size(); i++) {
      ImGui::Text("\"%s\"",layers[i]->name);
    }
    ImGui::EndChild();
  }

  if (!layers.empty()) {
    if (ImGui::Button("Duplicate these layers##DIALOGDUPLICATELAYERSDUPLICATE")) {
      currentCanvas->duplicateLayers();
      ImGui::CloseCurrentPopup();
    } else toolTip("Place duplicates of these layers directly above the selected layers");
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGDUPLICATELAYERSCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't do anything");
  DIALOGEND()
}

void dialogExtrudeSelection(bool openIt) {
  static Layer *clone = NULL;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  static bool updateIt = false;
  static float angle = 0;
  static float size = 10;
  const float maxLength = 100;
  if (openIt) {
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    memcpy(clone->pixelStates,l->pixelStates,clone->width*clone->height*sizeof(unsigned char)); // pixelStates get not cloned by default
    beforeUndoPoint();
    updateIt = true;
  }
  DIALOGSTART("Extrude selection");

  if (!clone->hasSelection()) {
    ImGui::Text("Nothing selected on this layer.");
  } else {
    ImGui::PushItemWidth(360);
    ImGui::Text("Angle"); ImGui::SameLine(); if (ImGui::SliderFloat("##DIALOGEXTRUDESELECTIONANGLE",&angle,0,360)) {updateIt = true;} else toolTip("A direction in which to expand the selection by"); if (angle<0) angle = 0; if (angle>360) angle = 360;
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(7*8);
    ImGui::Text("Length"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGEXTRUDESELECTIONLENGTH",&size,0,0,0)) {updateIt = true;} else toolTip("The length in pixels of the expansion"); if (size<0) size = 0; if (size>maxLength) size = maxLength;
    ImGui::PopItemWidth();

    if (updateIt) {
      if (!ImGui::IsAnyItemActive()) {
        updateIt = false;
        if (clone->width==l->width&&clone->height==l->height) {
          double sx = sin(-angle*2*PI/360.0);
          double sy = cos(-angle*2*PI/360.0);
          int s = (int)floor(size);
          initStatusBar();
          for (int y = 0; y < l->height; y++) {
            if ((y & 63)==0) updateStatusBar();
            for (int x = 0; x < l->width; x++) {
              bool selected = false;
              for (int i = s-1; i >= 0; i--) {
                int lx = (int)(x + sx * i);
                int ly = (int)(y + sy * i);
                if ((unsigned int)lx < (unsigned int)clone->width && (unsigned int)ly < (unsigned int)clone->height) {
                  unsigned char p = clone->pixelStates[lx+ly*clone->width];
                  selected = p & PIXELSTATE_SELECTED ? true : false;
                  if (selected) break;
                }
              }
              l->pixelStates[x+y*l->width] &= ~PIXELSTATE_SELECTED;
              if (selected) l->pixelStates[x+y*l->width] |= PIXELSTATE_SELECTED;
            }
          }
        }
        repaintLayer(l,true);
      }
    }

  
    if (ImGui::Button("Apply##DIALOGEXTRUDESELECTIONAPPLY")) {
      if (clone != NULL) {delete clone; clone = NULL;}
      ImGui::CloseCurrentPopup();
      shouldUpdateAll = true;
      updatedPixelStates(l);
      afterUndoPoint();
    } else toolTip("Expand the selection");
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGEXTRUDESELECTIONCLOSE")) {
    if (clone->width==l->width&&clone->height==l->height) memcpy(l->pixelStates,clone->pixelStates,l->width*l->height*sizeof(unsigned char));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
  } else toolTip("Don't expand the selection");
  DIALOGEND()
}

enum KEYINGTYPES {
  KEYINGTYPE_SIMPLE = 0,
  KEYINGTYPE_COLOR = 1,
  KEYINGTYPE_COUNT = 2
};
const char *keyingTypeNames[KEYINGTYPE_COUNT] = {"Simple","Color"};

void dialogKeying(bool openIt) {
  static Layer *clone = NULL;
  Layer *l = currentCanvas->getActiveLayer(); if (l == NULL) return;
  static bool updateIt = false;
  static bool binary = false;
  static bool invert = false;
  static int channel = 0;
  static float factor = 0;
  static float factor2 = 0;
  static int type = 0;
  if (openIt) {
    if (clone != NULL) {delete clone; clone = NULL;}
    clone = l->clone();
    beforeUndoPoint();
    updateIt = true;
    channel = 1;
    factor = 4;
    factor2 = 1;
    binary = false;
    invert = false;
    type = KEYINGTYPE_SIMPLE;
  }
  DIALOGSTART("Keying");
  ImGui::PushItemWidth(7*12);
  ImGui::Text("Type");ImGui::SameLine();if (ImGui::Combo("##DIALOGKEYINGTYPE",&type, keyingTypeNames, KEYINGTYPE_COUNT)) {updateIt = true;} else toolTip("The type of keying that should be applied");
  ImGui::PopItemWidth();
  switch(type) {
    case KEYINGTYPE_SIMPLE: {
      ImGui::Text("Simple channel keying");
      ImGui::Text("Red"); ImGui::SameLine(); if (ImGui::RadioButton("##DIALOGKEYINGCHANNEL0", &channel, 0)) {updateIt = true;} else toolTip("Just filter out the red parts");
      ImGui::Text("Green"); ImGui::SameLine(); if (ImGui::RadioButton("##DIALOGKEYINGCHANNEL1", &channel, 1)) {updateIt = true;} else toolTip("Just filter out the green parts");
      ImGui::Text("Blue"); ImGui::SameLine(); if (ImGui::RadioButton("##DIALOGKEYINGCHANNEL2", &channel, 2)) {updateIt = true;} else toolTip("Just filter out the blue parts");
      ImGui::PushItemWidth(300);
      ImGui::Text("Factor"); if (ImGui::SliderFloat("##DIALOGKEYINGFACTOR",&factor,0,8)) {updateIt = true;} else toolTip("Intensity of the keying"); if (factor<0) factor = 0;
      ImGui::PopItemWidth();
    } break;
    case KEYINGTYPE_COLOR: {
      ImGui::Text("Color keying");
      colorItem("","##DIALOGKEYINGCOLOR0",&foregroundColor);
      ImGui::PushItemWidth(300);
      ImGui::Text("Factor"); if (ImGui::SliderFloat("##DIALOGKEYINGFACTOR2",&factor2,0,8)) {updateIt = true;} else toolTip("Intensity of the keying"); if (factor2<0) factor2 = 0;
      ImGui::PopItemWidth();
    } break;
  }
  ImGui::Text("Binary");ImGui::SameLine(); if (ImGui::Checkbox("##DIALOGKEYINGBINARY",&binary)) {updateIt = true;} else toolTip("The keying should only be on/off (alpha = 0 or alpha = 255)");
  ImGui::SameLine();ImGui::Text("Invert");ImGui::SameLine(); if (ImGui::Checkbox("##DIALOGKEYINGINVERT",&invert)) {updateIt = true;} else toolTip("The keying should be inverted (1=0 and 0=1)");

  if (updateIt) {
    if (!ImGui::IsAnyItemActive()) {
      updateIt = false;
      if (clone->width == l->width && clone->height == l->height && (!locked(l,LOCK_ANY))) {
        switch(type) {
          case KEYINGTYPE_SIMPLE: {filterSimpleKeying(l->pixels,clone->pixels, l->width, l->height, channel, factor, l->pixelStates,l->hasSelection(),l->mode,binary,invert);} break;
          case KEYINGTYPE_COLOR: {filterColorKeying(l->pixels,clone->pixels, l->width, l->height, getUIntColor(foregroundColor), factor2, l->pixelStates,l->hasSelection(),l->mode,binary,invert);} break;
        }
        repaintLayer(l,true);
      }
    }
  }

  if (ImGui::Button("Apply##DIALOGKEYINGAPPLY")) {
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
    updatedPixels(l);
    afterUndoPoint();
  } else toolTip("Apply the current keying settings to the layer");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGKEYINGCLOSE")) {
    if (clone->width==l->width&&clone->height==l->height) memcpy(l->pixels,clone->pixels,l->width*l->height*sizeof(unsigned int));
    if (clone != NULL) {delete clone; clone = NULL;}
    ImGui::CloseCurrentPopup();
    shouldUpdateAll = true;
  } else toolTip("Don't apply any keying");
  DIALOGEND()
}

void dialogSelectByAlpha(bool openIt) {
  static int alphaThresholdMin = 0;
  static int alphaThresholdMax = 128;
  DIALOGSTART("Select by alpha");
  ImGui::PushItemWidth(64+16*2);
  ImGui::Text("Alpha >= min"); ImGui::SameLine(); ImGui::InputInt("##DIALOGSELECTBYALPHAMIN",&alphaThresholdMin); toolTip("The selection will no be made with alpha values below this");
  ImGui::Text("Alpha <= max"); ImGui::SameLine(); ImGui::InputInt("##DIALOGSELECTBYALPHAMAX",&alphaThresholdMax); toolTip("The selection will no be made with alpha values above this");
  if (alphaThresholdMin<0) alphaThresholdMin = 0;
  if (alphaThresholdMin>255) alphaThresholdMin = 255;
  if (alphaThresholdMax<0) alphaThresholdMax = 0;
  if (alphaThresholdMax>255) alphaThresholdMax = 255;
  ImGui::PopItemWidth();
  if (ImGui::Button("Apply##DIALOGSELECTBYALPHAAPPLY")) {
    beforeUndoPoint();
    currentCanvas->selectByAlpha(alphaThresholdMin,alphaThresholdMax);
    afterUndoPoint();
    ImGui::CloseCurrentPopup();
  } else toolTip("Create a selection with this setting");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGSELECTBYALPHACLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't create a selection here");
  DIALOGEND();
}

void dialogBakeLayers(bool openIt) {
  static int interpolation = INTERPOLATION_NONE;
  DIALOGSTART("Bake layers");
  Array<Layer *> layers;
  bool one = false; for (int i = 0; i < (int)currentCanvas->layers.size(); i++) if (currentCanvas->layers[i]->state.selected) {one = true; layers.push_back(currentCanvas->layers[i]);}
  if (!one) for (int i2 = 0; i2 < (int)currentCanvas->layers.size(); i2++) if (currentCanvas->layers[i2]->state.active) {layers.push_back(currentCanvas->layers[i2]);}

  if (layers.empty()) {
    ImGui::Text("No layers selected or active");
  } else {
    ImGui::Text("You may lose pixel information through baking");
    ImGui::Text("and the layers may be rotated/resized when saving the layers!");
    ImGui::Text("");
    ImGui::Text("Layers to bake");
    ImGui::BeginChild("DIALOGBAKELAYERSCHILD",ImVec2(main_window_xres/2,main_window_yres/3));
    for (int i = 0; i < (int)layers.size(); i++) {
      ImGui::Text("\"%s\"",layers[i]->name);
    }
    ImGui::EndChild();
    ImGui::PushItemWidth(7*12);
    ImGui::Text("Interpolation");ImGui::SameLine();ImGui::Combo("##DIALOGBAKELAYERSINTERPOLATION",&interpolation, interpolationNames, INTERPOLATION_COUNT);
  ImGui::SameLine(); ImGui::Text("%s",interpolationDescs[interpolation]);
    ImGui::PopItemWidth();
  }

  if (!layers.empty()) {
    if (ImGui::Button("Bake these layers##DIALOGBAKELAYERSAPPLY")) {
      beforeUndoPoint();
      for (int i = 0; i < (int)layers.size(); i++) {
        Layer *l = layers[i];
        if (l->type == LAYERTYPE_TEXT) {
          l->type = LAYERTYPE_BASIC;
          l->state.lockBits = LOCK_NONE;
        }
        l->bakeLayer(interpolation);
      }
      afterUndoPoint();
      shouldUpdateAll = true;
      ImGui::CloseCurrentPopup();
    } else toolTip("Create new layers without transformations and place it instead of the layers");
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGBAKELAYERSCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't bake any layers");
  DIALOGEND()
}

enum IMAGEMEASURES {
  IMAGEMEASURE_PIXELS = 0,
  IMAGEMEASURE_CM = 1,
  IMAGEMEASURE_INCHES = 2,
  IMAGEMEASURE_COUNT = 3
};
const char *imageMeasureNames[IMAGEMEASURE_COUNT] = {"Pixels","Centimeters","Inches"};

void dialogNewImage(bool openIt) {
  static bool keepAspect = true;
  static int w = 1920,h = 1080;
  static int type = IMAGEMEASURE_PIXELS;
  static int dpi = 72;
  static float wcm = 29.7; static float hcm = 21.0;
  static float winches = 11; static float hinches = 8.5;
  static float oneInch = 2.54;
  static char imageName[PICTURENAMEMAX];
  static double aspectPixels = 1;
  static double aspectCm = 1;
  static double aspectInches = 1;
  if (openIt) {
    static int counter = 0; counter++;
    sprintf(imageName,"New image %d",counter % 1023);
    aspectPixels = h != 0 ? (double)w / h : 1;
    aspectCm = hcm != 0 ? (double)wcm / hcm : 1;
    aspectInches = hinches != 0 ? (double)winches / hinches : 1;
  }
  DIALOGSTART("New picture");
  ImGui::Text("Picture name"); ImGui::SameLine(); DIALOGKEYSHERE(); ImGui::InputText("##DIALOGNEWIMAGENAME",imageName,PICTURENAMEMAX); toolTip("The name of the picture");
  ImGui::PushItemWidth(7*14);
  ImGui::Text("Type"); ImGui::SameLine(); ImGui::Combo("##DIALOGNEWIMAGETYPE",&type, imageMeasureNames, IMAGEMEASURE_COUNT); toolTip("The measure type for the image creation");
  ImGui::PopItemWidth();
  ImGui::PushItemWidth(64+16*2);
  switch(type) {
    case IMAGEMEASURE_PIXELS: {
      ImGui::Text("Picture width"); ImGui::SameLine(); if (ImGui::InputInt("##DIALOGNEWIMAGEWIDTH",&w)) {if (keepAspect) h = w/aspectPixels;} else toolTip("The picture width in pixels");
      ImGui::Text("Picture height"); ImGui::SameLine(); if (ImGui::InputInt("##DIALOGNEWIMAGEHEIGHT",&h)) {if (keepAspect) w = h*aspectPixels;} else toolTip("The picture height in pixels");
    } break;
    case IMAGEMEASURE_CM: {
      ImGui::Text("Picture dpi"); ImGui::SameLine(); ImGui::InputInt("##DIALOGNEWIMAGEDPI",&dpi); toolTip("The dots per inch (dpi) of the picture");
      ImGui::Text("Picture width in cm"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGNEWIMAGEWIDTHCM",&wcm,0,0,2)) {if (keepAspect) hcm = wcm/aspectCm;} toolTip("The width of the picture in centimeters");
      ImGui::Text("Picture height in cm"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGNEWIMAGEHEIGHTCM",&hcm,0,0,2)) {if (keepAspect) wcm = hcm*aspectCm;} toolTip("The height of the picture in centimeters");
      w = wcm / oneInch * dpi;
      h = hcm / oneInch * dpi;
      ImGui::Text("Pixels width %d height %d",w,h);
    } break;
    case IMAGEMEASURE_INCHES: {
      ImGui::Text("Picture dpi"); ImGui::SameLine(); ImGui::InputInt("##DIALOGNEWIMAGEDPI",&dpi); toolTip("The dots per inch (dpi) of the picture");
      ImGui::Text("Picture width in inches"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGNEWIMAGEWIDTHINCHES",&winches,0,0,2)) {if (keepAspect) hinches = winches/aspectInches;} toolTip("The width of the picture in inch");
      ImGui::Text("Picture height in inches"); ImGui::SameLine(); if (ImGui::InputFloat("##DIALOGNEWIMAGEHEIGHTINCHES",&hinches,0,0,2)) {if (keepAspect) winches = hinches*aspectInches;} toolTip("The height of the picture in inch");
      w = winches * dpi;
      h = hinches * dpi;
      ImGui::Text("Pixels width %d height %d",w,h);
    } break;
  }
  ImGui::PopItemWidth();

  ImGui::Text("Preserve proportions");ImGui::SameLine(); if (ImGui::Checkbox("##DIALOGNEWIMAGEKEEPASPECT",&keepAspect)) {
    if (keepAspect) {
      aspectPixels = h != 0 ? (double)w / h : 1;
      aspectCm = hcm != 0 ? (double)wcm / hcm : 1;
      aspectInches = hinches != 0 ? (double)winches / hinches : 1;
    }
  } else toolTip("Keep the images x vs y ratio the same when entering new width or height");

  if (ImGui::Button("Create##DIALOGNEWIMAGECREATE")) {
    newPicture(w,h,String(imageName));
    ImGui::CloseCurrentPopup();
  } else toolTip("Create a new picture");
  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGNEWIMAGECLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't create a new picture");
  DIALOGEND()
}

void dialogGetLayersFromPicture(bool openIt) {
  static int selected = -1;
  if (openIt) {
    selected = -1;
  }
  DIALOGSTART("Get selected layers from other picture");
  if ((int)canvas.size()>1) {
    ImGui::Text("Insert layers from this picture");
  } else {
    ImGui::Text("No other picture available.");
  }
  ImGui::BeginChild("CHILDDIALOGGETLAYERSFROMPICTURE",ImVec2(main_window_xres/2,main_window_yres/3));
  for (int i = 0; i < (int)canvas.size(); i++) {
    if (canvas[i] != currentCanvas) {
      int count = 0;
      for (int i2 = 0; i2 < (int)canvas[i]->layers.size(); i2++) {
        if (canvas[i]->layers[i2]->state.selected||canvas[i]->layers[i2]->state.active) {
          count++;
        }
      }
  
      if (ImGui::Button(combined(String(selected == i ? ">" : " ")+String("\"")+String(canvas[i]->pictureName) + String("\"  selected layers:") + String::fromInt(count) + String("##DIALOGGETLAYERSFROMPICTURELINE")).c_str())) {
        selected = i;
      }
    }
  }
  ImGui::EndChild();

  if (selected >= 0 && selected < (int)canvas.size()) {
    if (ImGui::Button("Get layers##DIALOGGETLAYERSFROMPICTUREGETLAYERS")) {
      int pos = 0;
      for (int i = 0; i < (int)currentCanvas->layers.size(); i++) {
        if (currentCanvas->layers[i]->state.selected||currentCanvas->layers[i]->state.active) {
          pos = i;
          break;
        }
      }
      Canvas *otherCanvas = canvas[selected];
      for (int i2 = 0; i2 < (int)otherCanvas->layers.size(); i2++) {
        Layer *l = otherCanvas->layers[i2];
        if (l->state.active || l->state.selected) {
          Layer *l2 = l->clone();
          currentCanvas->insertLayer(l2,pos); pos++;
        }
      }
      shouldUpdateAll = true;
      ImGui::CloseCurrentPopup();
    } else toolTip("Get the layers from the other picture and place them above the current selection");
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGGETLAYERSFROMPICTURECLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't get any layers");
  DIALOGEND()
}

void dialogUpdateLayer(bool openIt) {
  static int expandLeft = 0, expandRight = 0, expandTop = 0, expandBottom = 0;
  DIALOGSTART("Update layer");
  Layer *l = currentCanvas->getActiveLayer();
  if (l != NULL) {
    ImGui::Text("Layer type:%s", layerTypeNames[l->type]);
    switch(l->type) {
      case LAYERTYPE_TILEMAP: {
        Layer *t = currentCanvas->getLayerById(l->tileSourceLayerId);
        if (t == NULL) ImGui::Text("This layers tileset was deleted"); else ImGui::Text("Has tileset: \"%s\"",t->name);
      } break;
    }
    if (l->sourceLayerId != 0) {Layer *r = currentCanvas->getLayerById(l->sourceLayerId); if (r == NULL) ImGui::Text("This layers source/reference data was deleted"); else ImGui::Text("Displays as a reference of layer: \"%s\"",r->name);}
    ImGui::Text("");
    ImGui::Text("Old layer pixel data width %d\n", l->width);
    ImGui::Text("Old layer pixel data height %d\n", l->height);
    ImGui::Text("");
    int newWidth = l->width + expandLeft + expandRight;
    int newHeight = l->height + expandTop + expandBottom;
    if (newWidth < 1) {newWidth = 1; expandLeft = 0; expandRight = -l->width+1;}
    if (newHeight < 1) {newHeight = 1; expandTop = 0; expandBottom = -l->height+1;}
    ImGui::Text("New layer pixel data width %d\n", newWidth);
    ImGui::Text("New layer pixel data height %d\n", newHeight);
    ImGui::Text("");
    ImGui::PushItemWidth(64+16*2);
    ImGui::Text("Expand top"); ImGui::SameLine(); ImGui::InputInt("##DIALOGUPDATELAYEREXPANDTOP",&expandTop); toolTip("Expand the layer by this pixel count on the top");
    ImGui::Text("Expand left"); ImGui::SameLine(); ImGui::InputInt("##DIALOGUPDATELAYEREXPANDLEFT",&expandLeft); toolTip("Expand the layer by this pixel count on the left");
    ImGui::Text("Expand bottom"); ImGui::SameLine(); ImGui::InputInt("##DIALOGUPDATELAYEREXPANDBOTTOM",&expandBottom); toolTip("Expand the layer by this pixel count on the bottom");
    ImGui::Text("Expand right"); ImGui::SameLine(); ImGui::InputInt("##DIALOGUPDATELAYEREXPANDRIGHT",&expandRight); toolTip("Expand the layer by this pixel count on the right");
    ImGui::PopItemWidth();
    ImGui::Text("");
    if (ImGui::Button("Apply##DIALOGUPDATELAYERAPPLY")) {
      beforeUndoPoint();
      l->enlargenLayerBy(expandLeft,expandTop,expandRight,expandBottom);
      afterUndoPoint();
      ImGui::CloseCurrentPopup();
    } else toolTip("Update the layer");
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGUPDATELAYERCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't update the layer");
  DIALOGEND()
}

void exportLayersRawDialogOpen(const char *shortPath, const char *longFileName) {
  dialogShortPath = String(shortPath);
  dialogLongFileName = String(longFileName);
  dialogExportLayersRaw(true);
}

typedef void (*LOADSAVEFUNC)(const char *shortPath, const char *longFileName);
const char *imgs = ".png,.jpg,.tga,.bmp,.psd,.gif,.pic,.pnm,.hdr,.svg,.pcx,.lbm";
const char *png = ".png";
const char *psd = ".psd";
const char *all = "*.*";
const char *pal = "*.pa2";
const char *grad = "*.gr2";
const char *brush = "*.br2";
const char *svg = "*.svg";
const char *img = "*.wyp";
const char *fnt = "*.ttf";
const char *tilemap = "*.tm2";
const char *gradientCrv = "*.cr2";
const char *colorMatrix = "*.cm2";
const char *txt = "*.txt";
const char *lut = "*.cube";
const char *tga = "*.tga";
const char *cmapped = ".tga,.pcx,.lbm";
const char *calib = "*.cl2";
const char *dialogCaption[DIALOG_COUNT] = {"NO MODE","Import image","Insert image layer(s)","Export layer(s)","Export png","Load palette","Save palette","Export layer(s) raw","Insert psd","Load gradient","Save gradient","Insert svg","Load brush shape svg","Load brush","Save brush","Load brushes","Save brushes","Load image","Save image", "Add font", "Save font","Load gradient curves","Save gradient curves","Load tiling anim","Save tiling anim","Load tilemap","Save tilemap","Select gallery folder","Import folder into single layer","Load color matrix","Save color matrix","Load histogram","Color transfer","Load histogram per channel","Save text","Copy/paste text","Load UTF8 text","LUT Color lookup","Export (paletted) tga","Palette from (paletted) picture", "Load calibration", "Save calibration"};
const char *proceedButton[DIALOG_COUNT] = {"<button>", "Load","Load","Save","Save","Load","Save","Save","Load","Load","Save","Load","Load","Load","Save","Load","Save","Load","Save","Load","Save","Load","Save","Load","Save","Load","Save","Select","Select","Load","Save","Load","Load","Load","Save","Load","Load","Load","Save","Load","Load","Save"};
const char *dialogFileTypes[DIALOG_COUNT] = {"",imgs,imgs,png,png,pal,pal,png,psd,grad,grad,svg,svg,brush,brush,brush,brush,img,img,fnt,fnt,gradientCrv,gradientCrv,png,png,tilemap,tilemap,"","",colorMatrix,colorMatrix,imgs,imgs,imgs,txt,txt,txt,lut,tga,cmapped,calib,calib};
const char *dialogDefaultExtension[DIALOG_COUNT] = {"","","",".png",".png",".pa2",".pa2",".png",".psd",".gr2",".gr2",".svg",".svg",".br2",".br2",".br2",".br2",".wyp",".wyp",".ttf",".ttf",".cr2",".cr2",".png",".png",".tm2",".tm2","","",".cm2",".cm2",".png",".png",".png",".txt",".txt",".txt",".cube",".tga",".tga",".cl2",".cl2"};
LOADSAVEFUNC funcs[DIALOG_COUNT] = {NULL, importImage,insertImageLayers,exportLayers,exportPNG,loadPalette,savePalette,exportLayersRawDialogOpen,insertPSD,loadGradient,saveGradient,insertSVG,loadBrushShape,loadBrush,saveBrush,loadBrushes,saveBrushes,loadImage,saveImage,loadFont,saveFont,loadGradientCurve,saveGradientCurve,loadTilingAnim,saveTilingAnim,loadTileMap,saveTileMap,selectGalleryFolder,importFolder,loadColorMatrix,saveColorMatrix,histogramMatch,colorTransfer,histogramMatchingColored,saveText,loadText,loadTextUTF8,loadLUT,exportTGA,paletteFromPicture,loadCalibration,saveCalibration};

void sortFileNames(Array<File> &files, int column, bool sortOrder) {
  for (int i = 0; i < (int)files.size(); i++) {
    for (int j = i+1; j < (int)files.size(); j++) {
      bool bigger = false;
      String nameHere = (files[i].name.name + files[i].name.extension);
      bool dirHere = files[i].attributes & DOS_ATTR_SUBDIR;
      bool dirThere = files[j].attributes & DOS_ATTR_SUBDIR;
      switch(dirHere?0:column) {
      case 0: bigger = nameHere < (files[j].name.name + files[j].name.extension); break;
      case 1: bigger = (files[i].name.extension + files[i].name.name) < (files[j].name.extension + files[j].name.name); break;
      case 2: bigger = files[i].size < files[j].size; break;
      case 3: bigger = files[i].date < files[j].date; break;
      }
      if ((!dirHere)&&sortOrder) bigger = !bigger;
      if (dirHere != dirThere) {
        if (dirHere) bigger = false; else bigger = true;
      }
      if (bigger) {
        File t = files[i];
        files[i] = files[j];
        files[j] = t;
      }
    }
  }
}

unsigned int dialogLoadSaveGameObjectUniqueId = 0;
String dialogLoadSaveSetFileNameAndPath="";
int dialogLoadSaveModeId = 0;

LOADSAVEFUNC loadSaveFunc = NULL; 
String loadSaveShortPath; 
String loadSaveLongFileName;

void callLoadSaveFunc() {
  if (loadSaveFunc == NULL) {dialogLoadSaveModeId = 0;return;}
  loadSaveFunc(loadSaveShortPath.c_str(), loadSaveLongFileName.c_str());
  dialogLoadSaveModeId = 0;
}

bool dialogOverwriteDone = false;

void dialogOverwrite(bool openIt) {
  if (openIt) {
    dialogOverwriteDone = false;
    String shortPath = shortFileName(loadSaveShortPath+loadSaveLongFileName);
    FILE *in = fopen(dosFileName(shortPath).c_str(),"rb");
    if (in == NULL) {
      dialogOverwriteDone = true;
      callLoadSaveFunc();
      return;  // don't open dialog
    }
    fclose(in);
  }
  DIALOGSTART("Overwrite file")
  ImGui::Text("Overwrite old file?");
  ImGui::BeginChild("##DIALOGOVERWRITECHILD",ImVec2(main_window_xres/2,64));
  ImGui::TextWrapped("Filename:%s",loadSaveLongFileName.c_str());
  ImGui::TextWrapped("Path:%s",loadSaveShortPath.c_str());
  ImGui::EndChild();
  ImGui::Separator();
  if (ImGui::Button("Overwrite##DIALOGOVERWRITEOVERWRITE")) {
    callLoadSaveFunc();
    dialogOverwriteDone = true;
    dialogLoadSaveModeId = 0;
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Don't##DIALOGOVERWRITECLOSE")) {
    dialogLoadSaveModeId = 0;
    ImGui::CloseCurrentPopup();
  }
  DIALOGEND()
}

bool fileSystemUpdatedHere = false;
void dialogCreateFolder(bool openIt) {
  static char folderName[MAX_LFN_PATH];
  if (openIt) {
    sprintf(folderName,"%s","");
  }
  DIALOGSTART("Create a new folder")
  ImGui::BeginChild("##DIALOGCREATEFOLDERCHILD",ImVec2(main_window_xres/2,64));
  String k = String(folderName);
  String f = loadSaveShortPath + k + String(k.empty() ? "" : "/");
  ImGui::TextWrapped("Short path:%s",shortFileName(f).c_str()); toolTip("The short DOS path");
  ImGui::Separator();
  ImGui::TextWrapped("Long path:%s",longFileName(f).c_str()); toolTip("The long (doslfn.com) DOS path");
  ImGui::EndChild();
  ImGui::Separator();
  ImGui::Text("Folder name"); ImGui::SameLine(); DIALOGKEYSHERE(); ImGui::InputText("##DIALOGCREATEFOLDERCHILD",folderName,MAX_LFN_PATH); toolTip("How the new folder to create should be named here");
  ImGui::Separator();
  if (!k.empty()) {
    if (ImGui::Button("Create folder##DIALOGCREATEFOLDERCREATE")) {
      String f = loadSaveShortPath + k;
      if (!doslfnCreateDirectory(f.c_str())) {
        dosCreateFolder(f);
      }
      fileSystemUpdatedHere = true;
      ImGui::CloseCurrentPopup();
    } else toolTip("Create this folder");
    ImGui::SameLine();
  }
  if (ImGui::Button("Close##DIALOGCREATEFOLDERCLOSE")) {
    ImGui::CloseCurrentPopup();
  } else toolTip("Don't create any new folder");
  DIALOGEND()
}

void resize2(String &s, int len) {
  int a = s.length();
  s.resize(len);
  memset(&s[a],0,len-a);
}

String lastPath = "";

#define TOOLTIP(__a__) {if (!previewOn) toolTip(__a__);}

void dialogLoadSave(bool openIt, int dialogMode) {

  static int mode = DIALOG_NONE; if (dialogMode != DIALOG_NONE) mode = dialogMode;
#define FILTERNAMEMAX 16
  static char filter[FILTERNAMEMAX];


  static bool previewOn = false; 
  static bool driveReady = true;
  static bool needsRescanFiles = false;
  static bool longFileNames = false;
  static bool initColumns = false;
  static Array<char> driveLetters;
  static Array<File> files;
  static int sortByColumn = 0;
  static bool sortingOrder[4] = {false};
  static String paths[DIALOG_COUNT];
  static String fileNames[DIALOG_COUNT];
  static String path2 = "";
  static String scrollToFileName;
  static String returnToPath;
  static int scrollToHelper = 0;
  const int NAMEMAXHERE = 513;
  if (openIt) {
    if (!reOpen) {
      initColumns = true;
      previewOn = false;
      memset(filter,0,sizeof(filter));
      needsRescanFiles = true;
      driveLetters.clear();
      for (int i = 'A'; i <= 'Z'; i++) {
        if (dosHasDrive((char)i)) driveLetters.push_back((char)i);
      }
      for (int j = 0; j < DIALOG_COUNT; j++) {
        fileNames[j] = ""; // little bug
        resize2(fileNames[j],NAMEMAXHERE);
      }
      scrollToHelper=0;
      if (!dialogLoadSaveSetFileNameAndPath.empty()) {
        String zwo = dialogLoadSaveSetFileNameAndPath;
        dialogLoadSaveSetFileNameAndPath = unixFileName(SHORTFILENAME(zwo));
        fileNames[mode] = getName(zwo) + getExtension(zwo);
        scrollToFileName = toLower(fileNames[mode]);
        scrollToHelper = 2;
        resize2(fileNames[mode],NAMEMAXHERE);
        paths[mode] = getPath(dialogLoadSaveSetFileNameAndPath);
        dialogLoadSaveSetFileNameAndPath = "";
      }
    }
    reOpen = false;
  }
  DIALOGSTART(dialogCaption[mode])
  ImGui::PushStyleColor(ImGuiCol_ModalWindowDarkening,ImVec4(0,0,0,0.5)); dialogOverwrite(false); ImGui::PopStyleColor();
  ImGui::PushStyleColor(ImGuiCol_ModalWindowDarkening,ImVec4(0,0,0,0.5)); dialogCreateFolder(false); ImGui::PopStyleColor(); if (fileSystemUpdatedHere) {fileSystemUpdatedHere = false;needsRescanFiles=true;}

  noUndoMessage();
  if (paths[mode].empty()) {paths[mode] = getCWD();}
  if (needsRescanFiles) {
    clearPreview();
    needsRescanFiles = false;
    files.clear();
    char driveLetter = paths[mode][0];
    driveReady = true;
    if (driveLetter>='A'&&driveLetter<='B') {driveReady = dosIsDriveReady(driveLetter);}
    if (driveReady) {
      files = dosGetFiles(paths[mode]);
    }
    for (int i = 0; i < (int)files.size(); i++) {
      if (files[i].name.name == "." || files[i].name.name == "..") {files.erase(i,1);i--;} else
      if ((files[i].attributes & 0x1f) != DOS_ATTR_SUBDIR && (files[i].attributes & 0x1f) != DOS_ATTR_NORMAL) {files.erase(i,1);i--;}
    }
    if (longFileNames) {
      for (int i = 0; i < (int)files.size(); i++) {
          files[i].name.full = LONGFILENAME(files[i].name.full);
          files[i].name.name = getName(files[i].name.full);
          files[i].name.extension = getExtension(files[i].name.full);
      }
    }
    sortFileNames(files, sortByColumn, sortingOrder[sortByColumn]);
    path2 = LONGFILENAME(paths[mode]);
  }
#if !defined(__DJGPP__)
  ImGui::Text("Long filenames"); TOOLTIP("Use long filenames (provided by e.g. doslfn.com)"); ImGui::SameLine(); if(ImGui::Checkbox("##DIALOGLOADSAVELONGFILENAMES", &longFileNames)) needsRescanFiles = true;
#endif
  ImGui::SameLine();
  for (int i = 0; i < (int)driveLetters.size(); i++) {
    if (i != 0) ImGui::SameLine();
    char a = driveLetters[i];
    if (ImGui::Button((String::fromChar(a)+":##DIALOGLOADSAVEDRIVELETTER"+toString(i)).c_str())) {
      paths[mode] = String::fromChar(a) + ":/";
      needsRescanFiles = true;
      //log_setBiosCursor(0,0);
    } else TOOLTIP("Select this drive letter");
  }
  ImGui::Text("%s",dialogFileTypes[mode]); TOOLTIP("The default filetypes for this");
  ImGui::Separator();
  SVG *svg = &svgs["search"];ImGui::Image((ImTextureID)svg->glHandle,ImVec2(svg->w,svg->h));ImGui::SameLine();ImGui::InputText("##DIALOGLOADSAVEFILTER",filter,FILTERNAMEMAX); TOOLTIP("You only see files with this in their name");
  ImGui::SameLine();
  if (ImGui::Button("Create folder##DIALOGLOADSAVECREATEFOLDER")) {
      loadSaveShortPath = paths[mode]; 
      dialogCreateFolder(true);
  } else TOOLTIP("Create a new folder here");
  ImGui::SameLine();
  ImGui::Text("Preview"); ImGui::SameLine(); if (ImGui::Checkbox("##DIALOGLOADSAVEPREVIEW",&previewOn)) {if (previewOn) clearPreview();} else TOOLTIP("Show a tiny preview of this in a box");
  if (previewOn && previewAspect != 0) {
    ImGui::BeginTooltip();
    ImGui::Image((ImTextureID)previewTexture,ImVec2(previewWidth,previewHeight*aspectRatio*main_window_yres/main_window_xres/previewAspect));
    ImGui::EndTooltip();
  }
  ImGui::Separator();
  if (ImGui::Button("Home##DIALOGLOADSAVEHOME")) {
    paths[mode] = appFolder;
    needsRescanFiles=true;
  } else TOOLTIP("Navigate to the folder where you started the program");
  ImGui::SameLine();
  if (ImGui::Button("Last##DIALOGLOADSAVELASTPATH")) {
    paths[mode] = lastPath;
    needsRescanFiles=true;
  } else TOOLTIP("Navigate to the folder you used before");
  ImGui::SameLine();

  ImGui::SameLine();
  ImGui::Text("%s",path2.c_str()); TOOLTIP("The current path"); // Text Wrapped has no window Size here
  ImGui::Separator();
  bool setScrollY0 = false;
  if (paths[mode].length() > 3 && ImGui::Button("..##DIALOGLOADSAVEPATHBACK")) {
    int k = paths[mode].substr(0,paths[mode].length()-1).findLast("/");
    if (k>=0) paths[mode] = paths[mode].substr(0,k+1);
    needsRescanFiles = true;
    setScrollY0 = true;
  } else TOOLTIP("Navigate one folder up");
  ImGui::SameLine(); ImGui::Text("Sort by:"); TOOLTIP("Sort the files by this property"); ImGui::SameLine();
  for (int l = 0; l < 4; l++) {
    if (l!=0) ImGui::SameLine();
    static const char *headerButtonNames[4] = {"Filename##DIALOGLOADSAVEFILENAMES","Ext##DIALOGLOADSAVEEXTS","Size##DIALOGLOADSAVESIZES","Date##DIALOGLOADSAVEDATES"};
    if (ImGui::Button(headerButtonNames[l])) {
      sortByColumn = l;
      sortingOrder[l] = !sortingOrder[l];
      needsRescanFiles = true;
    } else TOOLTIP("Click on this to sort up or down");
  }
  ImGui::Separator();
  int w = main_window_xres/2;
  int h = main_window_yres/2;
  if (!driveReady) {
    if (ImGui::Button("Drive not ready.. Retry.##DIALOGLOADSAVERETRY")) {
      needsRescanFiles = true;
    } else TOOLTIP("Click this to try again");
  } else {
    ImGui::BeginChild("DIALOGLOADSAVECHILD",ImVec2(w,h));
    if (setScrollY0) {setScrollY0 = false; ImGui::SetScrollY(0);}
    ImGui::Columns(4);
    String filter2 = toLower(filter);  
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(1,0));  
    for (int j = 0; j < 4; j++) {
      if (initColumns) {
          if (j == 1) ImGui::SetColumnOffset(j,ImGui::GetColumnOffset(j-1)+7*20);
          if (j == 2) ImGui::SetColumnOffset(j,ImGui::GetColumnOffset(j-1)+7*5);
          if (j == 3) ImGui::SetColumnOffset(j,ImGui::GetColumnOffset(j-1)+7*10);
      }
      for (int i = 0; i < (int)files.size(); i++) {
        String fullFileName = toLower(files[i].name.name + files[i].name.extension);
        if (!filter2.empty() && fullFileName.findFirst(filter2)<0) continue;
        bool isDir = files[i].attributes & DOS_ATTR_SUBDIR;
        if (isDir) {
          if (j == 0) {
            ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0,0,0,0.5));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0,0,0,0));
            if (ImGui::Button((files[i].name.name+"##DIALOGLOADSAVEDIRBUTTON"+String::fromInt(i)).c_str())) {
              paths[mode] += files[i].name.name + files[i].name.extension + "/";
              paths[mode] = SHORTFILENAME(paths[mode]);
              fileNames[mode] = "";
              resize2(fileNames[mode],NAMEMAXHERE);
              needsRescanFiles = true;
              ImGui::SetScrollY(0);
            } else {
              //if (!previewOn) drawToolTip(files[i].name.name.c_str());
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
          } 
          if (j == 1) {
            ImGui::Text("DIR");
          }
          if (j == 2) {
            ImGui::Text("");
          }
          if (j == 3) {
            FileTime t = files[i].date;
            ImGui::Text("%02d.%02d.%04d %02d:%02d.%02d",t.day,t.month,t.year,t.hour,t.minute,t.second);
          }
        } else {
          if (j == 0) {
            ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0,0,0,0.5));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0,0,0,0));
            if (ImGui::Button((files[i].name.name+"##DIALOGLOADSAVEFILEBUTTON"+String::fromInt(i)).c_str())) {
              fileNames[mode] = files[i].name.name + files[i].name.extension;
              if (previewOn) {
                String p = paths[mode]; 
                String f = fileNames[mode];
                f.resize(strlen(f.c_str()));
                previewAspect = updateToolTip(preview, previewWidth, previewHeight, p.c_str(), f.c_str());
                updatePreview();
              }
              resize2(fileNames[mode],NAMEMAXHERE);
            } else {
              //if (!previewOn) drawToolTip((files[i].name.name + files[i].name.extension).c_str());
            }
            if (fullFileName==scrollToFileName) {ImGui::SetScrollHere();}
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
          }
          if (j == 1) {
            ImGui::Text("%s",files[i].name.extension.c_str());
          }
          if (j == 2) {
            ImGui::Text("%db",files[i].size);
          }
          if (j == 3) {
            FileTime t = files[i].date;
            ImGui::Text("%02d.%02d.%04d %02d:%02d.%02d",t.day,t.month,t.year,t.hour,t.minute,t.second);
          }
        }
      }
      ImGui::NextColumn();
    }
    ImGui::PopStyleVar();  
    initColumns = false;
    ImGui::EndChild();
  }
  ImGui::Separator();
  ImGui::PushItemWidth(w);
  ImGui::Text("Filename");
  DIALOGKEYSHERE(); ImGui::InputText("##DIALOGLOADSAVEFILENAME",&fileNames[mode][0],NAMEMAXHERE); TOOLTIP("The current filename")
  ImGui::PopItemWidth();
  ImGui::Separator();
  if (ImGui::Button(combined(String(proceedButton[mode])+String("##DIALOGLOADSAVEPROCEED")).c_str())) {
    lastPath = paths[mode];
    loadSaveFunc = NULL;

    String fileName = fileNames[mode];
    fileName.resize(strlen(fileName.c_str()));

    if (fileName.empty() && toLower(proceedButton[mode])!="select") {
      dialogOverwriteDone = true;
    } else {
      if (funcs[mode]!=NULL) {
        loadSaveFunc = funcs[mode]; 
        loadSaveShortPath = paths[mode]; 
        loadSaveLongFileName = fileName;
        if (toLower(proceedButton[mode])=="save") {
          if (loadSaveLongFileName.find(".")<0) {loadSaveLongFileName += dialogDefaultExtension[mode];}
          dialogOverwrite(true);
        } else {
          callLoadSaveFunc();
          dialogOverwriteDone = true;
        }
      } else dialogOverwriteDone = true;
    }
    //ImGui::CloseCurrentPopup(); (this done via dialogOverwriteDone)
  } else TOOLTIP("Go on with this and use the file/folder");

  if (dialogOverwriteDone) {
    dialogOverwriteDone = false;
    ImGui::CloseCurrentPopup();
  } 

  ImGui::SameLine();
  if (ImGui::Button("Close##DIALOGLOADSAVECLOSE")) {
    lastPath = paths[mode];
    ImGui::CloseCurrentPopup();
  } else TOOLTIP("Don't proceed with this and return");
  DIALOGEND()
  if (scrollToHelper>0) scrollToHelper--; else scrollToFileName="";
}

Array<String> fileErrors;
void errorWindow(bool openIt) {
  static const char *windowName = "Error window";
  static bool windowOpen = false;
  if (openIt) {
    windowOpen = true;
    return;
  }
  if (windowOpen) {
    int w = currentCanvas->width; int h = currentCanvas->height;
    ImGui::SetNextWindowPos(ImVec2(currentCanvas->screenPosX,currentCanvas->screenPosY));
    ImGui::Begin(windowName,&windowOpen);
    ImGui::SetWindowSize(ImVec2(w,h));
    ImGui::TextColored(ImVec4(1,0,0,1), "!!Some errors occured!!");
    ImGui::Text("Here you see the files that caused problems when reloading them.");
    ImGui::Text("Most probably they wheren't saved rightly to the drive.");
    ImGui::Text("Failing files:");
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImGui::BeginChild("DIALOGFILEERRORSCHILD", ImVec2(w-40,main_window_yres-cursor.y-30));
    for (int i = 0; i < (int)fileErrors.size(); i++) {
      ImGui::TextColored(ImVec4(1,0,0,1),"%s",fileErrors[i].last(50).c_str());
    }
    ImGui::EndChild();
    if (roundButton("Close error window##DIALOGFILEERRORSCLOSE",true)) {
      windowOpen = false;
    } else toolTip("Just close this view");
    ImGui::End();
    if (!windowOpen) fileErrors.clear();
  } else {
    ImGui::SetNextWindowPos(ImVec2(main_window_xres-64,0));
    ImGui::Begin(windowName, NULL, ImVec2(1,1),0.0,FROZENWINDOW);
    ImGui::SetWindowSize(ImVec2(1,1));
    ImGui::End();
  }
}
