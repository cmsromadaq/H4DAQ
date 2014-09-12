#include "interface/StandardIncludes.hpp"

#ifndef PROF_H
#define PROF_H
class Profiler
{
private:
	//this class takes care of reading time and compute the time spent in each operation
	time_t begin_;
	time_t end_;
	clock_t clockBegin_;
	clock_t clockEnd_;
public:	
	// --- Constructor
	Profiler();
	~Profiler();
	void Start();
	void Stop();
	int GetRealTime();
	float GetCPUTime();

};

#endif
