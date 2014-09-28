#include "interface/CAEN_V792.hpp"
#include <iostream>
#include <sstream>
#include <string>

#define CAENV792_DEBUG 

int CAEN_V792::Init()
{
  int status=0;
  ostringstream s; s << "[CAEN_V792]::[INFO]::++++++ CAEN V792 INIT ++++++";
  Log(s.str(),1);
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  if (!IsConfigured())
    return ERR_CONF_NOT_FOUND;

  //Read Version to check connection
  WORD data=0;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V792_FW,&data,CAEN_V792_ADDRESSMODE,CAEN_V792_DATAWIDTH);
  if (status)
    {
      s.str(""); s << "[CAEN_V792]::[ERROR]::Cannot open V792 board @0x" << std::hex << configuration_.baseAddress << std::dec; 
      Log(s.str(),1);
      return ERR_OPEN;
    }    

  unsigned short FWVersion[4];
  for (int i=0;i<4;++i)
    FWVersion[i]=(data>>(i*4))&0xF;

  s.str(""); s << "[CAEN_V792]::[INFO]::Open V792 board @0x" << std::hex << configuration_.baseAddress << std::dec << " FW Version Rev." << FWVersion[3] << FWVersion[2] << "." << FWVersion[1] << FWVersion[0];
  Log(s.str(),1);

  data=CAEN_V792_SOFTRESET_BITMASK; //Software reset. Clear registers
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V792_BIT_SET1,&data,CAEN_V792_ADDRESSMODE,CAEN_V792_DATAWIDTH);
  usleep(100);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V792_BIT_CLEAR1,&data,CAEN_V792_ADDRESSMODE,CAEN_V792_DATAWIDTH);
  if (status)
    {
      s.str(""); s << "[CAEN_V792]::[ERROR]::Cannot reset V792 board " << status; 
      Log(s.str(),1);
      return ERR_RESET;
    }    

  //Write ControlRegister1
  data  = 0;
  data |= configuration_.blkEnd*CAEN_V792_BLKEND_BITMASK; 
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V792_REG1_CONTROL,&data,CAEN_V792_ADDRESSMODE,CAEN_V792_DATAWIDTH);

  //Write BitSet2
  data  = 0;
  data = ((!configuration_.zeroSuppression)*CAEN_V792_ZS_BITMASK) | (configuration_.overRange*CAEN_V792_OVERRANGE_BITMASK) | (configuration_.emptyEnable*CAEN_V792_EMPTYEN_BITMASK); 
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V792_BIT_SET2,&data,CAEN_V792_ADDRESSMODE,CAEN_V792_DATAWIDTH);

  channels_=32;
  if (configuration_.model == CAEN_V792_N)
    channels_=16;

  data=0;
  data|=configuration_.zsThreshold & 0xFF; //8bit encoded threshold, 9th bit used to kill a particular channel

  int zsSize=2;
  if (configuration_.model == CAEN_V792_N)
    zsSize=4;

  for(unsigned int i=0; i<channels_; i++) 
      status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V792_ZS_THR+i*zsSize,&data,CAEN_V792_ADDRESSMODE,CAEN_V792_DATAWIDTH);

  data=0;
  data|=configuration_.iped & 0xFF; //8bit encoded ped current
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V792_IPED,&data,CAEN_V792_ADDRESSMODE,CAEN_V792_DATAWIDTH);
  
  if (status)
    {
      s.str(""); s << "[CAEN_V792]::[ERROR]::Config error" << status;
      Log(s.str(),1);
      return ERR_CONFIG;
    } 
  s.str(""); s << "[CAEN_V792]::[INFO]::++++++ CAEN V792 CONFIGURED ++++++";  
  Log(s.str(),1);
  return 0;
} 

int CAEN_V792::Clear()
{

  //Send a software reset. Module has to be re-initialized after this
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data=CAEN_V792_SOFTRESET_BITMASK; //Software reset. Clear registers
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V792_BIT_SET1,&data,CAEN_V792_ADDRESSMODE,CAEN_V792_DATAWIDTH);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V792_BIT_CLEAR1,&data,CAEN_V792_ADDRESSMODE,CAEN_V792_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[CAEN_V792]::[ERROR]::Cannot reset V792 board " << status; 
      Log(s.str(),1);
      return ERR_RESET;
    }

  status=Init();
  return status;
}      

