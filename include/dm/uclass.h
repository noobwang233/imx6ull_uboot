/*
 * Copyright (c) 2013 Google, Inc
 *
 * (C) Copyright 2012
 * Pavel Herrmann <morpheus.ibis@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _DM_UCLASS_H
#define _DM_UCLASS_H

#include <dm/uclass-id.h>
#include <linker_lists.h>
#include <linux/list.h>

/**
 * struct uclass - 一个 U-Boot 驱动类，将相似的驱动程序集合在一起
 *
 * 一个 uclass 提供了对特定功能的接口，该功能由一个或多个驱动程序实现。
 * 即使只有一个驱动程序，每个驱动程序都属于一个 uclass。例如 GPIO 是一个
 * 示例 uclass，它提供了改变读取输入、设置和清除输出等功能。可以有用于芯片
 * SoC GPIO bank、I2C GPIO 扩展器和 PMIC IO 线的驱动程序，所有这些驱动程
 * 序都以统一的方式通过 uclass 提供。
 *
 * @priv: 此 uclass 的私有数据
 * @uc_drv: 该 uclass 本身的驱动程序，不要与 'struct driver' 混淆
 * @dev_head: 此 uclass 中设备的列表（设备在调用其绑定方法时附加到其 uclass）
 * @sibling_node: uclass 链表中的下一个 uclass
 */
struct uclass {
	void *priv;
	struct uclass_driver *uc_drv;
	struct list_head dev_head;
	struct list_head sibling_node;
};

struct udevice;

/* Members of this uclass sequence themselves with aliases */
#define DM_UC_FLAG_SEQ_ALIAS			(1 << 0)

/**
 * struct uclass_driver - 用于 uclass 的驱动程序
 *
 * 一个 uclass_driver 提供了一个一致的接口给一组相关的驱动程序。
 *
 * @name: uclass 驱动程序的名称
 * @id: 该 uclass 的 ID 号码
 * @post_bind: 当一个新设备绑定到该 uclass 时调用
 * @pre_unbind: 当一个设备从该 uclass 解绑之前调用
 * @pre_probe: 在新设备被探测之前调用
 * @post_probe: 在新设备被探测之后调用
 * @pre_remove: 在一个设备被移除之前调用
 * @child_post_bind: 在该 uclass 中的设备的子级被绑定之后调用
 * @init: 被调用以设置该 uclass
 * @destroy: 被调用以销毁该 uclass
 * @priv_auto_alloc_size: 如果非零，则分配在 uclass 的 ->priv 指针中的私有数据的大小。
 * 如果为零，则 uclass 驱动程序负责分配所需的任何数据。
 * @per_device_auto_alloc_size: 每个设备可以拥有 uclass 拥有的私有数据。
 * 如果需要，如果此值为非零，则会自动分配。
 * @per_device_platdata_auto_alloc_size: 每个设备可以拥有 uclass 拥有的平台数据，
 * 作为 'dev->uclass_platdata'。如果值为非零，则将自动分配。
 * @per_child_auto_alloc_size: 每个子设备（属于该 uclass 中的父设备）可以保存设备/uclass 的父数据。
 * 如果驱动程序中的此成员为 0，则此值仅用作后备。
 * @per_child_platdata_auto_alloc_size: 总线喜欢存储有关其子级的信息。
 * 如果非零，则此数据的大小，将分配在子设备的 parent_platdata 指针中。
 * 如果驱动程序中的此成员为 0，则此值仅用作后备。
 * @ops: Uclass 操作，为 uclass 中的设备提供一致的接口。
 * @flags: 该 uclass 的标志（DM_UC_...）
 */
struct uclass_driver {
	const char *name;
	enum uclass_id id;
	int (*post_bind)(struct udevice *dev);
	int (*pre_unbind)(struct udevice *dev);
	int (*pre_probe)(struct udevice *dev);
	int (*post_probe)(struct udevice *dev);
	int (*pre_remove)(struct udevice *dev);
	int (*child_post_bind)(struct udevice *dev);
	int (*child_pre_probe)(struct udevice *dev);
	int (*init)(struct uclass *class);
	int (*destroy)(struct uclass *class);
	int priv_auto_alloc_size;
	int per_device_auto_alloc_size;
	int per_device_platdata_auto_alloc_size;
	int per_child_auto_alloc_size;
	int per_child_platdata_auto_alloc_size;
	const void *ops;
	uint32_t flags;
};

/* Declare a new uclass_driver */
#define UCLASS_DRIVER(__name)						\
	ll_entry_declare(struct uclass_driver, __name, uclass)

/**
 * uclass_get() - 根据 ID 获取一个 uclass，如果需要的话则创建它
 *
 * 每个 uclass 都由一个 ID 标识，这是一个从 0 到 n-1 的数字，其中 n 是
 * uclass 的数量。此函数允许通过其 ID 查找一个 uclass。
 *
 * @key: 要查找的 ID
 * @ucp: 返回指向 uclass 的指针（每个 ID 只有一个）
 * @return 如果成功则返回 0，否则返回负值
 */
