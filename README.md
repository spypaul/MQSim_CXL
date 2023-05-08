# CXL-flash Design Tools

CXL-flash Design Tools contain two primary tools, a memory tracing tool and a trace-driven simulator, to assist designing a CXL-enabled flash memory device for main memory expansion. 

## Getting Started

## Detailed Instructions for ATC'23 Evaluation

## References

[1] Iulia Turc, Ming-Wei Chang, Kenton Lee, and Kristina Toutanova. Well-read students learn better: On the importance of pre-training compact models. arXiv preprint arXiv:1908.08962v2, 2019.

[2] Scott Beamer, Krste Asanovi ́c, and David Patterson. The GAP benchmark suite, 2015.

[3] Christos Sakalis, Carl Leonardsson, Stefanos Kaxiras, and Alberto Ros. Splash-3: A properly synchronized benchmark suite for contemporary research. In 2016 IEEE International Symposium on Performance Analysis of Systems and Software (ISPASS), pages 101–111, 2016.

[4] Brian F. Cooper, Adam Silberstein, Erwin Tam, Raghu Ramakrishnan, and Russell Sears. Benchmarking cloud serving systems with YCSB. In Proceedings of
the 1st ACM Symposium on Cloud Computing, SoCC’10, page 143–154. Association for Computing Machinery, 2010.

[5] SPEC CPU 2017. https://www.spec.org/cpu2017/.


# MQSim CXL: A Simulator for CXL-flash

## Usage in Windows

1. Open the MQSim.sln solution file in MS Visual Studio 2022 or later.
2. Set the Solution Configuration to Release (it is set to Debug by default).
3. Compile the solution.
4. Run the generated executable file (e.g., MQSim.exe) either in command line mode or by clicking the MS Visual Studio run button. To run with the run button, 1) Right click on the empty space in solution explorer 2) Select Properties 3) Under Configuration Properties, select Debugging 4) In Command Arguments, enter "-i ssdconfig.xml -w workload.xml" 5) Click Apply

Command line execution:

```
$ .\MQSim.exe -i ssdconfig.xml -w workload.xml 
```
## MQSim CXL Specific Execution Configurations 

This simulator is built on top of MQSim. However, not all the MQSim-native configurations apply to this simulator; hence, we suggest users to utilize the existing ssdconfig.xml and workload.xml files. We recommend only make modifications to parameters specified below and keep others fixed as they are.

### ssdconfig.xml
1. **Flash_Channel_Count:** the number of flash channels in the SSD back end. Range = {all positive integer values}.
2. **Chip_No_Per_Channel:** the number of flash chips attached to each channel in the SSD back end. Range = {all positive integer values}.
3. **Flash_Technology:** Range = {SLC, MLC, TLC}.
4. **Page_Read_Latency_LSB:** the latency of reading LSB bits of flash memory cells in nanoseconds. Range = {all positive integer values}.
5. **Page_Read_Latency_CSB:** the latency of reading CSB bits of flash memory cells in nanoseconds. Range = {all positive integer values}.
6. **Page_Read_Latency_MSB:** the latency of reading MSB bits of flash memory cells in nanoseconds. Range = {all positive integer values}.
7. **Page_Program_Latency_LSB:** the latency of programming LSB bits of flash memory cells in nanoseconds. Range = {all positive integer values}.
8. **Page_Program_Latency_CSB:** the latency of programming CSB bits of flash memory cells in nanoseconds. Range = {all positive integer values}.
9. **Page_Program_Latency_MSB:** the latency of programming MSB bits of flash memory cells in nanoseconds. Range = {all positive integer values}.
10. **Block_Erase_Latency:** the latency of erasing a flash block in nanoseconds. Range = {all positive integer values}.
11. **Die_No_Per_Chip:** the number of dies in each flash chip. Range = {all positive integer values}.
12. **Plane_No_Per_Die:** the number of planes in each die. Range = {all positive integer values}.
13. **Block_No_Per_Plane:** the number of flash blocks in each plane. Range = {all positive integer values}.
14. **Page_No_Per_Block:** the number of physical pages in each flash block. Range = {all positive integer values}.
15. **Page_Capacity:** the size of each physical flash page in bytes. Range = {all positive integer values}.

