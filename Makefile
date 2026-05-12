ARCH = x86_64

SOURCE = src
INCLUDE = include
FONTS = fonts
BUILD = build
ISO = iso
FONT = font.psf

EFIINC = /usr/include/efi
EFIINCS = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
EFILIB = /usr/lib
EFI_CRT_OBJS = $(EFILIB)/crt0-efi-$(ARCH).o
EFI_LDS = $(EFILIB)/elf_$(ARCH)_efi.lds

CFLAGS = $(EFIINCS) -fno-stack-protector \
			-fshort-wchar -mno-red-zone -Wall \
			-DEFI_FUNCTION_WRAPPER -DGNU_EFI_USE_MS_ABI \
			-I$(INCLUDE)

LDFLAGS = -nostdlib -znocombreloc -shared -Bsymbolic -L$(EFILIB) --entry _start

CC = gcc
LD = ld
OBJCOPY = objcopy


# MISC

all: $(BUILD)/boot.efi $(BUILD)/kernel.bin

$(BUILD):
	mkdir -p $(BUILD)

disk.img:
	qemu-img create -f raw disk.img 100M

run-preq: all disk.img
	mkdir -p $(ISO)/EFI/BOOT
	cp $(BUILD)/boot.efi $(ISO)/EFI/BOOT/BOOTX64.EFI
	cp build/kernel.bin iso/kernel.bin

run: run-ahci

run-ahci: run-preq
	qemu-system-x86_64 -machine q35 \
					   -bios /usr/share/ovmf/OVMF.fd \
					   -drive file=fat:rw:$(ISO),format=raw \
					   -device ahci,id=ahci0 \
					   -drive id=drive0,file=disk.img,if=none,format=raw \
					   -device ide-hd,drive=drive0,bus=ahci0.0 \
					   -net none

run-nvme: run-preq
	qemu-system-x86_64 -machine q35 \
					   -bios /usr/share/ovmf/OVMF.fd \
					   -drive file=fat:rw:$(ISO),format=raw \
					   -drive file=disk.img,if=none,id=nvm0,format=raw \
					   -device nvme,serial=MDOS_DISK_1,drive=nvm0 \
					   -net none

run-ide: run-preq
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd \
					   -drive file=fat:rw:$(ISO),format=raw \
					   -drive file=disk.img,if=ide,index=1,media=disk,format=raw \
					   -net none

clean:
	rm -rf *.o *.so *.efi $(BUILD) $(ISO)


# BOOTLOADER

$(BUILD)/boot.o: $(SOURCE)/boot.c | $(BUILD)
	$(CC) $(CFLAGS) -fpic -c $< -o $@

$(BUILD)/boot.so: $(BUILD)/boot.o
	$(LD) $(LDFLAGS) -T $(EFI_LDS) $(EFI_CRT_OBJS) $< -o $@ -lgnuefi -lefi

$(BUILD)/boot.efi: $(BUILD)/boot.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .rodata -j .dynamic \
		-j .dynsym  -j .rel -j .rela -j .reloc \
		--target=efi-app-$(ARCH) --subsystem=10 $< $@


# KERNEL

$(BUILD)/kernel.o: $(SOURCE)/kernel.c | $(BUILD)
	$(CC) $(EFIINCS) $(CFLAGS) -mno-sse -mno-mmx -c $< -o $@

$(BUILD)/kernel.elf: $(BUILD)/kernel.o
	$(LD) -nostdlib -T src/kernel.ld $^ -o $@

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary -j .text -j .rodata -j .data $< $@