int uclass_get(enum uclass_id key, struct uclass **ucp);

/**
 * uclass_get_device() - 根据 ID 和索引获取一个 uclass 设备
 *
 * 设备将被探测以激活它以便准备使用。
 *
 * @id: 要查找的 ID
 * @index: 在该 uclass 中的设备编号（0=第一个）
 * @devp: 返回指向设备的指针（每个 ID 只有一个）
 * @return 如果成功则返回 0，否则返回负值
 */
int uclass_get_device(enum uclass_id id, int index, struct udevice **devp);

/**
 * uclass_get_device_by_name() - 根据名称获取一个 uclass 设备
 *
 * 这将在 uclass 中搜索具有完全给定名称的设备。
 *
 * 设备将被探测以激活它以便准备使用。
 *
 * @id: 要查找的 ID
 * @name: 要获取的设备的名称
 * @devp: 返回指向设备的指针（第一个具有该名称的设备）
 * @return 如果成功则返回 0，否则返回负值
 */
int uclass_get_device_by_name(enum uclass_id id, const char *name,
			      struct udevice **devp);

/**
 * uclass_get_device_by_seq() - Get a uclass device based on an ID and sequence
 *
 * If an active device has this sequence it will be returned. If there is no
 * such device then this will check for a device that is requesting this
 * sequence.
 *
 * The device is probed to activate it ready for use.
 *
 * @id: ID to look up
 * @seq: Sequence number to find (0=first)
 * @devp: Returns pointer to device (there is only one for each seq)
 * @return 0 if OK, -ve on error
 */
int uclass_get_device_by_seq(enum uclass_id id, int seq, struct udevice **devp);

/**
 * uclass_get_device_by_of_offset() - Get a uclass device by device tree node
 *
 * This searches the devices in the uclass for one attached to the given
 * device tree node.
 *
 * The device is probed to activate it ready for use.
 *
 * @id: ID to look up
 * @node: Device tree offset to search for (if -ve then -ENODEV is returned)
 * @devp: Returns pointer to device (there is only one for each node)
 * @return 0 if OK, -ve on error
 */
int uclass_get_device_by_of_offset(enum uclass_id id, int node,
				   struct udevice **devp);

/**
 * uclass_get_device_by_phandle() - Get a uclass device by phandle
 *
 * This searches the devices in the uclass for one with the given phandle.
 *
 * The device is probed to activate it ready for use.
 *
 * @id: uclass ID to look up
 * @parent: Parent device containing the phandle pointer
 * @name: Name of property in the parent device node
 * @devp: Returns pointer to device (there is only one for each node)
 * @return 0 if OK, -ENOENT if there is no @name present in the node, other
 *	-ve on error
 */
int uclass_get_device_by_phandle(enum uclass_id id, struct udevice *parent,
				 const char *name, struct udevice **devp);

/**
 * uclass_first_device() - Get the first device in a uclass
 *
 * The device returned is probed if necessary, and ready for use
 *
 * @id: Uclass ID to look up
 * @devp: Returns pointer to the first device in that uclass, or NULL if none
 * @return 0 if OK (found or not found), -1 on error
 */
int uclass_first_device(enum uclass_id id, struct udevice **devp);

/**
 * uclass_next_device() - Get the next device in a uclass
 *
 * The device returned is probed if necessary, and ready for use
 *
 * @devp: On entry, pointer to device to lookup. On exit, returns pointer
 * to the next device in the same uclass, or NULL if none
 * @return 0 if OK (found or not found), -1 on error
 */
int uclass_next_device(struct udevice **devp);

/**
 * uclass_resolve_seq() - Resolve a device's sequence number
 *
 * On entry dev->seq is -1, and dev->req_seq may be -1 (to allocate a
 * sequence number automatically, or >= 0 to select a particular number.
 * If the requested sequence number is in use, then this device will
 * be allocated another one.
 *
 * Note that the device's seq value is not changed by this function.
 *
 * @dev: Device for which to allocate sequence number
 * @return sequence number allocated, or -ve on error
 */
int uclass_resolve_seq(struct udevice *dev);

/**
 * uclass_foreach_dev() - Helper function to iteration through devices
 *
 * This creates a for() loop which works through the available devices in
 * a uclass in order from start to end.
 *
 * @pos: struct udevice * to hold the current device. Set to NULL when there
 * are no more devices.
 * @uc: uclass to scan
 */
#define uclass_foreach_dev(pos, uc)	\
	list_for_each_entry(pos, &uc->dev_head, uclass_node)

/**
 * uclass_foreach_dev_safe() - Helper function to safely iteration through devs
 *
 * This creates a for() loop which works through the available devices in
 * a uclass in order from start to end. Inside the loop, it is safe to remove
 * @pos if required.
 *
 * @pos: struct udevice * to hold the current device. Set to NULL when there
 * are no more devices.
 * @next: struct udevice * to hold the next next
 * @uc: uclass to scan
 */
#define uclass_foreach_dev_safe(pos, next, uc)	\
	list_for_each_entry_safe(pos, next, &uc->dev_head, uclass_node)

#endif
