#!/bin/bash
if [ "$EUID" -ne 0 ]; then
  echo "This script requires root rights. Execute it with sudo."
  exit 1
fi

# Function to detect Linux distribution and install packages accordingly
install_packages() {
  if [ -f /etc/arch-release ]; then
    echo "Detected Arch Linux or derivative"
    pacman -Sy --noconfirm git dkms base-devel
    pacman install $(pacman list --quiet --installed | grep "^linux[0-9]*[-rt]*$" | awk '{print $1"-headers"}' ORS=' ')
  elif [ -f /etc/debian_version ]; then
    echo "Detected Debian/Ubuntu or derivative"
    apt update && apt install -y git dkms build-essential linux-headers-$(uname -r)
  elif [ -f /etc/redhat-release ]; then
    echo "Detected Fedora/RHEL/CentOS or derivative"
    dnf -y group install "C Development Tools and Libraries"
    dnf -y group install "Development Tools"
    dnf -y install git dkms kernel-headers kernel-devel
  else
    echo "Unsupported Linux distribution."
    exit 1
  fi
}

# Install required packages
install_packages

rm -rf ~/clevo-keyboard
rmmod clevo_acpi
rmmod clevo_wmi
rmmod tuxedo_io
rmmod tuxedo_keyboard
rm /etc/modprobe.d/tuxedo_keyboard.conf
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
