#!/bin/bash

BOOTIMG=x8hp-boot.img
CPUS=$(( `cat /proc/cpuinfo | egrep 'processor\s+:' | tail -1 | cut -d':' -f2` + 1 ))
DTDFILES=meson8m2
ROOTFS="rootfs.cpio"

function die() {
    echo $1
    exit 1
}

grep -q Intel /proc/cpuinfo && CROSSARCH="ARCH=arm CROSS_COMPILE=armv7a-hardfloat-linux-gnueabi-"

#DTDFILE=meson8m2_n200_2G
#make ${DTDFILE}.dtd                    || die "${DTDFILE}.dtd"
#make ${DTDFILE}.dtb                    || die "${DTDFILE}.dtb"
#    --second ./arch/arm/boot/dts/amlogic/${DTDFILE}.dtb

make ${CROSSARCH} -j${CPUS} uImage      || die "uImage"

make ${CROSSARCH} -j${CPUS} modules     || die "modules"
make ${CROSSARCH} modules_install       || die "modules_install"

for dtd in `ls arch/arm/boot/dts/amlogic/ | grep ${DTDFILES}`; do
    make ${CROSSARCH} ${dtd}                 || die ${dtd}
    make ${CROSSARCH} ${dtd/.dtd/.dtb}  || die ${dtd/.dtd/.dtb}
done
./dtbTool -o out/dt.img -p scripts/dtc/ arch/arm/boot/dts/amlogic/

[ -e "${BOOTIMG}" ] && rm -f ${BOOTIMG}
./mkbootimg --kernel ./arch/arm/boot/uImage --ramdisk ./${ROOTFS} \
    --second ./out/dt.img --output ./${BOOTIMG} || die "mkbootimg"

dd if=/dev/zero      of=/dev/boot
dd if=${BOOTIMG}     of=/dev/boot

#./mkbootimg --kernel ./arch/arm/boot/uImage --ramdisk ./rootfs.cpio --second ./arch/arm/boot/dts/amlogic/meson8m2_n200_2G.dtb --output ./x8hp-boot.img
#dd if=/dev/zero     of=/dev/boot
#dd if=x8hp-boot.img of=/dev/boot

ls -Al ./${BOOTIMG}

echo
echo "${BOOTIMG} done"
exit 0
