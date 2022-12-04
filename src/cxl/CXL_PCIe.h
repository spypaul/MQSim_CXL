#ifndef CXL_PCIE_H
#define CXL_PCIE_H

#include <list>
#include "../sim/Sim_Defs.h"
#include "../sim/Sim_Object.h"
#include "../sim/Sim_Event.h"
#include"../sim/Engine.h"
#include "../host/Host_IO_Request.h"
#include "../host/IO_Flow_Base.h"
#include "../host/PCIe_Switch.h"
#include "../ssd/Host_Interface_Defs.h"
//#include "Host_Interface_CXL.h"

//namespace SSD_Components {
//	class Host_Interface_CXL;
//}


namespace Host_Components {
	class PCIe_Switch;
	class IO_Flow_Base;
	class CXL_PCIe :public MQSimEngine::Sim_Object {
	public:
		CXL_PCIe(const sim_object_id_type& id);

		void Deliver(PCIe_Message* message);
		void Start_simulation();
		void Validate_simulation_config();
		void Execute_simulator_event(MQSimEngine::Sim_Event* event);
		void Set_pcie_switch(PCIe_Switch* pcie_switch);
		void Set_io_flow(IO_Flow_Base* iof) { io_flow = iof; }

		bool device_avail() {
			return (requests_queue.size() < device_request_queue_max_size);
		}
		void Request_completed() {
			if (request_count >= device_request_queue_max_size) {
				Simulator->Register_sim_event(Simulator->Time(), (MQSimEngine::Sim_Object*)io_flow, 0,0);
			}
			//request_count--;
		}

		void MSHR_full();
		void MSHR_not_full();
		
		void mark_dram_full();

		void mark_dram_free();

		void mark_flash_full();

		void mark_flash_free();

		std::list<Host_IO_Request*> requests_queue;

		uint64_t skipped_trace_reading{ 0 };
		
	private:
		PCIe_Switch* pcie_switch{NULL};
		uint64_t returned_request_count{ 0 };

		uint64_t device_request_queue_max_size{ 32 };
		uint64_t request_count{ 0 };
		bool mshr_full{ 0 };
		IO_Flow_Base* io_flow{ NULL };
		uint64_t skipped_requests{ 0 };

		uint64_t device_dram_avail{ 1 };
		uint64_t flash_device_avail{ 1 };
	};
}

#endif