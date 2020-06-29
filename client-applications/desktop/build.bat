rmdir /S/Q .protocol
git clone --depth 1 https://github.com/fractalcomputers/protocol .protocol
cd .protocol
git reset --hard
git fetch --depth 25 origin %1:%1
git checkout %1
cmake -G "NMake Makefiles"
nmake FractalClient
cd ..
rmdir /S/Q protocol
mkdir protocol
cd protocol
mkdir desktop
mkdir desktop\loading
cd ..
xcopy /s .protocol\desktop\build64\Windows protocol\desktop
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://github.com/electron/rcedit/releases/download/v1.1.1/rcedit-x64.exe', 'rcedit-x64.exe')"
powershell -Command "Invoke-WebRequest https://github.com/electron/rcedit/releases/download/v1.1.1/rcedit-x64.exe -OutFile rcedit-x64.exe"
rcedit-x64.exe protocol\desktop\FractalClient.exe --set-icon build\icon.ico
del /Q rcedit-x64.exe
yarn package