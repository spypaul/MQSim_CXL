#pragma once
#include <iostream>
#include <fstream>

using namespace std;
class outlog {
public:
	ofstream of;
	outlog(string filename);
	~outlog();
};

//extern outlog outputf;
extern outlog evictf;
extern ofstream of_overall;