### workload.xml
1. **Channel_IDs:** a comma-separated list of channel IDs that are allocated to this workload. This list is used for resource partitioning. If there are C channels in the SSD (defined in the SSD configuration file), then the channel ID list should include values in the range 0 to C-1. If no resource partitioning is required, then all workloads should have channel IDs 0 to C-1.
2. **Chip_IDs:** a comma-separated list of chip IDs that are allocated to this workload. This list is used for resource partitioning. If there are W chips in each channel (defined in the SSD configuration file), then the chip ID list should include values in the range 0 to W-1. If no resource partitioning is required, then all workloads should have chip IDs 0 to W-1.
3. **Die_IDs:** a comma-separated list of chip IDs that are allocated to this workload. This list is used for resource partitioning. If there are D dies in each flash chip (defined in the SSD configuration file), then the die ID list should include values in the range 0 to D-1. If no resource partitioning is required, then all workloads should have die IDs 0 to D-1.
4. **Plane_IDs:** a comma-separated list of plane IDs that are allocated to this workload. This list is used for resource partitioning. If there are P planes in each die (defined in the SSD configuration file), then the plane ID list should include values in the range 0 to P-1. If no resource partitioning is required, then all workloads should have plane IDs 0 to P-1.
5. **Initial_Occupancy_Percentage:** the percentage of the storage space (i.e., logical pages) that is filled during preconditioning. Range = {all integer values in the range 1 to 100}.
6. **File_Path:** the relative/absolute path to the input trace file.
7. **Percentage_To_Be_Executed:** the percentage of requests in the input trace file that should be executed. Range = {all integer values in the range 1 to 100}.

## CXL-flash Architecture Configurations 

config.txt file contains all architectural configurations for the CXL-flash device simulated. The following explains the usage for each parameter.

1. **DRAM_mode:** 1 is for running the workload with DRAM only; 0 is for running with CXL-flash
2. **Has_cache:** 1 is for including a DRAM device cahce; 0 is for running only with the flash back end
3. **DRAM_size:** this is for setting the device cache size in bytes
4. **Mix_mode:** 1 is for mixing demand miss and prefetch data in cache; 0 is for creating seperate cache/buffer for demand miss and prefetch data. Currenlty, only mix mode is functional, please keep it at 1 for now
5. **Cache_portion_percentage:* this is for specifying the size of each cache/buffer portion. Please keep it at 100 for now
6. **Has_mshr:** 1 is for including MSHR; 0 is for running without
7. **Cache_placement:** this is for setting the set associativity for the cache. the value range from 1 to DRAM_size/4096 (4096 is the cache line size in byte)
8. **Cache_policy:** this is for specifying the cache policy. The available options are: "Random", "FIFO", "LRU", "CFLRU"  
9. **Prefetcher:** this is for specifying the prefetching policy. The available options are: "No" (no prefetcher), "Tagged" (for next-n-line prefetcher), "Best-offset", "Leap", "Feedback_direct"
10. **Total_number_of_requests:** please specify the number of requests in the trace file

## Simulator Output

The output of the simulator will be stored in the \Results folder. It contains the following files:

1. **overall.txt:** this file contains the information about cache hit count, prefetch amount, hit-under-misses count, flash read count, flsuh count (flash write count), prefetcher's performance metrics (coverage, accuracy, lateness, and pollution)
2. **latency_result.txt:** this file provides the raw access latency data for each access in nano-second. You can utilize the data to plot latency related graphs
3. **latency_results_no_cache.txt:** this file provides the raw access latency data for each access in nano-second specifically for DRAM only mode.
4. **repeated_access.txt:** this file provides data about repeated accesses when Has_cache = 1 and Has_mshr = 0. Each line is in the form of (PFN, is_repeated), where is_repeated can be either 1 or 0 (1 for being a repeated access)



<!---

# Original MQSim Related Information (only for your own reference)

## MQSim Execution Configurations 

You can specify your preferred SSD configuration in the XML format. If the SSD configuration file specified in the command line does not exist, MQSim will create a sample XML file in the specified path. Here are the definitions of configuration parameters available in the XML file:

