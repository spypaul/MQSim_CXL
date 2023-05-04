#include "DRAM_Model.h"

#include <fstream>

//ofstream ofi{ "cache_wait_time.txt" };
//ofstream ofi2{ "Flash_read_time.txt" };
//ofstream ofi3{ "Serviced_Request_Amount.txt" };

ofstream oflate{ "./Results/latency_result.txt" };
//ofstream offree{ "DRAM_FREE_TIME.txt" };

uint64_t totalcount{ 0 };
uint64_t LAST_CACHE_HIT{ 0 };

namespace SSD_Components {


	CXL_DRAM_Model::CXL_DRAM_Model(const sim_object_id_type& id, Host_Interface_Base* hosti,
		unsigned int dram_row_size, unsigned int dram_data_rate, unsigned int dram_busrt_size
		, sim_time_type dram_tRCD, sim_time_type dram_tCL, sim_time_type dram_tRP)
		: MQSimEngine::Sim_Object(id), hi(hosti), dram_row_size(dram_row_size), dram_data_rate(dram_data_rate), dram_busrt_size(dram_busrt_size),
		dram_tRCD(dram_tRCD), dram_tCL(dram_tCL), dram_tRP(dram_tRP)
	{
		dram_burst_transfer_time_ddr = (double)ONE_SECOND / (dram_data_rate * 1000 * 1000);
		waiting_request_queue = new std::list<CXL_DRAM_ACCESS*>;
		list_of_current_access = new std::map<sim_time_type, list<CXL_DRAM_ACCESS*>>;

	}

	CXL_DRAM_Model::~CXL_DRAM_Model() {
		if (waiting_request_queue) {
			for (auto i : *waiting_request_queue) {
				delete i;
			}
			delete waiting_request_queue;
		}

		if (list_of_current_access) {
			for (auto i : *list_of_current_access) {
				for (auto j : i.second) {
					delete j;
				}
			}
			delete list_of_current_access;
		}
	}


