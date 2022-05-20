# -*- bash -*-

#
# Written by: Kr1stj0n C1k0 <kristjoc@ifi.uio.no>
#

GIT_ROOT=`git rev-parse --show-toplevel`
MY_BUILD_DIR=`pwd`
MY_KERN_DIR="$GIT_ROOT/web10g-kis-0.13-5.4/"
MY_SCHED_DIR="$GIT_ROOT/web10g-kis-0.13-5.4/net/sched/"
MY_IPV4_DIR="$GIT_ROOT/web10g-kis-0.13-5.4/net/ipv4/"


build_stack() {

    CMD="cd $MY_KERN_DIR && make oldconfig && make modules_prepare && make -j32 && sudo make -j32 modules_install && sudo make -j32 headers_install;
         sudo depmod -a;
         sudo find /lib/modules/5.4.0/ -name *.ko -exec strip --strip-unneeded {} +;
         sudo rm -rf /boot/*5.4.0*;
         sudo make -j32 install"

    #eval $CMD
}

build_modules() {
    # SCH_MYFO
    CMD_MYFO="sudo rmmod -f sch_myfo;
			  cp $MY_BUILD_DIR/Makefile.myfo $MY_SCHED_DIR/Makefile;
			  cd $MY_KERN_DIR && make modules_prepare && make M=net/sched/ clean && make M=net/sched/ modules && sudo make M=net/sched/ modules_install;
			  sudo depmod -a;
              sudo cp /lib/modules/5.4.0/extra/sch_myfo.ko /lib/modules/5.4.0/kernel/net/sched/;
              sudo depmod -a;
              cp $MY_BUILD_DIR/Makefile.sched.init $MY_SCHED_DIR/Makefile;
              sudo modprobe sch_myfo"

    # SCH_SHQ
    CMD_SHQ="sudo rmmod -f sch_shq;
             cp $MY_BUILD_DIR/Makefile.shq $MY_SCHED_DIR/Makefile;
             cd $MY_KERN_DIR && make modules_prepare && make M=net/sched/ clean && make M=net/sched/ modules && sudo make M=net/sched/ modules_install;
             sudo depmod -a;
             sudo cp /lib/modules/5.4.0/extra/sch_shq.ko /lib/modules/5.4.0/kernel/net/sched/;
             sudo depmod -a;
             cp $MY_BUILD_DIR/Makefile.sched.init $MY_SCHED_DIR/Makefile;
             sudo modprobe sch_shq"

    # SCH_RED
    CMD_RED="sudo rmmod -f sch_red;
             cp $MY_BUILD_DIR/Makefile.red $MY_SCHED_DIR/Makefile;
             cd $MY_KERN_DIR && make modules_prepare && make M=net/sched/ clean && make M=net/sched/ modules && sudo make M=net/sched/ modules_install;
             sudo depmod -a;
             sudo cp /lib/modules/5.4.0/extra/sch_red.ko /lib/modules/5.4.0/kernel/net/sched/;
             sudo depmod -a;
             cp $MY_BUILD_DIR/Makefile.sched.init $MY_SCHED_DIR/Makefile;
             sudo modprobe sch_red"

    # SCH_HULL
    CMD_HULL="sudo rmmod -f sch_hull;
              cp $MY_BUILD_DIR/Makefile.hull $MY_SCHED_DIR/Makefile;
              cd $MY_KERN_DIR && make modules_prepare && make M=net/sched/ clean && make M=net/sched/ modules && sudo make M=net/sched/ modules_install;
              sudo depmod -a;
              sudo cp /lib/modules/5.4.0/extra/sch_hull.ko /lib/modules/5.4.0/kernel/net/sched/;
              sudo depmod -a;
              cp $MY_BUILD_DIR/Makefile.sched.init $MY_SCHED_DIR/Makefile;
              sudo modprobe sch_hull"

    # SCH_ABC
    CMD_ABC="sudo rmmod -f sch_abc;
             cp $MY_BUILD_DIR/Makefile.sched.abc $MY_SCHED_DIR/Makefile;
             cd $MY_KERN_DIR && make modules_prepare && make M=net/sched/ clean && make M=net/sched/ modules && sudo make M=net/sched/ modules_install;
             sudo depmod -a;
             sudo cp /lib/modules/5.4.0/extra/sch_abc.ko /lib/modules/5.4.0/kernel/net/sched/;
             sudo depmod -a;
             cp $MY_BUILD_DIR/Makefile.sched.init $MY_SCHED_DIR/Makefile;
             sudo modprobe sch_abc"

    # SCH_FQ
    CMD_FQ="sudo rmmod -f sch_fq;
            cp $MY_BUILD_DIR/Makefile.fq $MY_SCHED_DIR/Makefile;
            cd $MY_KERN_DIR && make modules_prepare && make M=net/sched/ clean && make M=net/sched/ modules && sudo make M=net/sched/ modules_install;
            sudo depmod -a;
            sudo cp /lib/modules/5.4.0/extra/sch_fq.ko /lib/modules/5.4.0/kernel/net/sched/;
            sudo depmod -a;
            cp $MY_BUILD_DIR/Makefile.sched.init $MY_SCHED_DIR/Makefile;
            sudo modprobe sch_fq"

    ####### CC #######

    # TCP_LGC
    CMD_LGC="sudo sysctl net.ipv4.tcp_congestion_control=cubic;
             sudo rmmod -f tcp_lgc;
             cp $MY_BUILD_DIR/Makefile.lgc $MY_IPV4_DIR/Makefile;
             cd $MY_KERN_DIR && make modules_prepare && make M=net/ipv4/ clean && make M=net/ipv4/ modules && sudo make M=net/ipv4/ modules_install;
             sudo depmod -a;
             sudo cp /lib/modules/5.4.0/extra/tcp_lgc.ko /lib/modules/5.4.0/kernel/net/ipv4/;
             sudo depmod -a;
             cp $MY_BUILD_DIR/Makefile.ipv4.init $MY_IPV4_DIR/Makefile;
             sudo modprobe tcp_lgc"

    # TCP_DCTCP
    CMD_DCTCP="sudo sysctl net.ipv4.tcp_congestion_control=cubic;
               sudo rmmod -f tcp_dctcp;
               cp $MY_BUILD_DIR/Makefile.dctcp $MY_IPV4_DIR/Makefile;
               cd $MY_KERN_DIR && make modules_prepare && make M=net/ipv4/ clean && make M=net/ipv4/ modules && sudo make M=net/ipv4/ modules_install;
               sudo depmod -a;
               sudo cp /lib/modules/5.4.0/extra/tcp_dctcp.ko /lib/modules/5.4.0/kernel/net/ipv4/;
               sudo depmod -a;
               cp $MY_BUILD_DIR/Makefile.ipv4.init $MY_IPV4_DIR/Makefile;
               sudo modprobe tcp_dctcp"

    # TCP_ABC
    CMD_TCP_ABC="sudo sysctl net.ipv4.tcp_congestion_control=cubic;
                 sudo rmmod -f tcp_abc;
                 cp $MY_BUILD_DIR/Makefile.cc.abc $MY_IPV4_DIR/Makefile;
                 cd $MY_KERN_DIR && make modules_prepare && make M=net/ipv4/ clean && make M=net/ipv4/ modules && sudo make M=net/ipv4/ modules_install;
                 sudo depmod -a;
                 sudo cp /lib/modules/5.4.0/extra/tcp_abc.ko /lib/modules/5.4.0/kernel/net/ipv4/;
                 sudo depmod -a;
                 cp $MY_BUILD_DIR/Makefile.ipv4.init $MY_IPV4_DIR/Makefile;
                 sudo modprobe tcp_abc"

#     eval $CMD_MYFO
#     eval $CMD_SHQ
#     eval $CMD_ABC
#     eval $CMD_RED
#     eval $CMD_HULL
#     eval $CMD_FQ
#     eval $CMD_LGC
#     eval $CMD_DCTCP
#     eval $CMD_TCP_ABC
}

# build_stack
# build_modules