#include "MDOS/bootinfo.h"
#include "MDOS/fat32.h"
#include "MDOS/pci.h"
#include "MDOS/utils.h"

__attribute__((section(".text._start"))) unsigned long __attribute__((sysv_abi)) _start(BOOT_INFO *info) {
	EFI_SYSTEM_TABLE *system_table = info->system_table;
	GRAPHICS_INFO *graphics_info = info->graphics_info;

	__asm__ volatile("and $0xfffffffffffffff0, %rsp");

	// PRINT(L"#################################\r\n# MODERN DRIVE OPERATING SYSTEM #\r\n#################################\r\n\r\n");

	DISK_MANAGER disk_mgr;
	disk_manager_init(&disk_mgr);
	pci_scan(system_table, &disk_mgr);

	LOG_INFO(L"KERNEL", L"Disk scan complete. Found %d disk(s).\r\n", disk_mgr.count);

	if (disk_mgr.count == 0) {
		LOG_ERROR(L"KERNEL", L"No disks found! Nothing to do.\r\n");
		EXIT(L"KERNEL");
		return EFI_NOT_FOUND;
	}
	
	LOG_INFO(L"KERNEL", L"Press any key to continue...\r\n");
	WAIT_FOR_KEY();

	LOG_INFO(L"KERNEL", L"Probing disks for FAT32 filesystem...\r\n");

	DISK_DEVICE *boot_disk = NULL;
	FAT32_FILESYSTEM fs;
	BOOLEAN fs_mounted = FALSE;

	for (int i = 0; i < disk_mgr.count; i++) {
		DISK_DEVICE *d = &disk_mgr.devices[i];
		uint32_t part_lba;

		LOG_INFO(L"KERNEL", L"Probing disk %d (type=%d, port=%d)...\r\n", i, d->type, d->port_index);

		if (fat32_find_partition(system_table, d, &part_lba)) {
			if (fat32_mount(system_table, d, part_lba, &fs)) {
				boot_disk = d;
				fs_mounted = TRUE;
				LOG_INFO(L"KERNEL", L"FAT32 mounted on disk %d!\r\n", i);
				break;
			}
		}
	}

	if (!fs_mounted) {
		EXIT(L"KERNEL");
		return EFI_NOT_FOUND;
	}

	LOG_INFO(L"KERNEL", L"Reading root directory...");

	fat32_list_dir(system_table, &fs, fs.root_cluster);

	while (1) {
		__asm__("hlt");
	}
}