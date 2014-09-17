#include "interface/StandardIncludes.hpp"
#include "interface/Configurator.hpp"
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
	// --- Configurable
	virtual int  Init()=0;
	virtual int Clear()=0;
	virtual int BufferClear()=0;
	virtual int Config(BoardConfig *bC)=0;
	virtual int Read(vector<WORD> &v)=0;
	virtual int SetHandle(WORD x)=0;

};

class HwManager: public Configurable, public LogUtility
{
/* this class take care of the hardware configuration
 * read the boards and send back the results to the 
 * control manager
 */
protected:
	vector<Board*> hw_;
	// --- Read Board i
	void Read(int i,vector<WORD> &v);

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
	inline void BufferClearAll(){};
	dataType ReadAll();
};
