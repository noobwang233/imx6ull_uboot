/*
 * include/linker_lists.h
 *
 * Implementation of linker-generated arrays
 *
 * Copyright (C) 2012 Marek Vasut <marex@denx.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __LINKER_LISTS_H__
#define __LINKER_LISTS_H__

#include <linux/compiler.h>

/*
 * There is no use in including this from ASM files, but that happens
 * anyway, e.g. PPC kgdb.S includes command.h which incluse us.
 * So just don't define anything when included from ASM.
 */

#if !defined(__ASSEMBLY__)

/**
 * 链接器列表是通过将链接器输入节分组而构建的，每个节包含列表的一个条目。每个输入节
 * 包含一个常量初始化的变量，该变量保存条目的内容。链接器列表输入节是从列表和条目名称，
 * 加上一个前缀，构建的，该前缀允许将所有列表组合在一起。假设_list和_entry是列表和条目名
 * 称，则相应的输入节名称是
 *
 *   .u_boot_list_ + 2_ + @_list + _2_ + @_entry
 *
 * 而C变量名称是
 *
 *   _u_boot_list + _2_ + @_list + _2_ + @_entry
 *
 * 这确保了输入节和C变量名称的唯一性。
 *
 * 注意，名称仅在第一个字符上有所不同，即节的名称为"."，变量的名称为"_"，因此链接器不会
 * 混淆节和符号名称。从现在开始，两个名称将被称为
 *
 *   %u_boot_list_ + 2_ + @_list + _2_ + @_entry
 *
 * 条目变量永远不需要直接引用。
 *
 * 输入节的命名方案允许将所有链接器列表分组到单个链接器输出节中，并将所有单个列表的条目
 * 分组。
 *
 * 请注意名称中的两个'_2_'常量组件：它们的存在允许将起始符号和结束符号放置在列表周围，方法
 * 是将这些符号映射到具有组件"1"（之前）和"3"（之后）而不是"2"（在内部）的节名称。列表的
 * 起始和结束符号通常可以定义为
 *
 *   %u_boot_list_2_ + @_list + _1_...
 *   %u_boot_list_2_ + @_list + _3_...
 *
 * 链接器列表区域的整个起始和结束符号可以定义为
 *
 *   %u_boot_list_1_...
 *   %u_boot_list_3_...
 *
 * 这是一个示例，显示了由三个条目“first”、“second”和“third”组成的列表“array”所产生的排
 * 序节。
 *
 *   .u_boot_list_2_array_1
 *   .u_boot_list_2_array_2_first
 *   .u_boot_list_2_array_2_second
 *   .u_boot_list_2_array_2_third
 *   .u_boot_list_2_array_3
 *
 * 如果列表必须分为子列表（例如，仅对列表的一部分进行迭代），可以简单地给出形式为'outer_2_
 * inner'的列表名称，其中'outer'是全局列表名称，'inner'是子列表名称。整个列表的迭代器应该使
 * 用全局列表名称（"outer"）；仅子列表的迭代器应该使用完整的子列表名称（"outer_2_inner"）。
 *
 * 下面是从名为"drivers"的全局列表生成的节的示例，以及名为"i2c"和"pci"的两个子列表，并为整
 * 个列表和每个子列表定义的迭代器：
 *
 *   %u_boot_list_2_drivers_1
 *   %u_boot_list_2_drivers_2_i2c_1
 *   %u_boot_list_2_drivers_2_i2c_2_first
 *   %u_boot_list_2_drivers_2_i2c_2_first
 *   %u_boot_list_2_drivers_2_i2c_2_second
 *   %u_boot_list_2_drivers_2_i2c_2_third
 *   %u_boot_list_2_drivers_2_i2c_3
 *   %u_boot_list_2_drivers_2_pci_1
 *   %u_boot_list_2_drivers_2_pci_2_first
 *   %u_boot_list_2_drivers_2_pci_2_second
 *   %u_boot_list_2_drivers_2_pci_2_third
 *   %u_boot_list_2_drivers_2_pci_3
 *   %u_boot_list_2_drivers_3
 */

