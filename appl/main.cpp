//
// main.cpp
//

#include <stdio.h>
#include <stdint.h>

#include <gem.h>
#include <gemx.h>
#include <mint/osbind.h>
#include <mint/mintbind.h>
#include <mint/cookie.h>
#include <mint/ssystem.h>

#include "main.h"
#include "utils.h"

#include "../core/system.h"

//
// global constants
//

static const char *APPL_NAME = "Handy";
static const char *BIOS_NAME = "lynxboot.img";
static const char *TITL_CART = "Select cartridge image file...";

static const char *ALERT_BIOS_NOT_FOUND = "[1][lynxboot.img not found.|Put it besides the program.][ Quit ]";
static const char *ALERT_FSEL_ISSUE = "[1][Could not open fileselector.][ Quit ]";
static const char *ALERT_NO_SOFTWARE_CLUT = "[1][HC15 to TC32 screenmode only.][ Quit ]";
static const char *ALERT_NOT_ENOUGH_RAM = "[1][Not enough memory for Lynx screen.][ Quit ]";

//
// global variables
//

uint16_t mxMask = 0xFFFF;

#define MAXPATHLEN 512
#define MAXNAMELEN 128

static char appl_pathname[MAXPATHLEN];
static char bios_pathname[MAXPATHLEN];
static char cart_pathname[MAXPATHLEN];
static char cart_filename[MAXNAMELEN];

#define VDI_BIT_ORDER_CLASSIC 0
#define VDI_BIT_ORDER_FALCON  1
#define VDI_BIT_ORDER_INTEL   2

static CSystem   *Lynx;
static uint16_t  viewport_rotate = 0;
static uint16_t  viewport_scale  = 1;
static uint16_t  viewport_width  = HANDY_SCREEN_WIDTH;
static uint16_t  viewport_height = HANDY_SCREEN_HEIGHT;
static uint16_t  viewport_pixel_format;
static uint16_t  viewport_pitch = 1;
static uint8_t   *viewport_buffer;

static uint16_t  scaling_available = 0;
static uint16_t  scaled_width  = HANDY_SCREEN_WIDTH;
static uint16_t  scaled_height = HANDY_SCREEN_HEIGHT;
static uint8_t   *scaled_buffer;

static int16_t vdi_bpp;
static int16_t vdi_handle;
static uint16_t screen_width;
static uint16_t screen_height;

static int16_t dx, dy, dw, dh;
static int16_t win_components;
static int16_t win_handle;
static int16_t redraw_pxy[4], window_pxy[4], todo_pxy[4];
static MFDB viewport_mfdb;
static MFDB scaled_mfdb;
static MFDB screen_mfdb;

//
// contents redraw
//

