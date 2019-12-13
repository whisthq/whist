UNAME = $(shell uname -s)


### LINUX ###
ifeq ($(UNAME), Linux)

PLATFORM = linux

SHARED_LIBS = \
	-lm \
	-ldl \
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
OBJS = ../include/fractal.o ../include/webserver.o main.o

# static libs to link against
STATIC_LIBS = # lib/$(PLATFORM)/libSDL2.a add ffmpeg and sdl libs

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
	$(CC) -o $(BIN_NAME) $(OBJS) $(STATIC_LIBS) $(SHARED_LIBS)

# clean prior object files
clean:
	-@rm -rf $(OBJS)
	-@rm -rf $(BIN_NAME)

# clear folder
clear:
	clear
