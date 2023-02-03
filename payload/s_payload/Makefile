CROSS_COMPILE = arm-none-eabi-

# Use our cross-compile prefix to set up our basic cross compile environment.
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

CFLAGS = \
	-mtune=generic-armv7-a \
	-mcpu=cortex-a9 \
	-mlittle-endian \
	-fno-stack-protector \
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
	-fpic 

OBJS = ../common/system_regs.o ../common/tegra30_uart.o ../common/printf.o ../common/lock.o

LDFLAGS = -T s_payload.lds -nostdlib -L /usr/lib/gcc/arm-none-eabi/9.2.1 -lgcc -Map=$(basename $@).map

all: s_primary.bin s_secondary.bin s_tertiary.bin s_quaternary.bin

s_primary.elf: s_payload.o $(OBJS)
	$(LD) $^ --defsym LOAD_ADDR=0x80112174 $(LDFLAGS) -o $@
	
s_secondary.elf: s_payload.o $(OBJS)
	$(LD) $^ --defsym LOAD_ADDR=0x83100000 $(LDFLAGS) -o $@

s_tertiary.elf: s_payload.o $(OBJS)
	$(LD) $^ --defsym LOAD_ADDR=0x00000000 $(LDFLAGS) -o $@

s_quaternary.elf: s_payload.o $(OBJS)
	$(LD) $^ --defsym LOAD_ADDR=0x00000000 $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) $< -c -o $@

%.o: %.S
	$(CC) $(CFLAGS) $(DEFINES) $< -c -o $@

%.bin: %.elf
	$(OBJCOPY) -v -O binary $< $@

clean:
	rm *.o *.bin *.elf *.map
	rm ../common/*.o
