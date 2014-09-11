#include "interface/StandardIncludes.hpp"

#ifndef PROF_H
#define PROF_H
class Profiler()
{
	//this class takes care of reading time and compute the time spent in each operation
	time_t begin_;
	time_t end_;
	clock_t clockBegin_;
	clock_t clockEnd_;
public:	
	void Start();
	void Stop();
	int GetRealTime();
	int GetCPUTime();

};

#endif
