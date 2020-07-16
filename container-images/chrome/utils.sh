#!/bin/bash

# This file contains the functions called in the Bash scripts
# You need to source this file via source ./utils.sh

function Update-Linux {
    echo "Updating Linux Ubuntu"
    sudo apt-get -y update
    sudo apt-get -y upgrade
    sudo apt-get -y autoremove
}

function Set-Time {
    echo "Setting Automatic Time & Timezone via Gnome Clocks"
    sudo apt-get -y install gnome-clocks
}

function Add-AutoLogin {
    echo "Setting Automatic Login on Linux by replacing /etc/gdm3/custom.conf File"
    # this action is done by Install-CustomGDMConfiguration
}

function Disable-AutomaticLockScreen {
    echo "Disabling Automatic Lock Screen"
    gsettings set org.gnome.desktop.lockdown disable-lock-screen 'true'
}

function Enable-FractalFirewallRule {
    echo "Creating Fractal Firewall Rules"
    yes | sudo ufw enable
    yes | sudo ufw allow 22 # SSH
    yes | sudo ufw allow 80 # HTTP
    yes | sudo ufw allow 443 # HTTPS
    yes | sudo ufw allow 32262 # Fractal Port Client->Server
    yes | sudo ufw allow 32263 # Fractal Port Server->Client
    yes | sudo ufw allow 32264 # Fractal Port Shared-TCP
}

function Install-VirtualDisplay-NoGnome {
    echo "Installing Virtual Display Dummy"
    sudo apt-get -y install xserver-xorg-video-dummy
}

function Install-VirtualDisplay {
    echo "Installing Gnome and Virtual Display Dummy"
    sudo apt-get -y install gnome xserver-xorg-video-dummy ubuntu-gnome-desktop
}

function Install-CustomGDMConfiguration {
   echo "Installing Custom Gnome Display Manager Configuration"
   sudo wget -qO /etc/gdm3/custom.conf "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/custom.conf"
}

function Install-CustomX11Configuration {
   echo "Installing Custom X11 Configuration"
   sudo wget -qO /usr/share/X11/xorg.conf.d/01-dummy.conf "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/01-dummy.conf"
}

function Install-FractalLinuxInputDriver {
    # first download the driver symlink file
    echo "Installing Fractal Linux Input Driver"
    sudo wget -qO /usr/share/fractal/fractal-input.rules "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/fractal-input.rules"

    # symlink file
    # do this as root AFTER fractal-input.rules has been moved to the final install directory
    sudo groupadd fractal || echo "fractal group already existed"
    sudo usermod -aG fractal fractal || echo "fractal already in fractal group" # (the -a is VERY important, else you overwrite a user's groups)
    sudo ln -sf /usr/share/fractal/fractal-input.rules /etc/udev/rules.d/90-fractal-input.rules
}

function Disable-Shutdown {
    # based on: https://askubuntu.com/questions/453479/how-to-disable-shutdown-reboot-from-lightdm-in-14-04
    echo "Disabling Shutdown Option in Start Menu"
    sudo wget -qO /etc/polkit-1/localauthority/50-local.d/restrict-login-powermgmt.pkla "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/restrict-login-powermgmt.pkla"
}

