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
#include "STL/PARSON.HPP"
#include "STL/JSONWRT.HPP"
#include "STL/MINIZ.HPP"
#include "UNDO.HPP"
#include "LAYERV.HPP"
#include "MENUBAR.HPP"
#include "NAVIGTOR.HPP"
#include "TOOLBAR.HPP"
#include "TOOLVIEW.HPP"
#include "UNDO.HPP"
#include "STL/HASHMAP.HPP"
#include "TEXT.HPP"
#include <math.h>

#define RGBAIMAGEFROMFILE(__image__,__path__,__fileName__) String fullFileName = String(__path__) + String(__fileName__); Array<unsigned char> arr; getFileArray(&arr,fullFileName); RGBAImage __image__ = arr.empty() ? RGBAImage() : RGBAImage::fromMemory(&(arr[0]), arr.size());

#define LONGFILENAME(__str__) (longFileNames ? longFileName(__str__) : unixFileName(__str__))
#define SHORTFILENAME(__str__) (longFileNames ? shortFileName(__str__) : unixFileName(__str__))
bool longFileNames = false;
char longFileNameBuffer[MAX_LFN_PATH];

void loadedSomething() {
  if (tool == TOOL_TILING) {lastTool = -1;}
}

void savedSomething() {
}

String fromLocalFileName(const String &str) {
  String ret = unixFileName(str);
  if (ret.startsWith("./")) ret = appFolder + ret.substr(2);
  return ret;
}

String toLocalFileName(const String &str) {
  String ret = unixFileName(str);
  if (ret.startsWith(appFolder)) ret = String("./") + ret.substr(appFolder.length());
  return ret;
}

enum JSONTYPE {
  JSONTYPE_NULL = 0,
  JSONTYPE_BOOL = 1,
  JSONTYPE_INT = 2,
  JSONTYPE_UINT = 3,
  JSONTYPE_FLOAT = 4,
  JSONTYPE_DOUBLE = 5,
  JSONTYPE_STRING = 6,
  JSONTYPE_NAME = 7,
  JSONTYPE_ARRAY_OPEN = 8,
  JSONTYPE_ARRAY_CLOSE = 9,
  JSONTYPE_OBJECT_OPEN = 10,
  JSONTYPE_OBJECT_CLOSE = 11,
  JSONTYPE_VECTOR = 12,
  JSONTYPE_QUATERNION = 13,
  JSONTYPE_FLOAT4 = 14,
  JSONTYPE_MATRIX = 15,
  JSONTYPE_FILENAME = 16,
  JSONTYPE_BASE64 = 17,
  JSONTYPE_IMVEC4 = 18
};

JSON_Value * json_parse_string(const char *string);

#define JSONPORTIONLENGTH ((sizeof(float)*128*128*4/3+128)*2) // ensure the biggest element is atleast one time writable (in this case the distanceField of the brushes

void loadArray(Array<char> *arr, FILE *in) {
  fseek(in,0,SEEK_END);
  unsigned int fileLen = ftell(in);
  fseek(in,0,SEEK_SET);
  arr->resize(fileLen);
  fread(&(*arr)[0],1,fileLen,in);
}

void readJson(Array<char> *arr, String *string) {
  arr->resize(string->length()+1);
  (*arr)[string->length()] = 0x00;
  for (int i = 0; i < string->length(); i++) {
    (*arr)[i] = (*string)[i];
  }
}

void writeJson(Array<char> *arr, String *string) {
  string->resize(arr->size());
  for (int i = 0; i < string->length(); i++) {
    (*string)[i] = (*arr)[i];
  }
}

#define STARTSERIALIZATION(__save__) int arrayIndex = 0; bool error = false; JSON_Value *jval; Array<JSON_Value*> jvals; Array<char> jsonArray; jsonArray.resize(JSONPORTIONLENGTH); jsonArray[0] = '{'; char *jsonPos = &jsonArray[1]; bool jsonIsSaving = __save__; if (!jsonIsSaving) {readJson(&jsonArray,string);jval=json_parse_string_with_comments(&jsonArray[0]); jvals.push_back(jval); jsonPos = (char*)&jvals; if (jval==NULL) error = true;}
#define FINISHSERIALIZATION() {if (jsonIsSaving) {if (jsonPos[-1] == ',' ) jsonPos--; sprintf(jsonPos,"}");jsonPos++;jsonArray.resize((unsigned int)(jsonPos-&jsonArray[0]));writeJson(&jsonArray,string);}}

