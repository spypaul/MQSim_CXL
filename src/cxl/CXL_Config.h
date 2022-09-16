#pragma once
#include<iostream>
#include<fstream>

using namespace std;

typedef enum class cachepolicy {
	random,
	lru2,
	lrfu,
	cpu
}cachepolicy;

typedef enum class prefetchertype {
	tagged,
	bo,
	stms,
	leap,
	readahead
}prefetchertype;


class cxl_config {
public:
	uint64_t dram_size;
	uint64_t cache_portion_size;
	uint64_t prefetch_portion_size;
	uint64_t ssd_page_size;
	cachepolicy cpolicy;
	double lrfu_p;
	double lrfu_lambda;
	uint16_t set_associativity;
	prefetchertype prefetch_policy;
	uint64_t total_number_of_requets;

	cxl_config() {
		dram_size = 0; 
		cache_portion_size = 0;
		prefetch_portion_size = 0;
		ssd_page_size = 4096;
		cpolicy = cachepolicy::random;
		lrfu_p = 2;
		lrfu_lambda = 1;
		set_associativity = 4;
		prefetch_policy = prefetchertype::tagged;
		total_number_of_requets = 0;
	};

	void readConfigFile();

};

