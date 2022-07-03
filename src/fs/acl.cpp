#include "fs.hpp"
#include <cstdio>

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
	entry.allow = allow;
	entry.isUser = isUser;
	entry.read = perms & 0b001;
	entry.write = perms & 0b010;
	entry.execute = perms & 0b100;
	entry.id = id;

	return append(acl, (uint8_t*)&entry, sizeof(ACLEntry));
}

