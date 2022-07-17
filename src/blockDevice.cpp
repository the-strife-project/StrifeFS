#include <common.hpp>
#include <shared_memory>
#include <rpc>
#include <userspace/block.hpp>
#include <cstdio>

bool peerBlockDevice() {
	if(!info.block) {
		info.block = std::resolve("block");
		if(!info.block)
			return false;

		bool ret = std::rpc(info.block,
							std::block::SELECT,
							info.uuid.a,
							info.uuid.b);

		if(!ret)
			return false;
	}

	return true;
}

std::mutex deviceLock;

bool readSectors(LBA lba, std::SMID smid, size_t n) {
	if(!deviceLock.isAcquired()) {
		std::printf("[StrifeFS] readSector: deviceLock not acquired!\n");
		std::exit(9);
	}

	return std::rpc(info.block,
					std::block::READ,
					smid,
					lba * info.sectorSize,
					n * info.sectorSize);
}

bool writeSectors(LBA lba, std::SMID smid, size_t n) {
	if(!deviceLock.isAcquired()) {
		std::printf("[StrifeFS] writeSector: deviceLock not acquired!\n");
		std::exit(9);
	}

	return std::rpc(info.block,
					std::block::WRITE,
					smid,
					lba * info.sectorSize,
					n * info.sectorSize);
}

bool Info::flushSuperblock(bool dontLock) {
	if(!sbLock.isAcquired()) {
		std::printf("[StrifeFS] flushSuperblock: sbLock not acquired!\n");
		std::exit(9);
	}
	if(dontLock && !deviceLock.isAcquired()) {
		std::printf("[StrifeFS] flushSuperblock: deviceLock not acquired!\n");
		std::exit(9);
	}

	if(!dontLock) deviceLock.acquire();
	std::SMID smid = std::smMake();
	uint8_t* buffer = (uint8_t*)std::smMap(smid);
	std::smAllow(smid, info.block);

	memcpy(buffer, &sb, sizeof(Superblock));
	bool ret = writeSectors(firstFreeSector, smid, 1);

	std::munmap(buffer);
	std::smDrop(smid);
	if(!dontLock) deviceLock.release();

	return ret;
}
