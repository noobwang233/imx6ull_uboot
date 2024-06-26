/*
 * Copyright (c) 2013 Google, Inc
 *
 * (C) Copyright 2012
 * Pavel Herrmann <morpheus.ibis@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _DM_LISTS_H_
#define _DM_LISTS_H_

#include <dm/uclass-id.h>

/**
 * lists_driver_lookup_name() - Return u_boot_driver corresponding to name
 *
 * This function returns a pointer to a driver given its name. This is used
 * for binding a driver given its name and platdata.
 *
 * @name: Name of driver to look up
 * @return pointer to driver, or NULL if not found
 */
struct driver *lists_driver_lookup_name(const char *name);

/**
 * lists_uclass_lookup() - Return uclass_driver based on ID of the class
 * id:		ID of the class
 *
 * This function returns the pointer to uclass_driver, which is the class's
 * base structure based on the ID of the class. Returns NULL on error.
 */
struct uclass_driver *lists_uclass_lookup(enum uclass_id id);

/**
 * lists_bind_drivers() - search for and bind all drivers to parent
 *
 * This searches the U_BOOT_DEVICE() structures and creates new devices for
 * each one. The devices will have @parent as their parent.
 *
 * @parent: parent device (root)
 * @early_only: If true, bind only drivers with the DM_INIT_F flag. If false
 * bind all drivers.
 */
int lists_bind_drivers(struct udevice *parent, bool pre_reloc_only);

/**
 * lists_bind_fdt() - 绑定一个设备树节点
 *
 * 这将创建一个新的设备，将其绑定到给定的设备树节点，其父级为 @parent。
 *
 * @parent: 父设备（根）
 * @blob: 设备树 blob
 * @offset: 此设备树节点的偏移量
 * @devp: 如果非空，则返回绑定的设备指针
 * @return 如果设备被绑定返回 0，如果设备树无效返回 -EINVAL，其他负值表示出错
 */
int lists_bind_fdt(struct udevice *parent, const void *blob, int offset,
		   struct udevice **devp);

/**
 * device_bind_driver() - bind a device to a driver
 *
 * This binds a new device to a driver.
 *
 * @parent:	Parent device
 * @drv_name:	Name of driver to attach to this parent
 * @dev_name:	Name of the new device thus created
 * @devp:	If non-NULL, returns the newly bound device
 */
int device_bind_driver(struct udevice *parent, const char *drv_name,
		       const char *dev_name, struct udevice **devp);

/**
 * device_bind_driver_to_node() - bind a device to a driver for a node
 *
 * This binds a new device to a driver for a given device tree node. This
 * should only be needed if the node lacks a compatible strings.
 *
 * @parent:	Parent device
 * @drv_name:	Name of driver to attach to this parent
 * @dev_name:	Name of the new device thus created
 * @node:	Device tree node
 * @devp:	If non-NULL, returns the newly bound device
 */
int device_bind_driver_to_node(struct udevice *parent, const char *drv_name,
			       const char *dev_name, int node,
			       struct udevice **devp);

#endif
