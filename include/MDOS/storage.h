#pragma once
#include "utils.h"
#include <efi.h>

static inline void outl(uint16_t port, uint32_t val) {
	__asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
	uint32_t ret;
	__asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
	__asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void insw(uint16_t port, void *addr, uint32_t count) {
	__asm__ volatile("cld; rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}

uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
	uint32_t address = (uint32_t) ((uint32_t) 0x80000000 |
								   ((uint32_t) bus << 16) |
								   ((uint32_t) device << 11) |
								   ((uint32_t) func << 8) |
								   (offset & 0xfc));
	outl(0xCF8, address);
	return inl(0xCFC);
}

void pci_write_config(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t val) {
	uint32_t address = (uint32_t) ((uint32_t) 0x80000000 |
								   ((uint32_t) bus << 16) |
								   ((uint32_t) device << 11) |
								   ((uint32_t) func << 8) |
								   (offset & 0xfc));
	outl(0xCF8, address);
	outl(0xCFC, val);
}

void scan_nvme(EFI_SYSTEM_TABLE *system_table, uint16_t bus, uint8_t dev, uint8_t func) {
	PRINT(L"-> Found NVMe controller at %d:%d:%d\r\n", bus, dev, func);

	uint32_t bar0 = pci_read_config((uint8_t) bus, dev, func, 0x10);
	uint64_t nvme_base_addr = (bar0 & 0xFFFFFFF0);

	if ((bar0 & 0x06) == 0x04) {
		uint32_t bar1 = pci_read_config((uint8_t) bus, dev, func, 0x14);
		nvme_base_addr |= ((uint64_t) bar1 << 32);
	}

	if (nvme_base_addr == 0) {
		PRINT(L"   !! BAR0 is unassigned by Firmware.\r\n");
		return;
	}

	PRINT(L"   -> BAR Address: 0x%llx\r\n", nvme_base_addr);

	uint32_t command = pci_read_config((uint8_t) bus, dev, func, 0x04);
	command |= 0x06;
	pci_write_config((uint8_t) bus, dev, func, 0x04, command);

	volatile uint64_t *cap_reg = (volatile uint64_t *) (uintptr_t) nvme_base_addr;
	uint64_t capabilities = *cap_reg;

	PRINT(L"   -> NVMe CAP: 0x%llx\r\n", capabilities);

	volatile uint32_t *vs_reg = (volatile uint32_t *) (uintptr_t) (nvme_base_addr + 0x08);
	PRINT(L"   -> NVMe Ver: %d.%d\r\n", (*vs_reg >> 16), (*vs_reg >> 8) & 0xFF);
}

void scan_ahci(EFI_SYSTEM_TABLE *system_table, uint16_t bus, uint8_t dev, uint8_t func) {
	PRINT(L"-> Found SATA AHCI controller at %d:%d:%d\r\n", bus, dev, func);

	uint32_t abar = pci_read_config((uint8_t) bus, dev, func, 0x24);
	uint64_t ahci_base = (abar & 0xFFFFFFF0);

	if ((abar & 0x06) == 0x04) {
		uint32_t abar_high = pci_read_config((uint8_t) bus, dev, func, 0x28);
		ahci_base |= ((uint64_t) abar_high << 32);
	}

	if (ahci_base == 0) {
		PRINT(L"   !! ABAR is not assigned.\r\n");
		return;
	}

	uint32_t cmd = pci_read_config((uint8_t) bus, dev, func, 0x04);
	pci_write_config((uint8_t) bus, dev, func, 0x04, cmd | 0x06);

	PRINT(L"   -> ABAR Address: 0x%llx\r\n", ahci_base);

	volatile uint32_t *ghc = (volatile uint32_t *) (uintptr_t) (ahci_base + 0x04);
	*ghc |= (1U << 31);

	volatile uint32_t *ghc_cap = (volatile uint32_t *) (uintptr_t) ahci_base;
	uint32_t cap = *ghc_cap;

	int num_ports = (cap & 0x1F) + 1;
	PRINT(L"   -> AHCI Ports: %d, CAP: 0x%x\r\n", num_ports, cap);

	volatile uint32_t *pi_reg = (volatile uint32_t *) (uintptr_t) (ahci_base + 0x0C);
	uint32_t pi = *pi_reg;

	for (int i = 0; i < 32; i++) {
		if (!(pi & (1 << i))) continue;

		uintptr_t port_base = ahci_base + 0x100 + (i * 0x80);

		volatile uint32_t *ssts = (volatile uint32_t *) (port_base + 0x28);
		uint32_t status = *ssts;
		if ((status & 0x0F0F) != 0x0103) continue;

		volatile uint32_t *sig_reg = (volatile uint32_t *) (port_base + 0x24);
		uint32_t sig = *sig_reg;

		switch (sig) {
			case 0x00000101:
				PRINT(L"      -> Port %d: SATA HDD/SSD found.\r\n", i);
				break;
			case 0xEB140101:
				PRINT(L"      -> Port %d: SATAPI (Optical) found.\r\n", i);
				break;
			default:
				PRINT(L"      -> Port %d: Unknown device (SIG: 0x%x).\r\n", i, sig);
				break;
		}
	}
}

void scan_ide(EFI_SYSTEM_TABLE *system_table, uint16_t bus, uint8_t dev, uint8_t func) {
	PRINT(L"-> Found IDE controller at %d:%d:%d\r\n", bus, dev, func);

	uint16_t channels[] = {0x1F0, 0x170};
	const CHAR16 *channel_names[] = {L"Primary", L"Secondary"};

	for (int i = 0; i < 2; i++) {
		uint16_t base = channels[i];

		for (int drive = 0; drive < 2; drive++) {
			outb(base + 6, (drive == 0) ? 0xA0 : 0xB0);
			outb(base + 7, 0xEC);

			uint8_t status = inb(base + 7);
			if (status == 0 || status == 0xFF) continue;

			uint32_t timeout = 100000;
			while ((inb(base + 7) & 0x80) && --timeout);

			uint8_t lba_mid = inb(base + 4);
			uint8_t lba_hi = inb(base + 5);

			if (lba_mid == 0x14 && lba_hi == 0xEB) {
				PRINT(L"  -> IDE %s %s: PATAPI (CD-ROM)\r\n",
					  channel_names[i], (drive == 0) ? L"Master" : L"Slave");
			} else if (inb(base + 7) & 0x01) {
				continue;
			} else {
				PRINT(L"  -> IDE %s %s: PATA HDD\r\n",
					  channel_names[i], (drive == 0) ? L"Master" : L"Slave");
			}
		}
	}
}

void scan_pci(EFI_SYSTEM_TABLE *system_table) {
	bool found = false;

	for (uint16_t bus = 0; bus < 256; bus++) {
		for (uint8_t dev = 0; dev < 32; dev++) {
			for (uint8_t func = 0; func < 8; func++) {
				uint32_t reg0 = pci_read_config((uint8_t) bus, dev, func, 0x00);
				uint16_t vendor = reg0 & 0xFFFF;

				if (vendor == 0xFFFF) {
					if (func == 0) break;
					continue;
				}

				uint32_t reg8 = pci_read_config((uint8_t) bus, dev, func, 0x08);
				uint8_t class_code = (reg8 >> 24) & 0xFF;
				uint8_t subclass = (reg8 >> 16) & 0xFF;
				uint8_t prog_if = (reg8 >> 8) & 0xFF;

				if (class_code == 0x01) {
					if (subclass == 0x08 && prog_if == 0x02) {
						scan_nvme(system_table, bus, dev, func);
						found = true;
					} else if (subclass == 0x06 && prog_if == 0x01) {
						scan_ahci(system_table, bus, dev, func);
						found = true;
					} else if (subclass == 0x01) {
						scan_ide(system_table, bus, dev, func);
						found = true;
					}
				}

				if (func == 0) {
					uint32_t regC = pci_read_config((uint8_t) bus, dev, 0, 0x0C);
					if (!((regC >> 16) & 0x80)) break;
				}
			}
		}
	}
	if (!found) PRINT(L"!! No controllers found!\r\n");
}