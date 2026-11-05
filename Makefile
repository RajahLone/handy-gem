#
# Makefile
#
CFLAGS  = -O3 -fomit-frame-pointer -ffast-math
LDFLAGS = -s
LDLIBS  = -lgem -lm -lstdc++

TARGET = handy.prg

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
	-@rm -f ./build/68000/*.*
	-@rm -f ./build/68020/*.*
	-@rm -f ./build/ColdFire/*.*
	@echo Cleaned

new: clean
	-@rm -f ./build/68000/$(TARGET)
	-@rm -f ./build/68020/$(TARGET)
	-@rm -f ./build/ColdFire/$(TARGET)
	$(MAKE) all

$(OBJS):

$(TARGET):
	mkdir -p ./build/68000
	mkdir -p ./build/68020
	mkdir -p ./build/ColdFire
	cd ./core && $(MAKE) all && cd ..
	cd ./appl && $(MAKE) all && cd ..
	$(CC) $(LDFLAGS) ./build/68000/*.o -o ./build/68000/$(TARGET) -m68000 -L$(PREFIX)/m68k-atari-mint/lib $(LDLIBS)
	$(CC) $(LDFLAGS) ./build/68020/*.o -o ./build/68020/$(TARGET) -m68020-60 -L$(PREFIX)/m68k-atari-mint/lib/m68020-60 $(LDLIBS)
	$(CC) $(LDFLAGS) ./build/ColdFire/*.o -o ./build/ColdFire/$(TARGET) -mcpu=5475 -L$(PREFIX)/m68k-atari-mint/lib/m5475 $(LDLIBS)
