// SPDX-License-Identifier: GPL-2.0+

#include <command.h>
#include <linux/sizes.h>
#include <asm/global_data.h>
#include <asm/sections.h>
#include <asm/addrspace.h>
#include <dm.h>
#include <mapmem.h>
#include <video_console.h>
#include <usb.h>
#include <image.h>
#include <mach/loongson.h>
#include <linux/delay.h>
#include <sound.h>
#include "bdinfo/bdinfo.h"
#include <env.h>
#include <net.h>
#include <phy.h>
#include <ansi.h>
#include <version_string.h>

#define ANSI_CURSOR_SAVE		"\e7"
#define ANSI_CURSOR_RESTORE		"\e8"

DECLARE_GLOBAL_DATA_PTR;

extern int multi_boards_check_store(void);

static const char* bdname;
static void find_bdname(void);

ulong board_get_usable_ram_top(ulong total_size)
{
	phys_addr_t ram_top = gd->ram_top;

	// U-boot 阶段仅使用最低的 256MB 内存，以便兼容不同内存容量的板卡。
	// 0x8F00_0000 ~ 0x8FFF_FFFF (16MB) 保留, 用于固件与内核的信息交互,
	// 具体参考 《龙芯CPU开发系统固件与内核接口详细规范》。
	// 注意：DVO0, DVO1 framebuffer 的保留内存 32MB (0x0D00_0000 ~ 0x0F00_0000),
	// 将在 board_f.c reserve_video() 中保留，此处无需处理。
	if (VA_TO_PHYS(ram_top) >= (phys_addr_t)(MEM_WIN_BASE + SZ_256M - SZ_16M)) {
		ram_top = (phys_addr_t)map_sysmem(MEM_WIN_BASE + SZ_256M - SZ_16M, 0);
	}

#ifdef CONFIG_SPL
	// Keep 4MB space for u-boot image.
	ram_top -= SZ_4M;
	if (CONFIG_TEXT_BASE < ram_top) {
		printf("Warning: Run u-boot from SPL, "
				"but the CONFIG_SYS_TEXT_BASE is out of "
				"the reserved space for u-boot image\n");
	}
#endif

	return ram_top;
}

u32 get_fdt_totalsize(const void *fdt)
{
	int conf, node, fdt_node, images, len;
	const char *fdt_name;
	const u32 *fdt_len;
	u32 totalsize = 0;

	if (fdt_check_header(fdt))
		return 0;

	totalsize = fdt_totalsize(fdt);

	conf = fdt_path_offset(fdt, FIT_CONFS_PATH);
	if (conf < 0) {
		debug("%s: Cannot find /configurations node: %d\n", __func__,
			  conf);
		goto finish;
	}

	images = fdt_path_offset(fdt, FIT_IMAGES_PATH);
	if (images < 0) {
		debug("%s: Cannot find /images node: %d\n", __func__, images);
		goto finish;
	}

	for (node = fdt_first_subnode(fdt, conf);
		node >= 0;
		node = fdt_next_subnode(fdt, node)) {

		fdt_name = fdt_getprop(fdt, node, FIT_FDT_PROP, &len);
		if (!fdt_name)
			continue;

		fdt_node = fdt_subnode_offset(fdt, images, fdt_name);
		if (fdt_node < 0)
			continue;

		fdt_len = fdt_getprop(fdt, fdt_node, "data-size", &len);
		if (!fdt_len || len != sizeof(*fdt_len))
			continue;

		totalsize += ALIGN(fdt32_to_cpu(*fdt_len), 4);
	}

finish:
	debug("fdt total size: %d\n", totalsize);
	return totalsize;
}

#ifdef CONFIG_OF_BOARD
void *board_fdt_blob_setup(int *err)
{
#ifdef CONFIG_SPL
	uint8_t *fdt_dst;
	uint8_t *fdt_src;
	ulong size;

	*err = 0;
	fdt_dst = (uint8_t*)ALIGN((ulong)&__bss_end, ARCH_DMA_MINALIGN);
#ifdef CONFIG_SPL_BUILD
	fdt_src = (uint8_t*)((ulong)&_image_binary_end - (ulong)__text_start +
                        BOOT_SPACE_BASE_UNCACHED);
#else
	fdt_src = (uint8_t*)&_image_binary_end;

	//当fdt段落在bss段里面时，需要把fdt复制到bss外面，因为启动过程中bss段会被清零，
	//当fdt段落在bss段外面时，返回fdt段的地址即可。
	if ((ulong)fdt_src >= (ulong)&__bss_end)
		fdt_dst = fdt_src;
#endif

	if (fdt_dst != fdt_src) {
		size = get_fdt_totalsize(fdt_src);
		memmove(fdt_dst, fdt_src, size);
		gd->fdt_size = size;
	}

	return fdt_dst;
#else
	*err = 0;
	return (void*)gd->fdt_blob;
#endif
}
#endif


