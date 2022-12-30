#pragma once

#include <iostream>
#include <list>
#include <vector>
#include <set>
#include <map>

using namespace std;

class boClass {
private:
	list<uint64_t> rrtable;
	uint64_t maxtablesize{ 1024 };
	uint64_t maxoffset{ 128 };
	vector <uint64_t> offsetscore;
	uint64_t badscore{ 3 };
	uint64_t maxround{ 8 };
	uint64_t boprefetchK{ 16 };
public:
	bool prefetch_on{ 0 };
	uint64_t offset{ 0 };
	uint64_t offsetundertest{ 1 };
	uint64_t round{ 0 };
	set<uint64_t> olist;

	boClass();
	void bosetvalues(uint64_t hsize, uint64_t maxoff, uint64_t  b, uint64_t r);
	void addhistory(uint64_t addr);
	void incrementscore(uint64_t offset);
	void resetscore();
	uint64_t findmax();
	void findoffsets();
	bool validoffset(uint64_t offset);
	bool inhistory(uint64_t addr);
	bool endround();
	bool endlphase();
	void reset();
};


class leapClass {
private:
	list<pair<uint64_t, int64_t>> hbuffer;
	uint64_t maxbuffersize{ 39 };
	//uint64_t windowsize{ maxbuffersize };
	uint64_t splitvalue{ 8 };
	const uint64_t maxprefetchamount{ 16 };
	uint64_t lastprefetchamount{ 0 };
	uint64_t lastprefetchhit{ 0 };

	uint64_t leapinitialprefetchamount{ 1 };
public:
	int64_t last_offset{ 0 };
	void setvalues(uint64_t bsize, uint64_t svalue);
	void leapinit();
	int64_t findoffset();
	void historyinsert(uint64_t addr);
	uint64_t getk(uint64_t prefetchHitCount);
	void reset();
};