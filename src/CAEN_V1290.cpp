#include "interface/CAEN_V1290.hpp"
#include <iostream>
#include <sstream>
#include <string>

//#define CAENV1290_DEBUG 

int CAEN_V1290::Init()
{
  int status=0;
  ostringstream s; s << "[CAEN_V1290]::[INFO]::++++++ CAEN V1290 INIT ++++++";
  Log(s.str(),1);
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  if (!IsConfigured())
    return ERR_CONF_NOT_FOUND;

  //Read Version to check connection
  WORD data=0;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V1290_FW_VER_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);
  if (status)
    {
      s.str(""); s << "[CAEN_V1290]::[ERROR]::Cannot open V1290 board @0x" << std::hex << configuration_.baseAddress << std::dec << " " << status; 
      Log(s.str(),1);
      return ERR_OPEN;
    }    

  unsigned short FWVersion[2];
  for (int i=0;i<2;++i)
    FWVersion[i]=(data>>(i*4))&0xF;

  s.str(""); s << "[CAEN_V1290]::[INFO]::Open V1290 board @0x" << std::hex << configuration_.baseAddress << std::dec << " FW Version Rev." << FWVersion[1] << "." << FWVersion[0];
  Log(s.str(),1);

  data=0xFF;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V1290_SW_RESET_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);
  if (status)
    {
      s.str(""); s << "[CAEN_V1290]::[ERROR]::Cannot reset V1290 board " << status; 
      Log(s.str(),1);
      return ERR_RESET;
    }    

  sleep(1);
  //Now the real configuration  

  channels_=32;
  if (configuration_.model == CAEN_V1290_N)
    channels_=16;

  if (configuration_.emptyEventEnable)
    {
      WORD data=0;
      status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V1290_CON_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);
      data |= CAEN_V1290_EMPTYEVEN_BITMASK; //enable emptyEvent
      status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V1290_CON_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);
      ostringstream s; s << "[CAEN_V1290]::[INFO]::Enabled Empty Event";
      Log(s.str(),1);

    }

  /* I step: set TRIGGER Matching mode via OPCODE 00xx */
  if (configuration_.triggerMatchMode)
    {
      status |= OpWriteTDC(CAEN_V1290_TRMATCH_OPCODE);
      ostringstream s; s << "[CAEN_V1290]::[INFO]::Enabled Trigger Match Mode";
      Log(s.str(),1);
    }
    
  usleep(100000); 
  /* I step: set Edge detection via OPCODE 22xx */
  status |= OpWriteTDC(CAEN_V1290_EDGEDET_OPCODE);
  status |= OpWriteTDC(configuration_.edgeDetectionMode);
  s.str(""); s << "[CAEN_V1290]::[INFO]::EdgeDetection " << configuration_.edgeDetectionMode;
  Log(s.str(),1);
 
  usleep(100000); 
  /* I step: set Time Reso via OPCODE 24xx */
  status |= OpWriteTDC(CAEN_V1290_TIMERESO_OPCODE);
  status |= OpWriteTDC(configuration_.timeResolution);
  s.str(""); s << "[CAEN_V1290]::[INFO]::TimeResolution " << configuration_.timeResolution;
  Log(s.str(),1);

    
  usleep(100000); 
  /* II step: set TRIGGER Window Width to value n */
  status |= OpWriteTDC(CAEN_V1290_WINWIDT_OPCODE); 
  status |= OpWriteTDC(configuration_.windowWidth);
    
  usleep(100000); 
  /* III step: set TRIGGER Window Offset to value -n */
  status |= OpWriteTDC(CAEN_V1290_WINOFFS_OPCODE); 
  status |= OpWriteTDC(configuration_.windowOffset);
  s.str(""); s << "[CAEN_V1290]::[INFO]::TimeWindowWidth " << configuration_.windowWidth << " TimeWindowOffset " << configuration_.windowOffset;
  Log(s.str(),1);


  usleep(100000); 
  /* IV step: enable channels*/
  //disable all channels
  status |= OpWriteTDC(CAEN_V1290_DISALLCHAN_OPCODE); 

  for (unsigned int i=0;i<channels_;++i)
    if (configuration_.enabledChannels & ( 1 << i ) ) 
      {
  	usleep(100000); 
	s.str(""); s << "[CAEN_V1290]::[INFO]::Enabling channel " << i;
	Log(s.str(),1);
	status |=OpWriteTDC(CAEN_V1290_ENCHAN_OPCODE+i);
      }
  
  
  usleep(100000); 
  status |= OpWriteTDC(CAEN_V1290_MAXHITS_OPCODE); 
  usleep(100000); 
  status |= OpWriteTDC(configuration_.maxHitsPerEvent); 
  
  usleep(100000); 
  /* IV step: Enable trigger time subtraction */
  if (configuration_.triggerTimeSubtraction)
    status |= OpWriteTDC(CAEN_V1290_ENTRSUB_OPCODE);
  else
    status |= OpWriteTDC(CAEN_V1290_DISTRSUB_OPCODE);
  
  if (status)
    {
      s.str(""); s << "[CAEN_V1290]::[ERROR]::Config error" << status;
      Log(s.str(),1);
      return ERR_CONFIG;
    } 
  s.str(""); s << "[CAEN_V1290]::[INFO]::++++++ CAEN V1290 CONFIGURED ++++++";  
  Log(s.str(),1);
  return 0;
} 

