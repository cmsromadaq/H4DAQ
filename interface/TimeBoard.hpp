#include "interface/StandardIncludes.hpp"
#include "interface/HwManager.hpp"
#include "interface/BoardConfig.hpp"


#ifndef TIME_BOARD_H
#define TIME_BOARD_H

class TimeBoard : public Board {
// this class implements a dummy board that will be put in the event containing the time it is called
public:
	int Init() ;
	int Clear();
	int Print();
	int BufferClear();
	int Config(BoardConfig*);
	int Read(vector<WORD> &v);
	int SetHandle(WORD x);
};

#endif