#define S_NULL(__name__)  {jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_NULL, __name__, NULL, jsonIsSaving,arrayIndex);}
#define S_BOOL(__name__, __v__)  {bool *typeSafe = &(__v__); UNUSED(typeSafe); jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_BOOL, __name__, &(__v__), jsonIsSaving,arrayIndex);}
#define S_INT(__name__, __v__)  {int *typeSafe = &(__v__); UNUSED(typeSafe); jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_INT, __name__, &(__v__), jsonIsSaving,arrayIndex);}
#define S_UINT(__name__, __v__)  {unsigned int *typeSafe = &(__v__); UNUSED(typeSafe); jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_UINT, __name__, &(__v__), jsonIsSaving,arrayIndex);}
#define S_FLOAT(__name__, __v__)  {float *typeSafe = &(__v__); UNUSED(typeSafe); jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_FLOAT, __name__, &(__v__), jsonIsSaving,arrayIndex);}
#define S_DOUBLE(__name__, __v__)  {double *typeSafe = &(__v__); UNUSED(typeSafe); jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_DOUBLE, __name__, &(__v__), jsonIsSaving,arrayIndex);}
#define S_STRING(__name__, __v__)  {String *typeSafe = &(__v__); UNUSED(typeSafe); jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_STRING, __name__, &(__v__), jsonIsSaving,arrayIndex);}
#define S_BASE64(__name__, __v__, __l__)  {currentBase64Length = __l__; jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_BASE64, __name__, (__v__), jsonIsSaving,arrayIndex);}
#define S_NAME(__name__, __v__, __l__)  {currentNameLength = __l__; char *typeSafe = (__v__); UNUSED(typeSafe); jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_NAME, __name__, (__v__), jsonIsSaving,arrayIndex);}
#define S_ARRAY_OPEN(__name__)  jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_ARRAY_OPEN, __name__, NULL, jsonIsSaving,arrayIndex)
#define S_ARRAY_CLOSE()  jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_ARRAY_CLOSE, NULL, NULL, jsonIsSaving,arrayIndex)
#define S_OBJECT_OPEN(__name__)  jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_OBJECT_OPEN, __name__, NULL, jsonIsSaving,arrayIndex)
#define S_OBJECT_CLOSE()  jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_OBJECT_CLOSE, NULL, NULL, jsonIsSaving,arrayIndex)
#define S_FILENAME(__name__, __v__)  {String *typeSafe = &(__v__); UNUSED(typeSafe); jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_FILENAME, __name__, &(__v__), jsonIsSaving,arrayIndex);}
#define S_VECTOR(__name__, __v__) {Vector *typeSafe = &(__v__); UNUSED(typeSafe); jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_VECTOR, __name__, &(__v__), jsonIsSaving,arrayIndex);}
#define S_MATRIX(__name__, __v__)  {Matrix *typeSafe = &(__v__); UNUSED(typeSafe); jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_MATRIX, __name__, &(__v__), jsonIsSaving,arrayIndex);}
#define S_ZIPLAYERDATA(__index__,__layer__) zipLayerData(String("layer")+String::fromInt(__index__),__layer__,jsonIsSaving)
#define S_IMVEC4(__name__, __v__) {ImVec4 *typeSafe = &(__v__); UNUSED(typeSafe); jsonSerialize(&jsonArray, (void**)&jsonPos, JSONTYPE_IMVEC4, __name__, &(__v__), jsonIsSaving,arrayIndex);}

#define S_ZIPARRAY(__fileName__,__v__,__es__) {\
  int a = __v__.size(); S_INT(((__fileName__)+"_ARRAYSIZE").c_str(),a);\
  int b = __es__; S_INT(((__fileName__)+"_ELEMENTSIZE").c_str(),b);\
  if (!save) __v__.resize(a);\
  zipData(save,__fileName__,&(__v__[0]),a*b);\
}


HashMap<unsigned int,unsigned int> idRemap;
Array<unsigned int*> idsToResolve;
#define S_ID(__name__,__id__) {unsigned int before = (__id__); S_UINT(__name__,__id__); unsigned int after = (__id__); idRemap[after] = before; (__id__) = before;} // before is an upcounting value already
#define S_IDREFERENCE(__name__,__id__) {S_UINT(__name__,__id__); idsToResolve.push_back(&(__id__));}
#define S_RESOLVEIDS() {\
  if (!save) {\
    for (int i = 0; i < idsToResolve.size(); i++) {\
      unsigned int *dest = idsToResolve[i];\
      if (idRemap.has(*dest)) *dest = idRemap[*dest]; else *dest = 0;\
    }\
  }\
  idRemap.clear(); idsToResolve.clear();\
}

void zipLayerData(const String &layerName, Layer *layer, bool save);
void zipPalette(const Array<ImVec4> &palette, int colorsPerRow, bool save);
void zipGradient(const Gradient &gradient, bool save);
void zipData(bool save,const String &fileName, void *data, unsigned int byteCount);

//char *base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char *base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
Array<unsigned char> invBase64;

String encodeBase64(void *data, unsigned int len) {
  unsigned char *src = (unsigned char *)data;
  String ret;
  ret.resize((len+2)/3*4);
  for (int i = 0; i < ret.size()/4; i++) {
    unsigned char a0 = src[i*3+0];
    unsigned char a1 = src[i*3+1];
    unsigned char a2 = src[i*3+2];
    unsigned char d0 = (a0>>2);
    unsigned char d1 = ((a0&3)<<4)|(a1>>4);
    unsigned char d2 = ((a1&15)<<2)|(a2>>6);
    unsigned char d3 = a2&63;
    ret[i*4+0] = base64[d0];
    ret[i*4+1] = base64[d1];
    ret[i*4+2] = base64[d2];
    ret[i*4+3] = base64[d3];
  }
  for (int i2 = 0; i2 < len % 3; i2++) ret[ret.size()-i2-1] = '=';
  return ret;
}

