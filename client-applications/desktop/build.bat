git clone --depth 1 https://github.com/fractalcomputers/protocol .protocol
cd .protocol
git reset --hard
git fetch --depth 25 origin bitrate:bitrate
git checkout bitrate
git checkout %1
cd desktop
nmake
cd ..
cd ..
rmdir /S/Q protocol
mkdir protocol
cd protocol
mkdir desktop
mkdir loading
cd ..
copy .protocol\desktop\build64 protocol\desktop
copy .protocol\desktop\loading protocol\loading
yarn package
