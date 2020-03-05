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
cp .fractal-protocol/lib/ffmpeg/macos/libavcodec.58.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/ffmpeg/macos/libavdevice.58.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/ffmpeg/macos/libavfilter.7.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/ffmpeg/macos/libavformat.58.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/ffmpeg/macos/libavutil.56.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/ffmpeg/macos/libpostproc.55.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/ffmpeg/macos/libswresample.3.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/ffmpeg/macos/libswscale.5.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/openssl/macos/libcrypto.1.1.dylib fractal-protocol/desktop
codesign -s "Fractal Computers, Inc." fractal-protocol/desktop/desktop
yarn package
