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

		std::SMID smid = std::smMake();
		info.buffer = (uint8_t*)std::smMap(smid);
		std::smAllow(smid, info.block);
		bool result = std::rpc(info.block, std::block::CONNECT, smid);
		if(!result)
			return false;
	}

	return true;
}

std::mutex deviceLock;

bool readSector(LBA lba) {
	if(!deviceLock.isAcquired()) {
		std::printf("[StrifeFS] readSector: deviceLock not acquired!\n");
		std::exit(9);
	}

	return std::rpc(info.block,
					std::block::READ,
					info.uuid.a,
					info.uuid.b,
					lba * info.sectorSize,
					info.sectorSize);
}

bool writeSector(LBA lba) {
	if(!deviceLock.isAcquired()) {
		std::printf("[StrifeFS] writeSector: deviceLock not acquired!\n");
		std::exit(9);
	}

	return std::rpc(info.block,
					std::block::WRITE,
					info.uuid.a,
					info.uuid.b,
					lba * info.sectorSize,
					info.sectorSize);
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
	memcpy(buffer, &sb, sizeof(Superblock));
	bool ret = writeSector(firstFreeSector);
	if(!dontLock) deviceLock.release();

	return ret;
}
