REM Get shared libs
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/shared-libs.tar.gz" | tar xzf -
if not exist desktop\build32 mkdir desktop\build32
if not exist desktop\build64 mkdir desktop\build64

xcopy /e /Y /c share\64\Windows\* desktop\build64\Windows\
xcopy /e /Y /c share\64\Darwin\* desktop\build64\Darwin\
xcopy /e /Y /c share\64\Windows\* server\build64\
