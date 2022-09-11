#include <math.h>
#include <vector>
#include <set>
#include "GC_and_WL_Unit_Page_Level.h"
#include "Flash_Block_Manager.h"
#include "FTL.h"
#include <algorithm>
#include <windows.h>
bool GC_on_for_debug = false;
bool read_subpg_offset_reaches_end = false;
int total_gc_rw_interval_ER = 0; //Total made read/write transaction in interval between 'select vicitim block' and 'erase block'

extern unsigned int Total_page_movements_for_debug;
extern unsigned int Total_gc_executions_for_debug;


namespace SSD_Components
{	
	double		 WAF[10] = {0,};
	double		 WAI[10] = {0,};
	
	GC_and_WL_Unit_Page_Level::GC_and_WL_Unit_Page_Level(const sim_object_id_type& id,
		Address_Mapping_Unit_Base* address_mapping_unit, Flash_Block_Manager_Base* block_manager, TSU_Base* tsu, NVM_PHY_ONFI* flash_controller, 
		GC_Block_Selection_Policy_Type block_selection_policy, double gc_threshold, bool preemptible_gc_enabled, double gc_hard_threshold,
		unsigned int ChannelCount, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
		unsigned int block_no_per_plane, unsigned int Page_no_per_block, unsigned int sectors_per_page, 
		bool use_copyback, double rho, unsigned int max_ongoing_gc_reqs_per_plane, bool dynamic_wearleveling_enabled, bool static_wearleveling_enabled, unsigned int static_wearleveling_threshold, int seed)
		: GC_and_WL_Unit_Base(id, address_mapping_unit, block_manager, tsu, flash_controller, block_selection_policy, gc_threshold, preemptible_gc_enabled, gc_hard_threshold,
		ChannelCount, chip_no_per_channel, die_no_per_chip, plane_no_per_die, block_no_per_plane, Page_no_per_block, sectors_per_page, use_copyback, rho, max_ongoing_gc_reqs_per_plane, 
			dynamic_wearleveling_enabled, static_wearleveling_enabled, static_wearleveling_threshold, seed)
	{
		rga_set_size = (unsigned int)log2(block_no_per_plane);

		gc_status = GCStatus::IDLE;

		int sum = 0;
		for (int index = 0; index < weighted_moving_average_N; index++){
			weight[index] = index + 1;
			sum += index + 1;
		}

		for (int index = 0; index < weighted_moving_average_N; index++){
			weight[index] /= sum;
		}
		
		is_saturated = false;
		prev_saturation_status = false;
		cumulative_count = 0;
		WAF_transiaction_period = 0;
		saturation_start_index = 0;
		
		saturation_criteria_count = 5;
		saturation_criteria_delta = 0.02; //0.006
		minimum_mode_difference = ((double)256/(256-3) - 1) * 0.8; // Mode 0 <--> Mode 1 minimum difference		
	}
	
	bool GC_and_WL_Unit_Page_Level::GC_is_in_urgent_mode(const NVM::FlashMemory::Flash_Chip* chip)
	{
		return false;
		
		if (!preemptible_gc_enabled) {
			return true;
		}

		NVM::FlashMemory::Physical_Page_Address addr;
		addr.ChannelID = chip->ChannelID; addr.ChipID = chip->ChipID;
		for (unsigned int die_id = 0; die_id < die_no_per_chip; die_id++) {
			for (unsigned int plane_id = 0; plane_id < plane_no_per_die; plane_id++) {
				addr.DieID = die_id; addr.PlaneID = plane_id;
				if (block_manager->Get_pool_size(addr) < block_pool_gc_hard_threshold)
					return true;
			}
		}

		return false;
	}
	
	void GC_and_WL_Unit_Page_Level::Check_gc_required(const unsigned int free_block_pool_size, const NVM::FlashMemory::Physical_Page_Address& plane_address)
	{
		PlaneBookKeepingType* pbke = &(block_manager->plane_manager[0][0][0][0]);
		unsigned int free_block_count = pbke->Get_free_block_pool_size();
		if (pbke->Get_free_block_pool_size() < 2) {
			//std::cout << "[pages: " << pbke->Get_free_block_pool_size();// << ", id: " << pbke->Free_block_pool.begin()->second->BlockID << "] ";
		}
		if ((free_block_count > block_pool_gc_threshold) && (gc_status != GCStatus::WRITE_STATE)) {
			token = intial_token;
			return;
		}
		
		switch (gc_status)
		{
			case GCStatus::IDLE:
				if (new_victim_required == true)
				{
					if (gc_pending_read_count != 0) PRINT_ERROR("check read_pages()")
					if (gc_pending_erase_count != 0)
					{					
						break;
					}

					// restore victim block
					for (int victim_index = 0; victim_index < gc_unit_count; victim_index++){
						Block_Pool_Slot_Type* block = victim_blocks[victim_index];
						NVM::FlashMemory::Physical_Page_Address& address = gc_victim_address[victim_index];
						static int invalid_block_count = 0;
						if (block->Invalid_subpage_count != pages_no_per_block*ALIGN_UNIT_SIZE)
						{
							//std::cout << "INVALID: " << block->Invalid_subpage_count << " " << block->Current_page_write_index << " ";
							////PRINT_MESSAGE(++invalid_block_count << "   INVALID: " << block->Invalid_page_count << " "<< block->Current_page_write_index <<  " " << block->Stream_id);
							////PRINT_MESSAGE("   ch : " <<address.ChannelID<< " way: "<< address.ChipID << " plane : " << address.PlaneID << " block : " << address.BlockID << std::endl);
						}

						if (block->Is_relieved == true)
						{
							Stats::Cur_relief_page_count -= block->Relief_page_count;
						}

						pbke = block_manager->Get_plane_bookkeeping_entry(address);
						if (address.ChannelID == 0 && address.ChipID == 0 && address.PlaneID == 0 && address.BlockID == 247) {
							//std::cout << "[DEBUG GC-read_pages()] "block id: 247, erase block (pg movements should have been processed)" << std::endl;
						}
						pbke->Ongoing_erase_operations.erase(pbke->Ongoing_erase_operations.find(block->BlockID));
						block_manager->Add_erased_block_to_pool(address);
						block_manager->GC_WL_finished(address);

						//std::cout << "********************************************* Total page copies # for GC (in subpg): " << total_gc_rw_interval_ER << std::endl;
						total_gc_rw_interval_ER = 0;

						//if (check_static_wl_required(address)) {	// NEED to MODIFY
						//	run_static_wearleveling(address);
						//}

						Stats::Erase_count++;
						Stats::Total_gc_executions++;
						Total_gc_executions_for_debug = Stats::Total_gc_executions; //used in IO_Flow_Synthetic.cpp
					}

					Stats::Physical_write_count += gc_unit_count * pages_no_per_block;
					Stats::Physical_write_count_subpg += gc_unit_count * pages_no_per_block * ALIGN_UNIT_SIZE;
					Stats::Interval_Physical_write_count += gc_unit_count * pages_no_per_block;
				}

				
				//victim selection
				if (select_victim_block() == 0){
					// NOTHING TO DO. dummy read/write_pages call
				}
				
				new_victim_required = 0;

				gc_status = GCStatus::READ_STATE;
				//Stats::Total_gc_executions++;
				/** break through **/
				
			case GCStatus::READ_STATE:
				if (gc_pending_write_count >= gc_unit_count / 2) {
					break;
				}

				read_pages();
				gc_status = GCStatus::WRITE_STATE;
				/** break through **/

			case GCStatus::WRITE_STATE:
				if (gc_pending_read_count == 0){
					write_pages();
					
					Stats::Consecutive_gc_write++;

					if (new_victim_required == true){
						tsu->Prepare_for_transaction_submit();
						for (int victim_index = 0; victim_index < gc_unit_count; victim_index++){
							Block_Pool_Slot_Type* block = victim_blocks[victim_index];
							NVM::FlashMemory::Physical_Page_Address& address = gc_victim_address[victim_index];
													
							NVM_Transaction_Flash_ER* gc_erase_tr = new NVM_Transaction_Flash_ER(Transaction_Source_Type::GC_WL, block->Stream_id, address);
							tsu->Submit_transaction(gc_erase_tr);

							gc_pending_erase_count++;
						}							
						tsu->Schedule();						

						gc_status = GCStatus::IDLE;
					}
					else
					{
						gc_status = GCStatus::READ_STATE;
					}
				}
								
				break;
					
			default:
				break;
		}
	}

