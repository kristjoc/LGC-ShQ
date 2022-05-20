# LGC-ShQ Build & Installation

The LGC-ShQ repository contains a <a
href="https://wiki.geant.org/display/public/EK/WebTenG">Web10G</a> patched Linux
kernel with extended performance statistics for TCP. This kernel can be used
with <a href="http://caia.swin.edu.au/tools/teacup/">TEACUP</a> to run TCP
performance experiments in a physical testbed. You may build the whole kernel to
test the LGC-ShQ mechanism, or only the LGC congestion controller and the Shadow
Queue scheduler. TEACUP is not required to run the experiments since we provide
all the necessary scripts to collect statistics such as queueing delay, queue
size, congestion window, etc.</br>

To set up the Shadow Queue and configure its parameters, we include in this
repository also the
[`iproute2`](<https://github.com/kr1stj0n/LGC-ShQ/tree/main/iproute2>)
directory, which contains the `tc-shq` program. You can compile the whole
`iproute2` directory or import
[`q_shq.c`](<https://github.com/kr1stj0n/LGC-ShQ/blob/main/iproute2/tc/q_shq.c>)
in your own `iproute2`. **Note** that in the latter case, the
[`pkt_sched.h`](<https://github.com/kr1stj0n/LGC-ShQ/blob/main/iproute2/include/uapi/linux/pkt_sched.h>)
header needs to be included.

## How to build the Linux kernel

### 1. Clone the LGC-ShQ repo

```bash
git clone https://github.com/kr1stj0n/LGC-ShQ.git
```

### 2. Build the new kernel

- First, you need a `.config` file in the root of the kernel source tree.<br/>
  Make sure to set `CONFIG_TCP_CONG_LGC=m` and `CONFIG_NET_SCH_SHQ=m` to enable
  the compilation of LGC congestion controller and Shadow Queue scheduler.<br/>
  Edit the current `.config` file on your own and run the following commands:

```bash
cd ~/LGC-ShQ/web10g-kis-0.13-5.4/
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
cd ~/LGC-ShQ/web10g-kis-0.13-5.4/
sudo make install
```

### 3. Reboot to the new kernel

```bash
sudo update-grub
sudo reboot
```

### 4. (Optional) If you are using TEACUP, build the kernel module which collects the TCP statistics

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

### 5. (Optional) If you are using TEACUP, install the userland program

This program is used by TEACUP to print the statistics to a file.

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

###  6. (Optional) Install TEACUP 3rd-party tools

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

## How to build **ONLY** the kernel modules

Check
[this](<https://github.com/kr1stj0n/LGC-ShQ/blob/main/web10g-kis-0.13-5.4/tools/lgc-shq/build.sh>)
build script to install **ONLY** the kernel modules of LGC and ShQ instead of
compiling the whole kernel source tree. Comment out the lines `eval $CMD_LGC`,
`eval $CMD_SHQ` and the last line `build_modules` and then run the `build.sh` script.

```bash
./build.sh
```

Make sure to first edit the
[`pkt_sched.h`](<https://github.com/kr1stj0n/LGC-ShQ/blob/main/web10g-kis-0.13-5.4/include/uapi/linux/pkt_sched.h>)
header accordingly.

## How to build the `iproute2` utilities

See the
[`README`](<https://github.com/kr1stj0n/LGC-ShQ/blob/main/iproute2/README>)
instructions to compile the `iproute2` utility programs.
