C = $(CROSS_COMPILE)gcc
PRULIB ?= $(PRULIB_PREFIX)/lib
PRUINC ?= $(PRULIB_PREFIX)/include

CFLAGS += -Wall -I$(PRUINC) -D__DEBUG -O2 \
	-mtune=cortex-a8 -march=armv7-a \
	-std=c99 -lrt
LDFLAGS += -L$(PRULIB) -lprussdrv

TARGET = test0

ASM_SRC = test0.p
ASM_DEP = hdr.hp mem_map.hp
FIRMWARE = $(patsubst %.p, %.bin, $(ASM_SRC))

C_SRC = test0.c
C_OBJ = $(patsubst %.c,%.o,$(C_SRC))
C_DEP = mem_map.h

default: $(TARGET) $(FIRMWARE)
	@echo
	@echo Finished building $^
	@echo

$(TARGET): $(C_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(C_OBJ): $(SOURCES) $(C_DEP)

%.o: %.c $(C_DEP)
	$(CC) $(CFLAGS) -c -o $@ $<

%.bin: %.p $(ASM_DEP)
	$(PASM) -V3 -b $<

%.h %.hp: %.in
	python make_structs.py $<

.PHONY: clean

clean:
	-rm -rf $(FIRMWARE) $(TARGET) $(C_OBJ)
	-rm mem_map.h mem_map.hp
