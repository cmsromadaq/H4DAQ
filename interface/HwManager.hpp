
#ifndef HW_MANAGER_H
#define HW_MANAGER_H

#include "interface/StandardIncludes.hpp"

class EventBuilder;
class Board;

#include "interface/Configurator.hpp"
#include "interface/Board.hpp"
#include "interface/BoardConfig.hpp"
#include "interface/Logger.hpp"
//#include "interface/EventBuilder.hpp"


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
	// -- Board associated to trigger (normally the crate controller)
	boardPtr trigBoard_;
        // -- Board associated to the ioControl (the first in the board list to be either CAENV513 or CAENV262)
	boardPtr ioControlBoard_;
	// -- Controller Board (the first in the board list to be CAENVX718)
	boardPtr controllerBoard_; 
	// -- Digitizer Board
	boardPtr digiBoard_;
	// -- Crate ID
	int crateId_;
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
	void SetBusyOff();
	void SetBusyOn();
	// --- RunControl Utility
	bool HaveControlBoard(){ if (ioControlBoard_.boardIndex_ >=0 ) return true ; else return false; };

	bool SignalReceived(CMD_t signal);
        void SetTriggerStatus(TRG_t triggerType, TRG_STATUS_t triggerStatus); 
        void ClearSignalStatus(); 
	// --- Promote To Run Control
	//inline void SetRunControl(bool rc=true){ runControl_=rc;}
	//inline bool IsRunControl() const {return runControl_;}
	static BoardTypes_t GetBoardTypeId(string type);
	// --- Get the N. of boards that will be read and add to the event stream
	inline WORD GetNboards(){ return hw_.size();};

private:
  //Initialize VME crate
  int CrateInit();
};

#endif
