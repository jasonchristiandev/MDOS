#pragma once
#include "utils.h"
#include <efi.h>

typedef struct {
	uint32_t clb;
	uint32_t clbu;
	uint32_t fb;
	uint32_t fbu;
	uint32_t is;
	uint32_t ie;
	uint32_t cmd;
	uint32_t reserved0;
	uint32_t tfd;
	uint32_t sig;
	uint32_t ssts;
	uint32_t sctl;
	uint32_t serr;
	uint32_t sact;
	uint32_t ci;
	uint32_t sntf;
	uint32_t fbs;
	uint32_t reserved1[11];
	uint32_t vendor[4];
} HBA_PORT;

typedef struct {
	uint8_t cfl : 5;
	uint8_t a : 1;
	uint8_t w : 1;
	uint8_t p : 1;
	uint8_t r : 1;
	uint8_t b : 1;
	uint8_t c : 1;
	uint8_t reserved0 : 1;
	uint8_t pmp : 4;
	uint16_t prdtl;

	volatile uint32_t prdbc;

	uint32_t ctba;
	uint32_t ctbau;

	uint32_t reserved1[4];
} HBA_CMD_HEADER;

typedef struct {
	uint32_t dba;
	uint32_t dbau;
	uint32_t reserved0;

	uint32_t dbc : 22;
	uint32_t reserved1 : 9;
	uint32_t i : 1;
} HBA_PRDT_ENTRY;

typedef struct {
	uint8_t fis_type;
	uint8_t pm : 4;
	uint8_t rsv0 : 3;
	uint8_t c : 1;
	uint8_t command;
	uint8_t featurel;

	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device;

	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t featureh;

	uint8_t countl;
	uint8_t counth;
	uint8_t icc;
	uint8_t control;

	uint8_t rsv1[4];
} FIS_REG_H2D;

typedef struct {
	uint8_t cfis[64];
	uint8_t acmd[16];
	uint8_t reserved[48];
	HBA_PRDT_ENTRY prdt_entry[1];
} HBA_CMD_TBL;

void start_port(HBA_PORT *port) {
	while (port->cmd & (1 << 15));

	port->cmd |= 0x10;
	port->cmd |= 0x01;
}

void stop_port(HBA_PORT *port) {
	port->cmd &= ~0x01;
	port->cmd &= ~0x10;

	while (1) {
		if (port->cmd & (1 << 14)) continue;
		if (port->cmd & (1 << 15)) continue;
		break;
	}
}

void init_port(EFI_SYSTEM_TABLE *st, HBA_PORT *port) {
	stop_port(port);

	// port->clb = (uint32_t) (uintptr_t) cl_addr;
	// port->clbu = (uint32_t) ((uintptr_t) cl_addr >> 32);

	// port->fb = (uint32_t) (uintptr_t) fis_addr;
	// port->fbu = (uint32_t) ((uintptr_t) fis_addr >> 32);

	port->is = 0xFFFFFFFF;
	port->serr = 0xFFFFFFFF;

	start_port(port);
}

#define ATA_CMD_READ_DMA_EXT 0x25

BOOLEAN ahci_read(EFI_SYSTEM_TABLE *system_table, HBA_PORT *port, uint64_t lba, uint32_t count, uint16_t *buffer) {
	port->is = 0xFFFF;
	int slot = 0;

	HBA_CMD_HEADER *cmd_header = (HBA_CMD_HEADER *) (uintptr_t) port->clb;
	cmd_header += slot;

	cmd_header->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
	cmd_header->w = 0;
	cmd_header->prdtl = 1;

	HBA_CMD_TBL *cmd_table = (HBA_CMD_TBL *) (uintptr_t) cmd_header->ctba;
	system_table->BootServices->SetMem(cmd_table, sizeof(HBA_CMD_TBL), 0);

	cmd_table->prdt_entry[0].dba = (uint32_t) (uintptr_t) buffer;
	cmd_table->prdt_entry[0].dbau = (uint32_t) ((uintptr_t) buffer >> 32);
	cmd_table->prdt_entry[0].dbc = (count << 9) - 1;
	cmd_table->prdt_entry[0].i = 1;

	FIS_REG_H2D *fis = (FIS_REG_H2D *) (&cmd_table->cfis);
	fis->fis_type = 0x27;
	fis->c = 1;
	fis->command = ATA_CMD_READ_DMA_EXT;

	fis->lba0 = (uint8_t) lba;
	fis->lba1 = (uint8_t) (lba >> 8);
	fis->lba2 = (uint8_t) (lba >> 16);
	fis->device = 1 << 6;

	fis->lba3 = (uint8_t) (lba >> 24);
	fis->lba4 = (uint8_t) (lba >> 32);
	fis->lba5 = (uint8_t) (lba >> 40);

	fis->countl = (uint8_t) count;
	fis->counth = (uint8_t) (count >> 8);

	uint32_t spin = 0;
	while ((port->tfd & (0x80 | 0x08)) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		PRINT(L"Port is hung!\r\n");
		return FALSE;
	}

	port->ci = (1 << slot);

	while (1) {
		if ((port->ci & (1 << slot)) == 0) break;

		if (port->is & (1 << 30)) {
			PRINT(L"Read disk error\r\n");
			return FALSE;
		}
	}

	return TRUE;
}