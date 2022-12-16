#ifndef DRAM_SUBSYSTEM_H
#define DRAM_SUBSYSTEM_H

#include <iostream>
#include <set>
#include <map>
#include <vector>
#include <list>
#include "CXL_Config.h"
#include "OutputLog.h"
#include "lrfu_heap.h"
#include "CFLRU.h"

class lruTwoListClass {
private:
	list<uint64_t> active;
	list<uint64_t> inactive;
	uint64_t activeBound;
public:
	lruTwoListClass() { activeBound = 0; }

	void init(uint64_t numCL);
	void add(uint64_t lba);
	uint64_t getCandidate();
	uint64_t evictLBA();
	void updateWhenHit(uint64_t lba, bool& falsehit);
	void reset();
};

class lfuNode {
public:
	uint64_t addr;
	uint64_t count;

	lfuNode(uint64_t a) { addr = a; count = 0; }
};

class lfuHeap {
public:

	void add(uint64_t addr);
	uint64_t top();
	void pop();
	void update(uint64_t addr);

private:
	vector<lfuNode> h;
	map<uint64_t, uint64_t> m;
};


using namespace std;
namespace SSD_Components {

	class dram_subsystem {
	public:
		dram_subsystem() {};
		dram_subsystem(cxl_config confival);
		~dram_subsystem();

		void initDRAM();
		bool isCacheHit(uint64_t lba);


		void process_cache_hit(bool rw, uint64_t lba, bool& falsehit);
		//void process_miss_data_ready(bool rw, uint64_t lba, list<uint64_t>* flush_lba, uint64_t simtime, set<uint64_t>* prefetched_lba);
		void process_miss_data_ready_new(bool rw, uint64_t lba, list<uint64_t>* flush_lba, uint64_t simtime, set<uint64_t>* prefetched_lba, set<uint64_t>& taggedAddr, set<uint64_t>& prefetch_pollution_tracker, set<uint64_t> not_finished);

		bool is_next_evict_candidate(uint64_t lba);

		uint64_t get_cache_index(uint64_t lba);

		uint64_t eviction_count{ 0 }, flush_count{0};

	private:
		cxl_config cpara;
		map<uint64_t, uint64_t>* dram_mapping{ NULL }; // LBA, cache line index


		list<uint64_t>* freeCL{ NULL }; // aligned by ssd block size

		vector<list<uint64_t>*>* all_freeCL{ NULL };
		
		set<uint64_t>* cachedlba{ NULL };//for random 
		//vector<set<uint64_t>*>* all_cachedlba{ NULL };
		vector<vector<uint64_t>*>* all_cachedlba{ NULL };

		vector<list<uint64_t>*>* all_fifocachedlba{ NULL };
		vector<CFLRU*>* all_cflrucachedlba{ NULL };
		vector<lfuHeap*>* all_lfucachedlba{NULL};

		lrfuHeap* lrfucachedlba{NULL}; //for lrfu
		vector<lrfuHeap*>* all_lrfucachedlba{ NULL };

		lruTwoListClass* lru2cachedlba{NULL};
		vector<lruTwoListClass*>* all_lru2cachedlba{ NULL };

		map < uint64_t, uint64_t>* dirtyCL{ NULL }; //cache line index, write count
		vector<map < uint64_t, uint64_t>*>* all_dirtyCL{ NULL };

		//For mix seoeration mode

		map<uint64_t, uint64_t>* pref_dram_mapping{ NULL };
		list<uint64_t>* pref_freeCL{ NULL };

		set<uint64_t>* pref_cachedlba{ NULL };//for random 
		lrfuHeap* pref_lrfucachedlba{ NULL }; //for lrfu
		lruTwoListClass* pref_lru2cachedlba{ NULL };


		map<uint64_t, uint64_t>* pref_dirtyCL{ NULL };
		
		uint64_t* next_eviction_candidate{ NULL };
		uint64_t* pref_next_eviction_candidate{ NULL };

	};


}


#endif