//#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "amiga.h"
#include "system.h"

// lecgacy amiga port functions

EXPORT ULONG nextstop = 0;

EXPORT void say(STRPTR text) // TODO: outputs to logfile u:\ram\handy.log if MiNT
{
  printf(text);
  printf("\n");
}

EXPORT void flipulong(ULONG* theaddress)
{
  ULONG oldvalue, newvalue;
  
  oldvalue = *(theaddress);
  newvalue = (((oldvalue & 0xFF000000) >> 24)   // -> 0x000000FF
              + ((oldvalue & 0x00FF0000) >>  8) // -> 0x0000FF00
              + ((oldvalue & 0x0000FF00) <<  8) // -> 0x00FF0000
              + ((oldvalue & 0x000000FF) << 24) // -> 0xFF000000
              );
  
  *(theaddress) = newvalue;
}

EXPORT void flipuword(UWORD* theaddress)
{
  UWORD oldvalue, newvalue;
  
  oldvalue = *(theaddress);
  newvalue = (((oldvalue & 0xFF00) >> 8) // -> 0x00FF
              + ((oldvalue & 0x00FF) << 8) // -> 0xFF00
              );
  
  *(theaddress) = newvalue;
}

EXPORT unsigned int afwrite(const void* ptr, unsigned int size, unsigned int n, FILE* f, UBYTE flip)
{
  int          i, j;
  ULONG        dest2;
  unsigned int retval;
  
  /* We must take care not to alter the passed argument! */
  
  dest2 = (ULONG) malloc(size * n); // should check success of this
  memcpy((void*) dest2, ptr, size * n);
  
  if (flip == 2)
  {
    j = 0;
    for (i = 0; i < (int) n; i++)
    {
      flipuword((UWORD*) (dest2 + j));
      j += 2;
    }
  }
  else if (flip == 4)
  {
    j = 0;
    for (i = 0; i < (int) n; i++)
    {
      flipulong((ULONG*) (dest2 + j));
      j += 4;
    }
  }
  
  retval = fwrite((const void*) dest2, size, n, f);
  free((void*) dest2);
  return retval;
}

EXPORT int lss_read(void* dest, int varsize, int varcount, LSS_FILE* fp, UBYTE flip)
{
  int   i, j;
  ULONG copysize,
  dest2;
  
  copysize=varsize*varcount;
  if((fp->index + copysize) > fp->index_limit) copysize=fp->index_limit - fp->index;
  memcpy(dest,fp->memptr+fp->index,copysize);
  fp->index+=copysize;
  
  dest2 = (ULONG) dest;
  
  if (flip == 2)
  {
    j = 0;
    for (i = 0; i < varcount; i++)
    {
      flipuword((UWORD*) (dest2 + j));
      j += 2;
    }
  }
  else if (flip == 4)
  {
    j = 0;
    for (i = 0; i < varcount; i++)
    {
      flipulong((ULONG*) (dest2 + j));
      j += 4;
    }
  }
  
  return copysize;
}