	bool cmp_gc(const NVM_Transaction* p1, const NVM_Transaction* p2) {
		if (((NVM_Transaction_Flash*)(p1))->PPA < ((NVM_Transaction_Flash*)(p2))->PPA) return true;
		else return false;
	}


	int GC_and_WL_Unit_Page_Level::write_pages()
	{
		int subwrite_count = waiting_writeback_transaction.size();
		NVM_Transaction_Flash_WR* write_tr;

		tsu->Prepare_for_transaction_submit();
		///* //flag0921
		if (waiting_writeback_transaction.size() != 0) {
			
			//combine and submit
			//Step 1. sort and combine
			stable_sort(waiting_writeback_transaction.begin(), waiting_writeback_transaction.end(), cmp_gc);

			int align_unit = ALIGN_UNIT_SIZE;
			bool stop_iterate = false;
			PPA_type bound_btm_PPA = 0;
			PPA_type bound_top_PPA = 0;

			if (waiting_writeback_transaction.size() > 1) {
				int end_PPA_of_arr = ((NVM_Transaction_Flash*)(waiting_writeback_transaction.back()))->PPA;
				for (std::list<NVM_Transaction*>::const_iterator it = waiting_writeback_transaction.begin(); it != waiting_writeback_transaction.end(); it++) {
					stop_iterate = false;
					bound_btm_PPA = (((NVM_Transaction_Flash*)(*it))->PPA) / align_unit; bound_btm_PPA *= align_unit;
					bound_top_PPA = bound_btm_PPA + align_unit;
					for (int i = 1; i < align_unit;) {
						int dist = (((NVM_Transaction_Flash*)(*(next(it, i))))->PPA) - (((NVM_Transaction_Flash*)(*it))->PPA);
						if (((NVM_Transaction_Flash*)(*(next(it, i))))->PPA == end_PPA_of_arr || abs(dist) >= align_unit) { stop_iterate = true; }
						if ((abs(dist) < align_unit) && ((((NVM_Transaction_Flash*)(*(next(it, i))))->PPA >= bound_btm_PPA) && (((NVM_Transaction_Flash*)(*(next(it, i))))->PPA < bound_top_PPA))) {
							((NVM_Transaction_Flash_WR*)(*it))->RelatedWrite_SUB.push_back(((NVM_Transaction_Flash_WR*)(*(next(it, i)))));
							//((NVM_Transaction_Flash_WR*)(*it))->RelatedWrite_SUB = ((NVM_Transaction_Flash_WR*)(*(next(it, i))));
							////std::cout << "subpage's CCDPBP: " << ((NVM_Transaction_Flash_WR*)(*it))->RelatedWrite_SUB->Address.ChannelID << "," << ((NVM_Transaction_Flash_WR*)(*it))->RelatedWrite_SUB->Address.ChipID << "," << ((NVM_Transaction_Flash_WR*)(*it))->RelatedWrite_SUB->Address.DieID << "," << ((NVM_Transaction_Flash_WR*)(*it))->RelatedWrite_SUB->Address.PlaneID << "," << ((NVM_Transaction_Flash_WR*)(*it))->RelatedWrite_SUB->Address.BlockID << "," << ((NVM_Transaction_Flash_WR*)(*it))->RelatedWrite_SUB->Address.PageID << std::endl;
							waiting_writeback_transaction.erase(next(it, i)); i--;
							//back_pressure_buffer_depth[0] -= count_sector_no_from_status_bitmap(((NVM_Transaction_Flash_WR*)(*it))->write_sectors_bitmap);
							//count--;
						}
						if (stop_iterate) break;
						i++;
					}
					//std::cout << std::endl;
					if (*it == waiting_writeback_transaction.back()) break;
					if (((NVM_Transaction_Flash*)(*(next(it, 1))))->PPA == end_PPA_of_arr) { break; }
				}
			}
			
		}
		//*///flag0921


		//Step 2. submit and clear
		while (waiting_writeback_transaction.size() != 0)
		{
			write_tr = (NVM_Transaction_Flash_WR*)(waiting_writeback_transaction.front());
			//std::cout << "write tr's ppa: " << write_tr->PPA << std::endl;
			//address_mapping_unit->Allocate_new_page_for_gc(write_tr, false);

			tsu->Submit_transaction(write_tr);

			waiting_writeback_transaction.pop_front();
		}

		tsu->Schedule();

		gc_pending_write_count += subwrite_count;

		token += token_per_write; //calculate for user write

		if (subwrite_count % (gc_unit_count) != 0) {
			// adjust unpaired pages.
			if (use_copyback == false) {
				NVM::FlashMemory::Physical_Page_Address addr;
				NVM_Transaction_Flash_WR* transaction = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, victim_blocks[0]->Stream_id, sector_no_per_page/ALIGN_UNIT_SIZE * SECTOR_SIZE_IN_BYTE,
					NO_LPA, NO_PPA, addr, NULL, 0, NULL, 0, INVALID_TIME_STAMP);
				//to make align (in plane parallelism level in GC-wf blocks)
				address_mapping_unit->Allocate_dummy_pages_for_gc(transaction, false);

				delete transaction;
			}
		}