### Host
1. **PCIe_Lane_Bandwidth:** the PCIe bandwidth per lane in GB/s. Range = {all positive double precision values}.
2. **PCIe_Lane_Count:** the number of PCIe lanes. Range = {all positive integer values}.
3. **SATA_Processing_Delay:** defines the aggregate hardware and software processing delay to send/receive a SATA message to the SSD device in nanoseconds. Range = {all positive integer values}.
4. **Enable_ResponseTime_Logging:** the toggle to enable response time logging. If enabled, response time is calculated for each running I/O flow over simulation epochs and is reported in a log file at the end of each epoch. Range = {true, false}.
5. **ResponseTime_Logging_Period_Length:** defines the epoch length for response time logging in nanoseconds. Range = {all positive integer values}.

### SSD Device
1. **Seed:** the seed value that is used for random number generation. Range = {all positive integer values}.
2. **Enabled_Preconditioning:** the toggle to enable preconditioning. Range = {true, false}.
3. **Memory_Type:** the type of the non-volatile memory used for data storage. Range = {FLASH}.
4. **HostInterface_Type:** the type of host interface. Range = {NVME, SATA}.
5. **IO_Queue_Depth:** the length of the host-side I/O queue. If the host interface is set to NVME, then **IO_Queue_Depth** defines the capacity of the I/O Submission and I/O Completion Queues. If the host interface is set to SATA, then **IO_Queue_Depth** defines the capacity of the Native Command Queue (NCQ). Range = {all positive integer values}
6. **Queue_Fetch_Size:** the value of the QueueFetchSize parameter as described in the FAST 2018 paper [1]. Range = {all positive integer values}
7. **Caching_Mechanism:** the data caching mechanism used on the device. Range = {SIMPLE: implements a simple data destaging buffer, ADVANCED: implements an advanced data caching mechanism with different sharing options among the concurrent flows}.
8. **Data_Cache_Sharing_Mode:** the sharing mode of the DRAM data cache (buffer) among the concurrently running I/O flows when an NVMe host interface is used. Range = {SHARED, EQUAL_PARTITIONING}.
9. **Data_Cache_Capacity:** the size of the DRAM data cache in bytes. Range = {all positive integers}
10. **Data_Cache_DRAM_Row_Size:** the size of the DRAM rows in bytes. Range = {all positive power of two numbers}.
11. **Data_Cache_DRAM_Data_Rate:** the DRAM data transfer rate in MT/s. Range = {all positive integer values}.
12. **Data_Cache_DRAM_Data_Burst_Size:** the number of bytes that are transferred in one DRAM burst (depends on the number of DRAM chips). Range = {all positive integer values}.
13. **Data_Cache_DRAM_tRCD:** the value of the timing parameter tRCD in nanoseconds used to access DRAM in the data cache. Range = {all positive integer values}.
14. **Data_Cache_DRAM_tCL:** the value of the timing parameter tCL in nanoseconds used to access DRAM in the data cache. Range = {all positive integer values}.
15. **Data_Cache_DRAM_tRP:** the value of the timing parameter tRP in nanoseconds used to access DRAM in the data cache. Range = {all positive integer values}.
16. **Address_Mapping:** the logical-to-physical address mapping policy implemented in the Flash Translation Layer (FTL). Range = {PAGE_LEVEL, HYBRID}.
17. **Ideal_Mapping_Table:** if mapping is ideal, table is enabled in which all address translations entries are always in CMT (i.e., CMT is infinite in size) and thus all adddress translation requests are always successful (i.e., all the mapping entries are found in the DRAM and there is no need to read mapping entries from flash)
18. **CMT_Capacity:** the size of the SRAM/DRAM space in bytes used to cache the address mapping table (Cached Mapping Table). Range = {all positive integer values}.
19. **CMT_Sharing_Mode:** the mode that determines how the entire CMT (Cached Mapping Table) space is shared among concurrently running flows when an NVMe host interface is used. Range = {SHARED, EQUAL_PARTITIONING}.
20. **Plane_Allocation_Scheme:** the scheme for plane allocation as defined in Tavakkol et al. [3]. Range = {CWDP, CWPD, CDWP, CDPW, CPWD, CPDW, WCDP, WCPD, WDCP, WDPC, WPCD, WPDC, DCWP, DCPW, DWCP, DWPC, DPCW, DPWC, PCWD, PCDW, PWCD, PWDC, PDCW, PDWC}
21. **Transaction_Scheduling_Policy:** the transaction scheduling policy that is used in the SSD back end. Range = {OUT_OF_ORDER as defined in the Sprinkler paper [2]}.
22. **Overprovisioning_Ratio:** the ratio of reserved storage space with respect to the available flash storage capacity. Range = {all positive double precision values}.
23. **GC_Exect_Threshold:** the threshold for starting Garbage Collection (GC). When the ratio of the free physical pages for a plane drops below this threshold, GC execution begins. Range = {all positive double precision values}.
24. **GC_Block_Selection_Policy:** the GC block selection policy. Range {GREEDY, RGA *(described in [4] and [5])*, RANDOM *(described in [4])*, RANDOM_P *(described in [4])*, RANDOM_PP *(described in [4])*, FIFO *(described in [6])*}.
25. **Use_Copyback_for_GC:** used in GC_and_WL_Unit_Page_Level to determine block_manager→Is_page_valid gc_write transaction
26. **Preemptible_GC_Enabled:** the toggle to enable pre-emptible GC (described in [7]). Range = {true, false}.
27. **GC_Hard_Threshold:** the threshold to stop pre-emptible GC execution (described in [7]). Range = {all possible positive double precision values less than GC_Exect_Threshold}.
28. **Dynamic_Wearleveling_Enabled:** the toggle to enable dynamic wear-leveling (described in [9]). Range = {true, false}.
29. **Static_Wearleveling_Enabled:** the toggle to enable static wear-leveling (described in [9]). Range = {all positive integer values}.
30. **Static_Wearleveling_Threshold:** the threshold for starting static wear-leveling (described in [9]). When the difference between the minimum and maximum erase count within a memory unit (e.g., plane in flash memory) drops below this threshold, static wear-leveling begins. Range = {true, false}.
31. **Preferred_suspend_erase_time_for_read:** the reasonable time to suspend an ongoing flash erase operation in favor of a recently-queued read operation. Range = {all positive integer values}.
32. **Preferred_suspend_erase_time_for_write:** the reasonable time to suspend an ongoing flash erase operation in favor of a recently-queued read operation. Range = {all positive integer values}.
33. **Preferred_suspend_write_time_for_read:** the reasonable time to suspend an ongoing flash erase operation in favor of a recently-queued program operation. Range = {all positive integer values}.
34. **Flash_Channel_Count:** the number of flash channels in the SSD back end. Range = {all positive integer values}.
35. **Flash_Channel_Width:** the width of each flash channel in byte. Range = {all positive integer values}.
36. **Channel_Transfer_Rate:** the transfer rate of flash channels in the SSD back end in MT/s. Range = {all positive integer values}.
37. **Chip_No_Per_Channel:** the number of flash chips attached to each channel in the SSD back end. Range = {all positive integer values}.
38. **Flash_Comm_Protocol:** the Open NAND Flash Interface (ONFI) protocol used for data transfer over flash channels in the SSD back end. Range = {NVDDR2}.

