
CROSS_COMPILE = arm-none-eabi-

# Use our cross-compile prefix to set up our basic cross compile environment.
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

CFLAGS = \
	-mtune=generic-armv7-a \
	-mlittle-endian \
	-fno-stack-protector \
	-fpic \
	-fno-common \
	-fno-builtin \
	-ffreestanding \
	-std=gnu99 \
	-Werror \
	-Wall \
	-Wno-error=unused-function \
	-fomit-frame-pointer \
	-g \
	-O0 \
	-mcpu=cortex-a9

LDFLAGS = -nostdlib -L /usr/lib/gcc/arm-none-eabi/9.2.1 -lgcc 

all: payload.bin payload_secondary.bin

payload.elf: payload.o system_regs.o tegra30_uart.o printf.o mmu_dump.o
	$(LD) $^ -T payload.lds  $(LDFLAGS) -Map=payload.map -o $@
	
payload_secondary.elf: payload.o system_regs.o tegra30_uart.o printf.o mmu_dump.o
	$(LD) $^ -T payload_secondary.lds  $(LDFLAGS) -Map=payload_secondary.map -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) $< -c -o $@

%.o: %.S
	$(CC) $(CFLAGS) $(DEFINES) $< -c -o $@

%.bin: %.elf
	$(OBJCOPY) -v -O binary $< $@

clean:
	rm *.o *.bin *.elf *.map
