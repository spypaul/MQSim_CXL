#ifndef CXL_PCIE_H
#define CXL_PCIE_H

#include <list>
#include "../sim/Sim_Defs.h"
#include "../sim/Sim_Object.h"
#include "../sim/Sim_Event.h"
#include "../host/Host_IO_Request.h"
#include "Host_Interface_CXL.h"

namespace SSD_Components {
	class Host_Interface_CXL;
}

namespace Host_Components {
	class CXL_PCIe :public MQSimEngine::Sim_Object {
	public:
		CXL_PCIe(const sim_object_id_type& id);
		void Start_simulation();
		void Validate_simulation_config();
		void Execute_simulator_event(MQSimEngine::Sim_Event* event);

		void Attach_ssd_device(SSD_Components::Host_Interface_CXL* cxl_host_interface);

		std::list<Host_IO_Request*> requests_queue;
	private:
		SSD_Components::Host_Interface_CXL* cxl_host_interface;
	};
}

#endif