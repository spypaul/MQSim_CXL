#include "Stats.h"


namespace SSD_Components
{
	unsigned long Stats::IssuedReadCMD = 0;
	unsigned long Stats::IssuedCopybackReadCMD = 0;
	unsigned long Stats::IssuedInterleaveReadCMD = 0;
	unsigned long Stats::IssuedMultiplaneReadCMD = 0;
	unsigned long Stats::IssuedMultiplaneCopybackReadCMD = 0;
	unsigned long Stats::IssuedProgramCMD = 0;
	unsigned long Stats::IssuedInterleaveProgramCMD = 0;
	unsigned long Stats::IssuedMultiplaneProgramCMD = 0;
	unsigned long Stats::IssuedMultiplaneCopybackProgramCMD = 0;
	unsigned long Stats::IssuedInterleaveMultiplaneProgramCMD = 0;
	unsigned long Stats::IssuedSuspendProgramCMD = 0;
	unsigned long Stats::IssuedCopybackProgramCMD = 0;
	unsigned long Stats::IssuedEraseCMD = 0;
	unsigned long Stats::IssuedInterleaveEraseCMD = 0;
	unsigned long Stats::IssuedMultiplaneEraseCMD = 0;
	unsigned long Stats::IssuedInterleaveMultiplaneEraseCMD = 0;
	unsigned long Stats::IssuedSuspendEraseCMD = 0;
	unsigned long Stats::Total_flash_reads_for_mapping = 0;
	unsigned long Stats::Total_flash_writes_for_mapping = 0;
	unsigned long Stats::Total_flash_reads_for_mapping_per_stream[MAX_SUPPORT_STREAMS] = { 0 };
	unsigned long Stats::Total_flash_writes_for_mapping_per_stream[MAX_SUPPORT_STREAMS] = { 0 };
	unsigned int***** Stats::Block_erase_histogram;
	unsigned int  Stats::CMT_hits = 0, Stats::readTR_CMT_hits = 0, Stats::writeTR_CMT_hits = 0;
	unsigned int  Stats::CMT_miss = 0, Stats::readTR_CMT_miss = 0, Stats::writeTR_CMT_miss = 0;
	unsigned int  Stats::total_CMT_queries = 0, Stats::total_readTR_CMT_queries = 0, Stats::total_writeTR_CMT_queries = 0;

	unsigned long Stats::Additional_WAF_by_mapping = 0;

	unsigned int Stats::Total_gc_executions = 0, Stats::Total_gc_executions_per_stream[MAX_SUPPORT_STREAMS] = { 0 };
	unsigned int Stats::Total_page_movements_for_gc = 0, Stats::Total_gc_page_movements_per_stream[MAX_SUPPORT_STREAMS] = { 0 };

	unsigned int Stats::Total_wl_executions = 0, Stats::Total_wl_executions_per_stream[MAX_SUPPORT_STREAMS] = { 0 };
	unsigned int Stats::Total_page_movements_for_wl = 0, Stats::Total_wl_page_movements_per_stream[MAX_SUPPORT_STREAMS] = { 0 };

	unsigned int Stats::CMT_hits_per_stream[MAX_SUPPORT_STREAMS] = { 0 }, Stats::readTR_CMT_hits_per_stream[MAX_SUPPORT_STREAMS] = { 0 }, Stats::writeTR_CMT_hits_per_stream[MAX_SUPPORT_STREAMS] = { 0 };
	unsigned int Stats::CMT_miss_per_stream[MAX_SUPPORT_STREAMS] = { 0 }, Stats::readTR_CMT_miss_per_stream[MAX_SUPPORT_STREAMS] = { 0 }, Stats::writeTR_CMT_miss_per_stream[MAX_SUPPORT_STREAMS] = { 0 };
	unsigned int Stats::total_CMT_queries_per_stream[MAX_SUPPORT_STREAMS] = { 0 }, Stats::total_readTR_CMT_queries_per_stream[MAX_SUPPORT_STREAMS] = { 0 }, Stats::total_writeTR_CMT_queries_per_stream[MAX_SUPPORT_STREAMS] = { 0 };


