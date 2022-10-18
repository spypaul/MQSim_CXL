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

uint64_t lruTwoListClass::getCandidate() {
	return inactive.front();
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
		next_eviction_candidate = new uint64_t{ 0 };
		pref_next_eviction_candidate = new uint64_t{ 0 };
	}

	dram_subsystem::~dram_subsystem() {

		if (dram_mapping) {
			dram_mapping->clear();
			delete dram_mapping;
		}


		for (auto i : *all_freeCL) {
			i->clear();
			delete i;
		}
		delete all_freeCL;

		for (auto i : *all_dirtyCL) {
			i->clear();
			delete i;
		}
		delete all_dirtyCL;


		if (all_cachedlba) {
			for (auto i : *all_cachedlba) {
				i->clear();
				delete i;
			}
			delete all_cachedlba;
		}

		if (all_lrfucachedlba) {

			for (auto i : *all_lrfucachedlba) {
				delete i;
			}
			delete all_lrfucachedlba;

		}

		if (all_lru2cachedlba) {

			for (auto i : *all_lru2cachedlba) {
				delete i;
			}
			delete all_lru2cachedlba;

		}



		//if (freeCL) {
		//	freeCL->clear();
		//	delete freeCL;
		//}

		//if (dirtyCL) {
		//	dirtyCL->clear();
		//	delete dirtyCL;
		//}

		//if (pref_freeCL) {
		//	pref_freeCL->clear();
		//	delete pref_freeCL;
		//}
		//if (pref_dram_mapping) {
		//	pref_dram_mapping->clear();
		//	delete pref_dram_mapping;
		//}
		//if (pref_dirtyCL) {
		//	pref_dirtyCL->clear();
		//	delete pref_dirtyCL;
		//}


		//if (cachedlba) {
		//	cachedlba->clear();
		//	delete cachedlba;
		//}
		//if (lrfucachedlba) {
		//	delete lrfucachedlba;
		//}
		//if (lru2cachedlba) {
		//	delete lru2cachedlba;
		//}

		//if (pref_cachedlba) {
		//	pref_cachedlba->clear();
		//	delete pref_cachedlba;
		//}
		//if (pref_lrfucachedlba) {
		//	delete pref_lrfucachedlba;
		//}
		//if (pref_lru2cachedlba) {
		//	delete pref_lru2cachedlba;
		//}

		//delete next_eviction_candidate;
		//delete pref_next_eviction_candidate;


	}

	void dram_subsystem::initDRAM() {
		dram_mapping = new map<uint64_t, uint64_t>;

		uint64_t cache_page_number{ 0 };
		all_freeCL = new vector<list<uint64_t>*>;
		all_dirtyCL = new vector < map < uint64_t, uint64_t>*>;

		if (cpara.cpolicy == cachepolicy::random) {
			all_cachedlba = new vector<set<uint64_t>*>;
		}
		else if (cpara.cpolicy == cachepolicy::lrfu) {
			all_lrfucachedlba = new vector<lrfuHeap*>;
		}
		else if (cpara.cpolicy == cachepolicy::lru2) {
			all_lru2cachedlba = new vector<lruTwoListClass*>;
		}

		for (uint64_t i = 0; i < cpara.cache_portion_size / cpara.ssd_page_size / cpara.set_associativity; i++) {
			list<uint64_t>* l{ new list<uint64_t> };
			for (auto j = 0; j < cpara.set_associativity; j++) {
				l->push_back(cache_page_number);
				cache_page_number++;
			}
			all_freeCL->push_back(l);
			
			map < uint64_t, uint64_t>* m1{ new map < uint64_t, uint64_t> };
			all_dirtyCL->push_back(m1);

			if (cpara.cpolicy == cachepolicy::random) {
				set<uint64_t>* s{ new set<uint64_t> };
				all_cachedlba->push_back(s);
			}
			else if (cpara.cpolicy == cachepolicy::lrfu) {
				lrfuHeap* lh{ new lrfuHeap };
				lh->init(cpara.lrfu_p, cpara.lrfu_lambda);
				all_lrfucachedlba->push_back(lh);
			}
			else if (cpara.cpolicy == cachepolicy::lru2) {
				lruTwoListClass* lt{ new lruTwoListClass };
				lt->init(cpara.set_associativity);
				all_lru2cachedlba->push_back(lt);

			}

		}
		


		/*freeCL = new list<uint64_t>;
		dirtyCL = new map<uint64_t, uint64_t>;
		
		if (cpara.prefetch_policy != prefetchertype::no && !cpara.mix_mode) {
			pref_dram_mapping = new map<uint64_t, uint64_t>;
			pref_freeCL = new list<uint64_t>;
			pref_dirtyCL = new map<uint64_t, uint64_t>;
		}


		for (uint64_t i = 0; i < cpara.cache_portion_size / cpara.ssd_page_size; i++) {
			freeCL->push_back(i);
		}

		if (cpara.prefetch_policy != prefetchertype::no && !cpara.mix_mode) {
			for (uint64_t i = cpara.cache_portion_size / cpara.ssd_page_size; i < cpara.dram_size / cpara.ssd_page_size; i++) {
				pref_freeCL->push_back(i);
			}
		}


		if (cpara.cpolicy == cachepolicy::random) {
			cachedlba = new set<uint64_t>;
		}
		else if (cpara.cpolicy == cachepolicy::lrfu) {
			lrfucachedlba = new lrfuHeap;
			lrfucachedlba->init(cpara.lrfu_p, cpara.lrfu_lambda);
		}
		else if (cpara.cpolicy == cachepolicy::lru2) {
			lru2cachedlba = new lruTwoListClass;
			lru2cachedlba->init(freeCL->size());
		}


		if (cpara.prefetch_policy != prefetchertype::no && !cpara.mix_mode) {

			if (cpara.pref_cpolicy == cachepolicy::random) {
				pref_cachedlba = new set<uint64_t>;
			}
			else if (cpara.pref_cpolicy == cachepolicy::lrfu) {
				pref_lrfucachedlba = new lrfuHeap;
				pref_lrfucachedlba->init(cpara.lrfu_p, cpara.lrfu_lambda);
			}
			else if (cpara.pref_cpolicy == cachepolicy::lru2) {
				pref_lru2cachedlba = new lruTwoListClass;
				pref_lru2cachedlba->init(freeCL->size());
			}

		}*/
		

	}

	bool dram_subsystem::is_next_evict_candidate(uint64_t lba) {
		

		if (cpara.mix_mode) {
			if (!freeCL->empty()) return 0;
			return lba == *next_eviction_candidate;
		}
		else {

			if (lba == *next_eviction_candidate) {
				return freeCL->empty();
			}
			else if (lba == *pref_next_eviction_candidate) {
				return pref_freeCL->empty();
			}
			return 0;
		}
	}



	bool dram_subsystem::isCacheHit(uint64_t lba) {

		if (!cpara.mix_mode) {
			return dram_mapping->count(lba) || pref_dram_mapping->count(lba);
		}
		else {
			return dram_mapping->count(lba);
		}
		
	}


	void dram_subsystem::process_cache_hit(bool rw, uint64_t lba, bool& falsehit) {


		if (!this->isCacheHit(lba)) {
			falsehit = 1;
			return;
		}


		uint64_t cache_page_addr{ 0 };

		if (!cpara.mix_mode) {
			if (pref_dram_mapping->count(lba)) {
				cache_page_addr = (*pref_dram_mapping)[lba];
			}
			else {
				cache_page_addr = (*dram_mapping)[lba];
			}

		}
		else {
			cache_page_addr = (*dram_mapping)[lba];
		}



		uint64_t cache_index{ get_cache_index(lba) };

		if (cache_page_addr < cpara.cache_portion_size / cpara.ssd_page_size) { // check if the cache hit is at cache portion or prefetch portion
			if (cpara.cpolicy == cachepolicy::lru2) {
				
				(*all_lru2cachedlba)[cache_index]->updateWhenHit(lba, falsehit);
				//lru2cachedlba->updateWhenHit(lba, falsehit);


				//*next_eviction_candidate = lru2cachedlba->getCandidate();
			}
			else if (cpara.cpolicy == cachepolicy::lrfu) {

				(*all_lrfucachedlba)[cache_index]->updateWhenHit(lba, falsehit);
				(*all_lrfucachedlba)[cache_index]->advanceTime();

				//lrfucachedlba->updateWhenHit(lba, falsehit);
				//lrfucachedlba->advanceTime();



				/*uint64_t oec{ *next_eviction_candidate };
				*next_eviction_candidate = lrfucachedlba->getCandidate();
				if (*next_eviction_candidate != oec) {
					cout << oec << " " << *next_eviction_candidate << endl;
				}*/
			}
			else if (cpara.cpolicy == cachepolicy::cpu) {

			}

			if (!rw && cpara.cpolicy != cachepolicy::cpu) {

				if ((*all_dirtyCL)[cache_index]->count(cache_page_addr) > 0) {
					(*(*all_dirtyCL)[cache_index])[cache_page_addr] ++;
				}
				else {
					(*all_dirtyCL)[cache_index]->emplace(cache_page_addr, 1);
				}


				//if (dirtyCL->count(cache_page_addr) > 0) {
				//	(*dirtyCL)[cache_page_addr] ++;
				//}
				//else {
				//	dirtyCL->emplace(cache_page_addr, 1);
				//}
			}
		}
		else {
			if (cpara.pref_cpolicy == cachepolicy::lru2) {
				pref_lru2cachedlba->updateWhenHit(lba, falsehit);
				//*next_eviction_candidate = pref_lru2cachedlba->getCandidate();
			}
			else if (cpara.pref_cpolicy == cachepolicy::lrfu) {
				pref_lrfucachedlba->updateWhenHit(lba, falsehit);
				pref_lrfucachedlba->advanceTime();
				//*next_eviction_candidate = pref_lrfucachedlba->getCandidate();
			}
			else if (cpara.pref_cpolicy == cachepolicy::cpu) {

			}

			if (!rw && cpara.pref_cpolicy != cachepolicy::cpu) {
				if (pref_dirtyCL->count(cache_page_addr) > 0) {
					(*pref_dirtyCL)[cache_page_addr] ++;
				}
				else {
					pref_dirtyCL->emplace(cache_page_addr, 1);
				}
			}

		}

	}

	//void dram_subsystem::process_miss_data_ready(bool rw, uint64_t lba, list<uint64_t>* flush_lba, uint64_t simtime, set<uint64_t>* prefetched_lba) {


	//	if (freeCL->empty()) {


	//		uint64_t evict_lba_base_addr{ 0 };
	//		if (cpara.cpolicy == cachepolicy::random) {
	//			//eviction policy random
	//			srand(static_cast<unsigned int>(lba));
	//			uint64_t evict_position{ rand() % cachedlba->size() };
	//			evict_lba_base_addr = (*cachedlba)[evict_position];
	//			cachedlba->erase(cachedlba->begin() + evict_position);
	//		}
	//		else if (cpara.cpolicy == cachepolicy::lru2) {
	//			//TODO
	//			evict_lba_base_addr = lru2cachedlba->evictLBA();

	//		}
	//		else if (cpara.cpolicy == cachepolicy::lrfu) {
	//			//TODO
	//			evict_lba_base_addr = lrfucachedlba->removeRoot();

	//		}

	//		uint64_t cl;
	//		cl = (*dram_mapping)[evict_lba_base_addr];
	//		dram_mapping->erase(dram_mapping->find(evict_lba_base_addr));
	//		outputf.of << "Finished_time " << simtime << " Starting_time " << 0 << " Eviction/Flush_at " << evict_lba_base_addr << std::endl;
	//		if (prefetched_lba->count(evict_lba_base_addr)) {
	//			prefetched_lba->erase(prefetched_lba->find(evict_lba_base_addr));
	//		}


	//		freeCL->push_back(cl);

	//		if (dirtyCL->count(cl) > 0) {
	//			flush_lba->push_back(evict_lba_base_addr);
	//			//evictf.of << evict_lba_base_addr << endl;
	//			dirtyCL->erase(cl);
	//		}

	//	}


	//	uint64_t cache_base_addr{ freeCL->front() };
	//	freeCL->pop_front();

	//	if (cpara.cpolicy == cachepolicy::random) {
	//		cachedlba->push_back(lba);//random
	//	}
	//	else if (cpara.cpolicy == cachepolicy::lru2) {
	//		lru2cachedlba->add(lba);
	//	}
	//	else if (cpara.cpolicy == cachepolicy::lrfu) {
	//		bnode* node{ new bnode{lrfucachedlba->F(0), lrfucachedlba->getTime(), lba} };
	//		lrfucachedlba->add(node);
	//		lrfucachedlba->advanceTime();

	//	}

	//	dram_mapping->emplace(lba, cache_base_addr);

	//	if (!rw) {
	//		dirtyCL->emplace(cache_base_addr, 1);
	//	}



	//}



	void dram_subsystem::process_miss_data_ready_new(bool rw, uint64_t lba, list<uint64_t>* flush_lba, uint64_t simtime, set<uint64_t>* prefetched_lba, set<uint64_t> not_finished) {


		list<uint64_t>* temp_freeCL{NULL};
		map<uint64_t, uint64_t>* temp_dram_mapping{ NULL };
		map<uint64_t, uint64_t>* temp_dirtyCL{ NULL };
		set<uint64_t>* temp_cachedlba{ NULL };
		lruTwoListClass* temp_lru2cachedlba{ NULL };
		lrfuHeap* temp_lrfucachedlba{NULL};
		cachepolicy cp{ cachepolicy::random };
		uint64_t* next_cand{ NULL };

		uint64_t cache_index{ get_cache_index(lba) };

		if (cpara.mix_mode) {

			temp_dram_mapping = dram_mapping;
			cp = cpara.cpolicy;
			next_cand = next_eviction_candidate;

			temp_freeCL = (* all_freeCL)[cache_index];
			temp_dirtyCL = (*all_dirtyCL)[cache_index];

			if (cpara.cpolicy == cachepolicy::random) {
				temp_cachedlba = (*all_cachedlba)[cache_index];
			}
			else if (cpara.cpolicy == cachepolicy::lrfu) {
				temp_lrfucachedlba = (*all_lrfucachedlba)[cache_index];
			}
			else if (cpara.cpolicy == cachepolicy::lru2) {
				temp_lru2cachedlba = (*all_lru2cachedlba)[cache_index];
			}



			//temp_freeCL = freeCL;
			//temp_dirtyCL = dirtyCL;
			//temp_cachedlba = cachedlba;
			//temp_lru2cachedlba = lru2cachedlba;
			//temp_lrfucachedlba = lrfucachedlba;

		}
		else {
			if (prefetched_lba->count(lba)) {
				temp_freeCL = pref_freeCL;
				temp_dram_mapping = pref_dram_mapping;
				temp_dirtyCL = pref_dirtyCL;
				cp = cpara.pref_cpolicy;
				temp_cachedlba = pref_cachedlba;
				temp_lru2cachedlba = pref_lru2cachedlba;
				temp_lrfucachedlba = pref_lrfucachedlba;
				next_cand = pref_next_eviction_candidate;
			}
			else {
				temp_freeCL = freeCL;
				temp_dram_mapping = dram_mapping;
				temp_dirtyCL = dirtyCL;
				cp = cpara.cpolicy;
				temp_cachedlba = cachedlba;
				temp_lru2cachedlba = lru2cachedlba;
				temp_lrfucachedlba = lrfucachedlba;
				next_cand = next_eviction_candidate;
			}
		}

		if (temp_dram_mapping->count(lba) > 0) {
			bool falsehit{ 0 };
			process_cache_hit(rw, lba, falsehit);
			return;
		}



		if (temp_freeCL->empty()) {


			uint64_t evict_lba_base_addr{ 0 };
			if (cp == cachepolicy::random) {
				//eviction policy random
				srand(static_cast<unsigned int>(lba));
				uint64_t evict_position{ rand() % temp_cachedlba->size() };
				auto it{ temp_cachedlba->begin() };
				for (auto i = 0; i < evict_position; i++)it++;
				evict_lba_base_addr = *it;
				temp_cachedlba->erase(temp_cachedlba->find(*it));

				//evict_lba_base_addr = *next_cand;
				//temp_cachedlba->erase(temp_cachedlba->find(*next_cand));

			}
			else if (cp == cachepolicy::lru2) {
				//TODO
				evict_lba_base_addr = temp_lru2cachedlba->evictLBA();

				//evict_lba_base_addr = *next_cand;

			}
			else if (cp == cachepolicy::lrfu) {
				//TODO
				evict_lba_base_addr = temp_lrfucachedlba->removeRoot();

				//evict_lba_base_addr = *next_cand;
			}

			if (not_finished.count(evict_lba_base_addr) > 0) {
				cout << "Check" << endl;
			}
			
			uint64_t cl;
			if (temp_dram_mapping->count(evict_lba_base_addr) == 0) {
				cout << "Check" << endl;
			}
			cl = (temp_dram_mapping->find(evict_lba_base_addr))->second;
			temp_dram_mapping->erase(temp_dram_mapping->find(evict_lba_base_addr));
			outputf.of << "Finished_time " << simtime << " Starting_time " << 0 << " Eviction/Flush_at " << evict_lba_base_addr << std::endl;
			if (prefetched_lba->count(evict_lba_base_addr)) {
				prefetched_lba->erase(prefetched_lba->find(evict_lba_base_addr));
			}


			temp_freeCL->push_back(cl);

			if (temp_dirtyCL->count(cl) > 0) {
				flush_lba->push_back(evict_lba_base_addr);
				//evictf.of << evict_lba_base_addr << endl;
				temp_dirtyCL->erase(cl);
			}

		}


		uint64_t cache_base_addr{ temp_freeCL->front() };
		temp_freeCL->pop_front();

		if (cp == cachepolicy::random) {
			temp_cachedlba->emplace(lba);//random

			//if (temp_freeCL->empty()) {
			//	srand(static_cast<unsigned int>(lba));
			//	uint64_t evict_position{ rand() % temp_cachedlba->size() };
			//	auto it{ temp_cachedlba->begin() };
			//	for (auto i = 0; i < evict_position; i++)it++;
			//	*next_cand = *it;
			//}


		}
		else if (cp == cachepolicy::lru2) {
			//if (temp_freeCL->empty()) {
			//	*next_cand = temp_lru2cachedlba->evictLBA();
			//}
			

			temp_lru2cachedlba->add(lba);
			
			
		}
		else if (cp == cachepolicy::lrfu) {
			//if (temp_freeCL->empty()) {
			//	*next_cand = temp_lrfucachedlba->removeRoot();
			//}

			bnode* node{ new bnode{temp_lrfucachedlba->F(0), temp_lrfucachedlba->getTime(), lba} };
			temp_lrfucachedlba->add(node);
			temp_lrfucachedlba->advanceTime();
			
			

		}

		if (temp_dram_mapping->count(lba) > 0) {
			cout << "Check" << endl;
		}
		temp_dram_mapping->emplace(lba, cache_base_addr);

		if (!rw) {
			temp_dirtyCL->emplace(cache_base_addr, 1);
		}

	}


	uint64_t dram_subsystem::get_cache_index(uint64_t lba) {
		return lba % (cpara.cache_portion_size / cpara.ssd_page_size / cpara.set_associativity);
	}


}






