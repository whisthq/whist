# specify compiler
CC = cl.exe

# specify linker
LINK = link.exe

# specify bin name
BIN_NAME = server.exe

STATIC_LIBS = \
  ../lib/ffmpeg/windows/avcodec.lib \
  ../lib/ffmpeg/windows/avdevice.lib \
  ../lib/ffmpeg/windows/avfilter.lib \
  ../lib/ffmpeg/windows/avformat.lib \
  ../lib/ffmpeg/windows/avutil.lib \
  ../lib/ffmpeg/windows/postproc.lib \
  ../lib/ffmpeg/windows/swresample.lib \
  ../lib/ffmpeg/windows/swscale.lib \
  ../lib/SDL2/windows/SDL2.lib
  
# compiler flags
CFLAGS = \
  /nologo \
  /W4 \
  /Ofast \
  /MT \
  /MP \
  -DWIN32_LEAN_AND_MEAN \
  -DUNICODE \
  -DWIN32

# system libs to link against
SHARED_LIBS = \
  libvcruntime.lib \
  libcmt.lib \
  kernel32.lib \
  ws2_32.lib \
  crypt32.lib \
  wldap32.lib \
  normaliz.lib \
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
  dxgi.lib \
  d3d11.lib \
  winspool.lib \
  comdlg32.lib \
  odbc32.lib \
  odbccp32.lib \
  ucrt.lib \
  vcruntime.lib \
  msvcrt.lib \
  msvcmrt.lib \
  libcpmt.lib \
  msvcprt.lib

# linking flags
LD_FLAGS = \
  /nodefaultlib \
  /nologo


# build all files into executable
all: clean clear fractal.obj videocapture.obj audiocapture.obj videoencode.obj audioencode.obj socket.obj main.obj
	link $(LD_FLAGS) $(STATIC_LIBS) /out:$(BIN_NAME) *.obj $(SHARED_LIBS)

# compile the main fractal header source file into object file
fractal.obj: ../include/fractal.c
	$(CC) /c ../include/fractal.c

# compile the video capture source file into object file
videocapture.obj: ../include/videocapture.c
	$(CC) /c ../include/videocapture.c

# compile the audio capture source file into object file
audiocapture.obj: ../include/audiocapture.c
	$(CC) /c ../include/audiocapture.c

# compile the video encoder source file into object file
videoencode.obj: ../include/videoencode.c
	$(CC) /c ../include/videoencode.c

# compile the audio encoder source file into object file
audioencode.obj: ../include/audioencode.c
	$(CC) /c ../include/audioencode.c

socket.obj: ../include/socket.c
  $(CC) /c ../include/socket.c
  
# compile the source main client file into object file
main.obj: main.c
  $(CC) /c main.c

# clean prior object files
clean:
  -@del $(BIN_NAME)
  -@del *.obj

# clear folder
clear:
  cls
