
#include <stdint.h>

#define ROUNDTOLONG(x) ((((x) + 15) >> 4) << 4)
#define MIN(a, b)      (((a) < (b)) ? (a) : (b))
#define MAX(a, b)      (((a) > (b)) ? (a) : (b))

uint8_t* displaycallback(void);
