UNAME = $(shell uname -s)
CC = gcc

### LINUX ###
ifeq ($(UNAME), Linux)

PLATFORM = linux

SHARED_LIBS = \
	-lm \
	-ldl \
	-lswscale -lavutil -lavcodec \
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
OBJS = ../include/fractal.o ../include/webserver.o ../include/videoencode.o main.o

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

fractal.o:
	$(CC) -c ../include/fractal.c

webserver.o:
	$(CC) -c ../include/webserver.c

videoencode.o:
	$(CC) -c ../include/videoencode.c

videodecode.o:
	$(CC) -c ../include/videodecode.c

audiodecode.o:
	$(CC) -c ../include/audiodecode.c

linkedlist.o:
	$(CC) -c ../include/linkedlist.c

main.o:
	$(CC) -c main.c

desktop: main.o fractal.o webserver.o videoencode.o videodecode.o audiodecode.o linkedlist.o
	$(CC) -o desktop main.o fractal.o webserver.o videoencode.o videodecode.o audiodecode.o linkedlist.o $(STATIC_LIBS) $(SHARED_LIBS)

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

