VERSION ?= 2

TARGET = main
OBJS = main.o startup_stm32f4.o fmc.o

COMMONFLAGS = -g -gdwarf-2 -mcpu=cortex-m4 -mthumb -I. -Iinclude
CFLAGS += $(COMMONFLAGS) -fpack-struct -Wall -O2
CFLAGS += -D VERSION=$(VERSION)
ASFLAGS = $(CFLAGS)
LDFLAGS = $(COMMONFLAGS) -fno-exceptions -ffunction-sections -fdata-sections -nostartfiles -Wl,--gc-sections,-Tstm32f446zc.ld

CC = arm-none-eabi-gcc
AS = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size
GDB = arm-none-eabi-gdb

all: $(TARGET).elf $(TARGET).bin $(TARGET).lst size

$(TARGET).elf: $(OBJS)
	$(CC) -o $(TARGET).elf $(LDFLAGS) -Wl,-Map,$(TARGET).map $^

%.bin: %.elf
	$(OBJCOPY) -O binary $^ $@

%.lst: %.elf
	$(OBJDUMP) -h -S $^ >$@

.PHONY: size burn clean gdb

size:
	$(SIZE) --format=berkeley $(TARGET).elf

burn:
	stm32f4_burn

gdb:
	$(GDB) -ex "target remote :3333" -ex "break main" -ex "monitor reset halt" -ex "monitor gdb_breakpoint_override hard" $(TARGET).elf

clean:
	rm -f $(TARGET).{elf,bin,lst,map} $(OBJS)
