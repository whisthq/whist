apt-get update && apt-get install python3-pip -y && pip3 install meson

apt-get install dbus-x11 pkg-config ninja-build build-essential libpython3-dev libdbus-1-dev libudev1 libudev-dev libglib2.0-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev bluez libbluetooth-dev libsbc-dev qjackctl pulseaudio-module-jack libjack-jackd2-dev libvulkan1 mesa-vulkan-drivers vulkan-utils libvulkan-dev rtkit -y

git clone https://gitlab.freedesktop.org/pipewire/pipewire.git && cd pipewire
# git checkout 0.3.38
./autogen.sh && make -j4 && make install

adduser root rtkit && adduser fractal rtkit
# These also need to be exported
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/x86_64-linux-gnu:/usr/local/lib/x86_64-linux-gnu/pipewire-0.3
export PIPEWIRE_RUNTIME_DIR="/pipewire-0"
export PULSE_RUNTIME_PATH="/pulse"
cd /
mkdir pipewire-0
mkdir pulse
mkdir -p /etc/pipewire/media-session.d
touch /etc/pipewire/pipewire.conf
touch /etc/pipewire/media-session.d/with-pulseaudio
cp -r /usr/local/lib/x86_64-linux-gnu/* /lib/
cp -r /usr/local/lib/x86_64-linux-gnu/pipewire-0.3/* /lib/
cp /usr/local/lib/systemd/user/* /etc/systemd/system/
systemctl --now disable pulseaudio.service fractal-audio
systemctl --now disable fractal-audio
rm /etc/systemd/system/fractal-audio.service
systemctl --now mask fractal-audio
cp /pipewire/builddir/src/daemon/pipewire.conf /etc/pipewire/pipewire.conf
cp /pipewire/src/daemon/media-session.d/media-session.conf /etc/pipewire/media-session.d/
dbus-launch --sh-syntax --exit-with-session

# Manual step here: allow root to run pipewire-pulse
systemctl daemon-reload

systemctl --now enable pipewire.service pipewire.socket pipewire-media-session.service pipewire-pulse.service pipewire-pulse.socket
systemctl --now restart pipewire.service pipewire.socket pipewire-media-session.service pipewire-pulse.service pipewire-pulse.socket
