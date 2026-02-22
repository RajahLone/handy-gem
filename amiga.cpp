#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <libraries/amigaguide.h>
#include <libraries/gadtools.h>

#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#ifdef __amigaos4__
#include <dos/obsolete.h>
#include <libraries/application.h>
#endif
#ifdef __amigaos4__
#include <proto/application.h>
#endif
#include <proto/asl.h>
#include <proto/icon.h>
#include <proto/lowlevel.h>
#include <proto/timer.h>
#include <proto/amigaguide.h>
#include <proto/gadtools.h>
#include <proto/wb.h>
#include <clib/alib_protos.h>

#include <exec/execbase.h>

#include "amiga.h"
#include "system.h"

#define ALL_REACTION_CLASSES
#define ALL_REACTION_MACROS
#include <reaction/reaction.h>

#define SOUND_SAMPLERATE   44100
#define SAMPLES             1378
#define AUDIO_BUFFER        1378
#define DEFAULT_BOOTIMAGE  "PROGDIR:lynxboot.img"

const char ver[] =         "\0$VER: Handy V0.95 R2.4 (2.7.2017)";
#define VERSION            "2.4"

#define PENS               256

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS     0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE    20
#endif

#ifndef __amigaos4__
#define ZERO           (BPTR) NULL
#endif

#define TEMPLATE "FILE,NOSOUND/S,TURBO/S,OVERCLOCK/S,NOPOINTER/S,NOSTATUSBAR/S,NOTITLEBAR/S,NOTOOLBAR/S,SIZE/N,PUBSCREEN/K"

#define OPT_FILE        0
#define OPT_NOSOUND     1
#define OPT_TURBO       2
#define OPT_OVERCLOCK   3
#define OPT_NOPOINTER   4
#define OPT_NOSTATUSBAR 5
#define OPT_NOTITLEBAR  6
#define OPT_NOTOOLBAR   7
#define OPT_SIZE        8
#define OPT_PUBSCREEN   9
#define OPTS           10

#define GID_LY1        0
#define GID_SP1        1
#define GID_SB1        2
#define GID_SB2        3
#define GID_IN1        4
#define GID_ST1        5
#define GIDS           GID_ST1

#define ROUNDTOLONG(x)    ((((x) + 15) >> 4) << 4)
#define GFXINIT(x,y)      (ROUNDTOLONG(x) * (y))
#define GFXACCESS(x,y,xx) ((x) + ((y) * ROUNDTOLONG(xx)))

#define BIGGADGETS         3 // ×2 images for each = 6
#define SMALLGADGETS       4 // ×2 images for each = 8
#define IMAGES            14
#define BUTTONSPACE       12

#define GADPOS_AUTOFIREA 0
#define GADPOS_AUTOFIREB 1
#define GADPOS_TURBO     2
#define GADPOS_SOUND     3

#ifndef BITMAP_Transparent          // OS4-only tag
#define BITMAP_Transparent      (BITMAP_Dummy + 18)
#endif

EXPORT int               use_audio      = 1;
EXPORT char              gamefile[MAX_PATH + 1];
EXPORT UBYTE             overclock      = FALSE,
turbo          = FALSE;
EXPORT UBYTE            *vbuffer        = NULL,
*bytebuffer     = NULL;
EXPORT ULONG             nextstop       = 0;
EXPORT struct Device*    TimerBase      = NULL;
EXPORT CSystem*          Lynx;

EXPORT struct Library   *BitMapBase     = NULL,
*IconBase       = NULL,
*IntegerBase    = NULL,
*LabelBase      = NULL,
*LayoutBase     = NULL,
*LowLevelBase   = NULL,
*SpaceBase      = NULL,
*SpeedBarBase   = NULL,
*StringBase     = NULL,
*WindowBase     = NULL;

#ifdef __amigaos4__
EXPORT struct BitMapIFace*         IBitMap        = NULL;
EXPORT struct IntegerIFace*        IInteger       = NULL;
EXPORT struct LabelIFace*          ILabel         = NULL;
EXPORT struct LayoutIFace*         ILayout        = NULL;
EXPORT struct GadToolsIFace*       IGadTools      = NULL;
EXPORT struct SpaceIFace*          ISpace         = NULL;
EXPORT struct SpeedBarIFace*       ISpeedBar      = NULL;
EXPORT struct StringIFace*         IString        = NULL;
EXPORT struct WindowIFace*         IWindow        = NULL;

EXPORT struct ApplicationIFace*    IApplication   = NULL;
EXPORT struct AslIFace*            IAsl           = NULL;
EXPORT struct GraphicsIFace*       IGraphics      = NULL;
EXPORT struct IconIFace*           IIcon          = NULL;
EXPORT struct IntuitionIFace*      IIntuition     = NULL;
EXPORT struct LowLevelIFace*       ILowLevel      = NULL;
EXPORT struct TimerIFace*          ITimer         = NULL;
EXPORT struct WorkbenchIFace*      IWorkbench     = NULL;
#else
EXPORT struct Library*             AmigaGuideBase = NULL;
#endif

EXPORT struct EasyStruct EasyStruct =
{   sizeof(struct EasyStruct),
  0,
  NULL,
  NULL,
  NULL
};

MODULE char                  gamedir[MAX_PATH + 1],
screenname[MAXPUBSCREENNAME + 1] = "";
MODULE int                   bpl,
lpl,
rotation           = MIKIE_NO_ROTATE,
scaling            = 1,
winwidth,
winheight,
wpl;
MODULE FLAG                  ahi_opened         = FALSE,
autofire_a         = FALSE,
autofire_b         = FALSE,
gotpen[PENS],
guideexists        = FALSE,
#ifndef __amigaos4__
morphos            = FALSE,
#endif
pending            = FALSE,
BigSpeedBarNodes   = FALSE,
SmallSpeedBarNodes = FALSE;
MODULE UWORD                 wordpens[PENS];
MODULE UWORD*                wordbuffer;
MODULE ULONG                 longpens[PENS];
MODULE ULONG*                longbuffer;
MODULE ULONG                 AppSignal,
AppLibSignal       = 0,
bytepens[PENS],
keymask            = 0,
MainSignal,
mFrameCount        = 0,
showpointer        = TRUE,
statusbar          = TRUE,
tiptag1            = TAG_IGNORE,
tiptag2            = TAG_IGNORE,
titlebar           = TRUE,
toolbar            = TRUE;
MODULE TEXT                  fn_emu[MAX_PATH + 1];
MODULE BPTR                  ProgLock           = ZERO;
MODULE struct List           BigSpeedBarList,
SmallSpeedBarList;
MODULE struct RastPort       wpa8rastport;
MODULE struct timerequest*   TimerIO            = NULL;
MODULE struct timeval        waittill;
MODULE struct Gadget*        gadgets[GIDS + 1];
MODULE struct Image*         images[IMAGES];
MODULE struct Menu*          MenuPtr;
MODULE struct Process*       ProcessPtr         = NULL;
MODULE struct Screen*        ScreenPtr          = NULL;
MODULE struct Window*        MainWindowPtr      = NULL;
MODULE struct FileRequester* gamereq;
MODULE struct BitMap*        bitmap;
MODULE struct VisualInfo*    VisualInfoPtr      = NULL;
MODULE struct DiskObject*    IconifiedIcon;
MODULE struct MsgPort       *AppPort            = NULL,
*TimerPort          = NULL;
MODULE struct Node          *BigSpeedBarNodePtr[BIGGADGETS],
*SmallSpeedBarNodePtr[SMALLGADGETS];
MODULE Object*               WinObject          = NULL; // note that WindowObject is a reserved macro
MODULE BYTE                  TimerDevice        = ~0;
MODULE struct WBStartup*     WBStartupPtr;
#ifdef __amigaos4__
MODULE struct MsgPort*   AppLibPort         = NULL;
MODULE ULONG             AppID              = 0; // not NULL!
#endif
// MODULE struct TextAttr*   FontAttrPtr        = NULL;
MODULE struct
{   ULONG red,
  green,
  blue;
} rgbtab[256];

#define MN_PROJECT    0
#define MN_VIEW       1
#define MN_SETTINGS   2
#define MN_HELP       3

// Project menu
#define IN_RESET      0
// ---                1
#define IN_OPEN       2
// ---                3
#define IN_SAVEAS     4
// ---                5
#define IN_ICONIFY    6
#define IN_QUIT       7

// View menu
#define IN_POINTER    0
#define IN_STATUSBAR  1
#define IN_TITLEBAR   2
#define IN_TOOLBAR    3

// Settings menu
#define IN_AUTOFIREA  0
#define IN_AUTOFIREB  1
// ---                2
#define IN_OVERCLOCK  3
#define IN_TURBO      4
// ---                5
#define IN_SOUND      6
// ---                7
#define IN_1XSIZE     8
#define IN_2XSIZE     9
#define IN_3XSIZE    10
#define IN_4XSIZE    11
#define IN_5XSIZE    12
#define IN_6XSIZE    13
// ---               14
#define IN_ROTATE_L  15
#define IN_ROTATE_C  16
#define IN_ROTATE_R  17

// Help menu
#define IN_MANUAL     0
// ---
#define IN_ABOUT      2

#define AddHLayout    LAYOUT_AddChild, HLayoutObject
#define AddVLayout    LAYOUT_AddChild, VLayoutObject
#define AddLabel(x)   LAYOUT_AddImage, LabelObject, LABEL_Text, x, LabelEnd

MODULE struct NewMenu NewMenu[] =
{   { NM_TITLE, "Project"                            ,  0 , 0,                    0     , 0},
  {  NM_ITEM, "Reset (F5)"                         , "N", 0,                    0     , 0},
  {  NM_ITEM, NM_BARLABEL                          ,  0 , 0,                    0     , 0},
  {  NM_ITEM, "Open..."                            , "O", 0,                    0     , 0},
  {  NM_ITEM, NM_BARLABEL                          ,  0 , 0,                    0     , 0},
  {  NM_ITEM, "Save as..."                         , "S", 0,                    0     , 0},
  {  NM_ITEM, NM_BARLABEL                          ,  0 , 0,                    0     , 0},
  {  NM_ITEM, "Iconify"                            , "I", 0,                    0     , 0},
  {  NM_ITEM, "Quit Handy (Esc)"                   , "Q", 0,                    0     , 0},
  { NM_TITLE, "View"                               ,  0 , 0,                    0     , 0},
  {  NM_ITEM, "Pointer?"                           , "X", CHECKIT,              0     , 0},
  {  NM_ITEM, "Status bar?"                        , "D", CHECKIT,              0     , 0},
  {  NM_ITEM, "Titlebar?"                          , "E", CHECKIT,              0     , 0},
  {  NM_ITEM, "Toolbar?"                           , "H", CHECKIT,              0     , 0},
  { NM_TITLE, "Settings"                           ,  0 , 0,                    0     , 0},
  {  NM_ITEM, "Autofire for button A?"             , "A", CHECKIT,              0     , 0},
  {  NM_ITEM, "Autofire for button B?"             , "B", CHECKIT,              0     , 0},
  {  NM_ITEM, NM_BARLABEL                          ,  0 , 0,                    0     , 0},
  {  NM_ITEM, "Overclocked?"                       , "V", CHECKIT,              0     , 0},
  {  NM_ITEM, "Turbo?"                             , "T", CHECKIT,              0     , 0},
  {  NM_ITEM, NM_BARLABEL                          ,  0 , 0,                    0     , 0},
  {  NM_ITEM, "Sound?"                             , "U", CHECKIT,              0     , 0},
  {  NM_ITEM, NM_BARLABEL                          ,  0 , 0,                    0     , 0},
  {  NM_ITEM, "1× size"                            , "1", CHECKIT | MENUTOGGLE, 0x3BE0, 0}, // %11,1-11,1110,-0-0,0-00
  {  NM_ITEM, "2× size"                            , "2", CHECKIT | MENUTOGGLE, 0x3BD0, 0}, // %11,1-11,1101,-0-0,0-00
  {  NM_ITEM, "3× size"                            , "3", CHECKIT | MENUTOGGLE, 0x3BB0, 0}, // %11,1-11,1011,-0-0,0-00
  {  NM_ITEM, "4× size"                            , "4", CHECKIT | MENUTOGGLE, 0x3B70, 0}, // %11,1-11,0111,-0-0,0-00
  {  NM_ITEM, "5× size"                            , "5", CHECKIT | MENUTOGGLE, 0x3AF0, 0}, // %11,1-10,1111,-0-0,0-00
  {  NM_ITEM, "6× size"                            , "6", CHECKIT | MENUTOGGLE, 0x39F0, 0}, // %11,1-01,1111,-0-0,0-00
  {  NM_ITEM, NM_BARLABEL                          ,  0 , 0,                    0     , 0},
  {  NM_ITEM, "Rotate screen left"                 , "L", CHECKIT | MENUTOGGLE, 0x33F0, 0}, // %11,0-11,1111,-0-0,0-00
  {  NM_ITEM, "Centred"                            , "C", CHECKIT | MENUTOGGLE, 0x2BF0, 0}, // %10,1-11,1111,-0-0,0-00
  {  NM_ITEM, "Rotate screen right"                , "R", CHECKIT | MENUTOGGLE, 0x1BF0, 0}, // %01,1-11,1111,-0-0,0-00
  { NM_TITLE, "Help"                               ,  0 , 0,                    0     , 0},
  {  NM_ITEM, "Manual..."                          , "M", NM_ITEMDISABLED,      0     , 0},
  {  NM_ITEM, NM_BARLABEL                          ,  0 , 0,                    0     , 0},
  {  NM_ITEM, "About Handy..."                     , "?", 0,                    0     , 0},
  {   NM_END, NULL                                 ,  0 , 0,                    0     , 0}
};

