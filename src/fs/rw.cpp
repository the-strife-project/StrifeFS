#include "fs.hpp"

static std::pair<size_t, size_t> inode2sector(Inodei i) {
	size_t sector = (i * sizeof(Inode)) / info.sectorSize;
	sector += info.firstFreeSector;
	size_t offset = i % (info.sectorSize / sizeof(Inode));
	return {sector, offset};
}

static std::pair<size_t, size_t> block2sector(Blocki b) {
	size_t sector = (b * BLOCK_SIZE) / info.sectorSize;
	sector += info.firstFreeSector;
	size_t offset = b % (info.sectorSize / BLOCK_SIZE);
	return {sector, offset};
}

Inode* readInode(Inodei i) {
	auto sector = inode2sector(i);
	if(!readSector(sector.f))
		return nullptr;

	Inode* orig = (Inode*)(info.buffer + sector.s * sizeof(Inode));
	Inode* ret = new Inode(*orig);
	return ret;
}

bool writeInode(Inodei i, const Inode& copy) {
	auto sector = inode2sector(i);
	if(!readSector(sector.f))
		return false;

	Inode* orig = (Inode*)(info.buffer + sector.s * sizeof(Inode));
	*orig = copy;

	return writeSector(sector.f);
}

uint8_t* readBlock(Blocki b) {
	auto sector = block2sector(b);
	if(!readSector(sector.f))
		return nullptr;

	uint8_t* orig = info.buffer + sector.s * BLOCK_SIZE;
	uint8_t* ret = new uint8_t[BLOCK_SIZE];
	memcpy(ret, orig, BLOCK_SIZE);
	return ret;
}

bool writeBlock(Blocki b, uint8_t* data) {
	auto sector = block2sector(b);
	if(!readSector(sector.f))
		return false;

	uint8_t* orig = info.buffer + sector.s * BLOCK_SIZE;
	memcpy(orig, data, BLOCK_SIZE);

	return writeSector(sector.f);
}
