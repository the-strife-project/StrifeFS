#include "fs.hpp"
#include "structures.hpp"
#include <cstdio>
#include <shared_memory>

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
	info.sbLock.acquire();
	info.flushSuperblock();
	info.sbLock.release();

	// Initialize bitmaps
	deviceLock.acquire();
	std::SMID smid = std::smMake();
	uint8_t* buffer = (uint8_t*)std::smMap(smid);
	std::smAllow(smid, info.block);
	memset(buffer, 0, PAGE_SIZE);
	bool ok = true;
	for(size_t i=0; i<ibsz && ok; ++i)
		if(!writeSectors(info.sb.inodeBitmap + i, smid, 1))
			ok = false;
	for(size_t i=0; i<bbsz && ok; ++i)
		if(!writeSectors(info.sb.blockBitmap + i, smid, 1))
			ok = false;
	deviceLock.release();
	std::munmap(buffer);
	std::smDrop(smid);
	if(!ok)
		return false;

	// Allocate first inode and block so they're reserved
	allocInode(); allocBlock();

	// Make the root
	if(!makeRoot())
		return false;

	deviceLock.acquire();
	Inode* root = readInode(1);
	if(!root) {
		deviceLock.release();
		return false;
	}
	deviceLock.release();

	std::ACLEntry acl;
	acl.isUser = true;
	acl.allow = true;
	acl.read = true;
	acl.write = true;
	root->addACL(SYSTEM_UID, acl);

	deviceLock.acquire();
	if(!writeInode(1, *root)) {
		deviceLock.release();
		delete root;
		return false;
	}
	deviceLock.release();

	delete root;
	return true;
}
