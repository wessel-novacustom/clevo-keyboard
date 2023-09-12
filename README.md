<img src="https://github.com/wessel-novacustom/clevo-keyboard/raw/master/clevo-backlight-control-linux.png" align="right" width="450" />

# The change we made

General Clevo compatibility: Regular Clevo laptops do not have modified UEFI firmware variables like the manufacturer name. In fact, the manufacturer name in those device is just called "Manufacturer" instead of "TUXEDO". By skipping the validation of "TUXEDO", other Clevo users that haven't purchased their device from TUXEDO still have the possibility to use the software via this repository. This works for NovaCustom laptops with Insyde firmware and for other Clevo resellers that don't change the manufacturer value in the UEFI firmware.

Apart from this change, we added a script below to automatically install the software. Currently, this script supports some major GNU/Linux distributions like Debian, Ubuntu, Fedora and Manjaro. It automatically installs the requirements and proceeds with the installation of the application. Also, it sets the default keyboard color to white.

## Automated installation

To install the software automatically, open a terminal and execute:

```sh
wget https://github.com/wessel-novacustom/clevo-keyboard/raw/master/kb.sh && chmod +x kb.sh && sudo ./kb.sh
```

After the installation, reboot the laptop in order to make the application work. You can change the keyboard illumination settings by holding the Fn key and use the keyboard control keys on the right side of your keyboard.

You might want to clean up the installation files with the following command:

```sh
sudo rm -rf ~/tuxedo-keyboard/ && rm ~/kb.sh
```

## Change the default color

To change the default color of the keyboard backlight, execute:

```sh
echo "options tuxedo_keyboard color=WHITE" > /etc/modprobe.d/tuxedo_keyboard.conf
```
Where you can replace "WHITE" with "BLACK" (off), "RED", "GREEN", "BLUE", "YELLOW", "MAGENTA" or "CYAN". As an alternative, you can set your own color HEX code like "0xFFFF00".

## A special note for Dasharo coreboot firmware users

<a href="https://configurelaptop.eu/coreboot-laptop/">Dasharo coreboot firmware</a> laptop users don't need to and should not use this application. This also applies for users of other coreboot firmware distributions. The keyboard backlight control is already included in the firmware for that firmware version.

---

# Original unchanged content below

---

# Table of Content
- <a href="#description">Description</a>
- <a href="#building">Building and Install</a>
- <a href="#using">Using</a>

# Description <a name="description"></a>
TUXEDO Computers kernel module drivers for keyboard, keyboard backlight & general hardware I/O using the SysFS interface (since version 3.2.0)

Features
- Driver for Fn-keys
- SysFS control of brightness/color/mode for most TUXEDO keyboards
    - [https://docs.kernel.org/leds/leds-class.html](https://docs.kernel.org/leds/leds-class.html)
    - [https://docs.kernel.org/leds/leds-class-multicolor.html](https://docs.kernel.org/leds/leds-class-multicolor.html)
- Hardware I/O driver for TUXEDO Control Center

Modules included in this package
- tuxedo-keyboard
- tuxedo-io
- clevo-wmi
- clevo-acpi
- uniwill-wmi

# Building and Install <a name="building"></a>

## Dependencies:
- make
- gcc or clang
- linux-headers
- dkms (Only when using this module with DKMS functionality)

## Warning when installing the module:

Use either method only. Do not combine installation methods, such as starting with the build step below and proceeding to use the same build artifacts with the DKMS module. Otherwise the module built via dkms will fail to load with an `exec_format` error on newer kernels due to a mismatched version magic.

This is why the DKMS build step begins with a `make clean` step. 

For convenience, on platforms where DKMS is in use, skip to the DKMS section directly.

## Clone the Git Repo:

```sh
git clone https://github.com/tuxedocomputers/tuxedo-keyboard.git

cd tuxedo-keyboard

git checkout release
```

## Build the Module:

```sh
make clean && make
```

## The DKMS route:

### Add as DKMS Module:

Install the Module:
```sh
make clean

sudo make dkmsinstall
```

Load the Module with modprobe:
```sh
modprobe tuxedo_keyboard
```
or
```sh
sudo modprobe tuxedo_keyboard
```

You might also want to activate `tuxedo_io` module the same way if you are using [TCC](https://github.com/tuxedocomputers/tuxedo-control-center).

### Uninstalling the DKMS module:

Remove the DKMS module and source:
```sh
sudo make dkmsremove

sudo rm /etc/modprobe.d/tuxedo_keyboard.conf
```

# Using <a name="using"></a>

## modprobe

```sh
modprobe tuxedo_keyboard
```

## Load the Module on boot:

If a module is relevant it will be loaded automatically on boot. If it is not loaded after a reboot, it most likely means that it is not needed.

Add Module to /etc/modules
```sh
sudo su

echo tuxedo_keyboard >> /etc/modules
```