/**
 * llsym() - Access a linker-generated array entry
 * @_type:	Data type of the entry
 * @_name:	Name of the entry
 * @_list:	name of the list. Should contain only characters allowed
 *		in a C variable name!
 */
#define llsym(_type, _name, _list) \
		((_type *)&_u_boot_list_2_##_list##_2_##_name)

/**
 * ll_entry_declare() - Declare linker-generated array entry
 * @_type:	Data type of the entry
 * @_name:	Name of the entry
 * @_list:	name of the list. Should contain only characters allowed
 *		in a C variable name!
 *
 * This macro declares a variable that is placed into a linker-generated
 * array. This is a basic building block for more advanced use of linker-
 * generated arrays. The user is expected to build their own macro wrapper
 * around this one.
 *
 * A variable declared using this macro must be compile-time initialized.
 *
 * Special precaution must be made when using this macro:
 *
 * 1) The _type must not contain the "static" keyword, otherwise the
 *    entry is generated and can be iterated but is listed in the map
 *    file and cannot be retrieved by name.
 *
 * 2) In case a section is declared that contains some array elements AND
 *    a subsection of this section is declared and contains some elements,
 *    it is imperative that the elements are of the same type.
 *
 * 4) In case an outer section is declared that contains some array elements
 *    AND an inner subsection of this section is declared and contains some
 *    elements, then when traversing the outer section, even the elements of
 *    the inner sections are present in the array.
 *
 * Example:
 * ll_entry_declare(struct my_sub_cmd, my_sub_cmd, cmd_sub) = {
 *         .x = 3,
 *         .y = 4,
 * };
 */
#define ll_entry_declare(_type, _name, _list)				\
	_type _u_boot_list_2_##_list##_2_##_name __aligned(4)		\
			__attribute__((unused,				\
			section(".u_boot_list_2_"#_list"_2_"#_name)))

/**
 * ll_entry_declare_list() - Declare a list of link-generated array entries
 * @_type:	Data type of each entry
 * @_name:	Name of the entry
 * @_list:	name of the list. Should contain only characters allowed
 *		in a C variable name!
 *
 * This is like ll_entry_declare() but creates multiple entries. It should
 * be assigned to an array.
 *
 * ll_entry_declare_list(struct my_sub_cmd, my_sub_cmd, cmd_sub) = {
 *	{ .x = 3, .y = 4 },
 *	{ .x = 8, .y = 2 },
 *	{ .x = 1, .y = 7 }
 * };
 */
#define ll_entry_declare_list(_type, _name, _list)			\
	_type _u_boot_list_2_##_list##_2_##_name[] __aligned(4)		\
			__attribute__((unused,				\
			section(".u_boot_list_2_"#_list"_2_"#_name)))

/**
 * We need a 0-byte-size type for iterator symbols, and the compiler
 * does not allow defining objects of C type 'void'. Using an empty
 * struct is allowed by the compiler, but causes gcc versions 4.4 and
 * below to complain about aliasing. Therefore we use the next best
 * thing: zero-sized arrays, which are both 0-byte-size and exempt from
 * aliasing warnings.
 */

/**
 * ll_entry_start() - Point to first entry of linker-generated array
 * @_type:	Data type of the entry
 * @_list:	Name of the list in which this entry is placed
 *
 * This function returns (_type *) pointer to the very first entry of a
 * linker-generated array placed into subsection of .u_boot_list section
 * specified by _list argument.
 *
 * Since this macro defines an array start symbol, its leftmost index
 * must be 2 and its rightmost index must be 1.
 *
 * Example:
 * struct my_sub_cmd *msc = ll_entry_start(struct my_sub_cmd, cmd_sub);
 */
#define ll_entry_start(_type, _list)					\
({									\
	static char start[0] __aligned(4) __attribute__((unused,	\
		section(".u_boot_list_2_"#_list"_1")));			\
	(_type *)&start;						\
})

/**
 * ll_entry_end() - Point after last entry of linker-generated array
 * @_type:	Data type of the entry
 * @_list:	Name of the list in which this entry is placed
 *		(with underscores instead of dots)
 *
 * This function returns (_type *) pointer after the very last entry of
 * a linker-generated array placed into subsection of .u_boot_list
 * section specified by _list argument.
 *
 * Since this macro defines an array end symbol, its leftmost index
 * must be 2 and its rightmost index must be 3.
 *
 * Example:
 * struct my_sub_cmd *msc = ll_entry_end(struct my_sub_cmd, cmd_sub);
 */
#define ll_entry_end(_type, _list)					\
({									\
	static char end[0] __aligned(4) __attribute__((unused,		\
		section(".u_boot_list_2_"#_list"_3")));			\
	(_type *)&end;							\
})
/**
 * ll_entry_count() - Return the number of elements in linker-generated array
 * @_type:	Data type of the entry
 * @_list:	Name of the list of which the number of elements is computed
 *
 * This function returns the number of elements of a linker-generated array
 * placed into subsection of .u_boot_list section specified by _list
 * argument. The result is of an unsigned int type.
 *
 * Example:
 * int i;
 * const unsigned int count = ll_entry_count(struct my_sub_cmd, cmd_sub);
 * struct my_sub_cmd *msc = ll_entry_start(struct my_sub_cmd, cmd_sub);
 * for (i = 0; i < count; i++, msc++)
 *         printf("Entry %i, x=%i y=%i\n", i, msc->x, msc->y);
 */
#define ll_entry_count(_type, _list)					\
	({								\
		_type *start = ll_entry_start(_type, _list);		\
		_type *end = ll_entry_end(_type, _list);		\
		unsigned int _ll_result = end - start;			\
		_ll_result;						\
	})

/**
 * ll_entry_get() - Retrieve entry from linker-generated array by name
 * @_type:	Data type of the entry
 * @_name:	Name of the entry
 * @_list:	Name of the list in which this entry is placed
 *
 * This function returns a pointer to a particular entry in linker-generated
 * array identified by the subsection of u_boot_list where the entry resides
 * and it's name.
 *
 * Example:
 * ll_entry_declare(struct my_sub_cmd, my_sub_cmd, cmd_sub) = {
 *         .x = 3,
 *         .y = 4,
 * };
 * ...
 * struct my_sub_cmd *c = ll_entry_get(struct my_sub_cmd, my_sub_cmd, cmd_sub);
 */
#define ll_entry_get(_type, _name, _list)				\
	({								\
		extern _type _u_boot_list_2_##_list##_2_##_name;	\
		_type *_ll_result =					\
			&_u_boot_list_2_##_list##_2_##_name;		\
		_ll_result;						\
	})

/**
 * ll_start() - Point to first entry of first linker-generated array
 * @_type:	Data type of the entry
 *
 * This function returns (_type *) pointer to the very first entry of
 * the very first linker-generated array.
 *
 * Since this macro defines the start of the linker-generated arrays,
 * its leftmost index must be 1.
 *
 * Example:
 * struct my_sub_cmd *msc = ll_start(struct my_sub_cmd);
 */
#define ll_start(_type)							\
({									\
	static char start[0] __aligned(4) __attribute__((unused,	\
		section(".u_boot_list_1")));				\
	(_type *)&start;						\
})

/**
 * ll_end() - Point after last entry of last linker-generated array
 * @_type:	Data type of the entry
 *
 * This function returns (_type *) pointer after the very last entry of
 * the very last linker-generated array.
 *
 * Since this macro defines the end of the linker-generated arrays,
 * its leftmost index must be 3.
 *
 * Example:
 * struct my_sub_cmd *msc = ll_end(struct my_sub_cmd);
 */
#define ll_end(_type)							\
({									\
	static char end[0] __aligned(4) __attribute__((unused,		\
		section(".u_boot_list_3")));				\
	(_type *)&end;							\
})

#endif /* __ASSEMBLY__ */

#endif	/* __LINKER_LISTS_H__ */
