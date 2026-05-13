#include "MDOS/bootinfo.h"
#include "MDOS/pci.h"
#include "MDOS/utils.h"
#include "MDOS/fat32.h"

__attribute__((section(".text._start"))) void __attribute__((sysv_abi)) _start(BOOT_INFO *info) {
	EFI_SYSTEM_TABLE *system_table = info->system_table;
	GRAPHICS_INFO *graphics_info = info->graphics_info;

	__asm__ volatile("and $0xfffffffffffffff0, %rsp");

	// PRINT(L"#################################\r\n# MODERN DRIVE OPERATING SYSTEM #\r\n#################################\r\n\r\n");

	DISK_MANAGER disk_mgr;
	disk_manager_init(&disk_mgr);
	pci_scan(system_table, &disk_mgr);

	LOG_INFO(L"Disk scan complete. Found %d disk(s).\r\n\r\n", disk_mgr.count);

	if (disk_mgr.count == 0) {
		LOG_ERROR(L"No disks found! Nothing to do.\r\n");
		while (1) __asm__("hlt");
	}

	LOG_INFO(L"Probing disks for FAT32 filesystem...\r\n\r\n");

	DISK_DEVICE *boot_disk = NULL;
	FAT32_FILESYSTEM fs;
	BOOLEAN fs_mounted = FALSE;

	for (int i = 0; i < disk_mgr.count; i++) {
		DISK_DEVICE *d = &disk_mgr.devices[i];
		uint32_t part_lba;

		LOG_INFO(L"-> Probing disk %d (type=%d, port=%d)...\r\n", i, d->type, d->port_index);

		if (fat32_find_partition(system_table, d, &part_lba)) {
			if (fat32_mount(system_table, d, part_lba, &fs)) {
				boot_disk = d;
				fs_mounted = TRUE;
				LOG_INFO(L"-> FAT32 mounted on disk %d!\r\n\r\n", i);
				break;
			}
		}
	}

	if (!fs_mounted) {
		LOG_ERROR(L"No FAT32 partition found on any disk.\r\n");
		while (1) __asm__("hlt");
	}

	LOG_INFO(L"Reading root directory...");

	fat32_list_dir(system_table, &fs, fs.root_cluster);

	while (1) {
		__asm__("hlt");
	}
}