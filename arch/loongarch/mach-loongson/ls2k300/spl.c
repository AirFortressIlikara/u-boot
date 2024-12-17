#include <common.h>
#include <init.h>
#include <spl.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <mach/loongson.h>
#include <common.h>
#include <command.h>
#include <env.h>
#include <env_internal.h>
#include <flash.h>
#include <init.h>
#include <led.h>
#include <log.h>
#include <malloc.h>
#include <net.h>
#include <spi.h>
#include <spi_flash.h>
#include <linux/delay.h>
#include <linux/stringify.h>
#include <u-boot/crc.h>
#include <uuid.h>
#include <linux/ctype.h>
#include <linux/io.h>

void spl_mach_init(void)
{
	arch_cpu_init();
}

static void setup_gpio_value(unsigned int gpio, int value)
{
	unsigned char val;
	if (gpio > LS_GPIO_MAX) {
		printf("Error: setup_gpio_value: gpio(%d) not exist!\n", gpio);
		return;
	}
	val = value ? 1 : 0;
	readb(LS_GPIO_B_DIR_BASE + gpio) = 0x0;
	readb(LS_GPIO_B_OUT_BASE + gpio) = val;
}

void spl_board_init_late(void)
{
	if (!strcmp(CONFIG_DEFAULT_DEVICE_TREE, "ls2k300_pai"))
		setup_gpio_value(75, 1);
	else if (!strcmp(CONFIG_DEFAULT_DEVICE_TREE, "ls2k300_atk_dl2k0300b_v01")) {
		setup_gpio_value(72, 1);	//phy0 output high
		setup_gpio_value(73, 1);	//phy1 output high
	}
}

void spl_mach_init_late(void)
{
	unsigned long unlock_base = LOCK_CACHE_BASE;

	debug("spl unlocked scache.\n");
    // unlock scache
	// the scache locked in lowlevel_init.S is used by stack for early stage.
	// now our stack sp is in sdram, so unlock the scache.
	writeq(0x0, LS_SCACHE_LOCK_WIN0_BASE);
	writeq(0x0, LS_SCACHE_LOCK_WIN0_MASK);

	if (!strcmp(CONFIG_DEFAULT_DEVICE_TREE, "ls2k300_atk_dl2k0300b_v01")) {
		iowrite32(0xD1034E4E, (void __iomem *)LS_GENERAL_CFG14);
	}

	/* flush scache using hit-invalidate */
	for (int i = 0; i < LOCK_CACHE_SIZE; i += 0x40) {
		asm(
			"cacop 0x13, %0, 0\n\t"
			:
			: "r"(unlock_base)
		);
		unlock_base += 0x40;
	}
}
