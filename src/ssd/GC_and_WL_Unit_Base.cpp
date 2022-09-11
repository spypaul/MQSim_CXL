#include "GC_and_WL_Unit_Base.h"

namespace SSD_Components
{
	GC_and_WL_Unit_Base* GC_and_WL_Unit_Base::_my_instance;
	
	GC_and_WL_Unit_Base::GC_and_WL_Unit_Base(const sim_object_id_type& id,
		Address_Mapping_Unit_Base* address_mapping_unit, Flash_Block_Manager_Base* block_manager, TSU_Base* tsu, NVM_PHY_ONFI* flash_controller,
		GC_Block_Selection_Policy_Type block_selection_policy, double gc_threshold, bool preemptible_gc_enabled, double gc_hard_threshold,
		unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
		unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int sector_no_per_page, 
		bool use_copyback, double rho, unsigned int max_ongoing_gc_reqs_per_plane, bool dynamic_wearleveling_enabled, bool static_wearleveling_enabled, unsigned int static_wearleveling_threshold, int seed) :
		Sim_Object(id), address_mapping_unit(address_mapping_unit), block_manager(block_manager), tsu(tsu), flash_controller(flash_controller), force_gc(false),
		block_selection_policy(block_selection_policy), gc_threshold(gc_threshold),	use_copyback(use_copyback), 
		preemptible_gc_enabled(preemptible_gc_enabled), gc_hard_threshold(gc_hard_threshold),
		random_generator(seed), max_ongoing_gc_reqs_per_plane(max_ongoing_gc_reqs_per_plane),
		channel_count(channel_count), chip_no_per_channel(chip_no_per_channel), die_no_per_chip(die_no_per_chip), plane_no_per_die(plane_no_per_die),
		block_no_per_plane(block_no_per_plane), pages_no_per_block(page_no_per_block), sector_no_per_page(sector_no_per_page),
		dynamic_wearleveling_enabled(dynamic_wearleveling_enabled), static_wearleveling_enabled(static_wearleveling_enabled), static_wearleveling_threshold(static_wearleveling_threshold)
	{
		_my_instance = this;

		block_pool_gc_threshold = (unsigned int)(gc_threshold * (double)block_no_per_plane);
		if (block_pool_gc_threshold < 1) {
			block_pool_gc_threshold = 1;
		}
		block_pool_gc_hard_threshold = (unsigned int)(gc_hard_threshold * (double)block_no_per_plane);
		if (block_pool_gc_hard_threshold < 1) {
			block_pool_gc_hard_threshold = 1;
		}
		random_pp_threshold = (unsigned int)(rho * pages_no_per_block);
		if (block_pool_gc_threshold < max_ongoing_gc_reqs_per_plane) {
			block_pool_gc_threshold = max_ongoing_gc_reqs_per_plane;
		}

		gc_unit_count = channel_count * chip_no_per_channel * die_no_per_chip * plane_no_per_die;
		
		victim_blocks = new Block_Pool_Slot_Type*[gc_unit_count];
		gc_victim_address = new NVM::FlashMemory::Physical_Page_Address[gc_unit_count];

		gc_pending_read_count = 0;
		gc_pending_write_count = 0;
		gc_pending_erase_count = 0;
		
		cur_page_offset = 0;
		cur_subpage_offset = 0;
		new_victim_required = false;

		intial_token = gc_unit_count * pages_no_per_block;			
		token = intial_token;
		token_per_write = 0;

		cur_state_index = 0;
		state_repeat_count = 0;
	}

	void GC_and_WL_Unit_Base::Setup_triggers()
	{
		Sim_Object::Setup_triggers();
		flash_controller->ConnectToTransactionServicedSignal(handle_transaction_serviced_signal_from_PHY);
	}

