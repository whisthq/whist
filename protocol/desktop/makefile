# specify compiler
CC = cl.exe

# specify linker
LINK = link.exe

# specify executable name
BIN_NAME = desktop.exe

# files to build into object files
OBJS =            \
  fractal.obj     \
  clipboard.obj   \
  aes.obj         \
  videodecode.obj \
  audiodecode.obj \
  video.obj       \
  audio.obj       \
  main.obj

# static libs to link against
STATIC_LIBS =                           \
  ../lib/ffmpeg/windows/avcodec.lib     \
  ../lib/ffmpeg/windows/avdevice.lib    \
  ../lib/ffmpeg/windows/avfilter.lib    \
  ../lib/ffmpeg/windows/avformat.lib    \
  ../lib/ffmpeg/windows/avutil.lib      \
  ../lib/ffmpeg/windows/postproc.lib    \
  ../lib/ffmpeg/windows/swresample.lib  \
  ../lib/ffmpeg/windows/swscale.lib     \
  ../lib/SDL2/windows/SDL2.lib          \
  ../lib/mfx/windows/libmfx.lib         \
  ../lib/openssl/windows/libcrypto.lib

# compiler flags
CFLAGS =                \
  /nologo               \
  /W4                   \
  /O3                   \
  /MT                   \
  /MP                   \
  -DWIN32_LEAN_AND_MEAN \
  -DUNICODE             \
  -DWIN32               \
  -I ../include

# system libs to link against
SHARED_LIBS =       \
  libvcruntime.lib  \
  libucrt.lib       \
  libcmt.lib        \
  kernel32.lib      \
  gdi32.lib         \
  winmm.lib         \
  imm32.lib         \
  shell32.lib       \
  advapi32.lib      \
  ole32.lib         \
  oleaut32.lib      \
  opengl32.lib      \
  user32.lib        \
  uuid.lib          \
  version.lib       \
  ws2_32.lib        \
  crypt32.lib

# linking flags
LD_FLAGS =      \
  /nodefaultlib \
  /nologo

# link all files into executable
all: clean clear $(OBJS)
  link $(LD_FLAGS) $(STATIC_LIBS) /out:$(BIN_NAME) *.obj $(SHARED_LIBS)

# compile the main fractal header source file into object file
fractal.obj:
  $(CC) /c $(CFLAGS) ../include/fractal.c

# compile the main clipboard source file into object file
clipboard.obj:
  $(CC) /c $(CFLAGS) ../include/clipboard.c

# compile the aes source file into object file
aes.obj:
  $(CC) /c $(CFLAGS) ../include/aes.c

# compile the video decoder source file into object file
videodecode.obj:
  $(CC) /c $(CFLAGS) ../include/videodecode.c

# compile the audio decoder source file into object file
audiodecode.obj:
  $(CC) /c $(CFLAGS) ../include/audiodecode.c

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
