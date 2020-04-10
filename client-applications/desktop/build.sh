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
cp .fractal-protocol/desktop/build64/unison fractal-protocol/desktop
cp .fractal-protocol/desktop/build64/loading_screen.bmp fractal-protocol/desktop
cp .fractal-protocol/desktop/build64/wallpaper.bmp fractal-protocol/desktop
cp .fractal-protocol/desktop/build64/sshkey fractal-protocol/desktop
cp .fractal-protocol/desktop/build64/sshkey.pub fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libavcodec.58.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libavdevice.58.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libavfilter.7.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libavformat.58.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libavutil.56.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libpostproc.55.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libswresample.3.dylib fractal-protocol/desktop
cp .fractal-protocol/lib/64/ffmpeg/macos/libswscale.5.dylib fractal-protocol/desktop
codesign -s "Fractal Computers, Inc." fractal-protocol/desktop/desktop
yarn -i
yarn upgrade
yarn package-ci
