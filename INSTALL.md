# How to upgrade to TEACUP-1.1 + kernel 5.4

Lorem ipsum ...

### -1- Clone the forked TEACUP repo

```bash
git clone https://github.com/kr1stj0n/web10g.git
```

### Build the new kernel
- First, you need a .config file in the main kernel tree.<br/>
  Grab one from majora and edit it on your own.

```bash
cd web10g/web10g-kis-0.13-5.4/
scp ocarina@majora:/home/ocarina/kristjoc/web10g/web10g-kis-0.13-5.4/.config .
make oldconfig
```

- Install necessary packages to build the kernel

```bash
sudo apt update && sudo apt upgrade
sudo apt-get install git fakeroot build-essential ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison
```

- Start the building process

```bash
make oldconfig
make -j32
sudo make -j32 modules_install
sudo make -j32 headers_install
```

- Make the initdr image much smaller by excluding unecessary modules.<br/>
This is **VERY** helpful if you have a small boot partition.

```bash
cd /lib/modules/5.4.0/
sudo find . -name *.ko -exec strip --strip-unneeded {} +
cd /home/ocarina/kristjoc/web10g/web10g-kis-0.13-5.4/
sudo make install
```

### Reboot to the new kernel

```bash
sudo update-grub
sudo reboot
```

## Build the kernel module which collects TCP statistics

```bash
cd web10g-dlkm
make all
sudo -E make install
```

- Make a quick test to see whether the module can be loaded.

```bash
sudo modprobe tcp_estats_nl
sudo sysctl -w net.ipv4.tcp_estats=127
```
- Check dmesg messages

```bash
sudo dmesg
```

## Install the userland program

This programm is used by TEACUP to print the statistics to a file

```bash
cd web10g-userland
sudo apt install libtool
sudo apt install libmnl-dev
./autogen.sh
./configure
autoconf
make && sudo make install
sudo ldconfig
```

# Install teacup

```bash
sudo apt install pdfjam
sudo pip install fabric==1.14.1
sudo apt install r-base r-base-dev
```

```R
R>
install.packages("ggplot2")
```

```bash
git clone https://bitbucket.org/caia-swin/spp.git
cd spp
make all
sudo make install

sudo apt-get install libpcap-dev
pip install pexpect==3.2

sudo apt install psmisc
cp nttcp /usr/local/bin/

sudo apt install ntp
```
# Analyse

```bash
fab analyse_all:source_filter="S_172.16.10.2_"
sudo fab analyse_all:source_filter="D_10.100.130.7_*"
```
## Use iptables-legacy in Debian buster
sudo update-alternatives --set iptables /usr/sbin/iptables-legacy
