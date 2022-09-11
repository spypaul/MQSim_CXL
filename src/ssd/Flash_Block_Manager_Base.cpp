#include "Flash_Block_Manager.h"


namespace SSD_Components
{
	unsigned int Block_Pool_Slot_Type::Page_vector_size = 0;

	unsigned int Block_Pool_Slot_Type::SubPage_vector_size = 0;

	Flash_Block_Manager_Base::Flash_Block_Manager_Base(GC_and_WL_Unit_Base* gc_and_wl_unit, unsigned int max_allowed_block_erase_count, unsigned int total_concurrent_streams_no,
		unsigned int channel_count, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
		unsigned int block_no_per_plane, unsigned int page_no_per_block)
		: gc_and_wl_unit(gc_and_wl_unit), max_allowed_block_erase_count(max_allowed_block_erase_count), total_concurrent_streams_no(total_concurrent_streams_no),
		channel_count(channel_count), chip_no_per_channel(chip_no_per_channel), die_no_per_chip(die_no_per_chip), plane_no_per_die(plane_no_per_die),
		block_no_per_plane(block_no_per_plane), pages_no_per_block(page_no_per_block)
	{
		plane_manager = new PlaneBookKeepingType***[channel_count];
		for (unsigned int channelID = 0; channelID < channel_count; channelID++) {
			plane_manager[channelID] = new PlaneBookKeepingType**[chip_no_per_channel];
			for (unsigned int chipID = 0; chipID < chip_no_per_channel; chipID++) {
				plane_manager[channelID][chipID] = new PlaneBookKeepingType*[die_no_per_chip];
				for (unsigned int dieID = 0; dieID < die_no_per_chip; dieID++) {
					plane_manager[channelID][chipID][dieID] = new PlaneBookKeepingType[plane_no_per_die];

					//Initialize plane book keeping data structure
					for (unsigned int planeID = 0; planeID < plane_no_per_die; planeID++) {
						plane_manager[channelID][chipID][dieID][planeID].Total_pages_count = block_no_per_plane * pages_no_per_block;
						plane_manager[channelID][chipID][dieID][planeID].Free_pages_count = block_no_per_plane * pages_no_per_block;
						plane_manager[channelID][chipID][dieID][planeID].Valid_pages_count = 0;
						plane_manager[channelID][chipID][dieID][planeID].Invalid_pages_count = 0;

						plane_manager[channelID][chipID][dieID][planeID].Total_subpages_count = block_no_per_plane * pages_no_per_block*ALIGN_UNIT_SIZE;
						plane_manager[channelID][chipID][dieID][planeID].Free_subpages_count = block_no_per_plane * pages_no_per_block*ALIGN_UNIT_SIZE;
						plane_manager[channelID][chipID][dieID][planeID].Valid_subpages_count = 0;
						plane_manager[channelID][chipID][dieID][planeID].Invalid_subpages_count = 0;
						plane_manager[channelID][chipID][dieID][planeID].Ongoing_erase_operations.clear();
						plane_manager[channelID][chipID][dieID][planeID].Blocks = new Block_Pool_Slot_Type[block_no_per_plane];
						
						//Initialize block pool for plane
						for (unsigned int blockID = 0; blockID < block_no_per_plane; blockID++) {
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].BlockID = blockID;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Current_page_write_index = 0;

							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Current_subpage_write_index = 0;

							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Current_status = Block_Service_Status::IDLE;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Invalid_page_count = 0;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Erase_count = 0;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Holds_mapping_data = false;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Has_ongoing_gc_wl = false;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Erase_transaction = NULL;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Ongoing_user_program_count = 0;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Ongoing_user_read_count = 0;

							
							Block_Pool_Slot_Type::SubPage_vector_size = (pages_no_per_block*ALIGN_UNIT_SIZE) / (sizeof(uint64_t) * 8) + ((pages_no_per_block * ALIGN_UNIT_SIZE) % (sizeof(uint64_t) * 8) == 0 ? 0 : 1);
							//std::cout << "subpage vector size: " << Block_Pool_Slot_Type::SubPage_vector_size << std::endl;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Invalid_Subpage_bitmap = new uint64_t[Block_Pool_Slot_Type::SubPage_vector_size];
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].subProgram_bypass_bitmap = new uint64_t[Block_Pool_Slot_Type::SubPage_vector_size];
							for (unsigned int i = 0; i < Block_Pool_Slot_Type::SubPage_vector_size; i++) {
								plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Invalid_Subpage_bitmap[i] = All_VALID_PAGE;
								plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].subProgram_bypass_bitmap[i] = All_VALID_PAGE;
							}
							

							Block_Pool_Slot_Type::Page_vector_size = pages_no_per_block / (sizeof(uint64_t) * 8) + (pages_no_per_block % (sizeof(uint64_t) * 8) == 0 ? 0 : 1);
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Invalid_page_bitmap = new uint64_t[Block_Pool_Slot_Type::Page_vector_size];
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Program_bypass_bitmap = new uint64_t[Block_Pool_Slot_Type::Page_vector_size];							
							for (unsigned int i = 0; i < Block_Pool_Slot_Type::Page_vector_size; i++) {
								plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Invalid_page_bitmap[i] = All_VALID_PAGE;
								plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Program_bypass_bitmap[i] = All_VALID_PAGE;																
							}

							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Is_relieved = false;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Relief_count = 0;
							plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID].Relief_page_count = 0;							
							plane_manager[channelID][chipID][dieID][planeID].Add_to_free_block_pool(&plane_manager[channelID][chipID][dieID][planeID].Blocks[blockID], false);
						}
						plane_manager[channelID][chipID][dieID][planeID].Data_wf = new Block_Pool_Slot_Type*[total_concurrent_streams_no];
						plane_manager[channelID][chipID][dieID][planeID].Translation_wf = new Block_Pool_Slot_Type*[total_concurrent_streams_no];
						plane_manager[channelID][chipID][dieID][planeID].GC_wf = new Block_Pool_Slot_Type*[total_concurrent_streams_no];
						for (unsigned int stream_cntr = 0; stream_cntr < total_concurrent_streams_no; stream_cntr++) {
							plane_manager[channelID][chipID][dieID][planeID].Data_wf[stream_cntr] = plane_manager[channelID][chipID][dieID][planeID].Get_a_free_block(stream_cntr, false);
							plane_manager[channelID][chipID][dieID][planeID].Translation_wf[stream_cntr] = plane_manager[channelID][chipID][dieID][planeID].Get_a_free_block(stream_cntr, true);
							plane_manager[channelID][chipID][dieID][planeID].GC_wf[stream_cntr] = plane_manager[channelID][chipID][dieID][planeID].Get_a_free_block(stream_cntr, false);
						}
					}
				}
			}
		}

		// default setting for normal test... 0 relief proportion.
		Stats::Relief_proportion = (double)0/page_no_per_block; //channel_count*chip_no_per_channel*plane_no_per_die*block_no_per_plane*6; // 1 WL per block
		Stats::Physical_page_count = channel_count*chip_no_per_channel*plane_no_per_die*block_no_per_plane*page_no_per_block;
	}

	Flash_Block_Manager_Base::~Flash_Block_Manager_Base() 
	{
		for (unsigned int channel_id = 0; channel_id < channel_count; channel_id++) {
			for (unsigned int chip_id = 0; chip_id < chip_no_per_channel; chip_id++) {
				for (unsigned int die_id = 0; die_id < die_no_per_chip; die_id++) {
					for (unsigned int plane_id = 0; plane_id < plane_no_per_die; plane_id++) {
						for (unsigned int blockID = 0; blockID < block_no_per_plane; blockID++) {
							delete[] plane_manager[channel_id][chip_id][die_id][plane_id].Blocks[blockID].Invalid_page_bitmap;
							delete[] plane_manager[channel_id][chip_id][die_id][plane_id].Blocks[blockID].Program_bypass_bitmap;														
						}
						delete[] plane_manager[channel_id][chip_id][die_id][plane_id].Blocks;
						delete[] plane_manager[channel_id][chip_id][die_id][plane_id].GC_wf;
						delete[] plane_manager[channel_id][chip_id][die_id][plane_id].Data_wf;
						delete[] plane_manager[channel_id][chip_id][die_id][plane_id].Translation_wf;
					}
					delete[] plane_manager[channel_id][chip_id][die_id];
				}
				delete[] plane_manager[channel_id][chip_id];
			}
			delete[] plane_manager[channel_id];
		}
		delete[] plane_manager;
	}

	void Flash_Block_Manager_Base::Set_GC_and_WL_Unit(GC_and_WL_Unit_Base* gcwl)
	{
		this->gc_and_wl_unit = gcwl;
	}

	unsigned int Relief_page_per_blk[] = {0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,  48,51,54,57,60,63}; // 0 ~ 9 10~15
	uint64_t Bypass_bitmap[] = {0,0x0000000070,0x0000000770,0x0000007770,0x0000077770,0x0000777770,0x0007777770,0x0077777770,0x0777777770,0x7777777770, 
								0x0000077777777770,0x0000777777777770, 0x0007777777777770, 0x0077777777777770, 0x0777777777777770, 0x7777777777777770,
								0x7777777777777777,0x7777777777777fff,0x7777777777ffffff,							//16, 17, 18
								0x7777777fffffffff,0x7777ffffffffffff,0x7fffffffffffffff,							//19, 20, 21
								}; // 0 ~ 9 10~15

	void Block_Pool_Slot_Type::Erase()
	{
		Current_page_write_index = 0;

		Current_subpage_write_index = 0;
		Invalid_subpage_count = 0;

		Invalid_page_count = 0;
		Erase_count++;

		for (unsigned int i = 0; i < Block_Pool_Slot_Type::SubPage_vector_size; i++) {
			Invalid_Subpage_bitmap[i] = All_VALID_PAGE;
			//subProgram_bypass_bitmap[i] = All_VALID_PAGE;
		}

		Is_relieved = false;
#if 1		
		Relief_page_count = Relief_page_per_blk[Stats::Relief_type];				// for TEMPORAL TEST __FIXME__
		Program_bypass_bitmap[0] = Bypass_bitmap[Stats::Relief_type];	// for TEMPORAL TEST __FIXME__
#else
		Relief_page_count = 24;				// for TEMPORAL TEST __FIXME__
		Program_bypass_bitmap[0] = 0x777777770;	// for TEMPORAL TEST __FIXME__
#endif
		Stream_id = NO_STREAM;
		Holds_mapping_data = false;
		Erase_transaction = NULL;
	}

	Block_Pool_Slot_Type* PlaneBookKeepingType::Get_a_free_block(stream_id_type stream_id, bool for_mapping_data)
	{
		Block_Pool_Slot_Type* new_block = NULL;
		new_block = (*Free_block_pool.begin()).second;//Assign a new write frontier block
		if (Free_block_pool.size() == 0) {
			PRINT_ERROR("Requesting a free block from an empty pool!")
		}
		Free_block_pool.erase(Free_block_pool.begin());
		new_block->Stream_id = stream_id;
		new_block->Holds_mapping_data = for_mapping_data;
		Block_usage_history.push(new_block->BlockID);
		//std::cout << "Get_a_free_block: remain size(): " << Free_block_pool.size() << std::endl;
		return new_block;
	}

	Block_Pool_Slot_Type* PlaneBookKeepingType::Get_a_free_block_b(stream_id_type stream_id, bool for_mapping_data)
	{
		Block_Pool_Slot_Type* new_block = NULL;
		new_block = (*Free_block_pool.rbegin()).second;//Assign a new write frontier block
		if (Free_block_pool.size() == 0) {
			PRINT_ERROR("Requesting a free block from an empty pool!")
		}
		Free_block_pool.erase(std::next(Free_block_pool.rbegin()).base());
		new_block->Stream_id = stream_id;
		new_block->Holds_mapping_data = for_mapping_data;
		Block_usage_history.push(new_block->BlockID);
	
		return new_block;
	}
			
	void PlaneBookKeepingType::Check_bookkeeping_correctness(const NVM::FlashMemory::Physical_Page_Address& plane_address)
	{
		//std::cout << "free cnt: " << Free_subpages_count << ", v cnt: " << Valid_subpages_count << ", inV cnt: " << Invalid_subpages_count << std::endl;
		if (Total_subpages_count != Free_subpages_count + Valid_subpages_count + Invalid_subpages_count) {
			std::cout << "free cnt: " << Free_subpages_count << ", v cnt: " << Valid_subpages_count << ", inV cnt: " << Invalid_subpages_count << std::endl;
			PRINT_ERROR("Inconsistent status in the plane bookkeeping record!")
		}

		if (Free_subpages_count == 0) {
			PRINT_ERROR("Plane " << "@" << plane_address.ChannelID << "@" << plane_address.ChipID << "@" << plane_address.DieID << "@" << plane_address.PlaneID << " pool size: " << Get_free_block_pool_size() << " ran out of free pages! Bad resource management! It is not safe to continue simulation!");
		}
	}

	unsigned int PlaneBookKeepingType::Get_free_block_pool_size()
	{
		return (unsigned int)Free_block_pool.size();
	}

	void PlaneBookKeepingType::Add_to_free_block_pool(Block_Pool_Slot_Type* block, bool consider_dynamic_wl)
	{
		if (consider_dynamic_wl) {
			//std::pair<unsigned int, Block_Pool_Slot_Type*> entry(block->Erase_count, block);
			std::pair<unsigned int, Block_Pool_Slot_Type*> entry(block->Relief_count, block);
			Free_block_pool.insert(entry);
		} else {
			std::pair<unsigned int, Block_Pool_Slot_Type*> entry(0, block);
			Free_block_pool.insert(entry);
		}
	}

	unsigned int Flash_Block_Manager_Base::Get_min_max_erase_difference(const NVM::FlashMemory::Physical_Page_Address& plane_address)
	{
		unsigned int min_erased_block = 0;
		unsigned int max_erased_block = 0;
		PlaneBookKeepingType *plane_record = &plane_manager[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID];

		for (unsigned int i = 1; i < block_no_per_plane; i++) {
			if (plane_record->Blocks[i].Erase_count > plane_record->Blocks[max_erased_block].Erase_count) {
				max_erased_block = i;
			}
			if (plane_record->Blocks[i].Erase_count < plane_record->Blocks[min_erased_block].Erase_count) {
				min_erased_block = i;
			}
		}

		//return max_erased_block - min_erased_block;
		return plane_record->Blocks[max_erased_block].Erase_count - plane_record->Blocks[min_erased_block].Erase_count;
	}

	flash_block_ID_type Flash_Block_Manager_Base::Get_coldest_block_id(const NVM::FlashMemory::Physical_Page_Address& plane_address)
	{
		unsigned int min_erased_block = 0;
		PlaneBookKeepingType *plane_record = &plane_manager[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID];

		for (unsigned int i = 1; i < block_no_per_plane; i++) {
			if (plane_record->Blocks[i].Erase_count < plane_record->Blocks[min_erased_block].Erase_count) {
				min_erased_block = i;
			}
		}
		
		return min_erased_block;
	}

	PlaneBookKeepingType* Flash_Block_Manager_Base::Get_plane_bookkeeping_entry(const NVM::FlashMemory::Physical_Page_Address& plane_address)
	{
		return &(plane_manager[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID]);
	}

	bool Flash_Block_Manager_Base::Block_has_ongoing_gc_wl(const NVM::FlashMemory::Physical_Page_Address& block_address)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID];
		return plane_record->Blocks[block_address.BlockID].Has_ongoing_gc_wl;
	}
	
	bool Flash_Block_Manager_Base::Can_execute_gc_wl(const NVM::FlashMemory::Physical_Page_Address& block_address)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID];
		return (plane_record->Blocks[block_address.BlockID].Ongoing_user_program_count + plane_record->Blocks[block_address.BlockID].Ongoing_user_read_count == 0);
	}
	
	void Flash_Block_Manager_Base::GC_WL_started(const NVM::FlashMemory::Physical_Page_Address& block_address)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID];
		plane_record->Blocks[block_address.BlockID].Has_ongoing_gc_wl = true;
	}
	
	void Flash_Block_Manager_Base::program_transaction_issued(const NVM::FlashMemory::Physical_Page_Address& page_address)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
		plane_record->Blocks[page_address.BlockID].Ongoing_user_program_count++;
	}
	
	void Flash_Block_Manager_Base::Read_transaction_issued(const NVM::FlashMemory::Physical_Page_Address& page_address)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
		plane_record->Blocks[page_address.BlockID].Ongoing_user_read_count++;
	}

	void Flash_Block_Manager_Base::Program_transaction_serviced(const NVM::FlashMemory::Physical_Page_Address& page_address)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
		plane_record->Blocks[page_address.BlockID].Ongoing_user_program_count--;
	}

	void Flash_Block_Manager_Base::Read_transaction_serviced(const NVM::FlashMemory::Physical_Page_Address& page_address)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
		plane_record->Blocks[page_address.BlockID].Ongoing_user_read_count--;
	}
	
	bool Flash_Block_Manager_Base::Is_having_ongoing_program(const NVM::FlashMemory::Physical_Page_Address& block_address)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID];
		return plane_record->Blocks[block_address.BlockID].Ongoing_user_program_count > 0;
	}

	void Flash_Block_Manager_Base::GC_WL_finished(const NVM::FlashMemory::Physical_Page_Address& block_address)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID];
		plane_record->Blocks[block_address.BlockID].Has_ongoing_gc_wl = false;
	}
	
	bool Flash_Block_Manager_Base::Is_page_valid(Block_Pool_Slot_Type* block, flash_page_ID_type page_id)
	{
		if ((block->Invalid_page_bitmap[page_id / 64] & (((uint64_t)1) << page_id)) == 0) {
			return true;
		}
		return false;
	}

	bool Flash_Block_Manager_Base::Is_Subpage_valid(Block_Pool_Slot_Type* block, flash_page_ID_type page_id, flash_page_ID_type subpage_id)
	{

		//std::cout << "access idx of Invalid_Subpage_bitmap[]: " << "pg: "<<page_id<<", subpg: "<< subpage_id << std:: endl;
		if ((block->Invalid_Subpage_bitmap[(page_id*ALIGN_UNIT_SIZE + subpage_id) / 64] & (((uint64_t)1) << ((page_id * ALIGN_UNIT_SIZE) + subpage_id))) == 0) {
			return true;
		}
		return false;
	}

	bool Flash_Block_Manager_Base::Is_page_bypass(const NVM::FlashMemory::Physical_Page_Address& block_address)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID];		

		if ( (plane_record->Blocks[block_address.BlockID].Is_relieved == true) && 
			  ((plane_record->Blocks[block_address.BlockID].Program_bypass_bitmap[block_address.PageID / 64] & (((uint64_t)1) << block_address.PageID)) != 0) ) {
			return true;
		}
		return false;
	}

	void Flash_Block_Manager_Base::Set_relief_status(const NVM::FlashMemory::Physical_Page_Address& block_address, bool status)
	{
		PlaneBookKeepingType *plane_record = &plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID];		

		plane_record->Blocks[block_address.BlockID].Is_relieved = status;

		if (status == true){
			unsigned int prev_relief_count = plane_record->Blocks[block_address.BlockID].Relief_count;
			Stats::Relief_histogram[prev_relief_count]--;
			
			plane_record->Blocks[block_address.BlockID].Relief_count = prev_relief_count+1;
			Stats::Relief_histogram[prev_relief_count+1]++;

			if (prev_relief_count+1 > Stats::Max_relief_count)
			{
				Stats::Max_relief_count = prev_relief_count+1;
			}

			Stats::Relief_count++;
		}
	}
}
