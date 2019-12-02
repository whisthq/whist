BIN_NAME = server.exe
OBJS = main.obj

CFLAGS = \
	/nologo \
	/W4 \
	/O2 \
	/MT \
	/MP \
	-DWIN32_LEAN_AND_MEAN \
	-DUNICODE \
	-DWIN32

SHARED_LIBS = \
	libvcruntime.lib \
	libucrt.lib \
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
	version.lib

LD_FLAGS = \
	/nodefaultlib \
	/nologo

all: clean clear $(OBJS)
	link $(LD_FLAGS) /out:$(BIN_NAME) *.obj $(SHARED_LIBS)

clean:
	-@del $(BIN_NAME)
	-@del *.obj

clear:
	cls