int CAEN_V1290::Clear()
{
  //Send a software reset. Module has to be re-initialized after this
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data=0xFF;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V1290_SW_RESET_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);

  if (status)
    {
      ostringstream s; s << "[CAEN_V1290]::[ERROR]::Cannot reset V1290 board " << status; 
      Log(s.str(),1);
      return ERR_RESET;
    }

  sleep(1);

  status=Init();
  return status;
}      

int CAEN_V1290::BufferClear()
{
 //Send a software reset. Module has to be re-initialized after this
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data=0xFF;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+CAEN_V1290_SW_CLEAR_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);

  if (status)
    {
      ostringstream s; s << "[CAEN_V1290]::[ERROR]::Cannot clear V1290 buffers " << status; 
      Log(s.str(),1);
      return ERR_RESET;
    }

  return 0;
}
 
int CAEN_V1290::Config(BoardConfig *bC)
{
  Board::Config(bC);
  GetConfiguration()->baseAddress=Configurator::GetInt( bC->getElementContent("baseAddress"));
  
  GetConfiguration()->model=static_cast<CAEN_V1290_Model_t>(Configurator::GetInt( bC->getElementContent("model")));
  
  GetConfiguration()->triggerTimeSubtraction=static_cast<bool>(Configurator::GetInt( bC->getElementContent("triggerTimeSubtraction")));
  GetConfiguration()->triggerMatchMode=static_cast<bool>(Configurator::GetInt( bC->getElementContent("triggerMatchMode")));
  GetConfiguration()->emptyEventEnable=static_cast<bool>(Configurator::GetInt( bC->getElementContent("emptyEventEnable")));
  
  GetConfiguration()->edgeDetectionMode=static_cast<CAEN_V1290_EdgeDetection_t>(Configurator::GetInt( bC->getElementContent("edgeDetectionMode")));
  GetConfiguration()->timeResolution=static_cast<CAEN_V1290_TimeResolution_t>(Configurator::GetInt( bC->getElementContent("timeResolution")));
  GetConfiguration()->maxHitsPerEvent=static_cast<CAEN_V1290_MaxHits_t>(Configurator::GetInt( bC->getElementContent("maxHitsPerEvent")));
  
  GetConfiguration()->enabledChannels=Configurator::GetInt( bC->getElementContent("enabledChannels"));
  GetConfiguration()->windowWidth=Configurator::GetInt( bC->getElementContent("windowWidth"));
  GetConfiguration()->windowOffset=Configurator::GetInt( bC->getElementContent("windowOffset"));

  return 0;
}

//Read the ADC buffer and send it out in WORDS vector
int CAEN_V1290::Read(vector<WORD> &v)
{
  v.clear();
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int status = 0; 

  WORD data;
  int v1290_rdy=0;
  int v1290_error=1;

  int ntry = 100, nt = 0;
  //Wait for a valid datum in the ADC
  while ( (v1290_rdy != 1 || v1290_error!=0 ) && nt<ntry )
    {
      status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V1290_STATUS_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);
      v1290_rdy = data & CAEN_V1290_RDY_BITMASK;
      v1290_error = 0; 
      v1290_error |= data & CAEN_V1290_ERROR0_BITMASK;
      v1290_error |= data & CAEN_V1290_ERROR1_BITMASK;
      v1290_error |= data & CAEN_V1290_ERROR2_BITMASK;
      v1290_error |= data & CAEN_V1290_ERROR3_BITMASK;
      v1290_error |= data & CAEN_V1290_TRGLOST_BITMASK;
      ++nt;
    }

  if (status || v1290_rdy==0 || v1290_error!=0)
    {
      ostringstream s; s << "[CAEN_V1290]::[ERROR]::Cannot get a valid data from V1290 board " << status; 
      Log(s.str(),1);
      return ERR_READ;
    }  

