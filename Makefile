#
# Makefile
#
CFLAGS   = -MMD -Wall -Wno-switch -Wno-non-virtual-dtor -O3 -fomit-frame-pointer -ffast-math -g -DMSB_FIRST -DANSI_GCC -DSDL_PATCH -Wno-comment -Wno-parentheses -Wno-stringop-truncation -Wno-mismatched-new-delete -Wno-sequence-point -Wno-maybe-uninitialized -Wno-unused-but-set-variable
LDFLAGS = -s
LDLIBS  = -lstdc++

TARGET = handy.prg

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

CC = $(CROSSPREFIX)gcc
CP = $(CROSSPREFIX)g++
AS = $(CC)
AR = $(CROSSPREFIX)ar
RANLIB = $(CROSSPREFIX)ranlib
STRIP = $(CROSSPREFIX)strip
FLAGS = $(CROSSPREFIX)flags
STACK = $(CROSSPREFIX)stack

OBJS = $(COBJS:.cpp=.o)

all: $(TARGET)
	$(STRIP) ./build/68000/$(TARGET)
	$(STACK) -S 128k ./build/68000/$(TARGET)
	-@rm -f ./build/68000/*.o
	-@rm -f ./build/68000/*.d
	$(STRIP) ./build/68020/$(TARGET)
	$(STACK) -S 128k ./build/68020/$(TARGET)
	-@rm -f ./build/68020/*.o
	-@rm -f ./build/68020/*.d
	$(STRIP) ./build/ColdFire/$(TARGET)
	$(STACK) -S 128k ./build/ColdFire/$(TARGET)
	-@rm -f ./build/ColdFire/*.o
	-@rm -f ./build/ColdFire/*.d
	@echo All done

clean:
	-@rm -f ./build/68000/*.o
	-@rm -f ./build/68000/*.d
	-@rm -f ./build/68000/$(TARGET)
	-@rm -f ./build/68020/*.o
	-@rm -f ./build/68020/*.d
	-@rm -f ./build/68020/$(TARGET)
	-@rm -f ./build/ColdFire/*.o
	-@rm -f ./build/ColdFire/*.d
	-@rm -f ./build/ColdFire/$(TARGET)
	@echo Cleaned

new: clean
	-@rm -f ./build/68000/$(TARGET)
	-@rm -f ./build/68020/$(TARGET)
	-@rm -f ./build/ColdFire/$(TARGET)
	$(MAKE) all

$(OBJS):

$(TARGET): $(OBJS)
	cd ./core && $(MAKE) all && cd ..
	$(CP) $(CFLAGS) -m68000 -c main.cpp -o ./build/68000/main.o
	$(CP) $(CFLAGS) -m68020-60 -c main.cpp -o ./build/68020/main.o
	$(CP) $(CFLAGS) -mcpu=5475 -c main.cpp -o ./build/ColdFire/main.o
	$(CC) ./build/68000/*.o -m68000 $(LDLIBS) -o ./build/68000/$(TARGET)
	$(CC) ./build/68020/*.o -m68020-60 $(LDLIBS) -o ./build/68020/$(TARGET)
	$(CC) ./build/ColdFire/*.o -mcpu=5475 $(LDLIBS) -o ./build/ColdFire/$(TARGET)
