#include "../fs.hpp"

bool read(Inodei i, uint64_t start, uint8_t* data, size_t sz) {
	deviceLock.acquire();
	Inode* inode = readInode(i);
	if(!inode) {
		deviceLock.release();
		return false;
	}

	size_t end = start + sz;

	// First block
	Blocki rbi = start / BLOCK_SIZE;
	Blocki bi = inode->getBlock(rbi);
	if(!bi) {
		deviceLock.release();
		delete inode;
		return false;
	}

	uint8_t* block = readBlock(bi);
	if(!block) {
		deviceLock.release();
		return false;
	}
	size_t off = start % BLOCK_SIZE;
	size_t copy = std::min(sz, BLOCK_SIZE - off);
	memcpy(data, block+off, copy);
	sz -= copy; start += copy; data += copy;
	delete [] block;

	// Middle blocks
	rbi = start / BLOCK_SIZE;
	while(rbi < end / BLOCK_SIZE) {
		bi = inode->getBlock(rbi);
		if(!bi) {
			deviceLock.release();
			delete inode;
			return false;
		}

		block = readBlock(bi);
		if(!block) {
			deviceLock.release();
			delete inode;
			return false;
		}

		copy = std::min(sz, BLOCK_SIZE);
		memcpy(data, block, copy);
		sz -= copy; start += copy; data += copy;

		delete [] block;
		rbi = start / BLOCK_SIZE;
	}

	// Final block
	if(sz) {
		bi = inode->getBlock(rbi);
		if(!bi) {
			deviceLock.release();
			delete inode;
			return false;
		}

		block = readBlock(bi);
		if(!block) {
			deviceLock.release();
			delete inode;
			return false;
		}

		memcpy(data, block, sz);
		delete [] block;
	}

	deviceLock.release();
	delete inode;
	return true;
}