void decodeBase64(const String &base64String, void *dest) {
  if (invBase64.empty()) {
    invBase64.resize(256);
    for (int i = 0; i < 256; i++) invBase64[i] = 0;
    if (strlen(base64) != 64) {glDone(); printf("base64 seems corrupt\n"); exit(0);}
    for (int j = 0; j < 64; j++) invBase64[((unsigned char)base64[j]) & 255] = j;
  }
  int outputSize = base64String.length()*3/4;
  if (base64String.length()>=1) if (base64String[base64String.length()-1] == '=') outputSize--;
  if (base64String.length()>=2) if (base64String[base64String.length()-2] == '=') outputSize--;
  int k = 0;
  unsigned char *dest2 = (unsigned char*)dest;
  for (int i = 0; i < outputSize; i++) {
    unsigned char a,b,val;
    switch(i % 3) {
    case 0: {a = invBase64[base64String[k]]; b = invBase64[base64String[k+1]]; val = (a<<2)+((b>>4)&3);} break;
    case 1: {a = invBase64[base64String[k+1]]; b = invBase64[base64String[k+2]]; val = (a<<4)+((b>>2)&15);} break;
    case 2: {a = invBase64[base64String[k+2]]; b = invBase64[base64String[k+3]]; val = ((a&3)<<6)+(b & 63); k+=4;} break;
    }
    dest2[i] = val;
  }
}

int currentNameLength = 0;
int currentBase64Length = 0;
int arraySize = 0; 
#define S_ARRAY(__name__,__array__,__elementFunc__) {\
  int k = arrayIndex;\
  S_ARRAY_OPEN(__name__);\
  if (!jsonIsSaving) {\
    (__array__).clear(); (__array__).resize(arraySize);\
  }\
  for (int i = 0; i < (__array__).size(); i++) {\
    arrayIndex = i;\
    __elementFunc__;\
  }\
  S_ARRAY_CLOSE();\
  arrayIndex = k;\
}

#define S_ARRAY_POINTER_RESIZE(__array__,__type__,__count__) {\
  if (!jsonIsSaving) {\
    for (int i1 = 0; i1 < (__array__).size(); i1++) if ((__array__)[i1] != NULL) {delete __array__[i1]; (__array__)[i1]=NULL;}\
    (__array__).clear(); (__array__).resize(__count__);\
    for (int i2 = 0; i2 < (__array__).size(); i2++) (__array__)[i2] = new __type__();\
  }\
}

#define S_POINTER_ARRAY(__name__,__array__,__elementType__,__elementFunc__) {\
  int k = arrayIndex;\
  S_ARRAY_OPEN(__name__);\
  S_ARRAY_POINTER_RESIZE(__array__,__elementType__,arraySize);\
  for (int i = 0; i < (__array__).size(); i++) {\
    arrayIndex = i;\
    __elementFunc__;\
  }\
  S_ARRAY_CLOSE();\
  arrayIndex = k;\
}

#define S_CARRAY(__name__,__array__,__elementFunc__,__count__) {\
  int k = arrayIndex;\
  S_ARRAY_OPEN(__name__);\
  for (int i = 0; i < (__count__); i++) {\
    arrayIndex = i;\
    __elementFunc__;\
  }\
  S_ARRAY_CLOSE();\
  arrayIndex = k;\
}

char tempBuffer[256];

#define _D(__double__) doubleToString(__double__).c_str()
#define _F(__float__) floatToString(__float__).c_str()
String doubleToString(double v) {
  String ret = String::fromDouble(v);
  int pointPos = ret.find(".");
  if (pointPos>=0) {
    for (int i = ret.length()-1; i >= pointPos; i--) {
      if (ret[i]=='0'||ret[i]=='.') {ret.resize(i);continue;}
      break;
    }
    if (ret.empty()) ret = "0";
  }
  return ret;
}
String floatToString(float v) {
  return doubleToString(v);
}

