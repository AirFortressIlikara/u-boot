#ifndef __ASM_MACH_LOONGSON_BOOT_PARAM_H_
#define __ASM_MACH_LOONGSON_BOOT_PARAM_H_
#include <efi_api.h>

struct efi_system_table *build_efi_table(void);
extern bool loongson_load_initrd;
#endif