#ifdef CAENV1290_DEBUG
  ostringstream s; s << "[CAEN_V1290]::[DEBUG]::RDY " << v1290_rdy << " ERROR " << v1290_error << " NTRY " << nt;
  Log(s.str(),3);
#endif

  data=0;
  
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1290_OUTPUT_BUFFER,&data,CAEN_V1290_ADDRESSMODE,cvD32);

#ifdef CAENV1290_DEBUG
  s.str(""); s << "[CAEN_V1290]::[DEBUG]::IS GLBHEADER " << ((data & 0x40000000)>>30);
  Log(s.str(),3);
#endif


  if ( ! (data & 0x40000000) || status )
    {
      ostringstream s; s << "[CAEN_V1290]::[ERROR]::First word not a Global trailer"; 
      Log(s.str(),1);
      return ERR_READ;
    }

  int evt_num = data>>5 & 0x3FFFFF;
  v.push_back( (0xA << 28) | (evt_num & 0xFFFFFFF ));

  //Read until trailer
  int glb_tra=0;
  while (!glb_tra && !status)
    {
      data=0;
      status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1290_OUTPUT_BUFFER,&data,CAEN_V1290_ADDRESSMODE,cvD32);
      int wordType = (data >> 27) & CAEN_V1290_WORDTYE_BITMASK;
#ifdef CAENV1290_DEBUG
      s.str(""); s << "[CAEN_V1290]::[DEBUG]::EVENT WORDTYPE " << wordType;
      Log(s.str(),3);
#endif

      if (wordType == CAEN_V1290_GLBTRAILER)
	{
#ifdef CAENV1290_DEBUG
	  s.str(""); s << "[CAEN_V1290]::[DEBUG]::WE FOUND THE EVENT TRAILER";
	  Log(s.str(),3);
#endif
	  glb_tra=1;
	  v.push_back(data);
	}
      else if (wordType == CAEN_V1290_TDCHEADER )
	{
#ifdef CAENV1290_DEBUG
	  s.str(""); s << "[CAEN_V1290]::[DEBUG]::WE FOUND THE TDC HEADER";
	  Log(s.str(),3);
#endif
	}
      else if (wordType == CAEN_V1290_TDCTRAILER )
	{
#ifdef CAENV1290_DEBUG
	  s.str(""); s << "[CAEN_V1290]::[DEBUG]::WE FOUND THE TDC TRAILER";
	  Log(s.str(),3);
#endif
	}
      else if (wordType == CAEN_V1290_TDCERROR )
	{
	  ostringstream s; s << "[CAEN_V1290]::[ERROR]::TDC ERROR!"; 
	  Log(s.str(),1);
	  v.clear();
	  return ERR_READ;
	}
      else if (wordType == CAEN_V1290_TDCMEASURE )
	{
	  v.push_back(data);
#ifdef CAENV1290_DEBUG
	  int measurement = data & 0x1fffff;
	  int channel = (data>>21) & 0x1f;
	  int trailing = (data>>26) & 0x1;
	  float tdc_time = (float)measurement*25./1000.;
	  s.str(""); s << "[CAEN_V1290]::[INFO]::HIT CHANNEL " << channel << " TYPE " << trailing << " TIME " << tdc_time; 
	  Log(s.str(),3);
#endif
	}
      else if (wordType == CAEN_V1290_GLBTRTIMETAG )
	{
	}
      else
	{
	  ostringstream s; s << "[CAEN_V1290]::[ERROR]::UNKNOWN WORD TYPE!"; 
	  Log(s.str(),1);
	  v.clear();
	  return ERR_READ;
	}
    }

  if (status)
    {
      ostringstream s; s << "[CAEN_V1290]::[ERROR]::READ ERROR!"; 
      Log(s.str(),1);
      v.clear();
      return ERR_READ;
    }

  status |= CheckStatusAfterRead();

  if (status)
    {
      ostringstream s; s << "[CAEN_V1290]::[ERROR]::MEB Full or problems reading" << status; 
      Log(s.str(),1);
      return ERR_READ;
    }  

  return 0;
}

