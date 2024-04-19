/*
 * A lowlevel_init function that sets up the stack to call a C function to
 * perform further init.
 *
 * (C) Copyright 2010
 * Texas Instruments, <www.ti.com>
 *
 * Author :
 *	Aneesh V	<aneesh@ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm-offsets.h>
#include <config.h>
#include <linux/linkage.h>

ENTRY(lowlevel_init)
	/*
	 * 设置临时的栈指针给c函数使用，全局变量还不能使用
	 */
	ldr	sp, =CONFIG_SYS_INIT_SP_ADDR 			//设置 sp指向 CONFIG_SYS_INIT_SP_ADDR，该宏定义板级头文件中有定义
	bic	sp, sp, #7 								/* 8-byte alignment for ABI compliance */
#ifdef CONFIG_SPL_DM							//并没有使能SPL
	mov	r9, #0
#else
	/*
	 * Set up global data for boards that still need it. This will be
	 * removed soon.
	 */
#ifdef CONFIG_SPL_BUILD
	ldr	r9, =gdata
#else
	sub	sp, sp, #GD_SIZE	//sp减去GD_SIZE，GD_SIZE 同样在generic-asm-offsets.h 中定义了，大小为sizeof(struct global_data)=248
	bic	sp, sp, #7			//对sp 做8 字节对齐,此时sp 的地址为0X0091FF00-248=0X0091FE08
	mov	r9, sp				//将sp 地址保存在r9 寄存器中
#endif
#endif
	/*
	 * 将旧的 lr（传递给了 ip）和当前的 lr 保存到堆栈中
	 * r12是内部调用暂时寄存器 ip。它在过程链接胶合代码（例如，交互操作胶合代码）中用于此角色。
	 * 在过程调用之间，可以将它用于任何用途。被调用函数在返回之前不必恢复 r12。
	*/
	push	{ip, lr}		//将ip 和lr 压栈,准备跳转到C函数，这里不把lr寄存器压栈的话，执行完c语言函数将无法跳转回来

	/*
	* 调用非常早期的初始化函数。这个函数应该只做绝对最少的工作以启动系统。它不应该：
	*
	* - 设置 DRAM
	* - 使用全局数据结构
	* - 清除 BSS 段
	* - 尝试启动控制台
	*
	* 对于具有 SPL 的板子来说，这个函数应该是空的，因为 SPL 可以在 SPL 的
	* board_init_f() 函数中执行所有这些初始化，而该函数会在此初始化函数之后立即被调用。
	*/
	bl	s_init				//跳转到s_init
	pop	{ip, pc}			//将第入栈的ip 和lr 进行出栈，并将lr 赋给pc
ENDPROC(lowlevel_init)