function Install-AutodeskMaya {
    echo "Installing Autodesk Maya"
    # Create Download Directory
    mkdir -p maya2017Install
    cd maya2017Install

    # Download Maya Install Files
    wget http://edutrial.autodesk.com/NET17SWDLD/2017/MAYA/ESD/Autodesk_Maya_2017_EN_JP_ZH_Linux_64bit.tgz
    sudo tar xvf Autodesk_Maya_2017_EN_JP_ZH_Linux_64bit.tgz

    # Install Dependencies
    sudo apt-get install -y libssl1.0.0 gcc  libssl-dev libjpeg62 \
      alien csh tcsh libaudiofile-dev libglw1-mesa elfutils libglw1-mesa-dev \
      mesa-utils xfstt xfonts-100dpi xfonts-75dpi ttf-mscorefonts-installer \
      libfam0 libfam-dev libcurl4-openssl-dev libtbb-dev
    sudo apt-get install -y libtbb-dev 
    sudo wget -q http://launchpadlibrarian.net/183708483/libxp6_1.0.2-2_amd64.deb
    sudo wget -q http://mirrors.kernel.org/ubuntu/pool/main/libp/libpng/libpng12-0_1.2.54-1ubuntu1_amd64.deb

    # Install Maya
    sudo alien -cv *.rpm
    sudo dpkg -i *.deb
    echo "int main (void) {return 0;}" > mayainstall.c
    sudo gcc mayainstall.c
    sudo mv /usr/bin/rpm /usr/bin/rpm_backup
    sudo cp a.out /usr/bin/rpm
    sudo chmod +x ./setup
    sudo ./setup --noui
    sudo mv /usr/bin/rpm_backup /usr/bin/rpm
    sudo rm /usr/bin/rpm

    # Copy lib*.so
    sudo cp libQt* /usr/autodesk/maya2017/lib/
    sudo cp libadlm* /usr/lib/x86_64-linux-gnu/

    # Fix Startup Errors
    sudo ln -s /usr/lib/x86_64-linux-gnu/libtiff.so.5.3.0 /usr/lib/libtiff.so.3
    sudo ln -s /usr/lib/x86_64-linux-gnu/libssl.so.1.0.0 /usr/autodesk/maya2017/lib/libssl.so.10
    sudo ln -s /usr/lib/x86_64-linux-gnu/libcrypto.so /usr/autodesk/maya2017/lib/libcrypto.so.10
    sudo ln -s /usr/lib/x86_64-linux-gnu/libtbb.so.2 /usr/lib/x86_64-linux-gnu/libtbb_preview.so.2
    sudo mkdir -p /usr/tmp
    sudo chmod 777 /usr/tmp
    sudo mkdir -p ~/maya/2017/
    sudo chmod 777 ~/maya/2017/

    # Fix Segmentation Fault Error
    echo "MAYA_DISABLE_CIP=1" >> ~/maya/2017/Maya.env

    # Fix Color Managment Errors
    echo "LC_ALL=C" >> ~/maya/2017/Maya.env
    sudo chmod 777 ~/maya/2017/Maya.env

    # Maya Camera Modifier Key
    sudo gsettings set org.gnome.desktop.wm.preferences mouse-button-modifier "<Super>"

    # Ensure that Fonts are Loaded
    sudo xset +fp /usr/share/fonts/X11/100dpi/
    sudo xset +fp /usr/share/fonts/X11/75dpi/
    sudo xset fp rehash

    # Cleanup
    echo "Maya was installed successfully"
    cd ..
    sudo rm -rf maya2017Install
    cd ~
}

function Install-FractalExitScript {
    # download exit Bash script
    echo "Downloading Fractal Exit script"
    sudo wget -qO /usr/share/fractal/exit/exit.sh "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/exit.sh"

    # download the Fractal logo for the icons
    echo "Downloading Fractal Logo icon"
    sudo wget -qO /usr/share/fractal/assets/logo.png "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/logo.png"

    # download Exit Fractal Desktop Shortcut
    echo "Creating Exit-Fractal Desktop Shortcut"
    sudo wget -qO "$HOME/Exit-Fractal.desktop" "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/Exit-Fractal.desktop"
    sudo mkdir -p "$HOME/.local/share/applications/"
    sudo mv "$HOME/Exit-Fractal.desktop" "$HOME/.local/share/applications/Exit-Fractal.desktop"

    # create Ubuntu dock shortcut
    echo "Creating Exit-Fractal Favorites Bar Shortcut"
    gsettings set org.gnome.shell favorite-apps "$(gsettings get org.gnome.shell favorite-apps | sed s/.$//), 'Exit-Fractal.desktop']"
}

function Install-FractalAutoUpdate {
    # no need to download version, update.sh will download it
    echo "Downloading Fractal Auto-update Script"
    ls -al /usr/share/fractal 

    echo "Downloading update.sh from https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/$1/Linux/update.sh"
    sudo wget -qO /usr/share/fractal/update.sh "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/$1/Linux/update.sh"
}

