//
//  utils.cpp
//

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <gem.h>
#include <mint/osbind.h>
#include <mint/mintbind.h>
#include <mint/cookie.h>
#include <mint/ssystem.h>


int16_t fsel_cart(char *path, char *name, int16_t *button, const char *label)
{
  int16_t ret;
  
  wind_update(BEG_MCTRL);
  if (_AESversion < 0x0104) { ret = fsel_input(path, name, button); } else { ret = fsel_exinput(path, name, button, (char *)label); }
  wind_update(END_MCTRL);
      
  return(ret);
}

// from MS copilot
int16_t has_extension(const char *filename, const char *ext)
{
  const char *dot = strrchr(filename, '.');
  if (!dot) { return 0; }
  return (strcasecmp(dot, ext) == 0);
}

// from libcmini
int16_t get_current_dir(int16_t drive, char *path) { int ret = Dgetpath(path, drive); if (ret < 0) { ret = -1; } return ret; }

// from Daroou's sources
uint32_t get_cookie_table_ptr(void) { return (*((uint32_t *)0x5A0L)); }
uint32_t *get_cookie_table(void) { return ((uint32_t *)Supexec(get_cookie_table_ptr)); }
int16_t get_cookie(const uint32_t cookie_id, uint32_t *cookie_val)
{
  *cookie_val = 0;

  int32_t ret = Ssystem(S_GETCOOKIE, cookie_id, 0);
  
  if (ret == -1) { return 0; }
  
  if (ret != -32) { *cookie_val = ret; return 1; }

  uint32_t *cookie_lst = get_cookie_table();

  if (cookie_lst == (void *)0) { return 0; }

  while (*cookie_lst) { if( *cookie_lst == cookie_id) { *cookie_val = *(cookie_lst + 1); return 1; } cookie_lst += 2; }

  return 0;
}

// from Anders Granlund (ScummST Atari port)
uint16_t Mxmask(void)
{
  uint16_t mxmask = 0xFFFF;
  
  int32_t sRAM  = (int32_t) Mxalloc( -1, 0);
  int32_t sRAMg = (int32_t) Mxalloc( -1, 0x40); /* In error case Mxalloc( -1, 3) */
  int32_t aRAM  = (int32_t) Mxalloc( -1, 1);
  int32_t aRAMg = (int32_t) Mxalloc( -1, 0x41); /* In error case Mxalloc( -1, 3) */
  
  if (sRAM == -32) { mxmask = 0x0000; /* Mxalloc is not implemented */ }
  else if ( ((sRAM + aRAM) == sRAMg) && ((sRAM + aRAM) == aRAMg) ) { mxmask = 0x0003; /* oldfashion Mxalloc() */ }
  else { mxmask = 0xFFFF; /* newfashion Mxalloc() */ }
    
  return mxmask;
}

