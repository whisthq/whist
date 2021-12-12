apt-get update && apt-get install python3-pip -y && pip3 install meson

apt-get install dbus-x11 pkg-config ninja-build build-essential libpython3-dev libdbus-1-dev libudev1 libudev-dev libglib2.0-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev bluez libbluetooth-dev libsbc-dev qjackctl pulseaudio-module-jack libjack-jackd2-dev libvulkan1 mesa-vulkan-drivers vulkan-utils libvulkan-dev rtkit libsystemd-dev libpulse-dev xdg-desktop-portal systemd-coredump -y

git clone https://gitlab.freedesktop.org/pipewire/pipewire.git && cd pipewire
git checkout 0.3.36
./autogen.sh -Dsystemd=enabled -Dsystemd-system-service=enabled \
  -Dpipewire-alsa=enabled -Dlibpulse=enabled \
  -Droc=disabled -Dlibusb=disabled \
  -Dsystemd-user-unit-dir=/etc/systemd/system
  -Ddbus=disabled \
  -Dbluez5=disabled -Dbluez5-backend-hsp-native=disabled -Dbluez5-backend-hfp-native=disabled \
  -Dbluez5-backend-ofono=disabled -Dbluez5-backend-hsphfpd=disabled -Dbluez5-codec-aptx=disabled \
  -Dbluez5-codec-ldac=disabled -Dbluez5-codec-aac=disabled \
  -Dgstreamer=disabled -Dlibcamera=disabled -Dvideoconvert=disabled -Dvulkan=disabled \
  -Dsndfile=disabled -Dlibusb=disabled
make -j4 && make install

adduser root rtkit && adduser fractal rtkit
# These also need to be exported
cd /
echo "PIPEWIRE_RUNTIME_DIR=\"/pipewire-0\"" > /test-env
echo "XDG_RUNTIME_DIR=\"/xdg-runtime\"" >> /test-env
echo "PULSE_RUNTIME_PATH=\"/pulse\"" >> /test-env
mkdir /pipewire-0 && mkdir /pulse && mkdir /xdg-runtime
mkdir -p /etc/pipewire/media-session.d
touch /etc/pipewire/pipewire.conf && touch /etc/pipewire/media-session.d/with-pulseaudio
cp -r /usr/local/lib/x86_64-linux-gnu/* /lib/
cp -r /usr/local/lib/x86_64-linux-gnu/pipewire-0.3/* /lib/
systemctl --now disable pulseaudio.service
dbus-launch --sh-syntax --exit-with-session

rm -r /usr/local/share/pipewire/client-rt.conf /usr/local/share/pipewire/filter-chain
rm -r /usr/local/share/pipewire/media-session.d/bluez-monitor.conf /usr/local/share/pipewire/media-session.d/with-jack

# systemctl daemon-reload
# systemctl enable pipewire.service pipewire.socket pipewire-media-session.service pipewire-pulse.service pipewire-pulse.socket
