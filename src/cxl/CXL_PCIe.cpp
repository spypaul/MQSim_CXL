#include "CXL_PCIe.h"


namespace Host_Components {
	CXL_PCIe::CXL_PCIe(const sim_object_id_type& id) : Sim_Object(id) {

	}

	void CXL_PCIe::Start_simulation(){

	}

	void CXL_PCIe::Validate_simulation_config() {

	}
	void CXL_PCIe::Execute_simulator_event(MQSimEngine::Sim_Event* event) {
		Host_IO_Request* io_request{ requests_queue.front()};
		requests_queue.pop_front();


	}

	void CXL_PCIe::Attach_ssd_device(SSD_Components::Host_Interface_CXL* cxl_host_interface) {
		this->cxl_host_interface = cxl_host_interface;
	}
}