function Install-NvidiaTeslaPublicDrivers {
    echo "Installing Nvidia Public Driver (GRID already installed at deployment through Azure)"

    echo "Checking for Ubuntu 18.04 vs Ubuntu 20.04..."
    release=$(lsb_release -r)
    release_num=$(cut -f2 <<< $release)
    echo "Found Ubuntu $release_num; Installing approriate Nvidia Drivers"

    # define proper variables
    if [ $release_num == "18.04" ]; then
        keypath="http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/7fa2af80.pub"
        downloadpath="http://us.download.nvidia.com/tesla/440.64.00/nvidia-driver-local-repo-ubuntu1804-440.64.00_1.0-1_amd64.deb"
        filepath="nvidia-driver-local-repo-ubuntu1804-440.64.00_1.0-1_amd64.deb"
    elif [ $release_num == "20.04" ]; then
        # Ubuntu 18 and 20 drivers are the same for now!
        keypath="http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/7fa2af80.pub"
        downloadpath="http://us.download.nvidia.com/tesla/440.64.00/nvidia-driver-local-repo-ubuntu1804-440.64.00_1.0-1_amd64.deb"
        filepath="nvidia-driver-local-repo-ubuntu1804-440.64.00_1.0-1_amd64.deb"
    else
        echo "Running neither Ubuntu 18.04 nor 20.04, no Nvidia driver installed"
        return
    fi

    echo "Installing Nvidia CUDA GPG Key"
    sudo apt-key adv --fetch-keys $keypath

    echo "Downloading Nvidia M60 Driver from Nvidia Website"
    sudo wget -q $downloadpath

    echo "Installing Nvidia M60 Driver"
    sudo dpkg -i $filepath

    echo "Cleaning up Nvidia Public Drivers Installation"
    sudo rm -f $filepath
}

function Set-OptimalGPUSettings {
    echo "Setting Optimal Tesla M60 GPU Settings"
    if ! [ -x "$(command -v nvidia-smi)"]; then
        sudo nvidia-smi --auto-boost-default=0
        sudo nvidia-smi -ac "2505,1177"
    else
        echo "Nvidia-smi does not exist, skipping Set-OptimalGPUSettings"
    fi
}

function Disable-TCC {
    echo "Disabling TCC Mode on Nvidia Tesla GPU"
    if ! [ -x "$(command -v nvidia-smi)"]; then
        sudo nvidia-smi -g 0 -fdm 0 
    else
        echo "Nvidia-smi does not exist, skipping Disable-TCC"
    fi
}

function Install-FractalService {
    echo "Installing Fractal Service"
    sudo wget -qO /etc/systemd/user/fractal.service "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/$1/Linux/fractal.service"
    sudo chmod +x /etc/systemd/user/fractal.service

    env
    if [[ $FRACTAL_GITHUB_ACTION = "yes" ]]; then
        echo "Skipping Enable-FractalService for Github Action"
        return
    fi

    # start a pam systemd process for the user if not started
    # https://unix.stackexchange.com/questions/423632/systemctl-user-not-available-for-www-data-user
    echo "Start Pam Systemd Process for User fractal"
    export FRACTAL_UID=`id -u fractal`
    sudo touch /etc/default/locale
    sudo install -d -o fractal /run/user/$FRACTAL_UID
    sudo systemctl start user@$FRACTAL_UID

    echo "Enabling Fractal Service"
    sudo -u fractal DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$FRACTAL_UID/bus systemctl --user enable fractal

    echo "Starting Fractal Service"
    sudo -u fractal DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$FRACTAL_UID/bus systemctl --user start fractal

    echo "Finished Starting Fractal Service"
}

