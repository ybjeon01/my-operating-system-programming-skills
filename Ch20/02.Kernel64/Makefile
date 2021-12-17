# All object files will be in Temp directory and Kernel64.bin will be in
# 02.Kernel64

OBJECTDIRECTORY = Temp
SOURCEDIRECTORY = Source

OBJECTFILE = Kernel64.bin

all: prepare $(OBJECTFILE)

prepare:
	mkdir -p $(OBJECTDIRECTORY)

# Kernel64.bin is compiled by child process
$(OBJECTFILE): ExecuteInternalBuild

# Execute two child processes:
# one for preparing and
# one for compiling
ExecuteInternalBuild:
	@echo ===    Make Dependency File    ===
	make -C $(OBJECTDIRECTORY) -f ../Makefile InternalDependency
	@echo === Dependency Search Complete ===

	make -C $(OBJECTDIRECTORY) -f ../Makefile Compile

clean:
	rm -f *.bin
	rm -rf $(OBJECTDIRECTORY)

################################################################################
# below code is related to making Kernel64.bin
# There will be multiple files written in C
# below dependencies are run in child process
# with TEMP as cwd
#
# One more thing to notice is that EntryPoint.bin is not concatenated with
# C kernel. Intead, EntryPoint.s is compile as object file and linker links all
# objects files
# In 01.Kernel32, EntryPoint.s must be at 0x10000 and Main.c at 0x10200 in
# memory layout. However, it is just one way to make a binary program.
# In Kernel64, Every file is compile as object files, and linker script set
# EntryPoint.s at 0x200000(2MB). That's it
################################################################################

NASM64 = nasm -f elf64
GCC64 = gcc -m64 -c -ffreestanding -fno-pie
LD64 = ld -T ../binary_amd64.x -nostdlib

ASSEMBLYSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.asm)
CSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.c)

ASSEMBLYOBJECTFILES = $(notdir $(patsubst %.asm, %.o, $(ASSEMBLYSOURCEFILES)))
CENTRYPOINTOBJECTFILE = Main.o
# all C object files except Main.o
COBJECTFILES = $(notdir $(patsubst %.c, %.o, $(CSOURCEFILES)))

InternalDependency:
	$(GCC64) -MM $(CSOURCEFILES) > Dependency.dep

Compile: EntryPoint.o  $(CENTRYPOINTOBJECTFILE) $(COBJECTFILES) $(ASSEMBLYOBJECTFILES)
	$(LD64) -o ../$(OBJECTFILE) $^

# Compile EntryPoint.s that switches Real Mode to Protected Mode
EntryPoint.o: ../$(SOURCEDIRECTORY)/EntryPoint.s
	$(NASM64) -o $@ $<

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif

%.o: ../$(SOURCEDIRECTORY)/%.c
	$(GCC64) -c $<

%.o: ../$(SOURCEDIRECTORY)/%.asm
	$(NASM64) -f elf64 -o $@ $<
