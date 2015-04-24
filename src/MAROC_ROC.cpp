#include "interface/MAROC_ROC.hpp"
#include <iostream>
#include <sstream>
#include <string>

//#define MAROC_DEBUG

int MAROC_ROC::Init()
{
  int status=0;
  ostringstream s; s << "[MAROC_ROC]::[INFO]::++++++ MAROC READOUT & CONTROL INIT ++++++";
  Log(s.str(),1);
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  if (!IsConfigured())
    return ERR_CONF_NOT_FOUND;

  //Check board is present
  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  if (status)
    {
      s.str(""); s << "[MAROC_ROC]::[ERROR]::Cannot open MAROC board @0x" << std::hex << configuration_.baseAddress << std::dec; 
      Log(s.str(),1);
      return ERR_OPEN;
    }    

  //Load configuration on Maroc FEB (829 bits)
  status |= LoadFEBConfiguration();
  if (status)
    {
      s.str(""); s << "[MAROC_ROC]::[ERROR]::Cannot config MAROC board @0x" << std::hex << configuration_.baseAddress << std::dec; 
      Log(s.str(),1);
      return ERR_CONFIG;
    }    

  s.str(""); s << "[MAROC_ROC]::[INFO]::Open MAROC board @0x" << std::hex << configuration_.baseAddress << std::dec << " conf 0x" << std::hex << data << std::dec;
  Log(s.str(),1);

  int run_mode = configuration_.debugMode==1 ? 0xd : 0x0; 

  //Put the system in readout mode
  status |= SendOnFEBBus(3,run_mode);
  status |= SendOnFEBBus(8,0);
  s.str(""); s << "[MAROC_ROC]::[INFO]::MAROC FEB in run_mode " << run_mode;
  Log(s.str(),1);
    
  //Initialize ADC
  status |= InitADC();
  // s.str(""); s << "[MAROC_ROC]::[INFO]::Init MAROC FEB ADC";
  // Log(s.str(),1);

  //Config hold values for the MAROC
  status |= ConfigFEBReadoutHold();
  // s.str(""); s << "[MAROC_ROC]::[INFO]::MAROC FEB readout configured: hold " << configuration_.holdValue << " hold2 delay " << configuration_.holdDeltaValue;
  // Log(s.str(),1);

  //Configure trigger mode
  status |= ConfigFEBTrigger();
  status |= ConfigROCTrigger();
  // s.str(""); s << "[MAROC_ROC]::[INFO]::Trigger inputs configured";
  // Log(s.str(),1);

  //Configure ADC
  status |= SetADCClock(400);
  status |= SetADCEnableDReset();
  status |= SetADCTestOff();
  status |= SetADCNormalReadoutMode();
  status |= SetADCZeroSuppressionMode();
  status |= SetADCSlowHold(20,0);
  // s.str(""); s << "[MAROC_ROC]::[INFO]::MAROC FEB ADC configured";
  // Log(s.str(),1);

  //Reset TimeStamp for the eventInfo
  status |= ResetTimeStamp();
  // s.str(""); s << "[MAROC_ROC]::[INFO]::MAROC TimeStamp reset";
  // Log(s.str(),1);
  
  if (status)
    {
      s.str(""); s << "[MAROC_ROC]::[ERROR]::Config error" << status;
      Log(s.str(),1);
      return ERR_CONFIG;
    } 

  s.str(""); s << "[MAROC_ROC]::[INFO]::++++++ CONFIGURED - READY FOR DAQ ++++++";  
  Log(s.str(),1);
  return 0;
} 

int MAROC_ROC::Clear()
{

  //Send a software reset. Module has to be re-initialized after this
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  //Issue a SoftReset
  status |= SoftwareReset();
  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Cannot reset  board " << status; 
      Log(s.str(),1);
      return ERR_RESET;
    }

  //Re-initialize and clear buffer
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

  //Just clear the buffers and reset the ADC busy
  status |= ResetFIFO();
  status |= SetMemoryMode(true);
  status |= ClearADCBusy();
  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_INPUT_CONNECTOR_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
#ifdef MAROC_DEBUG
  ostringstream s; s << "[MAROC_ROC]::[DEBUG]::ADC status after clear " << data;
  Log(s.str(),3);
