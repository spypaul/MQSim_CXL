#pragma once
#include <iostream>
#include <fstream>
#include <map>

#include <list>
#include <set>
#include "../ssd/Host_Interface_Defs.h"

using namespace std;

class mshr_request {
public:

	uint64_t time{0};
	Submission_Queue_Entry* sqe{NULL};

	mshr_request(uint64_t t, Submission_Queue_Entry* s) {
		time = t;
		sqe = s;
	};
	~mshr_request() {
		if (sqe) {
			delete sqe;
		}
	}
};

class cxl_mshr {
public:
	cxl_mshr();
	~cxl_mshr();

	bool isInProgress(uint64_t lba);

	void insertRequest(uint64_t lba, uint64_t time, Submission_Queue_Entry* sqe);
	bool removeRequest(uint64_t lba, list<uint64_t> &readcount, list<uint64_t> &writecount, bool& wasfull);

	mshr_request* removeRequestNew(uint64_t lba, list<uint64_t>& readcount, list<uint64_t>& writecount, bool& wasfull, uint64_t dram_avail, bool serviced_before, bool& completely_removed);

	bool isFull() { return full; }

	uint64_t getSize() {
		return max_row_size - row_count;
	}
	
private:
	map<uint64_t, list<mshr_request*>*>* mshr;

	uint64_t max_row_size{ 1024 };
	uint64_t max_col_size{ 65 };

	uint64_t row_count{ 0 };
	uint64_t max_col_count{ 0 };
	bool full{ 0 };

};