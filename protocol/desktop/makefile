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
  ../lib/mfx/libmfx.lib

# compiler flags
CFLAGS = \
  /nologo \
  /W4 \
  /O2 \
  /MT \
  /MP \
  -Iinclude \
  -DWIN32_LEAN_AND_MEAN \
  -DUNICODE \
  -DWIN32

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
  ws2_32.lib

# linking flags
LD_FLAGS = \
  /nodefaultlib \
  /nologo

# build all files into executable
all: clean clear fractal.obj webserver.obj videodecode.obj audiodecode.obj linkedlist.obj main.obj
  link $(LD_FLAGS) $(STATIC_LIBS) /out:$(BIN_NAME) *.obj $(SHARED_LIBS)

# compile the main fractal header source file into object file
fractal.obj: ../include/fractal.c
  $(CC) /c ../include/fractal.c

# compile the webserver source file into object file
webserver.obj: ../include/webserver.c
  $(CC) /c ../include/webserver.c

# compile the video decoder source file into object file
videodecode.obj: ../include/videodecode.c
  $(CC) /c ../include/videodecode.c

# compile the audio decoder source file into object file
audiodecode.obj: ../include/audiodecode.c
  $(CC) /c ../include/audiodecode.c

# compile the linked list source file into object file
linkedlist.obj: ../include/linkedlist.c
  $(CC) /c ../include/linkedlist.c

# compile the source main file into object file
main.obj: main.c
  $(CC) /c main.c

# clean prior object files
clean:
  -@del $(BIN_NAME)
  -@del *.obj

# clear folder
clear:
  cls
