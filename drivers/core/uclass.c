/*
 * Copyright (c) 2013 Google, Inc
 *
 * (C) Copyright 2012
 * Pavel Herrmann <morpheus.ibis@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <dm/device.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>
#include <dm/util.h>

DECLARE_GLOBAL_DATA_PTR;

struct uclass *uclass_find(enum uclass_id key)
{
	struct uclass *uc;

	if (!gd->dm_root)
		return NULL;
	/*
	 * TODO(sjg@chromium.org): Optimise this, perhaps moving the found
	 * node to the start of the list, or creating a linear array mapping
	 * id to node.
	 */
	list_for_each_entry(uc, &gd->uclass_root, sibling_node) {
		if (uc->uc_drv->id == key)
			return uc;
	}

	return NULL;
}

/**
 * uclass_add() - 在列表中创建新的 uclass
 * @id: 要创建的 ID 编号
 * @ucp: 返回指向 uclass 的指针，出错时返回 NULL
 * @return 成功返回 0，出错返回负值
 *
 * 新的 uclass 被添加到列表中。每个 id 只能有一个 uclass。
 */
static int uclass_add(enum uclass_id id, struct uclass **ucp)
{
	struct uclass_driver *uc_drv;
	struct uclass *uc;
	int ret;

	*ucp = NULL;
	uc_drv = lists_uclass_lookup(id);
	if (!uc_drv) {
		debug("Cannot find uclass for id %d: please add the UCLASS_DRIVER() declaration for this UCLASS_... id\n",
		      id);
		/*
		 * Use a strange error to make this case easier to find. When
		 * a uclass is not available it can prevent driver model from
		 * starting up and this failure is otherwise hard to debug.
		 */
		return -EPFNOSUPPORT;
	}
	uc = calloc(1, sizeof(*uc));
	if (!uc)
		return -ENOMEM;
	if (uc_drv->priv_auto_alloc_size) {
		uc->priv = calloc(1, uc_drv->priv_auto_alloc_size);
		if (!uc->priv) {
			ret = -ENOMEM;
			goto fail_mem;
		}
	}
	uc->uc_drv = uc_drv;
	INIT_LIST_HEAD(&uc->sibling_node);
	INIT_LIST_HEAD(&uc->dev_head);
	list_add(&uc->sibling_node, &DM_UCLASS_ROOT_NON_CONST);

	if (uc_drv->init) {
		ret = uc_drv->init(uc);
		if (ret)
			goto fail;
	}

	*ucp = uc;

	return 0;
fail:
	if (uc_drv->priv_auto_alloc_size) {
		free(uc->priv);
		uc->priv = NULL;
	}
	list_del(&uc->sibling_node);
fail_mem:
	free(uc);

	return ret;
}

int uclass_destroy(struct uclass *uc)
{
	struct uclass_driver *uc_drv;
	struct udevice *dev;
	int ret;

	/*
	 * We cannot use list_for_each_entry_safe() here. If a device in this
	 * uclass has a child device also in this uclass, it will be also be
	 * unbound (by the recursion in the call to device_unbind() below).
	 * We can loop until the list is empty.
	 */
	while (!list_empty(&uc->dev_head)) {
		dev = list_first_entry(&uc->dev_head, struct udevice,
				       uclass_node);
		ret = device_remove(dev);
		if (ret)
			return ret;
		ret = device_unbind(dev);
		if (ret)
			return ret;
	}

	uc_drv = uc->uc_drv;
	if (uc_drv->destroy)
		uc_drv->destroy(uc);
	list_del(&uc->sibling_node);
	if (uc_drv->priv_auto_alloc_size)
		free(uc->priv);
	free(uc);

	return 0;
}

int uclass_get(enum uclass_id id, struct uclass **ucp)
{
	struct uclass *uc;

	*ucp = NULL;
	uc = uclass_find(id);
	if (!uc)
		return uclass_add(id, ucp);
	*ucp = uc;

	return 0;
}

int uclass_find_device(enum uclass_id id, int index, struct udevice **devp)
{
	struct uclass *uc;
	struct udevice *dev;
	int ret;

	*devp = NULL;
	ret = uclass_get(id, &uc);
	if (ret)
		return ret;
	if (list_empty(&uc->dev_head))
		return -ENODEV;

	list_for_each_entry(dev, &uc->dev_head, uclass_node) {
		if (!index--) {
			*devp = dev;
			return 0;
		}
	}

	return -ENODEV;
}

