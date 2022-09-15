#pragma once
#include <iostream>
#include <fstream>
#include <map>

#include <list>
#include <tuple>
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
	void removeRequest(uint64_t lba);

	
private:
	map<uint64_t, list<mshr_request*>*>* mshr;

};