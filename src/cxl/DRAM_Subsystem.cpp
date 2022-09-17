#include "DRAM_Subsystem.h"

namespace SSD_Components {
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
		dram_mapping = new map<uint64_t, uint64_t>;
		freeCL = new list<uint64_t>;
		dirtyCL = new map<uint64_t, uint64_t>;
		for (uint64_t i = 0; i < cpara.cache_portion_size / cpara.ssd_page_size; i++) {
			freeCL->push_back(i);
		}

		if (cpara.cpolicy == cachepolicy::random) {
			cachedlba = new vector<uint64_t>;
		}
	}


	bool dram_subsystem::isCacheHit(uint64_t lba) {
		return dram_mapping->count(lba);
	}

	void dram_subsystem::process_cache_hit(bool rw, uint64_t lba) {

		if (!dram_mapping->count(lba)) {
			outputf.of << "Hit at LBA: " << lba << "is a false hit" << endl;
			return;
		}


		uint64_t cache_index{ (*dram_mapping)[lba] };

		if (cache_index < cpara.cache_portion_size / cpara.ssd_page_size) { // check if the cache hit is at cache portion or prefetch portion
			if (cpara.cpolicy == cachepolicy::lru2) {

			}
			else if (cpara.cpolicy == cachepolicy::lrfu) {

			}
			else if (cpara.cpolicy == cachepolicy::cpu) {

			}

			if (!rw && cpara.cpolicy != cachepolicy::cpu) {
				if (dirtyCL->count(cache_index) > 0) {
					(*dirtyCL)[cache_index] ++;
				}
				else {
					dirtyCL->emplace(cache_index, 1);
				}
			}
		}
		else {

		}

	}

	void dram_subsystem::process_miss_data_ready(bool rw, uint64_t lba, list<uint64_t>* flush_lba) {


		if (freeCL->empty()) {


			uint64_t evict_lba_base_addr{ 0 };
			if (cpara.cpolicy == cachepolicy::random) {
				//eviction policy random
				srand(static_cast<unsigned int>(lba));
				uint64_t evict_position{ rand() % cachedlba->size() };
				evict_lba_base_addr = (*cachedlba)[evict_position];
				cachedlba->erase(cachedlba->begin() + evict_position);
			}
			else if (cpara.cpolicy == cachepolicy::lru2) {
				//TODO

			}
			else if (cpara.cpolicy == cachepolicy::lrfu) {
				//TODO

			}

			uint64_t cl;
			cl = (*dram_mapping)[lba];


			freeCL->push_back(cl);

			if (dirtyCL->count(cl) > 0) {
				flush_lba->push_back(evict_lba_base_addr);
				dirtyCL->erase(cl);
			}

		}


		uint64_t cache_base_addr{ freeCL->front() };
		freeCL->pop_front();

		if (cpara.cpolicy == cachepolicy::random) {
			cachedlba->push_back(lba);//random
		}
		else if (cpara.cpolicy == cachepolicy::lru2) {

		}
		else if (cpara.cpolicy == cachepolicy::lrfu) {
		}

		dram_mapping->emplace(lba, cache_base_addr);

		if (!rw) {
			dirtyCL->emplace(cache_base_addr, 1);
		}



	}
}




