# specify compiler
CC = gcc

# specify platform
PLATFORM = linux

# executable name
BIN_NAME = server

# dynamic libs to link against
DYNAMIC_LIBS = 																	\
	../lib/ffmpeg/linux/libavcodec.so.58.100 	\
	../lib/ffmpeg/linux/libavdevice.so.58.8.100 	\
	../lib/ffmpeg/linux/libavfilter.so.7.57.100 	\
	../lib/ffmpeg/linux/libavformat.so.58.29.100 	\
	../lib/ffmpeg/linux/libavutil.so.56.31.100 	\
	../lib/ffmpeg/linux/libpostproc.so.55.5.100 	\
	../lib/ffmpeg/linux/libswresample.so.3.5.100 	\
	../lib/ffmpeg/linux/libswscale.so.5.5.100 	\
	../lib/mfx/linux/libmfx.so.1.32

# system libs to link against
SHARED_LIBS = 		\
	-lm 		\
	-ldl 		\
	-lpthread

# static libs to link against
STATIC_LIBS = 					\
	../lib/SDL2/$(PLATFORM)/libSDL2.a 	\
	../lib/openssl/$(PLATFORM)/libcrypto.a

# files to build into object files
OBJS = 				\
	fractal.o 		\
	videoencode.o 		\
	audioencode.o 		\
	aes.o 			\
	clipboard.o 		\
	main.o

# compiler flags
CFLAGS = 				\
	-Wall 				\
	-Wextra 			\
	-Wno-missing-braces \
	-Wno-unused-value \
	-Wno-unused-parameter \
	-O3 				\
	-I ../include

# link all files into executable
all: clean clear $(OBJS)
	$(CC) -o $(BIN_NAME) $(OBJS) $(STATIC_LIBS) $(DYNAMIC_LIBS) $(SHARED_LIBS)

# compile the main fractal header source file into object file
fractal.o:
	$(CC) -c $(CFLAGS) ../include/fractal.c

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
