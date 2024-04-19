/*
 * Copyright (c) 2013 Google, Inc
 *
 * (C) Copyright 2012
 * Pavel Herrmann <morpheus.ibis@gmail.com>
 * Marek Vasut <marex@denx.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _DM_DEVICE_H
#define _DM_DEVICE_H

#include <dm/uclass-id.h>
#include <fdtdec.h>
#include <linker_lists.h>
#include <linux/compat.h>
#include <linux/kernel.h>
#include <linux/list.h>

struct driver_info;

/* Driver is active (probed). Cleared when it is removed */
#define DM_FLAG_ACTIVATED		(1 << 0)

/* DM is responsible for allocating and freeing platdata */
#define DM_FLAG_ALLOC_PDATA		(1 << 1)

/* 设备模型应在重定位之前初始化此设备 */
#define DM_FLAG_PRE_RELOC		(1 << 2)

/* 设备模型负责分配和释放 parent_platdata */
#define DM_FLAG_ALLOC_PARENT_PDATA	(1 << 3)

/* 设备模型负责分配和释放 uclass_platdata */
#define DM_FLAG_ALLOC_UCLASS_PDATA	(1 << 4)

/* Allocate driver private data on a DMA boundary */
#define DM_FLAG_ALLOC_PRIV_DMA		(1 << 5)

/* Device is bound */
#define DM_FLAG_BOUND			(1 << 6)

/**
 * struct udevice - 一个驱动程序的实例
 *
 * 这个结构体保存有关设备的信息，设备是绑定到特定端口或外设的驱动程序（实际上是一个驱动程序实例）。
 *
 * 设备将通过 'bind' 调用而存在，这要么是由于 U_BOOT_DEVICE() 宏（在这种情况下 platdata 是非空的），
 * 要么是由于设备树中的一个节点（在这种情况下 of_offset >= 0）。在后一种情况下，我们将设备树信息转换为
 * platdata，这是由驱动程序的 ofdata_to_platdata 方法实现的（如果设备具有设备树节点，则在探测方法之前调用此方法）。
 *
 * platdata、priv 和 uclass_priv 这三个成员可以由驱动程序分配，或者您可以使用 struct driver 和
 * struct uclass_driver 的 auto_alloc_size 成员，使驱动程序模型自动执行此操作。
 *
 * @driver: 该设备使用的驱动程序
 * @name: 设备的名称，通常是 FDT 节点名称
 * @platdata: 该设备的配置数据
 * @parent_platdata: 该设备的父总线的配置数据
 * @uclass_platdata: 该设备的 uclass 的配置数据
 * @of_offset: 该设备的设备树节点偏移量（- 表示没有）
 * @driver_data: 与该设备匹配的条目的驱动程序数据字
 * @parent: 该设备的父级，对于顶级设备为 NULL
 * @priv: 该设备的私有数据
 * @uclass: 指向该设备的 uclass
 * @uclass_priv: 该设备的 uclass 的私有数据
 * @parent_priv: 该设备的父级的私有数据
 * @uclass_node: uclass 用于链接其设备的节点
 * @child_head: 该设备的子设备列表
 * @sibling_node: 所有设备列表中的下一个设备
 * @flags: 该设备的标志 DM_FLAG_...
 * @req_seq: 该设备的请求序号（-1 = 任何）
 * @seq: 该设备的分配序号（-1 = 无）。当设备被探测时，这会被设置，并且将在设备的 uclass 中是唯一的。
 * @devres_head: 与此设备相关联的内存分配列表。
 *   当启用 CONFIG_DEVRES 时，devm_kmalloc() 等将添加到此列表中。这样分配的内存将在设备被移除 / 解绑时自动释放。
 */