	double Stats::Utilization = 0;		
	unsigned int Stats::WAF_index = 0;
	
	double Stats::Accumulated_WAF = 0;
	double Stats::Accumulated_WAI = 0;
	double Stats::WAF[MAX_WAF_HISTORY] = {0, }; 	// Physical Write / Host Write
	double Stats::WAI[MAX_WAF_HISTORY] = {0, };
	double Stats::Reliefed[MAX_WAF_HISTORY] = {0, };
	
	unsigned long Stats::Host_write_count = 0;
	unsigned long Stats::Host_write_count_subpgs = 0;
	unsigned long Stats::Prev_host_write_count = 0;
	
	unsigned long Stats::Physical_write_count = 0;
	unsigned long Stats::Physical_write_count_subpg = 0;
	unsigned long Stats::Prev_physical_write_count = 0;
	unsigned int Stats::Interval_Physical_write_count = 0;

	double Stats::Relief_proportion = 0;	
	unsigned long Stats::Relief_page_count = 0;
	unsigned long Stats::Prev_relief_page_count = 0;
	unsigned int Stats::Cur_relief_page_count = 0;
	unsigned int Stats::Interval_Relief_page_count = 0;	

	unsigned int Stats::Host_alloc = 0;
	unsigned int Stats::GC_count = 0;
	unsigned int Stats::Consecutive_gc_write = 0;
	unsigned int Stats::Max_consecutive_gc_write = 0;

	unsigned int Stats::Relief_type = 0;
	unsigned int Stats::Physical_page_count = 0;
	unsigned int Stats::Max_relief_count = 0;
	unsigned int Stats::Relief_histogram[30];

	unsigned long Stats::Relief_count = 0;
	unsigned long Stats::Erase_count = 0;

	double Stats::Relief_WAF_Table[MAX_RELIEF_MODE] = {0, };

	void Stats::Init_stats(unsigned int channel_no, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die, 
		unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int max_allowed_block_erase_count, unsigned int info)
	{
		Block_erase_histogram = new unsigned int ****[channel_no];
		for (unsigned int channel_cntr = 0; channel_cntr < channel_no; channel_cntr++) {
			Block_erase_histogram[channel_cntr] = new unsigned int***[chip_no_per_channel];
			for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++) {
				Block_erase_histogram[channel_cntr][chip_cntr] = new unsigned int**[die_no_per_chip];
				for (unsigned int die_cntr = 0; die_cntr < die_no_per_chip; die_cntr++) {
					Block_erase_histogram[channel_cntr][chip_cntr][die_cntr] = new unsigned int*[plane_no_per_die];
					for (unsigned int plane_cntr = 0; plane_cntr < plane_no_per_die; plane_cntr++) {
						Block_erase_histogram[channel_cntr][chip_cntr][die_cntr][plane_cntr] = new unsigned int[max_allowed_block_erase_count];
						Block_erase_histogram[channel_cntr][chip_cntr][die_cntr][plane_cntr][0] = block_no_per_plane * page_no_per_block; //At the start of the simulation all pages have zero erase count
						for (unsigned int i = 1; i < max_allowed_block_erase_count; ++i) {
							Block_erase_histogram[channel_cntr][chip_cntr][die_cntr][plane_cntr][i] = 0;
						}
					}
				}
			}
		}

		IssuedReadCMD = 0; IssuedCopybackReadCMD = 0; IssuedInterleaveReadCMD = 0; IssuedMultiplaneReadCMD = 0; IssuedMultiplaneCopybackReadCMD = 0;
		IssuedProgramCMD = 0; IssuedInterleaveProgramCMD = 0; IssuedMultiplaneProgramCMD = 0; IssuedMultiplaneCopybackProgramCMD = 0; IssuedInterleaveMultiplaneProgramCMD = 0; IssuedSuspendProgramCMD = 0; IssuedCopybackProgramCMD = 0;
		IssuedEraseCMD = 0; IssuedInterleaveEraseCMD = 0; IssuedMultiplaneEraseCMD = 0; IssuedInterleaveMultiplaneEraseCMD = 0;
		IssuedSuspendEraseCMD = 0;
		Total_flash_reads_for_mapping = 0; Total_flash_writes_for_mapping = 0; 
		CMT_hits = 0; readTR_CMT_hits = 0; writeTR_CMT_hits = 0;
		CMT_miss = 0; readTR_CMT_miss = 0; writeTR_CMT_miss = 0;
		total_CMT_queries = 0; total_readTR_CMT_queries = 0; total_writeTR_CMT_queries = 0;