void win_redraw(int16_t rx, int16_t ry, int16_t rw, int16_t rh)
{
  int16_t viewport_pxy[8];

  wind_update(BEG_UPDATE);
  v_hide_c(vdi_handle);
  
  wind_get(win_handle, WF_WORKXYWH, &window_pxy[0], &window_pxy[1], &window_pxy[2], &window_pxy[3]);
  
  wind_get(win_handle, WF_FIRSTXYWH, &todo_pxy[0], &todo_pxy[1], &todo_pxy[2], &todo_pxy[3]);
  
  if ((todo_pxy[0] + todo_pxy[2] - 1) > screen_width) { todo_pxy[2] = screen_width - todo_pxy[0]; }
  if ((todo_pxy[1] + todo_pxy[3] - 1) > screen_height) { todo_pxy[3] = screen_height - todo_pxy[1]; }
  
  if (rw == 0 || rh == 0)
  {
    redraw_pxy[0] = window_pxy[0];
    redraw_pxy[1] = window_pxy[1];
    redraw_pxy[2] = window_pxy[2];
    redraw_pxy[3] = window_pxy[3];
  }
  else
  {
    redraw_pxy[0] = rx;
    redraw_pxy[1] = ry;
    redraw_pxy[2] = rw;
    redraw_pxy[3] = rh;
  }
  
  while (todo_pxy[2] && todo_pxy[3])
  {
    if (rc_intersect((GRECT *)redraw_pxy, (GRECT *)todo_pxy))
    {
      if (viewport_scale > 1)
      {
        viewport_pxy[0] = (todo_pxy[0] - window_pxy[0]);
        viewport_pxy[1] = (todo_pxy[1] - window_pxy[1]);
        viewport_pxy[2] = MIN(todo_pxy[2], scaled_width) + viewport_pxy[0] - 1;
        viewport_pxy[3] = MIN(todo_pxy[3], scaled_height) + viewport_pxy[1] - 1;
        
        viewport_pxy[4] = todo_pxy[0];
        viewport_pxy[5] = todo_pxy[1];
        viewport_pxy[6] = MIN(todo_pxy[2], scaled_width) + viewport_pxy[4] - 1;
        viewport_pxy[7] = MIN(todo_pxy[3], scaled_height) + viewport_pxy[5] - 1;
        
        vro_cpyfm(vdi_handle, S_ONLY, viewport_pxy, &scaled_mfdb, &screen_mfdb);
      }
      else
      {
        viewport_pxy[0] = (todo_pxy[0] - window_pxy[0]);
        viewport_pxy[1] = (todo_pxy[1] - window_pxy[1]);
        viewport_pxy[2] = MIN(todo_pxy[2], viewport_width) + viewport_pxy[0] - 1;
        viewport_pxy[3] = MIN(todo_pxy[3], viewport_height) + viewport_pxy[1] - 1;
        
        viewport_pxy[4] = todo_pxy[0];
        viewport_pxy[5] = todo_pxy[1];
        viewport_pxy[6] = MIN(todo_pxy[2], viewport_width) + viewport_pxy[4] - 1;
        viewport_pxy[7] = MIN(todo_pxy[3], viewport_height) + viewport_pxy[5] - 1;
        
        vro_cpyfm(vdi_handle, S_ONLY, viewport_pxy, &viewport_mfdb, &screen_mfdb);
      }
    }
    
    wind_get(win_handle, WF_NEXTXYWH, &todo_pxy[0], &todo_pxy[1], &todo_pxy[2], &todo_pxy[3]);
    
    if ((todo_pxy[0] + todo_pxy[2] - 1) > screen_width) { todo_pxy[2] = screen_width - todo_pxy[0]; }
    if ((todo_pxy[1] + todo_pxy[3] - 1) > screen_height) { todo_pxy[3] = screen_height - todo_pxy[1]; }
  }
  
  wind_update(END_UPDATE);
  v_show_c(vdi_handle, 1);
}

//
// contents resize
//

