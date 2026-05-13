#pragma once
#include "disk.h"
#include "utils.h"
#include <efi.h>

_Static_assert(1 == 1, "");
#pragma pack(push, 1)
typedef struct {
	uint8_t boot_indicator;
	uint8_t starting_chs[3];
	uint8_t partition_type;
	uint8_t ending_chs[3];
	uint32_t starting_lba;
	uint32_t total_sectors;
} MBR_PARTITION_ENTRY;

typedef struct {
	uint8_t bootstrap_code[446];
	MBR_PARTITION_ENTRY partitions[4];
	uint16_t signature;
} MBR;
#pragma pack(pop)

#define PARTITION_TYPE_FAT32_LBA 0x0C
#define PARTITION_TYPE_FAT32 0x0B

#pragma pack(push, 1)
typedef struct {
	uint8_t jmp_boot[3];
	char oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t num_fats;
	uint16_t root_entry_count;
	uint16_t total_sectors_16;
	uint8_t media_type;
	uint16_t sectors_per_fat_16;
	uint16_t sectors_per_track;
	uint16_t num_heads;
	uint32_t hidden_sectors;
	uint32_t total_sectors_32;

	uint32_t sectors_per_fat_32;
	uint16_t ext_flags;
	uint16_t fs_version;
	uint32_t root_cluster;
	uint16_t fs_info_sector;
	uint16_t backup_boot_sector;
	uint8_t reserved_2[12];
	uint8_t drive_number;
	uint8_t reserved_3;
	uint8_t boot_signature_ext;
	uint32_t volume_serial;
	char volume_label[11];
	char fs_type[8];
} FAT32_BPB;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
	char filename[8];
	char extension[3];
	uint8_t attributes;
	uint8_t nt_reserved;
	uint8_t creation_time_tenth;
	uint16_t creation_time;
	uint16_t creation_date;
	uint16_t last_access_date;
	uint16_t first_cluster_hi;
	uint16_t last_write_time;
	uint16_t last_write_date;
	uint16_t first_cluster_lo;
	uint32_t file_size;
} FAT32_DIR_ENTRY;

typedef struct {
	uint32_t first_cluster;
	uint8_t sequence_number;
	char name_utf16[26];
} FAT32_LFN_ENTRY;
#pragma pack(pop)

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LFN 0x0F

typedef struct {
	DISK_DEVICE *disk;
	uint32_t partition_lba;
	uint32_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint32_t cluster_size;
	uint32_t reserved_sectors;
	uint32_t fat_offset;
	uint32_t data_offset;
	uint32_t root_cluster;
	uint32_t sectors_per_fat;
} FAT32_FILESYSTEM;

static inline uint32_t fat32_cluster_to_lba(FAT32_FILESYSTEM *filesystem, uint32_t cluster) {
	return filesystem->data_offset + (cluster - 2) * filesystem->sectors_per_cluster;
}

static inline BOOLEAN fat32_is_end_of_chain(uint32_t cluster) {
	return cluster >= 0x0FFFFFF8;
}

static inline BOOLEAN fat32_is_free_cluster(uint32_t cluster) {
	return cluster == 0;
}

static inline uint32_t fat32_first_cluster(FAT32_DIR_ENTRY *entry) {
	return ((uint32_t) entry->first_cluster_hi << 16) | entry->first_cluster_lo;
}

