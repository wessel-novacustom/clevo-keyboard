#!/bin/bash
apt install -y git dkms build-essential linux-headers-$(uname -r)
rm -rf ~/clevo-keyboard
rmmod clevo_acpi
rmmod clevo_wmi
rmmod tuxedo_io
rmmod tuxedo_keyboard
sudo pacman -S base-devel
sudo pacman -S dkms
pamac install $(pamac list --quiet --installed | grep "^linux[0-9]*[-rt]*$" | awk '{print $1"-headers"}' ORS=' ')
dnf -y group install "C Development Tools and Libraries"
dnf -y group install "Development Tools"
dnf -y install git
dnf -y install dkms
dnf -y install kernel-headers
dnf -y install kernel-devel
git clone https://github.com/wessel-novacustom/clevo-keyboard
cd clevo-keyboard/
make clean
sudo make dkmsinstall
echo tuxedo_keyboard >> /etc/modules
modprobe tuxedo_keyboard
echo "options tuxedo_keyboard color=WHITE" > /etc/modprobe.d/tuxedo_keyboard.conf
rm -rf ~/clevo-keyboard
exit 0
