# Makefile for Breakout (Miyoo Mini Plus)
#
# 1) Native build, for fast iteration on your PC (Linux/macOS/WSL).
#    Requires SDL2 dev package (e.g. `sudo apt install libsdl2-dev`
#    or `brew install sdl2`):
#
#       make
#       ./breakout
#
# 2) Cross build for the Miyoo Mini Plus itself.
#    Run this *inside* the union-miyoomini-toolchain docker shell
#    (https://github.com/shauninman/union-miyoomini-toolchain),
#    from this project folder:
#
#       make TARGET=miyoo
#
#    That toolchain already ships a working sdl2-config/pkg-config
#    for the patched "mmiyoo" SDL2 used on-device, so this Makefile
#    will pick it up automatically. If `make TARGET=miyoo` can't find
#    SDL2, check the toolchain's own example project (e.g. hello-miyoo)
#    for the exact include/lib paths used by your toolchain version.

TARGET ?= native
CROSS  ?=

CC := $(CROSS)gcc

ifeq ($(TARGET),miyoo)
  CFLAGS := -O2 -marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -DMIYOO
else
  CFLAGS := -O2 -Wall -Wextra
endif

SDL_CFLAGS := $(shell pkg-config sdl2 --cflags 2>/dev/null || sdl2-config --cflags 2>/dev/null)
SDL_LIBS   := $(shell pkg-config sdl2 --libs   2>/dev/null || sdl2-config --libs   2>/dev/null)

BIN := breakout
SRC := main.c

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $(SRC) $(SDL_LIBS) -lm

clean:
	rm -f $(BIN)

.PHONY: all clean
