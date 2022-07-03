#include <common.hpp>
#include <shared_memory>
#include <rpc>
#include <mutex>
#include "fs/structures.hpp"
#include "fs/fs.hpp"

static std::mutex setupLock;
static bool isSetup = false;
bool setup(std::PID client, uint64_t uuida, uint64_t uuidb, bool mustFormat) {
	IGNORE(client);

	setupLock.acquire();
	if(isSetup) {
		setupLock.release();
		return false;
	}

	info.uuid = {uuida, uuidb};
	if(!peerBlockDevice()) {
		setupLock.release();
		return false;
	}

	// This will be changed in a long long time
	info.firstFreeSector = 0;
	info.sectorSize = PAGE_SIZE;
	info.inodesPerSector = info.sectorSize / sizeof(Inode);
	info.blocksPerSector = info.sectorSize / BLOCK_SIZE;

	if(mustFormat) {
		if(!format()) {
			setupLock.release();
			return false;
		}
	}

	isSetup = true;
	setupLock.release();
	return true;
}

bool connect(std::PID client, std::SMID smid) {
	return std::sm::connect(client, smid);
}

// list
// read
// write

void publish() {
	std::exportProcedure((void*)setup, 3);
	std::exportProcedure((void*)connect, 1);
	std::enableRPC();
	std::publish("StrifeFS");
	std::halt();
}
