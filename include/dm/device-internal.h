/*
 * Copyright (C) 2013 Google, Inc
 *
 * (C) Copyright 2012
 * Pavel Herrmann <morpheus.ibis@gmail.com>
 * Marek Vasut <marex@denx.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _DM_DEVICE_INTERNAL_H
#define _DM_DEVICE_INTERNAL_H

struct udevice;

/**
 * device_bind() - 创建一个设备并将其绑定到一个驱动程序
 *
 * 用于设置一个附加到驱动程序的新设备。设备将具有platdata或设备树节点，该节点可用于创建platdata。
 *
 * 绑定后，设备存在但尚未激活，直到调用device_probe()为止。
 *
 * @parent: 指向设备父级的指针，该驱动程序将存在于其下
 * @drv: 设备的驱动程序
 * @name: 设备的名称（例如，设备树节点名称）
 * @platdata: 指向此设备的数据的指针 - 结构是设备特定的，但可能包括设备的I/O地址等。对于使用设备树的设备，此值为NULL。
 * @of_offset: 此设备的设备树节点的偏移量。对于不使用设备树的设备，此值为-1。
 * @devp: 如果非空，则返回指向绑定设备的指针
 * @return: 如果成功返回0，否则返回负值错误码
 */
int device_bind(struct udevice *parent, const struct driver *drv,
		const char *name, void *platdata, int of_offset,
		struct udevice **devp);

/**
 * device_bind_by_name: 创建一个设备并将其绑定到一个驱动程序
 *
 * 这是一个用于绑定不使用设备树的设备的辅助函数。
 *
 * @parent: 指向设备父级的指针
 * @pre_reloc_only: 如果为true，则仅在其DM_INIT_F标志被设置时绑定驱动程序。如果为false，则始终绑定驱动程序。
 * @info: 此设备的名称和platdata
 * @devp: 如果非空，则返回指向绑定设备的指针
 * @return: 如果成功返回0，否则返回负值错误码
 */
int device_bind_by_name(struct udevice *parent, bool pre_reloc_only,
			const struct driver_info *info, struct udevice **devp);

/**
 * device_probe() - 探测设备，激活它
 *
 * 激活设备以使其准备好使用。首先探测所有的父级设备。
 *
 * @dev: 指向要探测的设备的指针
 * @return: 如果成功返回0，否则返回负值错误码
 */
int device_probe(struct udevice *dev);

/**
 * device_remove() - Remove a device, de-activating it
 *
 * De-activate a device so that it is no longer ready for use. All its
 * children are deactivated first.
 *
 * @dev: Pointer to device to remove
 * @return 0 if OK, -ve on error (an error here is normally a very bad thing)
 */
#if CONFIG_IS_ENABLED(DM_DEVICE_REMOVE)
int device_remove(struct udevice *dev);
#else
static inline int device_remove(struct udevice *dev) { return 0; }
#endif

/**
 * device_unbind() - Unbind a device, destroying it
 *
 * Unbind a device and remove all memory used by it
 *
 * @dev: Pointer to device to unbind
 * @return 0 if OK, -ve on error
 */
#if CONFIG_IS_ENABLED(DM_DEVICE_REMOVE)
int device_unbind(struct udevice *dev);
#else
static inline int device_unbind(struct udevice *dev) { return 0; }
#endif

#if CONFIG_IS_ENABLED(DM_DEVICE_REMOVE)
void device_free(struct udevice *dev);
#else
static inline void device_free(struct udevice *dev) {}
#endif

/**
 * simple_bus_translate() - translate a bus address to a system address
 *
 * This handles the 'ranges' property in a simple bus. It translates the
 * device address @addr to a system address using this property.
 *
 * @dev:	Simple bus device (parent of target device)
 * @addr:	Address to translate
 * @return new address
 */
fdt_addr_t simple_bus_translate(struct udevice *dev, fdt_addr_t addr);

/* Cast away any volatile pointer */
#define DM_ROOT_NON_CONST		(((gd_t *)gd)->dm_root)
#define DM_UCLASS_ROOT_NON_CONST	(((gd_t *)gd)->uclass_root)

/* device resource management */
#ifdef CONFIG_DEVRES

/**
 * devres_release_probe - Release managed resources allocated after probing
 * @dev: Device to release resources for
 *
 * Release all resources allocated for @dev when it was probed or later.
 * This function is called on driver removal.
 */
void devres_release_probe(struct udevice *dev);

/**
 * devres_release_all - Release all managed resources
 * @dev: Device to release resources for
 *
 * Release all resources associated with @dev.  This function is
 * called on driver unbinding.
 */
void devres_release_all(struct udevice *dev);

#else /* ! CONFIG_DEVRES */

static inline void devres_release_probe(struct udevice *dev)
{
}

static inline void devres_release_all(struct udevice *dev)
{
}

#endif /* ! CONFIG_DEVRES */
#endif