MODULE struct HintInfo hintinfo[] = {
  { GID_SB1,  1,                     "Reset (F5)"                       , 0}, // 0
  { GID_SB1,  2,                     "Open (Amiga-O)"                   , 0}, // 1
  { GID_SB1,  3,                     "Save as (Amiga-S)"                , 0}, // 2
  { GID_SB2,  1,                     "Autofire for button A? (Amiga-A)" , 0}, // 3
  { GID_SB2,  2,                     "Autofire for button B? (Amiga-B)" , 0}, // 4
  { GID_SB2,  3,                     "Turbo? (Amiga-T)"                 , 0}, // 5
  { GID_SB2,  4,                     "Sound? (Amiga-U)"                 , 0}, // 6
  {      -1, -1,                     NULL                               , 0}  // terminator
};

MODULE UWORD CHIP InvisiblePointerData[6] =
{   0x0000, 0x0000, // reserved
  
  0x0000, 0x0000, // 1st row 1st plane, 1st row 2nd plane
  
  0x0000, 0x0000  // reserved
};

// struct StackSwapStruct thestack;

// MODULE int do_timer(void);
MODULE FLAG Exists(STRPTR name);
MODULE void handlemenus(UWORD code);
MODULE void project_reset(void);
MODULE void project_open(FLAG revert);
MODULE void project_saveas(void);
MODULE void updatepointer(void);
MODULE void updatemenus(void);
MODULE void help_manual(void);
MODULE void help_about(void);
MODULE void cleanexit(SBYTE rc);
MODULE int open_audio(void);
MODULE void close_audio(void);
MODULE void get_amiga_key(void);
MODULE int get_joypad(void);
MODULE void InitHook(struct Hook* hook, ULONG (*func)(), void* data);
MODULE void iconify(void);
MODULE void uniconify(void);
MODULE void openwindow(void);
MODULE void closewindow(void);
MODULE void getpens(void);
MODULE void freepens(void);
MODULE void load_images(int first, int last);
MODULE void sb_clearlist(struct List* ListPtr);
MODULE void updatesmlgads(void);
MODULE void setbutton(int which, FLAG enabled, FLAG state);
MODULE void lockscreen(void);
MODULE void unlockscreen(void);
#ifdef __amigaos4__
MODULE void blanker_on(void);
MODULE void blanker_off(void);
#endif

MODULE ULONG MainHookFunc(struct Hook* h, VOID* o, VOID* msg);

EXPORT void refresh(void)
{   int    i = 0,
  x, y;
  
  switch (scaling)
  {
    case 1:
      for (y = 0; y < winheight; y++)
      {   i = y * ROUNDTOLONG(winwidth);
        for (x = 0; x < winwidth; x++)
        {   *(bytebuffer +  x          +   (y           * bpl)) = bytepens[*(vbuffer + i++)];
        }   }
      acase 2:
      for (y = 0; y < winheight; y++)
      {   i = y * ROUNDTOLONG(winwidth);
        for (x = 0; x < winwidth; x++)
        {   *(wordbuffer +  x          + (((y * 2)    ) * wpl)) =
          *(wordbuffer +  x          + (((y * 2) + 1) * wpl)) = wordpens[*(vbuffer + i++)];
        }   }
      acase 3:
      for (y = 0; y < winheight; y++)
      {   i = y * ROUNDTOLONG(winwidth);
        for (x = 0; x < winwidth; x++)
        {   *(bytebuffer + (x * 3) +     (((y * 3)    ) * bpl)) =
          *(bytebuffer + (x * 3) + 1 + (((y * 3)    ) * bpl)) =
          *(bytebuffer + (x * 3) + 2 + (((y * 3)    ) * bpl)) =
          *(bytebuffer + (x * 3) +     (((y * 3) + 1) * bpl)) =
          *(bytebuffer + (x * 3) + 1 + (((y * 3) + 1) * bpl)) =
          *(bytebuffer + (x * 3) + 2 + (((y * 3) + 1) * bpl)) =
          *(bytebuffer + (x * 3) +     (((y * 3) + 2) * bpl)) =
          *(bytebuffer + (x * 3) + 1 + (((y * 3) + 2) * bpl)) =
          *(bytebuffer + (x * 3) + 2 + (((y * 3) + 2) * bpl)) = bytepens[*(vbuffer + i++)];
        }   }
      acase 4:
      for (y = 0; y < winheight; y++)
      {   i = y * ROUNDTOLONG(winwidth);
        for (x = 0; x < winwidth; x++)
        {   *(longbuffer +  x          + (((y * 4)    ) * lpl)) =
          *(longbuffer +  x          + (((y * 4) + 1) * lpl)) =
          *(longbuffer +  x          + (((y * 4) + 2) * lpl)) =
          *(longbuffer +  x          + (((y * 4) + 3) * lpl)) = longpens[*(vbuffer + i++)];
        }   }
      acase 5:
      for (y = 0; y < winheight; y++)
      {   i = y * ROUNDTOLONG(winwidth);
        for (x = 0; x < winwidth; x++)
        {   *(bytebuffer + (x * 5) +     (((y * 5)    ) * bpl)) =
          *(bytebuffer + (x * 5) + 1 + (((y * 5)    ) * bpl)) =
          *(bytebuffer + (x * 5) + 2 + (((y * 5)    ) * bpl)) =
          *(bytebuffer + (x * 5) + 3 + (((y * 5)    ) * bpl)) =
          *(bytebuffer + (x * 5) + 4 + (((y * 5)    ) * bpl)) =
          *(bytebuffer + (x * 5) +     (((y * 5) + 1) * bpl)) =
          *(bytebuffer + (x * 5) + 1 + (((y * 5) + 1) * bpl)) =
          *(bytebuffer + (x * 5) + 2 + (((y * 5) + 1) * bpl)) =
          *(bytebuffer + (x * 5) + 3 + (((y * 5) + 1) * bpl)) =
          *(bytebuffer + (x * 5) + 4 + (((y * 5) + 1) * bpl)) =
          *(bytebuffer + (x * 5) +     (((y * 5) + 2) * bpl)) =
          *(bytebuffer + (x * 5) + 1 + (((y * 5) + 2) * bpl)) =
          *(bytebuffer + (x * 5) + 2 + (((y * 5) + 2) * bpl)) =
          *(bytebuffer + (x * 5) + 3 + (((y * 5) + 2) * bpl)) =
          *(bytebuffer + (x * 5) + 4 + (((y * 5) + 2) * bpl)) =
          *(bytebuffer + (x * 5) +     (((y * 5) + 3) * bpl)) =
          *(bytebuffer + (x * 5) + 1 + (((y * 5) + 3) * bpl)) =
          *(bytebuffer + (x * 5) + 2 + (((y * 5) + 3) * bpl)) =
          *(bytebuffer + (x * 5) + 3 + (((y * 5) + 3) * bpl)) =
          *(bytebuffer + (x * 5) + 4 + (((y * 5) + 3) * bpl)) =
          *(bytebuffer + (x * 5) +     (((y * 5) + 4) * bpl)) =
          *(bytebuffer + (x * 5) + 1 + (((y * 5) + 4) * bpl)) =
          *(bytebuffer + (x * 5) + 2 + (((y * 5) + 4) * bpl)) =
          *(bytebuffer + (x * 5) + 3 + (((y * 5) + 4) * bpl)) =
          *(bytebuffer + (x * 5) + 4 + (((y * 5) + 4) * bpl)) = bytepens[*(vbuffer + i++)];
        }   }
      acase 6:
      for (y = 0; y < winheight; y++)
      {   i = y * ROUNDTOLONG(winwidth);
        for (x = 0; x < winwidth; x++)
        {   *(wordbuffer + (x * 3)     + (((y * 6)    ) * wpl)) =
          *(wordbuffer + (x * 3)     + (((y * 6) + 1) * wpl)) =
          *(wordbuffer + (x * 3)     + (((y * 6) + 2) * wpl)) =
          *(wordbuffer + (x * 3)     + (((y * 6) + 3) * wpl)) =
          *(wordbuffer + (x * 3)     + (((y * 6) + 4) * wpl)) =
          *(wordbuffer + (x * 3)     + (((y * 6) + 5) * wpl)) =
          *(wordbuffer + (x * 3) + 1 + (((y * 6)    ) * wpl)) =
          *(wordbuffer + (x * 3) + 1 + (((y * 6) + 1) * wpl)) =
          *(wordbuffer + (x * 3) + 1 + (((y * 6) + 2) * wpl)) =
          *(wordbuffer + (x * 3) + 1 + (((y * 6) + 3) * wpl)) =
          *(wordbuffer + (x * 3) + 1 + (((y * 6) + 4) * wpl)) =
          *(wordbuffer + (x * 3) + 1 + (((y * 6) + 5) * wpl)) =
          *(wordbuffer + (x * 3) + 2 + (((y * 6)    ) * wpl)) =
          *(wordbuffer + (x * 3) + 2 + (((y * 6) + 1) * wpl)) =
          *(wordbuffer + (x * 3) + 2 + (((y * 6) + 2) * wpl)) =
          *(wordbuffer + (x * 3) + 2 + (((y * 6) + 3) * wpl)) =
          *(wordbuffer + (x * 3) + 2 + (((y * 6) + 4) * wpl)) =
          *(wordbuffer + (x * 3) + 2 + (((y * 6) + 5) * wpl)) = wordpens[*(vbuffer + i++)];
        }   }   }
  
  WritePixelArray8
  (   MainWindowPtr->RPort,
   gadgets[GID_SP1]->LeftEdge,
   gadgets[GID_SP1]->TopEdge,
   gadgets[GID_SP1]->LeftEdge + (winwidth  * scaling) - 1,
   gadgets[GID_SP1]->TopEdge  + (winheight * scaling) - 1,
   bytebuffer,
   &wpa8rastport
   );
}

