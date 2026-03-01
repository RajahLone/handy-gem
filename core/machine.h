#include <stdint.h>

#ifndef MACHINE_H
#define MACHINE_H

#define ULONG uint32_t
#define SLONG int32_t
#define UWORD uint16_t
#define SWORD int16_t
#define UBYTE uint8_t
#define SBYTE int8_t
#define STRPTR char*

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

