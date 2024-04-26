/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <config.h>
#include <errno.h>
#include <common.h>
#include <mapmem.h>
#include <part.h>
#include <ext4fs.h>
#include <fat.h>
#include <fs.h>
#include <sandboxfs.h>
#include <ubifs_uboot.h>
#include <asm/io.h>
#include <div64.h>
#include <linux/math64.h>

DECLARE_GLOBAL_DATA_PTR;

static block_dev_desc_t *fs_dev_desc;
static disk_partition_t fs_partition;
static int fs_type = FS_TYPE_ANY;

static inline int fs_probe_unsupported(block_dev_desc_t *fs_dev_desc,
				      disk_partition_t *fs_partition)
{
	printf("** Unrecognized filesystem type **\n");
	return -1;
}

static inline int fs_ls_unsupported(const char *dirname)
{
	return -1;
}

static inline int fs_exists_unsupported(const char *filename)
{
	return 0;
}

static inline int fs_size_unsupported(const char *filename, loff_t *size)
{
	return -1;
}

static inline int fs_read_unsupported(const char *filename, void *buf,
				      loff_t offset, loff_t len,
				      loff_t *actread)
{
	return -1;
}

static inline int fs_write_unsupported(const char *filename, void *buf,
				      loff_t offset, loff_t len,
				      loff_t *actwrite)
{
	return -1;
}

static inline void fs_close_unsupported(void)
{
}

static inline int fs_uuid_unsupported(char *uuid_str)
{
	return -1;
}

struct fstype_info {
	int fstype;
	char *name;
	/*
	 * Is it legal to pass NULL as .probe()'s  fs_dev_desc parameter? This
	 * should be false in most cases. For "virtual" filesystems which
	 * aren't based on a U-Boot block device (e.g. sandbox), this can be
	 * set to true. This should also be true for the dumm entry at the end
	 * of fstypes[], since that is essentially a "virtual" (non-existent)
	 * filesystem.
	 */
	bool null_dev_desc_ok;
	int (*probe)(block_dev_desc_t *fs_dev_desc,
		     disk_partition_t *fs_partition);
	int (*ls)(const char *dirname);
	int (*exists)(const char *filename);
	int (*size)(const char *filename, loff_t *size);
	int (*read)(const char *filename, void *buf, loff_t offset,
		    loff_t len, loff_t *actread);
	int (*write)(const char *filename, void *buf, loff_t offset,
		     loff_t len, loff_t *actwrite);
	void (*close)(void);
	int (*uuid)(char *uuid_str);
};

static struct fstype_info fstypes[] = {
#ifdef CONFIG_FS_FAT
	{
		.fstype = FS_TYPE_FAT,
		.name = "fat",
		.null_dev_desc_ok = false,
		.probe = fat_set_blk_dev,
		.close = fat_close,
		.ls = file_fat_ls,
		.exists = fat_exists,
		.size = fat_size,
		.read = fat_read_file,
#ifdef CONFIG_FAT_WRITE
		.write = file_fat_write,
#else
		.write = fs_write_unsupported,
#endif
		.uuid = fs_uuid_unsupported,
	},
#endif
#ifdef CONFIG_FS_EXT4
	{
		.fstype = FS_TYPE_EXT,
		.name = "ext4",
		.null_dev_desc_ok = false,
		.probe = ext4fs_probe,
		.close = ext4fs_close,
		.ls = ext4fs_ls,
		.exists = ext4fs_exists,
		.size = ext4fs_size,
		.read = ext4_read_file,
#ifdef CONFIG_CMD_EXT4_WRITE
		.write = ext4_write_file,
#else
		.write = fs_write_unsupported,
#endif
		.uuid = ext4fs_uuid,
	},
#endif
#ifdef CONFIG_SANDBOX
	{
		.fstype = FS_TYPE_SANDBOX,
		.name = "sandbox",
		.null_dev_desc_ok = true,
		.probe = sandbox_fs_set_blk_dev,
		.close = sandbox_fs_close,
		.ls = sandbox_fs_ls,
		.exists = sandbox_fs_exists,
		.size = sandbox_fs_size,
		.read = fs_read_sandbox,
		.write = fs_write_sandbox,
		.uuid = fs_uuid_unsupported,
	},
#endif
#ifdef CONFIG_CMD_UBIFS
	{
		.fstype = FS_TYPE_UBIFS,
		.name = "ubifs",
		.null_dev_desc_ok = true,
		.probe = ubifs_set_blk_dev,
		.close = ubifs_close,
		.ls = ubifs_ls,
		.exists = ubifs_exists,
		.size = ubifs_size,
		.read = ubifs_read,
		.write = fs_write_unsupported,
		.uuid = fs_uuid_unsupported,
	},
#endif
	{
		.fstype = FS_TYPE_ANY,
		.name = "unsupported",
		.null_dev_desc_ok = true,
		.probe = fs_probe_unsupported,
		.close = fs_close_unsupported,
		.ls = fs_ls_unsupported,
		.exists = fs_exists_unsupported,
		.size = fs_size_unsupported,
		.read = fs_read_unsupported,
		.write = fs_write_unsupported,
		.uuid = fs_uuid_unsupported,
	},
};

