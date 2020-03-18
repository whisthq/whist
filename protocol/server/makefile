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
  ../lib/SDL2/windows/SDL2.lib \
  ../lib/openssl/windows/libcrypto.lib
  
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
  -I ../include

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
  msvcprt.lib \
  avrt.lib \
  d3dcompiler.lib \
  dxguid.lib \
  crypt32.lib \
  Shlwapi.lib

# linking flags
LD_FLAGS = \
  /nodefaultlib \
  /nologo

# build all files into executable
all: clean fractal.obj clipboard.obj aes.obj videoencode.obj wasapicapture.obj dxgicapture.obj desktop.obj input.obj main.obj
	link $(LD_FLAGS) $(STATIC_LIBS) /out:$(BIN_NAME) *.obj $(SHARED_LIBS)

# compile the main fractal header source file into object file
fractal.obj:
	$(CC) /c ../include/fractal.c

# compile the main clipboard header source file into object file
clipboard.obj:
	$(CC) /c ../include/clipboard.c

# compile the aes source file into object file
aes.obj:
  $(CC) /c ../include/aes.c

# compile the video encoder source file into object file
videoencode.obj:
	$(CC) /c ../include/videoencode.c

# compile the audio capture source file into object file
wasapicapture.obj:
  $(CC) /c ../include/wasapicapture.c

# compile the video capture source file into object file
dxgicapture.obj:
  $(CC) /c ../include/dxgicapture.c

# compile the desktop setting source file into object file
desktop.obj:
  $(CC) /c ../include/desktop.c

# compile the input handling source file into object file
input.obj:
  $(CC) /c ../include/input.c

# compile the source main client file into object file
main.obj:
  $(CC) /c main.c

# clean prior object files
clean:
  -@del $(BIN_NAME)
  -@del *.obj

# clear folder
clear:
  cls
