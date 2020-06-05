#!/bin/bash
echo -e "Updating protocol submodule to latest master"
git submodule update --remote
cd protocol
echo -e "\n\n"
git --no-pager log -1 --pretty
echo -e ""

read -r -p "Please ensure that the protocol commit is the one you want to build off of [Y/n] " input
input=${input:="y"}
case $input in
    [yY][eE][sS]|[yY])
		echo "Yes"
		;;
    [nN][oO]|[nN])
		echo "No"
       		;;
    *)
	echo "Invalid input..."
	exit 1
	;;
esac

cmake .
make FractalClient
cd ..
cp -R loading protocol/desktop
echo -e "\n\nFinished makind FractalClient...\n\nPackaging...\n"
echo -e "OSTYPE=$OSTYPE"

rm protocol-build || echo "Already removed protocol-build"

if [[ "$OSTYPE" == "linux-gnu" ]]; then
  # Linux Ubuntu
  # copy over the Unison executable and make executable files executable
  sudo chmod +x protocol/desktop/build64/FractalClient
  sudo chmod +x protocol/desktop/build64/linux_unison

  # Change to linux folder after builds are separated by os
  ln -s protocol-build protocol/desktop/build64
elif [[ "$OSTYPE" == "darwin"* ]]; then
  # Mac OSX

  # macOS needs to copy .dylib s to build folder
  # Issue (https://github.com/fractalcomputers/protocol/issues/87)
  # Temporary workaround
  cp protocol/lib/64/ffmpeg/Darwin/lib* protocol/desktop/build64/

  # add logo to the FractalClient executable
  sips -i build/icon.png # take an image and make the image its own icon
  DeRez -only icns build/icon.png > tmpicns.rsrc # extract the icon to its own resource file
  Rez -append tmpicns.rsrc -o protocol/desktop/build64/FractalClient # append this resource to the file you want to icon-ize
  SetFile -a C protocol/desktop/build64/FractalClient # use the resource to set the icon
  rm tmpicns.rsrc # clean up
  
  
  # codesign the FractalClient executable
  codesign -s "Fractal Computers, Inc." protocol/desktop/build64/FractalClient

  # Change to macos folder after builds are separated by os
  ln -s protocol-build protocol/desktop/build64
fi
yarn -i

DEV=${DEV:=no}
if [ $DEV = yes ]; then
  echo -e "\n\n\nRunning local client...\n\n\n"
  sleep 3
  yarn dev
  exit
fi

PUBLISH=${PUBLISH:=no}
if [ $PUBLISH = yes ]; then
  echo -e "\n\n\nBuilding signed release and publishing...\n\n\n"
  sleep 3
  yarn package-ci
  exit
fi

RELEASE=${RELEASE:=yes}
if [ $RELEASE = yes ]; then
  echo -e "\n\n\nBuilding signed release.  Not publishing...\n\n\n"
  sleep 3
  yarn package
  exit
fi

