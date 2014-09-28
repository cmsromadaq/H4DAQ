//Contains some functions
//
//
#ifndef UTILITY_H
#define UTILITY_H


#include "interface/StandardIncludes.hpp"
namespace Utility{

	string AsciiData(void *data,int N,int M=5);
	string AsciiDataReadable(void *data,int N);
	string AsciiData(void *data,void *data2,int N,int M=5);
	
	string ConvertToString(int N); // c++11 has to_string
	
	long timevaldiff(struct timeval *starttime, struct timeval *finishtime) ;
	
	long timestamp(struct timeval *time, time_t *ref);

  int hibit(unsigned int n);
};



#endif
