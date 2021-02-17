REM This script downloads pre-built Fractal protocol libraries from AWS S3, on Windows
ECHO "Downloading Protocol Libraries"

REM ###############################
REM # Create Directory Structure
REM ###############################

if not exist desktop\build32 MKDIR desktop\build32
if not exist desktop\build64 MKDIR desktop\build64

REM ###############################
REM # Download Protocol Shared Libs
REM ###############################

aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/shared-libs.tar.gz - | tar xzf - || exit /b

REM Copy files to the right location
xcopy /e /Y /c share\64\Windows\* desktop\build64\Windows\
xcopy /e /Y /c share\64\Windows\* server\build64\

REM ###############################
REM # Download SDL2 headers
REM ###############################

if not exist include\SDL2 MKDIR include\SDL2
aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/fractal-sdl2-headers.tar.gz - | tar xzf - -C include\SDL2 || exit /b

REM Pull all SDL2 include files up a level and delete encapsulating folder
MOVE include\SDL2\include\* include\SDL2
RMDIR include\SDL2\include

REM ###############################
REM # Download SDL2 libraries
REM ###############################

REM Windows
if not exist lib\64\SDL2\Windows MKDIR lib\64\SDL2\Windows
aws s3 cp --only-show-errors s3://fractal-protocol-shared-libs/fractal-windows-sdl2-static-lib.tar.gz - | tar xzf - -C lib\64\SDL2\Windows || exit /b

REM ###############################
ECHO "Download Completed"
