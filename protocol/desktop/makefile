# specify compiler
CC = cl.exe

# specify linker
LINK = link.exe

# query arch
!IF "$(PROCESSOR_ARCHITEW6432)" == ""
ARCH = 32
!ELSE
ARCH = 64
!ENDIF

# specify executable name
BIN_NAME = build$(ARCH)/FractalClient.exe

# files to build into object files
OBJS = \
	fractal.obj \
	clock.obj \
	clipboard_synchronizer.obj \
	win_clipboard.obj \
	aes.obj \
	videodecode.obj \
	audiodecode.obj \
	logging.obj \
	network.obj \
        sdl_utils.obj \
	sdlscreeninfo.obj \
	video.obj \
	audio.obj \
	main.obj

# static libs to link against
STATIC_LIBS = \
	../lib/$(ARCH)/ffmpeg/windows/avcodec.lib \
	../lib/$(ARCH)/ffmpeg/windows/avdevice.lib \
	../lib/$(ARCH)/ffmpeg/windows/avfilter.lib \
	../lib/$(ARCH)/ffmpeg/windows/avformat.lib \
	../lib/$(ARCH)/ffmpeg/windows/avutil.lib \
	../lib/$(ARCH)/ffmpeg/windows/postproc.lib \
	../lib/$(ARCH)/ffmpeg/windows/swresample.lib \
	../lib/$(ARCH)/ffmpeg/windows/swscale.lib \
	../lib/$(ARCH)/SDL2/windows/SDL2.lib \
	../lib/$(ARCH)/mfx/windows/libmfx.lib \
	../lib/$(ARCH)/openssl/windows/libcrypto.lib

# compiler flags
CFLAGS = \
	/nologo \
	/W4 \
	/O2i \
	/MT \
	/MP \
	-DWIN32_LEAN_AND_MEAN \
	-DUNICODE \
	-DWIN32 \
	-I ../include \
	-I ../include/ffmpeg

# system libs to link against
SHARED_LIBS = \
	libvcruntime.lib \
	libucrt.lib \
	libcmt.lib \
	kernel32.lib \
	gdi32.lib \
	winmm.lib \
	imm32.lib \
	shell32.lib \
	advapi32.lib \
	ole32.lib \
	oleaut32.lib \
	opengl32.lib \
	user32.lib \
	uuid.lib \
	version.lib \
	ws2_32.lib \
	shlwapi.lib \
	crypt32.lib \
	d3d11.lib \
	dxgi.lib \
    shcore.lib

# linking flags
LD_FLAGS = \
	/nodefaultlib \
	/nologo

# link all files into executable
all: clean clear $(OBJS)
	link $(LD_FLAGS) $(STATIC_LIBS) /out:$(BIN_NAME) *.obj $(SHARED_LIBS)

# compile the main fractal header source file into object file
fractal.obj:
	$(CC) /c $(CFLAGS) ../fractal/core/fractal.c

# compile the clock header source file into object file
clock.obj:
	$(CC) /c $(CFLAGS) ../fractal/utils/clock.c

# compile the main clipboard thread source file into object file
clipboard_synchronizer.obj:
	$(CC) /c $(CFLAGS) ../fractal/clipboard/clipboard_synchronizer.c

# compile the main clipboard source file into object file
win_clipboard.obj:
	$(CC) /c $(CFLAGS) ../fractal/clipboard/win_clipboard.c

# compile the aes source file into object file
aes.obj:
	$(CC) /c $(CFLAGS) ../fractal/utils/aes.c

# compile the video decoder source file into object file
videodecode.obj:
	$(CC) /c $(CFLAGS) ../fractal/video/videodecode.c

# compile the logging source file into object file
logging.obj:
	$(CC) /c $(CFLAGS) ../fractal/utils/logging.c

# compile the networking source file into object file
network.obj:
	$(CC) /c $(CFLAGS) ../fractal/network/network.c

# compile the SDL screen info source file into object file
sdl_utils.obj:
	$(CC) /c $(CFLAGS) sdl_utils.c

# compile the SDL screen info source file into object file
sdlscreeninfo.obj:
	$(CC) /c $(CFLAGS) ../fractal/utils/sdlscreeninfo.c

# compile the audio decoder source file into object file
audiodecode.obj:
	$(CC) /c $(CFLAGS) ../fractal/audio/audiodecode.c

# compile the main video source file into object file
video.obj:
	$(CC) /c $(CFLAGS) video.c

# compile the main audio source file into object file
audio.obj:
	$(CC) /c $(CFLAGS) audio.c

# compile the main source file into object file
main.obj:
	$(CC) /c $(CFLAGS) main.c
	
# clean prior object files
clean:
	-@del $(BIN_NAME)
	-@del *.obj

# clear folder
clear:
	cls
