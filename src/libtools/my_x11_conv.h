#ifndef MY_X11_CONV
#define MY_X11_CONV
#include <stdint.h>

#include "box32.h"
#include "converter32.h"
#include "my_x11_defs.h"
#include "my_x11_defs_32.h"

void convertXEvent(my_XEvent_32_t* dst, my_XEvent_t* src);
void unconvertXEvent(my_XEvent_t* dst, my_XEvent_32_t* src);
void convert_XErrorEvent_to_32(void* d, void* s);
void convert_XErrorEvent_to_64(void* d, void* s);

// Add a new Native Display*, return a 32bits one
void* addDisplay(void* d);
// Find a Native Diplay* and return the 32bits one
void* FindDisplay(void* d);
// return the Native Display from a 32bits one
void* getDisplay(void* d);
// removed a 32bits Display and associated ressources
void delDisplay(void* d);

void convert_Screen_to_32(void* d, void* s);

void convert_XWMints_to_64(void* d, void* s);
void inplace_enlarge_wmhints(void* hints);
void inplace_shrink_wmhints(void* hints);
void convert_XSizeHints_to_64(void* d, void *s);
void inplace_enlarge_wmsizehints(void* hints);
void inplace_shrink_wmsizehints(void* hints);

void convert_XWindowAttributes_to_32(void* d, void* s);

void inplace_XModifierKeymap_shrink(void* a);
void inplace_XModifierKeymap_enlarge(void* a);

void convert_XVisualInfo_to_32(my_XVisualInfo_32_t* dst, my_XVisualInfo_t* src);
void convert_XVisualInfo_to_64(my_XVisualInfo_t* dst, my_XVisualInfo_32_t* src);
void inplace_XVisualInfo_shrink(void *a);
void inplace_XVisualInfo_enlarge(void *a);

void inplace_XdbeVisualInfo_shrink(void* a);
void inplace_XdbeScreenVisualInfo_shrink(void* a);
void inplace_XdbeVisualInfo_enlarge(void* a);
void inplace_XdbeScreenVisualInfo_enlarge(void* a);

void inplace_XExtDisplayInfo_shrink(void* a);
void inplace_XExtDisplayInfo_enlarge(void* a);
void* inplace_XExtensionInfo_shrink(void* a);
void* inplace_XExtensionInfo_enlarge(void* a);

void convert_XFontProp_to_32(my_XFontProp_32_t* dst, my_XFontProp_t* src);
void convert_XFontProp_to_64(my_XFontProp_t* dst, my_XFontProp_32_t* src);
void inplace_XFontProp_shrink(void* a);
void inplace_XFontProp_enlarge(void* a);
void inplace_XFontStruct_shrink(void* a);
void inplace_XFontStruct_enlarge(void* a);

void convert_XSetWindowAttributes_to_64(my_XSetWindowAttributes_t* dst, my_XSetWindowAttributes_32_t* src);

void WrapXImage(void* d, void* s);  //define in wrappedx11.c because it contains callbacks
void UnwrapXImage(void* d, void* s);
void* inplace_XImage_shrink(void* a);
void* inplace_XImage_enlarge(void* a);

void convert_XRRModeInfo_to_32(void* d, const void* s);
void convert_XRRModeInfo_to_64(void* d, const void* s);
void inplace_XRRScreenResources_shrink(void* s);
void inplace_XRRScreenResources_enlarge(void* s);
void inplace_XRRCrtcInfo_shrink(void* s);
void inplace_XRROutputInfo_shrink(void* s);
void inplace_XRRProviderInfo_shrink(void* a);
void inplace_XRRProviderInfo_enlarge(void* a);
void inplace_XRRProviderResources_shrink(void* a);
void inplace_XRRProviderResources_enlarge(void* a);
void* inplace_XRRPropertyInfo_shrink(void* a);

#endif//MY_X11_CONV