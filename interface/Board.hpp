#ifndef BOARD_H
#define BOARD_H

#include "interface/StandardIncludes.hpp"
#include "interface/Logger.hpp"

enum BoardTypes_t { _TIME_=1, //Fake board to get timestamps
		    _CAENVX718_=2, //Crate Controller 
		    _CAENV1742_=3, //5Gs Digitiser
		    _CAENV513_=4,  //I/O Register
		    _CAENV262_=5, //I/O Register
		    _CAENV792_=6, //32ch Charge ADC
		    _CAENV1290_=7, //16ch TDC
		    _CAENV1495PU_=8, //General Purpose I/O. FW 2.1 PatternUnit
		    _CAENV560_=9, //Scaler
		    _CAENV814_=10, //Low Edge Discriminator
		    _LECROY1182_=11, //8ch ADC 50fC per count 
		    _MAXBOARDTYPE_, // USED By DQM
		    _UNKWN_=0 };

class BoardConfig;

class Board: public LogUtility  { // don't inheriths from configurable 'cause I use BoardConfig
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
	// --- Get Id
	inline unsigned int GetId(){return id_;};
	inline void SetId(unsigned int id){id_=id;};
	// --- GetType
	inline string GetType() const { return type_;}
	// --- Configurable  
	virtual int Init()=0;
	virtual int Clear()=0;
	virtual int Print()=0;
	// --- Private members that must exist for each board
	virtual int BufferClear()=0;
	virtual int Config(BoardConfig *bC)=0;
        inline bool IsConfigured() const {return bC_!=0;};
	virtual int Read(vector<WORD> &v)=0;
	virtual int SetHandle(int)=0;
};

class TriggerBoard : public Board {
protected:

public:
	// --- Constructor. Calls Constructor Board
	TriggerBoard() : Board() {};
	// --- this are meaningful only for trigger boards. 
	virtual inline int  ClearBusy(){return 0;};
	virtual inline int  SetBusyOff(){return 0;};
	virtual inline int  SetBusyOn(){return 0;};
	virtual inline bool TriggerReceived(){return false;};
	virtual inline int  TriggerAck(){return 0;};

};

class IOControlBoard : public Board {
protected:

public:
	// --- Constructor. Calls Constructor Board
	IOControlBoard() : Board() {};
	// --- this are meaningful only for ioControl boards. 
	virtual inline bool  SignalReceived(CMD_t signal)=0;
        virtual inline int   SetTriggerStatus(TRG_t triggerType, TRG_STATUS_t triggerStatus)=0;
};

#endif
