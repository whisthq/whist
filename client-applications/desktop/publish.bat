git clone --depth 1 https://github.com/fractalcomputers/protocol .protocol
cd .protocol
git reset --hard
git fetch --depth 25 origin dev:dev
git checkout dev
git checkout %1
cd desktop
nmake
cd ..
cd ..
rmdir /S/Q protocol
mkdir protocol
cd protocol
mkdir desktop
mkdir desktop\loading
cd ..
copy .protocol\desktop\build64\FractalClient.exe protocol\desktop\FractalClient.exe
copy .protocol\desktop\build64\unison.exe protocol\desktop\unison.exe
copy .protocol\desktop\build64\ssh.exe protocol\desktop\ssh.exe
copy .protocol\desktop\build64\sshkey protocol\desktop\sshkey
copy .protocol\desktop\build64\sshkey.pub protocol\desktop\sshkey.pub
copy .protocol\desktop\build64\avcodec-58.dll protocol\desktop\avcodec-58.dll
copy .protocol\desktop\build64\avdevice-58.dll protocol\desktop\avdevice-58.dll
copy .protocol\desktop\build64\avfilter-7.dll protocol\desktop\avfilter-7.dll
copy .protocol\desktop\build64\avformat-58.dll protocol\desktop\avformat-58.dll
copy .protocol\desktop\build64\avutil-56.dll protocol\desktop\avutil-56.dll
copy .protocol\desktop\build64\postproc-55.dll protocol\desktop\postproc-55.dll
copy .protocol\desktop\build64\swresample-3.dll protocol\desktop\swresample-3.dll
copy .protocol\desktop\build64\swscale-5.dll protocol\desktop\swscale-5.dll
copy loading protocol\desktop\loading
rcedit-x64.exe protocol\desktop\FractalClient.exe --set-icon build\icon.ico
yarn package-ci
