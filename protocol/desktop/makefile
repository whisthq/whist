# specify compiler
CC = cl.exe

# specify linker
LINK = link.exe

# specify bin name
BIN_NAME = desktop.exe

# static libs to link against
STATIC_LIBS = \
  ../lib/ffmpeg/windows/avcodec.lib \
  ../lib/ffmpeg/windows/avdevice.lib \
  ../lib/ffmpeg/windows/avfilter.lib \
  ../lib/ffmpeg/windows/avformat.lib \
  ../lib/ffmpeg/windows/avutil.lib \
  ../lib/ffmpeg/windows/postproc.lib \
  ../lib/ffmpeg/windows/swresample.lib \
  ../lib/ffmpeg/windows/swscale.lib \
  ../lib/SDL2/windows/SDL2.lib \
  ../lib/mfx/windows/libmfx.lib \
  ../lib/openssl/windows/libcryptoMT.lib

# compiler flags
CFLAGS = \
  /nologo \
  /W4 \
  /O2i \
  /MT \
  /MP \
  /DWIN32 \
  -I ../include

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
  crypt32.lib

# linking flags
LD_FLAGS = \
  /nodefaultlib \
  /nologo

# build all files into executable
all: fractal.obj aes.obj webserver.obj videodecode.obj audiodecode.obj linkedlist.obj video.obj audio.obj main.obj
  link $(LD_FLAGS) $(STATIC_LIBS) /out:$(BIN_NAME) *.obj $(SHARED_LIBS)

# compile the main fractal header source file into object file
fractal.obj: ../include/fractal.c
  $(CC) /c $(CFLAGS) ../include/fractal.c

# compile the aes source file into object file
aes.obj: ../include/aes.c
  $(CC) /c $(CFLAGS) ../include/aes.c

# compile the webserver source file into object file
webserver.obj: ../include/webserver.c
  $(CC) /c $(CFLAGS) ../include/webserver.c

# compile the video decoder source file into object file
videodecode.obj: ../include/videodecode.c
  $(CC) /c $(CFLAGS) ../include/videodecode.c

# compile the audio decoder source file into object file
audiodecode.obj: ../include/audiodecode.c
  $(CC) /c $(CFLAGS) ../include/audiodecode.c

# compile the linked list source file into object file
linkedlist.obj: ../include/linkedlist.c
  $(CC) /c $(CFLAGS) ../include/linkedlist.c

# compile the source main file into object file
video.obj: video.c
  $(CC) /c $(CFLAGS) video.c

# compile the source main file into object file
audio.obj: audio.c
  $(CC) /c $(CFLAGS) audio.c

# compile the source main file into object file
main.obj: main.c
  $(CC) /c $(CFLAGS) main.c

# clean prior object files
clean:
  -@del $(BIN_NAME)
  -@del *.obj

# clear folder
clear:
  cls
