#ifndef STATS_H
#define STATS_H

#include "SSD_Defs.h"

#define MAX_WAF_HISTORY 	10000
#define MAX_RELIEF_MODE		6 //6
namespace SSD_Components
{
	class Stats
	{
	public:
		static void Init_stats(unsigned int channel_no, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die, unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int max_allowed_block_erase_count, unsigned int info);
		static void Clear_stats(unsigned int channel_no, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die, unsigned int block_no_per_plane, unsigned int page_no_per_block, unsigned int max_allowed_block_erase_count);
		static unsigned long IssuedReadCMD, IssuedCopybackReadCMD, IssuedInterleaveReadCMD, IssuedMultiplaneReadCMD, IssuedMultiplaneCopybackReadCMD;
		static unsigned long IssuedProgramCMD, IssuedInterleaveProgramCMD, IssuedMultiplaneProgramCMD, IssuedInterleaveMultiplaneProgramCMD, IssuedCopybackProgramCMD, IssuedMultiplaneCopybackProgramCMD;
		static unsigned long IssuedEraseCMD, IssuedInterleaveEraseCMD, IssuedMultiplaneEraseCMD, IssuedInterleaveMultiplaneEraseCMD;

		static unsigned long IssuedSuspendProgramCMD, IssuedSuspendEraseCMD;

		static unsigned long Total_flash_reads_for_mapping, Total_flash_writes_for_mapping;
		static unsigned long Total_flash_reads_for_mapping_per_stream[MAX_SUPPORT_STREAMS], Total_flash_writes_for_mapping_per_stream[MAX_SUPPORT_STREAMS];

		static unsigned int CMT_hits, readTR_CMT_hits, writeTR_CMT_hits;
		static unsigned int CMT_miss, readTR_CMT_miss, writeTR_CMT_miss;
		static unsigned int total_CMT_queries, total_readTR_CMT_queries, total_writeTR_CMT_queries;
		
		static unsigned int CMT_hits_per_stream[MAX_SUPPORT_STREAMS], readTR_CMT_hits_per_stream[MAX_SUPPORT_STREAMS], writeTR_CMT_hits_per_stream[MAX_SUPPORT_STREAMS];
		static unsigned int CMT_miss_per_stream[MAX_SUPPORT_STREAMS], readTR_CMT_miss_per_stream[MAX_SUPPORT_STREAMS], writeTR_CMT_miss_per_stream[MAX_SUPPORT_STREAMS];
		static unsigned int total_CMT_queries_per_stream[MAX_SUPPORT_STREAMS], total_readTR_CMT_queries_per_stream[MAX_SUPPORT_STREAMS], total_writeTR_CMT_queries_per_stream[MAX_SUPPORT_STREAMS];
		

		static unsigned int Total_gc_executions, Total_gc_executions_per_stream[MAX_SUPPORT_STREAMS];
		static unsigned int Total_page_movements_for_gc, Total_gc_page_movements_per_stream[MAX_SUPPORT_STREAMS];

		static unsigned int Total_wl_executions, Total_wl_executions_per_stream[MAX_SUPPORT_STREAMS];
		static unsigned int Total_page_movements_for_wl, Total_wl_page_movements_per_stream[MAX_SUPPORT_STREAMS];

		static unsigned int***** Block_erase_histogram;

		static double Utilization;		
		static unsigned int WAF_index;

		static unsigned long Additional_WAF_by_mapping;

		static double Accumulated_WAF;
		static double Accumulated_WAI;
		static double WAF[MAX_WAF_HISTORY];		// Physical Write / Host Write
		static double WAI[MAX_WAF_HISTORY];
		static double Reliefed[MAX_WAF_HISTORY];
			
		static unsigned long Host_write_count;
		static unsigned long Host_write_count_subpgs;
		static unsigned long Prev_host_write_count;
		
		static unsigned long Physical_write_count;
		static unsigned long Physical_write_count_subpg;
		static unsigned long Prev_physical_write_count;
		static unsigned int Interval_Physical_write_count;

		static double 		Relief_proportion;
		static unsigned long Relief_page_count;
		static unsigned long Prev_relief_page_count;
		static unsigned int Cur_relief_page_count;
		static unsigned int Interval_Relief_page_count;

		static unsigned int Host_alloc;
		static unsigned int GC_count;
		static unsigned int Consecutive_gc_write;
		static unsigned int Max_consecutive_gc_write;

		static unsigned int Relief_type;
		static unsigned int Max_relief_count;
		static unsigned int Relief_histogram[30];

		static unsigned long Relief_count;
		static unsigned long Erase_count;	

		static unsigned int Physical_page_count;

		static double Relief_WAF_Table[MAX_RELIEF_MODE];
	};
}

#endif // !STATS_H