#endif
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
// #ifdef MAROC_DEBUG
//   ostringstream s; s << "[MAROC_ROC]::[DEBUG]::INIT CONFIG";
//   Log(s.str(),3);
// #endif
  GetConfiguration()->baseAddress=Configurator::GetInt( bC->getElementContent("baseAddress"));
#ifdef MAROC_DEBUG
 ostringstream s; s << "[MAROC_ROC]::[DEBUG]::BASE ADDRESS 0x" << std::hex << GetConfiguration()->baseAddress << std::dec;
  Log(s.str(),3);
#endif
  GetConfiguration()->configFile=bC->getElementContent("configFile");
  ostringstream s1; s1 << "[MAROC_ROC]::[INFO]::CONFIG FILE " << std::hex << GetConfiguration()->configFile << std::dec;
  Log(s1.str(),1);

  GetConfiguration()->triggerType=static_cast<MAROC_ROC_TriggerType_t>(Configurator::GetInt( bC->getElementContent("triggerType")));
  
  GetConfiguration()->debugMode=static_cast<bool>(Configurator::GetInt( bC->getElementContent("debugMode")));
  
  GetConfiguration()->holdValue=Configurator::GetInt( bC->getElementContent("holdValue"));  
  GetConfiguration()->holdDeltaValue=Configurator::GetInt( bC->getElementContent("holdDeltaValue"));
// #ifdef MAROC_DEBUG
//   s.str(""); s << "[MAROC_ROC]::[DEBUG]::EXIT CONFIG";
//   Log(s.str(),3);
// #endif
  return 0;
}

//Read the ADC buffer and send it out in WORDS vector
int MAROC_ROC::Read(vector<WORD> &v)
{
  v.clear();
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int status = 0; 
  WORD dataV[MAROC_ROC_BOARDDATA_NWORD]; 
  //Empty event buffer
  for (unsigned int i=0;i<MAROC_ROC_BOARDDATA_NWORD;++i)
    dataV[i]=0;

  WORD data;
  int adc_rdy=0;

  int ntry = 1000, nt = 0, adcstatus=0;
  //Wait for a valid datum in the ADC
  while ( adc_rdy == 0 && nt<ntry )
    {
      status |= GetADCStatus(adcstatus);
#ifdef MAROC_DEBUG
      ostringstream s; s << "[MAROC_ROC]::[DEBUG]::ADC status " << std::hex << adcstatus << std::dec;
      Log(s.str(),3);
#endif
      //Assuming only one MAROC FEB
      adc_rdy = adcstatus & 0x3; 
      ++nt;
    }
#ifdef MAROC_DEBUG
  ostringstream s; s << "[MAROC_ROC]::[DEBUG]::Got a ready from ADC after " << nt << " ntry";
  Log(s.str(),3);
#endif
  if (status || adc_rdy==0 )
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Cannot get a valid data from  board " << status; 
      Log(s.str(),1);
      return ERR_READ;
    }  

  // //Prepare memory to be readout
  SetMemoryMode(false);
  int xor_data=0XAAAA;
  for(unsigned int iWord=0;iWord<MAROC_ROC_BOARDDATA_NWORD;++iWord)
    {
      //Read words one by one
      status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+iWord*2,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
      if (iWord<5)
	{
	  //Event Header values
	  dataV[iWord]=(data>>1) & 0xFFFF;
	}
      else if (iWord<MAROC_ROC_BOARDDATA_NWORD-1)
	{
	  //ADC data
	  dataV[iWord]=(data>>1) & 0x1FFF;
	  xor_data=(xor_data^dataV[iWord])&0xFFFF;
	}
      else
	{
	  //Last word is ADC data XOR values, checking consistency
	  dataV[iWord]=(data>>1) & 0x1FFF;
	  if (dataV[iWord] != (xor_data&0x1FFF) )
	    {
	      ostringstream s; s << "[MAROC_ROC]::[WARNING]::Event XOR data not consistent " << std::hex << dataV[iWord] << "," << (xor_data&0x1FFF) << std::dec;
	      Log(s.str(),1);
	    }
	}

#ifdef MAROC_DEBUG
      ostringstream s; s << "[MAROC_ROC]::[DEBUG]::DATA @ POS " << iWord << " " << std::hex << dataV[iWord] << std::dec << "," <<  dataV[iWord];
      Log(s.str(),3);
#endif
      v.push_back(dataV[iWord]);
      
      //at position 4 add the holdValue recorded in this event
      if (iWord==3)
   	{
   	  data=(configuration_.holdValue && 0xFFFF);
   	  v.push_back(data);
   	}
    }
  
  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Errors while reading" << status; 
      Log(s.str(),1);
      return ERR_READ;
    }  

  return 0;
}