	void CXL_DRAM_Model::Execute_simulator_event(MQSimEngine::Sim_Event* ev) {



		CXL_DRAM_EVENTS eventype{(CXL_DRAM_EVENTS) ev->Type};

		bool falsehit{ 0 };
		auto it{ ((*list_of_current_access)[Simulator->Time()]).begin() };

		while (it != ((*list_of_current_access)[Simulator->Time()]).end()) {

			if (eventype == (*it)->type) {
				current_access = *it;
				((*list_of_current_access)[Simulator->Time()]).erase(it);
				break;
			}			
			it++;
		}
		if (((*list_of_current_access)[Simulator->Time()]).size() == 0) {
			list_of_current_access->erase(list_of_current_access->find(Simulator->Time()));
		}
		num_working_request--;
		

		switch (eventype) {
		case CXL_DRAM_EVENTS::CACHE_HIT:
			number_of_accesses++;
			//update DRAM states
			//hi->Update_CXL_DRAM_state(current_access->rw, current_access->lba, falsehit);
			cache_hit_count++;
			if (!falsehit) {
				//outputf.of << "Finished_time " << Simulator->Time() << " Starting_time " << current_access->initiate_time << " Cache_hit_at " << current_access->lba << std::endl;
				oflate << Simulator->Time() - current_access->initiate_time << endl;
				//hi->Notify_CXL_Host_request_complete();
				totalcount++;
			}
			else {
				//outputf.of << "Finished_time " << Simulator->Time() << " Starting_time " << current_access->initiate_time << " False_hit_at " << current_access->lba << std::endl;
				//hi->Handle_CXL_false_hit(current_access->rw, current_access->lba);
			}
			delete current_access;
			break;
		case CXL_DRAM_EVENTS::CACHE_HIT_UNDER_MISS:
			number_of_accesses++;
			//hi->Update_CXL_DRAM_state(current_access->rw, current_access->lba, falsehit);
			cache_hum_count++;
			if (!falsehit) {
				//outputf.of << "Finished_time " << Simulator->Time() << " Starting_time " << current_access->initiate_time << " Cache_hit_under_miss_at " << current_access->lba << std::endl;
				oflate << Simulator->Time() - current_access->initiate_time << endl;
				//hi->Notify_CXL_Host_request_complete();
				totalcount++;
			}
			else {
				//outputf.of << "Finished_time " << Simulator->Time() << " Starting_time " << current_access->initiate_time << " False_hit_at " << current_access->lba << std::endl;
				//hi->Handle_CXL_false_hit(current_access->rw, current_access->lba);
			}
			delete current_access;
			break;
		case CXL_DRAM_EVENTS::CACHE_MISS:
			number_of_accesses++;
			flash_read_count++;
			//cache_miss_count++;
			//ofi2 << current_access->initiate_time << " " << Simulator->Time() << endl;
			//hi->Update_CXL_DRAM_state_when_miss_data_ready(current_access->rw, current_access->lba);
			//outputf.of << "Finished_time " << Simulator->Time()  << " Starting_time " << current_access->initiate_time << " Cache_miss_at " << current_access->lba << std::endl;
			oflate << Simulator->Time() - current_access->initiate_time << endl;
			//hi->Notify_CXL_Host_request_complete();
			totalcount++;
			delete current_access;
			break;
		case CXL_DRAM_EVENTS::PREFETCH_READY:
			//flash_read_count++;
			//outputf.of << "Finished_time " << Simulator->Time() << " Starting_time " << current_access->initiate_time << " Prefetch_ready_at " << current_access->lba << std::endl;
			prefetch_amount++;
			delete current_access;
			break;
		case CXL_DRAM_EVENTS::SLOW_PREFETCH:
			number_of_accesses++;
			//flash_read_count++;
			//cache_miss_count++;
			prefetch_amount++;
			//ofi2 << current_access->initiate_time << " " << Simulator->Time() << endl;
			//hi->Update_CXL_DRAM_state_when_miss_data_ready(current_access->rw, current_access->lba);
			//outputf.of << "Finished_time " << Simulator->Time() << " Starting_time " << current_access->initiate_time << " Slow_prefetch_at " << current_access->lba << std::endl;
			oflate << Simulator->Time() - current_access->initiate_time << endl;
			//hi->Notify_CXL_Host_request_complete();
			totalcount++;
			delete current_access;
			break;
		}

		if (num_working_request < num_chan)dram_is_busy = 0;

		if (!dram_is_busy && !waiting_request_queue->empty()) {

			//current_access = waiting_request_queue->front();
			CXL_DRAM_ACCESS* caccess{ waiting_request_queue->front() };
			waiting_request_queue->pop_front();

			sim_time_type ts{ Simulator->Time() + estimate_dram_access_time(caccess->Size_in_bytes, dram_row_size,
				dram_busrt_size, dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP) };

			if (list_of_current_access->count(ts)) {
				(*list_of_current_access)[ts].push_back(caccess);
			}
			else {
				list<CXL_DRAM_ACCESS*> l;
				l.push_back(caccess);
				(*list_of_current_access)[ts] = l;
			}
			num_working_request++;


			//ofi << Simulator->Time() - caccess->arrive_dram_time << endl;

			Simulator->Register_sim_event(Simulator->Time() + estimate_dram_access_time(caccess->Size_in_bytes, dram_row_size,
				dram_busrt_size, dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP), this, NULL, static_cast<int>(caccess->type));

			if (num_working_request >= num_chan)dram_is_busy = 1;

			if (waiting_request_queue->size() < max_wait_queue_size) {
				hi->Notify_DRAM_is_free();
				//offree << Simulator->Time() << endl;

			}
		}

		//ofi << totalcount << endl;
		//number_of_accesses++;

		float current_progress{ static_cast<float>(number_of_accesses) / static_cast<float>(total_number_of_requests) };
		if (current_progress<1 && current_progress * 100 - perc > 1) {
			perc += 1;
			uint8_t number_of_bars{ static_cast<uint8_t> (perc / 4) };

			std::cout << "Simulation progress: [";
			for (auto i = 0; i < number_of_bars; i++) {
				std::cout << "=";
			}
			for (auto i = 0; i < 25 - number_of_bars - 1; i++) {
				std::cout << " ";
			}

			std::cout << "] " << perc << "%   Cache Hit Count: " << cache_hit_count << "   Prefetch amount: "<< prefetch_amount << "\r";
		}

		if (number_of_accesses == total_number_of_requests && !results_printed) {
			results_printed = 1;
			std::cout << "Simulation progress: [";
			for (auto i = 0; i < 25; i++) {
				std::cout << "=";
			}
			std::cout << "] " << 100 << "%    Cache Hit Count: " << cache_hit_count << "   Prefetch amount: " << prefetch_amount << std::endl;
			of_overall<< "Cache Hit Count: " << cache_hit_count << "   Prefetch amount: " << prefetch_amount << std::endl;
			std::cout << "Hit under miss count: " << cache_hum_count << endl;
			of_overall << "Hit under miss count: " << cache_hum_count << endl;
			std::cout << "Flash Read Count: " << flash_read_count << endl;
			of_overall << "Flash Read Count: " << flash_read_count << endl;
			if (prefetch_amount > 0) {
				std::cout << "Prefetch Flash Read Count: " << prefetch_amount << endl;
				of_overall << "Prefetch Flash Read Count: " << prefetch_amount << endl;
				std::cout << "Total Flash Read Count: " << flash_read_count + prefetch_amount << endl;
				of_overall << "Total Flash Read Count: " << flash_read_count + prefetch_amount << endl;
			}
			//std::cout << "Flush count: " << hi->Get_flush_count() << endl;
			//std::cout << "Request ends at timestamp: " << static_cast<float>(Simulator->Time()) / 1000000000 << " s" << endl;
			hi->print_prefetch_info();
		}
		//if (number_of_accesses % 43000000 == 0) {
		//	of_overall << "Sample: "<< number_of_accesses / 43000000 << endl;
		//	of_overall << "Sample Cache Hit: " << cache_hit_count - LAST_CACHE_HIT << endl;
		//	LAST_CACHE_HIT = cache_hit_count;

		//	of_overall << "Cache Hit Count: " << cache_hit_count << endl;
		//	of_overall << "Hit under miss count: " << cache_hum_count << endl;
		//	hi->print_prefetch_info();
		//	of_overall << endl;

		//}
		//ofi3 << number_of_accesses << endl;
		
	}




