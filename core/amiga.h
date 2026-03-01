
#include <cstdio>
#include <stdint.h>

#define ULONG uint32_t
#define SLONG int32_t
#define UWORD uint16_t
#define SWORD int16_t
#define UBYTE uint8_t
#define SBYTE int8_t
#define STRPTR char*

#define MODULE static /* external static (file-scope) */
#define EXPORT

EXPORT void refresh(void);
EXPORT void flipuint(ULONG* theaddress);
EXPORT void flipuword(UWORD* theaddress);
EXPORT ULONG afwrite(const void* ptr, SLONG size, SLONG n, FILE* f, UBYTE flip);
EXPORT void check_keyboard_status(void);
EXPORT void say(STRPTR text);
EXPORT UBYTE* displaycallback(void);
