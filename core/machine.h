#ifndef MACHINE_H
#define MACHINE_H

// Bytes should be 8-bits wide
typedef signed char SBYTE;
typedef unsigned char UBYTE;

// Words should be 16-bits wide
typedef signed short SWORD;
typedef unsigned short UWORD;

// Longs should be 32-bits wide
typedef signed long SLONG;
typedef unsigned long ULONG;

// Read/Write Cycle definitions
#define CPU_RDWR_CYC    5
#define DMA_RDWR_CYC    4
#define SPR_RDWR_CYC    3
// Ammended to 2 on 28/04/00, 16Mhz = 62.5nS cycle
//
//    2 cycles is 125ns - PAGE MODE CYCLE
//    4 cycles is 250ns - NORMAL MODE CYCLE
//

#ifndef TRUE
#define TRUE    true
#endif

#ifndef FALSE
#define FALSE   false
#endif

#include "lynxbase.h"

#endif