MODULE void get_amiga_key(void)
{   UWORD code;
  ULONG result,
  temp;
#ifdef __amigaos4__
  ULONG                  msgtype,
  msgval;
  struct ApplicationMsg* AppLibMsg;
#endif
  
  while ((result = DoMethod(WinObject, WM_HANDLEINPUT, &code)) != WMHI_LASTMSG)
  {   switch (result & WMHI_CLASSMASK)
  {
    case WMHI_MENUPICK:
      handlemenus(code);
      acase WMHI_CLOSEWINDOW:
      cleanexit(EXIT_SUCCESS);
      acase WMHI_ICONIFY:
      iconify();
      acase WMHI_GADGETUP:
      switch (result & WMHI_GADGETMASK)
      {
        case GID_SB1:
          // assert(toolbar);
          // code is the position within the list, starting from 1
          switch (code - 1)
          {
            case 0: // reset
              project_reset();
              acase 1: // open
              project_open(FALSE);
              acase 2: // save as
              project_saveas();
          }
          acase GID_SB2:
          // assert(toolbar);
          // code is the position within the list, starting from 1
          switch (code - 1)
          {
              // input buttons
            case GADPOS_AUTOFIREA:
              GetSpeedButtonNodeAttrs
              (   SmallSpeedBarNodePtr[GADPOS_AUTOFIREA],
               SBNA_Selected, (ULONG*) &temp,
               TAG_DONE);
              autofire_a = (FLAG) temp;
              updatemenus();
              acase GADPOS_AUTOFIREB:
              GetSpeedButtonNodeAttrs
              (   SmallSpeedBarNodePtr[GADPOS_AUTOFIREB],
               SBNA_Selected, (ULONG*) &temp,
               TAG_DONE);
              autofire_b = (FLAG) temp;
              updatemenus();
              // speed buttons
              acase GADPOS_TURBO:
              GetSpeedButtonNodeAttrs
              (   SmallSpeedBarNodePtr[GADPOS_TURBO],
               SBNA_Selected, (ULONG*) &temp,
               TAG_DONE);
              turbo = (FLAG) temp;
              settitle();
              // sound buttons
              acase GADPOS_SOUND:
              GetSpeedButtonNodeAttrs
              (   SmallSpeedBarNodePtr[GADPOS_SOUND],
               SBNA_Selected, (ULONG*) &temp,
               TAG_DONE);
              use_audio = (FLAG) temp;
              if (use_audio)
              {   open_audio();
              } else
              {   close_audio();
              }
              updatemenus();
          }   }   }   }
  
#ifdef __amigaos4__
  if (AppLibPort)
  {   while ((AppLibMsg = (struct ApplicationMsg*) GetMsg(AppLibPort)))
  {   msgtype = AppLibMsg->type;
    msgval  = (ULONG) ((struct ApplicationOpenPrintDocMsg*) AppLibMsg)->fileName;
    ReplyMsg((struct Message *) AppLibMsg);
    
    switch (msgtype)
    {
      case APPLIBMT_Quit:
      case APPLIBMT_ForceQuit:
        cleanexit(EXIT_SUCCESS);
        acase APPLIBMT_Hide:
        iconify();
        acase APPLIBMT_ToFront:
        unlockscreen();
        lockscreen();
        ScreenToFront(ScreenPtr);
        WindowToFront(MainWindowPtr);
        ActivateWindow(MainWindowPtr);
    }   }   }
#endif
}

MODULE void updatepointer(void)
{   if (!MainWindowPtr)
{   return; // important!
}
  
  if (!showpointer)
  {   SetPointer(MainWindowPtr, InvisiblePointerData, 1, 1, 0, 0);
  } else
  {   ClearPointer(MainWindowPtr);
  }   }

MODULE void handlemenus(UWORD code)
{   struct MenuItem* ItemPtr;
  
  if (!showpointer)
  {   SetPointer(MainWindowPtr, InvisiblePointerData, 1, 1, 0, 0); // turn off again
  }
  
  while (code != MENUNULL)
  {   ItemPtr = ItemAddress(MenuPtr, code);
    switch (MENUNUM(code))
    {
      case MN_PROJECT:
        switch (ITEMNUM(code))
        {
          case IN_RESET:
            project_reset();
            acase IN_OPEN:
            project_open(FALSE);
            acase IN_SAVEAS:
            project_saveas();
            acase IN_ICONIFY:
            iconify();
            acase IN_QUIT:
            cleanexit(EXIT_SUCCESS);
        }
        acase MN_VIEW:
        switch (ITEMNUM(code))
        {
          case IN_POINTER:
            showpointer = showpointer ? FALSE : TRUE;
            updatepointer();
            updatemenus();
            acase IN_STATUSBAR:
            statusbar = statusbar ? FALSE : TRUE;
            pending = TRUE;
            acase IN_TITLEBAR:
            titlebar = titlebar ? FALSE : TRUE;
            pending = TRUE;
            acase IN_TOOLBAR:
            toolbar = toolbar ? FALSE : TRUE;
            pending = TRUE;
        }
        acase MN_SETTINGS:
        switch (ITEMNUM(code))
        {
          case IN_AUTOFIREA:
            autofire_a = autofire_a ? FALSE : TRUE;
            updatemenus();
            acase IN_AUTOFIREB:
            autofire_b = autofire_b ? FALSE : TRUE;
            updatemenus();
            acase IN_OVERCLOCK:
            overclock  = overclock  ? FALSE : TRUE;
            settitle();
            acase IN_TURBO:
            turbo      = turbo      ? FALSE : TRUE;
            settitle();
            acase IN_SOUND:
            use_audio  = use_audio  ?     0 :    1;
            if (use_audio)
            {   open_audio();
            } else
            {   close_audio();
            }
            updatemenus();
            acase IN_1XSIZE:
            if (scaling != 1)
            {   scaling = 1;
              pending = TRUE;
            }
            acase IN_2XSIZE:
            if (scaling != 2)
            {   scaling = 2;
              pending = TRUE;
            }
            acase IN_3XSIZE:
            if (scaling != 3)
            {   scaling = 3;
              pending = TRUE;
            }
            acase IN_4XSIZE:
            if (scaling != 4)
            {   scaling = 4;
              pending = TRUE;
            }
            acase IN_5XSIZE:
            if (scaling != 5)
            {   scaling = 5;
              pending = TRUE;
            }
            acase IN_6XSIZE:
            if (scaling != 6)
            {   scaling = 6;
              pending = TRUE;
            }
            acase IN_ROTATE_L:
            if (rotation != MIKIE_ROTATE_R)
            {   rotation = MIKIE_ROTATE_R;
              pending = TRUE;
            }
            acase IN_ROTATE_C:
            if (rotation != MIKIE_NO_ROTATE)
            {   rotation = MIKIE_NO_ROTATE;
              pending = TRUE;
            }
            acase IN_ROTATE_R:
            if (rotation != MIKIE_ROTATE_L)
            {   rotation = MIKIE_ROTATE_L;
              pending = TRUE;
            }   }
        acase MN_HELP:
        switch (ITEMNUM(code))
        {
          case IN_MANUAL:
            help_manual();
            acase IN_ABOUT:
            help_about();
        }   }
    code = ItemPtr->NextSelect;
  }   }

MODULE int get_joypad(void) // gets joystick too
{   if (LowLevelBase)
{   return ReadJoyPort(1);
}
  
  return -1;
}

EXPORT BOOL aslfile(int kind)
{   /* kind of 0 = load LNX/LSS2
     1 = save LSS2 */
  
  switch (kind)
  {
    case 0:
      gamereq = ((struct FileRequester*)
                 AllocAslRequestTags(
                                     ASL_FileRequest,
                                     ASLFR_TitleText,      "Load Cartridge/State",
                                     ASLFR_InitialDrawer,  gamedir,
                                     ASLFR_InitialPattern, "#?.(LNX|LSS)",
                                     ASLFR_DoPatterns,     TRUE,
                                     ASLFR_RejectIcons,    TRUE,
                                     TAG_DONE));
      acase 1:
      gamereq = ((struct FileRequester*)
                 AllocAslRequestTags(
                                     ASL_FileRequest,
                                     ASLFR_TitleText,      "Save State",
                                     ASLFR_InitialDrawer,  gamedir,
                                     ASLFR_InitialPattern, "#?.LSS",
                                     ASLFR_DoPatterns,     TRUE,
                                     ASLFR_DoSaveMode,     TRUE,
                                     ASLFR_RejectIcons,    TRUE,
                                     TAG_DONE));
  }
  
  if
    (   !AslRequest(gamereq, NULL) // safe even if gamereq is NULL
     || gamereq->fr_File[0] == EOS
     )
  {   FreeAslRequest(gamereq);
    return FALSE;
  }
  strcpy(gamedir, gamereq->fr_Drawer);
  strcpy(gamefile, gamereq->fr_Drawer);
  AddPart(gamefile, gamereq->fr_File, MAX_PATH);
  FreeAslRequest(gamereq);
  return TRUE;
}

EXPORT void flipulong(ULONG* theaddress)
{   ULONG oldvalue,
  newvalue;
  
  oldvalue = *(theaddress);
  newvalue = (((oldvalue & 0xFF000000) >> 24) // -> 0x000000FF
              + ((oldvalue & 0x00FF0000) >>  8) // -> 0x0000FF00
              + ((oldvalue & 0x0000FF00) <<  8) // -> 0x00FF0000
              + ((oldvalue & 0x000000FF) << 24) // -> 0xFF000000
              );
  
  *(theaddress) = newvalue;
}

EXPORT void flipuword(UWORD* theaddress)
{   UWORD oldvalue,
  newvalue;
  
  oldvalue = *(theaddress);
  newvalue = (((oldvalue & 0xFF00) >> 8) // -> 0x00FF
              + ((oldvalue & 0x00FF) << 8) // -> 0xFF00
              );
  
  *(theaddress) = newvalue;
}

EXPORT unsigned int afwrite(const void* ptr, unsigned int size, unsigned int n, FILE* f, UBYTE flip)
{   int          i, j;
  ULONG        dest2;
  unsigned int retval;
  
  /* We must take care not to alter the passed argument! */
  
  dest2 = (ULONG) malloc(size * n); // should check success of this
  memcpy((void*) dest2, ptr, size * n);
  
  if (flip == 2)
  {   j = 0;
    for (i = 0; i < (int) n; i++)
    {   flipuword((UWORD*) (dest2 + j));
      j += 2;
    }   }
  else if (flip == 4)
  {   j = 0;
    for (i = 0; i < (int) n; i++)
    {   flipulong((ULONG*) (dest2 + j));
      j += 4;
    }   }
  
  retval = fwrite((const void*) dest2, size, n, f);
  free((void*) dest2);
  return retval;
}

EXPORT void settitle(void)
{   PERSIST TEXT titlebartext[80 + 1];
  
  if (!MainWindowPtr)
  {   return;
  }
  
  strcpy((char*) titlebartext, "Handy " VERSION);
  if (turbo)
  {   strcat((char*) titlebartext, " (turbo)");
  } elif (overclock)
  {   strcat((char*) titlebartext, " (overclocked)");
  }
  
  if (titlebar)
  {   SetWindowTitles(MainWindowPtr, (char*) titlebartext, (const char*) -1);
  }
  updatemenus();
  
  if (statusbar)
  {   if (turbo)
  {   DISCARD SetGadgetAttrs
    (   gadgets[GID_ST1], MainWindowPtr, NULL,
     STRINGA_TextVal, "Turbo",
     TAG_DONE);
  } elif (overclock)
    {   DISCARD SetGadgetAttrs
      (   gadgets[GID_ST1], MainWindowPtr, NULL,
       STRINGA_TextVal, "90",
       TAG_DONE);
    } else
    {   DISCARD SetGadgetAttrs
      (   gadgets[GID_ST1], MainWindowPtr, NULL,
       STRINGA_TextVal, "60",
       TAG_DONE);
    }
    RefreshGList((struct Gadget *) gadgets[GID_ST1], MainWindowPtr, NULL, 1); // is this needed?
  }   }

MODULE int open_audio(void)
{   if (ahi_open(SOUND_SAMPLERATE, AUDIO_M8S, SAMPLES, 8) == 0)
{   ahi_opened = gAudioEnabled = TRUE;
  ActivateWindow(MainWindowPtr);
  return 1; // success
} // implied else
  {   say((STRPTR) "Can't open AHI!");
    use_audio = 0;
    return 0; // failure
  }   }

EXPORT void check_timer(void)
{   static ULONG count=0;
  static ULONG fcount=0;
  static ULONG last_cycle_count=0;
  static ULONG last_frame_count=0;
  
  static volatile ULONG mEmulationSpeed  = 0;
  static volatile ULONG mFramesPerSecond = 0;
  
  static float          factor, fpsfloat, micros;
  static struct timeval diff,
  newtime,
  oldtime = {0, 0}; // will give incorrect results on the very first occasion
  
  // Calculate emulation speed
  count++;
  fcount++;
  
  if (count==HANDY_TIMER_FREQ/2)
  {   count=0;
    if (last_cycle_count>gSystemCycleCount)
    {   mEmulationSpeed=0;
    } else
    {   // Add .5% correction factor for round down error
      mEmulationSpeed=(gSystemCycleCount-last_cycle_count)*100;
      mEmulationSpeed+=(gSystemCycleCount-last_cycle_count)/2;
      mEmulationSpeed/=HANDY_SYSTEM_FREQ/2;
    }
    last_cycle_count=gSystemCycleCount;
  }
  
  if (fcount==HANDY_TIMER_FREQ)
  {   fcount=0;
    if (last_frame_count>mFrameCount)
    {   mFramesPerSecond=0;
    } else
    {   mFramesPerSecond=(mFrameCount-last_frame_count);
    }
    last_frame_count=mFrameCount;
    
    GetSysTime((struct TimeVal*) &newtime);
    diff = newtime;
    SubTime((struct TimeVal*) &diff, (struct TimeVal*) &oldtime);
    
    micros = (diff.tv_secs * 1000000) + diff.tv_micro;
    factor = 1000000 / micros;
    fpsfloat = mFramesPerSecond;
    fpsfloat *= factor;
    fpsfloat += 0.5; // to compensate for rounding down
    mFramesPerSecond = (ULONG) fpsfloat;
    oldtime = newtime;
    
    if (statusbar)
    {   DISCARD SetGadgetAttrs
      (   gadgets[GID_IN1], MainWindowPtr, NULL,
       INTEGER_Number, mFramesPerSecond,
       TAG_DONE);
      RefreshGList((struct Gadget *) gadgets[GID_IN1], MainWindowPtr, NULL, 1);
    }   }
  
  // Increment system counter
  gTimerCount++;
}

