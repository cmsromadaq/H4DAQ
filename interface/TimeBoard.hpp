#ifndef TIME_BOARD_H
#define TIME_BOARD_H

#include "interface/StandardIncludes.hpp"
#include "interface/HwManager.hpp"


class TimeBoard : public Board {
// this class implements a dummy board that will be put in the event containing the time it is called
time_t ref;
public:
	TimeBoard();
	int Init() ;
	int Clear();
	int Print();
	inline int BufferClear(){};
	int Config(BoardConfig*);
	int Read(vector<WORD> &v);
	int SetHandle(int x);
};

#endif
