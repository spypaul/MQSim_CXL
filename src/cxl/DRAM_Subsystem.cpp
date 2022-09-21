#include "DRAM_Subsystem.h"

void lruTwoListClass::init(uint64_t numCL) {
	activeBound = numCL / 3;
}

void lruTwoListClass::add(uint64_t lba) {
	inactive.push_back(lba);
}


uint64_t lruTwoListClass::evictLBA() {
	uint64_t target{ inactive.front() };
	inactive.pop_front();
	return target;
}

void lruTwoListClass::updateWhenHit(uint64_t lba, bool& falsehit) {

	if (!active.empty()) {
		auto aiter{ --active.end() };


		while (aiter != active.begin()) {

			if (*aiter == lba) {
				active.erase(aiter);
				active.push_back(lba);
				return;
			}
			aiter--;
		}

		if (*aiter == lba) {
			active.erase(aiter);
			active.push_back(lba);
			return;
		}
	}

	auto initer{ --inactive.end() };

	while (initer != inactive.begin()) {
		if (*initer == lba) break;
		initer--;
	}

	if (initer == inactive.begin() && *initer != lba) {
		//cout << "Wrong" << endl;
		falsehit = 1;
		return;
	}

	inactive.erase(initer);

	if (active.size() == activeBound) {
		uint64_t movetarget{ active.front() };
		active.pop_front();
		inactive.push_back(movetarget);
	}
	active.push_back(lba);

}

void lruTwoListClass::reset() {
	active.clear();
	inactive.clear();
	activeBound = 0;
}


namespace SSD_Components {
	dram_subsystem::dram_subsystem(cxl_config confival) {
		cpara = confival;
	}

	dram_subsystem::~dram_subsystem() {
		freeCL->clear();
		delete freeCL;
		//pref_dirtyCL->clear();
		//delete pref_dirtyCL;

		if (dirtyCL) {
			delete dirtyCL;
		}
		//if (pref_dirtyCL) {
		//	delete pref_dirtyCL;
		//}
		if (cachedlba) {
			delete cachedlba;
		}
		if (lrfucachedlba) {
			delete lrfucachedlba;
		}

		if (lru2cachedlba) {
			delete lru2cachedlba;
		}

		//if (pref_lru2cachedlba) {
		//	delete pref_lru2cachedlba;
		//}

		if (dram_mapping) {
			dram_mapping->clear();
			delete dram_mapping;
		}

		//if (pref_dram_mapping) {
		//	dram_mapping->clear();
		//	delete dram_mapping;
		//}
	}

	void dram_subsystem::initDRAM() {
		dram_mapping = new map<uint64_t, uint64_t>;
		freeCL = new list<uint64_t>;
		dirtyCL = new map<uint64_t, uint64_t>;

		//pref_dram_mapping = new map<uint64_t, uint64_t>;
		//pref_freeCL = new list<uint64_t>;
		//pref_dirtyCL = new map<uint64_t, uint64_t>;

		for (uint64_t i = 0; i < cpara.cache_portion_size / cpara.ssd_page_size; i++) {
			freeCL->push_back(i);
		}

		//for (uint64_t i = cpara.cache_portion_size / cpara.ssd_page_size; i < cpara.dram_size / cpara.ssd_page_size; i++) {
		//	pref_freeCL->push_back(i);
		//}

		if (cpara.cpolicy == cachepolicy::random) {
			cachedlba = new vector<uint64_t>;
		}
		else if (cpara.cpolicy == cachepolicy::lrfu) {
			lrfucachedlba = new lrfuHeap;
			lrfucachedlba->init(cpara.lrfu_p, cpara.lrfu_lambda);
		}
		else if (cpara.cpolicy == cachepolicy::lru2) {
			lru2cachedlba = new lruTwoListClass;
			lru2cachedlba->init(freeCL->size());

			//pref_lru2cachedlba = new lruTwoListClass;
			//pref_lru2cachedlba->init(pref_freeCL->size());
		}
	}


