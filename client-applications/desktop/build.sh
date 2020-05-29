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
cp protocol/desktop/build64/FractalClient protocol/desktop
cp -R loading protocol/desktop
cp protocol/desktop/build64/sshkey protocol/desktop
cp protocol/desktop/build64/sshkey.pub protocol/desktop
chmod 600 protocol/desktop/sshkey # was previously in protocol, but operation not permitted from protocol

echo -e "OSTYPE=$OSTYPE"
if [[ "$OSTYPE" == "linux-gnu" ]]; then
  # Linux Ubuntu
  # copy over the Unison executable and make executable files executable
  cp protocol/desktop/build64/linux_unison protocol/desktop
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
  cp protocol/desktop/build64/mac_unison protocol/desktop
  cp protocol/lib/64/ffmpeg/Darwin/* protocol/desktop
  
  # codesign the FractalClient executable
  codesign -s "Fractal Computers, Inc." protocol/desktop/FractalClient
fi
yarn -i

DEV=${DEV:=no}
if [ $DEV = yes ]; then
  echo -e "Running local client..."
  sleep 3
  yarn dev
  exit
fi

PUBLISH=${PUBLISH:=no}
if [ $PUBLISH = yes ]; then
  echo -e "Building signed release and publishing..."
  sleep 3
  yarn package-ci
  exit
fi

RELEASE=${RELEASE:=yes}
if [ $RELEASE = yes ]; then
  echo -e "Building signed release.  Not publishing..."
  sleep 3
  yarn package
  exit
fi

