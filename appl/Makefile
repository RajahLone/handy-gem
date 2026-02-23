#
# Makefile
#
CFLAGS = -MMD -Wall

# list header files here
HEADER = main.h

# list C++ files here
#
COBJS = main.cpp

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
