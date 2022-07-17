#include "fs.hpp"
#include <bitmap>
#include <cstdio>
#include <shared_memory>

static size_t _allocBitmap(LBA begin, LBA end) {
	LBA lba = begin;
	size_t bmSize = info.sectorSize * W_CHAR;

	std::SMID smid = std::smMake();
	uint8_t* buffer = (uint8_t*)std::smMap(smid);
	std::smAllow(smid, info.block);

	size_t ret = 0;
	while(lba < end) {
		if(!readSectors(lba, smid, 1))
			break;

		std::bitmap bm(bmSize, buffer);
		size_t nth = bm.getFirstZeroAndFlip();
		if(nth == bmSize)
			continue; // Tough luck

		// Nice
		if(writeSectors(lba, smid, 1)) {
			LBA sector = lba - begin;
			nth += sector * bmSize;
			ret = nth;
		}

		break;
	}

	std::munmap(buffer);
	std::smDrop(smid);
	return ret;
}

static bool _freeBitmap(LBA begin, size_t idx) {
	std::SMID smid = std::smMake();
	uint8_t* buffer = (uint8_t*)std::smMap(smid);
	std::smAllow(smid, info.block);

	size_t bmSize = info.sectorSize * W_CHAR;
	LBA lba = begin + idx / bmSize;
	if(!readSectors(lba, smid, 1)) {
		std::munmap(buffer);
		std::smDrop(smid);
		return false;
	}

	Inodei relidx = idx % bmSize;
	std::bitmap bm(bmSize, buffer);
	if(!bm.get(relidx)) {
		std::printf("[StrifeFS] Tried to free a free item\n");
		return false;
	}

	bm.set(relidx, false);
	bool ret = writeSectors(lba, smid, 1);
	std::munmap(buffer);
	std::smDrop(smid);
	return ret;
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
