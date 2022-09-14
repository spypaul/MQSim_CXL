#pragma once
#include <iostream>
#include <set>
#include <map>
#include <vector>
#include "Host_Interface_CXL.h"
#include "CXL_Config.h"


using namespace std;


class dram_subsystem {
public:

	dram_subsystem(cxl_config confival);
	~dram_subsystem();

	void initDRAM();
	bool isCacheHit(uint64_t lba);
	
	void process_cache_hit(bool rw, uint64_t lba);



private:
	cxl_config cpara;
	map<uint64_t, uint64_t>* dram_state{NULL}; // LBA, cache line index
	vector<uint64_t>* freeCL{NULL}; // aligned by ssd block size
	vector<uint64_t>* cachedlba{NULL};
	map < uint64_t, uint64_t>* dirtyCL{NULL}; //cache line index, write count

};