function Install-FractalServer {
    # only download, the FractalServer will get started by the Fractal Service
    echo "Downloading Fractal Server"
    sudo wget -qO /usr/share/fractal/FractalServer "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/$1/Linux/FractalServer"
    sudo wget -qO /usr/share/fractal/FractalServer.sh "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/$1/Linux/FractalServer.sh"

    sudo chgrp fractal -R /usr/share/fractal
    sudo chmod g+rw -R /usr/share/fractal
    sudo chmod g+x /usr/share/fractal/FractalServer # make FractalServer executable
    sudo chmod g+x /usr/share/fractal/FractalServer.sh # make FractalServer executable

    # download the libraries
    echo "Downloading FFmpeg Dependencies"
    sudo apt-get install libx11-dev libxtst-dev libxdamage-dev libasound2-dev xclip -y

    # libraries to install FFmpeg system libs -- we use our own now
    # sudo apt-get install libavcodec-dev libavdevice-dev

    # our own Fractal FFmpeg Linux build, to put in the same folder as the FractalServer executable
    echo "Downloading Fractal FFmpeg .so from AWS S3 and Moving to /usr/share/fractal/"
    sudo wget -qO shared-libs.tar.gz "https://fractal-protocol-shared-libs.s3.amazonaws.com/shared-libs.tar.gz"
    tar -xvf shared-libs.tar.gz
    sudo mv share/64/Linux/* /usr/share/fractal/ # sudo required to avoid "permission denied" error

    echo "Cleaning Downloaded Tar"
    rm -rf shared-libs.tar.gz
    rm -rf share 
}

function Install-7Zip {
    echo "Installing 7Zip through Apt"
    sudo apt-get -y install p7zip-full
}

function Set-FractalDirectory {
    echo "Creating Fractal Directory in /usr/share"
    echo "Creating Fractal Asset Subdirectory in /usr/share/fractal"
    echo "Creating Fractal Exit Subdirectory in /usr/share/fractal"
    echo "Creating Fractal Sync Subdirectory in /usr/share/fractal"
    sudo mkdir -p /usr/share/fractal/assets /usr/share/fractal/exit /usr/share/fractal/sync
}

function Install-Unison {
    echo "Downloading Unison File Sync from S3 Bucket"
    sudo wget -qO /usr/share/fractal/linux_unison "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/linux_unison"
    sudo ln -s /usr/share/fractal/linux_unison /usr/bin/unison
}

function Enable-SSHKey {
    # NOTE: needed for later, when we update webserver to exchange SSH keys
    # echo "Generating SSH Key"
    # yes | ssh-keygen -f sshkey -q -N """"
    # cp sshkey.pub "$HOME/.ssh/authorized_keys"
    echo "Download SSH Administrator Authorized Key"
    if [[ $LOCAL = "yes" ]]; then
        echo "Skipping Enable-SSHKey LOCAL flag set"
        return
    fi

    sudo wget -qO /tmp/administrator_authorized_keys "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/administrators_authorized_keys"
    sudo cp /tmp/administrator_authorized_keys "$HOME/.ssh/authorized_keys"
    sudo chmod 600 "$HOME/.ssh/authorized_keys" # activate

    echo "Downloading SSH ECDSA Keys"
    sudo wget -qO /tmp/ssh_host_ecdsa_key "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/ssh_host_ecdsa_key"
    sudo wget -qO /tmp/ssh_host_ecdsa_key.pub "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/ssh_host_ecdsa_key.pub"
    sudo cp /tmp/ssh_host_ecdsa_key "/etc/ssh/ssh_host_ecdsa_key"
    sudo cp /tmp/ssh_host_ecdsa_key.pub "/etc/ssh/ssh_host_ecdsa_key.pub"
    sudo chmod 600 "/etc/ssh/ssh_host_ecdsa_key" # activate
    sudo chmod 600 "/etc/ssh/ssh_host_ecdsa_key.pub" # activate
}

function Install-FractalWallpaper {
    # first download the wallpaper
    echo "Downloading Fractal Wallpaper"
    sudo wget -qO /usr/share/fractal/assets/wallpaper.png "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/wallpaper.png"

    # then set the wallpaper
    echo "Setting Fractal Wallpaper"
    gsettings set org.gnome.desktop.background picture-uri /usr/share/fractal/assets/wallpaper.png
}

function Install-AdobeAcrobat {
    echo "Installing Adobe Acrobat Reader"
    sudo apt-get -y install gdebi-core libxml2:i386 libcanberra-gtk-module:i386 gtk2-engines-murrine:i386 libatk-adaptor:i386
    sudo wget -q "ftp://ftp.adobe.com/pub/adobe/reader/unix/9.x/9.5.5/enu/AdbeRdr9.5.5-1_i386linux_enu.deb"
    sudo gdebi "AdbeRdr9.5.5-1_i386linux_enu.deb"
    sudo rm -f "AdbeRdr9.5.5-1_i386linux_enu.deb"
}

function Install-VSCode {
    echo "Installing VSCode through Snap"
    sudo snap install vscode --classic
}

function Install-Docker {
    echo "Installing Docker through Snap"
    sudo snap install docker --classic
}

function Install-Blender {
    echo "Installing Blender through Snap"
    sudo snap install blender --classic
}

function Install-Cmake {
    echo "Installing Cmake through Apt"
    sudo apt-get -y install apt-transport-https ca-certificates gnupg software-properties-common -y
    sudo wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
    sudo apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ bionic main'
    sudo apt-get -y update
    sudo apt-get -y install cmake
}

function Install-Cppcheck {
    echo "Installing Cppcheck through Apt"
    sudo apt-get -y install cppcheck
}

function Install-Clangformat {
    echo "Installing Clang-format through Apt"
    sudo apt-get -y install clang-format
}

function Install-CUDAToolkit {
    echo "Install CUDA Not Implemented on Linux; very involved"
}

function Install-Git {
    echo "Installing Git through Apt"
    sudo apt-get -y install git
}

function Install-Atom {
    echo "Installing Atom through Snap"
    sudo snap install atom --classic
}

function Install-Slack {
    echo "Installing Slack through Snap"
    sudo snap install slack --classic
}

function Install-OpenCV {
    echo "Installing OpenCV through Apt"
    sudo apt-get -y install python3-opencv
}

function Install-GoogleChrome {
    echo "Installing Google Chrome"
    sudo wget -q "https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb"
    sudo apt-get -y install ./google-chrome-stable_current_amd64.deb
    sudo rm -f "google-chrome-stable_current_amd64.deb"
}

function Install-Firefox {
    echo "Installing Firefox through Apt"
    sudo apt-get -y install firefox
}

function Install-Dropbox {
    echo "Installing Dropbox"
    cd ~ && wget -O - "https://www.dropbox.com/download?plat=lnx.x86_64" | tar xzf -
    ~/.dropbox-dist/dropboxd
}

function Install-VLC {
    echo "Installing VLC through Apt"
    sudo apt-get -y install vlc
}

function Install-Gimp {
    echo "Installing Gimp through Apt"
    sudo apt-get -y install gimp
}

function Install-NodeJS {
    echo "Installing NodeJS through Apt"
    sudo apt-get -y install nodejs
}

function Install-AndroidStudio {
    echo "Installing Android Studio through Snap"
    sudo snap install android-studio --classic
}

function Install-GitHubDesktop {
    echo "GitHub Desktop does not run on Linux Ubuntu and thus cannot be installed"
}

function Install-Python2 {
    echo "Installing Python 2 through Apt"
    sudo apt-get -y install python2
}

function Install-Python3 {
    echo "Installing Python 3.6 through Apt"
    sudo apt-get -y install python3.6
}

function Install-MinGW {
    echo "MinGW does not run on Linux Ubuntu and thus cannot be installed"
}

function Install-GDB {
    echo "Installing GDB through Apt"
    sudo apt-get -y install libc6-dbg gdb
}

function Install-Valgrind {
    echo "Installing Valgrind through Apt"
    sudo apt-get -y install valgrind
}

function Install-Unity {
    echo "Installing Unity as AppImage"
    sudo wget -qO $HOME/Documents/UnityHub.AppImage https://public-cdn.cloud.unity3d.com/hub/prod/UnityHub.AppImage
    sudo chmod +x $HOME/Documents/UnityHub.AppImage
}

function Install-SublimeText {
    echo "Installing Sublime Text 3 through Apt"
    sudo wget -qO - https://download.sublimetext.com/sublimehq-pub.gpg | sudo apt-key add -
    sudo apt-get -y install apt-transport-https
    echo "deb https://download.sublimetext.com/ apt/stable/" | sudo tee /etc/apt/sources.list.d/sublime-text.list
    sudo apt-get -y update
    sudo apt-get -y install sublime-text
}

function Install-Telegram {
    echo "Installing Telegram through Snap"
    sudo snap install telegram-desktop --classic
}

function Install-WhatsApp {
    echo "WhatsApp does not run on Linux Ubuntu and thus cannot be installed"    
}

function Install-Curl {
    echo "Installing Curl through Apt"
    sudo apt-get -y install curl
}

function Install-Make {
    echo "Installing Make through Apt"
    sudo apt-get -y install make
}

function Install-GCC {
    echo "Installing GCC through Apt"
    sudo apt-get -y install gcc
}

function Install-Zoom {
    echo "Installing Zoom"
    sudo wget -q "https://zoom.us/client/latest/zoom_amd64.deb"
    sudo apt-get -y install "./zoom_amd64.deb"
    sudo rm -f "./zoom_amd64.deb"
}

function Install-Skype {
    echo "Installing Skype through Snap"
    sudo snap install skype --classic
}

function Install-Anaconda {
    echo "Installing Anaconda"
    cd /tmp
    sudo wget -q "https://repo.anaconda.com/archive/Anaconda3-2019.03-Linux-x86_64.sh"
    sudo sha256sum Anaconda3-2019.03-Linux-x86_64.sh
    sudo bash Anaconda3-2019.03-Linux-x86_64.sh -b
    source ~/.bashrc
    export PATH=~/anaconda3/bin:$PATH

    echo "Cleaning up Anaconda Install"
    sudo rm -f "Anaconda3-2019.03-Linux-x86_64.sh"
    cd ~
}

function Install-Spotify {
    echo "Installing Spotify through Snap"
    sudo snap install spotify --classic
}

function Install-Discord {
    echo "Installing Discord through Snap"
    sudo snap install discord --classic
}

function Install-Steam {
    echo "Installing Steam through Apt"
    sudo add-apt-repository -y multiverse
    sudo apt-get -y update
    sudo apt-get -y install steam
}

function Install-Matlab {
    echo "Matlab requires a license to be downloaded and thus cannot be installed"
}

function Install-Cinema4D {
    echo "Cinema4D does not run on Linux Ubuntu and thus cannot be installed"
}

function Install-Lightworks {
    echo "Installing Lightworks"
    sudo wget -q "https://downloads.lwks.com/v14-5/lightworks-14.5.0-amd64.deb"
    sudo dpkg -i "lightworks-14.5.0-amd64.deb"
    sudo apt-get -y -f install
    sudo dpkg -i "lightworks-14.5.0-amd64.deb"
    sudo rm -f "lightworks-14.5.0-amd64.deb"
}

function Install-VSPro2019 {
    echo "Visual Studio Professional does not run on Linux Ubuntu and thus cannot be installed"
}

function Install-ZBrush {
    echo "ZBrush does not run on Linux Ubuntu and thus cannot be installed"
}

function Install-GOG {
    echo "GOG does not run on Linux Ubuntu and thus cannot be installed"
}

function Install-Mathematica {
    echo "Mathematica requires a license to be downloaded and thus cannot be installed"
}

function Install-Blizzard {
    echo "Blizzard Battle.net does not run on Linux Ubuntu and thus cannot be installed"
}

function Install-EpicGamesLauncher {
    echo "Epic Games Launcher does not run on Linux and thus cannot be installed"
}

function Install-GeForceExperience {
    echo "GeForce Experience does not run on Linux and thus cannot be installed"
}

function Install-Solidworks {
    echo "Solidworks does not run on Linux and thus cannot be installed"
}

function Install-AdobeCreativeCloud {
    echo "Adobe Creative Cloud does not run on Linux and thus cannot be installed"
}

function Install-Office {
    echo "Microsoft Office does not run on Linux and thus cannot be installed"
}

function Install-Fusion360 {
    echo "Autodesk Fusion360 does not run on Linux Ubuntu and thus cannot be installed"
}

function Install-3DSMaxDesign {
    echo "Autodesk 3DSMaxDesign does not run on Linux Ubuntu and thus cannot be installed"
}

function Install-DaVinciResolve {
    echo "DaVinci Resolve does not run on Linux Ubuntu and thus cannot be installed"
}
