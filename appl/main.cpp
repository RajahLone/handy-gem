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
  
  int16_t win_handle;
  GRECT desk;
  int16_t wx, wy, ww, wh;
   
 	int16_t prevkc, prevks;
	
  static int16_t maskmouseb = 0;
  
  int16_t vdi_handle, dummy;
  int16_t vdi_bpp;
  int16_t work_in[12], work_out[272];

  ap_id = appl_init(); if (ap_id < 0) { return -1; }
  
 	graf_mouse(ARROW, NULL);

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
	
 
  wind_get(0, WF_WORKXYWH, &desk.g_x, &desk.g_y, &desk.g_w, &desk.g_h);
  
  wx = desk.g_x + ((desk.g_w - 160) / 2);
  wy = desk.g_y + ((desk.g_h - 102) / 2);
  ww = 160;
  wh = 102;
  
  win_handle = wind_create(NAME | CLOSER | MOVER | FULLER, wx, wy, ww, wh);
  
  if (win_handle < 0) { appl_exit(); return -1; }
  
  wind_set(win_handle, WF_NAME, (int16_t)(((unsigned long)app_name)>>16), (int16_t)(((unsigned long)app_name) & 0xffff), 0, 0);
  
  wind_open(win_handle, wx, wy, ww, wh);
  
  
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
  
  v_clsvwk(vdi_handle);
  appl_exit();

  return 0;
}
