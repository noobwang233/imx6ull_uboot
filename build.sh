#!/usr/bin/bash

make O=out distclean
# bear -- make O=out mx6ull_atk_emmc_defconfig
bear -- make O=out mx6ull_atk_mini_emmc_defconfig
bear -- make O=out -j8
# rm ~/OSD_share/u-boot.imx
cp out/u-boot-dtb.imx ~/OSD_share/
