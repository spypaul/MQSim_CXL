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


void leapClass::leapinit() {

}

void leapClass::historyinsert(uint64_t addr) {
	if (hbuffer.size() == 0) {
		hbuffer.push_back({ addr, 0 });
		return;
	}

	uint64_t preaddr{ hbuffer.back().first };
	int64_t delta{ static_cast<int64_t>(addr) - static_cast<int64_t>(preaddr) }; //careful

	hbuffer.push_back({ addr, delta });

	if (hbuffer.size() > maxbuffersize) {
		hbuffer.pop_front();
	}

}
int64_t leapClass::findoffset() {
	uint64_t wsize{ hbuffer.size() / splitvalue };
	int64_t delta{ 0 };
	while (1) {
		auto iter{ hbuffer.end() };
		for (uint64_t i = 0; i < wsize; i++)iter--;

		//boyer-moore algorithm
		int64_t candidate{ 0 };
		uint64_t vote{ 0 };
		auto iter2{ iter };

		while (iter2 != hbuffer.end()) {
			if (vote == 0) candidate = iter2->second;
			vote = (iter2->second == candidate) ? vote++ : vote--;
			iter2++;
		}

		uint64_t count{ 0 };

		while (iter != hbuffer.end()) {
			if (iter->second == candidate) count++;
			iter++;
		}

		if (count > wsize / 2) {
			delta = candidate;
		}

		wsize = 2 * wsize + 1;

		if (delta != 0 || wsize > hbuffer.size() || wsize == 0) return delta;

	}

	return delta;
}

uint64_t leapClass::getk(uint64_t prefetchHitCount) {
	uint64_t prefetchamount{ 0 };
	if ((prefetchHitCount - lastprefetchhit) == 0) {
		prefetchamount = leapinitialprefetchamount;
	}
	else {
		prefetchamount = pow(2, static_cast<uint64_t>(ceil(log2(prefetchHitCount - lastprefetchhit + 1))));
	}

	prefetchamount = (prefetchamount <= maxprefetchamount) ? prefetchamount : maxprefetchamount;
	if (prefetchamount < lastprefetchamount / 2) {
		prefetchamount = lastprefetchamount / 2;
	}
	lastprefetchhit = prefetchHitCount;

	lastprefetchamount = prefetchamount;

	return prefetchamount;

}
void leapClass::reset() {
	hbuffer.clear();
	//windowsize = maxbuffersize ;
	splitvalue = 8;
	lastprefetchamount = 0;
	lastprefetchhit = 0;
}

void leapClass::setvalues(uint64_t bsize, uint64_t svalue) {
	maxbuffersize = bsize;
	splitvalue = svalue;
}