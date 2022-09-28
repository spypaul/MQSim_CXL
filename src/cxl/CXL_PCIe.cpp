#include "CXL_PCIe.h"

uint64_t resumefeeding{ 0 };

namespace Host_Components {
	CXL_PCIe::CXL_PCIe(const sim_object_id_type& id) : Sim_Object(id) {

	}

	void CXL_PCIe::Deliver(PCIe_Message* message) {
		returned_request_count++;
		std::cout << "Returned Request Count: " << returned_request_count << std::endl;

	}

	void CXL_PCIe::Start_simulation(){}

	void CXL_PCIe::Validate_simulation_config() {}
	void CXL_PCIe::Execute_simulator_event(MQSimEngine::Sim_Event* event) {

		if (mshr_full) {
			skipped_requests++;
			return;
		}

		if (requests_queue.size() >= device_request_queue_max_size) {
			//std::cout << " resume feeding "<< resumefeeding << std::endl;
			Simulator->Register_sim_event(Simulator->Time(), (MQSimEngine::Sim_Object*)io_flow, 0, 0);
		}

		Host_IO_Request* io_request{ requests_queue.front()};
		requests_queue.pop_front();

		PCIe_Message* message{ new PCIe_Message };

		
		message->Destination = PCIe_Destination_Type::DEVICE;
		message->Type = PCIe_Message_Type::READ_COMP;
		message->Address = 0;

		Submission_Queue_Entry* sqe = new Submission_Queue_Entry;
		sqe->Command_Identifier = 0;
		sqe->Opcode = (io_request->Type == Host_IO_Request_Type::READ) ? NVME_READ_OPCODE : NVME_WRITE_OPCODE;
		sqe->Command_specific[0] = (uint32_t)io_request->Start_LBA;
		sqe->Command_specific[1] = (uint32_t)(io_request->Start_LBA >> 32);
		sqe->Command_specific[2] = ((uint32_t)((uint16_t)io_request->LBA_count)) & (uint32_t)(0x0000ffff);
		sqe->PRP_entry_1 = (DATA_MEMORY_REGION);//Dummy addresses, just to emulate data read/write access
		sqe->PRP_entry_2 = (DATA_MEMORY_REGION + 0x1000);//Dummy addresses

		message->Payload = sqe;
		message->Payload_size = sizeof(Submission_Queue_Entry);

		pcie_switch->Deliver_to_device(message);

		//request_count++;

		delete io_request;

	}

	void CXL_PCIe::Set_pcie_switch(PCIe_Switch* pcie_switch) {
		this->pcie_switch = pcie_switch;
	}

	void CXL_PCIe::MSHR_full() {
		mshr_full = 1;
	}

	void CXL_PCIe::MSHR_not_full() {
		mshr_full = 0;
		while (skipped_requests > 0) {

			skipped_requests--;
			Simulator->Register_sim_event(Simulator->Time(), this, 0, 0);
		}
	}

}