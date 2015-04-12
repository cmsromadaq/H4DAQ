#include "interface/MAROC_RO.hpp"
#include <iostream>
#include <sstream>
#include <string>

//#define CAENV792_DEBUG 

int MAROC_ROC::Init()
{
  int status=0;
  ostringstream s; s << "[MAROC_ROC]::[INFO]::++++++  INIT ++++++";
  Log(s.str(),1);
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  if (!IsConfigured())
    return ERR_CONF_NOT_FOUND;


  //Load configuration on Maroc FEB (829 bits)
  LoadFEBConfiguration();

  //Clear Conf
  WORD data=0x0;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROCC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  if (status)
    {
      s.str(""); s << "[MAROC_ROC]::[ERROR]::Cannot open  board @0x" << std::hex << configuration_.baseAddress << std::dec; 
      Log(s.str(),1);
      return ERR_OPEN;
    }    

  s.str(""); s << "[MAROC_ROC]::[INFO]::Open  board @0x" << std::hex << configuration_.baseAddress << std::dec;
  Log(s.str(),1);

  int run_mode = configuration_.debugMode==1 ? 0xd : 0x0; 
  //Put the system in readout mode
  status |= SendOnFEBBus(3,run_mode);
  status |= SendOnFEBBus(8,0);

  s.str(""); s << "[MAROC_ROC]::[INFO]::Board in readout mode";
  Log(s.str(),1);
    
  status |= InitADC();
  s.str(""); s << "[MAROC_ROC]::[INFO]::Init ADC";
  Log(s.str(),1);

  status |= ConfigFEBReadoutHold();
  s.str(""); s << "[MAROC_ROC]::[INFO]::FEB readout configured: hold " << configuration_.holdValue << " hold2 delay " << configuration_.holdDeltaValue;
  Log(s.str(),1);


  status |= ConfigFEBTrigger();
  status |= ConfigROCTrigger();
  s.str(""); s << "[MAROC_ROC]::[INFO]::Trigger input configured";
  Log(s.str(),1);

  status |= SetADCClock(400);
  status |= SetADCEnableDReset();
  status |= SetADCTestOff();
  status |= SetADCNormalReadoutMode();
  status |= SetADCZeroSuppressionMode();
  status |= SetADCSlowHold(20,0);
  s.str(""); s << "[MAROC_ROC]::[INFO]::ADC configured";
  Log(s.str(),1);

  status |= ResetTimeStamp();
  
  if (status)
    {
      s.str(""); s << "[MAROC_ROC]::[ERROR]::Config error" << status;
      Log(s.str(),1);
      return ERR_CONFIG;
    } 

  s.str(""); s << "[MAROC_ROC]::[INFO]::++++++  CONFIGURED ++++++";  
  Log(s.str(),1);
  return 0;
} 

int MAROC_ROC::Clear()
{

  //Send a software reset. Module has to be re-initialized after this
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  status |= GlobalReset();
  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Cannot reset  board " << status; 
      Log(s.str(),1);
      return ERR_RESET;
    }

  status=Init();
  status|=BufferClear();
  return status;
}      

int MAROC_ROC::BufferClear()
{
  //Send a data reset. Clear event buffer and counters 
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  status |= ResetFIFO();
  status |= SetMemoryMode(true);
  status |= ClearADCBusy();
  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_INPUT_CONNECTOR_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Cannot clear buffers  board " << status; 
      Log(s.str(),1);
      return ERR_RESET;
    }  

  return 0;
}      

int MAROC_ROC::Config(BoardConfig *bC)
{
  Board::Config(bC);
  GetConfiguration()->baseAddress=Configurator::GetInt( bC->getElementContent("baseAddress"));
  
  GetConfiguration()->model=static_cast<MAROC_ROC_Model_t>(Configurator::GetInt( bC->getElementContent("model")));
  
  GetConfiguration()->blkEnd=static_cast<bool>(Configurator::GetInt( bC->getElementContent("blkEnd")));
  GetConfiguration()->zeroSuppression=static_cast<bool>(Configurator::GetInt( bC->getElementContent("zeroSuppression")));
  GetConfiguration()->emptyEnable=static_cast<bool>(Configurator::GetInt( bC->getElementContent("emptyEnable")));
  GetConfiguration()->overRange=static_cast<bool>(Configurator::GetInt( bC->getElementContent("overRange")));
  
  GetConfiguration()->iped=Configurator::GetInt( bC->getElementContent("iped"));
  GetConfiguration()->zsThreshold=Configurator::GetInt( bC->getElementContent("zsThreshold"));

  return 0;
}

