#!/bin/bash
export GTK_IM_MODULE=ibus
export XMODIFIERS=@im=ibus
export QT_IM_MODULE=ibus
#DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus

# Exit on subcommand errors
set -Eeuo pipefail

# Disable any triggers for switching keyboards on the mandelbox
gsettings set org.freedesktop.ibus.general preload-engines "['xkb:us::eng', 'anthy', 'pinyin', 'hangul', 'Unikey']"
# gsettings set org.freedesktop.ibus.general.hotkey triggers "[]"
ibus-daemon -drx
