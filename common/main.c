/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/* #define	DEBUG	*/

#include <common.h>
#include <autoboot.h>
#include <cli.h>
#include <console.h>
#include <version.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
__weak void show_boot_progress(int val) {}

static void run_preboot_environment_command(void)
{
#ifdef CONFIG_PREBOOT
	char *p;

	p = getenv("preboot");
	if (p != NULL) {
# ifdef CONFIG_AUTOBOOT_KEYED
		int prev = disable_ctrlc(1);	/* disable Control C checking */
# endif

		run_command_list(p, -1, 0);

# ifdef CONFIG_AUTOBOOT_KEYED
		disable_ctrlc(prev);	/* restore Control C checking */
# endif
	}
#endif /* CONFIG_PREBOOT */
}

/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
	const char *s;

	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop"); //打印出启动进度。

#ifndef CONFIG_SYS_GENERIC_BOARD
	puts("Warning: Your board does not use generic board. Please read\n");
	puts("doc/README.generic-board and take action. Boards not\n");
	puts("upgraded by the late 2014 may break or be removed.\n");
#endif

#ifdef CONFIG_VERSION_VARIABLE
	setenv("ver", version_string);  /* 设置版本号环境变量 */
#endif /* CONFIG_VERSION_VARIABLE */

	cli_init();						/* cli_init函数，跟命令初始化有关，初始化 hush shell相关的变量 */

	run_preboot_environment_command();	/* 获取环境变量 perboot的内容， perboot是一些预启动命令，一般不使用这个环境变量 */

#if defined(CONFIG_UPDATE_TFTP)
	update_tftp(0UL, NULL, NULL);
#endif /* CONFIG_UPDATE_TFTP */

/*
 * 此函数会读取环境变量 bootdelay和 bootcmd的内容，然后将 bootdelay的值赋值给全局变量 
 * stored_bootdelay，返回值为环境变量 bootcmd的值。
 */
	s = bootdelay_process();
	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);
/*
 * 此函数就是检查倒计时是否结束？倒计时结束之前有
 * 没有被打断？此函数定义在文件common/autoboot.c中
 */
	autoboot_command(s);

	cli_loop();
}
