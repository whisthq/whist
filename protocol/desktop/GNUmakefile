# Prerequisite: sudo apt install libswscale-dev

UNAME = $(shell uname -s)
CC = gcc

### LINUX ###
ifeq ($(UNAME), Linux)

PLATFORM = linux

SHARED_LIBS = \
	-lm \
	-ldl \
	-lswscale \
	-lpthread
endif


### MACOS ###
ifeq ($(UNAME), Darwin)

export SDKROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk

PLATFORM = macos

SHARED_LIBS = \
	-liconv \
	-framework OpenGL \
	-framework VideoToolbox \
	-framework CoreMedia \
	-framework CoreAudio \
	-framework AudioToolbox \
	-framework IOKit \
	-framework ForceFeedback \
	-framework Carbon \
	-framework AppKit \
	-framework Metal \
	-framework QuartzCore
endif


### COMMON ###
# executable name
BIN_NAME = client

# objects to build
OBJS = fractal.o webserver.o videodecode.o main.o

# static libs to link against
STATIC_LIBS = ../lib/SDL2/$(PLATFORM)/libSDL2.a

# compiler flags
CFLAGS = \
	-Wall \
	-Wextra \
	-Wno-unused-value \
	-Wno-missing-field-initializers \
	-O3 \
	-Iinclude

fractal.obj: ../include/fractal.c
	$(CC) -c ../include/fractal.c
webserver.obj: ../include/webserver.c
	$(CC) -c ../include/webserver.c
videodecode.obj: ../include/videodecode.c
	$(CC) -c ../include/videodecode.c

# build all files into executable
all: clean clear $(OBJS)
	$(CC) -o $(BIN_NAME) $(OBJS) $(STATIC_LIBS) $(SHARED_LIBS)

# clean prior object files
clean:
	-@rm -rf $(OBJS)
	-@rm -rf $(BIN_NAME)

# clear folder
clear:
	clear

