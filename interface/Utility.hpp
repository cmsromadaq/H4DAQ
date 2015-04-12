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
	
	unsigned long timestamp(struct timeval *time, time_t *ref);
	string now();
	void SpaceToNull(int N,void*data,bool first=false);
	int  FindNull(int N,void*data,int iPos=1);
        
  void setbit(unsigned int* data,int bit);
  void setbit(int* data,int bit);
  void clearbit(unsigned int* data,int bit);
  void clearbit(int* data,int bit);

  int hibit(unsigned int n);
  int lowbit(uint32_t n);
};



#endif
