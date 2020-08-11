#!/bin/bash
# Amazon linux doesn't have this
# echo "Setting up Fractal Firewall"
# source /setup-scripts/linux/utils.sh && Enable-FractalFirewallRule
# yes | ufw allow 5900;

#Allow for ssh login
rm /var/run/nologin
# echo $SSH_PUBLIC_KEY_AWS > ~/.ssh/authorized_keys

sudo rm /etc/udev/rules.d/90-fractal-input.rules
sudo ln -sf /home/fractal/fractal-input.rules /etc/udev/rules.d/90-fractal-input.rules
# echo "Entry.sh handing off to bootstrap.sh" 

#This from setup-scripts install fractal service
echo "Start Pam Systemd Process for User fractal"
export FRACTAL_UID=`id -u fractal`
sudo install -d -o fractal /run/user/$FRACTAL_UID
sudo systemctl start user@$FRACTAL_UID
