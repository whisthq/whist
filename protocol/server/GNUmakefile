# specify compiler
CC = gcc

# specify platform
PLATFORM = linux

# query arch
ARCH = $(shell getconf LONG_BIT)

# executable name
BIN_NAME = build$(ARCH)/FractalServer

# dynamic libs to link against
DYNAMIC_LIBS = \
	../lib/$(ARCH)/mfx/linux/libmfx.so.1.32

# system libs to link against
SHARED_LIBS = \
	-lm \
	-ldl \
	-lpthread \
	-lX11 \
	-lXfixes \
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
	clock.o \
	x11capture.o \
	videoencode.o \
	alsacapture.o \
	linuxinput.o \
	linuxcursor.o \
	logging.o \
	network.o \
	sdlscreeninfo.o \
	audioencode.o \
	aes.o \
	x11_clipboard.o \
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
	-g \
	-rdynamic \
	-O0

# link all files into executable
all: clean clear $(OBJS)
	$(CC) $(LDFLAGS) -o $(BIN_NAME) $(OBJS) $(STATIC_LIBS) $(DYNAMIC_LIBS) $(SHARED_LIBS)

# compile the main fractal header source file into object file
fractal.o:
	$(CC) -c $(CFLAGS) ../fractal/core/fractal.c

# compile the main clock source file into object file
clock.o:
	$(CC) -c $(CFLAGS) ../fractal/utils/clock.c

# compile the logging source file into object file
logging.o:
	$(CC) -c $(CFLAGS) ../fractal/utils/logging.c

# compile the networking source file into object file
network.o:
	$(CC) -c $(CFLAGS) ../fractal/network/network.c

# compile the SDL screen info source file into object file
sdlscreeninfo.o:
	$(CC) -c $(CFLAGS) ../fractal/utils/sdlscreeninfo.c

# compile the main fractal header source file into object file
x11capture.o:
	$(CC) -c $(CFLAGS) ../fractal/video/x11capture.c

# compile the video encoder source file into object file
videoencode.o:
	$(CC) -c $(CFLAGS) ../fractal/video/videoencode.c

# compile the audio encoder source file into object file
audioencode.o:
	$(CC) -c $(CFLAGS) ../fractal/audio/audioencode.c

# compile the aes source file into object file
aes.o:
	$(CC) -c $(CFLAGS) ../fractal/utils/aes.c

# compile the general clipboard source file into object file
x11_clipboard.o:
	$(CC) -c $(CFLAGS) ../fractal/clipboard/x11_clipboard.c

# compile the audio capture source file into object file
alsacapture.o:
	$(CC) -c $(CFLAGS) ../fractal/audio/alsacapture.c

# compile the input playback source file into object file
linuxinput.o:
	$(CC) -c $(CFLAGS) ../fractal/input/linuxinput.c

# compile the cursor management source file into object file
linuxcursor.o:
	$(CC) -c $(CFLAGS) ../fractal/cursor/linuxcursor.c

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
