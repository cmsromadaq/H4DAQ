#include "interface/CAEN_V265.hpp"
#include <iostream>
#include <sstream>
#include <string>

#define CAENV265_DEBUG

int CAEN_V265::Init()
{
  int status=0;
  ostringstream s; s << "[CAEN_V265]::[INFO]::++++++ CAEN V265 INIT ++++++";
  Log(s.str(),1);
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  if (!IsConfigured())
    return ERR_CONF_NOT_FOUND;

  //Read Version to check connection
  WORD data=0;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V265_SERIAL,&data,CAEN_V265_ADDRESSMODE,CAEN_V265_DATAWIDTH);
  if (status)
    {
      s.str(""); s << "[CAEN_V265]::[ERROR]::Cannot open V265 board @0x" << std::hex << configuration_.baseAddress << std::dec; 
      Log(s.str(),1);
      return ERR_OPEN;
    }    

  s.str(""); s << "[CAEN_V265]::[INFO]::Open V265 board @0x" << std::hex << configuration_.baseAddress << std::dec << " S/N " << (data&CAEN_V265_SERIAL_BITMASK);
  Log(s.str(),1);

  data=0x0;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V265_CLEAR,&data,CAEN_V265_ADDRESSMODE,CAEN_V265_DATAWIDTH);
  if (status)
    {
      s.str(""); s << "[CAEN_V265]::[ERROR]::Cannot reset V265 board " << status; 
      Log(s.str(),1);
      return ERR_RESET;
    }    
  usleep(500);

  s.str(""); s << "[CAEN_V265]::[INFO]::++++++ CAEN V265 CONFIGURED ++++++";  
  Log(s.str(),1);
  return 0;
} 

int CAEN_V265::Clear()
{
  //Send a software reset. Module has to be re-initialized after this
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data=0x0;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V265_CLEAR,&data,CAEN_V265_ADDRESSMODE,CAEN_V265_DATAWIDTH);
  if (status)
    {
      ostringstream s; s << "[CAEN_V265]::[ERROR]::Cannot reset V265 board " << status; 
      Log(s.str(),1);
      return ERR_RESET;
    }
  usleep(500);

  status=Init();
#ifdef CAENV265_DEBUG
  ostringstream s; s << "[CAEN_V265]::[INFO]::V265 Board software reset" << status; 
  Log(s.str(),3);
#endif
  return status;
}      

int CAEN_V265::BufferClear()
{
  //Send a data reset. Clear event buffer and counters 
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data=0x0;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V265_CLEAR,&data,CAEN_V265_ADDRESSMODE,CAEN_V265_DATAWIDTH);
  if (status)
    {
      ostringstream s; s << "[CAEN_V265]::[ERROR]::Cannot reset V265 board " << status; 
      Log(s.str(),1);
      return ERR_RESET;
    }

#ifdef CAENV265_DEBUG
  ostringstream s; s << "[CAEN_V265]::[INFO]::V265 Buffers has been cleared"; 
  Log(s.str(),3);
#endif
  return 0;
}      

int CAEN_V265::ClearBusy()
{
  return BufferClear();
}      

int CAEN_V265::Config(BoardConfig *bC)
{
  Board::Config(bC);
  GetConfiguration()->baseAddress=Configurator::GetInt( bC->getElementContent("baseAddress"));
  
  return 0;
}

//Read the ADC buffer and send it out in WORDS vector
int CAEN_V265::Read(vector<WORD> &v)
{
  v.clear();

  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int status = 0; 
  WORD dataV[CAEN_V265_CHANNELS]; 

  //Empty event buffer
  for (unsigned int i=0;i<CAEN_V265_CHANNELS;++i)
    dataV[i]=0;

  WORD data;
  int v265_rdy=0;

  int ntry = 100, nt = 0;
  //Wait for a valid datum in the ADC
  while ( (v265_rdy != CAEN_V265_READY_BITMASK ) && nt<ntry )
    {
      status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V265_STATUS,&data,CAEN_V265_ADDRESSMODE,CAEN_V265_DATAWIDTH);
      v265_rdy = data & CAEN_V265_READY_BITMASK;
      ++nt;
    }

  if (status || v265_rdy==0)
    {
      ostringstream s; s << "[CAEN_V265]::[ERROR]::Cannot get a valid data from V265 board " << status; 
      Log(s.str(),1);
      return ERR_READ;
    }  

#ifdef CAENV265_DEBUG
  ostringstream s; s << "[CAEN_V265]::[DEBUG]::RDY " << v265_rdy << " NTRY " << nt;
  Log(s.str(),3);
#endif

  int iChannel=0;
  for(unsigned int iWord=0;iWord<2*CAEN_V265_CHANNELS;++iWord)
    {
      //Read words one by one
      status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V265_OUTPUT_BUFFER,&data,CAEN_V265_ADDRESSMODE,CAEN_V265_DATAWIDTH);
      if ( ((data&CAEN_V265_RANGE_BITMASK) >> 12 ) == 0 )
	dataV[iChannel]=(data&0xFFFF);
      else
	{
	  continue;
	}
      
#ifdef CAENV265_DEBUG
      ostringstream s; s << "[CAEN_V265_ROC]::[DEBUG]::DATA @ POS " << iChannel << ": " << std::hex << dataV[iChannel] << std::dec << "," <<  ((dataV[iChannel]&CAEN_V265_CHANNEL_BITMASK) >> 13 ) << "," <<  (dataV[iChannel]&CAEN_V265_ADCDATUM_BITMASK);
      Log(s.str(),3);
#endif
      v.push_back(dataV[iChannel]);
      ++iChannel;
    }
  
  if (status)
    {
      ostringstream s; s << "[CAEN_V265]::[ERROR]::Errors while reading" << status; 
      Log(s.str(),1);
      return ERR_READ;
    }  

  return 0;
}


