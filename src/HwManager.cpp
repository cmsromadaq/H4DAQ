#include "interface/HwManager.hpp"
#include "interface/CAEN_VX718.hpp"
#include "interface/BoardConfig.hpp"

// --- Board
Board::Board(){id_=0;bC_=NULL;};
Board::~Board(){};
int Board::Config(BoardConfig *bC){
	bC_=bC;
	return 0;
};
//unsigned int Board::GetId(){return id_;};

// -------------------  HW Manager ---------------
//
// --- Constructor
HwManager::HwManager(){

  trigBoard_.boardIndex_=-1;
  trigBoard_.boardHandle_=-1;

  controllerBoard_.boardIndex_=-1;
  controllerBoard_.boardHandle_=-1;

  digiBoard_.boardIndex_=-1;
  digiBoard_.boardHandle_=-1;

  //runControl_=false;
}
// --- Destructor
HwManager::~HwManager(){}
// --- Configure the HwManager
void HwManager::Config(Configurator &c){
	// take the configurator. Translate it into a Board config. Configure the board
	xmlNode *hw_node = NULL;	
	//locate Hardware Node
	for (hw_node = c.root_element->children; hw_node ; hw_node = hw_node->next)
	{
		if (hw_node->type == XML_ELEMENT_NODE &&
				xmlStrEqual (hw_node->name, xmlCharStrdup ("Hardware"))  )
			break;
	}
	if ( hw_node== NULL ) {
		printf("Hardware not found in configuration\n");
		throw  config_exception();
		}

	// locate each board node and extract info
	xmlNode *board_node = NULL;	
	for ( board_node=hw_node->children; board_node ; board_node = board_node->next)
	{
		if (  board_node->type == XML_ELEMENT_NODE 	
				&& xmlStrEqual (board_node->name, xmlCharStrdup ("board")) )
		{
		int ID=Configurator::GetInt(getElementContent(c, "ID" , board_node));
		Log("[2] Configuring Board ID="+ getElementContent(c, "ID" , board_node)+"type=" + getElementContent(c, "type" , board_node),2);
		//TODO -- construct the board -- if elif ... else throw exception 
		if ( getElementContent(c,"type",board_node) == "CAEN_VX718" )
			{
				// keep the index where I'm constructing stuff
				int bIdx=hw_.size();
				// construct a CAEN_VX718 Board, and push it back
				hw_.push_back( new CAEN_VX718() );
				// construct a board configurator and ask the board to configure itself
				BoardConfig bC;
				bC.Init(c);
				bC.SetBoardNode(board_node);
				hw_[bIdx]->Config(&bC);
			}
		// else if ( ... ==... ) ... TODO
		}
	}

	return;	
}
// --- Init
void HwManager::Init(){
  //Crate init
  if (hw_.size()>0 )
  	CrateInit();
  trigBoard_.boardIndex_=controllerBoard_.boardIndex_;
  for(unsigned int i=0;i<hw_.size();i++)
	{
	  if (hw_[i]->GetType() != "CAEN_V1742" )
	    hw_[i]->SetHandle(controllerBoard_.boardHandle_);
	  else
	    hw_[i]->SetHandle(digiBoard_.boardHandle_);
		int R=hw_[i]->Init();
		if ( R )  
		  {
		    ostringstream s;
		    s << "[HwManager]::[ERROR]::Error configuring board " << hw_[i]->GetType() << " exit code " << R;
		    Log(s.str(),1);
		    throw hw_exception();
		  }
	}
}

// --- Crate Init
int HwManager::CrateInit()
{
  int status =0;

  for(unsigned int i=0;i<hw_.size();i++)
    {
      if ( hw_[i]->GetType() == "CAEN_V1742" )
	{
	  digiBoard_.boardIndex_=i;
	  break; //do not support >1 digitizer board for the moment...
	}
    }

  for(unsigned int i=0;i<hw_.size();i++)
    {
      if ( hw_[i]->GetType() == "CAEN_VX718" )
	{
	  controllerBoard_.boardIndex_=i;
	  break; //do not support >1 digitizer board for the moment...
	}
    }
 
  if (controllerBoard_.boardIndex_<0)
    {
      Log("[HwManager]::[ERROR]::Cannot find controller board",1);
      throw config_exception();
    }
  
  if (digiBoard_.boardIndex_<0)
    {
      CAEN_VX718::CAEN_VX718_Config_t* controllerConfig=((CAEN_VX718*)hw_[controllerBoard_.boardIndex_])->GetConfiguration();

      status |= CAENVME_Init(controllerConfig->boardType, controllerConfig->LinkType, controllerConfig->LinkNum, &controllerBoard_.boardHandle_);

      if (status)
	{
	  ostringstream s;
	  s<<"[HwManager]::[ERROR]::VME Crate Init ERROR. BT "<<controllerConfig->boardType<<" LT "<<controllerConfig->LinkType<<" LN "<<controllerConfig->LinkNum;
	  Log(s.str(),1);
	  throw config_exception();
	}
      Log("[HwManager]::[INFO]::VME Crate Initialized",1);
    }
  else
    {
      //have digitizer
      // TODO
    }

  if (controllerBoard_.boardHandle_<0)
    {
      Log("[HwManager]::[ERROR]::VME Crate Controller Handle is wrong",1);
      throw config_exception();
    }

  status |= CAENVME_SystemReset(controllerBoard_.boardHandle_);
  if (status)
    {
      Log("[HwManager]::[ERROR]::VME Crate RESET ERROR",1);
      throw config_exception();
    }

  usleep(10000); 

  return 0;

}

// --- Clear
void HwManager::Clear(){
	// --- reset to un-initialized/ un-config state	
	//runControl_=false;
	for(int i=0;i<hw_.size();i++)
		hw_[i]->Clear();
	return;
}

void HwManager::Print(){
	Log("[2] Printing configuratio",2);
	for(vector<Board*>::iterator iBoard=hw_.begin();iBoard!=hw_.end();iBoard++)
		{
		int r = (*iBoard)->Print();// 0-1 are ok status
		if(r)Log( string("[2] Error on Board")+(*iBoard)->GetType(),2);
		}
	return ; 
}


void HwManager::Read(int i,vector<WORD> &v)
{
	v.clear();
	hw_[i]->Read(v);
	return;
}

void HwManager::ReadAll(dataType&R){
	EventBuilder::EventHeader(R);
	WORD M=hw_.size();
	EventBuilder::WordToStream(R,M);
	vector<WORD> v; 
	for(int i=0;i< hw_.size();i++)
	{
		hw_[i]->Read(v);
		EventBuilder::BoardToStream( R, hw_[i]->GetId(), v )  ;
	}
	EventBuilder::EventTrailer(R);
	return ;
}

void  HwManager::BufferClearAll(){
	for(int i=0;i< hw_.size();i++)
		hw_[i]->BufferClear();
	return;
}


void HwManager::ClearBusy(){
	if (trigBoard_.boardIndex_<0 ) return;
	hw_[trigBoard_.boardIndex_]->ClearBusy();
	return;
}

bool HwManager::TriggerReceived(){
	if (trigBoard_.boardIndex_<0 ) return false;
	return hw_[trigBoard_.boardIndex_]->TriggerReceived();
}

int HwManager::TriggerAck(){
	if (trigBoard_.boardIndex_<0) return -1;
	return hw_[trigBoard_.boardIndex_]->TriggerAck();
}
