
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

all: payload.bin

payload.elf: payload.o payload_asm.o pagetables.o
	$(LD) $^ -T payload.lds  $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) $< -c -o $@

%.o: %.S
	$(CC) $(CFLAGS) $(DEFINES) $< -c -o $@

%.bin: %.elf
	$(OBJCOPY) -v -O binary $< $@
