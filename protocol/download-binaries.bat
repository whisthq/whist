REM Get shared libs
curl -s "https://fractal-dev-secrets.s3.amazonaws.com/shared-libs.tar.gz" | tar xzf -
if not exist server\build32 mkdir server\build32
if not exist server\build64 mkdir server\build64
if not exist desktop\build32 mkdir desktop\build32
if not exist desktop\build64 mkdir desktop\build64

xcopy /e /Y /c share\32\Windows\* desktop\build32\Windows\
xcopy /e /Y /c share\32\Windows\* server\build32\
xcopy /e /Y /c share\64\Windows\* desktop\build64\Windows\
xcopy /e /Y /c share\64\Darwin\* desktop\build64\Darwin\
xcopy /e /Y /c share\64\Windows\* server\build64\

REM Get loading bitmaps
curl -s "https://fractal-dev-secrets.s3.amazonaws.com/loading.tar.gz" | tar xzf -
if exist desktop\loading rmdir /s /q desktop\loading
move loading desktop
