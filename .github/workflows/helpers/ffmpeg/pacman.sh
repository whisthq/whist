@@ -1,8 +0,0 @@
echo -ne "\033]0;install base system\007"
msysbasesystem="$(cat /etc/pac-base.pk | tr '\n\r' '  ')"
[[ "$(uname)" = *6.1* ]] && nargs="-n 4"
echo $msysbasesystem | xargs $nargs pacman -Sw --noconfirm --ask=20 --needed
echo $msysbasesystem | xargs $nargs pacman -S --noconfirm --ask=20 --needed
echo $msysbasesystem | xargs $nargs pacman -D --asexplicit
sleep 3
exit