		Total_gc_executions = 0;  Total_page_movements_for_gc = 0;
		Total_wl_executions = 0;  Total_page_movements_for_wl = 0;

		for (stream_id_type stream_id = 0; stream_id < MAX_SUPPORT_STREAMS; stream_id++) {
			Total_flash_reads_for_mapping_per_stream[stream_id] = 0;
			Total_flash_writes_for_mapping_per_stream[stream_id] = 0;
			CMT_hits_per_stream[stream_id] = 0; readTR_CMT_hits_per_stream[stream_id] = 0; writeTR_CMT_hits_per_stream[stream_id] = 0;
			CMT_miss_per_stream[stream_id] = 0; readTR_CMT_miss_per_stream[stream_id] = 0;  writeTR_CMT_miss_per_stream[stream_id] = 0;
			total_CMT_queries_per_stream[stream_id] = 0; total_readTR_CMT_queries_per_stream[stream_id] = 0; total_writeTR_CMT_queries_per_stream[stream_id] = 0;
			Total_gc_executions_per_stream[stream_id] = 0;
			Total_gc_page_movements_per_stream[stream_id] = 0;
			Total_wl_executions_per_stream[stream_id] = 0;
			Total_wl_page_movements_per_stream[stream_id] = 0;
		}

		Utilization = 0;		
		WAF_index = 0;
		
		Accumulated_WAF = 0;
		Accumulated_WAI = 0;
		
		Host_write_count = 0;
		Prev_host_write_count = 0;
		
		Physical_write_count = 0;
		Prev_physical_write_count = 0;
		Interval_Physical_write_count = 0;
				
		Relief_proportion = 0;	
		Relief_page_count = 0;
		Prev_relief_page_count = 0;
		Cur_relief_page_count = 0;
		Interval_Relief_page_count = 0;	
			
		Host_alloc = 0;
		GC_count = 0;
		Consecutive_gc_write = 0;
		Max_consecutive_gc_write = 0;

		if (info > 21){
			PRINT_MESSAGE("Relief Type : " << info << "  adjusted to 18");
			info = 18;
		}
		Relief_type = info;
		Max_relief_count = 0;
		
		for (unsigned int index = 0; index < 30; index++){
			Relief_histogram[index] = 0;
		}
		Relief_histogram[0] = channel_no*chip_no_per_channel*die_no_per_chip*plane_no_per_die*block_no_per_plane;

		Relief_count = 0;
		Erase_count = 0;

		for (unsigned int index = 0; index < MAX_RELIEF_MODE; index++){
			Relief_WAF_Table[index] = 0;
		}			
	}

	void Stats::Clear_stats(unsigned int channel_no, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
		unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int max_allowed_block_erase_count)
	{
		for (unsigned int channel_cntr = 0; channel_cntr < channel_no; channel_cntr++) {
			for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++) {
				for (unsigned int die_cntr = 0; die_cntr < die_no_per_chip; die_cntr++) {
					for (unsigned int plane_cntr = 0; plane_cntr < plane_no_per_die; plane_cntr++) {
						delete[] Block_erase_histogram[channel_cntr][chip_cntr][die_cntr][plane_cntr];
					}
					delete[] Block_erase_histogram[channel_cntr][chip_cntr][die_cntr];
				}
				delete[] Block_erase_histogram[channel_cntr][chip_cntr];
			}
			delete[] Block_erase_histogram[channel_cntr];
		}
		delete[] Block_erase_histogram;
	}
}
