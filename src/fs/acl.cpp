#include "fs.hpp"

bool Inode::addACL(size_t id, std::ACLEntry ent) {
	if(!acl) {
		acl = allocInode();
		if(!acl)
			return false;

		deviceLock.acquire();
		Inode* inode = readInode(acl);
		if(!inode) {
			deviceLock.release();
			return false;
		}

		inode->type = Types::ACL;
		if(!writeInode(acl, *inode)) {
			deviceLock.release();
			delete inode;
			return false;
		}

		deviceLock.release();
		delete inode;
	}

	ACLEntry entry;
	entry.id = id;
	entry.e = ent;

	return append(acl, (uint8_t*)&entry, sizeof(ACLEntry));
}

