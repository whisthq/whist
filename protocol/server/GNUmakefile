# specify compiler
CC = gcc

# specify platform
PLATFORM = linux

# query arch
ARCH = $(shell getconf LONG_BIT)

# executable name
BIN_NAME = build$(ARCH)/server

DYNAMIC_LIBS =

# dynamic libs to link against
DYNAMIC2_LIBS = \
	../lib/$(ARCH)/ffmpeg/linux/libavcodec.so.58.54.100 \
	../lib/$(ARCH)/ffmpeg/linux/libavdevice.so.58.8.100 \
	../lib/$(ARCH)/ffmpeg/linux/libavfilter.so.7.57.100 \
	../lib/$(ARCH)/ffmpeg/linux/libavformat.so.58.29.100 \
	../lib/$(ARCH)/ffmpeg/linux/libavutil.so.56.31.100 \
	../lib/$(ARCH)/ffmpeg/linux/libpostproc.so.55.5.100 \
	../lib/$(ARCH)/ffmpeg/linux/libswresample.so.3.5.100 \
	../lib/$(ARCH)/ffmpeg/linux/libswscale.so.5.5.100 \
	../lib/$(ARCH)/mfx/linux/libmfx.so.1.32

# system libs to link against
SHARED_LIBS = \
	-lm \
	-ldl \
	-lpthread \
	-lX11 \
	-lXext \
	-lXtst \
	-lasound \
	-lXdamage \
	-lavcodec \
	-lavdevice \
	-lavfilter \
	-lavformat \
	-lavutil \
	-lpostproc \
	-lswresample \
	-lswscale

# static libs to link against
STATIC_LIBS = \
	../lib/$(ARCH)/SDL2/$(PLATFORM)/libSDL2.a \
	../lib/$(ARCH)/openssl/$(PLATFORM)/libcrypto.a

# files to build into object files
OBJS = \
	fractal.o \
        x11capture.o \
	videoencode.o \
	alsacapture.o \
	linuxinput.o \
	audioencode.o \
	aes.o \
	clipboard.o \
	main.o

# compiler flags
CFLAGS = \
	-fno-pie \
	-g \
	-Wall \
	-Wextra \
	-Wno-missing-braces \
	-Wno-unused-value \
	-Wno-unused-parameter \
	-I ../include \
	-O0

LDFLAGS = \
	  -no-pie \
	  -g -rdynamic \
	  -O0

# link all files into executable
all: clean clear $(OBJS)
	$(CC) $(LDFLAGS) -o $(BIN_NAME) $(OBJS) $(STATIC_LIBS) $(DYNAMIC_LIBS) $(SHARED_LIBS)

# compile the main fractal header source file into object file
fractal.o:
	$(CC) -c $(CFLAGS) ../include/fractal.c

# compile the main fractal header source file into object file
x11capture.o:
	$(CC) -c $(CFLAGS) ../include/x11capture.c

# compile the video encoder source file into object file
videoencode.o:
	$(CC) -c $(CFLAGS) ../include/videoencode.c

# compile the audio encoder source file into object file
audioencode.o:
	$(CC) -c $(CFLAGS) ../include/audioencode.c

# compile the aes source file into object file
aes.o:
	$(CC) -c $(CFLAGS) ../include/aes.c

# compile the general clipboard source file into object file
clipboard.o:
	$(CC) -c $(CFLAGS) ../include/clipboard.c

# compile the audio capture source file into object file
alsacapture.o:
	$(CC) -c $(CFLAGS) ../include/alsacapture.c

# compile the input playback source file into object file
linuxinput.o:
	$(CC) -c $(CFLAGS) ../include/linuxinput.c

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
