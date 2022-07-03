#include <cstdio>
#include <common.hpp>

Info info;

extern "C" void _start() {
	

	publish();
	std::halt();
}
