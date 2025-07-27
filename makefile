CC = x86_64-elf-gcc
ASM = nasm
LD = x86_64-elf-ld
CFLAGS = -c -ffreestanding -nostdlib -mcmodel=kernel -I src/include/
ASMFLAGS = -f elf64
LDFLAGS = -n -T linker.ld
QEMU = qemu-system-x86_64 -cdrom onien.iso

SRC_DIR = src
BUILD_DIR = build
ONIEN_DIR = $(SRC_DIR)/onien

C_SOURCES = $(shell find $(SRC_DIR) -name '*.c')
ASM_SOURCES = $(shell find $(SRC_DIR) -name '*.asm')

C_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
ASM_OBJS = $(patsubst $(SRC_DIR)/%.asm, $(BUILD_DIR)/%.o, $(ASM_SOURCES))
ALL_OBJS = $(ASM_OBJS) $(C_OBJS)

.PHONY: all
all: run

run: onien.iso
	$(QEMU)

onien.iso: kernel.elf
	grub-mkrescue -o $@ $(ONIEN_DIR)

kernel.elf: $(ALL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)
	@mkdir -p $(ONIEN_DIR)/boot/kernel
	@mv $@ $(ONIEN_DIR)/boot/kernel/

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(@D)
	$(ASM) $(ASMFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(ALL_OBJS) kernel.elf onien.iso
	rm -rf $(BUILD_DIR)