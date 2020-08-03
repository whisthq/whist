#!/bin/bash
echo "Setting up Fractal Firewall"
source /utils.sh && Enable-FractalFirewallRule
yes | ufw allow 5900;

#Allow for ssh login
rm /var/run/nologin
echo $SSH_PUBLIC_KEY_AWS > ~/.ssh/authorized_keys

sudo rm /etc/udev/rules.d/90-fractal-input.rules
sudo ln -s /home/fractal/fractal-input.rules /etc/udev/rules.d/90-fractal-input.rules
echo "Entry.sh handing off to bootstrap.sh" 