#include "fs.hpp"
#include "structures.hpp"
#include <cstdio>

bool format() {
	info.sb = Superblock();

	// Random numbers, doesn't really matter at the moment
	info.sb.inodeCount = info.sb.freeInodes = 1 << 12;
	info.sb.blockCount = info.sb.freeBlocks = 1 << 12;

	// First sector has the superblock, so let's put the bitmap in the second
	info.sb.inodeBitmap = info.firstFreeSector + 1;
	// How big is it?
	LBA ibsz = info.sb.inodeCount / (W_CHAR * info.sectorSize);
	++ibsz; // That's right

	// Next, block bitmap
	info.sb.blockBitmap = info.sb.inodeBitmap + 1 + ibsz;
	// How big is it?
	LBA bbsz = info.sb.blockCount / (W_CHAR * info.blocksPerSector);
	++bbsz;

	// Inode section
	info.sb.firstInode = info.sb.blockBitmap + 1 + bbsz;
	LBA isz = info.sb.inodeCount / info.inodesPerSector;
	++isz;

	// Finally, block section
	info.sb.firstBlock = info.sb.firstInode + 1 + isz;
	// That's it



	// Copy time. First, superblock
	deviceLock.acquire();

	memcpy(info.buffer, &(info.sb), sizeof(Superblock));
	if(!writeSector(info.firstFreeSector))
		return false;
	// Bitmaps
	memset(info.buffer, 0, PAGE_SIZE);
	for(size_t i=0; i<ibsz; ++i)
		if(!writeSector(info.sb.inodeBitmap + i))
			return false;
	for(size_t i=0; i<bbsz; ++i)
		if(!writeSector(info.sb.blockBitmap + i))
			return false;

	deviceLock.release();

	// Allocate first inode and block so they're reserved
	allocInode(); allocBlock();

	return makeRoot();
}