	void CXL_DRAM_Model::service_cxl_dram_access(CXL_DRAM_ACCESS* request) {
		if (dram_is_busy) {
			request->arrive_dram_time = Simulator->Time();
			waiting_request_queue->push_back(request);

			if (waiting_request_queue->size() >= max_wait_queue_size) {
				hi->Notify_DRAM_is_full();
			}
		}
		else {
			//current_access = request;
			sim_time_type ts{ Simulator->Time() + estimate_dram_access_time(request->Size_in_bytes, dram_row_size,
				dram_busrt_size, dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP) };
			//when miss data ready, return ts, so the time for the sub requests can be right
			// if miss data ready is in wait, ...issue 
			//cout << Simulator->Time() << endl; 
			//cout << ts - Simulator->Time() << endl;
			if (list_of_current_access->count(ts)) {
				(*list_of_current_access)[ts].push_back(request);
			}
			else {
				list<CXL_DRAM_ACCESS*> l;
				l.push_back(request);
				(*list_of_current_access)[ts] = l;
			}
			num_working_request++;
			//list_of_current_access->push_back(request);


			Simulator->Register_sim_event(Simulator->Time() + estimate_dram_access_time(request->Size_in_bytes, dram_row_size,
				dram_busrt_size, dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP), this, NULL, static_cast<int>(request->type));
			if(num_working_request >= num_chan) dram_is_busy = 1;

		}
	}

	void CXL_DRAM_Model::attachHostInterface(Host_Interface_Base* hosti) {
		hi = hosti;
	}

	uint64_t CXL_DRAM_Model::getDRAMAvailability() {
		if (dram_is_busy) {
			return max_wait_queue_size - waiting_request_queue->size();
		}
		else {
			return max_wait_queue_size - waiting_request_queue->size() + 1;
		}
	}
}