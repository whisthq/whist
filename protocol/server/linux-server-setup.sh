#!/bin/bash
sudo apt update
sudo apt install libavcodec-dev libavdevice-dev libx11-dev libxtst-dev libxdamage-dev libasound2-dev xclip x11-xserver-utils -y
sudo rm /etc/udev/rules.d/90-fractal-input.rules
sudo ln -s $(pwd)/fractal-input.rules /etc/udev/rules.d/90-fractal-input.rules

