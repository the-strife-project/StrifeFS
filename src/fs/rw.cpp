#include "fs.hpp"
#include <shared_memory>

static std::pair<size_t, size_t> inode2sector(Inodei i) {
	size_t sector = (i * sizeof(Inode)) / info.sectorSize;
	sector += info.sb.firstInode;
	size_t offset = i % (info.sectorSize / sizeof(Inode));
	return {sector, offset};
}

static std::pair<size_t, size_t> block2sector(Blocki b) {
	size_t sector = (b * BLOCK_SIZE) / info.sectorSize;
	sector += info.sb.firstBlock;
	size_t offset = b % (info.sectorSize / BLOCK_SIZE);
	return {sector, offset};
}

Inode* readInode(Inodei i) {
	auto sector = inode2sector(i);

	std::SMID smid = std::smMake();
	uint8_t* buffer = (uint8_t*)std::smMap(smid);
	std::smAllow(smid, info.block);

	Inode* ret = nullptr;
	if(readSectors(sector.f, smid, 1)) {
		Inode* orig = (Inode*)(buffer + sector.s * sizeof(Inode));
		ret = new Inode(*orig);
	}

	std::munmap(buffer);
	std::smDrop(smid);
	return ret;
}

bool writeInode(Inodei i, const Inode& copy) {
	auto sector = inode2sector(i);

	std::SMID smid = std::smMake();
	uint8_t* buffer = (uint8_t*)std::smMap(smid);
	std::smAllow(smid, info.block);

	bool ret = false;
	if(readSectors(sector.f, smid, 1)) {
		Inode* orig = (Inode*)(buffer + sector.s * sizeof(Inode));
		*orig = copy;
		ret = writeSectors(sector.f, smid, 1);
	}

	std::munmap(buffer);
	std::smDrop(smid);
	return ret;
}

uint8_t* readBlock(Blocki b) {
	auto sector = block2sector(b);

	std::SMID smid = std::smMake();
	uint8_t* buffer = (uint8_t*)std::smMap(smid);
	std::smAllow(smid, info.block);

	uint8_t* ret = nullptr;
	if(readSectors(sector.f, smid, 1)) {
		ret = new uint8_t[BLOCK_SIZE];
		memcpy(ret, buffer + sector.s * BLOCK_SIZE, BLOCK_SIZE);
	}

	std::munmap(buffer);
	std::smDrop(smid);
	return ret;
}

bool writeBlock(Blocki b, uint8_t* data) {
	auto sector = block2sector(b);

	std::SMID smid = std::smMake();
	uint8_t* buffer = (uint8_t*)std::smMap(smid);
	std::smAllow(smid, info.block);

	bool ret = false;
	if(readSectors(sector.f, smid, 1)) {
		uint8_t* orig = buffer + sector.s * BLOCK_SIZE;
		memcpy(orig, data, BLOCK_SIZE);
		ret = writeSectors(sector.f, smid, 1);
	}

	std::munmap(buffer);
	std::smDrop(smid);
	return ret;
}