EXPORT void set_timer(void)
{   PERSIST const struct timeval delaytime_16mhz = { 0, 1000000 / HANDY_TIMER_FREQ },
  delaytime_24mhz = { 0,  666666 / HANDY_TIMER_FREQ };
  
  GetSysTime((struct TimeVal*) &waittill);
  if (overclock)
  {   AddTime((struct TimeVal*) &waittill, (struct TimeVal*) &delaytime_24mhz);
  } elif (!turbo)
  {   AddTime((struct TimeVal*) &waittill, (struct TimeVal*) &delaytime_16mhz);
  }   }

EXPORT void do_wait(void)
{   struct timeval newtime;
  
  do
  {   GetSysTime((struct TimeVal*) &newtime);
  } while (CmpTime((struct TimeVal*) &waittill, (struct TimeVal*) &newtime) == -1);
}

EXPORT int main(int argc, char *argv[])
{   int                i;
  struct DiskObject* DiskObject;
  STRPTR*            ToolArray;
  STRPTR             s;
  LONG               OldDir = -1,
  opts[OPTS];
  struct WBArg*      WBArgPtr;
  struct RDArgs*     rdargs = NULL;
  SLONG              number;
  ULONG              stacksize;
  
  for (i = 0; i < PENS; i++)
  {   gotpen[i] = FALSE;
    
    // Set an 3:3:2 palette
    rgbtab[i].red   = ((i & 0xe0)     ) << 24;
    rgbtab[i].green = ((i & 0x1c) << 3) << 24;
    rgbtab[i].blue  = ((i & 0x03) << 6) << 24;
  }
  for (i = 0; i < IMAGES; i++)
  {   images[i] = NULL;
  }
  for (i = 0; i < BIGGADGETS; i++)
  {   BigSpeedBarNodePtr[i] = NULL;
  }
  for (i = 0; i < SMALLGADGETS; i++)
  {   SmallSpeedBarNodePtr[i] = NULL;
  }
  NewList(&BigSpeedBarList);
  NewList(&SmallSpeedBarList);
  
#ifndef __amigaos4__
  if (SysBase->LibNode.lib_Version < 44)
  {   Write(Output(), (void*) "Handy: Need OS3.5+!\n", strlen("Handy: Need OS3.5+!\n"));
    cleanexit(EXIT_FAILURE);
  }
#endif
  
  ProcessPtr = (struct Process*) FindTask(NULL);
#ifdef __amigaos4__
  stacksize = ProcessPtr->pr_StackSize;
#else
  stacksize = *((ULONG*) ProcessPtr->pr_ReturnAddr);
#endif
  if (stacksize < 99996)
  {   printf("Stack size must be at least 100K!\n");
    cleanexit(EXIT_FAILURE);
  }
  
  generate_crctable();
  
#ifndef __amigaos4__
  if (FindResident("MorphOS"))
  {   morphos = TRUE;
    AmigaGuideBase = OpenLibrary("amigaguide.library",          0);
    // we don't call GetInterface(AmigaGuideBase) because we only use this library under MOS
  }
#endif
  
  if ( (LowLevelBase    = OpenLibrary("lowlevel.library",         0)))
  {
#ifdef __amigaos4__
    if (!(ILowLevel   = (struct LowLevelIFace*) GetInterface(LowLevelBase, "main", 1, 0)))
    {   CloseLibrary(LowLevelBase);
      LowLevelBase = NULL;
      say((STRPTR) "Can't get lowlevel.library interface!");
    }
#else
    ;
#endif
  } else
  {   say((STRPTR) "Can't open lowlevel.library!");
  }
  
  if (!(IconBase        = OpenLibrary("icon.library",             0)))
  {   rq((STRPTR) "Need icon.library!");
  }
  if (!(BitMapBase      = OpenLibrary("images/bitmap.image",      0)))
  {   rq((STRPTR) "Need bitmap.image!");
  }
  if (!(IntegerBase     = OpenLibrary("gadgets/integer.gadget",   0))) // presumably any version is OK
  {   rq((STRPTR) "Need string.gadget!");
  }
  if (!(LabelBase       = OpenLibrary("images/label.image",       0)))
  {   rq((STRPTR) "Need label.image!");
  }
  if (!(LayoutBase      = OpenLibrary("gadgets/layout.gadget",    0)))
  {   rq((STRPTR) "Need layout.gadget!");
  }
  if (!(SpaceBase       = OpenLibrary("gadgets/space.gadget",     0)))
  {   rq((STRPTR) "Need space.gadget!");
  }
  if (!(SpeedBarBase    = OpenLibrary("gadgets/speedbar.gadget", 41)))
  {   rq((STRPTR) "Need speedbar.gadget V41+!");
  }
  if (!(StringBase      = OpenLibrary("gadgets/string.gadget",    0))) // presumably any version is OK
  {   rq((STRPTR) "Need string.gadget!");
  }
  if (!(WindowBase      = OpenLibrary("window.class",             0)))
  {   rq((STRPTR) "Need window.class!"); // window.class requires layout.gadget!
  }
  
  if
    (    WindowBase->lib_Version >  45
     || (WindowBase->lib_Version == 45 && WindowBase->lib_Revision >= 11)
     )
  {   tiptag1 = WINDOW_HintInfo;
    tiptag2 = WINDOW_GadgetHelp;
  }
  
  TimerPort = (struct MsgPort*) CreateMsgPort();
  TimerIO = (struct timerequest*) CreateIORequest(TimerPort, sizeof(struct timerequest));
  TimerDevice = OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest*) TimerIO, 0);
  TimerBase = TimerIO->tr_node.io_Device;
  
#ifdef __amigaos4__
  if
    (   !(IBitMap         = (struct BitMapIFace*)         GetInterface(BitMapBase,         "main", 1, 0))
     || !(IGadTools       = (struct GadToolsIFace*)       GetInterface(GadToolsBase,       "main", 1, 0))
     || !(IInteger        = (struct IntegerIFace*)        GetInterface(IntegerBase,        "main", 1, 0))
     || !(ILabel          = (struct LabelIFace*)          GetInterface(LabelBase,          "main", 1, 0))
     || !(ILayout         = (struct LayoutIFace*)         GetInterface(LayoutBase,         "main", 1, 0))
     || !(ISpace          = (struct SpaceIFace*)          GetInterface(SpaceBase,          "main", 1, 0))
     || !(ISpeedBar       = (struct SpeedBarIFace*)       GetInterface(SpeedBarBase,       "main", 1, 0))
     || !(IString         = (struct StringIFace*)         GetInterface(StringBase,         "main", 1, 0))
     || !(IWindow         = (struct WindowIFace*)         GetInterface(WindowBase,         "main", 1, 0))
     
     || !(IAsl            = (struct AslIFace*)            GetInterface(                  AslBase,         "main", 1, 0))
     || !(IGraphics       = (struct GraphicsIFace*)       GetInterface((struct Library*) GfxBase,         "main", 1, 0))
     || !(IIcon           = (struct IconIFace*)           GetInterface(                  IconBase,        "main", 1, 0))
     || !(IIntuition      = (struct IntuitionIFace*)      GetInterface((struct Library*) IntuitionBase,   "main", 1, 0))
     || !(ITimer          = (struct TimerIFace*)          GetInterface((struct Library*) TimerBase,       "main", 1, 0))
     || !(IWorkbench      = (struct WorkbenchIFace*)      GetInterface(                  WorkbenchBase,   "main", 1, 0))
     )
  {   DISCARD printf("Can't get library interfaces!\n");
    cleanexit(EXIT_FAILURE);
  }
  
  if
    (   (ApplicationBase = OpenLibrary("application.library", 0))
     && (IApplication    = (struct ApplicationIFace*)
         GetInterface(ApplicationBase, "application", 2, 0))
     )
  {   AppID = RegisterApplication
    (   "Handy",
     REGAPP_Description,       "Atari Lynx emulator",
     REGAPP_URLIdentifier,     "amigan.1emu.net",
     REGAPP_AllowsBlanker,     FALSE,
     REGAPP_HasIconifyFeature, TRUE,
     REGAPP_CanCreateNewDocs,  FALSE,
     REGAPP_CanPrintDocs,      FALSE,
     TAG_DONE);
    GetApplicationAttrs(AppID, APPATTR_Port, &AppLibPort, TAG_END);
    AppLibSignal = 1 << AppLibPort->mp_SigBit;
    DISCARD SetApplicationAttrs
    (   AppID,
     APPATTR_NeedsGameMode,    TRUE,
     TAG_DONE);
  }