struct udevice {
    const struct driver *driver;        /**< 该设备使用的驱动程序 */
    const char *name;                   /**< 设备的名称，通常是 FDT 节点名称 */
    void *platdata;                     /**< 该设备的配置数据 */
    void *parent_platdata;              /**< 该设备的父总线的配置数据 */
    void *uclass_platdata;              /**< 该设备的 uclass 的配置数据 */
    int of_offset;                      /**< 该设备的设备树节点偏移量（- 表示没有） */
    ulong driver_data;                  /**< 与该设备匹配的条目的驱动程序数据字 */
    struct udevice *parent;             /**< 该设备的父级，对于顶级设备为 NULL */
    void *priv;                         /**< 该设备的私有数据 */
    struct uclass *uclass;              /**< 指向该设备的 uclass */
    void *uclass_priv;                  /**< 该设备的 uclass 的私有数据 */
    void *parent_priv;                  /**< 该设备的父级的私有数据 */
    struct list_head uclass_node;       /**< uclass 用于链接其设备的节点 */
    struct list_head child_head;        /**< 该设备的子设备列表 */
    struct list_head sibling_node;      /**< 所有设备列表中的下一个设备 */
    uint32_t flags;                     /**< 该设备的标志 DM_FLAG_... */
    int req_seq;                        /**< 该设备的请求序号（-1 = 任何） */
    int seq;                            /**< 该设备的分配序号（-1 = 无）。当设备被探测时，这会被设置，并且将在设备的 uclass 中是唯一的。 */
#ifdef CONFIG_DEVRES
    struct list_head devres_head;
#endif
};

/* Maximum sequence number supported */
#define DM_MAX_SEQ	999

/* Returns the operations for a device */
#define device_get_ops(dev)	(dev->driver->ops)

/* Returns non-zero if the device is active (probed and not removed) */
#define device_active(dev)	((dev)->flags & DM_FLAG_ACTIVATED)

/**
 * struct udevice_id - Lists the compatible strings supported by a driver
 * @compatible: Compatible string
 * @data: Data for this compatible string
 */
struct udevice_id {
	const char *compatible;
	ulong data;
};

#if CONFIG_IS_ENABLED(OF_CONTROL)
#define of_match_ptr(_ptr)	(_ptr)
#else
#define of_match_ptr(_ptr)	NULL
#endif /* CONFIG_IS_ENABLED(OF_CONTROL) */

/**
 * struct driver - 用于特性或外设的驱动程序
 *
 * 这个结构体保存了设置新设备和删除设备的方法。
 * 设备需要信息来设置自身 - 这些信息由 platdata 或设备树节点提供
 * （我们通过查找与 of_match 匹配的兼容字符串找到这些信息）。
 *
 * 所有驱动程序都属于一个 uclass，表示相同类型的设备的类别。
 * 驱动程序的公共元素可以在 uclass 中实现，或者 uclass 可以为其中的驱动程序提供一致的接口。
 *
 * @name: 设备名称
 * @id: 标识我们所属的 uclass
 * @of_match: 要匹配的兼容字符串列表，以及每个字符串的任何标识数据
 * @bind: 绑定设备到其驱动程序时调用
 * @probe: 探测设备，即激活设备时调用
 * @remove: 移除设备，即去激活设备时调用
 * @unbind: 解绑设备和其驱动程序时调用
 * @ofdata_to_platdata: 在探测之前调用以解码设备树数据
 * @child_post_bind: 在绑定新子设备后调用
 * @child_pre_probe: 在探测子设备之前调用。该设备已分配内存，但尚未探测。
 * @child_post_remove: 在移除子设备后调用。该设备已分配内存，但其 device_remove() 方法已被调用。
 * @priv_auto_alloc_size: 如果非零，则这是在设备的 ->priv 指针中分配的私有数据的大小。
 *   如果为零，则驱动程序负责分配所需的任何数据。
 * @platdata_auto_alloc_size: 如果非零，则这是在设备的 ->platdata 指针中分配的平台数据的大小。
 *   这通常只对设备树感知的驱动程序（具有 of_match 的驱动程序）有用，
 *   因为使用 platdata 的驱动程序将在 U_BOOT_DEVICE() 实例化中提供数据。
 * @per_child_auto_alloc_size: 每个设备可以持有其父级拥有的私有数据。如果需要，如果此值非零，则将自动分配。
 * @per_child_platdata_auto_alloc_size: 总线喜欢存储有关其子设备的信息。
 *   如果非零，则这是该数据的大小，要分配到子设备的 parent_platdata 指针中。
 * @ops: 驱动程序特定的操作。这通常是由驱动程序定义的函数指针列表，用于实现 uclass 所需的驱动程序功能。
 * @flags: 驱动程序标志 - 请参阅 DM_FLAGS_...
 */
