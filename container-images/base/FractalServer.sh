#!/bin/bash
export DISPLAY=$(systemctl --user show-environment | awk -F = '/DISPLAY/{print $2}')
export XAUTHORITY=$(systemctl --user show-environment | awk -F = '/XAUTHORITY/{print $2}')

# attempt to launch the server
./FractalServer

# if error --> 2 == No such file or directory
if [ "$?" == "2" ]; then
    echo "FractalServer missing, redownloading from AWS S3"
    sudo wget -qO /usr/share/fractal/FractalServer "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/master/Linux/FractalServer"
    sudo wget -qO /usr/share/fractal/update.sh "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/master/Linux/update.sh"

    # make them executable
    sudo chmod g+x /usr/share/fractal/FractalServer
    sudo chmod g+x /usr/share/fractal/update.sh

    # attempt to launch again
    ./FractalServer
fi