#endif /* __amigaos4__ */
  
  if (!(AppPort = CreateMsgPort()))
  {   rq((char*) "Can't create message port!");
  }
  
  strcpy((char*) fn_emu, "PROGDIR:");
  if (argc) // started from CLI
  {   AddPart((char*) fn_emu, FilePart(argv[0]), MAX_PATH);
    
    memset((char*) opts, 0, sizeof(opts));
    if ((rdargs = ReadArgs(TEMPLATE, opts, NULL)))
    {   /* Set prefs according to arguments */
      
      if (opts[OPT_FILE])        strcpy(gamefile, (char*) opts[OPT_FILE]);
      if (opts[OPT_NOSOUND])     use_audio   = 0;
      if (opts[OPT_TURBO])       turbo       = TRUE;
      if (opts[OPT_OVERCLOCK])   overclock   = TRUE;
      if (opts[OPT_NOPOINTER])   showpointer = FALSE;
      if (opts[OPT_NOSTATUSBAR]) statusbar   = FALSE;
      if (opts[OPT_NOTITLEBAR])  titlebar    = FALSE;
      if (opts[OPT_NOTOOLBAR])   toolbar     = FALSE;
      if (opts[OPT_SIZE])
      {   number = (SLONG) (*((SLONG *) opts[OPT_SIZE]));
        if (number < 1 || number > 6)
        {   DISCARD printf("%s: <size> must be 1-6! Ignored.\n", argv[0]);
        } else
        {   scaling = (int) number;
        }   }
      if (opts[OPT_PUBSCREEN])   strcpy(screenname, (char*) opts[OPT_PUBSCREEN]);
      
      FreeArgs((struct RDArgs*) rdargs);
    } else
    {   printf("\n%s\n\nUsage: %s <options>\n\n", &ver[6], argv[0]);
      
      printf("FILE         : game to play.\n");
      printf("NOSOUND/S    : turn AHI sound off (def: on).\n");
      printf("TURBO/S      : turbo speed on (def: off).\n");
      printf("OVERCLOCK/S  : 24MHz master oscillator (def: 16MHz).\n\n");
      printf("NOPOINTER/S  : don't show mouse pointer (def: on).\n\n");
      printf("NOSTATUSBAR/S: don't show status bar (def: on).\n\n");
      printf("NOTITLEBAR/S : don't show titlebar (def: on).\n\n");
      printf("NOTOOLBAR/S  : don't show toolbar (def: on).\n\n");
      printf("SIZE/N       : graphics size 1-6 (def: 1).\n\n");
      printf("PUBSCREEN/K  : public screen name.\n\n");
      
      cleanexit(EXIT_SUCCESS);
    }   }
  else // started from WB
  {   WBStartupPtr = (struct WBStartup*) argv;
    WBArgPtr     = WBStartupPtr->sm_ArgList;
    
    AddPart((char*) fn_emu, (char*) WBArgPtr->wa_Name, MAX_PATH);
    
    if ((WBArgPtr->wa_Lock) && (*WBArgPtr->wa_Name))
    {   OldDir = CurrentDir(WBArgPtr->wa_Lock);
    }
    
    if
      (   (*WBArgPtr->wa_Name)
       && (DiskObject = GetDiskObject((char*) WBArgPtr->wa_Name))
       )
    {   ToolArray = (STRPTR*) DiskObject->do_ToolTypes;
      
      if ((s = (char*) FindToolType(ToolArray, "FILE"      ))) strcpy(gamefile, (char*) s);
      if (             FindToolType(ToolArray, "NOSOUND"    )) use_audio   = 0;
      if (             FindToolType(ToolArray, "TURBO"      )) turbo       = TRUE;
      if (             FindToolType(ToolArray, "OVERCLOCK"  )) overclock   = TRUE;
      if (             FindToolType(ToolArray, "NOPOINTER"  )) showpointer = FALSE;
      if (             FindToolType(ToolArray, "NOSTATUSBAR")) statusbar   = FALSE;
      if (             FindToolType(ToolArray, "NOTITLEBAR" )) titlebar    = FALSE;
      if (             FindToolType(ToolArray, "NOTOOLBAR"  )) toolbar     = FALSE;
      if ((s = (char*) FindToolType(ToolArray, "SIZE"      )))
      {   scaling = atoi(s);
        if (scaling < 1 || scaling > 6)
        {   scaling = 1;
        }   }
      if ((s = (char*) FindToolType(ToolArray, "PUBSCREEN"))) strcpy(screenname, (char*) s);
      
      FreeDiskObject(DiskObject);
    }
    
    if (OldDir != -1)
    {   CurrentDir(OldDir);
    }   }
  
  if (!screenname[0])
  {   GetDefaultPubScreen(screenname);
  }
  lockscreen();
  
  ProgLock = GetProgramDir(); // never unlock this!
  guideexists = Exists((STRPTR) "PROGDIR:Handy.guide");
  
  if (gamefile[0])
  {   strcpy(gamedir, gamefile);
    *(PathPart(gamedir)) = EOS;
  } else
  {   strcpy(gamedir, "PROGDIR:ROMs");
    // aslfile(0);
  }
  
  load_images(0, IMAGES - 1);
  for (i = 0; i < BIGGADGETS; i++)
  {   if
    ((  BigSpeedBarNodePtr[i] = (struct Node *)
      AllocSpeedButtonNode((UWORD) (i + 1), // speed button ID
                           SBNA_Image,     images[i             ],
                           SBNA_SelImage,  images[i + BIGGADGETS],
                           SBNA_Enabled,   TRUE,
                           SBNA_Spacing,   2,
                           SBNA_Highlight, SBH_IMAGE,
                           TAG_DONE)))
  {   AddTail(&BigSpeedBarList, BigSpeedBarNodePtr[i]);
    BigSpeedBarNodes = TRUE;
  } else
  {   rq((char*) "Can't allocate speedbar images (out of memory?)!");
  }   }
  for (i = 0; i < SMALLGADGETS; i++)
  {   if
    ((  SmallSpeedBarNodePtr[i] = (struct Node *)
      AllocSpeedButtonNode((UWORD) (i + 1), // speed button ID
                           SBNA_Image,     images[(BIGGADGETS * 2) + i               ],
                           SBNA_SelImage,  images[(BIGGADGETS * 2) + i + SMALLGADGETS],
                           SBNA_Enabled,   TRUE,
                           SBNA_Spacing,   2,
                           SBNA_Highlight, SBH_IMAGE,
                           SBNA_Toggle,    TRUE,
                           TAG_DONE)))
  {   AddTail(&SmallSpeedBarList, SmallSpeedBarNodePtr[i]);
    SmallSpeedBarNodes = TRUE;
  } else
  {   rq((char*) "Can't allocate speedbar images (out of memory?)!");
  }   }
  
  try
  {   Lynx = new CSystem(gamefile, (STRPTR) DEFAULT_BOOTIMAGE);
  }
  catch (...)
  {   cleanexit(EXIT_FAILURE);
  }
  
  nextstop = gSystemCycleCount;
  switch (Lynx->mCart->CartGetRotate())
  {
    case 0:
      rotation = MIKIE_NO_ROTATE; // 1
      acase 1:
      rotation = MIKIE_ROTATE_L;  // 2
      acase 2:
      rotation = MIKIE_ROTATE_R;  // 3
  }
  switch (rotation)
  {
    case MIKIE_NO_ROTATE:
      winwidth  = 160;
      winheight = 102;
      acase MIKIE_ROTATE_L:
    case MIKIE_ROTATE_R:
      winwidth  = 102;
      winheight = 160;
  }
  bpl = ROUNDTOLONG(winwidth * scaling);
  wpl = bpl / 2;
  lpl = bpl / 4;
  
  SetJoyPortAttrs(1, SJA_Type, SJA_TYPE_AUTOSENSE, TAG_END);
  
  if (use_audio)
  {   open_audio();
  }
  
  openwindow();
  
  Lynx->DisplaySetAttributes(rotation, MIKIE_PIXEL_FORMAT_8BPP     , ROUNDTOLONG(winwidth));
  
  for (;;)
  {   set_timer();
    nextstop += HANDY_SYSTEM_FREQ / HANDY_TIMER_FREQ; // ie. 800,000 cycles (1/20th second)
    
    do
    {   for (i = 1024; i; i--)
    {   Lynx->Update();
    }
    } while (gSystemCycleCount < nextstop);
    
    do_wait();
    get_amiga_key();
    check_timer();
    
    if (pending)
    {   pending = FALSE;
      closewindow();
      openwindow();
    }   }   }

MODULE void cleanexit(SBYTE rc)
{   int             i;
  struct Message* MsgPtr;
  
  if (ahi_opened)
  {   // assert(use_audio);
    close_audio();
  }
  
  closewindow();
  
  if (BigSpeedBarNodes)
  {   sb_clearlist(&BigSpeedBarList);
    BigSpeedBarNodes = FALSE;
  }
  if (SmallSpeedBarNodes)
  {   sb_clearlist(&SmallSpeedBarList);
    SmallSpeedBarNodes = FALSE;
  }
  
  // Dispose the images ourselves as button.gadget doesn't
  // do this for its GA_Image (although LAYOUT_AddImage normally
  // does, but we're using LAYOUT_NoDispose)...
  for (i = 0; i < IMAGES; i++)
  {   if (images[i])
  {   DisposeObject((Object*) images[i]);
    images[i] = NULL;
  }   }
  
  if (AppPort)
  {   while ((MsgPtr = GetMsg(AppPort)))
  {   ReplyMsg(MsgPtr);
  }
    DeleteMsgPort(AppPort);
    AppPort = NULL;
  }
  
  unlockscreen();
  
#ifdef __amigaos4__
  if (IApplication)
  {   if (AppID)
  {   // assert(ApplicationBase);
    // assert(IApplication);
    UnregisterApplication(AppID, TAG_DONE);
    // AppID = 0;
  }
    DropInterface((struct Interface*) IApplication);
  }
  if (ApplicationBase) CloseLibrary(ApplicationBase); // ApplicationBase = NULL;
#endif
  
  if (!TimerDevice)
  {   CloseDevice((struct IORequest*) TimerIO);
    TimerDevice = ~0;
    TimerBase = NULL;
  }
  if (TimerIO)
  {   DeleteIORequest((struct IORequest*) TimerIO);
    TimerIO = NULL;
  }
  if (TimerPort)
  {   DeleteMsgPort(TimerPort);
    TimerPort = NULL;
  }
  
#ifdef __amigaos4__
  if (IBitMap)         DropInterface((struct Interface*) IBitMap);
  if (IInteger)        DropInterface((struct Interface*) IInteger);
  if (ILabel)          DropInterface((struct Interface*) ILabel);
  if (ILayout)         DropInterface((struct Interface*) ILayout);
  if (ISpace)          DropInterface((struct Interface*) ISpace);
  if (ISpeedBar)       DropInterface((struct Interface*) ISpeedBar);
  if (IString)         DropInterface((struct Interface*) IString);
  if (IWindow)         DropInterface((struct Interface*) IWindow);
  
  if (IAsl)            DropInterface((struct Interface*) IAsl);
  if (IGraphics)       DropInterface((struct Interface*) IGraphics);
  if (IIcon)           DropInterface((struct Interface*) IIcon);
  if (IIntuition)      DropInterface((struct Interface*) IIntuition);
  if (ILowLevel)       DropInterface((struct Interface*) ILowLevel);
  if (ITimer)          DropInterface((struct Interface*) ITimer);
  if (IWorkbench)      DropInterface((struct Interface*) IWorkbench);
#else
  if (AmigaGuideBase) { CloseLibrary(AmigaGuideBase); AmigaGuideBase = NULL; }
#endif
  
  if (      IconBase) { CloseLibrary(      IconBase);       IconBase = NULL; }
  if (  LowLevelBase) { CloseLibrary(  LowLevelBase);   LowLevelBase = NULL; }
  if (    BitMapBase) { CloseLibrary(    BitMapBase);     BitMapBase = NULL; }
  if (   IntegerBase) { CloseLibrary(   IntegerBase);    IntegerBase = NULL; }
  if (     LabelBase) { CloseLibrary(     LabelBase);      LabelBase = NULL; }
  if (     SpaceBase) { CloseLibrary(     SpaceBase);      SpaceBase = NULL; }
  if (  SpeedBarBase) { CloseLibrary(  SpeedBarBase);   SpeedBarBase = NULL; }
  if (    StringBase) { CloseLibrary(    StringBase);     StringBase = NULL; }
  if (    WindowBase) { CloseLibrary(    WindowBase);     WindowBase = NULL; }
  if (    LayoutBase) { CloseLibrary(    LayoutBase);     LayoutBase = NULL; }
  
  if (Lynx)
  {   delete Lynx;
    Lynx = NULL;
  }
  
  exit(rc); // this can cause guru $80000003 when running the OS3 build under OS4
}

#ifndef __amigaos4__
EXPORT void wbmain(struct WBStartup* wb)
{   // printf() has no effect when started from Workbench,
  // due to StormC not creating an output console for us (unlike
  // SAS/C).
  
  main(0, (char**) wb);
}
#endif

/* MODULE int do_timer(void)
 {   if (gSystemCycleCount>gThrottleNextCycleCheckpoint)
 {   int overrun=gSystemCycleCount-gThrottleNextCycleCheckpoint;
 int nextstep=(((HANDY_SYSTEM_FREQ/HANDY_TIMER_FREQ)*gThrottleMaxPercentage)/100);
 
 // We've gone thru the checkpoint, so therefore the
 // we must have reached the next timer tick, if the
 // timer hasnt ticked then we've got here early. If
 // so then put the system to sleep by saying there
 // is no more idle work to be done in the idle loop
 
 if (gThrottleLastTimerCount==gTimerCount)
 {   // All we know is that we got here earlier than expected as the
 // counter has not yet rolled over
 return 0;
 }
 
 //Set the next control point
 gThrottleNextCycleCheckpoint+=nextstep;
 
 // Set next timer checkpoint
 gThrottleLastTimerCount=gTimerCount;
 
 // Check if we've overstepped the speed limit
 if (overrun>nextstep)
 {   // We've exceeded the next timepoint, going way too
 // fast (sprite drawing) so reschedule.
 return 0;
 }   }
 
 return 1;
 } */

EXPORT void check_keyboard_status(void)
{   TRANSIENT int   portstate;
  PERSIST   ULONG joymask = 0,
  thedata,
  frames  = 0;
  
  portstate = get_joypad();
  if (portstate != -1)
  {   if (portstate & JPF_JOY_RIGHT    ) joymask |= BUTTON_RIGHT; else joymask &= ~BUTTON_RIGHT;
    if (portstate & JPF_JOY_LEFT     ) joymask |= BUTTON_LEFT;  else joymask &= ~BUTTON_LEFT;
    if (portstate & JPF_JOY_DOWN     ) joymask |= BUTTON_DOWN;  else joymask &= ~BUTTON_DOWN;
    if (portstate & JPF_JOY_UP       ) joymask |= BUTTON_UP;    else joymask &= ~BUTTON_UP;
    if (portstate & JPF_BUTTON_RED   ) joymask |= BUTTON_A;     else joymask &= ~BUTTON_A;
    if (portstate & JPF_BUTTON_BLUE  ) joymask |= BUTTON_B;     else joymask &= ~BUTTON_B;
    if (portstate & JPF_BUTTON_GREEN ) joymask |= BUTTON_OPT1;  else joymask &= ~BUTTON_OPT1;
    if (portstate & JPF_BUTTON_YELLOW) joymask |= BUTTON_OPT2;  else joymask &= ~BUTTON_OPT2;
    if (portstate & JPF_BUTTON_PLAY  ) joymask |= BUTTON_PAUSE; else joymask &= ~BUTTON_PAUSE;
  }
  
  frames++;
  if (autofire_a)
  {   if (frames & 2)
  {   joymask |= BUTTON_A;
  } else
  {   joymask &= ~BUTTON_A;
  }   }
  if (autofire_b)
  {   if (frames & 2)
  {   joymask |= BUTTON_B;
  } else
  {   joymask &= ~BUTTON_B;
  }   }
  
  thedata = keymask | joymask;
  Lynx->SetButtonData(thedata);
}