struct driver {
	char *name;
	enum uclass_id id;
	const struct udevice_id *of_match;
	int (*bind)(struct udevice *dev);
	int (*probe)(struct udevice *dev);
	int (*remove)(struct udevice *dev);
	int (*unbind)(struct udevice *dev);
	int (*ofdata_to_platdata)(struct udevice *dev);
	int (*child_post_bind)(struct udevice *dev);
	int (*child_pre_probe)(struct udevice *dev);
	int (*child_post_remove)(struct udevice *dev);
	int priv_auto_alloc_size;
	int platdata_auto_alloc_size;
	int per_child_auto_alloc_size;
	int per_child_platdata_auto_alloc_size;
	const void *ops;	/* driver-specific operations */
	uint32_t flags;
};

/* Declare a new U-Boot driver */
#define U_BOOT_DRIVER(__name)						\
	ll_entry_declare(struct driver, __name, driver)

/**
 * dev_get_platdata() - Get the platform data for a device
 *
 * This checks that dev is not NULL, but no other checks for now
 *
 * @dev		Device to check
 * @return platform data, or NULL if none
 */
void *dev_get_platdata(struct udevice *dev);

/**
 * dev_get_parent_platdata() - Get the parent platform data for a device
 *
 * This checks that dev is not NULL, but no other checks for now
 *
 * @dev		Device to check
 * @return parent's platform data, or NULL if none
 */
void *dev_get_parent_platdata(struct udevice *dev);

/**
 * dev_get_uclass_platdata() - Get the uclass platform data for a device
 *
 * This checks that dev is not NULL, but no other checks for now
 *
 * @dev		Device to check
 * @return uclass's platform data, or NULL if none
 */
void *dev_get_uclass_platdata(struct udevice *dev);

/**
 * dev_get_priv() - Get the private data for a device
 *
 * This checks that dev is not NULL, but no other checks for now
 *
 * @dev		Device to check
 * @return private data, or NULL if none
 */
void *dev_get_priv(struct udevice *dev);

/**
 * dev_get_parent_priv() - Get the parent private data for a device
 *
 * The parent private data is data stored in the device but owned by the
 * parent. For example, a USB device may have parent data which contains
 * information about how to talk to the device over USB.
 *
 * This checks that dev is not NULL, but no other checks for now
 *
 * @dev		Device to check
 * @return parent data, or NULL if none
 */
void *dev_get_parent_priv(struct udevice *dev);

/**
 * dev_get_uclass_priv() - Get the private uclass data for a device
 *
 * This checks that dev is not NULL, but no other checks for now
 *
 * @dev		Device to check
 * @return private uclass data for this device, or NULL if none
 */
void *dev_get_uclass_priv(struct udevice *dev);

/**
 * struct dev_get_parent() - Get the parent of a device
 *
 * @child:	Child to check
 * @return parent of child, or NULL if this is the root device
 */
struct udevice *dev_get_parent(struct udevice *child);

/**
 * dev_get_driver_data() - get the driver data used to bind a device
 *
 * When a device is bound using a device tree node, it matches a
 * particular compatible string in struct udevice_id. This function
 * returns the associated data value for that compatible string. This is
 * the 'data' field in struct udevice_id.
 *
 * As an example, consider this structure:
 * static const struct udevice_id tegra_i2c_ids[] = {
 *	{ .compatible = "nvidia,tegra114-i2c", .data = TYPE_114 },
 *	{ .compatible = "nvidia,tegra20-i2c", .data = TYPE_STD },
 *	{ .compatible = "nvidia,tegra20-i2c-dvc", .data = TYPE_DVC },
 *	{ }
 * };
 *
 * When driver model finds a driver for this it will store the 'data' value
 * corresponding to the compatible string it matches. This function returns
 * that value. This allows the driver to handle several variants of a device.
 *
 * For USB devices, this is the driver_info field in struct usb_device_id.
 *
 * @dev:	Device to check
 * @return driver data (0 if none is provided)
 */
