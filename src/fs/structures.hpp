#ifndef STRUCTURES_HPP
#define STRUCTURES_HPP

#include <pair>

typedef uint64_t LBA;
typedef uint64_t Timestamp;
typedef uint64_t Inodei;
typedef uint64_t Blocki;
typedef uint64_t UID;
typedef uint64_t GID;

const size_t BLOCK_SIZE = 512;

struct Superblock {
	char signature[8] = {'S', 't', 'r', 'i', 'f', 'e', 'F', 'S'};

	uint64_t inodeCount, blockCount;
	uint64_t freeInodes, freeBlocks;

	uint64_t inodeBitmap, blockBitmap;
	LBA firstInode, firstBlock;
} __attribute__((packed));

const size_t DIRECT_BLOCKS = 12;

struct Inode {
	// Metadata
	uint64_t size = 0;
	Timestamp ctime = 0, mtime = 0, atime = 0;
	uint64_t links = 0;
	uint64_t nblocks = 0;

	struct Types {
		enum {
			REGULAR,
			DIRECTORY,
			ACL,
			LINK,
		};
	};
	size_t type = Types::REGULAR;

	// ACL
	Blocki acl = 0; // Inherit parent

	// Data
	Blocki block0[DIRECT_BLOCKS] = {0};
	Blocki iblock[3] = {0};

	// Methods
	Blocki getBlock(size_t seq, bool shouldAlloc=false);
	bool setACL(bool allow, bool isUser, size_t id, size_t perms);
} __attribute__((packed));

struct ACLEntry {
	uint64_t allow : 1;
	uint64_t isUser : 1;
	uint64_t read : 1;
	uint64_t write : 1;
	uint64_t execute : 1;
	uint64_t reserved : 59;

	uint64_t id;
} __attribute__((packed));

#endif
