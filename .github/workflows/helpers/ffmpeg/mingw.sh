printf '\033]0;install 64 bit compiler\007'
[[ "$(uname)" = *6.1* ]] && nargs="-n 4"
sed 's/^/mingw-w64-x86_64-/g' /etc/pac-mingw.pk | xargs $nargs pacman -Sw --noconfirm --ask=20 --needed
sed 's/^/mingw-w64-x86_64-/g' /etc/pac-mingw.pk | xargs $nargs pacman -S --noconfirm --ask=20 --needed
sleep 3
exit
