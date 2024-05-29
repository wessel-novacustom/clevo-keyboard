#!/bin/bash

check_root () {
    # Check if script is run as root
    if [ "$EUID" -ne 0 ]; then
      echo "This script requires root rights. Execute it with sudo."
      exit 1
    fi
}

check_fedora () {
    if [ -f /etc/fedora-release ]; then
        echo "It is fedora"

    else
        echo "It is NOT fedora"
        exit 1
    fi
}


cleanup () {
    rm -rf ~/clevo-keyboard
}


unload_remove_modules () {
    rmmod clevo_acpi
    rmmod clevo_wmi
    rmmod tuxedo_io
    rmmod tuxedo_keyboard
    rm /etc/modprobe.d/tuxedo_keyboard.conf
}

install_packages () {

    dnf -y group install "C Development Tools and Libraries"
    dnf -y group install "Development Tools"
    dnf -y install git
    dnf -y install dkms
    dnf -y install kernel-headers
    dnf -y install kernel-devel
}

install_module () {
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
    echo tuxedo_keyboard >> /etc/modules-load.d/tuxedo_keyboard.conf
    echo "options tuxedo_keyboard color=WHITE brightness=0" > /etc/modprobe.d/tuxedo_keyboard.conf
    modprobe tuxedo_keyboard
}

# ------
# Main
# ------

# Run the functions
check_root
check_fedora
cleanup
unload_remove_modules
install_packages
install_module
cleanup