int CAEN_V1290::CheckStatusAfterRead()
{
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int status = 0; 
  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+CAEN_V1290_STATUS_REG,&data,CAEN_V1290_ADDRESSMODE,cvD16);
  if (status)
    {
      ostringstream s; s << "[CAEN_V1290]::[ERROR]::Cannot get status after read from V1290 board " << status; 
      Log(s.str(),1);
    }  

  int v1290_full = data & CAEN_V1290_FULL_BITMASK;
  int v1290_error = 0; 
  v1290_error |= data & CAEN_V1290_ERROR0_BITMASK;
  v1290_error |= data & CAEN_V1290_ERROR1_BITMASK;
  v1290_error |= data & CAEN_V1290_ERROR2_BITMASK;
  v1290_error |= data & CAEN_V1290_ERROR3_BITMASK;
  v1290_error |= data & CAEN_V1290_TRGLOST_BITMASK;
  

   if( v1290_full || v1290_error ) 
     { 
       ostringstream s; s << "[CAEN_V1290]::[INFO]::Trying to restore V1290 board. Reset" << status; 
       status=BufferClear();
       if (status)
	   status=Clear();
     }
   
   if (status)
     {
       ostringstream s; s << "[CAEN_V1290]::[ERROR]::Cannot restore healthy state in V1290 board " << status; 
      Log(s.str(),1);
      return ERR_READ;
     }  
   
  return 0;
}

int CAEN_V1290::OpWriteTDC(WORD data) 
{
  int status;
  const int TIMEOUT = 100000;

  int time=0;
  /* Check the Write OK bit */
  WORD rdata=0;
  do {
    status = CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1290_MICROHANDREG ,&rdata, CAEN_V1290_ADDRESSMODE, cvD16);
    time++;
// #ifdef CAENV1290_DEBUG
//     ostringstream s; s << "[CAEN_V1290]::[INFO]::Handshake micro op writing " << rdata << " #" << time << " " << status;
//     Log(s.str(),3);
// #endif
  } while (!(rdata & 0x1) && (time < TIMEOUT) );

  if ( time == TIMEOUT ) {
       ostringstream s; s << "[CAEN_V1290]::[ERROR]::Cannot handshake micro op writing " << status; 
       Log(s.str(),1);
       return ERR_WRITE_OP;
   }

  status=0;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress + CAEN_V1290_MICROREG,&data, CAEN_V1290_ADDRESSMODE,cvD16);
  if (status)
    {
       ostringstream s; s << "[CAEN_V1290]::[ERROR]::Cannot write micro op " << status; 
       Log(s.str(),1);
       return ERR_WRITE_OP;
    }

    return 0;
}

/*----------------------------------------------------------------------*/

int CAEN_V1290::OpReadTDC(WORD* data) 
{
  int status;
  const int TIMEOUT = 100000;

  int time=0;
  /* Check the Write OK bit */
  WORD rdata=0;
  do {
    status = CAENVME_ReadCycle(handle_,configuration_.baseAddress +  CAEN_V1290_MICROHANDREG ,&rdata, CAEN_V1290_ADDRESSMODE, cvD16);
    time++;
// #ifdef CAENV1290_DEBUG
//     ostringstream s; s << "[CAEN_V1290]::[INFO]::Handshake micro op reading " << rdata << " #" << time << " " << status; 
//     Log(s.str(),3);
// #endif
  } while (!(rdata & 0x2) && (time < TIMEOUT) );

  if ( time == TIMEOUT ) {
       ostringstream s; s << "[CAEN_V1290]::[ERROR]::Cannot handshake micro op reading " << status; 
       Log(s.str(),1);
       return ERR_WRITE_OP;
   }

  status=0;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1290_MICROREG,data, CAEN_V1290_ADDRESSMODE,cvD16);
  if (status)
    {
       ostringstream s; s << "[CAEN_V1290]::[ERROR]::Cannot read micro op " << status; 
       Log(s.str(),1);
       return ERR_WRITE_OP;
    }

    return 0;
}

