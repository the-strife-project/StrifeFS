#include "../fs.hpp"
#include <cstdio>

bool write(Inodei i, uint64_t start, uint8_t* data, size_t sz, bool append) {
	deviceLock.acquire();
	Inode* inode = readInode(i);
	if(!inode) {
		deviceLock.release();
		return false;
	}

	if(append)
		start = inode->size;

	size_t end = start + sz;

	// First block
	Blocki rbi = start / BLOCK_SIZE;
	Blocki bi = inode->getBlock(rbi, true);
	if(!bi) {
		deviceLock.release();
		delete inode;
		return false;
	}

	uint8_t* block = readBlock(bi);
	if(!block) {
		deviceLock.release();
		delete inode;
		return false;
	}
	size_t off = start % BLOCK_SIZE;
	size_t copy = std::min(sz, BLOCK_SIZE - off);
	memcpy(block+off, data, copy);
	sz -= copy; start += copy; data += copy;
	if(!writeBlock(bi, block)) {
		deviceLock.release();
		delete [] block;
		delete inode;
		return false;
	}
	delete [] block;

	// Middle blocks
	rbi = start / BLOCK_SIZE;
	while(rbi < end / BLOCK_SIZE) {
		bi = inode->getBlock(rbi, true);
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
		memcpy(block, data, copy);
		sz -= copy; start += copy; data += copy;

		if(!writeBlock(bi, block)) {
			deviceLock.release();
			delete [] block;
			delete inode;
			return false;
		}

		delete [] block;
		rbi = start / BLOCK_SIZE;
	}

	// Final block
	if(sz) {
		bi = inode->getBlock(rbi, true);
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

		memcpy(block, data, sz);
		if(!writeBlock(bi, block)) {
			deviceLock.release();
			delete [] block;
			delete inode;
			return false;
		}

		delete [] block;
	}

	// Did it grow?
	if(end > inode->size)
		inode->size = end;

	// Write inode
	if(!writeInode(i, *inode)) {
		deviceLock.release();
		delete inode;
		return false;
	}

	deviceLock.release();
	delete inode;
	return true;
}