int curpos = 0;
signed char audiobuf[AUDIO_BUFFER];
ULONG p = 0;

EXPORT UBYTE* displaycallback(void)
{   if (use_audio)
{   while (p != gAudioBufferPointer)
{   if (curpos >= AUDIO_BUFFER)
{   ahi_play_samples(audiobuf, SAMPLES, NOTIME, DOWAIT);
  curpos = 0;
}
  
  audiobuf[curpos] = gAudioBuffer[p]-128;
  curpos++;
  p++;
  
  if (p >= HANDY_AUDIO_BUFFER_SIZE)
  {   p = 0;
  }   }   }
  
  check_keyboard_status();
  refresh();
  mFrameCount++;
  
  return vbuffer;
}

EXPORT int lss_read(void* dest, int varsize, int varcount, LSS_FILE* fp, UBYTE flip)
{   int   i, j;
  ULONG copysize,
  dest2;
  
  copysize=varsize*varcount;
  if((fp->index + copysize) > fp->index_limit) copysize=fp->index_limit - fp->index;
  memcpy(dest,fp->memptr+fp->index,copysize);
  fp->index+=copysize;
  
  dest2 = (ULONG) dest;
  
  if (flip == 2)
  {   j = 0;
    for (i = 0; i < varcount; i++)
    {   flipuword((UWORD*) (dest2 + j));
      j += 2;
    }   }
  elif (flip == 4)
  {   j = 0;
    for (i = 0; i < varcount; i++)
    {   flipulong((ULONG*) (dest2 + j));
      j += 4;
    }   }
  
  return copysize;
}

MODULE void project_reset(void)
{   Lynx->Reset();
}

MODULE void project_open(FLAG revert)
{   if (!revert && !aslfile(0))
{   return;
}
  
  Lynx->DisplaySetAttributes(rotation, MIKIE_PIXEL_FORMAT_16BPP_565, ROUNDTOLONG(winwidth));
  
  if (Lynx)
  {   delete Lynx;
    Lynx = NULL;
  }
  
  try
  {   Lynx = new CSystem(gamefile, (STRPTR) DEFAULT_BOOTIMAGE);
  }
  catch (...)
  {   try
    {   Lynx = new CSystem((char*) "", (STRPTR) DEFAULT_BOOTIMAGE);
    }
    catch (...)
    {   cleanexit(EXIT_FAILURE);
    }   }
  
  nextstop = gSystemCycleCount;
  
  Lynx->DisplaySetAttributes(rotation, MIKIE_PIXEL_FORMAT_8BPP     , ROUNDTOLONG(winwidth));
  
  switch (Lynx->mCart->CartGetRotate())
  {
    case 0:
      if (rotation != MIKIE_NO_ROTATE)
      {   rotation = MIKIE_NO_ROTATE;
        pending = TRUE;
      }
      acase 1:
      if (rotation != MIKIE_ROTATE_L)
      {   rotation = MIKIE_ROTATE_L;
        pending = TRUE;
      }
      acase 2:
      if (rotation != MIKIE_ROTATE_R)
      {   rotation = MIKIE_ROTATE_R;
        pending = TRUE;
      }   }
  
  updatemenus();
}

MODULE void project_saveas(void)
{   if (!Lynx || !aslfile(1))
{   return;
}
  if (strlen(gamefile) >= 5 && stricmp(&gamefile[strlen(gamefile) - 4], ".LSS"))
  {   strcat(gamefile, ".LSS");
  }
  Lynx->DisplaySetAttributes(rotation, MIKIE_PIXEL_FORMAT_16BPP_565, ROUNDTOLONG(winwidth));
  // Save the context
  if (!Lynx->ContextSave(gamefile, 0))
  {   say((STRPTR) "An error occured during saving!");
  }
  Lynx->DisplaySetAttributes(rotation, MIKIE_PIXEL_FORMAT_8BPP     , ROUNDTOLONG(winwidth));
}

MODULE void updatemenus(void)
{   if (!MainWindowPtr || !MenuPtr)
{   return;
}
  
  ClearMenuStrip(MainWindowPtr);
  
  if (showpointer)
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_VIEW,     IN_POINTER,   NOSUB))->Flags |= CHECKED;
  } else
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_VIEW,     IN_POINTER,   NOSUB))->Flags &= ~CHECKED;
  }
  if (statusbar)
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_VIEW,     IN_STATUSBAR, NOSUB))->Flags |= CHECKED;
  } else
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_VIEW,     IN_STATUSBAR, NOSUB))->Flags &= ~CHECKED;
  }
  if (titlebar)
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_VIEW,     IN_TITLEBAR,  NOSUB))->Flags |= CHECKED;
  } else
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_VIEW,     IN_TITLEBAR,  NOSUB))->Flags &= ~CHECKED;
  }
  if (toolbar)
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_VIEW,     IN_TOOLBAR,   NOSUB))->Flags |= CHECKED;
  } else
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_VIEW,     IN_TOOLBAR,   NOSUB))->Flags &= ~CHECKED;
  }
  
  if (autofire_a)
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_SETTINGS, IN_AUTOFIREA, NOSUB))->Flags |= CHECKED;
  } else
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_SETTINGS, IN_AUTOFIREA, NOSUB))->Flags &= ~CHECKED;
  }
  if (autofire_b)
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_SETTINGS, IN_AUTOFIREB, NOSUB))->Flags |= CHECKED;
  } else
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_SETTINGS, IN_AUTOFIREB, NOSUB))->Flags &= ~CHECKED;
  }
  if (overclock)
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_SETTINGS, IN_OVERCLOCK, NOSUB))->Flags |= CHECKED;
  } else
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_SETTINGS, IN_OVERCLOCK, NOSUB))->Flags &= ~CHECKED;
  }
  if (turbo)
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_SETTINGS, IN_TURBO,     NOSUB))->Flags |= CHECKED;
  } else
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_SETTINGS, IN_TURBO,     NOSUB))->Flags &= ~CHECKED;
  }
  if (use_audio)
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_SETTINGS, IN_SOUND,     NOSUB))->Flags |= CHECKED;
  } else
  {   ItemAddress(MenuPtr, FULLMENUNUM(MN_SETTINGS, IN_SOUND,     NOSUB))->Flags &= ~CHECKED;
  }
  switch (rotation)
  {
    case  MIKIE_ROTATE_R:
      ItemAddress(MenuPtr, (ULONG) FULLMENUNUM(MN_SETTINGS, IN_ROTATE_L, NOSUB))->Flags |= CHECKED;
      acase MIKIE_NO_ROTATE:
      ItemAddress(MenuPtr, (ULONG) FULLMENUNUM(MN_SETTINGS, IN_ROTATE_C, NOSUB))->Flags |= CHECKED;
      acase MIKIE_ROTATE_L:
      ItemAddress(MenuPtr, (ULONG) FULLMENUNUM(MN_SETTINGS, IN_ROTATE_R, NOSUB))->Flags |= CHECKED;
  }
  switch (scaling)
  {
    case  1:
      ItemAddress(MenuPtr, (ULONG) FULLMENUNUM(MN_SETTINGS, IN_1XSIZE, NOSUB))->Flags |= CHECKED;
      acase 2:
      ItemAddress(MenuPtr, (ULONG) FULLMENUNUM(MN_SETTINGS, IN_2XSIZE, NOSUB))->Flags |= CHECKED;
      acase 3:
      ItemAddress(MenuPtr, (ULONG) FULLMENUNUM(MN_SETTINGS, IN_3XSIZE, NOSUB))->Flags |= CHECKED;
      acase 4:
      ItemAddress(MenuPtr, (ULONG) FULLMENUNUM(MN_SETTINGS, IN_4XSIZE, NOSUB))->Flags |= CHECKED;
      acase 5:
      ItemAddress(MenuPtr, (ULONG) FULLMENUNUM(MN_SETTINGS, IN_5XSIZE, NOSUB))->Flags |= CHECKED;
      acase 6:
      ItemAddress(MenuPtr, (ULONG) FULLMENUNUM(MN_SETTINGS, IN_6XSIZE, NOSUB))->Flags |= CHECKED;
  }
  
  DISCARD ResetMenuStrip(MainWindowPtr, MenuPtr);
  
  if (guideexists)
  {   OnMenu( MainWindowPtr, FULLMENUNUM(MN_HELP,    IN_MANUAL, NOSUB));
  } else
  {   OffMenu(MainWindowPtr, FULLMENUNUM(MN_HELP,    IN_MANUAL, NOSUB));
  }
  
  updatesmlgads();
}

MODULE FLAG Exists(STRPTR name)
{   APTR oldwinptr;
  BPTR lock;
  
  oldwinptr = (APTR) (ProcessPtr->pr_WindowPtr);
  ProcessPtr->pr_WindowPtr = (APTR) -1;
  lock = Lock(name, MODE_OLDFILE);
  ProcessPtr->pr_WindowPtr = oldwinptr;
  
  if (lock)
  {   UnLock(lock);
    return TRUE;
  } else
  {   return FALSE;
  }   }

MODULE void help_manual(void)
{
#ifndef __amigaos4__
  struct NewAmigaGuide nag =
  {   ZERO,                           // nag_Lock
    (STRPTR) "PROGDIR:Handy.guide", // nag_Name
    NULL,                           // nag_Screen
    NULL,                           // nag_PubScreen
    NULL,                           // nag_HostPort
    NULL,                           // nag_ClientPort
    NULL,                           // nag_BaseName
    0,                              // nag_Flags
    NULL,                           // nag_Context
    (STRPTR) "MAIN",                // nag_Node
    0,                              // nag_Line
    NULL,                           // nag_Extens
    NULL                            // nag_Client
  };
#endif
  BPTR OldDir /* = ZERO */ ;
  
  if (!guideexists)
  {   return;
  }
  
#ifndef __amigaos4__
  if (morphos)
  {   /* MorphOS OpenWorkbenchObject() function is a non-functional
       dummy; therefore we use OpenAmigaGuide() when running under
       MorphOS. */
    
    if (!AmigaGuideBase)
    {   return;
    }
    
    nag.nag_Screen = ScreenPtr;
    DISCARD OpenAmigaGuide(&nag, TAG_DONE);
  } else
  {
#endif
    OldDir = CurrentDir(ProgLock);
    DISCARD OpenWorkbenchObject
    (   "Handy.guide",
     WBOPENA_ArgLock, ProgLock,
     WBOPENA_ArgName, "Handy.guide",
     TAG_DONE);
    DISCARD CurrentDir(OldDir);
#ifndef __amigaos4__
  }
#endif
}

MODULE void close_audio(void)
{   ahi_close();
  ahi_opened = gAudioEnabled = FALSE;
}

EXPORT void rq(STRPTR text)
{   EasyStruct.es_TextFormat   = (STRING) text;
  EasyStruct.es_Title        = (STRING) "Handy: Fatal Error";
  EasyStruct.es_GadgetFormat = (STRING) "Quit";
  DISCARD EasyRequest(MainWindowPtr, &EasyStruct, NULL); // OK even if MainWindowPtr is NULL
  
  cleanexit(EXIT_FAILURE);
}

EXPORT void say(STRPTR text)
{   EasyStruct.es_TextFormat   = (STRING) text;
  EasyStruct.es_Title        = (STRING) "Handy: Warning";
  EasyStruct.es_GadgetFormat = (STRING) "OK";
  DISCARD EasyRequest(MainWindowPtr, &EasyStruct, NULL); // OK even if MainWindowPtr is NULL
  
  if (MainWindowPtr)
  {   ActivateWindow(MainWindowPtr);
  }   }