int16_t win_resize(uint16_t new_rotation, uint16_t new_scale)
{
  uint32_t  viewport_size;
  uint32_t  scaled_size;
  uintptr_t viewport_temp;
  uintptr_t scaled_temp;

  viewport_rotate = new_rotation;
  if ((viewport_rotate < MIKIE_NO_ROTATE) || (viewport_rotate > MIKIE_ROTATE_R)) { viewport_rotate = MIKIE_NO_ROTATE; }

  viewport_scale = new_scale;
  if (!scaling_available) { viewport_scale = 1; } else if ((viewport_scale < 1) || (viewport_scale > 3)) { viewport_scale = 1; }

  switch (viewport_rotate)
  {
    case MIKIE_NO_ROTATE:
      viewport_width  = (uint16_t)HANDY_SCREEN_WIDTH;
      viewport_height = (uint16_t)HANDY_SCREEN_HEIGHT;
      break;
    case MIKIE_ROTATE_L:
    case MIKIE_ROTATE_R:
      viewport_width  = (uint16_t)HANDY_SCREEN_HEIGHT;
      viewport_height = (uint16_t)HANDY_SCREEN_WIDTH;
      break;
  }

  Lynx->DisplaySetAttributes(viewport_rotate, viewport_pixel_format, ROUNDTOLONG(viewport_width) * viewport_pitch);

  // allocate memory for viewport

  if (viewport_buffer) { Mfree(viewport_buffer); viewport_buffer = NULL; }
  if (scaled_buffer) { Mfree(scaled_buffer); scaled_buffer = NULL; }


  viewport_size = (uint32_t)((ROUNDTOLONG(viewport_width) >> 3) * viewport_height * vdi_bpp);
  viewport_temp = (uintptr_t)(mxMask ? Mxalloc(viewport_size, 0x43 & mxMask) : Malloc(viewport_size));

  if (viewport_temp < 1) { form_alert(1, ALERT_NOT_ENOUGH_RAM); return 1; }
  
  viewport_buffer = (uint8_t *)viewport_temp;
  
  memset(viewport_buffer, 0, viewport_size);
  
  viewport_mfdb.fd_addr		  =	viewport_buffer;
  viewport_mfdb.fd_w			  =	viewport_width;
  viewport_mfdb.fd_h			  =	viewport_height;
  viewport_mfdb.fd_wdwidth	=	(ROUNDTOLONG(viewport_width) >> 4);
  viewport_mfdb.fd_stand	  =	0;
  viewport_mfdb.fd_nplanes	=	vdi_bpp;
  
  scaled_width = (uint16_t)(viewport_width * viewport_scale);
  scaled_height = (uint16_t)(viewport_height * viewport_scale);

  if (viewport_scale > 1)
  {
    scaled_size = (uint32_t)((ROUNDTOLONG(scaled_width) >> 3) * scaled_height * vdi_bpp);
    scaled_temp = (uintptr_t)(mxMask ? Mxalloc(scaled_size, 0x43 & mxMask) : Malloc(scaled_size));
    
    if (scaled_temp < 1) { form_alert(1, ALERT_NOT_ENOUGH_RAM); return 1; }
    
    scaled_buffer = (uint8_t *)scaled_temp;
    
    memset(scaled_buffer, 0, viewport_size);
    
    scaled_mfdb.fd_addr		  =	scaled_buffer;
    scaled_mfdb.fd_w			  =	scaled_width;
    scaled_mfdb.fd_h			  =	scaled_height;
    scaled_mfdb.fd_wdwidth	=	(ROUNDTOLONG(scaled_width) >> 4);
    scaled_mfdb.fd_stand	  =	0;
    scaled_mfdb.fd_nplanes	=	vdi_bpp;
  }
  
  if (win_handle)
  {
    int16_t ox, oy, ow, oh;

    wind_get(win_handle, WF_WORKXYWH, &window_pxy[0], &window_pxy[1], &window_pxy[2], &window_pxy[3]);
  
    wind_calc(0, win_components, window_pxy[0], window_pxy[1], (int16_t)scaled_width, (int16_t)scaled_height, &ox, &oy, &ow, &oh);

    wind_set(win_handle, WF_CURRXYWH, MAX(ox, dx + 1), MAX(oy, dy + 1), ow, oh);
    
    win_redraw(0, 0, 0, 0);
  }
  
  return 0;
}

//
// handy callback display
//

uint8_t* displaycallback(void)
{
  int16_t zoom_pxy[8];
  
  if (viewport_scale > 1)
  {
    zoom_pxy[0] = 0;
    zoom_pxy[1] = 0;
    zoom_pxy[2] = viewport_width - 1;
    zoom_pxy[3] = viewport_height - 1;
    
    zoom_pxy[4] = 0;
    zoom_pxy[5] = 0;
    zoom_pxy[6] = scaled_width - 1;
    zoom_pxy[7] = scaled_height - 1;
    
    vro_cpyfm(vdi_handle, S_ONLY | 0x8000, zoom_pxy, &viewport_mfdb, &scaled_mfdb);
  }
  
  win_redraw(0, 0, 0, 0);
  
  return viewport_buffer;
}

//
// main
//

