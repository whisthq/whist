#!/bin/bash
# Amazon Linux doesn't have the need for a Fractal firewall rule
# yes | ufw allow 5900;

# Allow for ssh login
rm /var/run/nologin
# echo $SSH_PUBLIC_KEY_AWS > ~/.ssh/authorized_keys

rm /etc/udev/rules.d/90-fractal-input.rules
ln -sf /home/fractal/fractal-input.rules /etc/udev/rules.d/90-fractal-input.rules
# echo "Entry.sh handing off to bootstrap.sh" 

# Create a tty within the container so we don't have to hook it up to one of the host's
# Also, create the device /dev/dri/card0 which is needed for GPU accel
# Note that this CANNOT be done in the Dockerfile because it affects /dev/ so we have
# to do it here.
# Note that we always use /dev/tty10 even though the minor number below (i.e.
# the number after 4 may change)
sudo mknod -m 620 /dev/tty10 c 4 10
sudo mkdir /dev/dri
sudo mknod -m 660 /dev/dri/card0 c 226 0

# This install fractal service
echo "Start Pam Systemd Process for User fractal"
export FRACTAL_UID=`id -u fractal`
install -d -o fractal /run/user/$FRACTAL_UID
systemctl start user@$FRACTAL_UID
