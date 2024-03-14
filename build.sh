#!/bin/bash

make distclean
bear make mx6ull_alientek_emmc_defconfig
bear make -j8
#rm ~/OSD_share/u-boot.imx
#cp u-boot.imx ~/OSD_share/
