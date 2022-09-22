#include "Prefetching_Alg.h"

boClass::boClass() {
	for (auto i = 0; i < maxoffset; i++) offsetscore.push_back(0);
}

void boClass::bosetvalues(uint64_t hsize, uint64_t maxoff, uint64_t  b, uint64_t r) {
	maxtablesize = hsize;
	maxoffset = maxoff;
	badscore = b;
	maxround = r;

	while (offsetscore.size() < maxoffset) offsetscore.push_back(0);
	while (offsetscore.size() > maxoffset) offsetscore.pop_back();
}

void boClass::addhistory(uint64_t addr) {
	if (rrtable.size() >= maxtablesize) {
		rrtable.pop_front();
	}
	rrtable.push_back(addr);
}

void boClass::incrementscore(uint64_t offset) {
	offsetscore[offset-1]++;
}

bool boClass::inhistory(uint64_t addr) {
	for (auto i : rrtable) {
		if (i == addr) return 1;
	}
	return 0;
}
uint64_t boClass::findmax() {
	uint64_t max{ offsetscore[0] };
	uint64_t targetoffset{ 0 };
	for (auto i = 1; i < offsetscore.size(); i++) {
		if (offsetscore[i] >= max) {
			max = offsetscore[i];
			targetoffset = i;
		}
	}
	return targetoffset + 1;
}

void boClass::findoffsets() {
	olist.clear();
	for (auto i = 0; i < boprefetchK; i++) {
		uint64_t max{ 0 };
		uint64_t targetoffset{ 0 };
		for (auto j = 0; j < offsetscore.size(); j++) {
			if (olist.count(j + 1)) continue;
			if (offsetscore[j] >= max) {
				max = offsetscore[j];
				targetoffset = j;
			}
		}
		if (max >= badscore) olist.insert(targetoffset + 1);
		else return;
	}
}

bool boClass::endround() {
	return offsetundertest >= maxoffset;
}
bool boClass::endlphase() {
	return round >= maxround;
}

bool boClass::validoffset(uint64_t offset) {
	return offsetscore[offset - 1] > badscore;
}

void boClass::resetscore() {
	for (auto& i : offsetscore) {
		i = 0;
	}
}
void boClass::reset() {

	prefetch_on = 0;
	offset = 0;
	offsetundertest = 1;
	round = 0;
	olist.clear();
	for (auto& i : offsetscore) i = 0;
	rrtable.clear();

}