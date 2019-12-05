BIN_NAME = client.exe
OBJS = main.obj ../include/webserver.obj

STATIC_LIBS = \
	lib/avcodec.lib \
	lib/avdevice.lib \
	lib/avfilter.lib \
	lib/avformat.lib \
	lib/avutil.lib \
	lib/postproc.lib \
	lib/swresample.lib \
	lib/swscale.lib \
	lib/windows/SDL2.lib

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

LD_FLAGS = \
	/nodefaultlib \
	/nologo

all: clean clear $(OBJS)
	link $(LD_FLAGS) $(STATIC_LIBS) $(SHARED_LIBS) /out:$(BIN_NAME) *.obj

clean:
	-@del $(BIN_NAME)
	-@del *.obj

clear:
	cls
