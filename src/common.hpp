#ifndef COMMON_HPP
#define COMMON_HPP

#include <types>
#include <mutex>
#define W_CHAR 8

#include "fs/structures.hpp"

struct Info {
	std::UUID uuid;
	std::PID block = 0;
	uint8_t* buffer = nullptr;

	LBA firstFreeSector;
	size_t sectorSize;

	// Cached
	size_t inodesPerSector;
	size_t blocksPerSector;
	// Cached too
	std::mutex sbLock;
	Superblock sb;
	bool flushSuperblock(bool dontLock=false);
};

extern Info info;

void publish();

// block service related
bool peerBlockDevice();
extern std::mutex deviceLock;
bool readSector(LBA lba);
bool writeSector(LBA lba);

#endif
