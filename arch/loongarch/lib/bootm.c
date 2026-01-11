// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#include <bootm.h>
#include <bootstage.h>
#include <env.h>
#include <image.h>
#include <fdt_support.h>
#include <log.h>
#include <asm/addrspace.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <stdlib.h>
#include <dm.h>

extern struct efi_system_table *build_efi_table(void);

// 新的传参规范里面无需把 command line 拆开成一个一个 直接字符串传进去就好
static char *linux_command_line;

static int boot_reloc_fdt(struct bootm_headers *images)
{
	/*
	 * In case of legacy uImage's, relocation of FDT is already done
	 * by do_bootm_states() and should not repeated in 'bootm prep'.
	 */
	if (images->state & BOOTM_STATE_FDT) {
		debug("## FDT already relocated\n");
		return 0;
	}

#if CONFIG_IS_ENABLED(OF_LIBFDT)
	boot_fdt_add_mem_rsv_regions(images->ft_addr);
	return boot_relocate_fdt(&images->ft_addr,
		&images->ft_len);
#else
	return 0;
#endif
}

static int boot_setup_fdt(struct bootm_headers *images)
{
	images->initrd_start = virt_to_phys((void *)images->initrd_start);
	images->initrd_end = virt_to_phys((void *)images->initrd_end);
	return image_setup_libfdt(images, images->ft_addr, true);
}

static void boot_prep_linux(struct bootm_headers *images)
{
	if (images->ft_len) {
		boot_reloc_fdt(images);
		boot_setup_fdt(images);
	}
}

static const char* boot_smbios_type2_board_name(void)
{
	struct udevice *dev;
	ofnode parent_node, node;

	const char* board_name = NULL;
	uclass_first_device(UCLASS_SYSINFO, &dev);
	if (dev) {
		parent_node = dev_read_subnode(dev, "smbios");
		if (!ofnode_valid(parent_node))
			return NULL;

		node = ofnode_find_subnode(parent_node, "baseboard");
		if (!ofnode_valid(node))
			return NULL;

		board_name = ofnode_read_string(node, "product");
	}
	return board_name;
}

static void boot_jump_linux(struct bootm_headers *images)
{
	typedef void __noreturn (*kernel_entry_t)(int, ulong, ulong);
	kernel_entry_t kernel = (kernel_entry_t)map_to_sysmem((void*)images->ep);
	void *fw_arg2 = NULL;
	struct efi_system_table *efitab = NULL;

	debug("## Transferring control to Linux (at address %p) ...\n", kernel);

	bootstage_mark(BOOTSTAGE_ID_RUN_OS);

#if CONFIG_IS_ENABLED(BOOTSTAGE_FDT)
	bootstage_fdt_add_report();
#endif
#if CONFIG_IS_ENABLED(BOOTSTAGE_REPORT)
	bootstage_report();
#endif

	efitab = build_efi_table();

	const char* board_name;
	// 见于 龙芯CPU统一系统架构规范（LA架构嵌入式系列）.pdf 的 4.1 传参约定 一节
	fw_arg2 = efitab;
	board_name = boot_smbios_type2_board_name();

	linux_command_line = (char*)calloc(256, sizeof(char));
	const char *bootargs = env_get("bootargs");
    if (!bootargs)
        bootargs = "";
    sprintf(linux_command_line, "%s", bootargs);

	if (board_name)
		sprintf(linux_command_line, "%s board_name=%s", linux_command_line, board_name);
	sprintf(linux_command_line, "%s noefi", linux_command_line);

	kernel(1, (ulong)linux_command_line, (ulong)fw_arg2);
}

int do_bootm_linux(int flag, struct bootm_info *bmi)
{
	struct bootm_headers *images = bmi->images;

	if (flag & BOOTM_STATE_OS_BD_T) // TODO
		return -1;

	/*
	 * Cmdline init has been moved to 'bootm prep' because it has to be
	 * done after relocation of ramdisk to always pass correct values
	 * for rd_start and rd_size to Linux kernel.
	 */
	if (flag & BOOTM_STATE_OS_CMDLINE)
		return 0;

	if (flag & BOOTM_STATE_OS_PREP) {
		boot_prep_linux(images);
		sync();
		return 0;
	}

	if (flag & (BOOTM_STATE_OS_GO | BOOTM_STATE_OS_FAKE_GO)) {
		boot_jump_linux(images);
		return 0;
	}

	/* does not return */
	return 1;
}