int uclass_find_first_device(enum uclass_id id, struct udevice **devp)
{
	struct uclass *uc;
	int ret;

	*devp = NULL;
	ret = uclass_get(id, &uc);
	if (ret)
		return ret;
	if (list_empty(&uc->dev_head))
		return 0;

	*devp = list_first_entry(&uc->dev_head, struct udevice, uclass_node);

	return 0;
}

int uclass_find_next_device(struct udevice **devp)
{
	struct udevice *dev = *devp;

	*devp = NULL;
	if (list_is_last(&dev->uclass_node, &dev->uclass->dev_head))
		return 0;

	*devp = list_entry(dev->uclass_node.next, struct udevice, uclass_node);

	return 0;
}

int uclass_find_device_by_name(enum uclass_id id, const char *name,
			       struct udevice **devp)
{
	struct uclass *uc;
	struct udevice *dev;
	int ret;

	*devp = NULL;
	if (!name)
		return -EINVAL;
	ret = uclass_get(id, &uc);
	if (ret)
		return ret;

	list_for_each_entry(dev, &uc->dev_head, uclass_node) {
		if (!strncmp(dev->name, name, strlen(name))) {
			*devp = dev;
			return 0;
		}
	}

	return -ENODEV;
}

int uclass_find_device_by_seq(enum uclass_id id, int seq_or_req_seq,
			      bool find_req_seq, struct udevice **devp)
{
	struct uclass *uc;
	struct udevice *dev;
	int ret;

	*devp = NULL;
	debug("%s: %d %d\n", __func__, find_req_seq, seq_or_req_seq);
	if (seq_or_req_seq == -1)
		return -ENODEV;
	ret = uclass_get(id, &uc);
	if (ret)
		return ret;

	list_for_each_entry(dev, &uc->dev_head, uclass_node) {
		debug("   - %d %d\n", dev->req_seq, dev->seq);
		if ((find_req_seq ? dev->req_seq : dev->seq) ==
				seq_or_req_seq) {
			*devp = dev;
			debug("   - found\n");
			return 0;
		}
	}
	debug("   - not found\n");

	return -ENODEV;
}

int uclass_find_device_by_of_offset(enum uclass_id id, int node,
				    struct udevice **devp)
{
	struct uclass *uc;
	struct udevice *dev;
	int ret;

	*devp = NULL;
	if (node < 0)
		return -ENODEV;
	ret = uclass_get(id, &uc);
	if (ret)
		return ret;

	list_for_each_entry(dev, &uc->dev_head, uclass_node) {
		if (dev->of_offset == node) {
			*devp = dev;
			return 0;
		}
	}

	return -ENODEV;
}

#if CONFIG_IS_ENABLED(OF_CONTROL)
static int uclass_find_device_by_phandle(enum uclass_id id,
					 struct udevice *parent,
					 const char *name,
					 struct udevice **devp)
{
	struct udevice *dev;
	struct uclass *uc;
	int find_phandle;
	int ret;

	*devp = NULL;
	find_phandle = fdtdec_get_int(gd->fdt_blob, parent->of_offset, name,
				      -1);
	if (find_phandle <= 0)
		return -ENOENT;
	ret = uclass_get(id, &uc);
	if (ret)
		return ret;

	list_for_each_entry(dev, &uc->dev_head, uclass_node) {
		uint phandle = fdt_get_phandle(gd->fdt_blob, dev->of_offset);

		if (phandle == find_phandle) {
			*devp = dev;
			return 0;
		}
	}

	return -ENODEV;
}
#endif

int uclass_get_device_tail(struct udevice *dev, int ret,
				  struct udevice **devp)
{
	if (ret)
		return ret;

	assert(dev);
	ret = device_probe(dev);
	if (ret)
		return ret;

	*devp = dev;

	return 0;
}

int uclass_get_device(enum uclass_id id, int index, struct udevice **devp)
{
	struct udevice *dev;
	int ret;

	*devp = NULL;
	ret = uclass_find_device(id, index, &dev);
	return uclass_get_device_tail(dev, ret, devp);
}

int uclass_get_device_by_name(enum uclass_id id, const char *name,
			      struct udevice **devp)
{
	struct udevice *dev;
	int ret;

	*devp = NULL;
	ret = uclass_find_device_by_name(id, name, &dev);
	return uclass_get_device_tail(dev, ret, devp);
}

int uclass_get_device_by_seq(enum uclass_id id, int seq, struct udevice **devp)
{
	struct udevice *dev;
	int ret;

	*devp = NULL;
	ret = uclass_find_device_by_seq(id, seq, false, &dev);
	if (ret == -ENODEV) {
		/*
		 * We didn't find it in probed devices. See if there is one
		 * that will request this seq if probed.
		 */
		ret = uclass_find_device_by_seq(id, seq, true, &dev);
	}
	return uclass_get_device_tail(dev, ret, devp);
}

