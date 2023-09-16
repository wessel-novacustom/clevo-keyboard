#!/bin/bash
if [ "$EUID" -ne 0 ]; then
  echo "This script requires root rights. Execute it with sudo."
  exit 1
fi
apt install -y git dkms build-essential linux-headers-$(uname -r)
rm -rf ~/clevo-keyboard
rmmod clevo_acpi
rmmod clevo_wmi
rmmod tuxedo_io
rmmod tuxedo_keyboard
rm /etc/modprobe.d/tuxedo_keyboard.conf
pacman -S base-devel
pacman -S dkms
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
cd src
file="tuxedo_keyboard.c"
output=$(dmidecode | grep Manufacturer)
sysvendor=$(echo "$output" | awk -F 'Manufacturer: ' 'NR==1{print $2}')
boardvendor=$(echo "$output" | awk -F 'Manufacturer: ' 'NR==2{print $2}')
chassisvendor=$(echo "$output" | awk -F 'Manufacturer: ' 'NR==3{print $2}')
sed -i "s/DMI_MATCH(DMI_SYS_VENDOR, .*)/DMI_MATCH(DMI_SYS_VENDOR, \"$sysvendor\")/g" "$file"
sed -i "s/DMI_MATCH(DMI_BOARD_VENDOR, .*)/DMI_MATCH(DMI_BOARD_VENDOR, \"$boardvendor\")/g" "$file"
sed -i "s/DMI_MATCH(DMI_CHASSIS_VENDOR, .*)/DMI_MATCH(DMI_CHASSIS_VENDOR, \"$chassisvendor\")/g" "$file"
cat $file
cd ..
make dkmsinstall
echo tuxedo_keyboard >> /etc/modules
modprobe tuxedo_keyboard
echo "options tuxedo_keyboard color=WHITE" > /etc/modprobe.d/tuxedo_keyboard.conf
rm -rf ~/clevo-keyboard
exit 0
