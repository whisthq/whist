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
BIN_NAME = build$(ARCH)/FractalServer.exe

# files to build into object files
OBJS = \
        webserver.obj \
	fractal.obj \
	win_clipboard.obj \
	aes.obj \
	videoencode.obj \
	audioencode.obj \
	wasapicapture.obj \
	dxgicapture.obj \
	desktop.obj \
	windowsinput.obj \
	windowscursor.obj \
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
	shlwapi.lib \
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
	$(CC) /c $(CFLAGS) ../include/fractal.c

# compile the main fractal header source file into object file
webserver.obj:
	$(CC) /c $(CFLAGS) ../include/webserver.c


# compile the main clipboard header source file into object file
win_clipboard.obj:
	$(CC) /c $(CFLAGS) ../include/win_clipboard.c

# compile the aes source file into object file
aes.obj:
	$(CC) /c $(CFLAGS) ../include/aes.c

# compile the video encoder source file into object file
videoencode.obj:
	$(CC) /c $(CFLAGS) ../include/videoencode.c

# compile the audio encoder source file into object file
audioencode.obj:
	$(CC) /c $(CFLAGS) ../include/audioencode.c

# compile the audio capture source file into object file
wasapicapture.obj:
	$(CC) /c $(CFLAGS) ../include/wasapicapture.c

# compile the video capture source file into object file
dxgicapture.obj:
	$(CC) /c $(CFLAGS) ../include/dxgicapture.c

# compile the desktop setting source file into object file
desktop.obj:
	$(CC) /c $(CFLAGS) ../include/desktop.c

# compile the input handling source file into object file
windowsinput.obj:
	$(CC) /c $(CFLAGS) ../include/windowsinput.c

# compile the cursor management source file into object file
windowscursor.obj:
	$(CC) /c $(CFLAGS) ../include/windowscursor.c

# compile the source main client file into object file
main.obj:
	$(CC) /c $(CFLAGS) main.c

# clean prior object files
clean:
	-@del $(BIN_NAME)
	-@del *.obj

# clear folder
clear:
	cls
