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

LDFLAGS = -nostdlib -L /usr/lib/gcc/arm-none-eabi/9.2.1 -lgcc

all: s_secondary.bin 

s_secondary.elf: s_secondary.o
	$(LD) $^ -T s_secondary.lds  $(LDFLAGS) -Map=s_secondary.map -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) $< -c -o $@

%.o: %.S
	$(CC) $(CFLAGS) $(DEFINES) $< -c -o $@

%.bin: %.elf
	$(OBJCOPY) -v -O binary $< $@

clean:
	rm *.o *.bin *.elf *.map
