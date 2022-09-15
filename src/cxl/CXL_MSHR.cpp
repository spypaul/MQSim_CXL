#include "CXL_MSHR.h"

cxl_mshr::cxl_mshr() {
	mshr = new map<uint64_t, list<mshr_request*>*>;
}

cxl_mshr::~cxl_mshr() {
	if (mshr) {
		for (auto i : *mshr) {
			for (auto j : *(i.second)) {
				delete j->sqe;
				delete j;
			}
			i.second->clear();
			delete i.second;
		}
		mshr->clear();

		delete mshr;

	}
}

bool cxl_mshr::isInProgress(uint64_t lba) {
	return mshr->count(lba);
}

void cxl_mshr::insertRequest(uint64_t lba, uint64_t time, Submission_Queue_Entry* sqe) {

	if (!mshr->count(lba)) {
		list<mshr_request*>* lp{ new list<mshr_request*> };
		mshr->emplace(lba, lp);
	}

	mshr_request* p{ new mshr_request{time, sqe} };
	(*mshr)[lba]->push_back(p);
	
}

void cxl_mshr::removeRequest(uint64_t lba, uint64_t& readcount, uint64_t& writecount) {
	for (auto i : *(*mshr)[lba]) {
		//process the data read/write of requests in the mshr
		if (i == *((*mshr)[lba]->begin())) {
			continue; // skip the initial miss
		}
		Submission_Queue_Entry* s{ i->sqe };

		if (s->Opcode == NVME_READ_OPCODE) {
			readcount++;
		}
		else {
			writecount++;
		}

		delete i;
	}

	delete (*mshr)[lba];
	mshr->erase(mshr->find(lba));
}