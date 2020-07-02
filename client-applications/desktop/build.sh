rm -rf .protocol
git clone --depth 1 https://github.com/fractalcomputers/protocol .protocol
cd .protocol
git reset --hard
git fetch --depth 25 origin $1:$1
git checkout $1
cmake .
make FractalClient
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
sudo chmod 600 protocol/desktop/sshkey # was previously in protocol, but operation not permitted from protocol
if [[ "$OSTYPE" == "linux-gnu" ]]; then
  # Linux Ubuntu
  # copy over the Unison executable and make executable files executable
  cp .protocol/desktop/build64/linux_unison protocol/desktop
  sudo chmod +x protocol/desktop/FractalClient
  sudo chmod +x protocol/desktop/linux_unison
elif [[ "$OSTYPE" == "darwin"* ]]; then
  # Mac OSX
  # add logo to the FractalClient executable
  sips -i build/icon.png # take an image and make the image its own icon
  DeRez -only icns build/icon.png > tmpicns.rsrc # extract the icon to its own resource file
  Rez -append tmpicns.rsrc -o protocol/desktop/FractalClient # append this resource to the file you want to icon-ize
  SetFile -a C protocol/desktop/FractalClient # use the resource to set the icon
  rm tmpicns.rsrc # clean up
  # copy over the Unison executable and FFmpeg dylibs
  cp -R .protocol/lib/64/ffmpeg/Darwin protocol-build/desktop/

  # codesign the FractalClient executable
  codesign -s "Fractal Computers, Inc." protocol/desktop/FractalClient
fi
yarn -i
yarn package