static struct fstype_info *fs_get_info(int fstype)
{
	struct fstype_info *info;
	int i;

	for (i = 0, info = fstypes; i < ARRAY_SIZE(fstypes) - 1; i++, info++) {
		if (fstype == info->fstype)
			return info;
	}

	/* Return the 'unsupported' sentinel */
	return info;
}

int fs_set_blk_dev(const char *ifname, const char *dev_part_str, int fstype)
{
	struct fstype_info *info;
	int part, i;
#ifdef CONFIG_NEEDS_MANUAL_RELOC //需要手动重定位
	static int relocated;
	//将fstypes数组更新为重定向之后的值
	if (!relocated) {
		for (i = 0, info = fstypes; i < ARRAY_SIZE(fstypes);
				i++, info++) {
			info->name += gd->reloc_off;
			info->probe += gd->reloc_off;
			info->close += gd->reloc_off;
			info->ls += gd->reloc_off;
			info->read += gd->reloc_off;
			info->write += gd->reloc_off;
		}
		relocated = 1;
	}
#endif
	// 从用户输入（eg: mmc 1:2）获取对应的块设备信息和分区信息，保存到全局变量fs_dev_desc和fs_partition
	// 返回值part为分区数量
	part = get_device_and_partition(ifname, dev_part_str, &fs_dev_desc,
					&fs_partition, 1);
	if (part < 0)
		return -1;
	// 遍历fstypes数组
	for (i = 0, info = fstypes; i < ARRAY_SIZE(fstypes); i++, info++) {
		// 如果传入的fstype不为FS_TYPE_ANY，则需要在fstypes数组中找到对应的文件系统类型
		if (fstype != FS_TYPE_ANY && info->fstype != FS_TYPE_ANY &&
				fstype != info->fstype)
			continue;
		//对应文件系统的的probe函数是否需要dev_desc不为NULL
		if (!fs_dev_desc && !info->null_dev_desc_ok)
			continue;
		//调用文件系统的probe函数，根据fs_dev_desc和fs_partition判断是否是对应的文件系统
		if (!info->probe(fs_dev_desc, &fs_partition)) {
			fs_type = info->fstype;   //更新全局变量fs_type
			return 0;
		}
	}

	return -1;
}

static void fs_close(void)
{
	struct fstype_info *info = fs_get_info(fs_type);

	info->close();

	fs_type = FS_TYPE_ANY;
}

int fs_uuid(char *uuid_str)
{
	struct fstype_info *info = fs_get_info(fs_type);

	return info->uuid(uuid_str);
}

int fs_ls(const char *dirname)
{
	int ret;

	struct fstype_info *info = fs_get_info(fs_type);

	ret = info->ls(dirname);

	fs_type = FS_TYPE_ANY;
	fs_close();

	return ret;
}

int fs_exists(const char *filename)
{
	int ret;

	struct fstype_info *info = fs_get_info(fs_type);

	ret = info->exists(filename);

	fs_close();

	return ret;
}

