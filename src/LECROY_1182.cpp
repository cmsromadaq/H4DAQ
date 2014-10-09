#include "interface/LECROY_1182.hpp"
#include <iostream>
#include <sstream>
#include <string>

#define LECROY1182_DEBUG 

int LECROY_1182::Init()
{
  int status=0;
  ostringstream s; s << "[LECROY_1182]::[INFO]::++++++ LECROY 1182 INIT ++++++";
  Log(s.str(),1);
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  if (!IsConfigured())
    return ERR_CONF_NOT_FOUND;

  WORD data= configuration_.clrRegWord;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+LECROY_1182_CLR0_REG,&data,LECROY_1182_ADDRESSMODE,LECROY_1182_DATAWIDTH);
  usleep(100);

  //Now reset the module
  data |= LECROY_1182_DATARESET_BITMASK;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+LECROY_1182_CLR0_REG,&data,LECROY_1182_ADDRESSMODE,LECROY_1182_DATAWIDTH);
  if (status)
    {
      s.str(""); s << "[LECROY_1182]::[ERROR]::Cannot reset LECROY_1182 board " << status; 
      Log(s.str(),1);
      return ERR_RESET;
    }    
  
  s.str(""); s << "[LECROY_1182]::[INFO]::++++++ LECROY_1182 CONFIGURED ++++++";  
  Log(s.str(),1);
  return 0;
} 

int LECROY_1182::Clear()
{
  return Init();
}      

int LECROY_1182::BufferClear()
{
  //Send a data reset. Clear event buffer and counters 
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data= configuration_.clrRegWord | LECROY_1182_DATARESET_BITMASK;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+LECROY_1182_CLR0_REG,&data,LECROY_1182_ADDRESSMODE,LECROY_1182_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[LECROY_1182]::[ERROR]::Cannot clear buffers LECROY_1182 board " << status; 
      Log(s.str(),1);
      return ERR_RESET;
    }  

  return 0;
}      

int LECROY_1182::Config(BoardConfig *bC)
{
  Board::Config(bC);
  GetConfiguration()->baseAddress=Configurator::GetInt( bC->getElementContent("baseAddress"));
  GetConfiguration()->clrRegWord=0x0004; //front panel input
  return 0;
}

//Read the ADC buffer and send it out in WORDS vector
int LECROY_1182::Read(vector<WORD> &v)
{
  v.clear();
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int status = 0; 

  WORD data;
  int lecroy1182_rdy=0;
  int lecroy1182_busy=1;

  int ntry = 1000, nt = 0;
  //Wait for a valid datum in the ADC
  while ( (lecroy1182_rdy != 1 || lecroy1182_busy!=0 ) && nt<ntry )
    {
      status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+LECROY_1182_CLR0_REG,&data,LECROY_1182_ADDRESSMODE,LECROY_1182_DATAWIDTH);
      lecroy1182_rdy = data & LECROY_1182_RDY_BITMASK;
      lecroy1182_busy  = data & LECROY_1182_BUSY_BITMASK;
      ++nt;
    }

  if (status || lecroy1182_rdy==0 || lecroy1182_busy==1)
    {
      ostringstream s; s << "[LECROY_1182]::[ERROR]::Cannot get a valid data from LECROY_1182 board " << status; 
      Log(s.str(),1);
      return ERR_READ;
    }  

#ifdef LECROY_1182_DEBUG
  ostringstream s; s << "[LECROY_1182]::[DEBUG]::RDY " << lecroy1182_rdy << " BUSY " << lecroy1182_busy << " NTRY " << nt;
  Log(s.str(),3);
#endif

  for (unsigned int i=0;i<channels_;++i)
    {
      status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+LECROY_1182_DATA_START_REG+(LECROY_1182_DATA_REG_SIZE*i),&data,LECROY_1182_ADDRESSMODE,LECROY_1182_DATAWIDTH);
      if (!status)
	{
	  v.push_back(data); //Filling event buffer
#ifdef LECROY1182_DEBUG
	  ostringstream s; s << "[LECROY_1182]::[INFO]::Read Channel " << "\tchannel " << i << "\tvalue " << data;  
	  Log(s.str(),3);
#endif
	}
      else
	{
	  ostringstream s; s << "[LECROY_1182]::[ERROR]::Error reading" << status; 
	  Log(s.str(),1);
	  v.clear();
	  return ERR_READ;
	}
    }

  status |= CheckStatusAfterRead();
  if (status)
    {
      ostringstream s; s << "[LECROY_1182]::[ERROR]::MEB !Empty or problems reading" << status; 
      Log(s.str(),1);
      return ERR_READ;
    }  

  return 0;
}

int LECROY_1182::CheckStatusAfterRead()
{
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int status = BufferClear(); 
  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+LECROY_1182_CLR0_REG,&data,LECROY_1182_ADDRESSMODE,LECROY_1182_DATAWIDTH);
  if (status)
    {
      ostringstream s; s << "[LECROY_1182]::[ERROR]::Cannot get status after read from LECROY_1182 board " << status; 
      Log(s.str(),1);
    }  

  int lecroy1182_full = ~(data & LECROY_1182_FULL_BITMASK);
  int lecroy1182_event = (data & LECROY_1182_EVTBUFFER_BITMASK)>>4; 

#ifdef LECROY1182_DEBUG
  ostringstream s; s << "[LECROY_1182]::[INFO]::Status after read full " << lecroy1182_full << " " << lecroy1182_event; 
      Log(s.str(),1);
#endif
  // if( lecroy1182_full || (lecroy1182_event != 0) || status!=1 ) 
  //    { 
  //      status=Clear();
  //    }

   if (status)
     {
       ostringstream s; s << "[LECROY_1182]::[ERROR]::Cannot restore healthy state in LECROY_1182 board " << status; 
      Log(s.str(),1);
      return ERR_READ;
     }  

  return 0;
}