int CAEN_V792::BufferClear()
{
  //Send a data reset. Clear event buffer and counters 
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data=CAEN_V792_DATARESET_BITMASK; //Software reset. Clear registers
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V792_BIT_SET2,&data,CAEN_V792_ADDRESSMODE,CAEN_V792_DATAWIDTH);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V792_BIT_CLEAR2,&data,CAEN_V792_ADDRESSMODE,CAEN_V792_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[CAEN_V792]::[ERROR]::Cannot clear buffers V792 board " << status; 
      Log(s.str(),1);
      return ERR_RESET;
    }  

  return 0;
}      

int CAEN_V792::Config(BoardConfig *bC)
{
  Board::Config(bC);
  return 0;
}

//Read the ADC buffer and send it out in WORDS vector
int CAEN_V792::Read(vector<WORD> &v)
{
  v.clear();
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int status = 0; 
  WORD dataV[channels_+2]; //each event is composed of max (channels_+2)x32bit words. Reserve space for a single event
  //Empty event buffer
  for (unsigned int i=0;i<channels_+2;++i)
    dataV[i]=0;

  WORD data;
  int v792_rdy=0;
  int v792_busy=1;

  int ntry = 100, nt = 0;
  //Wait for a valid datum in the ADC
  while ( (v792_rdy != 1 || v792_busy!=0 ) && nt<ntry )
    {
      status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V792_REG1_STATUS,&data,CAEN_V792_ADDRESSMODE,CAEN_V792_DATAWIDTH);
      v792_rdy = data & CAEN_V792_RDY_BITMASK;
      v792_busy  = data & CAEN_V792_BUSY_BITMASK;
      ++nt;
    }

  if (status)
    {
      ostringstream s; s << "[CAEN_V792]::[ERROR]::Cannot get a valid data from V792 board " << status; 
      Log(s.str(),1);
      return ERR_READ;
    }  

#ifdef CAENV792_DEBUG
  ostringstream s; s << "[CAEN_V792]::[DEBUG]::RDY " << v792_rdy << " BUSY " << v792_busy << " NTRY " << nt;
  Log(s.str(),3);
#endif

  int wr=(channels_+2)*sizeof(WORD); //number of expected words to read
  int nbytes_tran=0; //transferred bytes

  status |= CAENVME_BLTReadCycle(handle_,configuration_.baseAddress+CAEN_V792_OUTPUT_BUFFER,dataV,wr,CAEN_V792_ADDRESSMODE,cvD32,&nbytes_tran);

  if (status || nbytes_tran<=0 )
    {
      ostringstream s; s << "[CAEN_V792]::[ERROR]::Error while reading data from V792 board " << status ; 
      Log(s.str(),1);
      return ERR_READ;
    }  
  
  int nWordsRead=nbytes_tran/sizeof(WORD);
  for (int i=0;i<nWordsRead;++i)
    {
      int wordType=dataV[i] & CAEN_V792_EVENT_WORDTYPE_BITMASK;
      if (wordType == CAEN_V792_EVENT_DATA ||
	  wordType == CAEN_V792_EVENT_BOE ||
	  wordType == CAEN_V792_EVENT_EOE 
	  )
	v.push_back(dataV[i]); //Filling event buffer
      else
	{
	  ostringstream s; s << "[CAEN_V792]::[ERROR]::Invalid data read from V792 board " << wordType; 
	  Log(s.str(),1);
	}
    }


  status |= CheckStatusAfterRead();
  if (status)
    {
      ostringstream s; s << "[CAEN_V792]::[ERROR]::MEB !Empty or problems reading" << status; 
      Log(s.str(),1);
      return ERR_READ;
    }  

  return 0;
}

int CAEN_V792::CheckStatusAfterRead()
{
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int status = 0; 
  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V792_REG2_STATUS,&data,CAEN_V792_ADDRESSMODE,CAEN_V792_DATAWIDTH);
  if (status)
    {
      ostringstream s; s << "[CAEN_V792]::[ERROR]::Cannot get status after read from V792 board " << status; 
      Log(s.str(),1);
    }  

  int v792_full = data & CAEN_V792_FULL_BITMASK;
  int v792_empty = data & CAEN_V792_EMPTY_BITMASK; 
  

   if( v792_full || !v792_empty || status!=1 ) 
     { 
       status=BufferClear();
       if (status)
	   status=Clear();
     }

   if (status)
     {
       ostringstream s; s << "[CAEN_V792]::[ERROR]::Cannot restore healthy state in V792 board " << status; 
      Log(s.str(),1);
      return ERR_READ;
     }  

  return 0;
}

