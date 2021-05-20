all: BootLoader.bin

# options:
#  FLOPPY: decide where the os image will reside between 1.44 and 2.88 floppies
#    possible values: [144, 288]
FLOPPY ?= 288

BootLoader.bin: BootLoader.asm
	nasm -D FLOPPY=$(FLOPPY) -o BootLoader.bin BootLoader.asm
	
clean:
	rm -f BootLoader.bin\