MODULE ULONG MainHookFunc(struct Hook* h, VOID* o, VOID* msg)
{   /* "When the hook is called, the data argument points to the
     window object and message argument to the IntuiMessage." */
  
  ULONG theclass;
  UWORD code, qual;
  SWORD mousex, mousey;
  
#ifdef __SASC
  geta4(); // wait till here before doing anything
#endif
  
  theclass = ((struct IntuiMessage*) msg)->Class;
  code     = ((struct IntuiMessage*) msg)->Code;
  qual     = ((struct IntuiMessage*) msg)->Qualifier;
  mousex   = ((struct IntuiMessage*) msg)->MouseX;
  mousey   = ((struct IntuiMessage*) msg)->MouseY;
  
  switch (theclass)
  {
    case IDCMP_RAWKEY:
      if (code < 128)
      {   switch (code)
      {
        case 69: // Esc
          cleanexit(EXIT_SUCCESS);
          acase 16: // Q -> OPT1
        case 18:  // E -> OPT1
          keymask |= BUTTON_OPT1;
          acase 17: // W
          keymask |= BUTTON_OPT2;
          acase 32: // A -> A
        case 34:  // D -> A
          keymask |= BUTTON_A;
          acase 33: // S -> B
          keymask |= BUTTON_B;
          acase 76:
          switch (rotation)
          {
            case MIKIE_NO_ROTATE:
              keymask |= BUTTON_UP;
              acase MIKIE_ROTATE_L:
              keymask |= BUTTON_LEFT;
              acase MIKIE_ROTATE_R:
              keymask |= BUTTON_RIGHT;
          }
          acase 77:
          switch (rotation)
          {
            case MIKIE_NO_ROTATE:
              keymask |= BUTTON_DOWN;
              acase MIKIE_ROTATE_L:
              keymask |= BUTTON_RIGHT;
              acase MIKIE_ROTATE_R:
              keymask |= BUTTON_LEFT;
          }
          acase 78:
          switch (rotation)
          {
            case MIKIE_NO_ROTATE:
              keymask |= BUTTON_RIGHT;
              acase MIKIE_ROTATE_L:
              keymask |= BUTTON_UP;
              acase MIKIE_ROTATE_R:
              keymask |= BUTTON_DOWN;
          }
          acase 79:
          switch (rotation)
          {
            case MIKIE_NO_ROTATE:
              keymask |= BUTTON_LEFT;
              acase MIKIE_ROTATE_L:
              keymask |= BUTTON_DOWN;
              acase MIKIE_ROTATE_R:
              keymask |= BUTTON_UP;
          }
          acase 25: // P
        case  67: // ENTER
        case  68: // RETURN
          keymask |= BUTTON_PAUSE;
      }   }
      else
      {   code -= 128; // remove "key released" bit
        switch (code)
        {
          case 16: // Q -> OPT1
          case 18: // E -> OPT1
            keymask &= ~BUTTON_OPT1;
            acase 17: // W
            keymask &= ~BUTTON_OPT2;
            acase 32: // A -> A
          case 34:  // D -> A
            keymask &= ~BUTTON_A;
            acase 33: // S -> B
            keymask &= ~BUTTON_B;
            acase 76:
            switch (rotation)
            {
              case MIKIE_NO_ROTATE:
                keymask &= ~BUTTON_UP;
                acase MIKIE_ROTATE_L:
                keymask &= ~BUTTON_LEFT;
                acase MIKIE_ROTATE_R:
                keymask &= ~BUTTON_RIGHT;
            }
            acase 77:
            switch (rotation)
            {
              case MIKIE_NO_ROTATE:
                keymask &= ~BUTTON_DOWN;
                acase MIKIE_ROTATE_L:
                keymask &= ~BUTTON_RIGHT;
                acase MIKIE_ROTATE_R:
                keymask &= ~BUTTON_LEFT;
            }
            acase 78:
            switch (rotation)
            {
              case MIKIE_NO_ROTATE:
                keymask &= ~BUTTON_RIGHT;
                acase MIKIE_ROTATE_L:
                keymask &= ~BUTTON_UP;
                acase MIKIE_ROTATE_R:
                keymask &= ~BUTTON_DOWN;
            }
            acase 79:
            switch (rotation)
            {
              case MIKIE_NO_ROTATE:
                keymask &= ~BUTTON_LEFT;
                acase MIKIE_ROTATE_L:
                keymask &= ~BUTTON_DOWN;
                acase MIKIE_ROTATE_R:
                keymask &= ~BUTTON_UP;
            }
            acase 25: // P
          case  67: // ENTER
          case  68: // RETURN
            keymask &= ~BUTTON_PAUSE;
            acase 84: // F5: reset
            project_reset();
        }   }
      acase IDCMP_MENUVERIFY:
      if (!showpointer)
      {   ClearPointer(MainWindowPtr); // turn on temporarily
      }   }
  
  return 1;
}

MODULE void InitHook(struct Hook* hook, ULONG (*func)(), void* data)
{   // Make sure a pointer was passed
  
  if (hook)
  {   // Fill in the Hook fields
#ifdef __amigaos4__
    hook->h_Entry    = func;
#else
    hook->h_Entry    = (ULONG (*)()) HookEntry;
#endif
    hook->h_SubEntry = func;
    hook->h_Data     = data;
  } else
  {   rq((char*) "Can't initialize hook!");
  }   }

MODULE void iconify(void)
{   struct AppMessage*     AppMsg;
#ifdef __amigaos4__
  ULONG                  msgtype,
  msgval;
  struct ApplicationMsg* AppLibMsg;
#endif
  
  if (!MainWindowPtr)
  {   return;
  }
  
  DISCARD RA_Iconify(WinObject);
  // the WinObject must stay a valid pointer
  MainWindowPtr = NULL;
  freepens();
#ifdef __amigaos4__
  blanker_on();
  if (AppID)
  {   // assert(ApplicationBase);
    // assert(IApplication);
    SetApplicationAttrs
    (   AppID,
     APPATTR_Hidden, TRUE,
     TAG_DONE);
  }
#endif
  DISCARD GetAttr(WINDOW_SigMask, WinObject, &MainSignal);
  AppSignal = (1 << AppPort->mp_SigBit); // maybe unnecessary?
  
  do
  {   if ((Wait
           (   AppSignal
            | AppLibSignal
            | MainSignal
            | SIGBREAKF_CTRL_C
            )) & SIGBREAKF_CTRL_C)
  {   cleanexit(EXIT_SUCCESS);
  }
    
    while ((AppMsg = (struct AppMessage *) GetMsg(AppPort)))
    {   switch (AppMsg->am_Type)
    {
      case AMTYPE_APPICON:
        uniconify();
    }
      ReplyMsg((struct Message *) AppMsg);
    }
    
#ifdef __amigaos4__
    if (AppLibPort)
    {   while ((AppLibMsg = (struct ApplicationMsg*) GetMsg(AppLibPort)))
    {   msgtype = AppLibMsg->type;
      msgval  = (ULONG) ((struct ApplicationOpenPrintDocMsg*) AppLibMsg)->fileName;
      ReplyMsg((struct Message *) AppLibMsg);
      
      switch (msgtype)
      {
        case APPLIBMT_Quit:
        case APPLIBMT_ForceQuit:
          cleanexit(EXIT_SUCCESS);
          acase APPLIBMT_Unhide:
          uniconify();
          acase APPLIBMT_ToFront:
          uniconify();
          unlockscreen();
          lockscreen();
          ScreenToFront(ScreenPtr);
          // assert(MainWindowPtr);
          WindowToFront(MainWindowPtr);
          ActivateWindow(MainWindowPtr);
      }   }   }
#endif
    
  } while (!MainWindowPtr);
}
MODULE void uniconify(void)
{   // assert(!MainWindowPtr);
  
  getpens();
  
  if (!(MainWindowPtr = (struct Window *) RA_Uniconify(WinObject)))
  {   rq((char*) "Can't reopen window!");
  }
  
#ifdef __amigaos4__
  blanker_off();
  if (AppID)
  {   // assert(ApplicationBase);
    // assert(IApplication);
    SetApplicationAttrs
    (   AppID,
     APPATTR_Hidden, FALSE,
     TAG_DONE);
  }
#endif
  
  DISCARD GetAttr(WINDOW_SigMask, WinObject, &MainSignal);
  AppSignal = (1 << AppPort->mp_SigBit); // maybe unnecessary?
  
  ActivateWindow(MainWindowPtr);
}

MODULE void closewindow(void)
{   if (WinObject)
{   DisposeObject(WinObject);
  WinObject     = NULL;
  MainWindowPtr = NULL;
}
  freepens();
  if (bitmap)
  {   FreeBitMap(bitmap);
    bitmap = NULL;
  }
  if (vbuffer)
  {   free(vbuffer);
    vbuffer = NULL;
  }
  if (bytebuffer)
  {   free(bytebuffer);
    bytebuffer = NULL;
  }   }

MODULE void openwindow(void)
{   PERSIST struct Hook MainHookStruct; // doesn't work as TRANSIENT!
  
  switch (rotation)
  {
    case MIKIE_NO_ROTATE:
      winwidth  = 160;
      winheight = 102;
      acase MIKIE_ROTATE_L:
    case MIKIE_ROTATE_R:
      winwidth  = 102;
      winheight = 160;
  }
  
  bpl = ROUNDTOLONG(winwidth * scaling);
  wpl = bpl / 2;
  lpl = bpl / 4;
  
  InitHook(&MainHookStruct, (ULONG (*)()) MainHookFunc, NULL);
  
  if (!(VisualInfoPtr = (struct VisualInfo*) GetVisualInfo(ScreenPtr, TAG_DONE)))
  {   rq((STRPTR) "Can't get GadTools visual info!");
  }
  
  if (!(MenuPtr = (struct Menu*) CreateMenus(NewMenu, TAG_DONE)))
  {   rq((STRPTR) "Can't create menus!");
  }
  if (!(LayoutMenus(MenuPtr, VisualInfoPtr, GTMN_NewLookMenus, TRUE, TAG_DONE)))
  {   rq((STRPTR) "Can't lay out menus!");
  }
  
  IconifiedIcon = GetDiskObjectNew((char*) fn_emu);
  
  getpens();
  
  if (!(WinObject = (Object*)
        WindowObject,
        WA_PubScreen,                     ScreenPtr,
        WA_ScreenTitle,                   "Handy " VERSION,
        titlebar ? WA_Title : TAG_IGNORE, "Handy " VERSION,
        WA_Activate,                      TRUE,
        WA_DepthGadget,                   titlebar,
        WA_DragBar,                       titlebar,
        WA_CloseGadget,                   titlebar,
        WA_SizeGadget,                    FALSE,
        WA_IDCMP,                         IDCMP_RAWKEY | IDCMP_MENUVERIFY,
        WINDOW_IDCMPHook,                 &MainHookStruct,
        WINDOW_IDCMPHookBits,             IDCMP_RAWKEY | IDCMP_MENUVERIFY,
        WINDOW_MenuStrip,                 MenuPtr,
        WINDOW_Position,                  WPOS_CENTERSCREEN,
        WINDOW_IconifyGadget,             titlebar,
        titlebar                    ? WINDOW_IconTitle : TAG_IGNORE, "Handy",
        (titlebar && IconifiedIcon) ? WINDOW_Icon      : TAG_IGNORE, IconifiedIcon,
        WINDOW_AppPort,                   AppPort,
        tiptag1,                          &hintinfo,
        tiptag2,                          TRUE,
        WINDOW_ParentGroup,               gadgets[GID_LY1] = (struct Gadget*)
        VLayoutObject,
        toolbar ? LAYOUT_AddChild      : TAG_IGNORE,
        HLayoutObject,
        LAYOUT_SpaceInner,        TRUE,
        LAYOUT_SpaceOuter,        FALSE,
        LAYOUT_AddChild,          gadgets[GID_SB1] = (struct Gadget*)
        SpeedBarObject,
        GA_ID,                GID_SB1,
        GA_RelVerify,         TRUE,
        SPEEDBAR_Buttons,     &BigSpeedBarList,
        SPEEDBAR_BevelStyle,  BVS_NONE,
        SpeedBarEnd,
        CHILD_WeightedWidth,      100,
        LayoutEnd,
        toolbar ? CHILD_WeightedHeight : TAG_IGNORE, 0,
        toolbar ? LAYOUT_AddChild      : TAG_IGNORE,
        HLayoutObject,
        LAYOUT_SpaceInner,        TRUE,
        LAYOUT_SpaceOuter,        FALSE,
        LAYOUT_AddChild,          gadgets[GID_SB2] = (struct Gadget*)
        SpeedBarObject,
        GA_ID,                GID_SB2,
        GA_RelVerify,         TRUE,
        SPEEDBAR_Buttons,     &SmallSpeedBarList,
        SPEEDBAR_BevelStyle,  BVS_NONE,
        SpeedBarEnd,
        CHILD_WeightedWidth,      100,
        LayoutEnd,
        toolbar ? CHILD_WeightedHeight : TAG_IGNORE, 0,
        LAYOUT_AddChild,              gadgets[GID_SP1] = (struct Gadget*)
        SpaceObject,
        GA_ID,                    GID_SP1,
        SPACE_MinWidth,           winwidth  * scaling,
        SPACE_MinHeight,          winheight * scaling,
        SPACE_BevelStyle,         BVS_NONE,
        SpaceEnd,
        CHILD_WeightedWidth,          0,
        CHILD_WeightedHeight,         0,
        statusbar ? LAYOUT_AddChild    : TAG_IGNORE,
        HLayoutObject,
        LAYOUT_VertAlignment,     LALIGN_CENTER,
        AddLabel("FPS:"),
        LAYOUT_AddChild,          gadgets[GID_IN1] = (struct Gadget*)
        IntegerObject,
        GA_ID,                GID_IN1,
        GA_ReadOnly,          TRUE,
        INTEGER_Number,       0,
        INTEGER_Arrows,       FALSE,
        IntegerEnd,
        AddLabel("of"),
        LAYOUT_AddChild,          gadgets[GID_ST1] = (struct Gadget*)
        StringObject,
        GA_ID,                GID_ST1,
        GA_ReadOnly,          TRUE,
        STRINGA_TextVal,      "-",
        // STRINGA_MinVisible,   5 + 1, // looks bad on OS4
        STRINGA_Justification,GACT_STRINGRIGHT,
        StringEnd,
        LayoutEnd,
        statusbar ? CHILD_WeightedHeight : TAG_IGNORE, 0,
        LayoutEnd,
        WindowEnd))
  {   rq((STRPTR) "Can't create window!");
  }
  
  if (!(MainWindowPtr = (struct Window*) RA_OpenWindow(WinObject)))
  {   rq((STRPTR) "Can't open window!");
  }
  
  DISCARD GetAttr(WINDOW_SigMask, WinObject, &MainSignal);
  AppSignal = 1 << AppPort->mp_SigBit;
  
  updatemenus();
  settitle();
  updatepointer();
  
  InitRastPort(&wpa8rastport);
  if (!(bitmap = AllocBitMap
        (   bpl,
         winheight * scaling,
         GetBitMapAttr(MainWindowPtr->RPort->BitMap, BMA_DEPTH),
         BMF_DISPLAYABLE | BMF_MINPLANES,
         MainWindowPtr->RPort->BitMap
         )))
  {   rq((STRPTR) "Out of memory!");
  }
  wpa8rastport.BitMap = bitmap;
  
  vbuffer = (unsigned char*) malloc(ROUNDTOLONG(winwidth) * winheight);
  memset(vbuffer, 0, ROUNDTOLONG(winwidth) * winheight);
  
  bytebuffer = (unsigned char*) malloc(bpl * winheight * scaling);
  memset(bytebuffer, 0, bpl * winheight * scaling);
  wordbuffer = (UWORD*) bytebuffer;
  longbuffer = (ULONG*) bytebuffer;
  
  try
  {   Lynx->DisplaySetAttributes
    (   rotation,
     MIKIE_PIXEL_FORMAT_8BPP,
     ROUNDTOLONG(winwidth)
     );
  }
  catch (...)
  {   rq((STRPTR) "Can't set display attributes!");
  }   }