int MAROC_ROC::LoadFEBConfiguration()
{
  int status=0;
  ostringstream s; s << "[MAROC_ROC]::[INFO]::Loading FEB configuration";
  Log(s.str(),1);
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  
  unsigned int mar_mask[MAROC_ROC_FEB_CONFIG_BITSIZE];
  for (int i=0; i<MAROC_ROC_FEB_CONFIG_BITSIZE; i++) {
    mar_mask[i] = 0;
  }

  char config[8300]={0};
  char* config_val;
  //Open config file
  FILE* mfile = fopen(configuration_.configFile.c_str(),"r");
  if (!mfile)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Cannot find config file " << configuration_.configFile;
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  //fill the configuration
  char* read=fgets(config,MAROC_ROC_FEB_CONFIG_BITSIZE+1,mfile);
  if (read == NULL)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error reading from config file " << configuration_.configFile;
      Log(s.str(),1);
      return ERR_CONFIG;
    }
  config_val=config;
  for (int i=0; i<MAROC_ROC_FEB_CONFIG_BITSIZE; i++) {
    sscanf(config_val,"%1u",&mar_mask[i]);
    ++config_val;
  }

  fclose(mfile);

  unsigned int bit_local[MAROC_ROC_FEB_CONFIG_BITSIZE];
  for (int i=0; i<MAROC_ROC_FEB_CONFIG_BITSIZE; i++) {
    bit_local[i]=mar_mask[i];
  }

#ifdef MAROC_DEBUG
  s.str(""); s << "[MAROC_ROC]::[DEBUG]::CONFIG FILE READ";
  Log(s.str(),3);
#endif
  //Put the system in readout
  status |= SendOnFEBBus(3,10);

  WORD data_out;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_out,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  //Load wanted config
  s.str(""); s << "[MAROC_ROC]::[INFO]::Writing configuration "; 
  for (int iloop = 0; iloop<1; iloop++){
    for (int ibit=0; ibit<MAROC_ROC_FEB_CONFIG_BITSIZE; ibit++) {

#ifdef MAROC_DEBUG
      s << bit_local[ibit];
#endif

      /* set data */
      if (bit_local[ibit] == 1) {
	status |= RegIn(true);
      } else if (bit_local[ibit] == 0) {
	status |= RegIn(false);
      } else {
	ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error in MAROC FEB configuration " << configuration_.configFile << " @ bit " << ibit;
	Log(s.str(),1);
	return ERR_CONFIG;
      }

      status |= ClockReg();

      if ((ibit%(MAROC_ROC_FEB_CONFIG_BITSIZE/20))==0)
	s << "#";
    }
  }


  if (status)
    {
      ostringstream s1; s1 << "[MAROC_ROC]::[ERROR]::Error writing configuration";    
      Log(s1.str(),1);
      return ERR_CONFIG;
    }
  else
    {
      s << " 100% OK";
      Log(s.str(),1);
    }

  s.str(""); s << "[MAROC_ROC]::[INFO]::Checking configuration "; 
  //Resend & check wanted config
  int badconfig=0;
  for (int iloop = 0; iloop<1; iloop++){
    for (int ibit=0; ibit<MAROC_ROC_FEB_CONFIG_BITSIZE; ibit++) {
      /* set data */
      if (bit_local[ibit] == 1) {
	status |= RegIn(true);
      } else if (bit_local[ibit] == 0) {
	status |= RegIn(false);
      } else {
	ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error in MAROC FEB configuration " << configuration_.configFile << " @ bit " << ibit;
	Log(s.str(),1);
	return ERR_CONFIG;
      }
      WORD data_in;
      status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_INPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

      ClockReg();

      unsigned int readback_bit=data_in & 0x1;

      if (readback_bit !=  bit_local[ibit]) 
	{
// #ifdef MAROC_DEBUG
//   s.str(""); s << "[MAROC_ROC]::[DEBUG]::Config bit mismatch @ location " << ibit << " : wanted " << bit_local[ibit] << " read 0x" <<std::hex << data_in<< std::dec; 
//   Log(s.str(),3);
// #endif	
	  s << "X";
	  ++badconfig;
	}
      else
	{
	  if ((ibit%(MAROC_ROC_FEB_CONFIG_BITSIZE/20))==0)
	    s << "#";
	}
    }
  }

  if (!badconfig)
    s << " 100% OK";
  else
    s << " ERROR";
  Log(s.str(),1);
  
  if (badconfig)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Load configuration DO NOT match wanted configuration @" << configuration_.configFile;
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  status |= SendOnFEBBus(3,1);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error loading configuration";    
      Log(s.str(),1);
      return ERR_CONFIG;
    }
  
  s.str(""); s << "[MAROC_ROC]::[INFO]::Load FEB configuration OK";
  Log(s.str(),1);  
  return 0;
}