//Read the ADC buffer and send it out in WORDS vector
int MAROC_ROC::Read(vector<WORD> &v)
{
  v.clear();
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int status = 0; 
  WORD dataV[MAROC_ROC_BOARDDATA_NWORD]; //each event is composed of max (channels_+2)x32bit words. Reserve space for a single event
  //Empty event buffer
  for (unsigned int i=0;i<channels_+2;++i)
    dataV[i]=0;

  WORD data;
  int adc_rdy=0;
  int adc_busy=1;

  int ntry = 1000, nt = 0;
  //Wait for a valid datum in the ADC
  while ( (adc_rdy != 1 || adc_busy!=0 ) && nt<ntry )
    {
      // status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_REG1_STATUS,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
      // v792_rdy = data & MAROC_ROC_RDY_BITMASK;
      // v792_busy  = data & MAROC_ROC_BUSY_BITMASK;
      ++nt;
    }

  if (status || adc_rdy==0 || adc_busy==1)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Cannot get a valid data from  board " << status; 
      Log(s.str(),1);
      return ERR_READ;
    }  


  SetMemoryMode(false);
  for(unsigned int iWord=0;iWord<MAROC_ROC_BOARDDATA_NWORD;++iWord)
    {
      //Read words one by one
    }
  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error while reading data from  board " << status ; 
      Log(s.str(),1);
      return ERR_READ;
    }  
  
//   int nWordsRead=nbytes_tran/sizeof(WORD);
//   for (int i=0;i<nWordsRead;++i)
//     {
//       int wordType=(dataV[i] & MAROC_ROC_EVENT_WORDTYPE_BITMASK)>>24;
//       if (wordType == MAROC_ROC_EVENT_DATA ||
// 	  wordType == MAROC_ROC_EVENT_BOE ||
// 	  wordType == MAROC_ROC_EVENT_EOE 
// 	  )
// 	{
// 	  v.push_back(dataV[i]); //Filling event buffer
// #ifdef CAEN_DEBUG
// 	  if (i==0 && wordType != MAROC_ROC_EVENT_BOE)
// 	    {
// 	      ostringstream s; s << "[MAROC_ROC]::[WARNING]::First Word Not BOE " << wordType; 
// 	      Log(s.str(),1);
// 	    }
// 	  if (i==nWordsRead-1 && wordType != MAROC_ROC_EVENT_EOE)
// 	    {
// 	      ostringstream s; s << "[MAROC_ROC]::[WARNING]::Last Word Not EOE " << wordType; 
// 	      Log(s.str(),1);
// 	    }
// 	  if (wordType == MAROC_ROC_EVENT_DATA)
// 	    {
// 	      short adc_chan = dataV[i]>>16 & 0x1F; //For 792 [bit 16-20]
// 	      unsigned int adc_value = dataV[i] & 0xFFF; // adc data [bit 0-11]
// 	      bool adc_overflow = (dataV[i]>>12) & 0x1; // overflow bit [bit 12]
// 	      bool adc_underthreshold = (dataV[i]>>13) & 0x1; // under threshold bit [bit 13]
// 	      ostringstream s; s << "[MAROC_ROC]::[INFO]::Read Channel " << "\tchannel " << adc_chan << "\tvalue " << adc_value << "\toverflow " << adc_overflow << "\tunderthreshold " << adc_underthreshold; 
// 	      Log(s.str(),1);
// 	    }
// #endif
// 	}
//       else
// 	{
// 	  ostringstream s; s << "[MAROC_ROC]::[WARNING]::Invalid data read from  board " << wordType; 
// 	  Log(s.str(),1);
// 	}
//     }


//   status |= CheckStatusAfterRead();
  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::MEB !Empty or problems reading" << status; 
      Log(s.str(),1);
      return ERR_READ;
    }  

  return 0;
}

int MAROC_ROC::CheckStatusAfterRead()
{
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int status = 0; 
  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_REG2_STATUS,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Cannot get status after read from  board " << status; 
      Log(s.str(),1);
    }  

  int v792_full = data & MAROC_ROC_FULL_BITMASK;
  int v792_empty = data & MAROC_ROC_EMPTY_BITMASK; 
  

   if( v792_full || !v792_empty || status!=1 ) 
     { 
       status=BufferClear();
       if (status)
	   status=Clear();
     }

   if (status)
     {
       ostringstream s; s << "[MAROC_ROC]::[ERROR]::Cannot restore healthy state in  board " << status; 
      Log(s.str(),1);
      return ERR_READ;
     }  

  return 0;
}