MODULE void getpens(void)
{   int  i;
  LONG result;
  
  for (i = 0; i < PENS; i++)
  {   // assert(!gotpen[i]);
    
    result = ObtainPen
    (   ScreenPtr->ViewPort.ColorMap,
     (ULONG) -1,
     rgbtab[i].red,
     rgbtab[i].green,
     rgbtab[i].blue,
     PEN_EXCLUSIVE | PEN_NO_SETCOLOR
     );
    if (result == -1)
    {   bytepens[i] = (UBYTE) FindColor
      (   ScreenPtr->ViewPort.ColorMap,
       rgbtab[i].red,
       rgbtab[i].green,
       rgbtab[i].blue,
       -1
       );
    } else
    {   bytepens[i] = (UBYTE) result;
      gotpen[i] = TRUE;
      SetRGB32
      (   &ScreenPtr->ViewPort,
       (ULONG) bytepens[i],
       (ULONG) rgbtab[i].red,
       (ULONG) rgbtab[i].green,
       (ULONG) rgbtab[i].blue
       );
    }
    
    wordpens[i] = (bytepens[i] <<  8) | bytepens[i];
    longpens[i] = (wordpens[i] << 16) | wordpens[i];
  }   }

MODULE void freepens(void)
{   int i;
  
  for (i = 0; i < PENS; i++)
  {   if (gotpen[i])
  {   ReleasePen(ScreenPtr->ViewPort.ColorMap, (ULONG) bytepens[i]);
    gotpen[i] = FALSE;
  }   }   }

#define OPAQUE      0
#define TRANSPARENT 1
MODULE void load_images(int first, int last)
{   TRANSIENT TEXT  errstring[80 + 1],
  imagefilename[MAX_PATH + 1];
  TRANSIENT int   i;
  PERSIST   struct
  {   const STRPTR name;
    const UBYTE  transparency;
    const FLAG   aiss;
  } imageinit[IMAGES] = {
    { (char*) "new",           TRANSPARENT, TRUE  }, //  0
    { (char*) "open",          TRANSPARENT, TRUE  },
    { (char*) "save",          TRANSPARENT, TRUE  },
    { (char*) "new_s",         TRANSPARENT, TRUE  },
    { (char*) "open_s",        TRANSPARENT, TRUE  },
    { (char*) "save_s",        TRANSPARENT, TRUE  }, //  5
    { (char*) "autofirea_off", OPAQUE     , FALSE },
    { (char*) "autofireb_off", OPAQUE     , FALSE },
    { (char*) "turbo_off",     OPAQUE     , FALSE },
    { (char*) "sound_off",     OPAQUE     , FALSE },
    { (char*) "autofirea_on",  OPAQUE     , FALSE }, // 10
    { (char*) "autofireb_on",  OPAQUE     , FALSE },
    { (char*) "turbo_on",      OPAQUE     , FALSE },
    { (char*) "sound_on",      OPAQUE     , FALSE }, // 13
  };
  
  for (i = first; i <= last; i++)
  {   if (!images[i])
  {
#ifdef TRACKENTRY
    zprintf("Loading %s...\n", imageinit[i].name); // Delay(TRACKDELAY);
#endif
    
    if (imageinit[i].aiss)
    {   DISCARD strcpy((char*) imagefilename, "PROGDIR:images/aiss/");
      DISCARD strcat((char*) imagefilename, imageinit[i].name);
      if (!(Exists((char*) imagefilename)))
      {   DISCARD strcpy((char*) imagefilename, "TBImages:");
        DISCARD strcat((char*) imagefilename, imageinit[i].name);
      }   }
    else
    {   strcpy((char*) imagefilename, "PROGDIR:images/");
      strcat((char*) imagefilename, imageinit[i].name);
    }
    
    if (!(images[i] = (struct Image*)
          BitMapObject,
          BITMAP_SourceFile,  imagefilename,
          BITMAP_Screen,      ScreenPtr,
          BITMAP_Masking,     (imageinit[i].transparency == TRANSPARENT) ? TRUE : FALSE,
          BITMAP_Transparent, (imageinit[i].transparency == TRANSPARENT) ? TRUE : FALSE,
          End))
    {   sprintf((char*) errstring, "Can't load %s!", imageinit[i].name);
      rq((char*) errstring);
    }   }   }   }

// Function to free an Exec List of ReAction SpeedBar nodes.
MODULE void sb_clearlist(struct List* ListPtr)
{   struct Node *NodePtr,
  *NextNodePtr;
  
  if (!SpeedBarBase)
  {   return;
  }
  
  NodePtr = ListPtr->lh_Head;
  while ((NextNodePtr = NodePtr->ln_Succ))
  {   FreeSpeedButtonNode(NodePtr);
    NodePtr = NextNodePtr;
  }
  NewList(ListPtr); // prepare for reuse
}

MODULE void help_about(void)
{
#ifdef __amigaos4__
  EasyStruct.es_TextFormat   = (STRING) "Handy V0.95 R2.4 for AmigaOS 4.0+\n" \
  "2 July 2017\n\n" \
  "An Atari Lynx emulator\n" \
  "Windows version by Keith Wilkins\n" \
  "AmigaOS 4.0+ port by James Jacobs & AmiDog\n\n" \
  "amigan.1emu.net/releases/\n" \
  "amigansoftware@gmail.com";
#else
  EasyStruct.es_TextFormat   = (STRING) "Handy V0.95 R2.4 for AmigaOS 3.5+\n" \
  "2 July 2017\n\n" \
  "An Atari Lynx emulator\n" \
  "Windows version by Keith Wilkins\n" \
  "AmigaOS 3.5+ port by James Jacobs & AmiDog\n\n" \
  "amigan.1emu.net/releases/\n" \
  "amigansoftware@gmail.com";
#endif
  EasyStruct.es_Title        = (STRING) "About Handy";
  EasyStruct.es_GadgetFormat = (STRING) "OK";
  
#ifdef __amigaos4__
  blanker_on();
#endif
  DISCARD EasyRequest(MainWindowPtr, &EasyStruct, NULL); // OK even if MainWindowPtr is NULL (which it won't be)
#ifdef __amigaos4__
  blanker_off();
#endif
  // assert(MainWindowPtr);
  ActivateWindow(MainWindowPtr);
}

MODULE void updatesmlgads(void)
{   if (!MainWindowPtr || !toolbar)
{   return; // important!
}
  
  DISCARD SetGadgetAttrs
  (   gadgets[GID_SB2], MainWindowPtr, NULL,
   SPEEDBAR_Buttons, (ULONG) ~0,
   TAG_DONE);
  
  setbutton(GADPOS_AUTOFIREA, TRUE, autofire_a);
  setbutton(GADPOS_AUTOFIREB, TRUE, autofire_b);
  setbutton(GADPOS_TURBO,     TRUE, turbo);
  setbutton(GADPOS_SOUND,     TRUE, use_audio);
  
  DISCARD SetGadgetAttrs
  (   gadgets[GID_SB2], MainWindowPtr, NULL,
   SPEEDBAR_Buttons, &SmallSpeedBarList,
   TAG_DONE);
  RefreshGList((struct Gadget *) gadgets[GID_SB2], MainWindowPtr, NULL, 1);
}

MODULE void setbutton(int which, FLAG enabled, FLAG state)
{   SetSpeedButtonNodeAttrs
  (   SmallSpeedBarNodePtr[which],
   SBNA_Selected, state   ? TRUE  : FALSE,
   SBNA_Disabled, enabled ? FALSE : TRUE,
   TAG_DONE);
}

MODULE void lockscreen(void)
{   // assert(!ScreenPtr);
  
  if (screenname[0])
  {   if (!(ScreenPtr = (struct Screen*) LockPubScreen((CONST_STRPTR) screenname)))
  {   rq((char*) "Can't lock specified public screen!");
  }   }
  else
  {   if (!(ScreenPtr = (struct Screen*) LockPubScreen((CONST_STRPTR) NULL)))
  {   rq((char*) "Can't lock default public screen!");
  }   }   }
MODULE void unlockscreen(void)
{   if (ScreenPtr)
{   if (screenname[0])
{   UnlockPubScreen(screenname, ScreenPtr);
  ScreenPtr = NULL;
} else
{   UnlockPubScreen(NULL, ScreenPtr);
  ScreenPtr = NULL;
}   }   }

#ifdef __amigaos4__
MODULE void blanker_on(void)
{   if (AppID)
{   // assert(ApplicationBase);
  // assert(IApplication);
  SetApplicationAttrs
  (   AppID,
   APPATTR_AllowsBlanker, TRUE,
   TAG_DONE);
}   }
MODULE void blanker_off(void)
{   if (AppID)
{   // assert(ApplicationBase);
  // assert(IApplication);
  SetApplicationAttrs
  (   AppID,
   APPATTR_AllowsBlanker, FALSE,
   TAG_DONE);
}   }
#endif
