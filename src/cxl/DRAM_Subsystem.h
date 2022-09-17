#ifndef DRAM_SUBSYSTEM_H
#define DRAM_SUBSYSTEM_H

#include <iostream>
#include <set>
#include <map>
#include <vector>
#include <list>
#include "CXL_Config.h"
#include "OutputLog.h"

using namespace std;
namespace SSD_Components {

	class dram_subsystem {
	public:
		dram_subsystem() {};
		dram_subsystem(cxl_config confival);
		~dram_subsystem();

		void initDRAM();
		bool isCacheHit(uint64_t lba);

		void process_cache_hit(bool rw, uint64_t lba);
		void process_miss_data_ready(bool rw, uint64_t lba, list<uint64_t>* flush_lba);




	private:
		cxl_config cpara;
		map<uint64_t, uint64_t>* dram_mapping{ NULL }; // LBA, cache line index
		list<uint64_t>* freeCL{ NULL }; // aligned by ssd block size
		vector<uint64_t>* cachedlba{ NULL };
		map < uint64_t, uint64_t>* dirtyCL{ NULL }; //cache line index, write count

	};


}


#endif