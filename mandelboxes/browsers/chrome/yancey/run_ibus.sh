#!/bin/bash
export GTK_IM_MODULE=ibus
export XMODIFIERS=@im=ibus
export QT_IM_MODULE=ibus
#DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus
#gsettings set org.freedesktop.ibus.general preload-engines "['xkb:us::eng', 'anthy', 'pinyin']"
#gsettings set org.freedesktop.ibus.general.hotkey triggers "['<Control>space']"
ibus-daemon -drx