### NAND Flash
1. **Flash_Technology:** Range = {SLC, MLC, TLC}.
2. **CMD_Suspension_Support:** the type of suspend command support by flash chips. Range = {NONE, PROGRAM, PROGRAM_ERASE, ERASE}.
3. **Page_Read_Latency_LSB:** the latency of reading LSB bits of flash memory cells in nanoseconds. Range = {all positive integer values}.
4. **Page_Read_Latency_CSB:** the latency of reading CSB bits of flash memory cells in nanoseconds. Range = {all positive integer values}.
5. **Page_Read_Latency_MSB:** the latency of reading MSB bits of flash memory cells in nanoseconds. Range = {all positive integer values}.
6. **Page_Program_Latency_LSB:** the latency of programming LSB bits of flash memory cells in nanoseconds. Range = {all positive integer values}.
7. **Page_Program_Latency_CSB:** the latency of programming CSB bits of flash memory cells in nanoseconds. Range = {all positive integer values}.
8. **Page_Program_Latency_MSB:** the latency of programming MSB bits of flash memory cells in nanoseconds. Range = {all positive integer values}.
9. **Block_Erase_Latency:** the latency of erasing a flash block in nanoseconds. Range = {all positive integer values}.
10. **Block_PE_Cycles_Limit:** the PE limit of each flash block. Range = {all positive integer values}.
11. **Suspend_Erase_Time:** the time taken to suspend an ongoing erase operation in nanoseconds. Range = {all positive integer values}.
12. **Suspend_Program_Time:** the time taken to suspend an ongoing program operation in nanoseconds. Range = {all positive integer values}.
13. **Die_No_Per_Chip:** the number of dies in each flash chip. Range = {all positive integer values}.
14. **Plane_No_Per_Die:** the number of planes in each die. Range = {all positive integer values}.
15. **Block_No_Per_Plane:** the number of flash blocks in each plane. Range = {all positive integer values}.
16. **Page_No_Per_Block:** the number of physical pages in each flash block. Range = {all positive integer values}.
17. **Page_Capacity:** the size of each physical flash page in bytes. Range = {all positive integer values}.
18. **Page_Metadat_Capacity:** the size of the metadata area of each physical flash page in bytes. Range = {all positive integer values}.


