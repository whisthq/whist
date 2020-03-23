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
cp .fractal-protocol/desktop/build64/desktop fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libavcodec.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libavdevice.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libavfilter.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libavformat.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libavutil.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libpostproc.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libswresample.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libswscale.dylib fractal-protocol/desktop
codesign -s "Fractal Computers, Inc." fractal-protocol/desktop
yarn -i
yarn package
