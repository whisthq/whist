
echo "Updating protocol submodule to latest master"
git submodule update --remote
cd protocol
cmake -G "NMake Makefiles"
nmake FractalClient
cd ..
rm protocol-build
REM Change link file to correct directory when builds create os folders.
mklink protocol-build protocol\desktop\build64
copy loading protocol-build/
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://github.com/electron/rcedit/releases/download/v1.1.1/rcedit-x64.exe', 'rcedit-x64.exe')"
powershell -Command "Invoke-WebRequest https://github.com/electron/rcedit/releases/download/v1.1.1/rcedit-x64.exe -OutFile rcedit-x64.exe"
rcedit-x64.exe protocol-build\FractalClient.exe --set-icon build\icon.ico
del /Q rcedit-x64.exe
yarn package
