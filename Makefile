CC      = arm-none-eabi-gcc
CFLAGS  = -I . -std=gnu99
CFLAGS += -fno-common -O1
CFLAGS += -g3 -mcpu=cortex-m0 -mthumb -DSTM32F030x6
CFLAGS += -Wall
LFLAGS  = -Tstm32.ld -nostartfiles

GDB = arm-none-eabi-gdb
OOCD = openocd
OOCDFLAGS = -f interface/stlink-v2.cfg -f target/stm32f0x.cfg

BINNAME = tard.elf
CSRC = main.c board.c sound.o exterminate.o

all: $(BINNAME)

clean:
	rm -f *.elf *.o

program: $(BINNAME)
	$(OOCD) $(OOCDFLAGS) \
	  -c "init" -c "targets" -c "reset halt" \
	  -c "flash write_image erase $<" -c "verify_image $<" \
	  -c "reset run" -c "shutdown"

debug: $(BINNAME)
	$(GDB) -iex 'target extended | $(OOCD) $(OOCDFLAGS) -c "gdb_port pipe"' \
               -iex 'mon halt' $<

sound.o: sound.raw
	arm-none-eabi-ld -r -b binary -o $@ $<

exterminate.o: exterminate.raw
	arm-none-eabi-ld -r -b binary -o $@ $<

$(BINNAME): $(CSRC) *.h stm32.ld
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $(CSRC)  


