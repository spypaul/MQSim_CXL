#pragma once
#pragma once
#include <iostream>

#include <map>

using namespace std;

class LRUNode {
public:
	uint64_t page_num{ 0 };
	bool dirty{ 0 };
	LRUNode* left{ NULL };
	LRUNode* right{ NULL };
};

class CFLRU {
public:
	CFLRU() {};
	CFLRU(uint64_t cache_size);
	~CFLRU();
	void add(uint64_t pn);
	uint64_t pop();
	void modify(uint64_t pn, bool dirty);
private:
	uint64_t window{ 0 };
	LRUNode* head{ NULL };
	LRUNode* tail{ NULL };
	map<uint64_t, LRUNode*> m;



};