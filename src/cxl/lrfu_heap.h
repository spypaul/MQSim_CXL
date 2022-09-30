#pragma once
#include <vector>
#include <map>
using namespace std;


struct bnode {

	bnode(double cvalue, uint64_t ltime, uint64_t addr) { crf = cvalue; last_reference_time = ltime; lba = addr; }

	double crf;
	uint64_t last_reference_time;
	uint64_t lba;

};

class lrfuHeap {
private:
	vector<bnode*> Q;
	map<uint64_t, uint64_t> M;
	uint64_t current_time;
	double p;
	double lambda;

public:

	lrfuHeap() { current_time = 0; p = 1; lambda = 0; }
	~lrfuHeap();

	void init(double pvalue, double lvalue);
	void add(bnode* block);
	uint64_t removeRoot();
	uint64_t getCandidate();

	void updateWhenHit(uint64_t lba, bool& falsehit);
	void restore(uint64_t bindex);

	double F(uint64_t timeDiff);
	uint64_t getTime();
	void advanceTime();
	void reset();
};