int MAROC_ROC::SendOnFEBBus(int address, int data)
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  
  int local_addr = address | ((0 & 0xF) << 8);

  WORD data_in;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::clearbit(&data_in,1);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::setbit(&data_in,1);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::setbit(&data_in,2);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  status |= ClockFEBBusPulse();
  status |= ClockFEBBusPulse();

  //Encode address
  for (int ibit=0; ibit<16; ibit++) {
    int xbit = (local_addr >> (15-ibit)) & 0x1;
    if (xbit==1) {
      Utility::setbit(&data_in,2);
      status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
    } else {
      Utility::clearbit(&data_in,2);
      status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
    }
    status |= ClockFEBBusPulse();
  }

  //Encode data
  for (int ibit=0; ibit<16; ibit++) {
    int xbit = (data >> (15-ibit)) & 0x1;
    if (xbit==1) {
      Utility::setbit(&data_in,2);
      status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
    } else {
      Utility::clearbit(&data_in,2);
      status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
    }
    status |= ClockFEBBusPulse();
  }

  Utility::clearbit(&data_in,1);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  status |= ClockFEBBusPulse();

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error sending data on FEB bus";    
      Log(s.str(),1);
      return ERR_FEB_COMM;
    }

  return 0;
}

int MAROC_ROC::ClockFEBBusPulse()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  
  WORD data_in;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::clearbit(&data_in,0);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::setbit(&data_in,0);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::clearbit(&data_in,0);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error sending clock pulse on FEB bus";    
      Log(s.str(),1);
      return ERR_FEB_COMM;
    }

  return 0;

}

int MAROC_ROC::RegIn(bool up)
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int bit_reg= 5;

  WORD data_in;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  if (up)
    Utility::setbit(&data_in,bit_reg);
  else
    Utility::clearbit(&data_in,bit_reg);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error setting FEB register on output connector";    
      Log(s.str(),1);
      return ERR_FEB_COMM;
    }

  return 0;
}

int MAROC_ROC::ClockReg()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int bit_reg= 4;

  WORD data_in;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::setbit(&data_in,bit_reg);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::clearbit(&data_in,bit_reg);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data_in,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error while cycling the clock";    
      Log(s.str(),1);
      return ERR_FEB_COMM;
    }

  return 0;
}


int MAROC_ROC::InitADC()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;


  WORD data=0x0;
  Utility::setbit(&data,3);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error initializing the ADC";    
      Log(s.str(),1);
      return ERR_FEB_COMM;
    }

  ostringstream s; s << "[MAROC_ROC]::[INFO]::ADC initialized";    
  Log(s.str(),1);
  
  return 0;
}


int MAROC_ROC::SetMemoryMode(bool external)
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;


  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  if (external)
    Utility::clearbit(&data,0);
  else
    Utility::setbit(&data,0);

  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  
  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error setting memory mode";    
      Log(s.str(),1);
      return ERR_FEB_COMM;
    }

  return 0;
}

