#include "../fs.hpp"

bool list(Inodei i, std::unordered_map<std::string, Inodei>& result) {
	deviceLock.acquire();
	Inode* inode = readInode(i);
	deviceLock.release();
	if(!inode)
		return false;

	size_t sz = inode->size;
	uint8_t* aux = new uint8_t[sz];
	if(!read(i, 0, aux, sz)) {
		delete [] aux;
		return false;
	}

	delete inode;

	uint8_t* ptr = aux;
	while(sz) {
		Inodei ni = *(Inodei*)ptr;
		size_t namesz = *(size_t*)(ptr + sizeof(uint64_t));
		ptr += 2 * sizeof(Inodei);
		sz -= 2 * sizeof(size_t);

		char* aux = new char[namesz + 1];
		memcpy(aux, ptr, namesz);
		aux[namesz] = 0;
		ptr += namesz;
		sz -= namesz;

		result[aux] = ni;
		delete [] aux;
	}

	delete [] aux;
	return true;
}
