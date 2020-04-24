git clone --depth 1 https://github.com/fractalcomputers/protocol .protocol
cd .protocol
git reset --hard
git fetch --depth 25 origin dev:dev
git checkout dev
git checkout %1
cd desktop
make
cd ..
cd ..
rm -rf protocol
mkdir protocol
cd protocol
mkdir desktop
cd ..
cp .protocol/desktop/build64/FractalClient protocol/desktop
cp -R loading protocol/desktop
cp .protocol/desktop/build64/sshkey protocol/desktop
cp .protocol/desktop/build64/sshkey.pub protocol/desktop
if [[ "$OSTYPE" == "linux-gnu" ]]; then
  # Linux Ubuntu
  cp .protocol/desktop/build64/linux_unison protocol/desktop
  sudo chmod +x protocol/desktop/FractalClient # make file executable
  sudo chmod +x protocol/desktop/linux_unison # make file executable
elif [[ "$OSTYPE" == "darwin"* ]]; then
  # Mac OSX
  cp .protocol/desktop/build64/mac_unison protocol/desktop
  cp .protocol/lib/64/ffmpeg/macos/libavcodec.58.dylib protocol/desktop
  cp .protocol/lib/64/ffmpeg/macos/libavdevice.58.dylib protocol/desktop
  cp .protocol/lib/64/ffmpeg/macos/libavfilter.7.dylib protocol/desktop
  cp .protocol/lib/64/ffmpeg/macos/libavformat.58.dylib protocol/desktop
  cp .protocol/lib/64/ffmpeg/macos/libavutil.56.dylib protocol/desktop
  cp .protocol/lib/64/ffmpeg/macos/libpostproc.55.dylib protocol/desktop
  cp .protocol/lib/64/ffmpeg/macos/libswresample.3.dylib protocol/desktop
  cp .protocol/lib/64/ffmpeg/macos/libswscale.5.dylib protocol/desktop
  codesign -s "Fractal Computers, Inc." protocol/desktop/FractalClient
fi
yarn -i
yarn package-ci
