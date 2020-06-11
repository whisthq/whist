REM  Get shared libs
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/shared-libs.tar.gz" | tar xzf -
if not exist server\build32 mkdir server\build32
if not exist server\build64 mkdir server\build64
if not exist desktop\build32 mkdir desktop\build32
if not exist desktop\build64 mkdir desktop\build64

xcopy /e /Y /c share\32\Windows\* desktop\build32\Windows\
xcopy /e /Y /c share\32\Windows\* server\build32\
xcopy /e /Y /c share\64\Windows\* desktop\build64\Windows\
xcopy /e /Y /c share\64\Windows\* server\build64\

REM  Get Unison binary
curl -s -O "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/unison.exe"
if not exist desktop\external_utils\Windows mkdir desktop\external_utils\Windows
move unison.exe desktop\external_utils\Windows

REM  Get ssh.exe
curl -s -O "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/ssh.exe"
if not exist desktop\external_utils\Windows mkdir desktop\external_utils\Windows
move ssh.exe desktop\external_utils\Windows

REM  Get loading bitmaps
curl -s "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/loading.tar.gz" | tar xzf -
if exist desktop\loading rmdir /s /q desktop\loading
move loading desktop
