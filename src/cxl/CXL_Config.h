#pragma once
#include<iostream>
#include<fstream>

using namespace std;

typedef enum class cachepolicy {
	random,
	lru2,
	fifo,
	lfu,
	lru,
	cflru,
	lrfu,//not working
	cpu//no need of it
}cachepolicy;

typedef enum class prefetchertype {
	no,
	tagged,
	bo,
	stms,
	leap,
	readahead,
	feedback_direct
}prefetchertype;


class cxl_config {
public:
	uint64_t dram_size;
	bool mix_mode{1};
	uint64_t cache_portion_size;
	uint64_t prefetch_portion_size;
	uint64_t ssd_page_size;
	uint64_t logical_sector_size{ 512 };
	cachepolicy cpolicy;
	cachepolicy pref_cpolicy;
	double lrfu_p;
	double lrfu_lambda;
	uint64_t set_associativity;
	prefetchertype prefetch_policy;
	uint64_t total_number_of_requets;
	bool has_mshr;
	bool has_cache;
	bool dram_mode{ 0 };
	uint64_t num_sec{ 8 };

	cxl_config() {
		dram_size = 0; 
		cache_portion_size = 0;
		prefetch_portion_size = 0;
		ssd_page_size = 4096;
		cpolicy = cachepolicy::random;
		pref_cpolicy = cachepolicy::random;
		lrfu_p = 2;
		lrfu_lambda = 1;
		set_associativity = 4;
		prefetch_policy = prefetchertype::tagged;
		total_number_of_requets = 0;
		has_mshr = 1;
		has_cache = 1;
	};

	void readConfigFile();

};

