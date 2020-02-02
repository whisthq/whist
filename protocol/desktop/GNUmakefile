# shell
UNAME = $(shell uname -s)

################## LINUX ##################
ifeq ($(UNAME), Linux)

# specify compiler
CC = gcc

# specify platform
PLATFORM = linux

# system libs to link against
SHARED_LIBS = \
	-lm \
	-ldl \
	-lswscale \
	-lavutil \
	-lavcodec \
	-lpthread
endif


################## MACOS ##################
ifeq ($(UNAME), Darwin)

# specify compiler
CC = clang

# specify platform
PLATFORM = macos

# Apple SDK
export SDKROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk

# dynamic libs to link against
DYNAMIC_LIBS = \
  ../lib/ffmpeg/macos/libavcodec.58.dylib \
	../lib/ffmpeg/macos/libavdevice.58.dylib \
	../lib/ffmpeg/macos/libavfilter.7.dylib \
	../lib/ffmpeg/macos/libavformat.58.dylib \
	../lib/ffmpeg/macos/libavutil.56.dylib \
	../lib/ffmpeg/macos/libpostproc.55.dylib \
	../lib/ffmpeg/macos/libswresample.3.dylib \
	../lib/ffmpeg/macos/libswscale.5.dylib

# system libs to link against
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


################## COMMON ##################
# executable name
BIN_NAME = desktop

# files to build into object files
OBJS = \
	fractal.o \
	webserver.o \
	videodecode.o \
	audiodecode.o \
	linkedlist.o \
	video.o \
	audio.o \
	main.o

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

# build all files into executable
all: clean clear $(OBJS)
	$(CC) -o $(BIN_NAME) $(OBJS) $(STATIC_LIBS) $(DYNAMIC_LIBS) $(SHARED_LIBS)

# compile the main fractal header source file into object file
fractal.o:
	$(CC) -c ../include/fractal.c

# compile the webserver source file into object file
webserver.o:
	$(CC) -c ../include/webserver.c

# compile the video decoder source file into object file
videodecode.o:
	$(CC) -c ../include/videodecode.c

# compile the audio decoder source file into object file
audiodecode.o:
	$(CC) -c ../include/audiodecode.c

# compile the linked list source file into object file
linkedlist.o:
	$(CC) -c ../include/linkedlist.c

# compile the source main file into object file
video.o:
	$(CC) -c video.c

# compile the source main file into object file
audio.o:
	$(CC) -c audio.c

# compile the source main file into object file
main.o:
	$(CC) -c main.c

# clean prior object files
clean:
	-@rm -rf $(OBJS)
	-@rm -rf $(BIN_NAME)

# clear folder
clear:
	clear
