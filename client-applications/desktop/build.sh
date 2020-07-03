rm -rf .protocol
git clone --depth 1 https://github.com/fractalcomputers/protocol .protocol
cd .protocol
git reset --hard
git fetch --depth 25 origin $1:$1
git checkout $1
cmake .
make FractalClient
cd ..
rm -rf protocol-build 
mkdir protocol-build 
cd protocol-build
mkdir desktop
cd ..
cp .protocol/desktop/build64/Darwin/FractalClient protocol-build/desktop
cp -R .protocol/desktop/build64/Darwin/loading protocol-build/desktop
cp .protocol/desktop/build64/Darwin/sshkey protocol-build/desktop
cp .protocol/desktop/build64/Darwin/sshkey.pub protocol-build/desktop
sudo chmod 600 protocol-build/desktop/sshkey # was previously in protocol, but operation not permitted from protocol
if [[ "$OSTYPE" == "linux-gnu" ]]; then
  # Linux Ubuntu
  # copy over the Unison executable and make executable files executable
  cp .protocol/desktop/build64/linux_unison protocol-build/desktop
  sudo chmod +x protocol-build/desktop/FractalClient
  sudo chmod +x protocol-build/desktop/linux_unison
elif [[ "$OSTYPE" == "darwin"* ]]; then
  # Mac OSX
  # add logo to the FractalClient executable
  sips -i build/icon.png # take an image and make the image its own icon
  DeRez -only icns build/icon.png > tmpicns.rsrc # extract the icon to its own resource file
  Rez -append tmpicns.rsrc -o protocol-build/desktop/FractalClient # append this resource to the file you want to icon-ize
  SetFile -a C protocol-build/desktop/FractalClient # use the resource to set the icon
  rm tmpicns.rsrc # clean up
  # copy over the Unison executable and FFmpeg dylibs
  cp .protocol/desktop/build64/Darwin/mac_unison protocol-build/desktop
  cp .protocol/lib/64/ffmpeg/Darwin/libavcodec.58.dylib protocol-build/desktop
  cp .protocol/lib/64/ffmpeg/Darwin/libavdevice.58.dylib protocol-build/desktop
  cp .protocol/lib/64/ffmpeg/Darwin/libavfilter.7.dylib protocol-build/desktop
  cp .protocol/lib/64/ffmpeg/Darwin/libavformat.58.dylib protocol-build/desktop
  cp .protocol/lib/64/ffmpeg/Darwin/libavutil.56.dylib protocol-build/desktop
  cp .protocol/lib/64/ffmpeg/Darwin/libpostproc.55.dylib protocol-build/desktop
  cp .protocol/lib/64/ffmpeg/Darwin/libswresample.3.dylib protocol-build/desktop
  cp .protocol/lib/64/ffmpeg/Darwin/libswscale.5.dylib protocol-build/desktop
  # codesign the FractalClient executable
  codesign -s "Fractal Computers, Inc." protocol-build/desktop/FractalClient
fi
yarn -i
yarn package-ci