int fs_size(const char *filename, loff_t *size)
{
	int ret;

	struct fstype_info *info = fs_get_info(fs_type);

	ret = info->size(filename, size);

	fs_close();

	return ret;
}

int fs_read(const char *filename, ulong addr, loff_t offset, loff_t len,
	    loff_t *actread)
{
	struct fstype_info *info = fs_get_info(fs_type);
	void *buf;
	int ret;

	/*
	 * We don't actually know how many bytes are being read, since len==0
	 * means read the whole file.
	 */
	buf = map_sysmem(addr, len);
	ret = info->read(filename, buf, offset, len, actread);
	unmap_sysmem(buf);

	/* If we requested a specific number of bytes, check we got it */
	if (ret == 0 && len && *actread != len)
		printf("** %s shorter than offset + len **\n", filename);
	fs_close();

	return ret;
}

int fs_write(const char *filename, ulong addr, loff_t offset, loff_t len,
	     loff_t *actwrite)
{
	struct fstype_info *info = fs_get_info(fs_type);
	void *buf;
	int ret;

	buf = map_sysmem(addr, len);
	ret = info->write(filename, buf, offset, len, actwrite);
	unmap_sysmem(buf);

	if (ret < 0 && len != *actwrite) {
		printf("** Unable to write file %s **\n", filename);
		ret = -1;
	}
	fs_close();

	return ret;
}

int do_size(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[],
		int fstype)
{
	loff_t size;

	//参数个数检查
	if (argc != 4)
		return CMD_RET_USAGE;
	// 根据用户输入的 interface 和 dev[:part]（eg: mmc 1:2）
	// 1. 更新fs/fs.c下的全局变量fs_dev_desc和fs_partition，这两个变量分别表示当前选择的块设备信息和分区信息
	// 2. 遍历fstypes数组，找到fstype对应的struct fstype_info元素
	// 3. 调用对应文件系统的probe函数，判断fs_dev_desc和fs_partition对应的块设备和分区上是否是对应的文件系统
	if (fs_set_blk_dev(argv[1], argv[2], fstype))
		return 1;
	// 调用对应文件系统的size函数，获取size
	if (fs_size(argv[3], &size) < 0)
		return CMD_RET_FAILURE;
	// 结果写入环境量filesize
	setenv_hex("filesize", size);

	return 0;
}

int do_load(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[],
		int fstype)
{
	unsigned long addr;		//加载到内存的地址
	const char *addr_str;	//内存的地址字符串变量
	const char *filename;	//被加载的文件绝对路径
	loff_t bytes;			//加载的字节数
	loff_t pos;				//读的文件相对于文件首地址的偏移
	loff_t len_read;
	int ret;
	unsigned long time;
	char *ep;

	//参数个数检查
	if (argc < 2)
		return CMD_RET_USAGE;
	if (argc > 7)
		return CMD_RET_USAGE;

	// 获取块设备信息和分区信息，保存到全局变量fs_dev_desc和fs_partition，并执行对应文件系统的probe函数
	if (fs_set_blk_dev(argv[1], (argc >= 3) ? argv[2] : NULL, fstype))
		return 1;

	//参数数量大于等于4个，则第4个参数必须为加载到内存的地址addr
	if (argc >= 4) {
		//将第4个参数转换为ul类型
		addr = simple_strtoul(argv[3], &ep, 16);
		//没有发生转换或者转换结束后剩下的字符串第一个字符不为'\0',则输入参数有误
		if (ep == argv[3] || *ep != '\0')
			return CMD_RET_USAGE;
	} else {
		//参数数量小于4个，则从环境变量loadaddr里获取loadaddr
		addr_str = getenv("loadaddr");
		if (addr_str != NULL)
			addr = simple_strtoul(addr_str, NULL, 16);
		else
			addr = CONFIG_SYS_LOAD_ADDR;
	}
	//参数数量大于等于5个, 第5个参数必须为文件的绝对路径名称filename
	if (argc >= 5) {
		filename = argv[4];
	} else {
		//否则从环境变量bootfile获取filename
		filename = getenv("bootfile");
		if (!filename) {
			puts("** No boot file defined **\n");
			return 1;
		}
	}
	if (argc >= 6)//参数数量大于等于6个, 第6个参数必须为加载字节数量bytes
		bytes = simple_strtoul(argv[5], NULL, 16);
	else
		bytes = 0;//bytes默认值为0，表示读取整个文件
	if (argc >= 7)//参数数量大于等于7个, 第7个参数必须为相对于文件首地址的偏移pos
		pos = simple_strtoul(argv[6], NULL, 16);
	else
		pos = 0;//pos默认值为0，表示从文件首地址开始读取

	time = get_timer(0);//获取此时的时间
	//调用fs_read，稍后讲解
	ret = fs_read(filename, addr, pos, bytes, &len_read);
	time = get_timer(time);//计算read所消耗的时间
	if (ret < 0)
		return 1;

	//显示命令执行信息
	printf("%llu bytes read in %lu ms", len_read, time);
	if (time > 0) {
		puts(" (");
		print_size(div_u64(len_read, time) * 1000, "/s");
		puts(")");
	}
	puts("\n");

	//保存加载目的地址addr和读取到的长度len_read到环境变量fileaddr和filesize
	setenv_hex("fileaddr", addr);
	setenv_hex("filesize", len_read);

	return 0;
}