int MAROC_ROC::ConfigFEBReadoutHold()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  // Hold 1
  status |= SendOnFEBBus(MAROC_ROC_HOLD1_SUBADD,configuration_.holdValue);
  // Hold 2 -> hold1 + 20 tick=60ns
  status |= SendOnFEBBus(MAROC_ROC_HOLD2_SUBADD,configuration_.holdValue+configuration_.holdDeltaValue);
  /* set start dreset */
  status |= SendOnFEBBus(6,configuration_.holdValue+300);
  /* set end dreset */
  status |= SendOnFEBBus(5,configuration_.holdValue+600);
  /* set start readout */
  status |= SendOnFEBBus(4,configuration_.holdValue+900);
  /* load all the readout config */
  status |= SendOnFEBBus(0,0);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error configuring MAROC readout";    
      Log(s.str(),1);
      return ERR_FEB_COMM;
    }

  ostringstream s; s << "[MAROC_ROC]::[INFO]::MAROC hold readout configured;HOLD1 " << configuration_.holdValue << " HOLD2 " << configuration_.holdValue+configuration_.holdDeltaValue;    
  Log(s.str(),1);

  return 0;
}

int MAROC_ROC::ConfigFEBTrigger()
{
  //FEB always in external trigger mode (MAROC trigger generation not supported at the moment

  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  if (configuration_.triggerType == FEB)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::MAROC FEB Trigger Mode not yet supported";    
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  //Set MAROC FEB in external trigger mode
  status |= SendOnFEBBus(MAROC_ROC_FEB_CONFIG_TRIG_SUBADD,0);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error configuring MAROC FEB Trigger";          Log(s.str(),1);
      return ERR_FEB_COMM;
    }

  ostringstream s; s << "[MAROC_ROC]::[INFO]::MAROC FEB Trigger configured to EXTERNAL mode";     
  Log(s.str(),1);

  return 0;
}

int MAROC_ROC::ConfigROCTrigger()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  if (configuration_.triggerType == FEB)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::MAROC FEB Trigger Mode not yet supported";    
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  ostringstream s;

  switch(configuration_.triggerType)
    {
    case TTL_RISING: //0b000
      Utility::clearbit(&data,11);
      Utility::clearbit(&data,10);
      Utility::clearbit(&data,9);
      s.str(""); s << "[MAROC_ROC]::[INFO]::ROC Trigger set to TTL_RISING";     
      Log(s.str(),1);
      break;
    case TTL_FALLING: //0b010
      Utility::clearbit(&data,11);
      Utility::setbit(&data,10);
      Utility::clearbit(&data,9);
      s.str(""); s << "[MAROC_ROC]::[INFO]::ROC Trigger set to TTL_FALLING";     
      Log(s.str(),1);
      break;
    case NIM: //0b111
      Utility::setbit(&data,11);
      Utility::setbit(&data,10);
      Utility::setbit(&data,9);
      s.str(""); s << "[MAROC_ROC]::[INFO]::ROC Trigger set to NIM";     
      Log(s.str(),1);
      break;
    case INTERNAL: //0b001
      Utility::clearbit(&data,11);
      Utility::clearbit(&data,10);
      Utility::setbit(&data,9);
      s.str(""); s << "[MAROC_ROC]::[INFO]::ROC Trigger set to INTERNAL";     
      Log(s.str(),1);
      break;
    default:
     s.str(""); s << "[MAROC_ROC]::[ERROR]::Trigger Type not supported";    
     Log(s.str(),1);
     return ERR_CONFIG;
    }

  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);  
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error configuring ROC Trigger Input";
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  s.str(""); s << "[MAROC_ROC]::[INFO]::ROC Trigger Input configured";     
  Log(s.str(),1);

  return 0;
}

int MAROC_ROC::SetADCClock(int nclk)
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_ADCCLK_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  data=nclk&0xFFFF;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_ADCCLK_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);  
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_ADCCLK_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error configuring ADC CLOCK";
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  ostringstream s; s << "[MAROC_ROC]::[INFO]::ADC Clock set to " << nclk;     
  Log(s.str(),1);
  
  return 0;
}

