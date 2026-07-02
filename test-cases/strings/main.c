#include <stdio.h>

#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x)
#define PASTE_INNER(x, y) x##y
#define PASTE(x, y) PASTE_INNER(x, y)

//#ifdef DEFINED_STRING1
void PASTE(func_, DEFINED_STRING1)(void) {
	puts(STRINGIFY(DEFINED_STRING1));
}
//#endif

#ifdef DEFINED_STRING2
void PASTE(func_, DEFINED_STRING2)(void) {
	puts(STRINGIFY(DEFINED_STRING2));
}
#endif
