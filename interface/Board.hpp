#ifndef BOARD_H
#define BOARD_H

#include "interface/StandardIncludes.hpp"

enum BoardTypes_t { _TIME_=1, _CAENVX718_=2, _CAENV1742_=3, _UNKWN_=0 };

class BoardConfig;

class Board  { // don't inheriths from configurable 'cause I use BoardConfig
protected:
	// this is the BaseClass. Each other class needs to implement this
	unsigned int id_;
	string type_;
	BoardConfig *bC_; // don't destroy here
public:
	// --- Constructor 
	Board();
	// --- Destructor
	~Board();
	// -- Get Id
	inline unsigned int GetId(){return id_;};
	// --- GetType
	inline string GetType() const { return type_;}
	// --- Configurable  
	virtual int Init()=0;
	virtual int Clear()=0;
	virtual int Print()=0;
	virtual int BufferClear()=0;
	virtual int Config(BoardConfig *bC)=0;
        inline bool IsConfigured() const {return bC_!=0;};
	virtual int Read(vector<WORD> &v)=0;
	virtual int SetHandle(int)=0;
	// --- this are meaningful only for trigger boards
	virtual inline int ClearBusy(){return 0;};
	virtual inline bool TriggerReceived(){return false;};
	virtual inline int TriggerAck(){return 0;};
};

#endif
