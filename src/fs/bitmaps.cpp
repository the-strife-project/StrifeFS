#include "fs.hpp"
#include <bitmap>
#include <cstdio>

static size_t _allocBitmap(LBA begin, LBA end) {
	LBA lba = begin;
	size_t bmSize = info.sectorSize * W_CHAR;

	while(lba < end) {
		if(!readSector(lba))
			return 0;

		std::bitmap bm(bmSize, info.buffer);
		size_t ret = bm.getFirstZeroAndFlip();
		if(ret == bmSize)
			continue; // Tough luck

		// Nice
		if(!writeSector(lba))
			return 0;

		LBA sector = lba - begin;
		ret += sector * bmSize;

		return ret;
	}

	// None free!
	return 0;
}

static size_t _freeBitmap(LBA begin, size_t idx) {
	size_t bmSize = info.sectorSize * W_CHAR;
	LBA lba = begin + idx / bmSize;
	if(!readSector(lba))
		return false;

	Inodei relidx = idx % bmSize;
	std::bitmap bm(bmSize, info.buffer);
	if(!bm.get(relidx)) {
		std::printf("[StrifeFS] Tried to free a free item\n");
		return false;
	}

	bm.set(relidx, false);
	return writeSector(lba);
}



static inline size_t allocBitmap(LBA begin, LBA end, bool dontLock) {
	if(dontLock && !deviceLock.isAcquired()) {
		std::printf("[StrifeFS] allocBitmap: dontLock and not acquired\n");
		std::exit(3);
	}

	if(!dontLock) deviceLock.acquire();
	auto ret = _allocBitmap(begin, end);
	if(!dontLock) deviceLock.release();

	return ret;
}

static inline size_t freeBitmap(LBA begin, size_t idx) {
	deviceLock.acquire();
	auto ret = _freeBitmap(begin, idx);
	deviceLock.release();

	return ret;
}



Inodei allocInode() {
	info.sbLock.acquire();

	if(!info.sb.freeInodes) {
		info.sbLock.release();
		return 0;
	}

	// blockBitmap is right after inodeBitmap
	auto ret = allocBitmap(info.sb.inodeBitmap, info.sb.blockBitmap, false);
	if(ret) {
		--info.sb.freeInodes;
		info.flushSuperblock();
	}

	info.sbLock.release();
	return ret;
}

bool freeInode(Inodei idx) {
	info.sbLock.acquire();

	auto ret = freeBitmap(info.sb.inodeBitmap, idx);
	if(ret) {
		++info.sb.freeInodes;
		info.flushSuperblock();
	}

	info.sbLock.release();
	return ret;
}

uint8_t* zeros = nullptr;
Blocki allocBlock(bool dontLock) {
	info.sbLock.acquire();

	if(!info.sb.freeBlocks) {
		info.sbLock.release();
		return 0;
	}

	// firstInode is right after blockBitmap
	auto ret = allocBitmap(info.sb.blockBitmap, info.sb.firstInode, dontLock);
	if(ret) {
		--info.sb.freeBlocks;
		info.flushSuperblock(dontLock);

		// Housekeeping
		if(!zeros) {
			zeros = new uint8_t[BLOCK_SIZE];
			memset(zeros, 0, BLOCK_SIZE);
		}
		if(!writeBlock(ret, zeros)) {
			info.sbLock.release();
			return 0;
		}
	}

	info.sbLock.release();
	return ret;
}

bool freeBlock(Blocki idx) {
	info.sbLock.acquire();

	auto ret = freeBitmap(info.sb.blockBitmap, idx);
	if(ret) {
		++info.sb.freeBlocks;
		info.flushSuperblock();
	}

	info.sbLock.release();
	return ret;
}
