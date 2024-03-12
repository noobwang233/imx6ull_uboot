#!/usr/bin/bash

make distclean
make mx6ull_alientek_emmc_defconfig
make -j8
rm ~/OSD_share/u-boot.imx
cp u-boot.imx ~/OSD_share/
