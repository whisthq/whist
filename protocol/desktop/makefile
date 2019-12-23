# specify compiler
CC = cl.exe

# specify linker
LINK = link.exe

# specify bin name
BIN_NAME = client.exe

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
all: clean clear findip.obj socket.obj main.obj
  link $(LD_FLAGS) /out:$(BIN_NAME) *.obj $(SHARED_LIBS)

# compile the IP finder helper file into object file
findip.obj: ../include/findip.c
	$(CC) /c ../include/findip.c

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
