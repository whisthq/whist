REM This script downloads pre-built Fractal protocol libraries from AWS S3, on Windows
ECHO "Downloading Protocol Libraries"

REM ###############################
REM # Create Directory Structure
REM ###############################

if not exist desktop\build32 mkdir desktop\build32
if not exist desktop\build64 mkdir desktop\build64

REM ###############################
REM # Download Protocol Shared Libs
REM ###############################

aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/shared-libs.tar.gz - | tar xzf -

REM Copy files to the right location
xcopy /e /Y /c share\64\Windows\* desktop\build64\Windows\
xcopy /e /Y /c share\64\Darwin\* desktop\build64\Darwin\
xcopy /e /Y /c share\64\Windows\* server\build64\

REM ###############################
REM # Download SDL2 headers
REM ###############################

mkdir include\SDL2
aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/fractal-sdl2-headers.tar.gz - | tar xzf - -C include\SDL2

REM Pull all SDL2 include files up a level and delete encapsulating folder
move include\SDL2\include\* include\SDL2
rmdir include\SDL2\include

REM ###############################
REM # Download SDL2 libraries
REM ###############################

REM macOS
mkdir lib\64\SDL2
mkdir lib\64\SDL2\Darwin
aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/fractal-macos-sdl2-static-lib.tar.gz - | tar xzf - -C lib\64\SDL2\Darwin

REM Linux Ubuntu
mkdir lib\64\SDL2\Linux
aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/fractal-linux-sdl2-static-lib.tar.gz - | tar xzf - -C lib\64\SDL2\Linux

REM Windows
mkdir lib\64\SDL2\Windows
aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/fractal-windows-sdl2-static-lib.tar.gz - | tar xzf - -C lib\64\SDL2\Windows

REM ###############################
REM # Download OpenSSL libraries
REM ###############################

REM Emscripten
REM mkdir lib\64\openssl
REM mkdir lib\64\openssl\Emscripten
REM aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/fractal-emscripten-libcrypto.tar.gz - | tar xzf - -C lib\64\openssl\Emscripten

REM ###############################
ECHO "Download Completed"