## MQSim Workload Definition
You can define your preferred set of workloads in the XML format. If the specified workload definition file does not exist, MQSim will create a sample workload definition file in XML format for you (i.e., workload.xml). Here is the explanation of the XML attributes and tags for the workload definition file:

1. The entire workload definitions should be embedded within <MQSim_IO_Scenarios></MQSim_IO_Scenarios> tags. You can define different sets of *I/O scenarios* within these tags. MQSim simulates each I/O scenario separately.

2. We call a set of workloads that should be executed together, an *I/O scenario*. An I/O scenario is defined within the <IO_Scenario></IO_Scenario> tags. For example, two different I/O scenarios are defined in the workload definition file in the following way:
```
<MQSim_IO_Scenarios>
	<IO_Scenario>
	.............
	</IO_Scenario>
	<IO_Scenario>
	.............
	</IO_Scenario>
</MQSim_IO_Scenarios>
```

For each I/O scenario, MQSim 1) rebuilds the Host and SSD Drive model and executes the scenario to completion, and 2) creates an output file and writes the simulation results to it. For the example mentioned above, MQSim builds the Host and SSD Drive models twice, executes the first and second I/O scenarios, and finally writes the execution results into the workload_scenario_1.xml and workload_scenario_2.xml files, respectively.

You can define up to 8 different workloads within each IO_Scenario tag. Each workload could either be a disk trace file that has already been collected on a real system or a synthetic stream of I/O requests that are generated by MQSim's request generator.

### Defining a Trace-based Workload
You can define a trace-based workload for MQSim, using the <IO_Flow_Parameter_Set_Trace_Based> XML tag. Currently, MQSim can execute ASCII disk traces define in [8] in which each line of the trace file has the following format:
1.Request_Arrival_Time 2.Device_Number 3.Starting_Logical_Sector_Address 4.Request_Size_In_Sectors 5.Type_of_Requests[0 for write, 1 for read]

The following parameters are used to define a trace-based workload:
1. **Priority_Class:** the priority class of the I/O queue associated with this I/O request. Range = {URGENT, HIGH, MEDIUM, LOW}.
2. **Device_Level_Data_Caching_Mode:** the type of on-device data caching for this flow. Range={WRITE_CACHE, READ_CACHE, WRITE_READ_CACHE, TURNED_OFF}. If the caching mechanism mentioned above is set to SIMPLE, then only WRITE_CACHE and TURNED_OFF modes could be used.
3. **Channel_IDs:** a comma-separated list of channel IDs that are allocated to this workload. This list is used for resource partitioning. If there are C channels in the SSD (defined in the SSD configuration file), then the channel ID list should include values in the range 0 to C-1. If no resource partitioning is required, then all workloads should have channel IDs 0 to C-1.
4. **Chip_IDs:** a comma-separated list of chip IDs that are allocated to this workload. This list is used for resource partitioning. If there are W chips in each channel (defined in the SSD configuration file), then the chip ID list should include values in the range 0 to W-1. If no resource partitioning is required, then all workloads should have chip IDs 0 to W-1.
5. **Die_IDs:** a comma-separated list of chip IDs that are allocated to this workload. This list is used for resource partitioning. If there are D dies in each flash chip (defined in the SSD configuration file), then the die ID list should include values in the range 0 to D-1. If no resource partitioning is required, then all workloads should have die IDs 0 to D-1.
6. **Plane_IDs:** a comma-separated list of plane IDs that are allocated to this workload. This list is used for resource partitioning. If there are P planes in each die (defined in the SSD configuration file), then the plane ID list should include values in the range 0 to P-1. If no resource partitioning is required, then all workloads should have plane IDs 0 to P-1.
7. **Initial_Occupancy_Percentage:** the percentage of the storage space (i.e., logical pages) that is filled during preconditioning. Range = {all integer values in the range 1 to 100}.
8. **File_Path:** the relative/absolute path to the input trace file.
9. **Percentage_To_Be_Executed:** the percentage of requests in the input trace file that should be executed. Range = {all integer values in the range 1 to 100}.
10. **Relay_Count:** the number of times that the trace execution should be repeated. Range = {all positive integer values}.
11. **Time_Unit:** the unit of arrival times in the input trace file. Range = {PICOSECOND, NANOSECOND, MICROSECOND}