ulong dev_get_driver_data(struct udevice *dev);

/**
 * dev_get_driver_ops() - get the device's driver's operations
 *
 * This checks that dev is not NULL, and returns the pointer to device's
 * driver's operations.
 *
 * @dev:	Device to check
 * @return void pointer to driver's operations or NULL for NULL-dev or NULL-ops
 */
const void *dev_get_driver_ops(struct udevice *dev);

/**
 * device_get_uclass_id() - return the uclass ID of a device
 *
 * @dev:	Device to check
 * @return uclass ID for the device
 */
enum uclass_id device_get_uclass_id(struct udevice *dev);

/**
 * dev_get_uclass_name() - return the uclass name of a device
 *
 * This checks that dev is not NULL.
 *
 * @dev:	Device to check
 * @return  pointer to the uclass name for the device
 */
const char *dev_get_uclass_name(struct udevice *dev);

/**
 * device_get_child() - Get the child of a device by index
 *
 * Returns the numbered child, 0 being the first. This does not use
 * sequence numbers, only the natural order.
 *
 * @dev:	Parent device to check
 * @index:	Child index
 * @devp:	Returns pointer to device
 * @return 0 if OK, -ENODEV if no such device, other error if the device fails
 *	   to probe
 */
int device_get_child(struct udevice *parent, int index, struct udevice **devp);

/**
 * device_find_child_by_seq() - Find a child device based on a sequence
 *
 * This searches for a device with the given seq or req_seq.
 *
 * For seq, if an active device has this sequence it will be returned.
 * If there is no such device then this will return -ENODEV.
 *
 * For req_seq, if a device (whether activated or not) has this req_seq
 * value, that device will be returned. This is a strong indication that
 * the device will receive that sequence when activated.
 *
 * @parent: Parent device
 * @seq_or_req_seq: Sequence number to find (0=first)
 * @find_req_seq: true to find req_seq, false to find seq
 * @devp: Returns pointer to device (there is only one per for each seq).
 * Set to NULL if none is found
 * @return 0 if OK, -ve on error
 */
int device_find_child_by_seq(struct udevice *parent, int seq_or_req_seq,
			     bool find_req_seq, struct udevice **devp);

/**
 * device_get_child_by_seq() - Get a child device based on a sequence
 *
 * If an active device has this sequence it will be returned. If there is no
 * such device then this will check for a device that is requesting this
 * sequence.
 *
 * The device is probed to activate it ready for use.
 *
 * @parent: Parent device
 * @seq: Sequence number to find (0=first)
 * @devp: Returns pointer to device (there is only one per for each seq)
 * Set to NULL if none is found
 * @return 0 if OK, -ve on error
 */
int device_get_child_by_seq(struct udevice *parent, int seq,
			    struct udevice **devp);

/**
 * device_find_child_by_of_offset() - Find a child device based on FDT offset
 *
 * Locates a child device by its device tree offset.
 *
 * @parent: Parent device
 * @of_offset: Device tree offset to find
 * @devp: Returns pointer to device if found, otherwise this is set to NULL
 * @return 0 if OK, -ve on error
 */
int device_find_child_by_of_offset(struct udevice *parent, int of_offset,
				   struct udevice **devp);

/**
 * device_get_child_by_of_offset() - Get a child device based on FDT offset
 *
 * Locates a child device by its device tree offset.
 *
 * The device is probed to activate it ready for use.
 *
 * @parent: Parent device
 * @of_offset: Device tree offset to find
 * @devp: Returns pointer to device if found, otherwise this is set to NULL
 * @return 0 if OK, -ve on error
 */
int device_get_child_by_of_offset(struct udevice *parent, int of_offset,
				  struct udevice **devp);

