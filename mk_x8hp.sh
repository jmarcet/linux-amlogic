#!/bin/bash

BOOTIMG=x8hp-boot.img
CPUS=$(( `cat /proc/cpuinfo | egrep 'processor\s+:' | tail -1 | cut -d':' -f2` + 1 ))
DTDFILE=meson8m2_n200_2G
grep -q ARM /proc/cpuinfo \
    && EXT='.arm' \
    || CROSSARCH="ARCH=arm CROSS_COMPILE=armv7a-hardfloat-linux-gnueabi-"

function die() {
    echo $1
    exit 1
}

function native_install() {
    echo "Creating ${BOOTIMG}..."
    [ -e "${BOOTIMG}" ] && rm -f ${BOOTIMG}
    [ -e ./arch/arm/boot/uImage ] || die "Missing ./arch/arm/boot/uImage"
    [ -e ./rootfs.cpio ]          || die "Missing ./rootfs.cpio"
    ./mkbootimg${EXT} --kernel ./arch/arm/boot/uImage --ramdisk ./rootfs.cpio \
        --second ./arch/arm/boot/dts/amlogic/${DTDFILE}.dtb \
        --output ./${BOOTIMG} || die "mkbootimg"
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

#make ${CROSSARCH} -j${CPUS} uImage  || die "uImage"
#make ${CROSSARCH} -j${CPUS} modules || die "modules"

make -j4 uImage      || die "uImage"
make -j4 modules     || die "modules"

make ${DTDFILE}.dtd  || die "${DTDFILE}.dtd"
make ${DTDFILE}.dtb  || die "${DTDFILE}.dtb"

#DTDFILES=meson8m2
#for dtd in `ls arch/arm/boot/dts/amlogic/ | grep ${DTDFILES}`; do
#    echo "Compiling $dtd"
#    make ${CROSSARCH} ${dtd}                 || die ${dtd}
#    make ${CROSSARCH} ${dtd/.dtd/.dtb}  || die ${dtd/.dtd/.dtb}
#    echo
#done
#if [ ! -e dt.img ]; then
#    ./dtbTool${EXT} -o dt.img -p scripts/dtc/ arch/arm/boot/dts/amlogic/ || die "dtbTool"
#fi

exit
grep -q ARM /proc/cpuinfo && native_install

ls -Al ./${BOOTIMG}

echo
echo "${BOOTIMG} done"
exit 0
