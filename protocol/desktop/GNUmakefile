# shell
UNAME = $(shell uname -s)

################## LINUX ##################
ifeq ($(UNAME), Linux)

# specify compiler
CC = gcc

# specify platform
PLATFORM = linux

# dynamic libs to link against
DYNAMIC_LIBS = \
    ../lib/ffmpeg/linux/libavcodec.so.58.100 \
	../lib/ffmpeg/linux/libavdevice.so.58.8.100 \
	../lib/ffmpeg/linux/libavfilter.so.7.57.100 \
	../lib/ffmpeg/linux/libavformat.so.58.29.100 \
	../lib/ffmpeg/linux/libavutil.so.56.31.100 \
	../lib/ffmpeg/linux/libpostproc.so.55.5.100 \
	../lib/ffmpeg/linux/libswresample.so.3.5.100 \
	../lib/ffmpeg/linux/libswscale.so.5.5.100 \
	../lib/mfx/linux/libmfx.so.1.32

# system libs to link against
SHARED_LIBS = \
	-lm \
	-ldl \
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

# static libs to link against
STATIC_LIBS = \
	../lib/SDL2/$(PLATFORM)/libSDL2.a \
	../lib/openssl/$(PLATFORM)/libcrypto.a

# files to build into object files
OBJS = \
	fractal.o \
	videodecode.o \
	audiodecode.o \
	video.o \
	audio.o \
	main.o \
	aes.o

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

# compile the video decoder source file into object file
videodecode.o:
	$(CC) -c ../include/videodecode.c

# compile the audio decoder source file into object file
audiodecode.o:
	$(CC) -c ../include/audiodecode.c

# compile the aes source file into object file
aes.o:
	$(CC) -c ../include/aes.c

# compile the main video source file into object file
video.o:
	$(CC) -c video.c

# compile the main audio source file into object file
audio.o:
	$(CC) -c audio.c

# compile the main source file into object file
main.o:
	$(CC) -c main.c

# clean prior object files
clean:
	-@rm -rf $(OBJS)
	-@rm -rf $(BIN_NAME)

# clear folder
clear:
	clear
