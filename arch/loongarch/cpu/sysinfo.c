// SPDX-License-Identifier: GPL-2.0+
/*
 * Author: Xinyu Zheng <3435193369@qq.com>
 */
#include <dm.h>
#include <smbios_plat.h>
#include <sysinfo.h>
#include <cpu_func.h>
#include <asm/cache.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/loongarch.h>
#include <asm/system.h>
#include <linux/bug.h>

static u8 ways_to_smbios[] = {SMBIOS_CACHE_ASSOC_UNKNOWN, SMBIOS_CACHE_ASSOC_DMAPPED, SMBIOS_CACHE_ASSOC_2WAY,
    SMBIOS_CACHE_ASSOC_UNKNOWN, SMBIOS_CACHE_ASSOC_4WAY, SMBIOS_CACHE_ASSOC_UNKNOWN, SMBIOS_CACHE_ASSOC_UNKNOWN,
    SMBIOS_CACHE_ASSOC_UNKNOWN, SMBIOS_CACHE_ASSOC_8WAY,
};

int sysinfo_get_cache_info(u8 level, struct cache_info *cinfo)
{
	unsigned int lsize, sets, ways;
	unsigned int lsize1, sets1, ways1;
	unsigned int config;
    sysinfo_cache_info_default(cinfo);
    switch (level) {
        case 0:
            config = read_cpucfg(LOONGARCH_CPUCFG17);
            lsize = 1 << ((config & CPUCFG17_L1I_SIZE_M) >> CPUCFG17_L1I_SIZE);
            sets  = 1 << ((config & CPUCFG17_L1I_SETS_M) >> CPUCFG17_L1I_SETS);
            ways  = ((config & CPUCFG17_L1I_WAYS_M) >> CPUCFG17_L1I_WAYS) + 1;

            config = read_cpucfg(LOONGARCH_CPUCFG18);
            lsize1 = 1 << ((config & CPUCFG18_L1D_SIZE_M) >> CPUCFG18_L1D_SIZE);
            sets1  = 1 << ((config & CPUCFG18_L1D_SETS_M) >> CPUCFG18_L1D_SETS);
            ways1  = ((config & CPUCFG18_L1D_WAYS_M) >> CPUCFG18_L1D_WAYS) + 1;

            switch((lsize > 0 ? 2 : 0) + (lsize1 > 0 ? 1 : 0)) {
                case 2:
                    cinfo->cache_type = SMBIOS_CACHE_SYSCACHE_TYPE_INST;
                    break;
                case 1:
                    cinfo->cache_type = SMBIOS_CACHE_SYSCACHE_TYPE_DATA;
                    break;
                case 3:
                    cinfo->cache_type = SMBIOS_CACHE_SYSCACHE_TYPE_UNIFIED;
                    break;
                default:
                    return -1;
            }
	        cinfo->line_size = lsize;
            cinfo->associativity = ways_to_smbios[ways];
            cinfo->max_size = (lsize * sets * ways + lsize1 * sets1 * ways1) / 1024;
	        cinfo->inst_size = cinfo->max_size;
            break;
        case 1:
            config = read_cpucfg(LOONGARCH_CPUCFG19);
            lsize = 1 << ((config & CPUCFG19_L2_SIZE_M) >> CPUCFG19_L2_SIZE);
            sets  = 1 << ((config & CPUCFG19_L2_SETS_M) >> CPUCFG19_L2_SETS);
            ways  = ((config & CPUCFG19_L2_WAYS_M) >> CPUCFG19_L2_WAYS) + 1;
            if (lsize == 0) {
                return -1;
            }
            cinfo->cache_type = SMBIOS_CACHE_SYSCACHE_TYPE_UNIFIED;
	        cinfo->line_size = lsize;
            cinfo->associativity = ways_to_smbios[ways];
            cinfo->max_size = (lsize * sets * ways) / 1024;
	        cinfo->inst_size = cinfo->max_size;
            break;
        case 2:
            config = read_cpucfg(LOONGARCH_CPUCFG20);
            lsize = 1 << ((config & CPUCFG20_L3_SIZE_M) >> CPUCFG20_L3_SIZE);
            sets  = 1 << ((config & CPUCFG20_L3_SETS_M) >> CPUCFG20_L3_SETS);
            ways  = ((config & CPUCFG20_L3_WAYS_M) >> CPUCFG20_L3_WAYS) + 1;
            if (lsize == 0) {
                return -1;
            }
            cinfo->cache_type = SMBIOS_CACHE_SYSCACHE_TYPE_UNIFIED;
	        cinfo->line_size = lsize;
            cinfo->associativity = ways_to_smbios[ways];
            cinfo->max_size = (lsize * sets * ways) / 1024;
	        cinfo->inst_size = cinfo->max_size;
            break;
        default:
            return -1;
    }
    return 0;
}

int sysinfo_get_processor_info(struct processor_info *pinfo)
{
    pinfo->manufacturer = "Loongson Technology Co Ltd";
    pinfo->core_count = 1;
    pinfo->core_enabled = pinfo->core_count;
	pinfo->characteristics = SMBIOS_PROCESSOR_64BIT;
    return 0;
}
