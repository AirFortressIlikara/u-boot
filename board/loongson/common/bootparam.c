#include <command.h>
#include <asm/addrspace.h>
#include <smbios.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <ram.h>
#include <env.h>
#include "bootparam.h"

#define SMBIOS_PHYSICAL_ADDRESS 	0x0fffe000
#define SMBIOS_SIZE_LIMIT 			0x800
#define ACPI_TABLE_PHYSICAL_ADDRESS	0x0fefe000
#define ACPI_TABLE_SIZE_LIMIT 		0x100000

extern struct efi_system_table systab;
extern efi_status_t efi_init_systab(void);
extern efi_status_t efi_install_configuration_table(const efi_guid_t *guid,
			void *table);

#if defined(CONFIG_GENERATE_SMBIOS_TABLE)
extern ulong write_smbios_table(ulong addr);
void loongson_smbios_init(void)
{
	const efi_guid_t smbios_guid = SMBIOS_TABLE_GUID;

	write_smbios_table((ulong)SMBIOS_PHYSICAL_ADDRESS);

	efi_install_configuration_table(&smbios_guid,
			(void *)SMBIOS_PHYSICAL_ADDRESS);
}
#endif

void loongson_fdt_init(void)
{
	// 新的传参规范里面 可以把 fdt 放在 efi table 里面
	const efi_guid_t fdt_guid = EFI_FDT_GUID;
	void* fdt = (void*)env_get("fdt_addr");
	if (fdt) {
		fdt = (void *)simple_strtoul(fdt, NULL, 16);
		if (fdt_check_header(fdt)) {
			printf("Warning: invalid device tree. Used linux default dtb\n");
			fdt = NULL;
		}
	}

	if (fdt)
		efi_install_configuration_table(&fdt_guid,
						(void *)fdt);
}

struct efi_system_table *build_efi_table(void) {
#if defined(CONFIG_GENERATE_SMBIOS_TABLE)
	loongson_smbios_init();
#endif
	loongson_fdt_init();
	if(efi_init_systab() == EFI_SUCCESS)
		return &systab;
	return NULL;
}
