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
	-O0

OBJS = 

LDFLAGS = -T s_park_core.lds -nostdlib -L /usr/lib/gcc/arm-none-eabi/9.2.1 -lgcc -Map=$(basename $@).map

all: s_park_core_1.bin s_park_core_2.bin s_park_core_3.bin

s_park_core_1.elf: s_park_core.o $(OBJS)
	$(LD) $^ --defsym LOAD_ADDR=0x00000000 $(LDFLAGS) -o $@

s_park_core_2.elf: s_park_core.o $(OBJS)
	$(LD) $^ --defsym LOAD_ADDR=0x00000000 $(LDFLAGS) -o $@

s_park_core_3.elf: s_park_core.o $(OBJS)
	$(LD) $^ --defsym LOAD_ADDR=0x00000000 $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) $< -c -o $@

%.o: %.S
	$(CC) $(CFLAGS) $(DEFINES) $< -c -o $@

%.bin: %.elf
	$(OBJCOPY) -v -O binary $< $@

clean:
	rm *.o *.bin *.elf *.map
