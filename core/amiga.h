
#include <cstdio>

typedef short          BOOL; // from <exec/types.h>
typedef unsigned long  ULONG;
typedef unsigned short UWORD;
typedef unsigned char  UBYTE;
typedef unsigned char  FLAG;
typedef char*          STRPTR;

#define MODULE static /* external static (file-scope) */
#define EXPORT

EXPORT void refresh(void);
EXPORT void flipulong(ULONG* theaddress);
EXPORT void flipuword(UWORD* theaddress);
EXPORT unsigned int afwrite(const void* ptr, unsigned int size, unsigned int n, FILE* f, UBYTE flip);
EXPORT void check_keyboard_status(void);
EXPORT void say(STRPTR text);
EXPORT UBYTE* displaycallback(void);