void jsonSerialize(void *json, void **jsonPos, int type, const char *name, void *data, bool save, int arrayIndex) {
  if (save) {
    Array<char> *w = (Array<char>*)json;
    char *oldPos = *(char**)jsonPos;
    char *newPos = oldPos;
    size_t len = &(*w)[w->size()] - oldPos;
    switch(type) {
      case JSONTYPE_NULL: {newPos = json_null( oldPos, name, &len );} break;
      case JSONTYPE_BOOL: {newPos = json_bool( oldPos, name, *((bool*)data), &len );} break;
      case JSONTYPE_INT: {newPos = json_int( oldPos, name, *((int*)data), &len );} break;
      case JSONTYPE_UINT: {newPos = json_uint( oldPos, name, *((unsigned int*)data), &len );} break;
      case JSONTYPE_FLOAT: {double v = *((float*)data); newPos = json_double( oldPos, name, v, &len );} break;
      case JSONTYPE_DOUBLE: {newPos = json_double( oldPos, name, *((double*)data), &len );} break;
      case JSONTYPE_STRING: {String str = *(String*)data; newPos = json_str( oldPos, name, str.c_str() != NULL ? str.c_str() : "", &len );} break;
      case JSONTYPE_NAME: {String str = (const char *)data; newPos = json_str( oldPos, name, str.c_str() != NULL ? str.c_str() : "", &len );} break;
      case JSONTYPE_ARRAY_OPEN: {newPos = json_arrOpen( oldPos, name, &len );} break;
      case JSONTYPE_ARRAY_CLOSE: {newPos = json_arrClose( oldPos,&len );} break;
      case JSONTYPE_OBJECT_OPEN: {newPos = json_objOpen( oldPos, name, &len );} break;
      case JSONTYPE_OBJECT_CLOSE: {newPos = json_objClose( oldPos,&len );} break;
      case JSONTYPE_FILENAME: {String str = toLocalFileName(*(String*)data); newPos = json_str( oldPos, name, str.c_str() != NULL ? str.c_str() : "", &len );} break;
      case JSONTYPE_BASE64: { String str = encodeBase64(data,currentBase64Length); newPos = json_str( oldPos, name, str.c_str() != NULL ? str.c_str() : "", &len );} break;
      case JSONTYPE_VECTOR: {Vector *v = (Vector*)data; sprintf(tempBuffer,"%s,%s,%s,%s", _D(v->x), _D(v->y), _D(v->z), _D(v->w)); newPos = json_str( oldPos, name, tempBuffer, &len );} break;
      case JSONTYPE_MATRIX: {double *v = ((Matrix*)data)->m; sprintf(tempBuffer,"%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s", _D(v[0]), _D(v[1]), _D(v[2]), _D(v[3]), _D(v[4]), _D(v[5]), _D(v[6]), _D(v[7]), _D(v[8]), _D(v[9]), _D(v[10]), _D(v[11]), _D(v[12]), _D(v[13]), _D(v[14]), _D(v[15])); newPos = json_str( oldPos, name, tempBuffer, &len );} break;
      case JSONTYPE_IMVEC4: {ImVec4 *v = (ImVec4*)data; sprintf(tempBuffer,"%s,%s,%s,%s", _D(v->x), _D(v->y), _D(v->z), _D(v->w)); newPos = json_str( oldPos, name, tempBuffer, &len );} break;
    }
    int posInString = (int)(newPos - &(*w)[0]);
    if (posInString > w->size()-JSONPORTIONLENGTH) {
      w->resize(w->size() + JSONPORTIONLENGTH*2);
      newPos = &(*w)[0] + posInString;
    }
    *jsonPos = newPos;
  }
  if (!save) {
    Array<JSON_Value*> *jvals = *(Array<JSON_Value*>**)jsonPos;
    JSON_Value *jval = jvals->back();
    JSON_Value *val = NULL;
    switch(json_value_get_type(jval)) {
      case JSONObject: {val = json_object_get_value(json_value_get_object(jval), name);} break;
      case JSONArray: {val = json_array_get_value(json_value_get_array(jval), arrayIndex);} break;
    }
    bool has = json_value_get_type(val)!=JSONError;
    switch(type) {
      case JSONTYPE_NULL: {} break;
      case JSONTYPE_BOOL: {if(!has)break;*((bool*)data) = json_value_get_boolean(val)?true:false;} break;
      case JSONTYPE_INT: {if(!has)break;*((int*)data) = (int)json_value_get_number(val);} break;
      case JSONTYPE_UINT: {if(!has)break;*((unsigned int*)data) = (unsigned int)json_value_get_number(val);} break;
      case JSONTYPE_FLOAT: {if(!has)break;*((float*)data) = (float)json_value_get_number(val);} break;
      case JSONTYPE_DOUBLE: {if(!has)break;*((double*)data) = (double)json_value_get_number(val);} break;
      case JSONTYPE_STRING: {if(!has)break;*(String*)data = (const char *)json_value_get_string(val);} break;
      case JSONTYPE_NAME: {if(!has)break;sprintf((char*)data,"%s",(const char *)json_value_get_string(val));} break;
      case JSONTYPE_ARRAY_OPEN: {jvals->push_back(val);arraySize=json_array_get_count(json_value_get_array(val));} break;
      case JSONTYPE_ARRAY_CLOSE: {jvals->pop_back();} break;
      case JSONTYPE_OBJECT_OPEN: {jvals->push_back(val);} break;
      case JSONTYPE_OBJECT_CLOSE: {jvals->pop_back();} break;
      case JSONTYPE_FILENAME: {if(!has)break;*(String*)data = fromLocalFileName((const char *)json_value_get_string(val));} break;
      case JSONTYPE_BASE64: {if(!has)break; decodeBase64(String((const char *)json_value_get_string(val)),data); } break;
      case JSONTYPE_VECTOR: {if(!has)break;Vector *v = (Vector*)data; String t = (const char *)json_value_get_string(val); sscanf(t.c_str(),"%lf,%lf,%lf,%lf", &v->x, &v->y, &v->z, &v->w);} break;
      case JSONTYPE_MATRIX: {if(!has)break;double *v = ((Matrix*)data)->m; String t = (const char *)json_value_get_string(val); sscanf(t.c_str(),"%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9], &v[10], &v[11], &v[12], &v[13], &v[14], &v[15]);} break;
      case JSONTYPE_IMVEC4: {if(!has)break;ImVec4 *v = (ImVec4*)data; String t = (const char *)json_value_get_string(val); sscanf(t.c_str(),"%f,%f,%f,%f", &v->x, &v->y, &v->z, &v->w);} break;
    }
  }
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

String getStringFromFile(const char *shortPath, const char *longFileName) {
  String fullFileName = shortFileName(dosFileName(String(shortPath)+String(longFileName)));
  String string;
  FILE *in = fopen(dosFileName(fullFileName).c_str(),"rb");
  if (in == NULL) return String("");
  fseek(in,0,SEEK_END);
  unsigned int fileLen = ftell(in);
  if (fileLen == 0) return String("");
  fseek(in,0,SEEK_SET);
  string.resize(fileLen);
  fread(&string[0],1,fileLen,in);
  fclose(in);
  return string;
}

void putStringToFile(const String &string, const char *shortPath, const char *longFileName) {
  FILE *out = createTempFile(shortPath,"wb");
  if (!string.empty()) fwrite(&string[0],1,string.size(),out);
  fclose(out);
  renameTempFile(shortPath, longFileName);
}


bool serializeBrushes(Array<Brush*> &brushes, bool save, String *string) {
  STARTSERIALIZATION(save);
  if (error) {return false;}

  String id = "WYPA BRUSHES"; S_STRING("id",id);
  S_POINTER_ARRAY("brushes",brushes,Brush,S_BRUSH(NULL,brushes[i]));

  FINISHSERIALIZATION();
  return true;
}

Matrix inverse2(const Matrix &a);

#define S_WORKSPACE(__name__)\
  S_DIALOGS_CPP();\
  S_COLORFLD_CPP();\
  S_CONFIG_CPP();\
  S_LAYERS_CPP();\
  S_LAYERV_CPP();\
  S_MENUBAR_CPP();\
  S_NAVIGATOR_CPP();\
  S_TOOLBAR_CPP();\
  S_TOOLS_CPP();\
  S_TOOLVIEW_CPP();\
  S_UNDO_CPP();\
  S_TEXT_CPP();

bool serializeCanvas(Array<Canvas*> &canvas, bool save, String *string) {
  STARTSERIALIZATION(save);
  if (error) { return false;}

  String id = "WYPA IMAGE FILE"; S_STRING("id",id);

  S_WORKSPACE("workspace");
  S_POINTER_ARRAY("brushes",brushes,Brush,S_BRUSH(NULL,brushes[i]));
  S_POINTER_ARRAY("canvas",canvas,Canvas,S_CANVAS(NULL,canvas[i]));

  FINISHSERIALIZATION();
  return true;
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

void importImage(const char *shortPath, const char *longFileName) {
  addPicture(String(shortPath)+String(longFileName));
  loadedSomething();
}

void insertImageLayers(const char *shortPath, const char *longFileName) {
  insertPicture(String(shortPath)+String(longFileName));
  loadedSomething();
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
  savedSomething();
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
  savedSomething();
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
      if (applyColorTransform) {
        for (int j = 0; j < l->width*l->height; j++) {
          if (l->colorTransform.useColorTransform) img.data[j] = l->colorTransform.applyColorTransform(img.data[j]);
          if (l->colorTransform.usePosterize) img.data[j] = l->colorTransform.applyPosterize(img.data[j]);
          if (l->colorTransform.useThreshold) img.data[j] = l->colorTransform.applyThreshold(img.data[j]);
          if (l->colorTransform.useQuantization) img.data[j] = quantizeFast(img.data[j], l->colorTransform.ditherStrength, j % l->width, j / l->width);
        }
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
  savedSomething();
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
  loadedSomething();
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
  loadedSomething();
}

void insertSVG(const char *shortPath, const char *longFileName) {
  String fullFileName = shortFileName(dosFileName(String(shortPath)+String(longFileName)));
  image = nsvgParseFromFile(dosFileName(fullFileName).c_str(), "px", 96);
  if (image == NULL) {
    return;
  }
  dialogInsertSVG(true,image->width,image->height);
  loadedSomething();
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

void paletteFromString(const char *str) {
  if (str == NULL) return;
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
}

String getPaletteString(const Array<ImVec4> &palette, int paletteColorsPerRow) {
  String ret;
  char zwo[256];
  sprintf(zwo,"%d\n",paletteColorsPerRow);
  ret += String(zwo);
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
    sprintf(zwo,"%d %d %d %d\n",r,g,b,a);
    ret += String(zwo);
  }
  return ret;
}


void loadPalette(const char *shortPath, const char *longFileName) {
  Array<unsigned char> arr;
  getFileArray(&arr,String(shortPath)+String(longFileName));
  if (arr.empty()) return;
  paletteFromString((const char *)&arr[0]);
  updatePalette();
  loadedSomething();
}

void savePalette(const char *shortPath, const char *longFileName) {
  FILE *out = createTempFile(shortPath,"w");
  String string = getPaletteString(palette,paletteColorsPerRow);
  fprintf(out,"%s",string.c_str());
  fclose(out);
  renameTempFile(shortPath, longFileName);
  savedSomething();
}

void gradientFromString(const char *str) {
  if (str == NULL) return;
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

String getGradientString(const Gradient &gradient) {
  String ret;
  char zwo[256];
  for (int i = 0; i < gradient.elements.size(); i++) {
    const GradientElement *e = &gradient.elements[i];
    float pos = e->position;
    int r = e->color & 255;
    int g = (e->color>>8) & 255;
    int b = (e->color>>16) & 255;
    int a = (e->color>>24) & 255;
    bool step = e->isStep;
    sprintf(zwo,"%f %d %d %d %d %c\n",pos,r,g,b,a,step ? 's' : 'l');
    ret += String(zwo);
  }
  return ret;
}

void loadGradient(const char *shortPath, const char *longFileName) {
  Array<unsigned char> arr;
  getFileArray(&arr,String(shortPath)+String(longFileName));
  if (arr.empty()) return;
  const char *str = (char*)&arr[0];
  gradientFromString(str);
  loadedSomething();
}

void saveGradient(const char *shortPath, const char *longFileName) {
  FILE *out = createTempFile(shortPath,"w");
  
  String gradientString = getGradientString(gradient);
  fprintf(out,"%s",gradientString.c_str());

  fclose(out);
  renameTempFile(shortPath, longFileName);
  savedSomething();
}

class ZipDesc {
public:
  ZipDesc() {
    sprintf(fileName,"");
    data = NULL;
    dataSize = 0;
  }
  ZipDesc(const String &fileName, void *data, int dataSize) {
    sprintf(this->fileName,"%s",fileName.c_str());
    this->data = data;
    this->dataSize = dataSize;
  }
  char fileName[512];
  void *data;
  int dataSize;
};

bool writeZippedFiles(const Array<ZipDesc> &desc, const char *shortPath, const char *longFileName) {
  mz_zip_archive zip;
  mz_zip_zero_struct(&zip);
  String zipName = getTempFileName(shortPath);
  if (!mz_zip_writer_init_file(&zip, zipName.c_str(), 0)) return false;
  for (int i = 0; i < desc.size(); i++) {
    if (!mz_zip_writer_add_mem(&zip, desc[i].fileName, desc[i].data, desc[i].dataSize, MZ_BEST_COMPRESSION|MZ_ZIP_FLAG_ASCII_FILENAME)) {
    }
  }
  if (!mz_zip_writer_finalize_archive(&zip)) return false;
  if (!mz_zip_writer_end(&zip)) return false;
  renameTempFile(shortPath, longFileName);
  savedSomething();
  return true;
}

bool readZippedFile(const char *shortPath, const char *longFileName, const char *inZipFileName, Array<unsigned char> &array) {
  bool ret = false;
  String fullFileName = shortFileName(dosFileName(String(shortPath)+String(longFileName)));
  String fileName = dosFileName(fullFileName);
  String toLookFor = toLower(unixFileName(inZipFileName));
  FILE *in = fopen(fileName.c_str(),"rb"); if (in == NULL) return ret;
  mz_zip_archive zip;  mz_zip_archive_file_stat file_stat;  mz_zip_zero_struct(&zip);

  if (mz_zip_reader_init_cfile(&zip, in, 0, MZ_ZIP_FLAG_ASCII_FILENAME)) {
    unsigned int fileCount = mz_zip_reader_get_num_files(&zip);
    for (int i = 0; i < fileCount; i++) {
      if (mz_zip_reader_file_stat(&zip, i, &file_stat)) {
        String a = (const char *)file_stat.m_filename; a = toLower(unixFileName(a));
        if (a == toLookFor) {
          array.resize((size_t)file_stat.m_uncomp_size);
          if (mz_zip_reader_extract_to_mem(&zip,i, &(array[0]), array.size(), MZ_ZIP_FLAG_ASCII_FILENAME)) {
            ret = true;
          }
          break;
        }
      }
    }
    mz_zip_reader_end(&zip);
  }
  fclose(in);
  loadedSomething();
  return ret;
}

String getStringFromZippedFile(const char *shortPath, const char *longFileName, const char *inZipFileName) {
  static Array<unsigned char> array;
  readZippedFile(shortPath, longFileName, inZipFileName, array);
  String ret;
  ret.resize(array.size());
  memcpy(&ret[0],&array[0],array.size()*sizeof(unsigned char));
  return ret;
}

void getBrushesArray(Array<Brush *> &dest, const char *shortPath, const char *longFileName) {
  //String string = getStringFromFile(shortPath,longFileName);
  String string = getStringFromZippedFile(shortPath,longFileName,"brushes.jsn");
  if (!string.empty()) {
    serializeBrushes(dest, false, &string);
  }
}

void putBrushesArray(Array<Brush *> &source, const char *shortPath, const char *longFileName) {
  if (!source.empty()) {
    String string;
    serializeBrushes(source, true, &string);
    //putStringToFile(string, shortPath,longFileName);
    Array<ZipDesc> desc; desc.push_back(ZipDesc("brushes.jsn",&string[0], string.size())); writeZippedFiles(desc, shortPath, longFileName);
  }
}

void loadBrush(const char *shortPath, const char *longFileName) {
  if ((unsigned int)currentBrush<=brushes.size()) {
    Array<Brush *> brushesHere; getBrushesArray(brushesHere, shortPath, longFileName);
    if (brushesHere.size()>0) {
      delete brushes[currentBrush];
      brushes[currentBrush] = brushesHere[0];
      brushes[currentBrush]->update();
    }
  }
  loadedSomething();
}

void saveBrush(const char *shortPath, const char *longFileName) {
  if ((unsigned int)currentBrush<=brushes.size()) {
    Array<Brush *> brushesHere; 
    brushesHere.push_back(brushes[currentBrush]);
    putBrushesArray(brushesHere, shortPath, longFileName);
  }
  savedSomething();
}

void loadBrushes(const char *shortPath, const char *longFileName) {
  Array<Brush *> brushesHere; getBrushesArray(brushesHere, shortPath, longFileName);
  for (int i = 0; i < brushes.size(); i++) delete brushes[i];
  brushes.resize(brushesHere.size());
  for (int i2 = 0; i2 < brushes.size(); i2++) brushes[i2] = brushesHere[i2];
  currentBrush = 0;
}

void saveBrushes(const char *shortPath, const char *longFileName) {
  Array<Brush *> brushesHere; 
  for (int i = 0; i < brushes.size(); i++) brushesHere.push_back(brushes[i]);
  putBrushesArray(brushesHere, shortPath, longFileName);
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
  loadedSomething();
}

mz_zip_archive globalZip;
const char *readZipShortPath = "";
const char *readZipLongFileName = "";

void zipLayerData(const String &layerName, Layer *layer, bool save) {
  updateStatusBar();
  mz_zip_archive &zip = globalZip;
  String fileName = layerName + ".png";
  if (save) {
    RGBAImage w2;
    w2.data = layer->pixels;
    w2.width = layer->width;
    w2.height = layer->height;
    if (w2.data != NULL && w2.width>0 && w2.height>0) {
      dosRemoveFile(fileName.c_str());
      updateStatusBar();
      w2.savePNG(fileName.c_str());
      updateStatusBar();
      mz_zip_writer_add_file(&zip, fileName.c_str(), fileName.c_str(), "", 0, MZ_BEST_COMPRESSION|MZ_ZIP_FLAG_ASCII_FILENAME);
      updateStatusBar();
      dosRemoveFile(fileName.c_str());
      updateStatusBar();
    }
  } else {
    Array<unsigned char> array;
    if (readZippedFile(readZipShortPath, readZipLongFileName, fileName.c_str(), array)) {
      updateStatusBar();
      RGBAImage img = RGBAImage::fromMemory(&array[0],array.size());
      if (img.data != NULL && img.width>0 && img.height>0) {
        layer->init(img.width,img.height);
        memcpy(layer->pixels,img.data,img.width*img.height*sizeof(unsigned int));
        img.free();
      }
    }
  }
}

void zipData(bool save,const String &fileName, void *data, unsigned int byteCount) {
  updateStatusBar();
  mz_zip_archive &zip = globalZip;
  if (save) {
    dosRemoveFile(fileName.c_str());
    updateStatusBar();
    String pal = getPaletteString(palette,paletteColorsPerRow);
    FILE *out = fopen(fileName.c_str(),"wb");
    fwrite(data,1,byteCount,out);
    fclose(out);
    updateStatusBar();
    mz_zip_writer_add_file(&zip, fileName.c_str(), fileName.c_str(), "", 0, MZ_BEST_COMPRESSION|MZ_ZIP_FLAG_ASCII_FILENAME);
    updateStatusBar();
    dosRemoveFile(fileName.c_str());
    updateStatusBar();
  } else {
    Array<unsigned char> array;
    if (readZippedFile(readZipShortPath, readZipLongFileName, fileName.c_str(), array)) {
      if (array.size() == byteCount) {
        memcpy(data,&array[0],byteCount);
        updateStatusBar();
      }
    }
  }
}

void zipPalette(const Array<ImVec4> &palette, int colorsPerRow, bool save) {
  updateStatusBar();
  mz_zip_archive &zip = globalZip;
  String fileName = "palette.pa2";
  if (save) {
    dosRemoveFile(fileName.c_str());
    updateStatusBar();
    String pal = getPaletteString(palette,paletteColorsPerRow);
    FILE *out = fopen(fileName.c_str(),"wb");
    fwrite(&pal[0],1,pal.size(),out);
    fclose(out);
    updateStatusBar();
    mz_zip_writer_add_file(&zip, fileName.c_str(), fileName.c_str(), "", 0, MZ_BEST_COMPRESSION|MZ_ZIP_FLAG_ASCII_FILENAME);
    updateStatusBar();
    dosRemoveFile(fileName.c_str());
    updateStatusBar();
  } else {
    Array<unsigned char> array;
    if (readZippedFile(readZipShortPath, readZipLongFileName, fileName.c_str(), array)) {
      paletteFromString((const char*)&array[0]);
      updateStatusBar();
    }
  }
}

void zipGradient(const Gradient &gradient, bool save) {
  updateStatusBar();
  mz_zip_archive &zip = globalZip;
  String fileName = "gradient.gr2";
  if (save) {
    dosRemoveFile(fileName.c_str());
    updateStatusBar();
    String grad = getGradientString(gradient);
    FILE *out = fopen(fileName.c_str(),"wb");
    fwrite(&grad[0],1,grad.size(),out);
    fclose(out);
    updateStatusBar();
    mz_zip_writer_add_file(&zip, fileName.c_str(), fileName.c_str(), "", 0, MZ_BEST_COMPRESSION|MZ_ZIP_FLAG_ASCII_FILENAME);
    updateStatusBar();
    dosRemoveFile(fileName.c_str());
    updateStatusBar();
  } else {
    Array<unsigned char> array;
    if (readZippedFile(readZipShortPath, readZipLongFileName, fileName.c_str(), array)) {
      gradientFromString((const char*)&array[0]);
      updateStatusBar();
    }
  }
}


void loadImage(const char *shortPath, const char *longFileName) {
  initStatusBar();
  bool allWentWell = true;
  readZipShortPath = shortPath;
  readZipLongFileName = longFileName;

  // canvas
  Array<Canvas *> canvasHere;
  String string = getStringFromZippedFile(shortPath,longFileName,"canvas.jsn");
  if (!string.empty()) {
    if (!serializeCanvas(canvasHere, false, &string)) {allWentWell = false;}
  } else allWentWell = false;

  // finished
  if (allWentWell) {
    if (!canvasHere.empty()) {
      addCanvas(canvasHere[0]);
      canvasHere.erase(0,1);
    }
  }
  for (int i = 0; i < canvasHere.size(); i++) delete canvasHere[i];
  canvasHere.clear();
  loadedSomething();
}

void saveImage(const char *shortPath, const char *longFileName) {
  initStatusBar();
  mz_zip_archive &zip = globalZip;
  mz_zip_zero_struct(&zip);

  String zipName = getTempFileName(shortPath);
  if (!mz_zip_writer_init_file(&zip, zipName.c_str(), 0)) return;

  String string; 
  bool allWentWell = true;

  // canvas
  Array<Canvas *> canvasHere; canvasHere.push_back(currentCanvas);
  if (serializeCanvas(canvasHere, true, &string)) {
    if (!mz_zip_writer_add_mem(&zip, "canvas.jsn", &string[0], string.size(), MZ_BEST_COMPRESSION|MZ_ZIP_FLAG_ASCII_FILENAME)) allWentWell = false;
  } else allWentWell = false;

  updateStatusBar();
  Layer *l = currentCanvas->getSelectionAsLayer(false,true,true);
  if (l != NULL) {
    updateStatusBar();
    String fileName = "image.png";
    RGBAImage img;
    img.data = l->pixels;
    img.width = l->width;
    img.height = l->height;
    dosRemoveFile(fileName.c_str());
    updateStatusBar();
    img.savePNG(fileName.c_str());
    updateStatusBar();
    mz_zip_writer_add_file(&zip, fileName.c_str(), fileName.c_str(), "", 0, MZ_BEST_COMPRESSION|MZ_ZIP_FLAG_ASCII_FILENAME);
    updateStatusBar();
    dosRemoveFile(fileName.c_str());
    updateStatusBar();
    delete l;
  }

  if (!mz_zip_writer_finalize_archive(&zip)) allWentWell = false;
  if (!mz_zip_writer_end(&zip)) allWentWell = false;
  renameTempFile(shortPath, longFileName);

  // finished
  if (allWentWell) {currentCanvas->signSave(); signSaveForUndo();}
  savedSomething();
}

void loadFont(const char *shortPath, const char *longFileName) {
  String fullFileName = String(shortPath)+String(longFileName);
  Array<unsigned char> *arr = new Array<unsigned char>();
  getFileArray(arr, fullFileName);
  if (!arr->empty()) {
    addFontSource(arr, fullFileName);
  } else {
    delete arr;
  }
}

unsigned int saveFontId = 0;
void saveFont(const char *shortPath, const char *longFileName) {
  if (saveFontId == 0) return;
  FontSource *font = NULL;
  for (int i = 0; i < fonts.size(); i++) {
    if (fonts[i]->id == saveFontId) {font = fonts[i]; break;}
  }
  if (font == NULL) return;
  if (font->data == NULL) return;
  if (font->data->size() == NULL) return;
  FILE *out = createTempFile(shortPath,"wb");
  fwrite(&((*(font->data))[0]),1,font->data->size(),out);
  fclose(out);
  renameTempFile(shortPath, longFileName);
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

  if (img.data == NULL) {
    if (toLower(getExtension(fileName)) == ".wyp") {
      Array<unsigned char> array;
      if (readZippedFile(filePath.c_str(), fileName.c_str(), "image.png", array)) {
        img = RGBAImage::fromMemory(&array[0],array.size());
      }
    }
  }

  if (img.data != NULL) {
    ret = new Layer();
    ret->init(img.width,img.height);
    delete[] ret->pixels;
    ret->pixels = img.data;
    // no free of img needed here
  }
  loadedSomething();
  return ret;
}

double updateToolTip(unsigned int *rgba, int width, int height, const char *path, const char *fileName) {
  Layer *l = loadImage(String(path), String(fileName));
  if (l != NULL && l->width > 0 && l->height > 0) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        rgba[x+y*width] = l->pixels[x*l->width/width + y*l->height/height * l->width];
      }
    }
    double aspect = (double)l->width/l->height;
    delete l;
    return aspect;
  }
  return 0;
}
