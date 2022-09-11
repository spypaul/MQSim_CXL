
#include "../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "Flash_Block_Manager.h"
#include "Stats.h"

//#define RELIEF_HOST_ACTIVE_BLOCK
//#define RELIEF_GC_ACTIVE_BLOCK


namespace SSD_Components
{
	Flash_Block_Manager::Flash_Block_Manager(GC_and_WL_Unit_Base* gc_and_wl_unit, unsigned int max_allowed_block_erase_count, unsigned int total_concurrent_streams_no,
		unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
		unsigned int block_no_per_plane, unsigned int page_no_per_block)
		: Flash_Block_Manager_Base(gc_and_wl_unit, max_allowed_block_erase_count, total_concurrent_streams_no, channel_count, chip_no_per_channel, die_no_per_chip,
			plane_no_per_die, block_no_per_plane, page_no_per_block)
	{
	}

	Flash_Block_Manager::~Flash_Block_Manager()
	{
	}
	
	//assign PageID & subPageID
	void Flash_Block_Manager::Allocate_block_and_page_in_plane_for_user_write(const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& page_address)
	{


		
		PlaneBookKeepingType* plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
		page_address.BlockID = plane_record->Data_wf[stream_id]->BlockID;
		//std::cout << "Current_subpage_write_index: " << plane_record->Data_wf[stream_id]->Current_subpage_write_index<< std::endl;
		page_address.subPageID = plane_record->Data_wf[stream_id]->Current_subpage_write_index++;
		page_address.PageID = plane_record->Data_wf[stream_id]->Current_page_write_index;
		//std::cout << "page_address.subPageID: " << page_address.subPageID << std::endl;
		plane_record->Valid_subpages_count++;
		plane_record->Free_subpages_count--;

		if (plane_record->Data_wf[stream_id]->Current_subpage_write_index == ALIGN_UNIT_SIZE) {
			plane_record->Data_wf[stream_id]->Current_subpage_write_index = 0;
			plane_record->Data_wf[stream_id]->Current_page_write_index++;
			plane_record->Valid_pages_count++;
			plane_record->Free_pages_count--;
		}
		else {
			page_address.PageID = plane_record->Data_wf[stream_id]->Current_page_write_index;
			//page_address.subPageID = plane_record->Data_wf[stream_id]->Current_subpage_write_index++;
		}
		

		program_transaction_issued(page_address);

		//The current write frontier block is written to the end
		if(plane_record->Data_wf[stream_id]->Current_page_write_index == pages_no_per_block) {
			bool bRelief = false;
#ifdef RELIEF_HOST_ACTIVE_BLOCK		
//			if (Stats::Relief_page_count < Stats::Relief_proportion*Stats::Physical_write_count){
			if (Stats::Interval_Relief_page_count < Stats::Relief_proportion*Stats::Interval_Physical_write_count){
				bRelief = true; // __FIXME_ TEMPROAL..
			}
#endif
			//Assign a new write frontier block
			if (bRelief == true){
				plane_record->Data_wf[stream_id] = plane_record->Get_a_free_block(stream_id, false);
			}
			else{
				plane_record->Data_wf[stream_id] = plane_record->Get_a_free_block_b(stream_id, false);
			}

			// need to check quotal.
			NVM::FlashMemory::Physical_Page_Address new_page_address(page_address);
			new_page_address.BlockID = plane_record->Data_wf[stream_id]->BlockID;
			Set_relief_status(new_page_address, bRelief);

			//gc_and_wl_unit->Check_gc_required(plane_record->Get_free_block_pool_size(), page_address);

			if ( (page_address.ChannelID == 0) && (page_address.ChipID == 0) && (page_address.PlaneID == 0))
			{
				if ((Stats::Host_alloc % 1000) == 0)
				{
					PRINT_MESSAGE("  New Host active : " << plane_record->Data_wf[stream_id]->BlockID << " -- " << Stats::Host_alloc << " stream	  " << stream_id << " max gc write info : " << Stats::Max_consecutive_gc_write << " u: " << Stats::Utilization << " R: " << Stats::Cur_relief_page_count);
				}
				Stats::Host_alloc++;
			}
		}
		
		plane_record->Check_bookkeeping_correctness(page_address);
	}
	
