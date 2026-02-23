/*
 
 */

#include <stdio.h>
#include <sys/types.h>

#include <gem.h>
#include <mint/osbind.h>
#include <mint/cookie.h>

#include "main.h"
#include "../core/system.h"

static const char *app_name = "Handy";



int main(int argc, char *argv[])
{
  int16_t ap_id;
  
  GRECT desk;

  int16_t wind_handle;
  int16_t wx, wy, ww, wh;
  
 
 	int16_t prevkc, prevks;
	
  static int16_t maskmouseb = 0;

  
  ap_id = appl_init();
  
  if (ap_id < 0) { return -1; }
  
  wind_get(0, WF_WORKXYWH, &desk.g_x, &desk.g_y, &desk.g_w, &desk.g_h);
  
  wx = desk.g_x + ((desk.g_w - 160) / 2);
  wy = desk.g_y + ((desk.g_h - 102) / 2);
  ww = 160;
  wh = 102;
  
  wind_handle = wind_create(NAME | CLOSER | MOVER | FULLER, wx, wy, ww, wh);
  
  if (wind_handle < 0) { appl_exit(); return -1; }
  
  wind_set(wind_handle, WF_NAME, (int16_t)(((unsigned long)app_name)>>16), (int16_t)(((unsigned long)app_name) & 0xffff), 0, 0);
  
  wind_open(wind_handle, wx, wy, ww, wh);
  
  while (1)
  {
    int16_t ev_which;
    int16_t msg[8];
    int16_t mouse_event;
    int16_t mousex, mousey, mouseb, dummy;
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
          if (msg[3] == wind_handle) { goto exit_app; }
          break;
          
        case AP_TERM:
          goto exit_app;
          
        case WM_MOVED:
          if (msg[3] == wind_handle) { wind_set(wind_handle, WF_CURRXYWH, msg[4], msg[5], msg[6], msg[7]); }
          break;
      
        case WM_REDRAW:
          if (msg[3] == wind_handle)
          {
            /* Handle redraw - draw your content here */
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

  wind_close(wind_handle);
  wind_delete(wind_handle);
  
  appl_exit();
  
  return 0;
}
