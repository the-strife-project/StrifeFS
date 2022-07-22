#include "fs.hpp"

bool Inode::setACL(bool allow, bool isUser, size_t id, size_t perms) {
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
	entry.e.allow = allow;
	entry.e.isUser = isUser;
	entry.e.read = (perms & 0b01) != 0;
	entry.e.write = (perms & 0b10) != 0;
	entry.id = id;

	return append(acl, (uint8_t*)&entry, sizeof(ACLEntry));
}

