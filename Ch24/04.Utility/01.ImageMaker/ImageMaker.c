/* ImageMaker program concatenate Bootloader.bin. Kernel32.bin, and
 * Kernel64.bin to make Disk.img in a way that you do not have to change
 * TOTALSECTORCOUNT and KERNEL32SECTORCOUNT part of BootLoader.asm manually.
 * Also, This program aligns Bootloader.bin and Kernel32.bin into multiply
 * of 512 so the image works on QEMU without problem.
 * Old QEMU fails to load floppy image that is not multiply of 512.
 * The reason that I made this program is that I do not want to change
 * TOTALSECTORCOUNT in BootLoader.asm manually every time I add more C code.
 * Actually, I cannot expect how many bytes the c code will occupy in Disk.img
 * before compiling.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define BYTESOFSECTOR 512

int AdjustInSectorSize(int iFD, int iSourceSize);
void WriteKernelInformation(
    int iTargetFd,
    int iTotalKernelSectorCount,
    int iKernel32SectorCount
);
int ConcatenateFile(int iSourceFd, int iTargetFd);

int main(int argc, char* argv[]) {
    int iSourceFd;
    int iTargetFd;
    int iBootLoaderSize;
    int iKernel32SectorCount;
    int iKernel64SectorCount;
    int iSourceSize;

    if (argc < 4) {
    	fprintf(
            stderr,
            "[ERROR] ImageMaker BootLoader.bin Kernel32.bin Kernel64.bin\n"
        );
    	exit(-1);
    }

    // Section 1
    // make a empty Disk.img file and put bootloader into Disk.img
    // and then add 0x00 padding to the end of the Disk, so its size
    // is multiply of 512
    if ((iTargetFd = open("Disk.img", O_RDWR | O_CREAT | O_TRUNC,
    		S_IREAD | S_IWRITE)) == -1) {
    	fprintf(stderr, "[ERROR] Disk.img open fail.\n");
    	exit(-1);
    }

    printf("[INFO] Copy boot loader to image file\n");
    if ((iSourceFd = open(argv[1], O_RDONLY)) == -1) {
    	fprintf(stderr, "[ERROR] %s open fail\n", argv[1]);
    	exit(-1);
    }

    // copy bootloader to Disk.img
    // ISourceSize is size of bootloader
    iSourceSize = ConcatenateFile(iSourceFd, iTargetFd);
    close(iSourceFd);

    // add padding
    // iBootLoaderSize is number of sectors of (size of bootloader + padding)
    iBootLoaderSize = AdjustInSectorSize(iTargetFd, iSourceSize);
    printf("[INFO] %s size = [%d] and sector count = [%d]\n",
    		argv[1], iSourceSize, iBootLoaderSize);

    printf("[INFO] Copy protected mode kernel to image file\n");

    // Section 2
    // add Kernel32.bin to Disk.img
    if ((iSourceFd = open(argv[2], O_RDONLY)) == -1) {
    	fprintf(stderr, "[ERROR] %s open fail\n", argv[2]);
    	exit(-1);
    }

    // copy Kernel32.bin to Disk.img
    // ISourceSize is size of Kernel32.bin
    iSourceSize = ConcatenateFile(iSourceFd, iTargetFd);
    close(iSourceFd);

    // add padding
    // iKernel32SectorCount is number of sectors of
    // (size of Kernel32.bin + padding)
    iKernel32SectorCount = AdjustInSectorSize(iTargetFd, iSourceSize);
    printf("[INFO] %s size = [%d] and sector count = [%d]\n",
    		argv[2], iSourceSize, iKernel32SectorCount);


    // Section 3
    // add Kernel64.bin to Disk.img

    // copy Kernel64.bin to Disk.img
    // iSourceSize is size of Kernel64.bin
    if ((iSourceFd = open(argv[3], O_RDONLY)) == -1) {
    	fprintf(stderr, "[ERROR] %s open fail\n", argv[3]);
    	exit(-1);
    }

    // copy Kernel64.bin to Disk.img
    // ISourceSize is size of Kernel64.bin
    iSourceSize = ConcatenateFile(iSourceFd, iTargetFd);
    close(iSourceFd);

    // add padding
    // iKernel64SectorCount is number of sectors of
    // (size of Kernel64.bin + padding)
    iKernel64SectorCount = AdjustInSectorSize(iTargetFd, iSourceSize);
    printf("[INFO] %s size = [%d] and sector count = [%d]\n",
    		argv[3], iSourceSize, iKernel64SectorCount);


    // Section 4
    // modify TOTALSECTORCOUNT part of Bootloader.asm
    printf("[INFO] Start to write kernel information\n");
    WriteKernelInformation(
        iTargetFd,
        iKernel32SectorCount + iKernel64SectorCount,
        iKernel32SectorCount
    );
    printf("[INFO] Image file create complete\n");

    close(iTargetFd);
    return 0;
}

// add 0x00 padding to the end of iFd so size of iFd is multiply of 512
// iFd: descriptor of target file
// iSourceSize: size of last file that was added to the iFd
// return: number of sectors of target file after adding padding 
// Instead of counting size of iFd from the first, this function uses
// iSourceSize to calculate the padding to add
int AdjustInSectorSize(int iFd, int iSourceSize) {
    int i;
    int iAdjustSizeToSector;
    char cCh;
    int iSectorCount;

    iAdjustSizeToSector = iSourceSize % BYTESOFSECTOR;
    cCh = 0x00;

    if (iAdjustSizeToSector != 0) {
    	iAdjustSizeToSector = BYTESOFSECTOR - iAdjustSizeToSector;
    	printf("[INFO] File size [%d] and fill [%d] byte\n", iSourceSize,
    			iAdjustSizeToSector);

    	for (i = 0; i < iAdjustSizeToSector; i++) {
    		write(iFd, &cCh, 1);
    	}
    }
    else {
    	printf("[INFO] File size is aligned 512 byte\n");
    }

    iSectorCount = (iSourceSize + iAdjustSizeToSector) / BYTESOFSECTOR;
    return iSectorCount;
}

// modify TOTALSECTORCOUNT part of Bootloader.asm so bootloader can
// load image to the memory
// iTargetFd: descriptor of target file
// iTotalKernelSectorCount: number of sectors to load to the memory
// iKernel32SectorCount: number of sectors of Kernel32.bin
void WriteKernelInformation(
    int iTargetFd,
    int iTotalKernelSectorCount,
    int iKernel32SectorCount
) {
    unsigned short usData;
    long lPosition;

    lPosition = lseek(iTargetFd, (off_t) 5, SEEK_SET);

    if (lPosition == -1) {
    	fprintf(stderr, "lseek fail. Return value = %ld, errno = %d, %d\n",
    			lPosition, errno, SEEK_SET);
    	exit(-1);
    }

    usData = (unsigned short) iTotalKernelSectorCount;
    write(iTargetFd, &usData, 2);
    printf("[INFO] Total sector count except boot loader [%d]\n",
    		iTotalKernelSectorCount);

    // file descriptor position pointer is incremented by 2
    // automatically. No need to call lseek again
    usData = (unsigned short) iKernel32SectorCount;
    write(iTargetFd, &usData, 2);
    printf("[INFO] Total sector count of protected mode kernel [%d]\n",
    		iKernel32SectorCount);
}

// concatenate Source file to the end of Target file
// iSourceFd: source file
// iTargetFd: target file
// return: size of source file
int ConcatenateFile(int iSourceFd, int iTargetFd) {
	int iSourceFileSize;
	int iRead;
	int iWrite;
	char vcBuffer[BYTESOFSECTOR];

	iSourceFileSize = 0;

	while (1) {
		iRead = read(iSourceFd, vcBuffer, sizeof(vcBuffer));
		iWrite = write(iTargetFd, vcBuffer, iRead);

		if (iRead != iWrite) {
			fprintf(stderr, "[ERROR] iRead != iWrite.. \n");
			exit(-1);
		}
		iSourceFileSize += iRead;

		if (iRead != sizeof(vcBuffer)) {
			break;
		}
	}
    return iSourceFileSize;
}