### Defining a Synthetic Workload
You can define a synthetic workload for MQSim, using the <IO_Flow_Parameter_Set_Synthetic> XML tag. 

The following parameters are used to define a trace-based workload:
1. **Priority_Class:** same as trace-based parameters mentioned above.
2. **Device_Level_Data_Caching_Mode:** same as trace-based parameters mentioned above.
3. **Channel_IDs:** same as trace-based parameters mentioned above.
4. **Chip_IDs:** same as trace-based parameters mentioned above.
5. **Die_IDs:** same as trace-based parameters mentioned above.
6. **Plane_IDs:** same as trace-based parameters mentioned above.
7. **Initial_Occupancy_Percentage:** same as trace-based parameters mentioned above.
8. **Working_Set_Percentage:** the percentage of available logical storage space that is accessed by generated requests. Range = {all integer values in the range 1 to 100}.
9. **Synthetic_Generator_Type:** determines the way that the stream of requests is generated. Currently, there are two modes for generating consecutive requests, 1) based on the average bandwidth of I/O requests, or 2) based on the average depth of the I/O queue. Range = {BANDWIDTH, QUEUE_DEPTH}.
10. **Read_Percentage:** the ratio of read requests in the generated flow of I/O requests. Range = {all integer values in the range 1 to 100}.
11. **Address_Distribution:** the distribution pattern of addresses in the generated flow of I/O requests. Range = {STREAMING, RANDOM_UNIFORM, RANDOM_HOTCOLD, MIXED_STREAMING_RANDOM}.
12. **Percentage_of_Hot_Region:** if RANDOM_HOTCOLD is set for address distribution, then this parameter determines the ratio of the hot region with respect to the entire logical address space. Range = {all integer values in the range 1 to 100}.
13. **Generated_Aligned_Addresses:** the toggle to enable aligned address generation. Range = {true, false}.
14. **Address_Alignment_Unit:** the unit that all generated addresses must be aligned to in sectors (i.e. 512 bytes). Range = {all positive integer values}.
15. **Request_Size_Distribution:** the distribution pattern of request sizes in the generated flow of I/O requests. Range = {FIXED, NORMAL}.
16. **Average_Request_Size:** average size of generated I/O requests in sectors (i.e. 512 bytes). Range = {all positive integer values}.
17. **Variance_Request_Size:** if the request size distribution is set to NORMAL, then this parameter determines the variance of I/O request sizes in sectors. Range = {all non-negative integer values}.
18. **Seed:** the seed value that is used for random number generation. Range = {all positive integer values}.
19. **Average_No_of_Reqs_in_Queue:** average number of I/O requests enqueued in the host-side I/O queue (i.e., the intensity of the generated flow). This parameter is used in QUEUE_DEPTH mode of request generation. Range = {all positive integer values}.
20. **Bandwidth:** the average bandwidth of I/O requests (i.e., the intensity of the generated flow) in bytes per seconds. MQSim uses this parameter in BANDWIDTH mode of request generation.
21. **Stop_Time:** defines when to stop generating I/O requests in nanoseconds.
22. **Total_Requests_To_Generate:** if Stop_Time is set to zero, then MQSim's request generator considers Total_Requests_To_Generate to decide when to stop generating I/O requests.


## Analyze MQSim's XML Output
You can use an XML processor to easily read and analyze an MQSim output file. For example, you can open an MQSim output file in MS Excel. Then, MS Excel shows a set of options and you should choose "Use the XML Source task pane". The XML file is processed in MS Excel and a task pane is shown with all output parameters listed in it. In the task pane on the right, you see different types of statistics available in the MQSim's output file. To read the value of a parameter, you should:<br />
1. Drag and drop that parameter from the task source pane to the Excel sheet.,<br />
2. Right click on the cell that you have dropped the parameter and select *XML* > *Refresh XML Data* from the drop-down menue.

