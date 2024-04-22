/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0
 */
#ifndef _FS_H
#define _FS_H

#include <common.h>

#define FS_TYPE_ANY	0
#define FS_TYPE_FAT	1
#define FS_TYPE_EXT	2
#define FS_TYPE_SANDBOX	3
#define FS_TYPE_UBIFS	4

/*
 * 告诉文件系统层将来命令要使用的块设备和分区。这还在内部标识了分区中存在的文件系统。
 * 通过在 fstype 参数中传递 FS_*，可以将识别过程限制为特定的文件系统类型。
 *
 * 成功时返回 0。
 * 如果访问磁盘或分区时出现错误，或者无法识别任何已知的文件系统类型，则返回非零值。
 */
int fs_set_blk_dev(const char *ifname, const char *dev_part_str, int fstype);

/*
 * Print the list of files on the partition previously set by fs_set_blk_dev(),
 * in directory "dirname".
 *
 * Returns 0 on success. Returns non-zero on error.
 */
int fs_ls(const char *dirname);

/*
 * Determine whether a file exists
 *
 * Returns 1 if the file exists, 0 if it doesn't exist.
 */
int fs_exists(const char *filename);

/*
 * fs_size - Determine a file's size
 *
 * @filename: Name of the file
 * @size: Size of file
 * @return 0 if ok with valid *size, negative on error
 */
int fs_size(const char *filename, loff_t *size);

/*
 * fs_read - Read file from the partition previously set by fs_set_blk_dev()
 * Note that not all filesystem types support either/both offset!=0 or len!=0.
 *
 * @filename: Name of file to read from
 * @addr: The address to read into
 * @offset: The offset in file to read from
 * @len: The number of bytes to read. Maybe 0 to read entire file
 * @actread: Returns the actual number of bytes read
 * @return 0 if ok with valid *actread, -1 on error conditions
 */
int fs_read(const char *filename, ulong addr, loff_t offset, loff_t len,
	    loff_t *actread);

/*
 * fs_write - Write file to the partition previously set by fs_set_blk_dev()
 * Note that not all filesystem types support offset!=0.
 *
 * @filename: Name of file to read from
 * @addr: The address to read into
 * @offset: The offset in file to read from. Maybe 0 to write to start of file
 * @len: The number of bytes to write
 * @actwrite: Returns the actual number of bytes written
 * @return 0 if ok with valid *actwrite, -1 on error conditions
 */
int fs_write(const char *filename, ulong addr, loff_t offset, loff_t len,
	     loff_t *actwrite);

/*
 * Common implementation for various filesystem commands, optionally limited
 * to a specific filesystem type via the fstype parameter.
 */
int do_size(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[],
		int fstype);
int do_load(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[],
		int fstype);
int do_ls(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[],
		int fstype);
int file_exists(const char *dev_type, const char *dev_part, const char *file,
		int fstype);
int do_save(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[],
		int fstype);

/*
 * Determine the UUID of the specified filesystem and print it. Optionally it is
 * possible to store the UUID directly in env.
 */
int do_fs_uuid(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[],
		int fstype);

/*
 * Determine the type of the specified filesystem and print it. Optionally it is
 * possible to store the type directly in env.
 */
int do_fs_type(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

#endif /* _FS_H */
