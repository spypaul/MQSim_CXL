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

bool cxl_mshr::removeRequest(uint64_t lba, set<uint64_t>& readcount, set<uint64_t>& writecount) {
	bool rw{ 1 };
	for (auto i : *(*mshr)[lba]) {
		Submission_Queue_Entry* s{ i->sqe };

		//process the data read/write of requests in the mshr
		if (i == *((*mshr)[lba]->begin())) {
			rw = (s->Opcode == NVME_READ_OPCODE) ? true : false;
			continue;
		}


		if (s->Opcode == NVME_READ_OPCODE) {
			readcount.emplace(i->time);
		}
		else {
			writecount.emplace(i->time);
		}

		delete i;
	}

	delete (*mshr)[lba];
	mshr->erase(mshr->find(lba));
	return rw;
}