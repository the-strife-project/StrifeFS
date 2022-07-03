#include "../fs.hpp"

bool add(Inodei from, Inodei to, std::string& name) {
	// Set up directory entry
	size_t sz = sizeof(Inodei) + sizeof(uint64_t) + name.size();
	uint8_t* aux = new uint8_t[sz];

	Inodei* inodei = (Inodei*)aux;
	*inodei = to;
	uint64_t* namesz = (uint64_t*)(aux + sizeof(Inodei));
	*namesz = name.size();
	memcpy(aux + sizeof(Inodei) + sizeof(uint64_t), name.c_str(), sz);

	// Append
	bool ret = append(from, aux, sz);
	delete [] aux;
	return ret;

	// TODO: Increment links
}