	void Flash_Block_Manager::Allocate_block_and_page_in_plane_for_gc_write(const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& page_address)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];

		//The current write frontier block is written to the end
		if (plane_record->GC_wf[stream_id]->Current_page_write_index == pages_no_per_block) {
			bool bRelief = false;
			NVM::FlashMemory::Physical_Page_Address tmp_page_address;
			PlaneBookKeepingType *tmp_plane_record;

			if ((page_address.ChannelID != 0) || (page_address.ChipID != 0) || (page_address.DieID != 0) || (page_address.PlaneID != 0))
			{
				PRINT_MESSAGE("NOT EXPECTED " << page_address.ChannelID << " " << page_address.PlaneID);
			}

#ifdef RELIEF_GC_ACTIVE_BLOCK
			if (Stats::Interval_Relief_page_count < Stats::Relief_proportion*Stats::Interval_Physical_write_count) {
				bRelief = true;
			}
#endif
			for (unsigned int channelID = 0; channelID < channel_count; channelID++) {
				tmp_page_address.ChannelID = channelID;
				for (unsigned int chipID = 0; chipID < chip_no_per_channel; chipID++) {
					tmp_page_address.ChipID = chipID;
					for (unsigned int dieID = 0; dieID < die_no_per_chip; dieID++) {
						tmp_page_address.DieID = dieID;
						for (unsigned int planeID = 0; planeID < plane_no_per_die; planeID++) {
							tmp_page_address.PlaneID = planeID;
							tmp_plane_record = &plane_manager[tmp_page_address.ChannelID][tmp_page_address.ChipID][tmp_page_address.DieID][tmp_page_address.PlaneID];

							//Assign a new write frontier block
							if (bRelief == true) {
								tmp_plane_record->GC_wf[stream_id] = tmp_plane_record->Get_a_free_block(stream_id, false);
							}
							else {
								tmp_plane_record->GC_wf[stream_id] = tmp_plane_record->Get_a_free_block_b(stream_id, false);
							}

							tmp_page_address.BlockID = tmp_plane_record->GC_wf[stream_id]->BlockID;
							Set_relief_status(tmp_page_address, bRelief);
						}
					}
				}
			}
		}


		page_address.BlockID = plane_record->GC_wf[stream_id]->BlockID;
		page_address.PageID = plane_record->GC_wf[stream_id]->Current_page_write_index;
		page_address.subPageID = plane_record->GC_wf[stream_id]->Current_subpage_write_index++;
		plane_record->Valid_subpages_count++;
		plane_record->Free_subpages_count--;
		if (plane_record->GC_wf[stream_id]->Current_subpage_write_index == ALIGN_UNIT_SIZE) {
			plane_record->Valid_pages_count++;
			plane_record->Free_pages_count--;
			plane_record->GC_wf[stream_id]->Current_page_write_index++;
			plane_record->GC_wf[stream_id]->Current_subpage_write_index = 0;
		}
		else {
			page_address.PageID = plane_record->GC_wf[stream_id]->Current_page_write_index;
		}

		////std::cout << "[debugGC1] pg: " << page_address.ChannelID << ", " << page_address.ChipID << ", " << page_address.DieID << ", " << page_address.PlaneID << ", " << page_address.BlockID << ", pg:" << page_address.PageID << ", " << page_address.subPageID << std::endl;
		plane_record->Check_bookkeeping_correctness(page_address);
	}

	void Flash_Block_Manager::Allocate_Pages_in_block_and_invalidate_remaining_for_preconditioning(const stream_id_type stream_id, const NVM::FlashMemory::Physical_Page_Address& plane_address, std::vector<NVM::FlashMemory::Physical_Page_Address>& page_addresses)
	{
#if PATCH_PRECOND
		if (stream_id == 255) {
			std::cout << "[DEBUG PRECOND] stream_id = 255" << std::endl;
			exit(1);
		}
		if (page_addresses.size() > pages_no_per_block * ALIGN_UNIT_SIZE) {
			PRINT_ERROR("Error while precondition a physical block: the size of the address list is larger than the pages_no_per_block!")
		}
#else
		if (page_addresses.size() > pages_no_per_block) {
			PRINT_ERROR("Error while precondition a physical block: the size of the address list is larger than the pages_no_per_block!")
		}
#endif
			
		PlaneBookKeepingType *plane_record = &plane_manager[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID];
		if (plane_record->Data_wf[stream_id]->Current_page_write_index > 0) {
			PRINT_ERROR("Illegal operation: the Allocate_Pages_in_block_and_invalidate_remaining_for_preconditioning function should be executed for an erased block!")
		}


#if PATCH_PRECOND
		//Assign (validate) physical addresses
		//std::cout << "page_addresses.size(): " << page_addresses.size() << std::endl; //it depends on preconditioning occupancy parameter 
#if PATCH_PRECOND
		for (int i = 0; i < (page_addresses.size()); i++) {
#else
		for (int i = 0; i < (page_addresses.size() / ALIGN_UNIT_SIZE) * ALIGN_UNIT_SIZE; i++) {
#endif
			plane_record->Valid_subpages_count++;
			plane_record->Free_subpages_count--;
			page_addresses[i].BlockID = plane_record->Data_wf[stream_id]->BlockID;
			page_addresses[i].subPageID = plane_record->Data_wf[stream_id]->Current_subpage_write_index++;
			page_addresses[i].PageID = plane_record->Data_wf[stream_id]->Current_page_write_index;
			if (plane_record->Data_wf[stream_id]->Current_subpage_write_index == ALIGN_UNIT_SIZE) {
				plane_record->Data_wf[stream_id]->Current_page_write_index++;
#if PATCH_PRECOND
				//if (plane_record->Data_wf[stream_id]->Current_page_write_index == pages_no_per_block) break;
#endif
				plane_record->Data_wf[stream_id]->Current_subpage_write_index = 0;
				plane_record->Valid_pages_count++;
				plane_record->Free_pages_count--;
			}
			else {
				page_addresses[i].PageID = plane_record->Data_wf[stream_id]->Current_page_write_index;
			}
			plane_record->Check_bookkeeping_correctness(page_addresses[i]);
#if PATCH_PRECOND
			Block_Pool_Slot_Type* block = &(plane_record->Blocks[page_addresses[i].BlockID]);
			//if (page_addresses[i].ChannelID == 2 && page_addresses[i].ChipID == 0 && page_addresses[i].DieID == 0 && page_addresses[i].PlaneID == 2 && page_addresses[i].BlockID == 0) {
				//std::cout << "[DEBUG PRECOND] validate 20020 block and pageid: " << page_addresses[i].PageID << ", " << page_addresses[i].subPageID << std::endl;
				//if (Is_Subpage_valid(block, page_addresses[i].PageID, page_addresses[i].subPageID)) {
					//std::cout << "[DEBUG PRECOND] validated well" << std::endl;
				//}
			//}
#endif
		}
#else
		//Assign physical addresses
		for (int i = 0; i < page_addresses.size(); i++) {
			plane_record->Valid_pages_count++;
			plane_record->Free_pages_count--;
			page_addresses[i].BlockID = plane_record->Data_wf[stream_id]->BlockID;
			page_addresses[i].PageID = plane_record->Data_wf[stream_id]->Current_page_write_index++;
			plane_record->Check_bookkeeping_correctness(page_addresses[i]);
		}
#endif


		//Invalidate the remaining pages in the block

#if PATCH_PRECOND
		NVM::FlashMemory::Physical_Page_Address target_address(plane_address);
		while (plane_record->Data_wf[stream_id]->Current_page_write_index < pages_no_per_block) {
#if PATCH_PRECOND
			for (int subpg = plane_record->Data_wf[stream_id]->Current_subpage_write_index; subpg < ALIGN_UNIT_SIZE; subpg++) {
#else
			for (int subpg = 0; subpg < ALIGN_UNIT_SIZE; subpg++) {
#endif
				plane_record->Free_subpages_count--;
				target_address.BlockID = plane_record->Data_wf[stream_id]->BlockID;
				target_address.PageID = plane_record->Data_wf[stream_id]->Current_page_write_index;
				target_address.subPageID = plane_record->Data_wf[stream_id]->Current_subpage_write_index++;
				if (plane_record->Data_wf[stream_id]->Current_subpage_write_index == ALIGN_UNIT_SIZE) {
					plane_record->Data_wf[stream_id]->Current_page_write_index++;
					plane_record->Data_wf[stream_id]->Current_subpage_write_index = 0;
					plane_record->Invalid_pages_count++;
					plane_record->Free_pages_count--;
				}
				Invalidate_subpage_in_block_for_preconditioning(stream_id, target_address);

#if PATCH_PRECOND
				Block_Pool_Slot_Type* block = &(plane_record->Blocks[target_address.BlockID]);
				//if (target_address.ChannelID == 2 && target_address.ChipID == 0 && target_address.DieID == 0 && target_address.PlaneID == 2 && target_address.BlockID == 0) {
					//if (!Is_Subpage_valid(block, target_address.PageID, target_address.subPageID)) {
						//std::cout << "[DEBUG PRECOND] Invalidated well" << std::endl;
					//}
				//}
#endif

				plane_record->Check_bookkeeping_correctness(plane_address);
				//std::cout << "[DEBUG PRECOND] Blk, Page, subPage: " << target_address.BlockID << ", " << target_address.PageID << ", " << target_address.subPageID << std::endl;
				//std::cout << "[DEBUG PRECOND] Invalid/valid/free pages: " << plane_record->Invalid_pages_count << ", " << plane_record->Valid_pages_count << ", " << plane_record->Free_pages_count << std::endl;
				//std::cout << "[DEBUG PRECOND] Invalid/valid/free subpages: " << plane_record->Invalid_subpages_count << ", " << plane_record->Valid_subpages_count << ", " << plane_record->Free_subpages_count << std::endl;
#if PATCH_PRECOND
				//if (target_address.ChannelID == 2 && target_address.ChipID == 0 && target_address.DieID == 0 && target_address.PlaneID == 2 && target_address.BlockID == 0) {
					//std::cout << "[DEBUG PRECOND] invalidated 20020 block and pageid: " << target_address.PageID << ", " << target_address.subPageID << std::endl;
				//}
#endif
			}
			}
#else
		NVM::FlashMemory::Physical_Page_Address target_address(plane_address);
		while (plane_record->Data_wf[stream_id]->Current_page_write_index < pages_no_per_block) {
			plane_record->Free_pages_count--;
			target_address.BlockID = plane_record->Data_wf[stream_id]->BlockID;
			target_address.PageID = plane_record->Data_wf[stream_id]->Current_page_write_index++;
			Invalidate_page_in_block_for_preconditioning(stream_id, target_address);
			plane_record->Check_bookkeeping_correctness(plane_address);
		}
#endif


		//Update the write frontier
		plane_record->Data_wf[stream_id] = plane_record->Get_a_free_block(stream_id, false);
	}

	void Flash_Block_Manager::Allocate_block_and_page_in_plane_for_translation_write(const stream_id_type streamID, NVM::FlashMemory::Physical_Page_Address& page_address, bool is_for_gc)
	{

		PlaneBookKeepingType* plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
		page_address.BlockID = plane_record->Translation_wf[streamID]->BlockID;
		page_address.subPageID = plane_record->Translation_wf[streamID]->Current_subpage_write_index++;
		page_address.PageID = plane_record->Translation_wf[streamID]->Current_page_write_index;
		plane_record->Valid_subpages_count++;
		plane_record->Free_subpages_count--;
		if (plane_record->Translation_wf[streamID]->Current_subpage_write_index == ALIGN_UNIT_SIZE) {
			plane_record->Translation_wf[streamID]->Current_subpage_write_index = 0;
			plane_record->Translation_wf[streamID]->Current_page_write_index++;
			plane_record->Valid_pages_count++;
			plane_record->Free_pages_count--;
		}
		else {
			page_address.PageID = plane_record->Translation_wf[streamID]->Current_page_write_index;
		}

		program_transaction_issued(page_address);

		//The current write frontier block for translation pages is written to the end
		if (plane_record->Translation_wf[streamID]->Current_page_write_index == pages_no_per_block) {
			//Assign a new write frontier block
			plane_record->Translation_wf[streamID] = plane_record->Get_a_free_block(streamID, true);
			if (!is_for_gc) {
			//	gc_and_wl_unit->Check_gc_required(plane_record->Get_free_block_pool_size(), page_address);
			}
		}
		plane_record->Check_bookkeeping_correctness(page_address);
	}

	inline void Flash_Block_Manager::Invalidate_subpage_in_block(const stream_id_type stream_id, const NVM::FlashMemory::Physical_Page_Address& page_address)
	{
		PlaneBookKeepingType* plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
		plane_record->Invalid_subpages_count++;
		plane_record->Valid_subpages_count--;

		if (plane_record->Blocks[page_address.BlockID].Stream_id != stream_id) {
			PRINT_MESSAGE("Page Info " << page_address.ChannelID << " " << page_address.ChipID << " " << page_address.DieID << " " << page_address.PlaneID << " " << page_address.BlockID << " " << page_address.PageID << ", subPg: "<<page_address.subPageID<< " Stream: " << plane_record->Blocks[page_address.BlockID].Stream_id);
			PRINT_ERROR("Inconsistent status in the Invalidate_subpage_in_block function! The accessed block is not allocated to stream " << stream_id)
		}

		plane_record->Blocks[page_address.BlockID].Invalid_subpage_count++;
		plane_record->Blocks[page_address.BlockID].Invalid_Subpage_bitmap[(page_address.PageID*ALIGN_UNIT_SIZE + page_address.subPageID) / 64] |= ((uint64_t)0x1) << (((page_address.PageID * ALIGN_UNIT_SIZE) + page_address.subPageID) % 64);

	}

	inline void Flash_Block_Manager::Invalidate_page_in_block(const stream_id_type stream_id, const NVM::FlashMemory::Physical_Page_Address& page_address)
	{
		PlaneBookKeepingType* plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
		plane_record->Invalid_pages_count++;
		plane_record->Valid_pages_count--;
		if (plane_record->Blocks[page_address.BlockID].Stream_id != stream_id) {
			PRINT_MESSAGE("Page Info " << page_address.ChannelID << " " << page_address.ChipID << " " << page_address.BlockID << " " << page_address.PageID << " S: " << plane_record->Blocks[page_address.BlockID].Stream_id);
			PRINT_ERROR("Inconsistent status in the Invalidate_page_in_block function! The accessed block is not allocated to stream " << stream_id)				
		}
		plane_record->Blocks[page_address.BlockID].Invalid_page_count++;
		plane_record->Blocks[page_address.BlockID].Invalid_page_bitmap[page_address.PageID / 64] |= ((uint64_t)0x1) << (page_address.PageID % 64);
	}

	inline void Flash_Block_Manager::Invalidate_page_in_block_for_preconditioning(const stream_id_type stream_id, const NVM::FlashMemory::Physical_Page_Address& page_address)
	{
		PlaneBookKeepingType* plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
		plane_record->Invalid_pages_count++;
		if (plane_record->Blocks[page_address.BlockID].Stream_id != stream_id) {
			PRINT_ERROR("Inconsistent status in the Invalidate_page_in_block (preconditioning) function! The accessed block is not allocated to stream " << stream_id)
		}
		plane_record->Blocks[page_address.BlockID].Invalid_page_count++;
		plane_record->Blocks[page_address.BlockID].Invalid_page_bitmap[page_address.PageID / 64] |= ((uint64_t)0x1) << (page_address.PageID % 64);
	}

	inline void Flash_Block_Manager::Invalidate_subpage_in_block_for_preconditioning(const stream_id_type stream_id, const NVM::FlashMemory::Physical_Page_Address& page_address)
	{
		PlaneBookKeepingType* plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
		plane_record->Invalid_subpages_count++;
		if (plane_record->Blocks[page_address.BlockID].Stream_id != stream_id) {
			PRINT_ERROR("Inconsistent status in the Invalidate_subpage_in_block (preconditioning) function! The accessed block is not allocated to stream " << stream_id)
		}
		plane_record->Blocks[page_address.BlockID].Invalid_subpage_count++;
		plane_record->Blocks[page_address.BlockID].Invalid_Subpage_bitmap[(page_address.PageID * ALIGN_UNIT_SIZE + page_address.subPageID) / 64] |= ((uint64_t)0x1) << (((page_address.PageID * ALIGN_UNIT_SIZE) + page_address.subPageID) % 64);
	
	
	}

	void Flash_Block_Manager::Add_erased_block_to_pool(const NVM::FlashMemory::Physical_Page_Address& block_address)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID];
		Block_Pool_Slot_Type* block = &(plane_record->Blocks[block_address.BlockID]);

		plane_record->Free_subpages_count += block->Invalid_subpage_count;
		plane_record->Invalid_subpages_count -= block->Invalid_subpage_count;

		Stats::Block_erase_histogram[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID][block->Erase_count]--;
		block->Erase();
		Stats::Block_erase_histogram[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID][block->Erase_count]++;
		plane_record->Add_to_free_block_pool(block, gc_and_wl_unit->Use_dynamic_wearleveling());
		//std::cout << "add_to_free_block_pool: " << block_address.ChannelID<<", "<< block_address.ChipID << ", " << block_address.DieID << ", " << block_address.PlaneID << ", " << block_address.BlockID << std::endl;
		plane_record->Check_bookkeeping_correctness(block_address);
	}

	inline unsigned int Flash_Block_Manager::Get_pool_size(const NVM::FlashMemory::Physical_Page_Address& plane_address)
	{
		return (unsigned int) plane_manager[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].Free_block_pool.size();
	}
}
