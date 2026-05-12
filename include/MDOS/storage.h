#pragma once
#include "printf.h"
#include <efi.h>

static inline void outl(uint16_t port, uint32_t val) {
	__asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
	uint32_t ret;
	__asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
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

void scan_pci(EFI_SYSTEM_TABLE *system_table) {
	for (int bus = 0; bus < 256; bus++) {
		for (int dev = 0; dev < 32; dev++) {
			uint32_t reg0 = pci_read_config(bus, dev, 0, 0x00);
			uint16_t vendor = reg0 & 0xFFFF;
			if (vendor == 0xFFFF) continue;

			uint32_t reg8 = pci_read_config(bus, dev, 0, 0x08);
			uint8_t class_code = (reg8 >> 24) & 0xFF;
			uint8_t subclass = (reg8 >> 16) & 0xFF;
			uint8_t prog_if = (reg8 >> 8) & 0xFF;

			if (class_code == 0x01) {
				if (subclass == 0x06 && prog_if == 0x01) {
					EfiPrintF(system_table, L"Found SATA AHCI controller at %d:%d:0.\r\n", bus, dev);
				} else if (subclass == 0x08 && prog_if == 0x02) {
					EfiPrintF(system_table, L"Found NVMe controller at %d:%d:0.\r\n", bus, dev);
				} else if (subclass == 0x01) {
					EfiPrintF(system_table, L"Found IDE controller at %d:%d:0.\r\n", bus, dev);
				}
			}
		}
	}
}