static inline BOOLEAN fat32_find_partition(EFI_SYSTEM_TABLE *system_table, DISK_DEVICE *disk, uint32_t *out_partition_lba) {
	uint8_t mbr_buf[512];

	if (!disk_read(system_table, disk, 0, 1, mbr_buf)) {
		LOG_ERROR(L"FAT32", L"Failed to read MBR!\r\n");
		return FALSE;
	}

	MBR *mbr = (MBR *) mbr_buf;

	if (mbr->signature != 0xAA55) {
		LOG_ERROR(L"FAT32", L"Invalid MBR signature (0x%x)!\r\n", mbr->signature);
		return FALSE;
	}

	LOG_INFO(L"FAT32", L"MBR signature OK (0xAA55).\r\n");

	BOOLEAN has_any_partition = FALSE;
	for (int i = 0; i < 4; i++) {
		MBR_PARTITION_ENTRY *part = &mbr->partitions[i];

		if (part->partition_type == 0x00) continue;
		has_any_partition = TRUE;

		if (part->partition_type == PARTITION_TYPE_FAT32_LBA ||
			part->partition_type == PARTITION_TYPE_FAT32) {

			LOG_INFO(L"FAT32", L"Found partition %d: type=0x%02x, start_lba=%d, size=%d sectors.\r\n",
				  i, part->partition_type, part->starting_lba, part->total_sectors);

			*out_partition_lba = part->starting_lba;
			return TRUE;
		}

		LOG_INFO(L"FAT32", L"Partition %d: type=0x%02x (not FAT32).\r\n", i, part->partition_type);
	}

	if (!has_any_partition) {
		FAT32_BPB *bpb = (FAT32_BPB *) mbr_buf;

		if ((mbr_buf[0] == 0xEB || mbr_buf[0] == 0xE9) &&
			bpb->bytes_per_sector == 512 &&
			bpb->sectors_per_cluster > 0 &&
			(bpb->sectors_per_cluster & (bpb->sectors_per_cluster - 1)) == 0 &&
			bpb->sectors_per_fat_32 > 0 &&
			bpb->root_cluster >= 2) {

			LOG_INFO(L"FAT32", L"No partition table found, but VBR detected at sector 0 (superfloppy)\r\n");
			*out_partition_lba = 0;
			return TRUE;
		}
	}

	LOG_ERROR(L"FAT32", L"No FAT32 partition found in MBR\r\n");
	return FALSE;
}

static inline BOOLEAN fat32_mount(EFI_SYSTEM_TABLE *system_table, DISK_DEVICE *disk, uint32_t partition_lba, FAT32_FILESYSTEM *filesystem) {
	uint8_t vbr_buf[512];

	filesystem->disk = disk;
	filesystem->partition_lba = partition_lba;

	if (!disk_read(system_table, disk, partition_lba, 1, vbr_buf)) {
		LOG_ERROR(L"FAT32", L"Failed to read VBR at LBA %d!\r\n", partition_lba);
		return FALSE;
	}

	FAT32_BPB *bpb = (FAT32_BPB *) vbr_buf;

	if (bpb->bytes_per_sector != 512) {
		LOG_ERROR(L"FAT32", L"Unsupported sector size: %d!\r\n", bpb->bytes_per_sector);
		return FALSE;
	}

	if (bpb->sectors_per_cluster == 0 || (bpb->sectors_per_cluster & (bpb->sectors_per_cluster - 1)) != 0) {
		LOG_ERROR(L"FAT32", L"Invalid sectors per cluster: %d!\r\n", bpb->sectors_per_cluster);
		return FALSE;
	}

	filesystem->bytes_per_sector = bpb->bytes_per_sector;
	filesystem->sectors_per_cluster = bpb->sectors_per_cluster;
	filesystem->cluster_size = bpb->bytes_per_sector * bpb->sectors_per_cluster;
	filesystem->reserved_sectors = bpb->reserved_sectors;
	filesystem->fat_offset = filesystem->reserved_sectors;
	filesystem->sectors_per_fat = bpb->sectors_per_fat_32;
	filesystem->data_offset = filesystem->reserved_sectors + (bpb->num_fats * filesystem->sectors_per_fat);
	filesystem->root_cluster = bpb->root_cluster;

	LOG_INFO(L"FAT32", L"Mounted at LBA %d\r\n", partition_lba);
	LOG_INFO(L"FAT32", L"  bytes/sector:    %d\r\n", filesystem->bytes_per_sector);
	LOG_INFO(L"FAT32", L"  sectors/cluster: %d\r\n", filesystem->sectors_per_cluster);
	LOG_INFO(L"FAT32", L"  cluster_size:    %d bytes\r\n", filesystem->cluster_size);
	LOG_INFO(L"FAT32", L"  reserved:        %d sectors\r\n", filesystem->reserved_sectors);
	LOG_INFO(L"FAT32", L"  FAT sectors:     %d\r\n", filesystem->sectors_per_fat);
	LOG_INFO(L"FAT32", L"  data offset:     LBA +%d\r\n", filesystem->data_offset);
	LOG_INFO(L"FAT32", L"  root cluster:    %d\r\n", filesystem->root_cluster);
	LOG_INFO(L"FAT32", L"  volume label:    %.11s\r\n", bpb->volume_label);
	LOG_INFO(L"FAT32", L"  filesystem type:         %.8s\r\n", bpb->fs_type);

	return TRUE;
}

