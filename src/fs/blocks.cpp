#include <common.hpp>
#include <list>
#include "fs.hpp"
#include <cstdio>

// The most difficult 100 lines ever

// Block pointers per indirect level
static inline size_t getBPI() { return BLOCK_SIZE / sizeof(Blocki); }

// How many blocks before a level?
static size_t howManyBeforeLevel(size_t level) {
	size_t bpi = getBPI();
	size_t ret = 0;

	switch(level) {
	case 3: ret += bpi*bpi*bpi; fallthrough;
	case 2: ret += bpi*bpi; fallthrough;
	case 1: ret += bpi; fallthrough;
	case 0: ret += DIRECT_BLOCKS;
	}

	return ret;
}

// How many blocks inside a level?
static size_t howManyInsideLevel(size_t level) {
	size_t bpi = getBPI();

	switch(level) {
	case 3: return bpi*bpi; // Width of level 2
	case 2: return bpi; // Width of level 1
	}

	return 0;
}

// Get an index list for a sequential block
static std::list<size_t> _solveML(size_t relseq, size_t level) {
	std::list<size_t> ret;

	// Finished?
	if(level == 1) {
		ret.push_front(relseq);
		return ret;
	}

	// Which would it be in this level?
	size_t chunk = howManyInsideLevel(level);
	size_t idx = relseq / chunk;

	relseq -= idx * chunk;
	ret = _solveML(relseq, level-1);
	ret.push_front(idx);
	return ret;
}

// Given a sequential block, which level is it part of?
static size_t getLevel(size_t seq) {
	if(seq >= howManyBeforeLevel(2))
		return 3;
	else if(seq >= howManyBeforeLevel(1))
		return 2;
	else if(seq >= howManyBeforeLevel(0))
		return 1;
	else
		return 0;
}

// Wrapper around _solveML
static inline std::list<size_t> solveML(size_t seq) {
	auto level = getLevel(seq);
	return _solveML(seq-howManyBeforeLevel(level-1), level);
}

Blocki Inode::getBlock(size_t seq, bool shouldAlloc) {
	if(!deviceLock.isAcquired()) {
		std::printf("[StrifeFS] getBlock: not acquired!\n");
		std::exit(4);
	}

	auto level = getLevel(seq);
	if(!level) {
		if(shouldAlloc && !block0[seq]) {
			block0[seq] = allocBlock(DO_NOT_LOCK);
			if(!block0[seq])
				return 0;
			++nblocks;
		}

		return block0[seq];
	}

	bool didAlloc = false;
	Blocki cur = iblock[level-1];
	for(auto idx : solveML(seq)) {
		Blocki* blocks = (Blocki*)readBlock(cur);
		if(!blocks) {
			// Error reading
			return 0;
		}

		Blocki prevcur = cur;
		cur = blocks[idx];
		if(!cur) {
			if(!shouldAlloc) {
				delete [] blocks;
				return 0; // Not allocated
			}

			// Time to add it
			cur = allocBlock(DO_NOT_LOCK);
			if(!cur) {
				delete [] blocks;
				return 0;
			}
			blocks[idx] = cur;
			writeBlock(prevcur, (uint8_t*)blocks);
			didAlloc = true;
		}

		delete [] blocks;
	}

	if(didAlloc)
		++nblocks;

	return cur;
}
