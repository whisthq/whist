UNAME = $(shell uname -s)


### LINUX ###
ifeq ($(UNAME), Linux)

PLATFORM = linux

SHARED_LIBS = -ldl

endif


### MACOS ###
ifeq ($(UNAME), Darwin)

PLATFORM = macos

export SDKROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk

endif


### COMMON ###
# executable name
BIN_NAME = server

# objects to build
OBJS = ../include/fractal.o ../include/videocapture.o \
 			 ../include/encode.o ../include/main.o

# static libs to link against
STATIC_LIBS = # lib/$(PLATFORM)/libSDL2.a add ffmpeg & SDL libs here

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