		cur_state_index++;

		if (read_subpg_offset_reaches_end) {
			new_victim_required = true;
			read_subpg_offset_reaches_end = false;
		}


		return subwrite_count;
	}


	// Make gc_reads and gc_writes (into mapping granularity) and combine into NAND flash page granularity and submit
	int GC_and_WL_Unit_Page_Level::read_pages()
	{
		int read_count = 0;
		int read_count_subpg = 0;
		int cur_subpage_offset = 0;
		//bool stop_increase_page_offset = false;
		bool subpg_read = false;

		NVM_Transaction_Flash_RD* gc_read = NULL;
		NVM_Transaction_Flash_WR* gc_write = NULL;
		NVM::FlashMemory::Physical_Page_Address gc_candidate_address;
		Block_Pool_Slot_Type* block = NULL;

		tsu->Prepare_for_transaction_submit();

		//cur_page_offset, read_count = NAND FLASH page unit count, cur_subpage_offset = Mapping granularity unit count
		for (; cur_page_offset < pages_no_per_block; cur_page_offset++) {
			//parallel process across all planes in SSD. 
			//std::cout << "[DEBUG GC-read_pages()] cur_page_offset: " << cur_page_offset<< std::endl;
			for (int victim_index = 0; victim_index < gc_unit_count; victim_index++) {
				block = victim_blocks[victim_index];
				if (block->Stream_id != 0) {
					std::cout << "[DOODU ERROR] Stream_id ! =0 (read_pages()) " << std::endl;
					exit(1);
				}
#if DEBUG_GC_READ
				for (cur_subpage_offset; cur_subpage_offset < ALIGN_UNIT_SIZE; cur_subpage_offset++) {
#else
				for (cur_subpage_offset = 0; cur_subpage_offset < ALIGN_UNIT_SIZE; cur_subpage_offset++) {
#endif
					//std::cout << "[DEBUG GC-read_pages()] cur_page_offset: " << cur_page_offset << ", " << cur_subpage_offset << std::endl;
					//std::cout << "cur_subpage_offset: " << cur_subpage_offset << std::endl;

					if (block_manager->Is_Subpage_valid(block, cur_page_offset, cur_subpage_offset)) {
						gc_candidate_address = gc_victim_address[victim_index];
						gc_candidate_address.PageID = cur_page_offset;
						gc_candidate_address.subPageID = cur_subpage_offset;
						//std::cout << "[DEBUG GC-read_pages()] cur_page_offset: " << gc_candidate_address.ChannelID << ", " << gc_candidate_address.ChipID << ", " << gc_candidate_address.DieID << ", " << gc_candidate_address.PlaneID << ", " << gc_candidate_address.BlockID << ", pg" << gc_candidate_address.PageID << ", " << gc_candidate_address.subPageID << std::endl;
						//stop_increase_page_offset = true;

						Stats::Total_page_movements_for_gc++;
						Total_page_movements_for_debug = Stats::Total_page_movements_for_gc;

						if (use_copyback) {
							PRINT_ERROR("Check 4k mapping implementation when use copyback. Didn't consider copyback when implement 4k mapping scheme");
							gc_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
								NO_LPA, address_mapping_unit->Convert_address_to_ppa(gc_candidate_address), NULL, 0, NULL, 0, INVALID_TIME_STAMP);
							gc_write->ExecutionMode = WriteExecutionModeType::COPYBACK;
							tsu->Submit_transaction(gc_write);
						}
						else {
							//std::cout << "[DEBUG GC] gc read subpg ppa :" << address_mapping_unit->Convert_address_to_ppa(gc_candidate_address) << std::endl;
							gc_read = new NVM_Transaction_Flash_RD(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page / ALIGN_UNIT_SIZE * SECTOR_SIZE_IN_BYTE,
								NO_LPA, address_mapping_unit->Convert_address_to_ppa(gc_candidate_address), gc_candidate_address, NULL, 0, NULL, 0, INVALID_TIME_STAMP);
							gc_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page / ALIGN_UNIT_SIZE * SECTOR_SIZE_IN_BYTE,
								NO_LPA, NO_PPA, gc_candidate_address, NULL, 0, gc_read, 0, INVALID_TIME_STAMP);
							gc_write->ExecutionMode = WriteExecutionModeType::SIMPLE;
							gc_write->RelatedErase = NULL; //gc_erase_tr;
							gc_read->RelatedWrite = gc_write;
							waiting_submit_transaction.push_back(gc_read);
							total_gc_rw_interval_ER++;

							if (gc_read->Stream_id != 0) {
								std::cout << "[DOODU ERROR] Stream_id (read_pages()): " << gc_read->Stream_id << std::endl;
								exit(1);
							}
						}
						read_count_subpg++;
#if DEBUG_GC_READ
						if ((read_count_subpg == gc_unit_count * ALIGN_UNIT_SIZE)) {
							break;
						}
#endif
					}
#if DEBUG_GC_READ
					else {
						gc_candidate_address = gc_victim_address[victim_index];
						gc_candidate_address.PageID = cur_page_offset;
						gc_candidate_address.subPageID = cur_subpage_offset;
						//if (gc_candidate_address.ChannelID == 2 && gc_candidate_address.ChipID == 0 && gc_candidate_address.DieID == 0 && gc_candidate_address.PlaneID == 2 && gc_candidate_address.BlockID == 0) {
							////std::cout << "[DEBUG PRECOND] (invalid) gc read subpg ppa :" << address_mapping_unit->Convert_address_to_ppa(gc_candidate_address) << ", address(block): " << gc_candidate_address.ChannelID << gc_candidate_address.ChipID << gc_candidate_address.DieID << gc_candidate_address.PlaneID << gc_candidate_address.BlockID << ", page: " << gc_candidate_address.PageID << ", " << gc_candidate_address.subPageID << std::endl;
						//}
					}
#endif
				}// check subpgs in a page

#if DEBUG_GC_READ
				if (cur_subpage_offset == ALIGN_UNIT_SIZE) {
					cur_subpage_offset = 0;
				}
#endif

				if ((read_count_subpg == gc_unit_count * ALIGN_UNIT_SIZE)) {
					break;
				}

			}// check in a parallelism blocks (= gc_unit_count)

			if ((read_count_subpg == gc_unit_count * ALIGN_UNIT_SIZE)) {
				break;
			}

		} //end of for (; cur_page_offset ; ;)


		if (cur_page_offset == pages_no_per_block && cur_subpage_offset == ALIGN_UNIT_SIZE) {
			read_subpg_offset_reaches_end = true;
		}

		int align_unit = ALIGN_UNIT_SIZE;
		bool stop_iterate = false;
		PPA_type bound_btm_PPA = 0;
		PPA_type bound_top_PPA = 0;

		//combine and submit
		//Step 1. sort and combine
		int front_idx = 0;
		//std::cout << "[before] waiting_submit_transaction: " << waiting_submit_transaction.size() << std::endl;

