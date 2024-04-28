#!/usr/bin/bash

make distclean
bear -- make O=out mx6ull_alientek_emmc_defconfig
bear -- make O=out -j8
# rm ~/OSD_share/u-boot.imx
cp out/u-boot.imx ~/OSD_share/