static inline uint32_t fat32_next_cluster(EFI_SYSTEM_TABLE *system_table, FAT32_FILESYSTEM *filesystem, uint32_t cluster) {
	uint32_t fat_entry_offset = cluster * 4;
	uint32_t fat_sector_index = fat_entry_offset / filesystem->bytes_per_sector;
	uint32_t entry_offset_in_sector = fat_entry_offset % filesystem->bytes_per_sector;

	uint32_t lba = filesystem->partition_lba + filesystem->fat_offset + fat_sector_index;
	uint8_t sector_buf[512];

	if (!disk_read(system_table, filesystem->disk, lba, 1, sector_buf)) {
		LOG_INFO(L"FAT32", L"Failed to read FAT sector at LBA %d\r\n", lba);
		return 0xFFFFFFFF;
	}

	uint32_t *entry = (uint32_t *) (sector_buf + entry_offset_in_sector);
	return (*entry) & 0x0FFFFFFF;
}

static inline BOOLEAN fat32_read_cluster(EFI_SYSTEM_TABLE *system_table, FAT32_FILESYSTEM *filesystem, uint32_t cluster, void *buffer) {
	uint32_t lba = filesystem->partition_lba + fat32_cluster_to_lba(filesystem, cluster);
	return disk_read(system_table, filesystem->disk, lba, filesystem->sectors_per_cluster, buffer);
}

static inline uint32_t fat32_read_chain(EFI_SYSTEM_TABLE *system_table, FAT32_FILESYSTEM *filesystem, uint32_t start_cluster, void *buffer, uint32_t max_bytes) {
	uint32_t bytes_read = 0;
	uint32_t cluster = start_cluster;
	uint8_t *buf = (uint8_t *) buffer;

	while (!fat32_is_end_of_chain(cluster) && !fat32_is_free_cluster(cluster) && bytes_read < max_bytes) {
		uint8_t cluster_buf[4096];

		if (!fat32_read_cluster(system_table, filesystem, cluster, cluster_buf)) {
			LOG_INFO(L"FAT32", L"Failed to read cluster %d\r\n", cluster);
			break;
		}

		uint32_t to_copy = filesystem->cluster_size;
		if (bytes_read + to_copy > max_bytes) {
			to_copy = max_bytes - bytes_read;
		}

		for (uint32_t i = 0; i < to_copy; i++) {
			buf[bytes_read + i] = cluster_buf[i];
		}

		bytes_read += to_copy;

		cluster = fat32_next_cluster(system_table, filesystem, cluster);
	}

	return bytes_read;
}

static inline BOOLEAN fat32_name_match(const char *entry_name, const char *entry_ext, const char *target) {
	for (int i = 0; i < 8; i++) {
		char a = entry_name[i];
		char b = target[i];
		if (a >= 'a' && a <= 'z') a -= 32;
		if (b >= 'a' && b <= 'z') b -= 32;
		if (a != b) return FALSE;
	}
	for (int i = 0; i < 3; i++) {
		char a = entry_ext[i];
		char b = target[8 + i];
		if (a >= 'a' && a <= 'z') a -= 32;
		if (b >= 'a' && b <= 'z') b -= 32;
		if (a != b) return FALSE;
	}
	return TRUE;
}

static inline void fat32_to_83(const char *input, char *out) {
	for (int i = 0; i < 11; i++) out[i] = ' ';
	out[11] = '\0';

	int i = 0;
	while (input[i] && input[i] != '.' && i < 8) {
		char c = input[i];
		if (c >= 'a' && c <= 'z') c -= 32;
		out[i] = c;
		i++;
	}

	while (input[i] && input[i] != '.') i++;
	if (input[i] == '.') i++;

	int ext_pos = 8;
	while (input[i] && ext_pos < 11) {
		char c = input[i];
		if (c >= 'a' && c <= 'z') c -= 32;
		out[ext_pos] = c;
		ext_pos++;
		i++;
	}
}

