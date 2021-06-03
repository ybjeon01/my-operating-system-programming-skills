# options:
#  FLOPPY: decide where the os image will reside between 1.44 and 2.88 floppies
#    possible values: [144, 288]


.PHONY: all clean run

all: Utility BootLoader Kernel32 Kernel64 Disk.img

BootLoader:
	@echo
	@echo ============== Build Boot Loader ==============
	@echo 

	make -C 00.BootLoader
	
	@echo ============== Build Complete ==============
	@echo

Kernel32: 
	@echo
	@echo ============== Build 32bit Kernel ==============
	@echo 

	make -C 01.Kernel32
	
	@echo ============== Build Complete ==============
	@echo

Kernel64: 
	@echo
	@echo ============== Build 64bit Kernel ==============
	@echo 

	make -C 02.Kernel64
	
	@echo ============== Build Complete ==============
	@echo	

Utility:
	@echo
	@echo ============== Build Utility programs ==============
	@echo 

	make -C 04.Utility

	@echo ============== Build Complete ==============
	@echo


Disk.img: 00.BootLoader/BootLoader.bin 01.Kernel32/Kernel32.bin 02.Kernel64/Kernel64.bin
	@echo
	@echo ============== Disk Image Build Start ==============
	@echo
	
	04.Utility/01.ImageMaker/ImageMaker $^
	
	@echo
	@echo ============== All Build Complete ==============
	@echo
	
clean:
	make -C 00.BootLoader clean
	make -C 01.Kernel32 clean
	make -C 02.Kernel64 clean
	make -C 04.Utility clean
	rm -f Disk.img
	
run:
	qemu-system-x86_64 -m 64 -fda Disk.img -rtc base=localtime -M pc