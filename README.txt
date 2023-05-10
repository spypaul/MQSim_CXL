# CXL-flash Design Tools

CXL-flash Design Tools contain two primary tools, a [memory tracing tool](https://github.com/dgist-datalab/trace_generator.git) and a trace-driven simulator (this repository), to assist designing a CXL-enabled flash memory device for main memory expansion. This is an active joint research project of [S4 group](https://web.ecs.syr.edu/~bkim01/proj/rackcxl.html) at Syracuse University, [DGIST DataLab](https://datalab.dgist.ac.kr/), [DATOS Lab](https://sites.google.com/view/datoslab) at Soongsil University, and [FADU](https://www.fadu.io/).

To learn more, please keep reading this documentation. Our research paper, _Overcoming the Memory Wall with CXL-Enabled SSDs_, is currently in the revision stage of [USENIX ATC'23](https://www.usenix.org/conference/atc23). 

## Getting Started

### Prerequisites
1. The tracing tool runs on a Linux machine. We recommend using a machine with Ubuntu 20.04.5 LTS with Linux Kernel 5.17.4 installed.
2. Our simulator is designed to be independent of the hardware setup. However, it is developed using [Visual Studio 2022 IDE](https://visualstudio.microsoft.com/downloads/) on Windows, and it was only tested on a Windows 11 environment. 

*Note that currently, the simulator can only run on a Windows environment. Hence, to utilize the full toolset, the user need to have one Linux environment and one Windows environment ready. 
Also, the simulator can take up large amount of memory space, we recommend using machines with 32GB of DRAM or more for fast and stable simulation. 

Source code: 

1. [Trace Generator](https://github.com/dgist-datalab/trace_generator.git): a memory tracing tool utilized in this work.
2. [MQSim CXL](#mqsim-cxl-a-simulator-for-cxl-flash) (included in this repository): a trace-driven simulator utilized in this work. 
3. [Trace Translator](https://github.com/spypaul/trace_translation.git): an example code to translate \*.vout or \*.pout files to \*.trace files.

Trace files:
1. [Memory Traces for the CXL-flash Simulator](https://doi.org/10.5281/zenodo.7916219): memory trace files (\*.trace) used in our research paper.

### Installation
1. To install Trace Generator, please follow the instructions specified in the [memory tracing tool repository](https://github.com/dgist-datalab/trace_generator.git). 
2. To install MQSim CXL, please download and extract or clone this repository to a preferred location. 

### Workflow and Usage
This process consists of three main stages: produce \*.vout or \*.pout files --> convert the files into \*.trace files --> run simulation with the traces. 

On the Linux environment:
1. Generate a memory trace file (\*.vout or \*.pout file) for a workload utilizing the tracing tool. Please follow the instructions in the [memory tracing tool repository](https://github.com/dgist-datalab/trace_generator.git).
2. The \*.vout or \*.pout trace files need to be translated into a simulator-compatible format. Please utilize example [trace translation code](https://github.com/spypaul/trace_translation.git) to generate \*.trace files from the \*.vout or \*.pout files.

\*Note that users can also can download a .trace file from [here](https://doi.org/10.5281/zenodo.7916219) on Zenodo, so that they can test the simulator separately without having to generate their own traces

On the Windows environment:

3. To configure the simulator environment, please follow the instructions specified in [MQSim CXL Specific Execution Configurations](#mqsim-cxl-specific-execution-configurations) and [CXL-flash Architecture Configurations](#cxl-flash-architecture-configurations) of this document.
4. Create a folder named "traces" and a folder named "Results" in the root directory of the simulator package.
5. Place the generated \*.trace file into the /traces folder. Please specify the file path in workload.xml and the total number of memory accesses in config.txt. 
6. To run the simulator, please follow the instructions specified in [Usage in Windows](#usage-in-windows) of this document. 
7. The simulator output will be in the /Results folder in the root directory of the simulator package. For more details, please read the descriptions in [Simulator Output](#simulator-output) of this document. 

## Detailed Instructions for ATC'23 Evaluation
In this section, we provide detailed instructions on setting up experiments for reproducing evaluation results from our research paper, _Overcoming the Memory Wall with CXL-Enabled SSDs_. 

### Workloads and Trace Files
We evaluate CXL-flash device with two types of workloads: synthetic and real-world. 

The synthetic workload contains:
* Hash map
* Matrix multiply (indirect delta)
* Min heap 
* Random
* Stride

and 

the real-world workload contains:
* BERT[1]
* Page rank (GAPBS[2])
* Radiosity (Splash-3[3])
* XZ (Spec CPU [4], [5])
* YCSB F[6]

We include our synthetic workload source codes in test/Synthetic_Workloads of [Trace Generator](https://github.com/dgist-datalab/trace_generator.git).
Users can find the source codes of the real-world workloads from their original code bases. 

Since tracing results can be different across machines and runs, we also publish the trace files (\*.trace) utilized for the research paper in [here](https://doi.org/10.5281/zenodo.7916219) on Zenodo. 

Conetent of published trace files:
* \*\_p.trace files are the traces with physical memory addresses
* \*\_v.trace files are the traces with virtual memory addresses
* \*\_hint.trace files are the traces with hints included
* trace_synthetic_workloads.zip contains all \*.trace files of synthetic workloads


### Evaluation with Synthetic Workloads

Simulation time for each experiment: 5 minutes - 2 hours

Example configuration files: /examples/synthetic workloads/config.txt, /examples/synthetic workloads/ssdconfig.xml, /examples/synthetic workloads/workload.xml

Please replace config.txt, ssdconfig.xml, and workload.xml in the root directory with the example configuration files included in this repository. These files contain the default setup for reproducing results for the synthetic workload evaluations. Most of the parameters in the files can be left as they are. Only the ones specified in the guidelines need to be changed according to the paper. 

Experiments setup guidelines:

* To run without device DRAM cache, please set **<DRAM_mode>** = 0, **<Has_cache>** = 0, and **<Has_mshr>** = 0 in the config.txt file.
* To run with device DRAM cache only (without MSHR), please set **<DRAM_mode>** = 0, **<Has_cache>** = 1, and **<Has_mshr>** = 0 in the config.txt file.
* To run with device DRAM cache and MSHR, please set **<DRAM_mode>** = 0, **<Has_cache>** = 1, and **<Has_mshr>** = 1 in the config.txt file.
* To vary DRAM cache size, please specify **<DRAM_size>** in the config.txt file.
* For the evaluation with synthetic workloads, we configure the cache to be fully associative with FIFO as the policy. Please adjust the **Cache_placement** to **<DRAM_size>** / 4096 in the config.txt file to keep the cache fully associative.
* To include a next-n-line prefetcher, please adjust **<Prefetcher>** to "Tagged" in config.txt.
* To adjust the degree of next-n-line prefetcher, please modify line 65 of /src/cxl/Host_Interface_CXL.h by changing the initialization value of uint16_t prefetchK and recompile the code.
* To adjust the offset of next-n-line prefetcher, please modify line 66 of /src/cxl/Host_Interface_CXL.h by changing the initialization value of uint16_t prefetch_timing_offset and recompile the code.
* To adjust the parallelism (Channels x Chips) of the flash back-end, please modify **<Flash_Channel_Count>** and **<Chip_No_Per_Channel>** in ssdconfig.xml and make sure **Channel_IDs** and **<Chip_IDs>** in workload.xml are changed accordinly as well. 
* To adjust the flash technology, please adjust **<Flash_Technology>**, **<Page_Read_Latency_*>**, **<Page_Program_Latency_*>**, and **<Block_Erase_Latency>** in ssdconfig.xml. For simplicity, we keep the latency for LSB, CSB, and MSB the same. 
* To setup an ULL flash, please make **<Flash_Technology>** = "SLC", **<Page_Read_Latency_*>** = 3000, **<Page_Program_Latency_*>** = 100000, and **<Block_Erase_Latency>** = 1000000 in ssdconfig.xml. 

*Note that for Figure 6 in our paper, we plot the generated output from "repeated_access.txt." However, we accidently divide the PFN by 4096 when plotting Figure 6. 
We will re-plot the results in the final version of the paper. This does not affect the correctness of the results generated from the simulator.

### Evaluation with Real-world Workloads
	
Simulation time for each experiment: 30 minutes - 2 hours
	
Example configuration files: /examples/real world workloads/config.txt, /examples/real world workloads/ssdconfig.xml, /examples/real world workloads/workload.xml

Please replace config.txt, ssdconfig.xml, and workload.xml in the root directory with the example configuration files included in this repository. These files contain the default setup for reproducing results for the real-world workload evaluations. Most of the parameters in the files can be left as they are. Only the ones specified in the guidelines need to be changed according to the paper. 
	
Experiments setup guidelines:

* Adjust **<Cache_placement>** in config.txt if needed, but the default value for most of the experiment should be 16. 
* Adjust **<Cache_policy>** to a desired policy in config.txt if needed.
* Adjust **<Prefetcher>** to a desired algorithm in config.txt if needed.
* For the evaluation on virtual vs physical traces, please utilize two version of *.trace files provided in [here](https://doi.org/10.5281/zenodo.7916219) on Zenodo.
* For the evaluation on BERT with Hints, please utilize the bert_hint.trace provided in [here](https://doi.org/10.5281/zenodo.7916219) on Zenodo.

*Note that the trace files and generated output files for real-world workloads can be large (10s of GB). Please be sure that the running environment has enough disk space for the files.

# MQSim CXL: A Simulator for CXL-flash

MQSim CXL is a trace-driven CXL-flash device simulator built on top of MQSim-E[7], a version of MQSim[8]. 
The following documentation provides detailed information about the useage of MQSim CXL. 

## Usage in Windows

1. Open the MQSim.sln solution file in MS Visual Studio 2022 or later.
2. Set the Solution Configuration to Release (it is set to Debug by default).
3. Compile the solution.
4. Run the generated executable file (e.g., MQSim.exe) either in command line mode or by clicking the MS Visual Studio run button. 

To run with the run button, 1) under Debug of the top panel, 2) select MQSim Debug Properties, 3) under Configuration Properties, 4) select Debugging, 5) in Command Arguments, enter "-i ssdconfig.xml -w workload.xml" 5) Click OK

Command line execution:

```
$ .\MQSim.exe -i ssdconfig.xml -w workload.xml 
```
## MQSim CXL Specific Execution Configurations 

This simulator is built on top of MQSim-E. However, not all the MQSim-E-native configurations apply to this simulator; hence, we suggest users to utilize the ssdconfig.xml and workload.xml files included in this repository. We recommend only making modifications to parameters specified below and leave rest as they are.

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

1. **DRAM_mode:** 1 is for running the workload with DRAM only; 0 is for running with CXL-flash.
2. **Has_cache:** 1 is for including a DRAM device cache; 0 is for running only with the flash back end.
3. **DRAM_size:** this is for setting the device cache size in bytes.
4. **Mix_mode:** 1 is for mixing demand miss and prefetch data in cache; 0 is for creating seperate cache/buffer for demand miss and prefetch data. Currenlty, only mix mode is functional, please keep it at 1 for now.
5. **Cache_portion_percentage:** this is for specifying the size of each cache/buffer portion. Please keep it at 100 for now.
6. **Has_mshr:** 1 is for including MSHR; 0 is for running without.
7. **Cache_placement:** this is for setting the set associativity for the cache. the value range from 1 to DRAM_size/4096 (4096 is the cache line size in byte).
8. **Cache_policy:** this is for specifying the cache policy. The available options are: "Random", "FIFO", "LRU", "CFLRU".  
9. **Prefetcher:** this is for specifying the prefetching policy. The available options are: "No" (no prefetcher), "Tagged" (for next-n-line prefetcher), "Best-offset", "Leap", "Feedback_direct".
10. **Total_number_of_requests:** please specify the number of requests in the trace file.

## Simulator Output

The output of the simulator will be stored in the /Results folder. It contains the following files:

1. **overall.txt:** this file contains the information about cache hit count, prefetch amount, hit-under-misses count, flash read count, flush count (flash write count), prefetcher's performance metrics (coverage, accuracy, lateness, and pollution).
2. **latency_result.txt:** this file provides the raw access latency data for each access in nano-second. Users can utilize the data to plot latency related graphs.
3. **latency_results_no_cache.txt:** this file provides the raw access latency data for each access in nano-second specifically for DRAM only mode.
4. **repeated_access.txt:** this file provides data about repeated accesses when Has_cache = 1 and Has_mshr = 0. Each line is in the form of (PFN, is_repeated), where is_repeated can be either 1 or 0 (1 for being a repeated access).


# References

[1] Iulia Turc, Ming-Wei Chang, Kenton Lee, and Kristina Toutanova. Well-read students learn better: On the importance of pre-training compact models. arXiv preprint arXiv:1908.08962v2, 2019.

[2] Scott Beamer, Krste Asanović, David Patterson. The GAP Benchmark Suite. arXiv:1508.03619 [cs.DC], 2015.

[3] Christos Sakalis, Carl Leonardsson, Stefanos Kaxiras, and Alberto Ros. Splash-3: A properly synchronized benchmark suite for contemporary research. In 2016 IEEE International Symposium on Performance Analysis of Systems and Software (ISPASS), pages 101–111, 2016.

[4] SPEC CPU 2017. https://www.spec.org/cpu2017/.

[5] SPEC Redistributable Sources Archive. https://www.spec.org/sources/.

[6] Brian F. Cooper, Adam Silberstein, Erwin Tam, Raghu Ramakrishnan, and Russell Sears. Benchmarking cloud serving systems with YCSB. In Proceedings of
the 1st ACM Symposium on Cloud Computing, SoCC’10, page 143–154. Association for Computing Machinery, 2010.

[7] Dusol Lee, Duwon Hong, Wonil Choi, and Jihong Kim. MQSim-E: An enterprise SSD simulator. IEEE Computer Architecture Letters, 21(1):13–16, 2022.

[8] Arash Tavakkol, Juan Gómez-Luna, Mohammad Sadrosadati, Saugata Ghose, and Onur Mutlu. MQsim: A framework for enabling realistic studies of modern multi-queue SSD devices. In Proceedings of the 16th USENIX Conference on File and Storage Technologies, page 49–65. USENIX Association, 2018.

[9] Shao-Peng Yang, Minjae Kim, Sanghyun Nam, & Juhyung Park. (2023). Memory Traces for the CXL-flash Simulator [Data set]. Zenodo. https://doi.org/10.5281/zenodo.7916219
