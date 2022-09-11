#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define LOG_USER_READ 0
#define LOG_USER_WRITE 0

#define DEBUG_GC_READ 1
#define PATCH_PRECOND 1

#define ALIGN_UNIT_SIZE 4 // Flash pand page size / address mapping unit granularity. For example, when nand page size: 16KB and mapping granularity: 4KB, BUFFERING_SCALE_4K should be 4 (16KB/4KB).
	#define BUFFERING_SCALE_4K 4 //This value should be same to ALIGN_UNIT_SIZE.
#define try_combine_GCread 1 
#define PATCH_SEGMENT_REQ 1 //segment user request into trs

#define ANLZ_AFTER_PRECOND 1
	#define PORTION_PRECOND 2
#define ANLZ_AFTER_GC 1
#define PATCH_SEGMENT_REQ 1 //segment user request into trs

#define PATCH_ONLINE_CREATE_READ_SUBPG 0 //check online_create_entry_for_reads()
#define STATIC_ALLOC_ONLINE 1 //make read subpg trs in staticaly when first access read (in case no lpa-ppa in table when read)
	#define DYNAMIC_ALLOC_ONLINE 0

#define IGNORE_TIME_STAMP  0// Ignore trace time(request arrival) when use trace-based workload
	#define ENQUEUED_REQUEST_NUMBER		1 //When use trace-based, as go up, IOPS go up. 
#define DEBUG_GC 0

#include<cstdint>
#include<string>
#include<iostream>

typedef uint64_t sim_time_type;
typedef uint16_t stream_id_type;
typedef sim_time_type data_timestamp_type;

#define INVALID_TIME 18446744073709551615ULL
#define T0 0
#define INVALID_TIME_STAMP 18446744073709551615ULL
#define MAXIMUM_TIME 18446744073709551615ULL
#define ONE_SECOND 1000000000
typedef std::string sim_object_id_type;

//__asm int 3;\   exit(1);\

#define CurrentTimeStamp Simulator->Time()
#define PRINT_ERROR(MSG) {\
							std::cerr << "ERROR:" ;\
							std::cerr << MSG << std::endl; \
							std::cin.get();\
							while(1);\
						 }
#define PRINT_MESSAGE(M) std::cout << M << std::endl;
#define DEBUG(M) //std::cout<<M<<std::endl;
#define DEBUG2(M) //std::cout<<M<<std::endl;
#define SIM_TIME_TO_MICROSECONDS_COEFF 1000
#define SIM_TIME_TO_SECONDS_COEFF 1000000000
#endif // !DEFINITIONS_H
