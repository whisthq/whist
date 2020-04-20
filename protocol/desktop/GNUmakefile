# shell
UNAME = $(shell uname -s)

# query arch
ARCH = $(shell getconf LONG_BIT)

################## LINUX ##################
ifeq ($(UNAME), Linux)

# specify compiler
CC = gcc

# specify platform
PLATFORM = linux

# files to build into object files
OBJS = \
	fractal.o \
	videodecode.o \
	audiodecode.o \
	video.o \
	audio.o \
	aes.o \
	clipboard_thread.o \
	x11_clipboard.o \
	main.o

# dynamic libs to link against (using system libs for Linux)
DYNAMIC_LIBS =

# system libs to link against
SHARED_LIBS = \
	-lm \
	-ldl \
	-lpthread \
	-lavcodec \
	-lavdevice \
	-lavfilter \
	-lavformat \
	-lavutil \
	-lpostproc \
	-lswresample \
	-lswscale \
	-lX11 \
	-lXfixes
endif

################## MACOS ##################
ifeq ($(UNAME), Darwin)

# specify compiler and minimum MacOS version support
CC = clang -mmacosx-version-min=10.13

# specify platform
PLATFORM = macos

# files to build into object files
OBJS = \
	fractal.o \
	videodecode.o \
	audiodecode.o \
	video.o \
	audio.o \
	aes.o \
	clipboard_osx.o \
	mac_utils.o \
	mac_clipboard.o \
	clipboard_thread.o \
	main.o

# Apple SDK (standard MacOSX or MacOSX10.15 is ideal - use 10.14 if Xcode bug)
# export SDKROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX10.14.sdk
# export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX10.15.sdk
# export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk

# dynamic libs to link against
DYNAMIC_LIBS = \
	../lib/64/ffmpeg/macos/libavcodec.58.dylib \
	../lib/64/ffmpeg/macos/libavdevice.58.dylib \
	../lib/64/ffmpeg/macos/libavfilter.7.dylib \
	../lib/64/ffmpeg/macos/libavformat.58.dylib \
	../lib/64/ffmpeg/macos/libavutil.56.dylib \
	../lib/64/ffmpeg/macos/libpostproc.55.dylib \
	../lib/64/ffmpeg/macos/libswresample.3.dylib \
	../lib/64/ffmpeg/macos/libswscale.5.dylib

# system libs to link against
SHARED_LIBS = \
	-lobjc \
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
BIN_NAME = build$(ARCH)/desktop

# static libs to link against
STATIC_LIBS = \
	../lib/$(ARCH)/SDL2/$(PLATFORM)/libSDL2.a \
	../lib/$(ARCH)/openssl/$(PLATFORM)/libcrypto.a

# compiler flags
CFLAGS = \
	-g \
	-Wall \
	-Wextra \
	-Wno-missing-braces \
	-Wno-unused-value \
	-Wno-unused-parameter \
	-I ../include \
	-I ../include/ffmpeg \
	-O0

LDFLAGS = \
	  -g \
	  -rdynamic \
	  -O0

# link all files into executable
all: clean clear $(OBJS)
	$(CC) $(LDFLAGS) -o $(BIN_NAME) $(OBJS) $(STATIC_LIBS) $(DYNAMIC_LIBS) $(SHARED_LIBS)

# compile the main fractal header source file into object file
fractal.o:
	$(CC) -c $(CFLAGS) ../include/fractal.c

# compile the video decoder source file into object file
videodecode.o:
	$(CC) -c $(CFLAGS) ../include/videodecode.c

# compile the audio decoder source file into object file
audiodecode.o:
	$(CC) -c $(CFLAGS) ../include/audiodecode.c

# compile the aes source file into object file
aes.o:
	$(CC) -c $(CFLAGS) ../include/aes.c

# compile the MacOS clipboard source file into object file (Requires MacOS 10.14+)
clipboard_osx.o:
	$(CC) -c $(CFLAGS) ../include/clipboard_osx.m

# compile the MacOS utils source file into object file (Requires MacOS 10.14+)
mac_utils.o:
	$(CC) -c $(CFLAGS) ../include/mac_utils.c

# compile the general clipboard source file into object file
clipboard_thread.o:
	$(CC) -c $(CFLAGS) ../include/clipboard_thread.c

# compile the general mac clipboard file into object file
mac_clipboard.o:
	$(CC) -c $(CFLAGS) ../include/mac_clipboard.c

# compile the general clipboard source file into object file
x11_clipboard.o:
	$(CC) -c $(CFLAGS) ../include/x11_clipboard.c

# compile the main video source file into object file
video.o:
	$(CC) -c $(CFLAGS) video.c

# compile the main audio source file into object file
audio.o:
	$(CC) -c $(CFLAGS) audio.c

# compile the main source file into object file
main.o:
	$(CC) -c $(CFLAGS) main.c

# clean prior object files
clean:
	-@rm -rf $(OBJS)
	-@rm -rf $(BIN_NAME)

# clear folder
clear:
	clear
