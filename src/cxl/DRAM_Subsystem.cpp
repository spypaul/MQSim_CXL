#include "DRAM_Subsystem.h"


dram_subsystem::dram_subsystem(cxl_config confival) {
	cpara = confival;
}

dram_subsystem::~dram_subsystem() {
	freeCL->clear();
	delete freeCL;
	if (dirtyCL) {
		delete dirtyCL;
	}
	if (cachedlba) {
		delete cachedlba;
	}

}

void dram_subsystem::initDRAM() {
	dram_state = new map<uint64_t, uint64_t>;
	freeCL = new vector<uint64_t>;
	dirtyCL = new map<uint64_t, uint64_t>;
	for (uint64_t i = 0; i < cpara.cache_portion_size / cpara.ssd_page_size; i ++ ) {
		freeCL->push_back(i);
	}

	if (cpara.cpolicy == cachepolicy::random) {
		cachedlba = new vector<uint64_t>;
	}
}


bool dram_subsystem::isCacheHit(uint64_t lba) {
	return dram_state->count(lba);
}

void dram_subsystem::process_cache_hit(bool rw, uint64_t lba) {

	uint64_t cache_index{ ( * dram_state)[lba]};

	if (cache_index < cpara.cache_portion_size / cpara.ssd_page_size) { // check if the cache hit is at cache portion or prefetch portion
		if (cpara.cpolicy == cachepolicy::lru2) {
			
		}
		else if (cpara.cpolicy == cachepolicy::lrfu) {
		
		}
		else if (cpara.cpolicy == cachepolicy::cpu) {
			
		}

		if (!rw && cpara.cpolicy != cachepolicy::cpu) {
			if (dirtyCL->count(cache_index) > 0) {
				(*dirtyCL)[lba] ++;
			}
			else {
				dirtyCL->emplace(cache_index, 1);
			}
		}
	}
	else {

	}

	//model latency
	if (rw) {
	}
	else {
	}

	

}