	bool dram_subsystem::isCacheHit(uint64_t lba) {
		return dram_mapping->count(lba);
	}

	//bool dram_subsystem::isPrefetchHit(uint64_t lba) {
	//	return pref_dram_mapping->count(lba);
	//}

	void dram_subsystem::process_cache_hit(bool rw, uint64_t lba, bool& falsehit) {


		if (!dram_mapping->count(lba)) {
			falsehit = 1;
			return;
		}


		uint64_t cache_index{ (*dram_mapping)[lba] };



		//if (!dram_mapping->count(lba) && !pref_dram_mapping->count(lba)) {
		//	falsehit = 1;
		//	return;
		//}


		//uint64_t cache_index{ 0 };

		//if (pref_dram_mapping->count(lba)) {
		//	cache_index = (*pref_dram_mapping)[lba];
		//}
		//else {
		//	cache_index = (*dram_mapping)[lba];
		//}

		

		if (cache_index < cpara.cache_portion_size / cpara.ssd_page_size) { // check if the cache hit is at cache portion or prefetch portion
			if (cpara.cpolicy == cachepolicy::lru2) {
				lru2cachedlba->updateWhenHit(lba, falsehit);
			}
			else if (cpara.cpolicy == cachepolicy::lrfu) {
				lrfucachedlba->updateWhenHit(lba, falsehit);
				lrfucachedlba->advanceTime();
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
			//if (cpara.cpolicy == cachepolicy::lru2) {
			//	pref_lru2cachedlba->updateWhenHit(lba, falsehit);
			//}
			//else if (cpara.cpolicy == cachepolicy::lrfu) {

			//}
			//else if (cpara.cpolicy == cachepolicy::cpu) {

			//}

			//if (!rw && cpara.cpolicy != cachepolicy::cpu) {
			//	if (pref_dirtyCL->count(cache_index) > 0) {
			//		(*pref_dirtyCL)[cache_index] ++;
			//	}
			//	else {
			//		pref_dirtyCL->emplace(cache_index, 1);
			//	}
			//}

		}

	}

	void dram_subsystem::process_miss_data_ready(bool rw, uint64_t lba, list<uint64_t>* flush_lba, uint64_t simtime, set<uint64_t>* prefetched_lba) {


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
				evict_lba_base_addr = lru2cachedlba->evictLBA();

			}
			else if (cpara.cpolicy == cachepolicy::lrfu) {
				//TODO
				evict_lba_base_addr = lrfucachedlba->removeRoot();

			}

			uint64_t cl;
			cl = (*dram_mapping)[evict_lba_base_addr];
			dram_mapping->erase(dram_mapping->find(evict_lba_base_addr));
			outputf.of << "Finished_time " << simtime << " Starting_time " << 0 << " Eviction/Flush_at " << evict_lba_base_addr << std::endl;
			if (prefetched_lba->count(evict_lba_base_addr)) {
				prefetched_lba->erase(prefetched_lba->find(evict_lba_base_addr));
			}


			freeCL->push_back(cl);

			if (dirtyCL->count(cl) > 0) {
				flush_lba->push_back(evict_lba_base_addr);
				//evictf.of << evict_lba_base_addr << endl;
				dirtyCL->erase(cl);
			}

		}


		uint64_t cache_base_addr{ freeCL->front() };
		freeCL->pop_front();

		if (cpara.cpolicy == cachepolicy::random) {
			cachedlba->push_back(lba);//random
		}
		else if (cpara.cpolicy == cachepolicy::lru2) {
			lru2cachedlba->add(lba);
		}
		else if (cpara.cpolicy == cachepolicy::lrfu) {
			bnode* node{ new bnode{lrfucachedlba->F(0), lrfucachedlba->getTime(), lba} };
			lrfucachedlba->add(node);
			lrfucachedlba->advanceTime();

		}

		dram_mapping->emplace(lba, cache_base_addr);

		if (!rw) {
			dirtyCL->emplace(cache_base_addr, 1);
		}



	}
}




