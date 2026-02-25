/*
 
 */

#include <stdio.h>
#include <stdint.h>

#include <gem.h>
#include <mint/osbind.h>
#include <mint/mintbind.h>
#include <mint/cookie.h>
#include <mint/ssystem.h>

#include "main.h"
#include "../core/system.h"

static const char *APPL_NAME = "Handy";
static const char *BIOS_NAME = "lynxboot.img";
static const char *TITL_CART = "Select cartridge image file...";

static const char *ALERT_BIOS_NOT_FOUND = "[1][lynxboot.img not found.|Put it besides the program.][ Ok ]";
static const char *ALERT_FSEL_ISSUE = "[1][Could not open fileselector.][ Ok ]";

#define MAXPATHLEN 512
#define MAXNAMELEN 128

static char appl_pathname[MAXPATHLEN];
static char bios_pathname[MAXPATHLEN];
static char cart_pathname[MAXPATHLEN];
static char cart_filename[MAXNAMELEN];

//
// tools
//

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
  if (dot == NULL) { return 0; }
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

//
// main
//

int main(int argc, char *argv[])
{
  int16_t ap_id;
  
  int16_t win_handle;
  GRECT desk;
  int16_t wx, wy, ww, wh;
   
 	int16_t prevkc, prevks;
	
  static int16_t maskmouseb = 0;
  
  int16_t vdi_handle, dummy;
  int16_t vdi_bpp;
  int16_t work_in[12], work_out[272];

  uint32_t dummy2;
  int16_t modern_aes;
  int16_t argv_len;
  int16_t fsel_button;
  
  // app init

  ap_id = appl_init(); if (ap_id < 0) { return -1; }
    
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

  // vdi init

  vdi_handle = graf_handle(&dummy, &dummy, &dummy, &dummy); if (vdi_handle < 1) { return -2; }

 	work_in[0] = Getrez() + 2;
	for (int16_t i = 1; i < 10; i++) { work_in[i] = 1; }
	work_in[10] = 2;

	v_opnvwk(work_in, &vdi_handle, work_out); if (vdi_handle == 0) { return -2; }

	vq_extnd(vdi_handle, 1, work_out);
	vdi_bpp = work_out[4];
 
 	vsf_color(vdi_handle, 0);
	vsf_interior(vdi_handle, 1);
	vsf_perimeter(vdi_handle, 0);
	
  // find bios rom
  
  strcpy(bios_pathname, BIOS_NAME);
  
  if (!shel_find(bios_pathname)) { form_alert(1, ALERT_BIOS_NOT_FOUND);  goto exit_prg; }
	
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
  
 // win init
 
  wind_get(0, WF_WORKXYWH, &desk.g_x, &desk.g_y, &desk.g_w, &desk.g_h);
  
  wx = desk.g_x + ((desk.g_w - 160) / 2);
  wy = desk.g_y + ((desk.g_h - 102) / 2);
  ww = 160;
  wh = 102;
  
  win_handle = wind_create(NAME | CLOSER | MOVER | FULLER, wx, wy, ww, wh);
  
  if (win_handle < 0) { appl_exit(); return -1; }
  
  wind_set(win_handle, WF_NAME, (int16_t)(((unsigned long)APPL_NAME)>>16), (int16_t)(((unsigned long)APPL_NAME) & 0xffff), 0, 0);
  
  wind_open(win_handle, wx, wy, ww, wh);
  
  // main loop
  
  while (1)
  {
    int16_t ev_which;
    int16_t msg[8];
    int16_t mouse_event;
    int16_t mousex, mousey, mouseb;
    int16_t kstate, kc;

    ev_which = evnt_multi(
                          MU_MESAG | MU_TIMER | MU_KEYBD | MU_BUTTON,
                          0x101, 7, maskmouseb,
                          mouse_event,
                          0, 0, 0, 0,
                          0, 0, 0, 0, 0,
                          msg,
                          10,
                          &mousex, &mousey, &mouseb, &kstate, &kc, &dummy
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
          if (msg[3] == win_handle) { wind_set(win_handle, WF_CURRXYWH, msg[4], msg[5], msg[6], msg[7]); }
          break;
          
        case WM_TOPPED:
          if (msg[3] == win_handle) { wind_set(win_handle, WF_TOP, msg[4], 0, 0, 0); }
          break;

        case WM_REDRAW:
          if (msg[3] == win_handle)
          {
            int16_t inside_pxy[4], todo_pxy[4];

            wind_update(BEG_UPDATE);
            v_hide_c(vdi_handle);
            
            wind_get(win_handle, WF_WORKXYWH, &inside_pxy[0], &inside_pxy[1], &inside_pxy[2], &inside_pxy[3]);
            //inside_pxy[0] = msg[4];
            //inside_pxy[1] = msg[5];
            //inside_pxy[2] = msg[6];
            //inside_pxy[3] = msg[7];
            
            if (wind_get(win_handle, WF_FIRSTXYWH, &todo_pxy[0], &todo_pxy[1], &todo_pxy[2], &todo_pxy[3]) != 0)
            {
              while (todo_pxy[2] && todo_pxy[3])
              {
                if (rc_intersect((GRECT *)inside_pxy, (GRECT *)todo_pxy))
                {
                  todo_pxy[2] += todo_pxy[0] - 1;
                  todo_pxy[3] += todo_pxy[1] - 1;
                  
                  v_bar(vdi_handle, todo_pxy);
                }
                
                if (wind_get(win_handle, WF_NEXTXYWH, &todo_pxy[0], &todo_pxy[1], &todo_pxy[2], &todo_pxy[3]) == 0) { break; }
              }
            }
            
            wind_update(END_UPDATE);
            v_show_c(vdi_handle,1);
          }
          break;
      }
    }
    if (ev_which & MU_BUTTON)
    {
      // TODO: mouse events
      maskmouseb = mouseb & 7;
    }
    if (ev_which & MU_BUTTON)
    {
			if ((prevkc != kc) || (prevks != kstate))
      {
        // TODO: keyboard events
			}
    }
    if (ev_which & MU_TIMER)
    {
      // TODO: background tasks
    }
  }
  
exit_app:
  wind_close(win_handle);
  wind_delete(win_handle);
  
exit_prg:
  v_clsvwk(vdi_handle);
  appl_exit();

  return 0;
}
