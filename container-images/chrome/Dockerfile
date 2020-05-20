FROM ubuntu:18.04

# general container preparation and installation of Chrome
RUN apt-get update && apt-get clean && apt-get install -y \
    wget \
    python \
    python3 \
    && wget -q -O - https://dl-ssl.google.com/linux/linux_signing_key.pub | apt-key add - \
    && echo "deb http://dl.google.com/linux/chrome/deb/ stable main" >> /etc/apt/sources.list.d/google.list \
    && apt-get update && apt-get -y install google-chrome-stable

# fetch our bash script utils
RUN wget "utils.sh" "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/utils.sh" \
    source ./utils.sh

# install virtual display 
RUN Install-VirtualDisplay && reboot

# install all required components for Fractal to perform well
RUN Update-Linux \
    Set-Time \
    Install-7Zip \
    Install-Curl \
    Install-CustomGDMConfiguration \
    Install-CustomX11Configuration \
    Set-FractalDirectory \
    Install-FractalServer \
    Install-ProcessManager \
    Install-FractalExitScript \
    Install-FractalAutoUpdate \
    Install-FractalLinuxInputDriver \
    Install-FractalWallpaper \
    Enable-FractalFirewallRule \
    Install-Unison \
    Enable-SSHKey \
    Disable-Shutdown \
    Add-AutoLogin

# create user
RUN useradd fractal \
    && mkdir -p /home/fractal \
    && chown -v -R fractal:fractal /home/fractal

# clean up and reboot for use
RUN sudo rm -f "utils.sh" && reboot
