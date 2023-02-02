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

all: s_primary.bin s_secondary.bin

s_primary.elf: s_primary.o ../common/system_regs.o ../common/tegra30_uart.o ../common/printf.o ../common/lock.o
	$(LD) $^ -T s_primary.lds  $(LDFLAGS) -Map=s_primary.map -o $@
	
# 
s_secondary.elf: s_primary.o ../common/system_regs.o ../common/tegra30_uart.o ../common/printf.o ../common/lock.o
	$(LD) $^ -T s_secondary.lds  $(LDFLAGS) -Map=s_secondary.map -o $@

s_tertiary.elf: s_other_cores.o ../common/system_regs.o ../common/tegra30_uart.o ../common/printf.o
	$(LD) $^ -T s_tertiary.lds  $(LDFLAGS) -Map=s_tertiary.map -o $@

s_quaternary.elf: s_other_cores.o ../common/system_regs.o ../common/tegra30_uart.o ../common/printf.o
	$(LD) $^ -T s_quaternary.lds  $(LDFLAGS) -Map=s_quaternary.map -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) $< -c -o $@

%.o: %.S
	$(CC) $(CFLAGS) $(DEFINES) $< -c -o $@

%.bin: %.elf
	$(OBJCOPY) -v -O binary $< $@

clean:
	rm *.o *.bin *.elf *.map
	rm ../common/*.o