/**
 * device_get_global_by_of_offset() - Get a device based on FDT offset
 *
 * Locates a device by its device tree offset, searching globally throughout
 * the all driver model devices.
 *
 * The device is probed to activate it ready for use.
 *
 * @of_offset: Device tree offset to find
 * @devp: Returns pointer to device if found, otherwise this is set to NULL
 * @return 0 if OK, -ve on error
 */
int device_get_global_by_of_offset(int of_offset, struct udevice **devp);

/**
 * device_find_first_child() - Find the first child of a device
 *
 * @parent: Parent device to search
 * @devp: Returns first child device, or NULL if none
 * @return 0
 */
int device_find_first_child(struct udevice *parent, struct udevice **devp);

/**
 * device_find_next_child() - Find the next child of a device
 *
 * @devp: Pointer to previous child device on entry. Returns pointer to next
 *		child device, or NULL if none
 * @return 0
 */
int device_find_next_child(struct udevice **devp);

/**
 * dev_get_addr() - Get the reg property of a device
 *
 * @dev: Pointer to a device
 *
 * @return addr
 */
fdt_addr_t dev_get_addr(struct udevice *dev);

/**
 * dev_get_addr_index() - Get the indexed reg property of a device
 *
 * @dev: Pointer to a device
 * @index: the 'reg' property can hold a list of <addr, size> pairs
 *	   and @index is used to select which one is required
 *
 * @return addr
 */
fdt_addr_t dev_get_addr_index(struct udevice *dev, int index);

/**
 * device_has_children() - check if a device has any children
 *
 * @dev:	Device to check
 * @return true if the device has one or more children
 */
bool device_has_children(struct udevice *dev);

/**
 * device_has_active_children() - check if a device has any active children
 *
 * @dev:	Device to check
 * @return true if the device has one or more children and at least one of
 * them is active (probed).
 */
bool device_has_active_children(struct udevice *dev);

/**
 * device_is_last_sibling() - check if a device is the last sibling
 *
 * This function can be useful for display purposes, when special action needs
 * to be taken when displaying the last sibling. This can happen when a tree
 * view of devices is being displayed.
 *
 * @dev:	Device to check
 * @return true if there are no more siblings after this one - i.e. is it
 * last in the list.
 */
bool device_is_last_sibling(struct udevice *dev);

/**
 * device_set_name() - set the name of a device
 *
 * This must be called in the device's bind() method and no later. Normally
 * this is unnecessary but for probed devices which don't get a useful name
 * this function can be helpful.
 *
 * @dev:	Device to update
 * @name:	New name (this string is allocated new memory and attached to
 *		the device)
 * @return 0 if OK, -ENOMEM if there is not enough memory to allocate the
 * string
 */
int device_set_name(struct udevice *dev, const char *name);

/**
 * device_is_on_pci_bus - Test if a device is on a PCI bus
 *
 * @dev:	device to test
 * @return:	true if it is on a PCI bus, false otherwise
 */
static inline bool device_is_on_pci_bus(struct udevice *dev)
{
	return device_get_uclass_id(dev->parent) == UCLASS_PCI;
}

/**
 * device_foreach_child_safe() - iterate through child devices safely
 *
 * This allows the @pos child to be removed in the loop if required.
 *
 * @pos: struct udevice * for the current device
 * @next: struct udevice * for the next device
 * @parent: parent device to scan
 */
#define device_foreach_child_safe(pos, next, parent)	\
	list_for_each_entry_safe(pos, next, &parent->child_head, sibling_node)

/* device resource management */
typedef void (*dr_release_t)(struct udevice *dev, void *res);
typedef int (*dr_match_t)(struct udevice *dev, void *res, void *match_data);

#ifdef CONFIG_DEVRES

#ifdef CONFIG_DEBUG_DEVRES
void *__devres_alloc(dr_release_t release, size_t size, gfp_t gfp,
		     const char *name);
