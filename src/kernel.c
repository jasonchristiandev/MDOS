#include "MDOS/bootinfo.h"

void __attribute__((sysv_abi)) _start(BOOT_INFO *info) {
	while (1) {
		asm("hlt");
	}
}