int32_t main(int32_t argc, char *argv[])
{
  int16_t ap_id;
  
  int16_t wx, wy, ww, wh;
  int16_t ox, oy, ow, oh;
   
  int16_t msg[8];
	  
  int16_t dummy;
  int16_t work_in[12], work_out[272];
  uint16_t vdi_pixel_format;

  uint32_t dummy2;
  int16_t modern_aes;
  int16_t argv_len;
  int16_t fsel_button;
  
  uint16_t loop;
  
  // app init

  ap_id = appl_init(); if (ap_id < 0) { return -1; }
  
  mxMask = Mxmask();
  
 	graf_mouse(ARROW, NULL);
  get_current_dir(0, appl_pathname);
 
  modern_aes = 0;
  
  if (get_cookie(C_MiNT, &dummy2)) { modern_aes = 1; }
  else
  if (get_cookie(C_MagX, &dummy2)) { modern_aes = 1; }
  
  if (modern_aes)
  {
    shel_write(9, 1, 1, "", "");
    Pdomain(1);
  }
 
  wind_get(0, WF_WORKXYWH, &dx, &dy, &dw, &dh);
  
  wx = dx + ((dw - viewport_width) / 2);
  wy = dy + ((dh - viewport_height) / 2);
  ww = viewport_width;
  wh = viewport_height;
  
  screen_width = dx + dw;
  screen_height = dy + dh;

  // vdi init

  vdi_handle = graf_handle(&dummy, &dummy, &dummy, &dummy); if (vdi_handle < 1) { return -2; }

 	work_in[0] = Getrez() + 2;
	for (int16_t i = 1; i < 10; i++) { work_in[i] = 1; }
	work_in[10] = 2;

	v_opnvwk(work_in, &vdi_handle, work_out); if (vdi_handle == 0) { return -2; }

	vq_extnd(vdi_handle, 1, work_out);
	vdi_bpp = work_out[4];
  vdi_pixel_format = VDI_BIT_ORDER_CLASSIC;
  if (work_out[30] & 0x1) { scaling_available = 1; } else { scaling_available = 0; }

 	vsf_color(vdi_handle, 0);
	vsf_interior(vdi_handle, 1);
	vsf_perimeter(vdi_handle, 0);
 
  if (get_cookie(C_EdDI, &dummy2))
  {
    vq_scrninfo(vdi_handle, work_out);
    
    vdi_bpp = work_out[2];
    
    if (work_out[1] != 2) { form_alert(1, ALERT_NO_SOFTWARE_CLUT); goto exit_prg; }
    
    if ((work_out[14] & 2) && (vdi_bpp < 24)) { vdi_pixel_format = VDI_BIT_ORDER_FALCON; }
    
    if (work_out[14] & 128) { vdi_pixel_format = VDI_BIT_ORDER_INTEL; }
  }
  
  if (vdi_bpp < 15) { form_alert(1, ALERT_NO_SOFTWARE_CLUT); goto exit_prg; }
 
  screen_mfdb.fd_addr		  =	0;
  screen_mfdb.fd_w			  =	0;
  screen_mfdb.fd_h			  =	0;
  screen_mfdb.fd_wdwidth	=	0;
  screen_mfdb.fd_stand	  =	0;
  screen_mfdb.fd_nplanes	=	0;
 
  // find bios rom
  
  strcpy(bios_pathname, BIOS_NAME);
  
  if (!shel_find(bios_pathname)) { form_alert(1, ALERT_BIOS_NOT_FOUND); goto exit_prg; }
	
  // find cart rom: in argv or use fileselector
  
  cart_pathname[0] = 0;
  
  if (argc > 1)
  {
    argv_len = strlen(argv[1]);
    if ((argv_len > 0) && (argv_len < MAXPATHLEN))
    {
      if (has_extension(argv[1], ".lnx")) { strcpy(cart_pathname, argv[1]); }
    }
  }
  
  if (strlen(cart_pathname) == 0)
  {
    strcpy(cart_pathname, appl_pathname);
    strcat(cart_pathname, "\\*.lnx");

    if (fsel_cart(cart_pathname, cart_filename, &fsel_button, TITL_CART) == 0) { form_alert(1, ALERT_FSEL_ISSUE); goto exit_prg; }
    
    if (fsel_button == 0) { goto exit_prg; }

    if (cart_pathname[0] && cart_filename[0])
    {
      argv_len = strlen(cart_pathname);
      while ( cart_pathname[argv_len - 1] != '\\' ) { --argv_len; }
      cart_pathname[argv_len] = 0;
      strcat(cart_pathname, cart_filename);
      
      if (!has_extension(cart_pathname, ".lnx")) { goto exit_prg; }
    }
  }
  
  // instanciate handy

  gAudioEnabled = FALSE; // audio is disabled
  
  try { Lynx = new CSystem(cart_pathname, bios_pathname); } catch (...) { goto exit_prg; }
  
  nextstop = gSystemCycleCount;

  if (win_resize(Lynx->mCart->CartGetRotate(), viewport_scale)) { goto exit_prg; }
  
  // TODO: vdi_pixel_format <- VDI_BIT_ORDER_INTEL, VDI_BIT_ORDER_FALCON;
  switch(vdi_bpp)
  {
    case 15:
      if (vdi_pixel_format == VDI_BIT_ORDER_INTEL) { viewport_pixel_format = MIKIE_PIXEL_FORMAT_16BPP_555; }
      else { viewport_pixel_format = MIKIE_PIXEL_FORMAT_16BPP_555; }
      viewport_pitch = 2;
      break;
    case 16:
      if (vdi_pixel_format == VDI_BIT_ORDER_FALCON) { viewport_pixel_format = MIKIE_PIXEL_FORMAT_16BPP_555; }
      else if (vdi_pixel_format == VDI_BIT_ORDER_INTEL) { viewport_pixel_format = MIKIE_PIXEL_FORMAT_16BPP_565; }
      else { viewport_pixel_format = MIKIE_PIXEL_FORMAT_16BPP_565; }
      viewport_pitch = 2;
      break;
    case 24:
      if (vdi_pixel_format == VDI_BIT_ORDER_INTEL) { viewport_pixel_format = MIKIE_PIXEL_FORMAT_24BPP; }
      else { viewport_pixel_format = MIKIE_PIXEL_FORMAT_24BPP; }
      viewport_pitch = 3;
      break;
    case 32:
      if (vdi_pixel_format == VDI_BIT_ORDER_INTEL) { viewport_pixel_format = MIKIE_PIXEL_FORMAT_32BPP; }
      else { viewport_pixel_format = MIKIE_PIXEL_FORMAT_32BPP; }
      viewport_pitch = 4;
      break;
    default:
      viewport_pixel_format = MIKIE_PIXEL_FORMAT_8BPP;
      viewport_pitch = 1;
      break;
  }

  Lynx->DisplaySetAttributes(viewport_rotate, viewport_pixel_format, ROUNDTOLONG(viewport_width) * viewport_pitch);

  // win init

  win_components = NAME | CLOSER | MOVER; if (scaling_available) { win_components |= FULLER; }

  wind_calc(0, win_components, wx, wy, ww, wh, &ox, &oy, &ow, &oh);
  
  ox = MAX(ox, dx + 1);
  oy = MAX(oy, dy + 1);

  win_handle = wind_create(win_components, ox, oy, ow, oh);
  
  if (win_handle < 1) { appl_exit(); return -1; }
  
  wind_set(win_handle, WF_NAME, (int16_t)(((uint32_t)APPL_NAME)>>16), (int16_t)(((uint32_t)APPL_NAME) & 0xffff), 0, 0);
  
  wind_open(win_handle, ox, oy, ow, oh);

  // main loop
  
  while (1)
  {
    int16_t ev_which, kstate, kc;

    ev_which = evnt_multi(
                          MU_MESAG | MU_TIMER | MU_KEYBD,
                          0,0,0,
                          0,0,0,0,0,
                          0,0,0,0,0,
                          msg,
                          10,
                          &dummy, &dummy, &dummy, &kstate, &kc, &dummy
                          );
    
    if (ev_which & MU_MESAG)
    {
      switch (msg[0])
      {
        case WM_CLOSED:
          if (msg[3] == win_handle) { goto exit_app; }
          break;
          
        case AP_TERM:
          goto exit_app;
          
        case WM_MOVED:
          if (msg[3] == win_handle) { wind_set(win_handle, WF_CURRXYWH, MAX(dx + 1, msg[4]), MAX(dy + 1, msg[5]), msg[6], msg[7]); }
          break;
          
        case WM_TOPPED:
          if (msg[3] == win_handle) { wind_set(win_handle, WF_TOP, msg[4], 0, 0, 0); }
          break;

        case WM_FULLED:
          if (msg[3] == win_handle) { if (scaling_available) { if (win_resize(viewport_rotate, ++viewport_scale)) { goto exit_prg; } } }
          break;

        case WM_REDRAW:
          if (msg[3] == win_handle) { win_redraw(msg[4], msg[5], msg[6], msg[7]); }
          break;
      }
    }
    if (ev_which & MU_KEYBD)
    {
      uint32_t OldKeyMask, KeyMask = Lynx->GetButtonData();
      OldKeyMask = KeyMask;

      if (kstate == K_CTRL)
      {
        switch (kc & 0xff)
        {
          case 17: // Ctrl+Q
          case 21: // Ctrl+U
            goto exit_app;
            break;
          case 18: // Ctrl+R
            if (win_resize(++viewport_rotate, viewport_scale)) { goto exit_prg; }
            break;
          case 6:  // Ctrl+F
            if (scaling_available) { if (win_resize(viewport_rotate, ++viewport_scale)) { goto exit_prg; } }
            break;
        }
      }
      else if (kstate == 0)
      {
        switch ((kc >> 8) & 0xff)
        {
          case 0x48: // up arrow
            KeyMask |= BUTTON_UP;
            break;
          case 0x4b: // left arrow
            KeyMask |= BUTTON_LEFT;
            break;
          case 0x4d: // right arrow
            KeyMask |= BUTTON_RIGHT;
            break;
          case 0x50: // down arrow
            KeyMask |= BUTTON_DOWN;
            break;
          case 0x3b: // F1
          case 0x54: // Shift+F1
            KeyMask |= BUTTON_OPT1;
            break;
          case 0x3c: // F2
          case 0x55: // Shift+F2
            KeyMask |= BUTTON_OPT2;
            break;
          default:
            switch (kc & 0xff)
            {
              case 0x41: // A
              case 0x61: // a
                KeyMask |= BUTTON_A;
                break;
              case 0x42: // B
              case 0x62: // b
                KeyMask |= BUTTON_B;
                break;
              case 0x50: // P
              case 0x70: // p
                KeyMask |= BUTTON_PAUSE;
                break;
            }
        }

        if (OldKeyMask != KeyMask) { Lynx->SetButtonData(KeyMask); }
			}
    }
    if (ev_which & MU_TIMER)
    {
      nextstop += HANDY_SYSTEM_FREQ / HANDY_TIMER_FREQ; // ie. 800,000 cycles (50ms)
      nextstop -= 10;                                   // minus 10ms of MU_TIMER

      do { for (loop = 1024; loop; loop--) { Lynx->Update(); } } while (gSystemCycleCount < nextstop);
      
      // TODO: fix timers managment (currently too slow)
    }
  }
  
exit_app:
  wind_close(win_handle);
  wind_delete(win_handle);
  
  if (viewport_buffer) { Mfree(viewport_buffer); }
  if (scaled_buffer) { Mfree(scaled_buffer); }
  
exit_prg:
  v_clsvwk(vdi_handle);
  appl_exit();

  return 0;
}
