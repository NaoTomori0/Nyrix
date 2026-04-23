# Makefile для Nyrix OS
CXX = i686-elf-g++
ASM = nasm
LD = i686-elf-ld

CXXFLAGS = -ffreestanding -fno-exceptions -fno-rtti -std=c++17 -O2 -Wall -Wextra -Isrc/include
ASMFLAGS = -f elf32
LDFLAGS = -T linker.ld

BUILD_DIR = build
SRC_DIR = src
ISO_DIR = iso

KERNEL_BIN = $(BUILD_DIR)/nyrix.bin
ISO_IMAGE  = $(BUILD_DIR)/nyrix.iso

OBJS = $(BUILD_DIR)/boot.o \
       $(BUILD_DIR)/gdt_flush.o \
       $(BUILD_DIR)/interrupts.o \
       $(BUILD_DIR)/kernel.o \
       $(BUILD_DIR)/gdt.o \
       $(BUILD_DIR)/idt.o \
       $(BUILD_DIR)/keyboard.o \
       $(BUILD_DIR)/pmm.o \
       $(BUILD_DIR)/kmalloc.o \
       $(BUILD_DIR)/commands.o


all: $(ISO_IMAGE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/boot.o: $(SRC_DIR)/boot/boot.asm | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

$(BUILD_DIR)/gdt_flush.o: $(SRC_DIR)/boot/gdt_flush.asm | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

$(BUILD_DIR)/interrupts.o: $(SRC_DIR)/boot/interrupts.asm | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

$(BUILD_DIR)/keyboard.o: $(SRC_DIR)/kernel/keyboard.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: $(SRC_DIR)/kernel/kernel.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/gdt.o: $(SRC_DIR)/kernel/gdt.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/idt.o: $(SRC_DIR)/kernel/idt.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/pmm.o: $(SRC_DIR)/kernel/pmm.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/kmalloc.o: $(SRC_DIR)/kernel/kmalloc.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/commands.o: $(SRC_DIR)/kernel/commands.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(KERNEL_BIN): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(ISO_IMAGE): $(KERNEL_BIN)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/nyrix.bin
	echo 'set timeout=0' > $(ISO_DIR)/boot/grub/grub.cfg
	echo 'set default=0' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo 'menuentry "Nyrix OS" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '    multiboot /boot/nyrix.bin' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '    boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISO_DIR) 2>/dev/null

run: $(ISO_IMAGE)
	qemu-system-i386 -cdrom $(ISO_IMAGE)

clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR)

.PHONY: all clean run