#if try_combine_GCread
		stable_sort(waiting_submit_transaction.begin(), waiting_submit_transaction.end(), cmp_gc);

		if (waiting_submit_transaction.size() > 1) {
			for (std::list<NVM_Transaction*>::const_iterator it = waiting_submit_transaction.begin(); it != waiting_submit_transaction.end(); it++) {


				if (it == waiting_submit_transaction.end()) break;
				stop_iterate = false;
				for (int idx = 1; idx < (waiting_submit_transaction.size() - (front_idx));) {
					if ((next(it, idx)) == waiting_submit_transaction.end()) { stop_iterate = true; };

					if (((NVM_Transaction_Flash*)(*(next(it, idx))))->Address.ChannelID == ((NVM_Transaction_Flash*)(*it))->Address.ChannelID
						&& ((NVM_Transaction_Flash*)(*(next(it, idx))))->Address.ChipID == ((NVM_Transaction_Flash*)(*it))->Address.ChipID
						&& ((NVM_Transaction_Flash*)(*(next(it, idx))))->Address.DieID == ((NVM_Transaction_Flash*)(*it))->Address.DieID
						&& ((NVM_Transaction_Flash*)(*(next(it, idx))))->Address.PlaneID == ((NVM_Transaction_Flash*)(*it))->Address.PlaneID) {

						((NVM_Transaction_Flash_RD*)(*it))->RelatedRead_SUB.push_back(((NVM_Transaction_Flash_RD*)(*(next(it, idx)))));
						waiting_submit_transaction.erase(next(it, idx));
						idx--;
					}
					if (stop_iterate) break;
					idx++;
				}
				front_idx++;
			}
		}
