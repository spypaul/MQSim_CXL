#include "DRAM_Model.h"


namespace SSD_Components {


	CXL_DRAM_Model::CXL_DRAM_Model(const sim_object_id_type& id, Host_Interface_Base* hosti,
		unsigned int dram_row_size, unsigned int dram_data_rate, unsigned int dram_busrt_size
		, sim_time_type dram_tRCD, sim_time_type dram_tCL, sim_time_type dram_tRP)
		: MQSimEngine::Sim_Object(id), hi(hosti), dram_row_size(dram_row_size), dram_data_rate(dram_data_rate), dram_busrt_size(dram_busrt_size),
		dram_tRCD(dram_tRCD), dram_tCL(dram_tCL), dram_tRP(dram_tRP)
	{
		dram_burst_transfer_time_ddr = (double)ONE_SECOND / (dram_data_rate * 1000 * 1000);
		waiting_request_queue = new std::list<CXL_DRAM_ACCESS*>;

	}

	CXL_DRAM_Model::~CXL_DRAM_Model() {
		if (waiting_request_queue) {
			for (auto i : *waiting_request_queue) {
				delete i;
			}
			delete waiting_request_queue;
		}
	}


	void CXL_DRAM_Model::Execute_simulator_event(MQSimEngine::Sim_Event* ev) {
		CXL_DRAM_EVENTS eventype{(CXL_DRAM_EVENTS) ev->Type};

		bool falsehit{ 0 };

		switch (eventype) {
		case CXL_DRAM_EVENTS::CACHE_HIT:
			//update DRAM states
			hi->Update_CXL_DRAM_state(current_access->rw, current_access->lba, falsehit);
			if (!falsehit) {
				outputf.of << "Finished_time " << Simulator->Time() << " Starting_time " << current_access->initiate_time << " Cache_hit_at " << current_access->lba << std::endl;
			}
			else {
				outputf.of << "Finished_time " << Simulator->Time() << " Starting_time " << current_access->initiate_time << " False_hit_at " << current_access->lba << std::endl;
				hi->Handle_CXL_false_hit(current_access->rw, current_access->lba);
			}
			delete current_access;
			break;
		case CXL_DRAM_EVENTS::CACHE_MISS:
			hi->Update_CXL_DRAM_state_when_miss_data_ready(current_access->rw, current_access->lba);
			outputf.of << "Finished_time " << Simulator->Time()  << " Starting_time " << current_access->initiate_time << " Cache_miss_at " << current_access->lba << std::endl;
			delete current_access;
			break;
		}

		dram_is_busy = 0;
		if (!waiting_request_queue->empty()) {
			current_access = waiting_request_queue->front();
			waiting_request_queue->pop_front();
			Simulator->Register_sim_event(Simulator->Time() + estimate_dram_access_time(current_access->Size_in_bytes, dram_row_size,
				dram_busrt_size, dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP), this, NULL, static_cast<int>(current_access->type));
			dram_is_busy = 1;
		}


		
	}




	void CXL_DRAM_Model::service_cxl_dram_access(CXL_DRAM_ACCESS* request) {
		if (dram_is_busy) {
			waiting_request_queue->push_back(request);
		}
		else {
			current_access = request;

			Simulator->Register_sim_event(Simulator->Time() + estimate_dram_access_time(request->Size_in_bytes, dram_row_size,
				dram_busrt_size, dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP), this, NULL, static_cast<int>(request->type));
			dram_is_busy = 1;

		}
	}

	void CXL_DRAM_Model::attachHostInterface(Host_Interface_Base* hosti) {
		hi = hosti;
	}

}