	//called from Flash_Chip::finish_command()
	void GC_and_WL_Unit_Base::handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction)
	{
		PlaneBookKeepingType* pbke = &(_my_instance->block_manager->plane_manager[transaction->Address.ChannelID][transaction->Address.ChipID][transaction->Address.DieID][transaction->Address.PlaneID]);

		switch (transaction->Source) {
			case Transaction_Source_Type::USERIO:
			case Transaction_Source_Type::MAPPING:
			case Transaction_Source_Type::CACHE:
				switch (transaction->Type)
				{
					case Transaction_Type::READ:
						_my_instance->block_manager->Read_transaction_serviced(transaction->Address);
						break;
					case Transaction_Type::WRITE:
						_my_instance->block_manager->Program_transaction_serviced(transaction->Address);

						if (((NVM_Transaction_Flash_WR*)transaction)->RelatedWrite_SUB.size() != 0) {
							for (int idx = 0; idx < ((NVM_Transaction_Flash_WR*)transaction)->RelatedWrite_SUB.size(); idx++) {
								//std::cout << "RelatedWrite_SUB's size: " << ((NVM_Transaction_Flash_WR*)transaction)->RelatedWrite_SUB.size() << std::endl;
								_my_instance->block_manager->Program_transaction_serviced(transaction->Address);
							}
						}

						break;
					default:
						PRINT_ERROR("Unexpected situation in the GC_and_WL_Unit_Base function!")
				}
				
				return;
		}

		switch (transaction->Type) {
			case Transaction_Type::READ:
			{
				//Loop transaction and sub-transactions and put RelatedWrites into waiting_writeback_transaction 
				PPA_type ppa;
				MPPN_type mppa;
				page_status_type page_status_bitmap;
				NVM_Transaction_Flash_WR* RelatedWrite = ((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite;
					
				if (pbke->Blocks[transaction->Address.BlockID].Holds_mapping_data) {
					PRINT_ERROR("[DOODU ERROR] Didn't consider this condition. Check Logic");
					_my_instance->address_mapping_unit->Get_translation_mapping_info_for_gc(transaction->Stream_id, (MVPN_type)transaction->LPA, mppa, page_status_bitmap);
					//There has been no write on the page since GC start, and it is still valid
					if (mppa == transaction->PPA) {
						//_my_instance->tsu->Prepare_for_transaction_submit();
						RelatedWrite->write_sectors_bitmap = FULL_PROGRAMMED_PAGE;
						RelatedWrite->LPA = transaction->LPA;
						RelatedWrite->RelatedRead = NULL;

						_my_instance->address_mapping_unit->Allocate_new_page_for_gc(RelatedWrite, true);

						_my_instance->waiting_writeback_transaction.push_back(RelatedWrite);
						//_my_instance->tsu->Submit_transaction(RelatedWrite);
						//_my_instance->tsu->Schedule();
					} else {
						//PRINT_ERROR("Inconsistency found when moving a page for GC/WL!")
						// source page is not valid anymore. 
						
						delete RelatedWrite;					
					}
				} else {		
					_my_instance->address_mapping_unit->Get_data_mapping_info_for_gc(transaction->Stream_id, transaction->LPA, ppa, page_status_bitmap);
					
					//There has been no write on the page since GC start, and it is still valid
					if (ppa == transaction->PPA) {
						//std::cout << "match! (ppa and PPA): " << ((NVM_Transaction_Flash_RD*)transaction)->PPA << std::endl;
						//_my_instance->tsu->Prepare_for_transaction_submit();
						RelatedWrite->write_sectors_bitmap = page_status_bitmap;
						RelatedWrite->LPA = transaction->LPA;
						RelatedWrite->RelatedRead = NULL;
						
						_my_instance->address_mapping_unit->Allocate_new_page_for_gc(RelatedWrite, false);

						_my_instance->waiting_writeback_transaction.push_back(RelatedWrite);
					} else {
						// source page is not valid anymore. 												
						/*PRINT_MESSAGE("Overwrite in the middle of GC lpa : " << transaction->LPA  << " ch : " << RelatedWrite->Address.ChannelID << " plane : " << RelatedWrite->Address.PlaneID << " blk: " << RelatedWrite->Address.BlockID << " page: " << RelatedWrite->Address.PageID);						
						PRINT_MESSAGE(" Org PPA : " << transaction->PPA << " New PPA : " << ppa );	

						NVM::FlashMemory::Physical_Page_Address address;
						_my_instance->address_mapping_unit->Convert_ppa_to_address(ppa, address);
						PRINT_MESSAGE("Overwritted address ? : " << " ch : " << address.ChannelID << " plane : " << address.PlaneID << " blk " << address.BlockID << " page " << address.PageID << std::endl);
						*/
						//allocate new dummy page for GC page and invalidate the new allocated(dummy) GC page

						_my_instance->address_mapping_unit->Allocate_dummy_pages_for_gc(RelatedWrite, false, false);
						delete RelatedWrite;
					}
				}


				_my_instance->gc_pending_read_count--;
				if (_my_instance->gc_pending_read_count == 0){
					if (((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB.size() != 0) { 
						PRINT_ERROR("gc_pending read count should not be 0 yet") 
					}
					_my_instance->Check_gc_required(pbke->Get_free_block_pool_size(), transaction->Address);
					//std::cout << "flag1" << std::endl;
					if (_my_instance->gc_pending_write_count == 0){
						// abnormal case. special case handling

						_my_instance->address_mapping_unit->Start_servicing_writes_for_overfull();	
				
					}
				}



				if (((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB.size() != 0) {
					for (int idx = 0; idx < ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB.size(); idx++) {
						NVM_Transaction_Flash_WR* RelatedWrite_subpg = ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->RelatedWrite;
						if (pbke->Blocks[((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.BlockID].Holds_mapping_data) {
							PRINT_ERROR("[DOODU ERROR] Didn't consider this condition. Check Logic");
						}
						else {
							//std::cout << "RelatedRead_SUB[idx]'s LPA  (Get_data_mapping info target): " << ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->LPA << ", tr's PPA: "<<((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->PPA <<std::endl;
							_my_instance->address_mapping_unit->Get_data_mapping_info_for_gc(((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Stream_id, ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->LPA, ppa, page_status_bitmap);

							//There has been no write on the page since GC start, and it is still valid
							if (ppa == ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->PPA) {
								//std::cout << "match! PPA: " << ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->PPA << std::endl;
								//_my_instance->tsu->Prepare_for_transaction_submit();
								RelatedWrite_subpg->write_sectors_bitmap = page_status_bitmap;
								RelatedWrite_subpg->LPA = ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->LPA;
								RelatedWrite_subpg->RelatedRead = NULL;
						
								_my_instance->address_mapping_unit->Allocate_new_page_for_gc(RelatedWrite_subpg, false);

								_my_instance->waiting_writeback_transaction.push_back(RelatedWrite_subpg);
							}
							else {
								Block_Pool_Slot_Type* target_Block;
								PlaneBookKeepingType* pbke = &(_my_instance->block_manager->plane_manager[((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.ChannelID][((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.ChipID][((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.DieID][((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.PlaneID]);
								target_Block = &(pbke->Blocks[((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.BlockID]);
								//std::cout << "not match! PPA: " << ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->PPA << std::endl;
								if (_my_instance->block_manager->Is_Subpage_valid(target_Block, ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.PageID, ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.subPageID)) {
									std::cout << "Something wrong. try to invalidate this becuz we expected user invalidated. but user had not invalidated. info: " << ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.ChannelID << ", " << ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.ChipID << ", " << ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.DieID << ", " << ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.PlaneID << ", " << ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.BlockID << ", " << ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.PageID << ", " << ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address.subPageID <<", ppa in table: "<<ppa<< ", tr's PPA: "<< ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->PPA<< ", lpa: "<< ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->LPA<< std::endl;
									exit(1);
								}
	
								_my_instance->address_mapping_unit->Allocate_dummy_pages_for_gc(RelatedWrite_subpg, false, false);
								delete RelatedWrite_subpg;
							}
						}

						_my_instance->gc_pending_read_count--;
						if (_my_instance->gc_pending_read_count == 0) {
							_my_instance->Check_gc_required(pbke->Get_free_block_pool_size(), ((NVM_Transaction_Flash_RD*)transaction)->RelatedRead_SUB[idx]->Address);
							if (_my_instance->gc_pending_write_count == 0) {
								// abnormal case. special case handling

								_my_instance->address_mapping_unit->Start_servicing_writes_for_overfull();	

							}
						}
					}
				}

				break;
			}
			case Transaction_Type::WRITE:
				// intentionally remove barrier related implementation.

				// erase will be done at allocation.
				//pbke->Blocks[((NVM_Transaction_Flash_WR*)transaction)->RelatedErase->Address.BlockID].Erase_transaction->Page_movement_activities.remove((NVM_Transaction_Flash_WR*)transaction);
				_my_instance->gc_pending_write_count--;
				_my_instance->Check_gc_required(pbke->Get_free_block_pool_size(), transaction->Address);



				if (((NVM_Transaction_Flash_WR*)transaction)->RelatedWrite_SUB.size() != 0) {
					for (int idx = 0; idx < ((NVM_Transaction_Flash_WR*)transaction)->RelatedWrite_SUB.size(); idx++) {
						_my_instance->gc_pending_write_count--;
						_my_instance->Check_gc_required(pbke->Get_free_block_pool_size(), ((NVM_Transaction_Flash_WR*)transaction)->RelatedWrite_SUB[idx]->Address);
					}
				}

				//std::cout << "flag2" << std::endl;

				// TEMP.
				_my_instance->address_mapping_unit->Start_servicing_writes_for_overfull_plane(transaction->Address);

				_my_instance->address_mapping_unit->Start_servicing_writes_for_overfull();

				
				break;
			case Transaction_Type::ERASE:
				_my_instance->gc_pending_erase_count--;
				if (_my_instance->gc_pending_erase_count == 0)
				{
					_my_instance->Check_gc_required(pbke->Get_free_block_pool_size(), transaction->Address);
					//std::cout << "flag3" << std::endl;

					_my_instance->address_mapping_unit->Start_servicing_writes_for_overfull();	
	
				}
				
				break;
		} //switch (transaction->Type)
	}

	void GC_and_WL_Unit_Base::Start_simulation()
	{
	}

	void GC_and_WL_Unit_Base::Validate_simulation_config()
	{
	}

	void GC_and_WL_Unit_Base::Execute_simulator_event(MQSimEngine::Sim_Event* ev)
	{
	}

	GC_Block_Selection_Policy_Type GC_and_WL_Unit_Base::Get_gc_policy()
	{
		return block_selection_policy;
	}

	unsigned int GC_and_WL_Unit_Base::Get_GC_policy_specific_parameter()
	{
		switch (block_selection_policy) {
			case GC_Block_Selection_Policy_Type::RGA:
				return rga_set_size;
			case GC_Block_Selection_Policy_Type::RANDOM_PP:
				return random_pp_threshold;
		}

		return 0;
	}

	unsigned int GC_and_WL_Unit_Base::Get_minimum_number_of_free_pages_before_GC()
	{
		return block_pool_gc_threshold;
		/*if (preemptible_gc_enabled)
			return block_pool_gc_hard_threshold;
		else return block_pool_gc_threshold;*/
	}

	bool GC_and_WL_Unit_Base::Use_dynamic_wearleveling()
	{
		return dynamic_wearleveling_enabled;
	}

	inline bool GC_and_WL_Unit_Base::Use_static_wearleveling()
	{
		return static_wearleveling_enabled;
	}
	
	bool GC_and_WL_Unit_Base::Stop_servicing_writes(const NVM::FlashMemory::Physical_Page_Address& plane_address)
	{
		PlaneBookKeepingType* pbke = &(_my_instance->block_manager->plane_manager[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID]);
		return  block_manager->Get_pool_size(plane_address) < max_ongoing_gc_reqs_per_plane;
	}

	bool GC_and_WL_Unit_Base::Consume_token(int token_count)
	{
		// when the free block is enough - it always returns true.		
		bool consume_token = true;
		PlaneBookKeepingType* pbke = &(block_manager->plane_manager[0][0][0][0]);
		unsigned int free_block_count = pbke->Get_free_block_pool_size();
		//std::cout << "token: " << token << std::endl;
		if ( free_block_count <= block_pool_gc_threshold)
		{

			if (token > 0 )

			{
				token -= token_count;
			}
			else
			{
				consume_token = false;				
			}

			Check_gc_required(free_block_count, NULL);
			//std::cout << "flag4" << std::endl;
		}

		return consume_token;		
	}

	void GC_and_WL_Unit_Base::Adjust_token(int token_count)
	{
		PlaneBookKeepingType* pbke = &(block_manager->plane_manager[0][0][0][0]);
		unsigned int free_block_count = pbke->Get_free_block_pool_size();
	
		if ( free_block_count <= block_pool_gc_threshold)
		{
			token -= token_count;

			//Check_gc_required(free_block_count, NULL);
		}	
	}

	bool GC_and_WL_Unit_Base::is_safe_gc_wl_candidate(const PlaneBookKeepingType* plane_record, const flash_block_ID_type gc_wl_candidate_block_id)
	{
		//The block shouldn't be a current write frontier
		for (unsigned int stream_id = 0; stream_id < address_mapping_unit->Get_no_of_input_streams(); stream_id++) {
			if ((&plane_record->Blocks[gc_wl_candidate_block_id]) == plane_record->Data_wf[stream_id]
				|| (&plane_record->Blocks[gc_wl_candidate_block_id]) == plane_record->Translation_wf[stream_id]
				|| (&plane_record->Blocks[gc_wl_candidate_block_id]) == plane_record->GC_wf[stream_id]) {
				return false;
			}
		}

		//The block shouldn't have an ongoing program request (all pages must already be written)
		if (plane_record->Blocks[gc_wl_candidate_block_id].Ongoing_user_program_count > 0) {
			std::cout << "flag1: "<< plane_record->Blocks[gc_wl_candidate_block_id].Ongoing_user_program_count << std::endl;
			return false;
		}

		if (plane_record->Blocks[gc_wl_candidate_block_id].Has_ongoing_gc_wl) {
			//std::cout << "flag2" << std::endl;
			return false;
		}

		return true;
	}

	bool GC_and_WL_Unit_Base::check_static_wl_required(const NVM::FlashMemory::Physical_Page_Address plane_address)
	{
		return static_wearleveling_enabled && (block_manager->Get_min_max_erase_difference(plane_address) >= static_wearleveling_threshold);
	}

	void GC_and_WL_Unit_Base::run_static_wearleveling(const NVM::FlashMemory::Physical_Page_Address plane_address)
	{
		PlaneBookKeepingType* pbke = block_manager->Get_plane_bookkeeping_entry(plane_address);
		flash_block_ID_type wl_candidate_block_id = block_manager->Get_coldest_block_id(plane_address);
		if (!is_safe_gc_wl_candidate(pbke, wl_candidate_block_id)) {
			return;
		}

		NVM::FlashMemory::Physical_Page_Address wl_candidate_address(plane_address);
		wl_candidate_address.BlockID = wl_candidate_block_id;
		Block_Pool_Slot_Type* block = &pbke->Blocks[wl_candidate_block_id];

		//Run the state machine to protect against race condition
		block_manager->GC_WL_started(wl_candidate_block_id);
		pbke->Ongoing_erase_operations.insert(wl_candidate_block_id);
		address_mapping_unit->Set_barrier_for_accessing_physical_block(wl_candidate_address);//Lock the block, so no user request can intervene while the GC is progressing
		if (block_manager->Can_execute_gc_wl(wl_candidate_address)) {//If there are ongoing requests targeting the candidate block, the gc execution should be postponed
			Stats::Total_wl_executions++;
			tsu->Prepare_for_transaction_submit();

			NVM_Transaction_Flash_ER* wl_erase_tr = new NVM_Transaction_Flash_ER(Transaction_Source_Type::GC_WL, pbke->Blocks[wl_candidate_block_id].Stream_id, wl_candidate_address);
			if (block->Current_page_write_index - block->Invalid_page_count > 0) {//If there are some valid pages in block, then prepare flash transactions for page movement
				NVM_Transaction_Flash_RD* wl_read = NULL;
				NVM_Transaction_Flash_WR* wl_write = NULL;
				for (flash_page_ID_type pageID = 0; pageID < block->Current_page_write_index; pageID++) {
					if (block_manager->Is_page_valid(block, pageID)) {
						Stats::Total_page_movements_for_gc;
						wl_candidate_address.PageID = pageID;
						if (use_copyback) {
							wl_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
								NO_LPA, address_mapping_unit->Convert_address_to_ppa(wl_candidate_address), NULL, 0, NULL, 0, INVALID_TIME_STAMP);
							wl_write->ExecutionMode = WriteExecutionModeType::COPYBACK;
							tsu->Submit_transaction(wl_write);
						} else {
							wl_read = new NVM_Transaction_Flash_RD(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
								NO_LPA, address_mapping_unit->Convert_address_to_ppa(wl_candidate_address), wl_candidate_address, NULL, 0, NULL, 0, INVALID_TIME_STAMP);
							wl_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
								NO_LPA, NO_PPA, wl_candidate_address, NULL, 0, wl_read, 0, INVALID_TIME_STAMP);
							wl_write->ExecutionMode = WriteExecutionModeType::SIMPLE;
							wl_write->RelatedErase = wl_erase_tr;
							wl_read->RelatedWrite = wl_write;
							tsu->Submit_transaction(wl_read);//Only the read transaction would be submitted. The Write transaction is submitted when the read transaction is finished and the LPA of the target page is determined
						}
						wl_erase_tr->Page_movement_activities.push_back(wl_write);
					}
				}
			}
			block->Erase_transaction = wl_erase_tr;
			tsu->Submit_transaction(wl_erase_tr);

			tsu->Schedule();
		}
	}
}
