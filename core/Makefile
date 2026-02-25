#
# Makefile
#
CFLAGS = -MMD -Wall -Wno-switch -Wno-non-virtual-dtor -O3 -fomit-frame-pointer -ffast-math -g -DANSI_GCC -DSDL_PATCH -Wno-comment -Wno-parentheses -Wno-stringop-truncation -Wno-mismatched-new-delete -Wno-sequence-point -Wno-maybe-uninitialized -Wno-unused-but-set-variable

# list header files here
HEADER = c65c02.h c6502mak.h cart.h lynxbase.h lynxdef.h machine.h memmap.h mikie.h ram.h rom.h susie.h sysbase.h system.h amiga.h

# list C++ files here
#
COBJS = cart.cpp memmap.cpp mikie.cpp ram.cpp rom.cpp susie.cpp system.cpp amiga.cpp

SRCFILES = $(HEADER) $(COBJS) $(SOBJS)

#############################
CROSSPREFIX=/opt/cross-mint/bin/m68k-atari-mint-
PREFIX=/opt/cross-mint
PATH = $(PREFIX)/m68k-atari-mint/bin:$(PREFIX)/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
LD_LIBRARY_PATH=$(PREFIX)/lib:$(PREFIX)/m68k-atari-mint/lib:$LD_LIBRARY_PATH

CP = $(CROSSPREFIX)g++
AS = $(CC)
AR = $(CROSSPREFIX)ar
FLAGS = $(CROSSPREFIX)flags

OBJS = $(COBJS:.cpp=.o)

all: $(OBJS)

$(OBJS): $(COBJS) $(HEADER)
	$(CP) $(CFLAGS) -m68000 $(INCS) -c $*.cpp -o ../build/68000/$*.o
	$(CP) $(CFLAGS) -m68020-60 $(INCS) -c $*.cpp -o ../build/68020/$*.o
	$(CP) $(CFLAGS) -mcpu=5475 $(INCS) -c $*.cpp -o ../build/ColdFire/$*.o
