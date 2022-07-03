#ifndef FS_HPP
#define FS_HPP

#include <common.hpp>
#include <string>
#include <unordered_map>

#define DO_NOT_LOCK true

// Medium level
Inodei allocInode();
bool freeInode(Inodei);
Blocki allocBlock(bool dontLock=false);
bool freeBlock(Blocki);

Inode* readInode(Inodei);
bool writeInode(Inodei, const Inode&);
uint8_t* readBlock(Blocki);
bool writeBlock(Blocki, uint8_t*);

// High level
bool format();
bool read(Inodei, uint64_t start, uint8_t*, size_t sz);
bool write(Inodei, uint64_t start, uint8_t*, size_t sz, bool append=false);
inline bool append(Inodei i, uint8_t* data, size_t sz) {
	return write(i, 0, data, sz, true);
}

bool list(Inodei i, std::unordered_map<std::string, Inodei>& result);

bool add(Inodei from, Inodei to, std::string&);
Inodei newFile(Inodei parent, std::string&);
Inodei newDirectory(Inodei parent, std::string&);
inline bool makeRoot() {
	std::string empty = "";
	return newDirectory(0, empty);
}

#endif
