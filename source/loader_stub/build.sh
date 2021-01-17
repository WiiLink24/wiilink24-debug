set -e

# $(CC) $(LDFLAGS) -Wl,-n -o $@ -Wl,-Tlinker-high.lds $(OBJS) -lm
powerpc-eabi-gcc -mcpu=750 -meabi -mhard-float -nostdlib -mregnames -Wl,-n -o loader_stub.elf -Wl,-Tstub.lds loader_stub.S -lm
# $(OBJCOPY) -O binary $< $@
powerpc-eabi-objcopy -O binary loader_stub.elf loader_stub.bin

# for verification
hexf loader_stub.bin
