#!/usr/bin/bash

make distclean
# bear -- make O=out mx6ull_alientek_emmc_defconfig
bear -- make O=out mx6ull_14x14_evk_emmc_defconfig
bear -- make O=out -j8
# rm ~/OSD_share/u-boot.imx
cp out/u-boot-dtb.imx ~/OSD_share/