static void acpi_config(void)
{
#ifndef NOT_USE_ACPI
	volatile u32 val;
	// disable wake on lan
	val = readl(LS_PM_RTC_REG);
	writel(val & ~(0x3 << 7), LS_PM_RTC_REG);

	// disable pcie and rtc wakeup
	val = readl(LS_PM1_EN_REG);
	val &= ~(1 << 10);
	val |= 1 << 14;
	writel(val, LS_PM1_EN_REG);

	// disable usb/ri/gmac/bat/lid wakeup event
	// and enable cpu thermal interrupt.
	writel(0xe, LS_GPE0_EN_REG);

	// clear pm status
	writel(0xffffffff, LS_PM1_STS_REG);
#endif
}

#ifdef CONFIG_NET
// mac地址来源优先级：env > bdinfo > random
static void ethaddr_setup(void)
{
	uchar bdi_ethaddr[ARP_HLEN];
	uchar env_ethaddr[ARP_HLEN];
	char *bdi_ethaddr_str;
	int id;
	char* env_val;
	int need_update_bdinfo;

	for (id = 0; id < 2; ++id) {
		if (!eth_env_get_enetaddr_by_index("eth", id, env_ethaddr)) {
			need_update_bdinfo = 0;
			if (id == 0)
				bdi_ethaddr_str = bdinfo_get(BDI_ID_MAC0);
			else
				bdi_ethaddr_str = bdinfo_get(BDI_ID_MAC1);

			string_to_enetaddr(bdi_ethaddr_str, bdi_ethaddr);
			if (is_valid_ethaddr(bdi_ethaddr)) {
				memcpy(env_ethaddr, bdi_ethaddr, ARP_HLEN);
			} else {
				need_update_bdinfo = 1;
				net_random_ethaddr(env_ethaddr);
				printf("\neth%d: using random MAC address - %pM\n",
					id, env_ethaddr);
			}
			eth_env_set_enetaddr_by_index("eth", id, env_ethaddr);
			if (need_update_bdinfo) {
				if (id == 0){
					env_val = env_get("ethaddr");
					bdinfo_set(BDI_ID_MAC0, env_val);
				} else {
					env_val = env_get("eth1addr");
					bdinfo_set(BDI_ID_MAC1, env_val);
				}
				bdinfo_save();
			}
		}
	}
}
#endif

#ifdef CONFIG_MISC_INIT_R
int misc_init_r(void)
{
	acpi_config();
#ifdef CONFIG_NET
	ethaddr_setup();
#endif
	return 0;
}
#endif

#ifdef CONFIG_BOARD_EARLY_INIT_F
__weak int ls_board_early_init_f(void)
{
	return 0;
}

int board_early_init_f(void)
{
	return ls_board_early_init_f();
}
#endif

#ifdef CONFIG_BOARD_EARLY_INIT_R
__weak int ls_board_early_init_r(void)
{
	return 0;
}

int board_early_init_r(void)
{
	bdinfo_init();
	return ls_board_early_init_r();
}
#endif

#ifdef CONFIG_BOARD_LATE_INIT
__weak int ls_board_late_init(void)
{
	return 0;
}

int board_late_init(void)
{
	return ls_board_late_init();
}
#endif

#ifdef CONFIG_LAST_STAGE_INIT
extern void user_env_fetch(void);
extern int recover(void);

int last_stage_init(void)
{
	user_env_fetch();

#ifdef CONFIG_PHY_LS2K_USB
	phy_ls2k_usb_init();
#endif

	env_set("ver", version_string); // save env 通过 loongson_env_trigger init 减少对spi的刷写

	return 0;
}
#endif

static void find_bdname(void)
{
	struct udevice *dev;
	ofnode parent_node, node;

	bdname = NULL;
	uclass_first_device(UCLASS_SYSINFO, &dev);
	if (dev) {
		parent_node = dev_read_subnode(dev, "smbios");
		if (!ofnode_valid(parent_node))
			return;

		node = ofnode_find_subnode(parent_node, "baseboard");
		if (!ofnode_valid(node))
			return;

		bdname = ofnode_read_string(node, "product");
	}
}

int checkboard(void)
{
	find_bdname();
	if (bdname)
		printf("Board: %s\n", bdname);
	return 0;
}

#if defined(CONFIG_SYS_CONSOLE_IS_IN_ENV) && \
	defined(CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE)
int overwrite_console(void)
{
	return 0;
}
#endif

#ifdef CONFIG_SPL_BOARD_INIT
__weak void spl_board_init(void)
{
	return ;
}
#endif