int do_ls(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[],
	int fstype)
{
	if (argc < 2)
		return CMD_RET_USAGE;
	if (argc > 4)
		return CMD_RET_USAGE;

	if (fs_set_blk_dev(argv[1], (argc >= 3) ? argv[2] : NULL, fstype))
		return 1;
	//不给出目录的话，默认为/
	if (fs_ls(argc >= 4 ? argv[3] : "/"))
		return 1;

	return 0;
}

int file_exists(const char *dev_type, const char *dev_part, const char *file,
		int fstype)
{
	if (fs_set_blk_dev(dev_type, dev_part, fstype))
		return 0;

	return fs_exists(file);
}

int do_save(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[],
		int fstype)
{
	unsigned long addr;
	const char *filename;
	loff_t bytes;
	loff_t pos;
	loff_t len;
	int ret;
	unsigned long time;

	if (argc < 6 || argc > 7)
		return CMD_RET_USAGE;

	if (fs_set_blk_dev(argv[1], argv[2], fstype))
		return 1;

	addr = simple_strtoul(argv[3], NULL, 16);
	filename = argv[4];
	bytes = simple_strtoul(argv[5], NULL, 16);
	if (argc >= 7)
		pos = simple_strtoul(argv[6], NULL, 16);
	else
		pos = 0;

	time = get_timer(0);
	ret = fs_write(filename, addr, pos, bytes, &len);
	time = get_timer(time);
	if (ret < 0)
		return 1;

	printf("%llu bytes written in %lu ms", len, time);
	if (time > 0) {
		puts(" (");
		print_size(div_u64(len, time) * 1000, "/s");
		puts(")");
	}
	puts("\n");

	return 0;
}

int do_fs_uuid(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[],
		int fstype)
{
	int ret;
	char uuid[37];
	memset(uuid, 0, sizeof(uuid));

	if (argc < 3 || argc > 4)
		return CMD_RET_USAGE;

	if (fs_set_blk_dev(argv[1], argv[2], fstype))
		return 1;

	ret = fs_uuid(uuid);
	if (ret)
		return CMD_RET_FAILURE;

	if (argc == 4)
		setenv(argv[3], uuid);
	else
		printf("%s\n", uuid);

	return CMD_RET_SUCCESS;
}

int do_fs_type(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct fstype_info *info;

	if (argc < 3 || argc > 4)
		return CMD_RET_USAGE;
	// 获取块设备信息和分区信息，保存到全局变量fs_dev_desc和fs_partition，并执行对应文件系统的probe函数
	if (fs_set_blk_dev(argv[1], argv[2], FS_TYPE_ANY))
		return 1;
	//从fstypes查找fs_type对应的元素
	info = fs_get_info(fs_type);

	if (argc == 4)//参数4个的话，就把结果保存到argv[3]环境变量
		setenv(argv[3], info->name);
	else		  //否则就显示结果
		printf("%s\n", info->name);

	return CMD_RET_SUCCESS;
}