int uclass_get_device_by_of_offset(enum uclass_id id, int node,
				   struct udevice **devp)
{
	struct udevice *dev;
	int ret;

	*devp = NULL;
	ret = uclass_find_device_by_of_offset(id, node, &dev);
	return uclass_get_device_tail(dev, ret, devp);
}

#if CONFIG_IS_ENABLED(OF_CONTROL)
int uclass_get_device_by_phandle(enum uclass_id id, struct udevice *parent,
				 const char *name, struct udevice **devp)
{
	struct udevice *dev;
	int ret;

	*devp = NULL;
	ret = uclass_find_device_by_phandle(id, parent, name, &dev);
	return uclass_get_device_tail(dev, ret, devp);
}
#endif

int uclass_first_device(enum uclass_id id, struct udevice **devp)
{
	struct udevice *dev;
	int ret;

	*devp = NULL;
	ret = uclass_find_first_device(id, &dev);
	if (!dev)
		return 0;
	return uclass_get_device_tail(dev, ret, devp);
}

int uclass_next_device(struct udevice **devp)
{
	struct udevice *dev = *devp;
	int ret;

	*devp = NULL;
	ret = uclass_find_next_device(&dev);
	if (!dev)
		return 0;
	return uclass_get_device_tail(dev, ret, devp);
}

int uclass_bind_device(struct udevice *dev)
{
	struct uclass *uc;
	int ret;

	uc = dev->uclass;
	list_add_tail(&dev->uclass_node, &uc->dev_head);

	if (dev->parent) {
		struct uclass_driver *uc_drv = dev->parent->uclass->uc_drv;

		if (uc_drv->child_post_bind) {
			ret = uc_drv->child_post_bind(dev);
			if (ret)
				goto err;
		}
	}

	return 0;
err:
	/* There is no need to undo the parent's post_bind call */
	list_del(&dev->uclass_node);

	return ret;
}

#if CONFIG_IS_ENABLED(DM_DEVICE_REMOVE)
int uclass_unbind_device(struct udevice *dev)
{
	struct uclass *uc;
	int ret;

	uc = dev->uclass;
	if (uc->uc_drv->pre_unbind) {
		ret = uc->uc_drv->pre_unbind(dev);
		if (ret)
			return ret;
	}

	list_del(&dev->uclass_node);
	return 0;
}
#endif

int uclass_resolve_seq(struct udevice *dev)
{
	struct udevice *dup;
	int seq;
	int ret;

	assert(dev->seq == -1);
	ret = uclass_find_device_by_seq(dev->uclass->uc_drv->id, dev->req_seq,
					false, &dup);
	if (!ret) {
		dm_warn("Device '%s': seq %d is in use by '%s'\n",
			dev->name, dev->req_seq, dup->name);
	} else if (ret == -ENODEV) {
		/* Our requested sequence number is available */
		if (dev->req_seq != -1)
			return dev->req_seq;
	} else {
		return ret;
	}

	for (seq = 0; seq < DM_MAX_SEQ; seq++) {
		ret = uclass_find_device_by_seq(dev->uclass->uc_drv->id, seq,
						false, &dup);
		if (ret == -ENODEV)
			break;
		if (ret)
			return ret;
	}
	return seq;
}

int uclass_pre_probe_device(struct udevice *dev)
{
	struct uclass_driver *uc_drv;
	int ret;

	uc_drv = dev->uclass->uc_drv;
	if (uc_drv->pre_probe) {
		ret = uc_drv->pre_probe(dev);
		if (ret)
			return ret;
	}

	if (!dev->parent)
		return 0;
	uc_drv = dev->parent->uclass->uc_drv;
	if (uc_drv->child_pre_probe)
		return uc_drv->child_pre_probe(dev);

	return 0;
}

int uclass_post_probe_device(struct udevice *dev)
{
	struct uclass_driver *uc_drv = dev->uclass->uc_drv;

	if (uc_drv->post_probe)
		return uc_drv->post_probe(dev);

	return 0;
}

#if CONFIG_IS_ENABLED(DM_DEVICE_REMOVE)
int uclass_pre_remove_device(struct udevice *dev)
{
	struct uclass *uc;
	int ret;

	uc = dev->uclass;
	if (uc->uc_drv->pre_remove) {
		ret = uc->uc_drv->pre_remove(dev);
		if (ret)
			return ret;
	}

	return 0;
}
#endif
