#include "../fs.hpp"

Inodei newFile(Inodei parent, std::string& name) {
	Inodei ret = allocInode();
	if(!ret)
		return 0;

	deviceLock.acquire();
	Inode inode;

	// ctime, mtime, and atime would be here
	inode.links = 1; // Parent

	if(!writeInode(ret, inode)) {
		deviceLock.release();
		return 0;
	}

	deviceLock.release();

	// Add reference from parent
	if(parent)
		if(!add(parent, ret, name))
			return 0;

	return ret;
}

Inodei newDirectory(Inodei parent, std::string& name) {
	Inodei ret = newFile(parent, name);
	if(!ret)
		return 0;

	deviceLock.acquire();
	Inode* inode = readInode(ret);
	if(!inode) {
		deviceLock.release();
		return 0;
	}

	inode->type = Inode::Types::DIRECTORY;

	if(!writeInode(ret, *inode)) {
		deviceLock.release();
		delete inode;
		return 0;
	}

	delete inode;
	deviceLock.release();

	std::string dot = ".";
	std::string dotdot = "..";

	// Add '.' entry
	if(!add(ret, ret, dot))
		return 0;
	// Add '..' entry
	if(!add(ret, parent ? parent : ret, dotdot))
		return 0;

	return ret;
}
