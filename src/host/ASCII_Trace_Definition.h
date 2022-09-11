#ifndef ASCII_TRACE_DEFINITION_H
#define ASCII_TRACE_DEFINITION_H

enum class Trace_Time_Unit { PICOSECOND, NANOSECOND, MICROSECOND};//The unit of arrival times in the input file
#define PicoSecondCoeff  1000000000000	//the coefficient to convert picoseconds to second
#define NanoSecondCoeff  1000000000	//the coefficient to convert nanoseconds to second
#define MicroSecondCoeff  1000000	//the coefficient to convert microseconds to second

//#define MSR_TRACE 
//#define OLD_TRACE


/// Default Definition
#if defined(MSR_TRACE)
/// Definition for MSR Trace.
#define ASCIITraceTimeColumn 0
#define ASCIITraceDeviceColumn 2
#define ASCIITraceAddressColumn 4
#define ASCIITraceSizeColumn 5
#define ASCIITraceTypeColumn 3

#define ASCIITraceWriteCode "Write,"
#define ASCIITraceReadCode "Read,"
#define ASCIILineDelimiter ','
#define ASCIIItemsPerLine 7
 
#elif defined(OLD_TRACE)
#define ASCIITraceTimeColumn 0
#define ASCIITraceAddressColumn 2
#define ASCIITraceSizeColumn 3
#define ASCIITraceTypeColumn 1

#define ASCIITraceWriteCode "write "
#define ASCIITraceReadCode "read "
#define ASCIILineDelimiter ' '
#define ASCIIItemsPerLine 4

#else
#define ASCIITraceTimeColumn 0
#define ASCIITraceDeviceColumn 1
#define ASCIITraceAddressColumn 2
#define ASCIITraceSizeColumn 3
#define ASCIITraceTypeColumn 4

#define ASCIITraceWriteCode "0"
#define ASCIITraceReadCode "1"
#define ASCIILineDelimiter ' '
#define ASCIIItemsPerLine 5
#endif




#endif // !ASCII_TRACE_DEFINITION_H
