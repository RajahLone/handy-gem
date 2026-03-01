# Handy

Attempt to port from Amiga version to Atari ST in GEM environment.

Handy 0.95 is an Atari Lynx emulator, written by Keith Wilkins [https://handy.sourceforge.net/]
and was released under the GPL on 14th April 2004.

work-in-progress, TODOs are:
- fix pixel format for INTEL and FALCON, currently OK for TC24 and TC32 screenmodes.
- fix timers or optimize (currently too slow).
- audio? (sound routines are currently disabled).

usage:
- lynxboot.img must be placed besides the program, drag'n'drop a lnx file or select it with the fileselector.
- only for high/true color video screenmodes.
- keys A for button A, B for button B, P for 'Pause' button, F1 for 'Options 1', F2 for 'Options 2'
- Ctrl+R to rotate screen.
- Ctrl+F or window fuller for x2 then x3 window zoom (available only if the VDI support vro_cpyfm scaling, such as in some NVDI drivers).
- Ctrl+Q or Ctrl+U to quit the application.

notes:
- verify if the handy.prg flags are sets to be loaded and use TTRAM, to gain some speed on FireBee and CT60.
