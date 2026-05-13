#pragma once
#include "storage.h"
#include "utils.h"
#include <efi.h>

#define DISK_TYPE_NONE 0
#define DISK_TYPE_AHCI 1
#define DISK_TYPE_NVME 2
#define DISK_TYPE_IDE 3

#define DISK_SECTOR_SIZE 512

typedef struct {
	uint8_t type;
	uint16_t bus;
	uint8_t dev;
	uint8_t func;
	uint8_t port_index;
	uint64_t sector_count;
	void *port;
} DISK_DEVICE;

typedef struct {
	DISK_DEVICE devices[8];
	int count;
} DISK_MANAGER;

static inline void disk_manager_init(DISK_MANAGER *manager) {
	manager->count = 0;
	for (int i = 0; i < 8; i++) {
		manager->devices[i].type = DISK_TYPE_NONE;
		manager->devices[i].port = NULL;
	}
}

static inline void disk_manager_register(DISK_MANAGER *manager, uint8_t type, uint16_t bus, uint8_t dev, uint8_t func, uint8_t port_index, void *port) {
	if (manager->count >= 8) return;
	DISK_DEVICE *d = &manager->devices[manager->count];
	d->type = type;
	d->bus = bus;
	d->dev = dev;
	d->func = func;
	d->port_index = port_index;
	d->port = port;
	d->sector_count = 0;
	manager->count++;
}

static inline BOOLEAN disk_read(EFI_SYSTEM_TABLE *system_table, DISK_DEVICE *disk, uint64_t lba, uint32_t count, void *buffer) {
	switch (disk->type) {
		case DISK_TYPE_AHCI:
			return storage_ahci_read(system_table, (HBA_PORT *) disk->port, disk->port_index, lba, count, (uint16_t *) buffer);
		case DISK_TYPE_NVME:
			LOG_ERROR(L"DISK_READ", L"-> NVMe read not yet implemented!\r\n");
			return FALSE;
		case DISK_TYPE_IDE:
			LOG_ERROR(L"DISK_READ", L"-> IDE read not yet implemented!\r\n");
			return FALSE;
		default:
			LOG_ERROR(L"DISK_READ", L"-> Unknown disk type %d!\r\n", disk->type);
			return FALSE;
	}
}

static inline BOOLEAN disk_write(EFI_SYSTEM_TABLE *system_table, DISK_DEVICE *disk, uint64_t lba, uint32_t count, void *buffer) {
	switch (disk->type) {
		case DISK_TYPE_AHCI:
			return storage_ahci_write(system_table, (HBA_PORT *) disk->port, disk->port_index, lba, count, (uint16_t *) buffer);
		case DISK_TYPE_NVME:
			LOG_ERROR(L"DISK_WRITE", L"-> NVMe write not yet implemented!\r\n");
			return FALSE;
		case DISK_TYPE_IDE:
			LOG_ERROR(L"DISK_WRITE", L"-> IDE write not yet implemented!\r\n");
			return FALSE;
		default:
			LOG_ERROR(L"DISK_WRITE", L"-> Unknown disk type %d!\r\n", disk->type);
			return FALSE;
	}
}