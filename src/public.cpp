#include <common.hpp>
#include <shared_memory>
#include <rpc>
#include <mutex>
#include "fs/structures.hpp"
#include "fs/fs.hpp"
#include <registry>

static std::mutex setupLock;
static bool isSetup = false;
bool setup(std::PID client, uint64_t uuida, uint64_t uuidb, bool mustFormat) {
	if(!std::registry::has(client, "STRIFEFS_SETUP"))
		return false;

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

bool pubGetInode(std::PID client, std::SMID smid, Inodei i) {
	if(!std::registry::has(client, "STRIFEFS_READ"))
		return false;

	setupLock.acquire();
	if(!isSetup) {
		setupLock.release();
		return false;
	}
	setupLock.release();

	auto link = std::sm::link(client, smid);
	size_t npages = link.s;
	if(!npages)
		return false;
	uint8_t* buffer = link.f;

	deviceLock.acquire();
	Inode* inode = readInode(i);
	if(!inode) {
		deviceLock.release();
		return false;
	}
	deviceLock.release();

	memcpy(buffer, inode, sizeof(Inode));
	delete inode;

	std::sm::unlink(smid);
	return true;
}

bool pubRead(std::PID client, std::SMID smid, Inodei i, uint64_t start, size_t size) {
	if(!std::registry::has(client, "STRIFEFS_READ"))
		return false;

	setupLock.acquire();
	if(!isSetup) {
		setupLock.release();
		return false;
	}
	setupLock.release();

	auto link = std::sm::link(client, smid);
	size_t npages = link.s;
	if(!npages)
		return false;
	uint8_t* buffer = link.f;

	bool ret = read(i, start, buffer, size);
	std::sm::unlink(smid);
	return ret;
}

bool pubWrite(std::PID client, std::SMID smid, Inodei i, uint64_t start, size_t size) {
	if(!std::registry::has(client, "STRIFEFS_WRITE"))
	   return false;
	setupLock.acquire();
	if(!isSetup) {
		setupLock.release();
		return false;
	}
	setupLock.release();

	auto link = std::sm::link(client, smid);
	size_t npages = link.s;
	if(!npages)
		return false;
	uint8_t* buffer = link.f;

	bool ret = write(i, start, buffer, size);
	std::sm::unlink(smid);
	return ret;
}

static Inodei makeStuff(std::PID client, std::SMID smid, Inodei parent, bool isDir) {
	setupLock.acquire();
	if(!isSetup) {
		setupLock.release();
		return 0;
	}
	setupLock.release();

	auto link = std::sm::link(client, smid);
	size_t npages = link.s;
	if(!npages)
		return false;
	uint8_t* buffer = link.f;
	buffer[PAGE_SIZE - 1] = 0;
	std::string name((char*)buffer);

	Inodei ret;
	if(isDir)
		ret = newDirectory(parent, name);
	else
		ret = newFile(parent, name);

	std::sm::unlink(smid);
	return ret;
}

Inodei pubMakeDir(std::PID client, std::SMID smid, Inodei parent) {
	if(!std::registry::has(client, "STRIFEFS_WRITE"))
		return 0;

	return makeStuff(client, smid, parent, true);
}

Inodei pubMakeFile(std::PID client, std::SMID smid, Inodei parent) {
	if(!std::registry::has(client, "STRIFEFS_WRITE"))
		return 0;

	return makeStuff(client, smid, parent, false);
}

bool pubAddACL(std::PID client, Inodei i, size_t uid, size_t rawentry) {
	if(!std::registry::has(client, "STRIFEFS_WRITE"))
		return 0;

	deviceLock.acquire();
	Inode* inode = readInode(i);
	if(!inode) {
		deviceLock.release();
		return false;
	}
	deviceLock.release();

	std::ACLEntry entry;
	entry.raw = rawentry;
	bool ret = inode->addACL(uid, entry);

	deviceLock.acquire();
	if(!writeInode(i, *inode)) {
		deviceLock.release();
		delete inode;
		return false;
	}
	deviceLock.release();

	return ret;
}



void publish() {
	std::exportProcedure((void*)setup, 3);
	std::exportProcedure((void*)pubGetInode, 2);
	std::exportProcedure((void*)pubRead, 4);
	std::exportProcedure((void*)pubWrite, 4);
	std::exportProcedure((void*)pubMakeDir, 2);
	std::exportProcedure((void*)pubMakeFile, 2);
	std::exportProcedure((void*)pubAddACL, 3);
	std::enableRPC();
	std::publish("StrifeFS");
	std::halt();
}
