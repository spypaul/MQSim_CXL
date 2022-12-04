#pragma once

#include <iostream>
#include <list>
#include <map>
#include "../sim/Sim_Object.h"
#include "../sim/Sim_Reporter.h"
#include "../ssd/User_Request.h"
#include "../ssd/Data_Cache_Manager_Base.h"
#include "../ssd/Host_Interface_Base.h"
#include "OutputLog.h"


namespace SSD_Components {

	enum class CXL_DRAM_EVENTS {
		CACHE_HIT,
		CACHE_MISS,
		CACHE_HIT_UNDER_MISS,
		PREFETCH_READY,
		SLOW_PREFETCH
	};
	struct CXL_DRAM_ACCESS {
		unsigned int Size_in_bytes{ 0 };
		stream_id_type stream_id{ 0 };
		uint64_t lba{ 0 };
		bool rw{ 1 };
		CXL_DRAM_EVENTS type{ CXL_DRAM_EVENTS::CACHE_HIT };
		sim_time_type initiate_time{0};
		sim_time_type arrive_dram_time{ 0 };

		CXL_DRAM_ACCESS(unsigned int size, uint64_t addr, bool read, CXL_DRAM_EVENTS t , sim_time_type ti) {
			Size_in_bytes = size;
			lba = addr;
			rw = read;
			type = t;
			initiate_time = ti;
		}
	};
	class CXL_DRAM_Model : public MQSimEngine::Sim_Object {
	public:
		CXL_DRAM_Model(const sim_object_id_type& id, Host_Interface_Base* hosti,
			unsigned int dram_row_size, unsigned int dram_data_rate, unsigned int dram_busrt_size
			, sim_time_type dram_tRCD, sim_time_type dram_tCL, sim_time_type dram_tRP);

		~CXL_DRAM_Model();

		void service_cxl_dram_access(CXL_DRAM_ACCESS* request);


		void Start_simulation() {};


		void Validate_simulation_config() {};

		void Setup_triggers() {};

		void attachHostInterface(Host_Interface_Base* hosti);

		void Execute_simulator_event(MQSimEngine::Sim_Event* ev);

		bool dram_is_busy{ 0 }; 

		uint64_t getDRAMAvailability();

		uint64_t cache_miss_count{ 0 }, cache_hum_count{ 0 },cache_hit_count{ 0 }, total_number_of_requests{ 0 }, flash_read_count{ 0 }, number_of_accesses{0}, prefetch_amount{0};
		float perc{ 1 };
		bool results_printed{ 0 };

	private:
		unsigned int dram_row_size{0};//The size of the DRAM rows in bytes
		unsigned int dram_data_rate{0};//in MT/s
		unsigned int dram_busrt_size{0};
		double dram_burst_transfer_time_ddr{0};//The transfer time of two bursts, changed from sim_time_type to double to increase precision
		sim_time_type dram_tRCD{ 0 }, dram_tCL{ 0 }, dram_tRP{0};//DRAM access parameters in nano-seconds
		Host_Interface_Base* hi{NULL};

		std::list<CXL_DRAM_ACCESS*>* waiting_request_queue;
		uint64_t max_wait_queue_size{ 8 };

		CXL_DRAM_ACCESS* current_access{NULL};
		std::map<sim_time_type, list<CXL_DRAM_ACCESS*>>* list_of_current_access{ NULL };
		uint64_t num_working_request{ 0 };
		uint64_t num_chan{ 1 };

		

	};
}