#define _devres_alloc(release, size, gfp) \
	__devres_alloc(release, size, gfp, #release)
#else
void *_devres_alloc(dr_release_t release, size_t size, gfp_t gfp);
#endif

/**
 * devres_alloc() - Allocate device resource data
 * @release: Release function devres will be associated with
 * @size: Allocation size
 * @gfp: Allocation flags
 *
 * Allocate devres of @size bytes.  The allocated area is associated
 * with @release.  The returned pointer can be passed to
 * other devres_*() functions.
 *
 * RETURNS:
 * Pointer to allocated devres on success, NULL on failure.
 */
#define devres_alloc(release, size, gfp) \
	_devres_alloc(release, size, gfp | __GFP_ZERO)

/**
 * devres_free() - Free device resource data
 * @res: Pointer to devres data to free
 *
 * Free devres created with devres_alloc().
 */
void devres_free(void *res);

/**
 * devres_add() - Register device resource
 * @dev: Device to add resource to
 * @res: Resource to register
 *
 * Register devres @res to @dev.  @res should have been allocated
 * using devres_alloc().  On driver detach, the associated release
 * function will be invoked and devres will be freed automatically.
 */
void devres_add(struct udevice *dev, void *res);

/**
 * devres_find() - Find device resource
 * @dev: Device to lookup resource from
 * @release: Look for resources associated with this release function
 * @match: Match function (optional)
 * @match_data: Data for the match function
 *
 * Find the latest devres of @dev which is associated with @release
 * and for which @match returns 1.  If @match is NULL, it's considered
 * to match all.
 *
 * @return pointer to found devres, NULL if not found.
 */
void *devres_find(struct udevice *dev, dr_release_t release,
		  dr_match_t match, void *match_data);

/**
 * devres_get() - Find devres, if non-existent, add one atomically
 * @dev: Device to lookup or add devres for
 * @new_res: Pointer to new initialized devres to add if not found
 * @match: Match function (optional)
 * @match_data: Data for the match function
 *
 * Find the latest devres of @dev which has the same release function
 * as @new_res and for which @match return 1.  If found, @new_res is
 * freed; otherwise, @new_res is added atomically.
 *
 * @return ointer to found or added devres.
 */
void *devres_get(struct udevice *dev, void *new_res,
		 dr_match_t match, void *match_data);

/**
 * devres_remove() - Find a device resource and remove it
 * @dev: Device to find resource from
 * @release: Look for resources associated with this release function
 * @match: Match function (optional)
 * @match_data: Data for the match function
 *
 * Find the latest devres of @dev associated with @release and for
 * which @match returns 1.  If @match is NULL, it's considered to
 * match all.  If found, the resource is removed atomically and
 * returned.
 *
 * @return ointer to removed devres on success, NULL if not found.
 */
void *devres_remove(struct udevice *dev, dr_release_t release,
		    dr_match_t match, void *match_data);

/**
 * devres_destroy() - Find a device resource and destroy it
 * @dev: Device to find resource from
 * @release: Look for resources associated with this release function
 * @match: Match function (optional)
 * @match_data: Data for the match function
 *
 * Find the latest devres of @dev associated with @release and for
 * which @match returns 1.  If @match is NULL, it's considered to
 * match all.  If found, the resource is removed atomically and freed.
 *
 * Note that the release function for the resource will not be called,
 * only the devres-allocated data will be freed.  The caller becomes
 * responsible for freeing any other data.
 *
 * @return 0 if devres is found and freed, -ENOENT if not found.
 */
int devres_destroy(struct udevice *dev, dr_release_t release,
		   dr_match_t match, void *match_data);

/**
 * devres_release() - Find a device resource and destroy it, calling release
 * @dev: Device to find resource from
 * @release: Look for resources associated with this release function
 * @match: Match function (optional)
 * @match_data: Data for the match function
 *
 * Find the latest devres of @dev associated with @release and for
 * which @match returns 1.  If @match is NULL, it's considered to
 * match all.  If found, the resource is removed atomically, the
 * release function called and the resource freed.
 *
 * @return 0 if devres is found and freed, -ENOENT if not found.
 */
int devres_release(struct udevice *dev, dr_release_t release,
		   dr_match_t match, void *match_data);

/* managed devm_k.alloc/kfree for device drivers */
/**
 * devm_kmalloc() - Resource-managed kmalloc
 * @dev: Device to allocate memory for
 * @size: Allocation size
 * @gfp: Allocation gfp flags
 *
 * Managed kmalloc.  Memory allocated with this function is
 * automatically freed on driver detach.  Like all other devres
 * resources, guaranteed alignment is unsigned long long.
 *
 * @return pointer to allocated memory on success, NULL on failure.
 */
void *devm_kmalloc(struct udevice *dev, size_t size, gfp_t gfp);
static inline void *devm_kzalloc(struct udevice *dev, size_t size, gfp_t gfp)
{
	return devm_kmalloc(dev, size, gfp | __GFP_ZERO);
}
static inline void *devm_kmalloc_array(struct udevice *dev,
				       size_t n, size_t size, gfp_t flags)
{
	if (size != 0 && n > SIZE_MAX / size)
		return NULL;
	return devm_kmalloc(dev, n * size, flags);
}
static inline void *devm_kcalloc(struct udevice *dev,
				 size_t n, size_t size, gfp_t flags)
{
	return devm_kmalloc_array(dev, n, size, flags | __GFP_ZERO);
}

/**
 * devm_kfree() - Resource-managed kfree
 * @dev: Device this memory belongs to
 * @ptr: Memory to free
 *
 * Free memory allocated with devm_kmalloc().
 */
void devm_kfree(struct udevice *dev, void *ptr);

#else /* ! CONFIG_DEVRES */

static inline void *devres_alloc(dr_release_t release, size_t size, gfp_t gfp)
{
	return kzalloc(size, gfp);
}

static inline void devres_free(void *res)
{
	kfree(res);
}

static inline void devres_add(struct udevice *dev, void *res)
{
}

static inline void *devres_find(struct udevice *dev, dr_release_t release,
				dr_match_t match, void *match_data)
{
	return NULL;
}

static inline void *devres_get(struct udevice *dev, void *new_res,
			       dr_match_t match, void *match_data)
{
	return NULL;
}

static inline void *devres_remove(struct udevice *dev, dr_release_t release,
				  dr_match_t match, void *match_data)
{
	return NULL;
}

static inline int devres_destroy(struct udevice *dev, dr_release_t release,
				 dr_match_t match, void *match_data)
{
	return 0;
}

static inline int devres_release(struct udevice *dev, dr_release_t release,
				 dr_match_t match, void *match_data)
{
	return 0;
}

static inline void *devm_kmalloc(struct udevice *dev, size_t size, gfp_t gfp)
{
	return kmalloc(size, gfp);
}

static inline void *devm_kzalloc(struct udevice *dev, size_t size, gfp_t gfp)
{
	return kzalloc(size, gfp);
}

static inline void *devm_kmaloc_array(struct udevice *dev,
				      size_t n, size_t size, gfp_t flags)
{
	/* TODO: add kmalloc_array() to linux/compat.h */
	if (size != 0 && n > SIZE_MAX / size)
		return NULL;
	return kmalloc(n * size, flags);
}

static inline void *devm_kcalloc(struct udevice *dev,
				 size_t n, size_t size, gfp_t flags)
{
	/* TODO: add kcalloc() to linux/compat.h */
	return kmalloc(n * size, flags | __GFP_ZERO);
}

static inline void devm_kfree(struct udevice *dev, void *ptr)
{
	kfree(ptr);
}

#endif /* ! CONFIG_DEVRES */

/**
 * dm_set_translation_offset() - Set translation offset
 * @offs: Translation offset
 *
 * Some platforms need a special address translation. Those
 * platforms (e.g. mvebu in SPL) can configure a translation
 * offset in the DM by calling this function. It will be
 * added to all addresses returned in dev_get_addr().
 */
void dm_set_translation_offset(fdt_addr_t offs);

/**
 * dm_get_translation_offset() - Get translation offset
 *
 * This function returns the translation offset that can
 * be configured by calling dm_set_translation_offset().
 *
 * @return translation offset for the device address (0 as default).
 */
fdt_addr_t dm_get_translation_offset(void);

#endif
