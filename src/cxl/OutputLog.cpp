#include "OutputLog.h"

//outlog outputf{ "output.txt" };
//outlog evictf{ "evictionlog.txt" };
ofstream of_overall{ "./Results/overall.txt" };


outlog::outlog(string filename) {
	if (!of.is_open()) {
		of.open(filename);
	}
}

outlog::~outlog() {
	if (of.is_open()) {
		of.close();
	}
}
