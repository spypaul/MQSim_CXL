#include "lrfu_heap.h"


lrfuHeap::~lrfuHeap() {
	while (!Q.empty()) {
		bnode* ptr{ Q.back() };

		Q.pop_back();
		delete ptr;
	}
}

void lrfuHeap::init(double pvalue, double lvalue) {
	current_time = 0;
	p = pvalue;
	lambda = lvalue;
}

void lrfuHeap::add(bnode* block) {
	Q.push_back(block);
	uint64_t bindex{ Q.size() - 1 };
	bool violation{ 1 };

	while (bindex > 0 && violation) {
		uint64_t parentindex{ (bindex - 1) / 2 };

		if (F(current_time - block->last_reference_time) * block->crf < F(current_time - Q[parentindex]->last_reference_time) * Q[parentindex]->crf) {

			swap(Q[bindex], Q[parentindex]);

			M[Q[bindex]->lba] = bindex;

			bindex = parentindex;

		}
		else {
			violation = 0;
		}

	}
	M.emplace(block->lba, bindex);
}
uint64_t lrfuHeap::removeRoot() {
	bnode* victimn{ Q[0] };
	uint64_t victimlba{ victimn->lba };
	M.erase(Q[0]->lba);
	delete victimn;

	if (Q.size() == 1) {
		Q.pop_back();
		return victimlba;
	}
	Q[0] = Q.back();
	Q.pop_back();
	M[Q[0]->lba] = 0;
	restore(0);

	return victimlba;
}
uint64_t lrfuHeap::getCandidate() {
	bnode* candidate{ Q[0] };
	return candidate->lba;
}

void lrfuHeap::updateWhenHit(uint64_t lba, bool& falsehit) {
	if (!M.count(lba)) {
		falsehit = 1;
		return;
	}

	bnode* n{ Q[M[lba]] };
	n->crf = F(0) + F(getTime() - n->last_reference_time) * n->crf;
	n->last_reference_time = getTime();
	restore(M[lba]);
	advanceTime();
}

void lrfuHeap::restore(uint64_t bindex) {
	if ((2 * bindex + 1) >= Q.size()) return; // if the node is a leaf node

	uint64_t lindex{ 2 * bindex + 1 }, rindex{ 2 * bindex + 2 };
	uint64_t smallerindex{ 0 };
	if (lindex < Q.size() && rindex < Q.size()) {//have two child
		smallerindex = (F(current_time - Q[lindex]->last_reference_time) * Q[lindex]->crf < F(current_time - Q[rindex]->last_reference_time)* Q[rindex]->crf) ? lindex : rindex;
	}
	else {
		smallerindex = lindex;
	}

	if (F(current_time - Q[bindex]->last_reference_time) * Q[bindex]->crf > F(current_time - Q[smallerindex]->last_reference_time) * Q[smallerindex]->crf) {
		swap(Q[bindex], Q[smallerindex]);
		M[Q[bindex]->lba] = bindex;
		bindex = smallerindex;
		M[Q[bindex]->lba] = bindex;
		restore(bindex);
	}


}



double lrfuHeap::F(uint64_t timeDiff) {
	return pow(1 / p, lambda * static_cast<double>(timeDiff));
}

uint64_t lrfuHeap::getTime() {
	return current_time;
}

void lrfuHeap::advanceTime() {
	current_time++;
}

void lrfuHeap::reset() {
	M.clear();

	while (!Q.empty()) {
		bnode* ptr{ Q.back() };

		Q.pop_back();
		delete ptr;
	}
	Q.clear();
}