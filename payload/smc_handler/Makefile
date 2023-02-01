CROSS_COMPILE = arm-none-eabi-

# Use our cross-compile prefix to set up our basic cross compile environment.
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

CFLAGS = \
	-mtune=generic-armv7-a \
	-mlittle-endian \
	-mcpu=cortex-a9 \
	-fno-stack-protector \
	-fpic \
	-fno-common \
	-fno-builtin \
	-ffreestanding \
	-fomit-frame-pointer \
	-std=gnu99 \
	-Werror \
	-Wall \
	-Wno-error=unused-function \
	-g \
	-O0 \
	

LDFLAGS = -nostdlib -L /usr/lib/gcc/arm-none-eabi/9.2.1 -lgcc 

all: primary.bin secondary.bin tertiary.bin

primary.elf: primary.o ../common/system_regs.o ../common/tegra30_uart.o ../common/printf.o
	$(LD) $^ -T primary.lds  $(LDFLAGS) -Map=primary.map -o $@
	
# 
secondary.elf: other_cores.o ../common/system_regs.o ../common/tegra30_uart.o ../common/printf.o
	$(LD) $^ -T secondary.lds  $(LDFLAGS) -Map=secondary.map -o $@

tertiary.elf: other_cores.o
	$(LD) $^ -T tertiary.lds  $(LDFLAGS) -Map=tertiary.map -o $@

quaternary.elf: other_cores.o
	$(LD) $^ -T quaternary.lds  $(LDFLAGS) -Map=quaternary.map -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) $< -c -o $@

%.o: %.S
	$(CC) $(CFLAGS) $(DEFINES) $< -c -o $@

%.bin: %.elf
	$(OBJCOPY) -v -O binary $< $@

clean:
	rm *.o *.bin *.elf *.map
