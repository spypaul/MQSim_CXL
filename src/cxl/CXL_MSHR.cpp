#include "CXL_MSHR.h"

cxl_mshr::cxl_mshr() {
	mshr = new map<uint64_t, list<mshr_request*>*>;
}

cxl_mshr::~cxl_mshr() {
	if (mshr) {
		for (auto i : *mshr) {
			for (auto j : *(i.second)) {
				//delete j->sqe;
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
		row_count++;
	}

	mshr_request* p{ new mshr_request{time, sqe} };
	(*mshr)[lba]->push_back(p);



	//if ((*mshr)[lba]->size() % max_col_size == 1) {
	//	row_count++;
	//}

	//if (row_count == max_row_size) {
	//	full = 1;
	//}

	if (row_count == max_row_size) {
		full = 1;
		return;
	}
	
	if ((*mshr)[lba]->size() > max_col_count) {
		max_col_count = (*mshr)[lba]->size();
		if (max_col_count == max_col_size) {
			full = 1;
		}
	}
}

bool cxl_mshr::removeRequest(uint64_t lba, list<uint64_t>& readcount, list<uint64_t>& writecount, bool& wasfull) {
	bool rw{ 1 };
	
	if (((*mshr)[lba])->size() == max_col_count && max_col_count >= max_col_size) {
		full = 0;
		max_col_count = 0;
		wasfull = 1;
	}

	if (row_count >= max_row_size) {
		full = 0;
		wasfull = 1;
	}
	row_count--;
	
	for (auto i : *(*mshr)[lba]) {
		Submission_Queue_Entry* s{ i->sqe };

		//process the data read/write of requests in the mshr
		if (i == *((*mshr)[lba]->begin())) {
			rw = (s->Opcode == NVME_READ_OPCODE) ? true : false;
			continue;
		}


		if (s->Opcode == NVME_READ_OPCODE) {
			readcount.push_back(i->time);
		}
		else {
			writecount.push_back(i->time);
		}

		delete i;
	}

	delete (*mshr)[lba];
	mshr->erase(mshr->find(lba));
	return rw;
}

mshr_request* cxl_mshr::removeRequestNew(uint64_t lba, list<uint64_t>& readcount, list<uint64_t>& writecount, bool& wasfull, uint64_t dram_avail, bool serviced_before, bool& completely_removed) {
	//bool rw{ 1 };
	mshr_request* first_entry{ NULL };

	//uint64_t oldrowcount{ (*mshr)[lba]->size() / max_col_size };
	//if ((*mshr)[lba]->size() % max_col_size > 0) oldrowcount++;
	
	if (!serviced_before && ((*mshr)[lba])->size() == max_col_count && max_col_count >= max_col_size) {
		full = 0;
		max_col_count = 0;
		wasfull = 1;
	}



	if (!serviced_before) {
		mshr_request* r{ (*mshr)[lba]->front() };
		Submission_Queue_Entry* s{ r->sqe };
		(*mshr)[lba]->pop_front();
		
		first_entry = r;
		dram_avail--;
		//delete r;
	}


	
	for (auto i = 0; i < dram_avail; i++) {
		if ((*mshr)[lba]->empty()) {
			break;
		}

		mshr_request* r{ (*mshr)[lba]->front() };
		Submission_Queue_Entry* s{ r->sqe };
		(*mshr)[lba]->pop_front();

		if (s->Opcode == NVME_READ_OPCODE) {
			readcount.push_back(r->time);
		}
		else {
			writecount.push_back(r->time);
		}

		delete r;
	}

	//uint64_t newrowcount{ (*mshr)[lba]->size() / max_col_size };
	//if ((*mshr)[lba]->size() % max_col_size > 0) newrowcount++;
	//
	//if (newrowcount < oldrowcount) {
	//	if (row_count >= max_row_size) {
	//		full = 0;
	//		wasfull = 1;
	//	}
	//	row_count-= oldrowcount - newrowcount;
	//	if (((*mshr)[lba])->empty()) {
	//		delete (*mshr)[lba];
	//		mshr->erase(mshr->find(lba));
	//		completely_removed = 1;
	//	}

	//}

	
	if (((*mshr)[lba])->empty()) {
		if (row_count >= max_row_size) {
			full = 0;
			wasfull = 1;
		}
		row_count--;
		delete (*mshr)[lba];
		mshr->erase(mshr->find(lba));
		completely_removed = 1;
	}
	return first_entry;
	
}