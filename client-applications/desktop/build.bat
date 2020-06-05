<<<<<<< HEAD

echo "Updating protocol submodule to latest master"
git submodule update --remote
cd protocol
=======
rmdir /S/Q .protocol
git clone --depth 1 https://github.com/fractalcomputers/protocol .protocol
cd .protocol
git reset --hard
git fetch --depth 25 origin %1:%1
git checkout %1
>>>>>>> master
cmake -G "NMake Makefiles"
nmake FractalClient
cd ..
copy protocol\desktop\build64\FractalClient.exe protocol\desktop\FractalClient.exe
copy protocol\desktop\build64\unison.exe protocol\desktop\unison.exe
copy protocol\desktop\build64\ssh.exe protocol\desktop\ssh.exe
copy protocol\desktop\build64\sshkey protocol\desktop\sshkey
copy protocol\desktop\build64\sshkey.pub protocol\desktop\sshkey.pub
copy protocol\desktop\build64\avcodec-58.dll protocol\desktop\avcodec-58.dll
copy protocol\desktop\build64\avdevice-58.dll protocol\desktop\avdevice-58.dll
copy protocol\desktop\build64\avfilter-7.dll protocol\desktop\avfilter-7.dll
copy protocol\desktop\build64\avformat-58.dll protocol\desktop\avformat-58.dll
copy protocol\desktop\build64\avutil-56.dll protocol\desktop\avutil-56.dll
copy protocol\desktop\build64\postproc-55.dll protocol\desktop\postproc-55.dll
copy protocol\desktop\build64\swresample-3.dll protocol\desktop\swresample-3.dll
copy protocol\desktop\build64\swscale-5.dll protocol\desktop\swscale-5.dll
copy loading protocol\desktop\loading
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://github.com/electron/rcedit/releases/download/v1.1.1/rcedit-x64.exe', 'rcedit-x64.exe')"
powershell -Command "Invoke-WebRequest https://github.com/electron/rcedit/releases/download/v1.1.1/rcedit-x64.exe -OutFile rcedit-x64.exe"
rcedit-x64.exe protocol\desktop\FractalClient.exe --set-icon build\icon.ico
del /Q rcedit-x64.exe
yarn package
