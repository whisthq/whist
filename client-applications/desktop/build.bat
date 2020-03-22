git clone --depth 1 https://github.com/fractalcomputers/fractal-protocol .fractal-protocol
cd .fractal-protocol
git reset --hard
git fetch --depth 25 origin dev:dev
git checkout dev
git checkout %1
cd desktop
nmake
cd ..
cd ..
rmdir /S/Q fractal-protocol
mkdir fractal-protocol
cd fractal-protocol
mkdir desktop
cd ..
copy .fractal-protocol\desktop\build64 fractal-protocol\desktop
yarn package
