git clone --depth 1 https://github.com/fractalcomputers/fractal-protocol .fractal-protocol
cd .fractal-protocol
git reset --hard
git fetch --depth 25 origin dev:dev
git checkout dev
git checkout %1
cd desktop
make
cd ..
cd ..
rm -rf fractal-protocol
mkdir fractal-protocol
cd fractal-protocol
mkdir desktop
cd ..
cp .fractal-protocol/desktop/desktop fractal-protocol/desktop
codesign -s "Fractal Computers, Inc." fractal-protoco/desktop/desktop
yarn package
