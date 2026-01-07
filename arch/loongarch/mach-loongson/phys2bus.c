// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2026 Xinyu Zheng <3435193369@qq.com>
 */

#include <asm/addrspace.h>
#include <config.h>
#include <phys2bus.h>

unsigned long phys_to_bus(unsigned long phys)
{
	return PHYS_TO_UNCACHED(phys);
}

unsigned long bus_to_phys(unsigned long bus)
{
	return VA_TO_PHYS(bus);
}

dma_addr_t dev_phys_to_bus(struct udevice *dev, phys_addr_t phys)
{
	return VA_TO_PHYS(phys);
}

phys_addr_t dev_bus_to_phys(struct udevice *dev, dma_addr_t bus)
{
	return PHYS_TO_CACHED(bus);
}