#endif

		//Step 2. Submit and clear waiting_submit_transaction

		if (waiting_submit_transaction.size() > gc_unit_count) {
			std::cout << "cannot warrant that subpgs (combine targets) are in ALIGN_UNIT_SIZE boundary. so waiting_submit_transaction.size() can bigger than gc_unit_count." << std::endl;
			//exit(1); //flag1031
		}

		for (std::list<NVM_Transaction*>::const_iterator itr = waiting_submit_transaction.begin(); itr != waiting_submit_transaction.end(); itr++) {
			tsu->Submit_transaction(((NVM_Transaction_Flash*)(*itr)));
		}
		for (std::list<NVM_Transaction*>::const_iterator itr = waiting_submit_transaction.begin(); itr != waiting_submit_transaction.end(); itr++) {
			waiting_submit_transaction.pop_front();
		}

		tsu->Schedule();

		//std::cout << "[DEBUG GC] scheduled GC cur_page_offset: " << cur_page_offset << std::endl;
		gc_pending_read_count = read_count_subpg;


		if (cur_page_offset == pages_no_per_block || (read_count_subpg != gc_unit_count*ALIGN_UNIT_SIZE)) {
			new_victim_required = true;
		}

		return read_count;
	}

	bool GC_and_WL_Unit_Page_Level::has_valid_subpg(flash_channel_ID_type ChannelID, flash_chip_ID_type ChipID, flash_die_ID_type DieID, flash_plane_ID_type PlaneID, flash_block_ID_type block_id, flash_page_ID_type page_id ) {
	
		bool has_valid_subpg_in_pg = false;
		PlaneBookKeepingType* pbke;
		NVM::FlashMemory::Physical_Page_Address plane_address;
		plane_address.ChannelID = ChannelID;
		plane_address.ChipID = ChipID;
		plane_address.DieID = DieID;
		plane_address.PlaneID = PlaneID;
		plane_address.BlockID = block_id;
		pbke = block_manager->Get_plane_bookkeeping_entry(plane_address);
		
		for (int idx = 0; idx < ALIGN_UNIT_SIZE; idx++) {
			if (pbke->Blocks[block_id].Invalid_Subpage_bitmap[(page_id * ALIGN_UNIT_SIZE + idx) / 64] & (((uint64_t)0x1) << (((page_id * ALIGN_UNIT_SIZE) + idx) % 64))) {
				//do nothing
			}
			else {
				has_valid_subpg_in_pg = true;
				return has_valid_subpg_in_pg;
			}
		}
		return has_valid_subpg_in_pg;
	}

	int GC_and_WL_Unit_Page_Level::count_valid_pages_in_block(const NVM::FlashMemory::Physical_Page_Address& block_address, int page_no_per_block) {
		int count_valid_pages_in_blk = 0;
		for (int pg_idx = 0; pg_idx < page_no_per_block; pg_idx++) {
			if (has_valid_subpg(block_address.ChannelID, block_address.ChipID, block_address.DieID, block_address.PlaneID, block_address.BlockID, pg_idx)) {
				count_valid_pages_in_blk++;
			}
		}
		//////std::cout << "[DEBUG] count_valid_pages_in_blk: " << count_valid_pages_in_blk << std::endl;
		return count_valid_pages_in_blk;
	}

	int GC_and_WL_Unit_Page_Level::get_valid_subpage_count(flash_block_ID_type block_id)
	{

		PlaneBookKeepingType* pbke;
		NVM::FlashMemory::Physical_Page_Address plane_address;
		unsigned int total_valid_subpage_count = 0;

		for (unsigned int plane_idx = 0; plane_idx < plane_no_per_die; plane_idx++) {
			plane_address.PlaneID = plane_idx;

			for (unsigned int die_idx = 0; die_idx < die_no_per_chip; die_idx++) {
				plane_address.DieID = die_idx;

				for (unsigned int chip_idx = 0; chip_idx < chip_no_per_channel; chip_idx++) {
					plane_address.ChipID = chip_idx;

					for (unsigned int ch_idx = 0; ch_idx < channel_count; ch_idx++) {
						plane_address.ChannelID = ch_idx;

						pbke = block_manager->Get_plane_bookkeeping_entry(plane_address);
						//total_valid_page_count += (pbke->Blocks[block_id].Current_page_write_index - pbke->Blocks[block_id].Invalid_page_count);
						//to make accurate, have to consider subpage's Current_subpage_write_index. but approximate it.
						total_valid_subpage_count += ((pbke->Blocks[block_id].Current_page_write_index * ALIGN_UNIT_SIZE + pbke->Blocks[block_id].Current_subpage_write_index)  - pbke->Blocks[block_id].Invalid_subpage_count);
					}
				}
			}
		}
		return total_valid_subpage_count;

	}


	int GC_and_WL_Unit_Page_Level::get_valid_page_count(flash_block_ID_type block_id)
	{
		PlaneBookKeepingType* pbke;
		NVM::FlashMemory::Physical_Page_Address plane_address;
		unsigned int total_valid_page_count = 0;

		for (unsigned int plane_idx = 0; plane_idx < plane_no_per_die; plane_idx++){
			plane_address.PlaneID = plane_idx;

			for (unsigned int die_idx = 0; die_idx < die_no_per_chip; die_idx++){
				plane_address.DieID = die_idx;

				for (unsigned int chip_idx = 0; chip_idx < chip_no_per_channel; chip_idx++){
					plane_address.ChipID = chip_idx;

					for (unsigned int ch_idx = 0; ch_idx < channel_count; ch_idx++){
						plane_address.ChannelID = ch_idx;

						pbke = block_manager->Get_plane_bookkeeping_entry(plane_address);
						total_valid_page_count += (pbke->Blocks[block_id].Current_page_write_index - pbke->Blocks[block_id].Invalid_page_count);
					}
				}
			}
		}
		
		return total_valid_page_count;
	}
	double g_diff = 0;
	int GC_and_WL_Unit_Page_Level::select_victim_block()
	{
		NVM::FlashMemory::Physical_Page_Address plane_address;
		NVM::FlashMemory::Physical_Page_Address plane_address_tmp;
		int victim_count = 0;
		int valid_page_count = 0; //mapping granularity
		int valid_page_count_nand_granu = 0; //nand flash page granularity

		flash_block_ID_type gc_candidate_block_id = block_manager->Get_coldest_block_id(plane_address);
		PlaneBookKeepingType* pbke = block_manager->Get_plane_bookkeeping_entry(plane_address);

		unsigned int total_pages_count = 0;
		unsigned int valid_pages_count = 0;
		if (block_selection_policy != GC_Block_Selection_Policy_Type::GREEDY) {
			PRINT_ERROR("block_selection_policy should be GREEDY ")
		}
		switch (block_selection_policy) {
			case SSD_Components::GC_Block_Selection_Policy_Type::GREEDY://Find the set of blocks with maximum number of invalid pages and no free pages
			{
				unsigned int min_valid_page_count = 0x7fffffff;
				unsigned int cur_valid_subpage_count;
									
				gc_candidate_block_id = 0; //
				
				for (flash_block_ID_type block_id = 0; block_id < block_no_per_plane; block_id++) {
					cur_valid_subpage_count = get_valid_subpage_count(block_id);
					//std::cout << "[DEBUG GC] blk id:"<<block_id<<", Valid subpg_cnt: " << cur_valid_subpage_count << ", Write idx: " << pbke->Blocks[block_id].Current_page_write_index << std::endl;

					if ((cur_valid_subpage_count < min_valid_page_count) 
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block)
						&& is_safe_gc_wl_candidate(pbke, block_id)) {
						gc_candidate_block_id = block_id;
						min_valid_page_count = cur_valid_subpage_count;
					}
				}
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RGA:
			{				
				unsigned int min_valid_page_count;
				unsigned int cur_valid_page_count;
				std::set<flash_block_ID_type> random_set;
				while (random_set.size() < rga_set_size) {
					flash_block_ID_type block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
					if (pbke->Ongoing_erase_operations.find(block_id) == pbke->Ongoing_erase_operations.end()
						&& is_safe_gc_wl_candidate(pbke, block_id)) {
						random_set.insert(block_id);
						}
				}
				gc_candidate_block_id = *random_set.begin();
				min_valid_page_count = get_valid_page_count(gc_candidate_block_id);
				for(auto &block_id : random_set) {
					cur_valid_page_count = get_valid_page_count(block_id);
					if (min_valid_page_count > cur_valid_page_count
						&& pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) {
						gc_candidate_block_id = block_id;
					}
				}
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM:
			{
				gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				unsigned int repeat = 0;
		
				//A write frontier block should not be selected for garbage collection
				while (!is_safe_gc_wl_candidate(pbke, gc_candidate_block_id) && repeat++ < block_no_per_plane) {
					gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				}
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM_P:
			{
				gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				unsigned int repeat = 0;
		
				//A write frontier block or a block with free pages should not be selected for garbage collection
				while ((pbke->Blocks[gc_candidate_block_id].Current_page_write_index < pages_no_per_block || !is_safe_gc_wl_candidate(pbke, gc_candidate_block_id))
					&& repeat++ < block_no_per_plane) {
					gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				}
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM_PP:
			{
				gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				unsigned int repeat = 0;
		
				//The selected gc block should have a minimum number of invalid pages
				while ((pbke->Blocks[gc_candidate_block_id].Current_page_write_index < pages_no_per_block 
					|| pbke->Blocks[gc_candidate_block_id].Invalid_page_count < random_pp_threshold
					|| !is_safe_gc_wl_candidate(pbke, gc_candidate_block_id))
					&& repeat++ < block_no_per_plane) {
					gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				}
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::FIFO:
				gc_candidate_block_id = pbke->Block_usage_history.front();
				pbke->Block_usage_history.pop();
				break;
			default:
				break;
		}
		
		//This should never happen, but we check it here for safty
		if (pbke->Ongoing_erase_operations.find(gc_candidate_block_id) != pbke->Ongoing_erase_operations.end()) {
			PRINT_ERROR("something wrong !! GC");
		}
		
		for (unsigned int plane_idx = 0; plane_idx < plane_no_per_die; plane_idx++){			
			plane_address.PlaneID = plane_idx;
			
			for (unsigned int die_idx = 0; die_idx < die_no_per_chip; die_idx++){
				plane_address.DieID = die_idx;				

				for (unsigned int chip_idx = 0; chip_idx < chip_no_per_channel; chip_idx++){
					plane_address.ChipID = chip_idx;
					
					for (unsigned int ch_idx = 0; ch_idx < channel_count; ch_idx++){
						plane_address.ChannelID = ch_idx;

						NVM::FlashMemory::Physical_Page_Address gc_candidate_address(plane_address);
						gc_candidate_address.BlockID = gc_candidate_block_id;
						gc_victim_address[victim_count] = gc_candidate_address;

						pbke = block_manager->Get_plane_bookkeeping_entry(plane_address);
						Block_Pool_Slot_Type* block = &pbke->Blocks[gc_candidate_block_id];
						victim_blocks[victim_count++] = block;
						///std::cout << "[DEBUG GC] gc candidate id: " << gc_candidate_block_id<<", Cha,Chip,die,plane: "<<ch_idx<<", "<< chip_idx << ", "<<die_idx << ", " <<plane_idx  << std::endl;
						if (block->Stream_id != 0) {
							std::cout << "[ERROR] block->Stream_id ! =0 (select_victim_block()), block_id: "<< gc_candidate_block_id << std::endl;
							exit(1);
						}

						//valid_page_count += ((block->Current_page_write_index*ALIGN_UNIT_SIZE + block->Current_subpage_write_index) - (block->Invalid_subpage_count));
						valid_page_count += ((block->Current_page_write_index * ALIGN_UNIT_SIZE) - (block->Invalid_subpage_count));
						//////std::cout << "[DEBUG] subpg index: " << (block->Current_page_write_index * ALIGN_UNIT_SIZE) << ", invalid subpgs cnt: "<< (block->Invalid_subpage_count) <<std::endl;
						//std::cout << "valid_page_count: " << valid_page_count << std::endl;
						int count_valid_pages_in_block_tmp = count_valid_pages_in_block(gc_candidate_address, pages_no_per_block);
						valid_page_count_nand_granu += count_valid_pages_in_block_tmp;
						////std::cout << "[DEBUG]count_valid_pages_in_block_tmp: " << count_valid_pages_in_block_tmp << std::endl;
						////std::cout << "[DEBUG] valid_page_count_nand_granu: " << valid_page_count_nand_granu << std::endl;
						//valid_page_count += (block->Current_page_write_index - block->Invalid_page_count);

								
						//Run the state machine to protect against race condition
						block_manager->GC_WL_started(gc_candidate_address);
						pbke->Ongoing_erase_operations.insert(gc_candidate_block_id);

						if (victim_count > 1)
						{
							if (victim_blocks[victim_count-1]->Stream_id != victim_blocks[victim_count-2]->Stream_id)
							{
								PRINT_MESSAGE("Wrong Error......"<< victim_count-1 <<" "<< victim_blocks[victim_count-1]->Stream_id << "  "<< victim_blocks[victim_count-2]->Stream_id);
								PRINT_MESSAGE("Block Id "<< victim_count-1 <<" "<< victim_blocks[victim_count-1]->BlockID << "  "<< victim_blocks[victim_count-2]->BlockID);								
							}
						}

						total_pages_count += pbke->Total_pages_count;
						valid_pages_count += pbke->Valid_pages_count;						
					}
				}
			}
		}

		Stats::Utilization = (double)valid_pages_count / total_pages_count; // informaive

		Stats::GC_count++;
		if (Stats::GC_count % 100 == 0)
		{
			Stats::WAF[Stats::WAF_index] = (double)( (Stats::Physical_write_count - Stats::Prev_physical_write_count) - (Stats::Relief_page_count - Stats::Prev_relief_page_count))/(Stats::Host_write_count - Stats::Prev_host_write_count);
			Stats::WAI[Stats::WAF_index] = (double)(Stats::Physical_write_count - Stats::Prev_physical_write_count)/(Stats::Host_write_count - Stats::Prev_host_write_count);
			Stats::Reliefed[Stats::WAF_index] = (double)Stats::Cur_relief_page_count/Stats::Physical_page_count;

			WAI[Stats::WAF_index % 10] = Stats::WAI[Stats::WAF_index];
			WAF[Stats::WAF_index % 10] = Stats::WAF[Stats::WAF_index];

			Stats::Prev_physical_write_count = Stats::Physical_write_count;
			Stats::Prev_host_write_count = Stats::Host_write_count;
			Stats::Prev_relief_page_count = Stats::Relief_page_count;
			Stats::WAF_index++;

			//PRINT_MESSAGE(" WAF : " << WAF[0] <<" " << WAF[1] <<" " << WAF[2]<<" " << WAF[3]<<" " << WAF[4]<<" " << WAF[5]<<" " << WAF[6]<<" " << WAF[7]<<" " << WAF[8] <<" " << WAF[9]);
			//PRINT_MESSAGE(" WAI : " << WAI[0] <<" " << WAI[1] <<" " << WAI[2]<<" " << WAI[3]<<" " << WAI[4]<<" " << WAI[5]<<" " << WAI[6]<<" " << WAI[7]<<" " << WAI[8] <<" " << WAI[9]);
			
			//PRINT_MESSAGE("WAF index  " << Stats::WAF_index << " Is saturated " << is_WAF_saturated(g_diff) << " diff " << g_diff);

			//select_relief_mode();
		}

		cur_page_offset = 0;
		cur_state_index = 0;

		//int roundup_valid = ((valid_page_count/ALIGN_UNIT_SIZE + gc_unit_count - 1) / gc_unit_count) * gc_unit_count; //fix it. not all pg has ALIGN_UNIT_SIZE valid subpgs.
		//std::cout << "valid_page_count_nand_granu: " << valid_page_count_nand_granu << std::endl;
		GC_on_for_debug = true;
		//std::cout << "will make free subpgs in gc_unit_count: " << pages_no_per_block* gc_unit_count*ALIGN_UNIT_SIZE - valid_page_count << std::endl;
		int roundup_valid = ((valid_page_count_nand_granu + gc_unit_count - 1) / gc_unit_count) * gc_unit_count;
		//////std::cout << "valid_page_count_nand_granu: " << valid_page_count_nand_granu << std::endl;
		//////std::cout << "valid_page_count: " << valid_page_count << std::endl;

		if (roundup_valid  != 0){
			token_per_write = (((double)gc_unit_count * (gc_unit_count * pages_no_per_block - (roundup_valid)) / (roundup_valid))); 
			state_repeat_count = roundup_valid / gc_unit_count;
			//std::cout << "1token_per_write: " << token_per_write << std::endl;
		} else {
			token_per_write = gc_unit_count * (pages_no_per_block-1);
			state_repeat_count = 1;
			//std::cout << "2token_per_write: " << token_per_write << std::endl;
		}
		
		unsigned int free_block_count = pbke->Get_free_block_pool_size();
		if (free_block_count <= block_pool_gc_threshold)
		{
			double reduced_factor = ((double)block_pool_gc_threshold - free_block_count -1)/(block_pool_gc_threshold * 4); // upto 12.5% reduced..
			token_per_write = token_per_write * (1-reduced_factor);
		}

		if ((Stats::GC_count % 100) == 0)
		{
			PRINT_MESSAGE(Stats::GC_count << " New Victim: " << gc_candidate_block_id << "  stream : " <<victim_blocks[0]->Stream_id << "  valid : " << valid_page_count << " cur_token : " << token_per_write << " free blk : " << free_block_count << " c_token : " << token << " U: " << Stats::Utilization  << " W: " << WAF[(Stats::WAF_index-1) % 10]); 
		}

		return valid_page_count;
	}

	bool GC_and_WL_Unit_Page_Level::is_WAF_saturated(double& diff)
	{
		bool saturated = false;

		if ((Stats::WAF_index < weighted_moving_average_N*2) || (Stats::WAF_index < WAF_transiaction_period)){
		//if ((Stats::WAF_index < 500) || (Stats::WAF_index < WAF_transiaction_period)){
			is_saturated = false;
			return is_saturated;
		}

		double avg_WAI0 = 0;
		double avg_WAI1 = 0;

		for (int index = 0; index < weighted_moving_average_N; index++)
		{
			int old_offs = index + Stats::WAF_index - weighted_moving_average_N*2;
			int new_offs = index + Stats::WAF_index - weighted_moving_average_N;
		
			avg_WAI0 += Stats::WAI[old_offs] * weight[index];
			avg_WAI1 += Stats::WAI[new_offs] * weight[index];
		}
		
		g_diff = avg_WAI1/avg_WAI0 - 1;

		if ((g_diff < saturation_criteria_delta) && (g_diff > -saturation_criteria_delta))
		{
			saturated = true;
		
			if (cumulative_count < 0) {
				cumulative_count = 0;
			}

			if (prev_saturation_status == true){
				cumulative_count++;
			
				if ((is_saturated == false) && (cumulative_count >= saturation_criteria_count))
				{
					is_saturated = true;

					saturation_start_index = Stats::WAF_index - saturation_criteria_count;
				}
			}
		}
		else{
			if (cumulative_count > 0){
				cumulative_count = 0;
			}
			else {
				cumulative_count--;
			}

			if (cumulative_count*-1 >= saturation_criteria_count)
			{
				is_saturated = false;
			}
		}

		prev_saturation_status = saturated;

		return is_saturated;		
	}



	void GC_and_WL_Unit_Page_Level::select_relief_mode()
	{
		static int cur_relief_mode = 0;
		static int cur_saturation_count = 0;
		static int cur_abnormal_cnt = 0;
		static int pre_abnormal_WAF = 0;
		
		static int NAND_Endurance[] = {1000, 1100, 1150, 1180, 1200, 1210}; // Relief mode 1 +10%, Relief mode 2 +15%, Relief mode 3 - 18%...
		static bool WAF_increased = false;
		static int mode_start_index = 0;


		bool WAF_transition  = false;
		int next_relief_mode = cur_relief_mode;
		double WAF_delta;
		if (is_saturated == false)
		{
			return;
		}

		if (cur_saturation_count == 0){
			mode_start_index = saturation_start_index;
		}
		cur_saturation_count++;

		//update cur WAF
		double average_WAF = 0;
		for (int index = saturation_start_index; index < Stats::WAF_index; index++){
			average_WAF += Stats::WAI[index];
		}
		average_WAF /= (Stats::WAF_index - saturation_start_index);
							
		// 1. WAF transition check between diff relief mode
		if ((cur_relief_mode+1 < MAX_RELIEF_MODE) && (Stats::Relief_WAF_Table[cur_relief_mode+1] != 0)){
			WAF_delta = Stats::Relief_WAF_Table[cur_relief_mode+1]/average_WAF;
			if (WAF_delta -1 < minimum_mode_difference){
				if (cur_saturation_count == 1){ 			
					// after mode change. 
					PRINT_MESSAGE(" --ignore first transition between diff relief mode: cur: " << cur_relief_mode << " next : "<< cur_relief_mode+1 << "  WAF " << average_WAF);
					
					//check WAF one more for new interval					
					WAF_transiaction_period = Stats::WAF_index + 5;  // 5 or 10?
					return;
				} else {
					WAF_transition = true;	
					PRINT_MESSAGE(" --WAF TRANSITION between diff relief mode: cur: " << cur_relief_mode << " next : "<< cur_relief_mode+1 << "  WAF " << average_WAF);
					
					WAF_increased = true;
				}
			}
		} 
	
		// 2. WAF transition check between diff relief mode
		if ((cur_relief_mode != 0) && (Stats::Relief_WAF_Table[cur_relief_mode-1] != 0)){
			WAF_delta = average_WAF/Stats::Relief_WAF_Table[cur_relief_mode-1];
			if (WAF_delta -1 < minimum_mode_difference){
				if (cur_saturation_count == 1){ 			
					// after mode change. 
					PRINT_MESSAGE(" --ignore first transition between diff relief mode: cur: " << cur_relief_mode << " prev : "<< cur_relief_mode-1 << "  WAF " << average_WAF);
					
					//check WAF one more for new interval					
					WAF_transiaction_period = Stats::WAF_index + 5;  // 5 or 10?
					return;
				} else {
					WAF_transition = true;	
					PRINT_MESSAGE(" --WAF TRANSITION between diff relief mode: cur: " << cur_relief_mode << " prev : "<< cur_relief_mode-1<< "  WAF " << average_WAF);
					WAF_increased = false;
				}				
			}
		}


		if (Stats::Relief_WAF_Table[cur_relief_mode] == 0){
			Stats::Relief_WAF_Table[cur_relief_mode] = average_WAF;

			PRINT_MESSAGE("first WAF update  mode " <<cur_relief_mode << "WAF : "<< average_WAF);
		} else{ 		
			// 1. WAF transition check between same relief mode
			if ((average_WAF/Stats::Relief_WAF_Table[cur_relief_mode] -1 > saturation_criteria_delta) || (average_WAF/Stats::Relief_WAF_Table[cur_relief_mode] -1 < -saturation_criteria_delta)){
				cur_abnormal_cnt++;
				if (cur_abnormal_cnt > 1){
					// gap is too big. 1. need to reset Relief WAF Table 2. try current mode again. 							
					WAF_transition = true;	
					PRINT_MESSAGE(" --WAF TRANSITION same relief mode:	" << cur_relief_mode << " WAF  " << average_WAF);
					if (average_WAF > Stats::Relief_WAF_Table[cur_relief_mode]){
						WAF_increased = true;
					} else{
						WAF_increased = false;
					}

					Stats::Relief_WAF_Table[cur_relief_mode] = average_WAF;
				} else {
					//check WAF one more for new interval					
					WAF_transiaction_period = Stats::WAF_index + 5; 
					
					PRINT_MESSAGE("CHECK again, check again,  WAF " << average_WAF);
					return;
				}
			} else {
				// accumulated cur WAF
				double accumulated_cur_WAF = 0;
				for (int index = mode_start_index; index < Stats::WAF_index; index++){
					accumulated_cur_WAF += Stats::WAI[index];
				}
				accumulated_cur_WAF /= (Stats::WAF_index - mode_start_index);

				PRINT_MESSAGE("WAF update  mode " <<cur_relief_mode << "  accWAF "<< accumulated_cur_WAF << " curWAF	" <<average_WAF  << "  old WAF " << Stats::Relief_WAF_Table[cur_relief_mode]);							
				Stats::Relief_WAF_Table[cur_relief_mode] = accumulated_cur_WAF;
				cur_abnormal_cnt = 0;
			}
		}


		if (WAF_transition == true){
			for (unsigned int index = 0; index < MAX_RELIEF_MODE; index++){
				if (index != cur_relief_mode){
					Stats::Relief_WAF_Table[index] = 0;
				}
			}
		} else{  //if (WAF_transition == false)
			// TBW = Endurance/WAF;
			double cur_TBW = NAND_Endurance[cur_relief_mode] / Stats::Relief_WAF_Table[cur_relief_mode];		
			double next_TBW = 0;
			double prev_TBW = 0;

			// calculate TBW, Prev,Next
			if ((cur_relief_mode+1 < MAX_RELIEF_MODE) && (Stats::Relief_WAF_Table[cur_relief_mode+1] != 0)){
				next_TBW = NAND_Endurance[cur_relief_mode+1] / Stats::Relief_WAF_Table[cur_relief_mode+1];
			}

			if ((cur_relief_mode != 0) && (Stats::Relief_WAF_Table[cur_relief_mode-1] != 0)){
				prev_TBW = NAND_Endurance[cur_relief_mode-1] / Stats::Relief_WAF_Table[cur_relief_mode-1];
			}

			if ((prev_TBW == 0) && (next_TBW == 0)){
				if ((WAF_increased == false) && (cur_relief_mode+1 < MAX_RELIEF_MODE)){
					next_TBW = cur_TBW + 1; // Need to try next relief mode
					PRINT_MESSAGE("TRY NEXT A");
				} else if (cur_relief_mode > 0){
					prev_TBW = cur_TBW + 1; // Need to try prev relief mode
					PRINT_MESSAGE("TRY PREV B");
				}
			} else if (prev_TBW == 0){
				if ((cur_relief_mode != 0) && (cur_TBW > next_TBW)){
					prev_TBW = cur_TBW + 1; // Need to try prev relief mode
					PRINT_MESSAGE("TRY PREV C");
				}
			} else if (next_TBW == 0){
				if ((cur_relief_mode+1 < MAX_RELIEF_MODE) && (cur_TBW > prev_TBW)){
					next_TBW = cur_TBW + 1; // Need to try next relief mode
					PRINT_MESSAGE("TRY NEXT D");
				}
			}

			// Compare TBW
			if (prev_TBW > cur_TBW){				
				next_relief_mode = cur_relief_mode - 1; 

				if (Stats::Relief_WAF_Table[cur_relief_mode-1] != 0){
					PRINT_MESSAGE("<-- TBW COMPARE	prev: " <<prev_TBW << " cur  : " << cur_TBW << " next_relief mode : " << next_relief_mode);
				} else{
					PRINT_MESSAGE("<-- without COMPARE	" << "	next_relief mode : " << next_relief_mode);
				}
			} else if (next_TBW > cur_TBW){
				next_relief_mode = cur_relief_mode + 1; 

				if (Stats::Relief_WAF_Table[cur_relief_mode+1] != 0){
					PRINT_MESSAGE("--> TBW COMPARE	cur: " <<cur_TBW << " next	: " << next_TBW << " next_relief mode : " << next_relief_mode);
				} else{
					PRINT_MESSAGE("--> without COMPARE	" << "	next_relief mode : " << next_relief_mode);
				}
			}
		}

		if ((cur_relief_mode != next_relief_mode) || (WAF_transition == true)){
			is_saturated = false;
			cumulative_count = 0;
			cur_relief_mode = next_relief_mode;

			WAF_transiaction_period = Stats::WAF_index + 10;

			Stats::Interval_Relief_page_count = 0;
			Stats::Interval_Physical_write_count = 0;

			cur_saturation_count = 0;
			cur_abnormal_cnt = 0;
		}

		Stats::Relief_proportion = (double)next_relief_mode*3/256;// 1 WL per block
	}
}
	

