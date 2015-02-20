#!/bin/bash

BOOTIMG=x8hp-boot.img
CPUS=$(( `cat /proc/cpuinfo | egrep 'processor\s+:' | tail -1 | cut -d':' -f2` + 1 ))
DTDFILES=meson8m2

function die() {
    echo $1
    exit 1
}

function native_install() {
    echo "Installing modules natively..."
    make modules_install            || die "modules_install"
    echo "Writing kernel to nand..."
    dd if=/dev/zero  of=/dev/boot   || die "dd if=/dev/zero"
    dd if=${BOOTIMG} of=/dev/boot   || die "dd if=${BOOTIMG}"
}

if [ "$1" == "install" ]; then
    grep -q ARM /proc/cpuinfo && \
        native_install || \
        echo "Native install called but not native arch"
    exit 1
fi

grep -q ARM /proc/cpuinfo || CROSSARCH="ARCH=arm CROSS_COMPILE=armv7a-hardfloat-linux-gnueabi-"

#DTDFILE=meson8m2_n200_2G
#make ${DTDFILE}.dtd                || die "${DTDFILE}.dtd"
#make ${DTDFILE}.dtb                || die "${DTDFILE}.dtb"
#    --second ./arch/arm/boot/dts/amlogic/${DTDFILE}.dtb

make ${CROSSARCH} -j${CPUS} uImage  || die "uImage"

make ${CROSSARCH} -j${CPUS} modules || die "modules"

for dtd in `ls arch/arm/boot/dts/amlogic/ | grep ${DTDFILES}`; do
    echo "Compiling $dtd"
    make ${CROSSARCH} ${dtd}                 || die ${dtd}
    make ${CROSSARCH} ${dtd/.dtd/.dtb}  || die ${dtd/.dtd/.dtb}
    echo
done

if [ ! -e dt.img ]; then
    grep -q ARM /proc/cpuinfo && die "dtbTool needs an x86"
    ./dtbTool -o dt.img -p scripts/dtc/ arch/arm/boot/dts/amlogic/ || die "dtbTool"
fi

echo

[ -e "${BOOTIMG}" ] && rm -f ${BOOTIMG}
./mkbootimg --kernel ./arch/arm/boot/uImage --ramdisk ./rootfs.cpio \
    --second dt.img --output ./${BOOTIMG} || die "mkbootimg"

grep -q ARM /proc/cpuinfo && native_install

ls -Al ./${BOOTIMG}

#./mkbootimg --kernel ./arch/arm/boot/uImage --ramdisk ./rootfs.cpio --second ./arch/arm/boot/dts/amlogic/meson8m2_n200_2G.dtb --output ./x8hp-boot.img
#dd if=/dev/zero     of=/dev/boot
#dd if=x8hp-boot.img of=/dev/boot

echo
echo "${BOOTIMG} done"
exit 0