The parameters used to define the output file of the simulator are divided into categories:

### Host
For each defined IO_Flow, the following parameters are shown:
1. **Name:** The name of the IO flow, e.g. Host.IO_Flow.Synth.No_0
2. **Request_Count:** The total number of requests from this IO_flow.
3. **Read_Request_Count:** The total number of read requests from this IO_flow.
4. **Write_Request_Count:** The total number of write requests from this IO_flow.
5. **IOPS:** The number of IO operations per second, i.e. how many requests are served per second.
6. **IOPS_Read:** The number of read IO operations per second.
7. **IOPS_Write:** The number of write IO operations per second.
8. **Bytes_Transferred:** The total number of data bytes transferred across the interface.
9. **Bytes_Transferred_Read:** The total number of data bytes read from the SSD Device.
10. **Bytes_Transferred_write:** The total number of data bytes written to the SSD Device.
11. **Bandwidth:** The total bandwidth delivered by the SSD Device in bytes per second.
12. **Bandwidth_Read:** The total read bandwidth delivered by the SSD Device in bytes per second.
13. **Bandwidth_Write:** The total write bandwidth delivered by the SSD Device in bytes per second.
14. **Device_Response_Time:** The average SSD device response time for a request, in nanoseconds. This is defined as the time between enqueueing the request in the I/O submission queue, and removing it from the I/O completion queue.
15. **Min_Device_Response_Time:** The minimum SSD device response time for a request, in nanoseconds. 
16. **Max_Device_Response_Time:** The maximum SSD device response time for a request, in nanoseconds.
17. **End_to_End_Request_Delay:** The average delay between generating an I/O request and receiving a corresponding answer. This is defined as the difference between the request arrival time, and its removal time from the I/O completion queue. Note that the request arrival_time is the same as the request enqueue_time, when using the multi-queue properties of NVMe drives.
18. **Min_End_to_End_Request_Delay:** The minimum end-to-end request delay.
19. **Max_End_to_End_Request_Delay:** The maximum end-to-end request delay.

### SSDDevice
The output parameters in the SSDDevice category contain values for:
1. Average transaction times at a lower abstraction level (SSDDevice.IO_Stream)
2. Statistics for the flash transaction layer (FTL)
3. Statistics for each queue in the SSD's internal flash Transaction Scheduling Unit (TSU): In the TSU exists a User_Read_TR_Queue, a User_Write_TR_Queue, a Mapping_Read_TR_Queue, a Mapping_Write_TR_Queue, a GC_Read_TR_Queue, a GC_Write_TR_queue, a GC_Erase_TR_Queue for each combination of channel and package.
4. For each package: the fraction of time in the exclusive memory command execution, exclusive data transfer, overlapped memory command execution and data transfer, and idle mode.


## References
[1] A. Tavakkol et al., "MQSim: A Framework for Enabling Realistic Studies of Modern Multi-Queue SSD Devices," FAST, pp. 49 - 66, 2018.

[2] M. Jung and M. T. Kandemir, "Sprinkler: Maximizing Resource Utilization in Many-chip Solid State Disks," HPCA, pp. 524-535, 2014.

[3] A. Tavakkol  et al., "Performance Evaluation of Dynamic Page Allocation Strategies in SSDs," ACM TOMPECS, pp. 7:1--7:33, 2016.

[4] B. Van Houdt, "A Mean Field Model for a Class of Garbage Collection Algorithms in Flash-based Solid State Drives," SIGMETRICS, pp. 191-202, 2013.

[5] Y. Li et al., "Stochastic Modeling of Large-Scale Solid-State Storage Systems: Analysis, Design Tradeoffs and Optimization," SIGMETRICS, pp. 179-190, 2013.

[6] P. Desnoyers, "Analytic Modeling of SSD Write Performance", SYSTOR, pp. 12:1-12:10, 2012.

[7] J. Lee et al., "Preemptible I/O Scheduling of Garbage Collection for Solid State Drives," Vol. 32, No. 2, pp. 247-260, 2013.

[8] J. S. Bucy et al., "The DiskSim Simulation Environment Version 4.0 Reference Manual", CMU Tech Rep. CMU-PDL-08-101, 2008.

[9] Micron Technology, Inc., "Wear Leveling in NAND Flash Memory", Application Note AN1822, 2010.
--->
