#include"CXL_Config.h"






void cxl_config::readConfigFile() {

	ifstream configfile{ "config.txt" };


	while (!configfile.eof()) {
		string info;

		configfile >> info;

		if (info == "DRAM_size") {
			uint64_t value;
			configfile >> dec >> value;
			dram_size = value;
		}
		else if (info == "DRAM_mode") {
			uint64_t value;
			configfile >> dec >> value;
			dram_mode = (bool)value;
		}
		else if (info == "Has_cache") {
			uint64_t value;
			configfile >> dec >> value;
			has_cache = value;
		}
		else if (info == "Mix_mode") {
			uint64_t value;
			configfile >> dec >> value;
			mix_mode = static_cast<bool>(value);
		}
		else if (info == "Cache_portion_percentage") {
			uint64_t value;
			configfile >> dec >> value;
			cache_portion_size = dram_size * value / 100;
			prefetch_portion_size = dram_size - cache_portion_size;
		}
		else if (info == "Has_mshr") {
			uint64_t value;
			configfile >> dec >> value;
			has_mshr = static_cast<bool>(value);
		}
		else if (info == "SSD_page_size") {
			uint64_t value;
			configfile >> dec >> value;
			ssd_page_size = value;
		}
		else if (info == "Cache_policy") {
			string policy;
			configfile >> policy;
			if (policy == "Random") {
				cpolicy = cachepolicy::random;
			}
			else if (policy == "LRU2") {
				cpolicy = cachepolicy::lru2;
			}
			else if (policy == "FIFO") {
				cpolicy = cachepolicy::fifo;
			}
			else if (policy == "LFU") {
				cpolicy = cachepolicy::lfu;
			}
			else if (policy == "LRFU") {
				cpolicy = cachepolicy::lrfu;
			}
			else if (policy == "LRU") {
				cpolicy = cachepolicy::lru;
			}
			else if (policy == "CFLRU") {
				cpolicy = cachepolicy::cflru;
			}
			else if (policy == "CPU") {
				cpolicy = cachepolicy::cpu;
			}
		}
		else if (info == "Prefetch_cache_policy") {
			string policy;
			configfile >> policy;
			if (policy == "Random") {
				pref_cpolicy = cachepolicy::random;
			}
			else if (policy == "LRU2") {
				pref_cpolicy = cachepolicy::lru2;
			}
			else if (policy == "LRFU") {
				pref_cpolicy = cachepolicy::lrfu;
			}
			else if (policy == "CPU") {
				pref_cpolicy = cachepolicy::cpu;
			}
		}
		else if (info == "LRFU_p_lambda") {
			double p, lambda;
			configfile >> p >> lambda;
			lrfu_p = p;
			lrfu_lambda = lambda;
		}
		else if (info == "Cache_placement") {
			uint64_t sa;
			configfile >> sa;
			set_associativity = sa;
		}
		else if (info == "Prefetcher") {
			string ptype;
			configfile >> ptype;
			if (ptype == "No") {
				prefetch_policy = prefetchertype::no;
			}
			else if (ptype == "Tagged") {
				prefetch_policy = prefetchertype::tagged;
			}
			else if (ptype == "Best-offset") {
				prefetch_policy = prefetchertype::bo;
			}
			else if (ptype == "STMS") {
				prefetch_policy = prefetchertype::stms;
			}
			else if (ptype == "Leap") {
				prefetch_policy = prefetchertype::leap;
			}
			else if (ptype == "Readahead") {
				prefetch_policy = prefetchertype::readahead;

			}
			else if (ptype == "Feedback_direct") {
				prefetch_policy = prefetchertype::feedback_direct;

			}

		}
		else if (info == "Total_number_of_requests") {
			uint64_t value{ 0 };

			configfile >> value;

			total_number_of_requets = value;
		}

	}
	configfile.close();
}