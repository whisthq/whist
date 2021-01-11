REM Get shared libs
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/shared-libs.tar.gz" | tar xzf -
if not exist desktop\build32 mkdir desktop\build32
if not exist desktop\build64 mkdir desktop\build64

xcopy /e /Y /c share\64\Windows\* desktop\build64\Windows\
xcopy /e /Y /c share\64\Darwin\* desktop\build64\Darwin\
xcopy /e /Y /c share\64\Windows\* server\build64\

REM Get SDL2 includes
mkdir include\SDL2
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/fractal-sdl2-headers.tar.gz" | tar xzf - -C include\SDL2

REM Pull all SDL2 include files up a level and delete encapsulating folder
move include\SDL2\include\* include\SDL2
rmdir include\SDL2\include

REM Get SDL2 shared libraries
mkdir lib\64\SDL2
mkdir lib\64\SDL2\Darwin
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/fractal-macos-sdl2-static-lib.tar.gz" | tar xzf - -C lib\64\SDL2\Darwin
mkdir lib\64\SDL2\Linux
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/fractal-linux-sdl2-static-lib.tar.gz" | tar xzf - -C lib\64\SDL2\Linux
mkdir lib\64\SDL2\Windows
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/fractal-windows-sdl2-static-lib.tar.gz" | tar xzf - -C lib\64\SDL2\Windows