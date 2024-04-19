/*
 * Copyright (c) 2013 Google, Inc
 *
 * (C) Copyright 2012
 * Pavel Herrmann <morpheus.ibis@gmail.com>
 * Marek Vasut <marex@denx.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _DM_PLATDATA_H
#define _DM_PLATDATA_H

#include <linker_lists.h>

/**
 * struct driver_info - 实例化设备所需的信息
 *
 * 注意：除了在极端情况下不可行（例如，SPL 中的串行驱动程序，其中可用的 SRAM 少于 8KB）之外，尽量避免使用此结构。U-Boot 的驱动程序模型使用设备树进行配置。
 *
 * @name: 驱动程序名称
 * @platdata: 驱动程序特定的平台数据
 */
struct driver_info {
	const char *name;
	const void *platdata;
};

/**
 * NOTE: Avoid using these except in extreme circumstances, where device tree
 * is not feasible (e.g. serial driver in SPL where <8KB of SRAM is
 * available). U-Boot's driver model uses device tree for configuration.
 */
#define U_BOOT_DEVICE(__name)						\
	ll_entry_declare(struct driver_info, __name, driver_info)

/* Declare a list of devices. The argument is a driver_info[] array */
#define U_BOOT_DEVICES(__name)						\
	ll_entry_declare_list(struct driver_info, __name, driver_info)

#endif
