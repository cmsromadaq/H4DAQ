
#ifndef HW_MANAGER_H
#define HW_MANAGER_H

#include "interface/StandardIncludes.hpp"
#include "interface/Configurator.hpp"
#include "interface/BoardConfig.hpp"
#include "interface/Logger.hpp"
#include "interface/EventBuilder.hpp"
#include "interface/BoardConfig.hpp"

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

class HwManager: public Configurable, public LogUtility
{
/* this class take care of the hardware configuration
 * read the boards and send back the results to the 
 * control manager
 */
protected:
  typedef struct 
  {
    // index in the hw_ array
    int boardIndex_;
    // handle (in case it applies)
    int boardHandle_;
  } boardPtr;

	vector<Board*> hw_;
	// --- Read Board i
	void Read(int i,vector<WORD> &v);

  // ----- Special functions boards -----
	// -- Board associated to trigger
	boardPtr trigBoard_;
	// -- Controller Board
	boardPtr controllerBoard_;
	// -- Digitizer Board
	boardPtr digiBoard_;
  // ------------------------------------
 	// -- Flag to set if it has the SPS Boards or not
	//bool runControl_;

public:
	// --- Constructor
	HwManager();
	// --- Destructor
	~HwManager();
	// --- Configurable
	void Init();
	void Clear();
	void Config(Configurator&c);
	// --- Read All the Boards
	void BufferClearAll();
	// Return Event
	void ReadAll(dataType &);
	void Print();
	// --- Trigger Utility
	void ClearBusy();
	bool TriggerReceived();
	void TriggerAck();
	// --- Promote To Run Control
	//inline void SetRunControl(bool rc=true){ runControl_=rc;}
	//inline bool IsRunControl() const {return runControl_;}

private:
  //Initialize VME crate
  int CrateInit();
};

#endif
