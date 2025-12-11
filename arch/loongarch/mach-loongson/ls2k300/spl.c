#include <common.h>
#include <init.h>
#include <spl.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <mach/loongson.h>
#include <linux/io.h>

void spl_mach_init(void)
{
	arch_cpu_init();
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