static inline void fat32_list_dir(EFI_SYSTEM_TABLE *system_table, FAT32_FILESYSTEM *filesystem, uint32_t dir_cluster) {
	LOG_INFO(L"FAT32", L"Listing directory (cluster %d):\r\n", dir_cluster);

	uint8_t cluster_buf[4096];
	uint32_t cluster = dir_cluster;
	int file_count = 0;

	while (!fat32_is_end_of_chain(cluster) && !fat32_is_free_cluster(cluster)) {
		if (!fat32_read_cluster(system_table, filesystem, cluster, cluster_buf)) {
			LOG_INFO(L"FAT32", L"Failed to read directory cluster %d\r\n", cluster);
			return;
		}

		int entries_per_cluster = filesystem->cluster_size / sizeof(FAT32_DIR_ENTRY);

		for (int i = 0; i < entries_per_cluster; i++) {
			FAT32_DIR_ENTRY *entry = (FAT32_DIR_ENTRY *) (cluster_buf + i * sizeof(FAT32_DIR_ENTRY));

			if (entry->filename[0] == 0x00) {
				return;
			}

			if ((uint8_t) entry->filename[0] == 0xE5) continue;
			if (entry->attributes == ATTR_LFN) continue;
			if (entry->attributes & ATTR_VOLUME_ID) continue;

			char display[13];
			int j;
			for (j = 0; j < 8 && entry->filename[j] != ' '; j++) {
				display[j] = entry->filename[j];
			}
			if (entry->extension[0] != ' ') {
				display[j++] = '.';
				for (int k = 0; k < 3 && entry->extension[k] != ' '; k++) {
					display[j++] = entry->extension[k];
				}
			}
			display[j] = '\0';

			CHAR16 wname[13];
			for (int x = 0; x <= j; x++) {
				wname[x] = (CHAR16) display[x];
			}

			uint32_t fc = fat32_first_cluster(entry);
			uint32_t size = entry->file_size;
			const CHAR16 *type_str = (entry->attributes & ATTR_DIRECTORY) ? L"<DIR>" : L"<FILE>";

			LOG_INFO(L"FAT32", L"%s  %s  cluster=%d  size=%d\r\n", type_str, wname, fc, size);
			file_count++;
		}

		cluster = fat32_next_cluster(system_table, filesystem, cluster);
	}

	if (file_count == 0) {
		LOG_INFO(L"FAT32", L"   (empty directory)\r\n");
	}
}

static inline BOOLEAN fat32_read_file(EFI_SYSTEM_TABLE *system_table, FAT32_FILESYSTEM *filesystem, const char *filename, void *buffer, uint32_t *out_size) {
	uint8_t cluster_buf[4096];
	uint32_t cluster = filesystem->root_cluster;

	while (!fat32_is_end_of_chain(cluster) && !fat32_is_free_cluster(cluster)) {
		if (!fat32_read_cluster(system_table, filesystem, cluster, cluster_buf)) {
			return FALSE;
		}

		int entries_per_cluster = filesystem->cluster_size / sizeof(FAT32_DIR_ENTRY);

		for (int i = 0; i < entries_per_cluster; i++) {
			FAT32_DIR_ENTRY *entry = (FAT32_DIR_ENTRY *) (cluster_buf + i * sizeof(FAT32_DIR_ENTRY));

			if (entry->filename[0] == 0x00) return FALSE;
			if ((uint8_t) entry->filename[0] == 0xE5) continue;
			if (entry->attributes == ATTR_LFN) continue;
			if (entry->attributes & ATTR_VOLUME_ID) continue;
			if (entry->attributes & ATTR_DIRECTORY) continue;

			char full_83[12];
			for (int x = 0; x < 8; x++) full_83[x] = entry->filename[x];
			for (int x = 0; x < 3; x++) full_83[8 + x] = entry->extension[x];
			full_83[11] = '\0';

			if (fat32_name_match(entry->filename, entry->extension, filename)) {
				uint32_t fc = fat32_first_cluster(entry);
				*out_size = fat32_read_chain(system_table, filesystem, fc, buffer, entry->file_size);
				return TRUE;
			}
		}

		cluster = fat32_next_cluster(system_table, filesystem, cluster);
	}

	return FALSE;
}
