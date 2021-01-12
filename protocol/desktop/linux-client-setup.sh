#!/bin/bash

sudo apt-get install libssl-dev libavcodec-dev libavdevice-dev libx11-dev libxtst-dev xclip x11-xserver-utils libasound2-dev libxdamage-dev libcurl4-openssl-dev zlib1g-dev -y

# install aws cli
sudo apt-get install curl unzip -y
curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip"
unzip awscliv2.zip
sudo ./aws/install
sudo apt-get remove curl unzip -y