int MAROC_ROC::SetADCTestOff()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::clearbit(&data,MAROC_ROC_CONF_REGISTER_TEST_ON_BIT);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);  
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error disabling test";
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  ostringstream s; s << "[MAROC_ROC]::[INFO]::test enabled";     
  Log(s.str(),1);
  
  return 0;
}

int MAROC_ROC::SetADCNormalReadoutMode()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::clearbit(&data,MAROC_ROC_CONF_REGISTER_TEST_MODE_BIT);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);  
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error enabing readout mode";
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  ostringstream s; s << "[MAROC_ROC]::[INFO]::readout mode enabled ";     
  Log(s.str(),1);
  
  return 0;
}

int MAROC_ROC::SetADCEnableDReset()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::setbit(&data,MAROC_ROC_CONF_REGISTER_DRESET_ON_BIT);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);  
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error enabling dreset";
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  ostringstream s; s << "[MAROC_ROC]::[INFO]::dreset enabled ";     
  Log(s.str(),1);
  
  return 0;
}

int MAROC_ROC::SetADCZeroSuppressionMode()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_MODESUPP_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  data=0x0;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_MODESUPP_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);  
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_MODESUPP_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error disabling zero suppression";
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  ostringstream s; s << "[MAROC_ROC]::[INFO]::zero suppression disabled";     
  Log(s.str(),1);
  
  return 0;
}

int MAROC_ROC::SetADCSlowHold(int slowhold, int clock_sel)
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  if (slowhold>16383) {
    ostringstream s; s << "[MAROC_ROC]::[ERROR]::Slowhold value " << slowhold << " too high!";
    Log(s.str(),1);
    return ERR_CONFIG;
  }

  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_HOLD_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  data=(((clock_sel<<14)&0xc000) | (slowhold&0x3fff));
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_HOLD_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);  
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_HOLD_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  
  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error setting ADC slowhold";
      Log(s.str(),1);
      return ERR_CONFIG;
    }

  ostringstream s; s << "[MAROC_ROC]::[INFO]::ADC slowhold configured to " << slowhold << " clock_sel " << clock_sel;     
  Log(s.str(),1);
  
  return 0;
}

int MAROC_ROC::ResetTimeStamp()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data;
  data=0x0;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_TS_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);  
  data=0x3;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_TS_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);  

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error resetting timestamp";
      Log(s.str(),1);
      return ERR_RESET;
    }

  ostringstream s; s << "[MAROC_ROC]::[INFO]::Internal timestamp reset";
  Log(s.str(),1);
  
  return 0;

}

int MAROC_ROC::ResetFIFO()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data;
  data=0xFF00;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_FIFO_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);  
  data=0x0;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_FIFO_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);  
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_FIFO_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
#ifdef MAROC_DEBUG
  ostringstream s; s << "[MAROC_ROC]::[DEBUG]::FIFO register after clear " << data;
  Log(s.str(),3);
#endif

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error resetting FIFO";
      Log(s.str(),1);
      return ERR_RESET;
    }

  return 0;
}

int MAROC_ROC::ClearADCBusy()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::clearbit(&data,3);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);  
  Utility::setbit(&data,3);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);  
  Utility::clearbit(&data,3);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_OUTPUT_CONNECTOR_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);  

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error clearing ADC Busy";
      Log(s.str(),1);
      return ERR_RESET;
    }

  return 0;
}

int MAROC_ROC::GetADCStatus(int & mystat)
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_INPUT_CONNECTOR_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error getting ADC status";
      Log(s.str(),1);
      mystat=-1;
      return ERR_READ;
    }

  mystat = data&0xFFFF;
  return 0;
}

int MAROC_ROC::SoftwareReset()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::setbit(&data,1);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::clearbit(&data,1);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);
  Utility::setbit(&data,1);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress+MAROC_ROC_CONF_REGISTER,&data,MAROC_ROC_ADDRESSMODE,MAROC_ROC_DATAWIDTH);

  if (status)
    {
      ostringstream s; s << "[MAROC_ROC]::[ERROR]::Error in software reset";
      Log(s.str(),1);
      return ERR_RESET;
    }

  ostringstream s; s << "[MAROC_ROC]::[INFO]::MAROC ROC Software reset";
  Log(s.str(),1);
  
  return 0;
}
