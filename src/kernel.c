#include "MDOS/bootinfo.h"
#include "MDOS/utils.h"

__attribute__((section(".text._start")))
void __attribute__((sysv_abi)) _start(BOOT_INFO *info) {
	EFI_SYSTEM_TABLE *system_table = info->system_table;
	GRAPHICS_INFO *graphics_info = info->graphics_info;

	__asm__ volatile("and $0xfffffffffffffff0, %rsp");

	while (1) {
		asm("hlt");
	}
}