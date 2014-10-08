#include "interface/CAEN_V1495PU.hpp"
#include <iostream>
#include <sstream>
#include <string>

#define CAEN_V1495PU_DEBUG

int CAEN_V1495PU::Init()
{
  int status=0;
  ostringstream s; s << "[CAEN_V1495PU]::[INFO]::++++++ CAEN V1495PU INIT ++++++";
  Log(s.str(),1);
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;
  if (!IsConfigured())
    return ERR_CONF_NOT_FOUND;

  s.str(""); s <<"[CAEN_V1495PU]::[INFO]::PATTERN UNIT @0x" <<  std::hex << configuration_.baseAddress << std::dec << " CONFIGURATION +++++" ;
  Log(s.str(),1);

  //Reset
  WORD data;
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_MODULERESET_ADDRESS, &data ,CAEN_V1495PU_ADDRESSMODE,cvD32);
  sleep(1);


  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_VMEFPGA_FWVERSION_ADDRESS, &data ,CAEN_V1495PU_ADDRESSMODE,cvD32);
  short vmefwMajorVersion=data>>8 & CAEN_V1495_PATTERNUNIT_VMEFPGAFWVERSION_MAJORNUMBER_BITMASK;
  short vmefwMinorVersion=data & CAEN_V1495_PATTERNUNIT_VMEFPGAFWVERSION_MINORNUMBER_BITMASK;
  status |= CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_USERFPGA_FWVERSION_ADDRESS, &data ,CAEN_V1495PU_ADDRESSMODE,cvD32);
  short userfpgafwMajorVersion=data>>4 & CAEN_V1495_PATTERNUNIT_USERFPGAFWVERSION_MAJORNUMBER_BITMASK;
  short userfpgafwMinorVersion=data & CAEN_V1495_PATTERNUNIT_USERFPGAFWVERSION_MINORNUMBER_BITMASK;

  if (status !=1 )
    {
      s.str(""); s <<"[CAEN_V1495PU]::[ERROR]::Communicator error for device @0x" << std::hex << configuration_.baseAddress << " " << status ;
      Log(s.str(),1);
      return ERR_OPEN;
    }
  else
    {
      s.str(""); s <<"[CAEN_V1495PU]::[INFO]::VME FPGA FW Version " << vmefwMajorVersion << "." << vmefwMinorVersion << " USER FPGA FW Version " << userfpgafwMajorVersion << "." << userfpgafwMinorVersion ;
      Log(s.str(),1);
    }

  //Ctrl Reg & Mask words
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_CTRLREG_ADDRESS , &configuration_.ctrlRegWord ,CAEN_V1495PU_ADDRESSMODE,cvD32);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_MASKA_ADDRESS , &configuration_.maskA ,CAEN_V1495PU_ADDRESSMODE,cvD32);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_MASKB_ADDRESS , &configuration_.maskB ,CAEN_V1495PU_ADDRESSMODE,cvD32);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_MASKE_ADDRESS , &configuration_.maskE ,CAEN_V1495PU_ADDRESSMODE,cvD32);
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_MASKF_ADDRESS , &configuration_.maskF ,CAEN_V1495PU_ADDRESSMODE,cvD32);
  if (userfpgafwMajorVersion>=2 && userfpgafwMinorVersion>=1) //delay implemented from FW version 2.1
    {
      //delay
      status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_DELAY_ADDRESS , &configuration_.sigDelay ,CAEN_V1495PU_ADDRESSMODE,cvD32);
    }
  if (status !=1 )
    {
      s.str(""); s <<"[CAEN_V1495PU]::[ERROR]::Configruation error for device @0x" <<std::hex <<  configuration_.baseAddress << std::dec << " " << status ;
      Log(s.str(),1);
      return ERR_CONFIG;;
    }

  //  s.str(""); s <<"[CAEN_V1495PU]::[INFO]::Initializing completed for device @0x" <<std::hex <<  configuration_.baseAddress << std::dec ;
  s.str(""); s << "[CAEN_V1495PU]::[INFO]::++++++ CAEN V560 CONFIGURED ++++++";  
  Log(s.str(),1);
  return status;

}

int CAEN_V1495PU::Clear()
{
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  //Re-Initialize the Board
  return Init();
}      

int CAEN_V1495PU::BufferClear()
{
  //Just clear the buffers
  int status=0;
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  WORD data=configuration_.ctrlRegWord | 0x200; //Software clear Regs + Event Counter + clear Busy (active high) - PULSE
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_CTRLREG_ADDRESS, &data ,CAEN_V1495PU_ADDRESSMODE,cvD32);
  //re-enable gate
  data = configuration_.ctrlRegWord | 0x1000; //Enable gate
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_CTRLREG_ADDRESS, &data ,CAEN_V1495PU_ADDRESSMODE,cvD32);  

  if (status)
    {
      ostringstream s; s  << "[CAEN_V1495PU]::[ERROR]::Error clearing and re-enabling gate for deveice @0x" <<std::hex <<  configuration_.baseAddress << std::dec <<  status;
      Log(s.str(),1);
      return ERR_READ;
    }

  return 0;
}      

int CAEN_V1495PU::Config(BoardConfig *bC)
{
  Board::Config(bC);
  //here the parsing of the xmlnode...
  GetConfiguration()->baseAddress=Configurator::GetInt( bC->getElementContent("baseAddress"));
  GetConfiguration()->ctrlRegWord=Configurator::GetInt( bC->getElementContent("ctrlRegWord"));
  GetConfiguration()->maskA=Configurator::GetInt( bC->getElementContent("maskA"));
  GetConfiguration()->maskB=Configurator::GetInt( bC->getElementContent("maskB"));
  GetConfiguration()->maskE=Configurator::GetInt( bC->getElementContent("maskE"));
  GetConfiguration()->maskF=Configurator::GetInt( bC->getElementContent("maskF"));
  GetConfiguration()->sigDelay=Configurator::GetInt( bC->getElementContent("sigDelay"));

  return 0;
}

int CAEN_V1495PU::Read(vector<WORD> &v)
{
  int status=0;
  v.clear();
  if (handle_<0)
    return ERR_CONF_NOT_FOUND;

  int TIMEOUT=1000;
  int ntry=0;
  ostringstream s;

  int gate=0;
  WORD data;
  while (!gate && ntry<TIMEOUT)
    {
      status = CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_STATUS_ADDRESS , &data ,CAEN_V1495PU_ADDRESSMODE,cvD32);
      if (status !=1 )
	{
	  s.str(""); s  << "[CAEN_V1495PU]::[ERROR]::Error reading status for device @0x" << std::hex <<  configuration_.baseAddress << std::dec <<  status;
	  Log(s.str(),1);
	  return ERR_READ;
	}
      else
	{
	  gate=(data>>20)&0x1;
	}
    }

  if (ntry == TIMEOUT)
    {
      s.str(""); s << "[CAEN_V1495PU]::[ERROR]::Cannot get a valid data from V1495PU board " << status; 
      Log(s.str(),1);
      return ERR_READ;

    }


  WORD header=0x50000004; //BOE + nPatternsReadout
  v.push_back(header);
  v.push_back(data); //status word (includes internal event counter)

#ifdef CAEN_V1495PU_DEBUG
  s.str(""); s  << "[CAEN_V1495PU]::[INFO]::STATUS ===> 0x" << std::hex << data << std::dec;
  Log(s.str(),1);
#endif
  status = CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_PATTERNA_ADDRESS , &data ,CAEN_V1495PU_ADDRESSMODE,cvD32);
#ifdef CAEN_V1495PU_DEBUG
  s.str(""); s  << "[CAEN_V1495PU]::[INFO]::PATTERNA ===> 0x" << std::hex << data << std::dec;
  Log(s.str(),1);
#endif
  v.push_back(data);
  status = CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_PATTERNB_ADDRESS , &data ,CAEN_V1495PU_ADDRESSMODE,cvD32);
#ifdef CAEN_V1495PU_DEBUG
  s.str(""); s  << "[CAEN_V1495PU]::[INFO]::PATTERNB ===> 0x" << std::hex << data << std::dec;
  Log(s.str(),1);
#endif
  v.push_back(data);
  status = CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_PATTERNE_ADDRESS , &data ,CAEN_V1495PU_ADDRESSMODE,cvD32);
#ifdef CAEN_V1495PU_DEBUG
  s.str(""); s  << "[CAEN_V1495PU]::[INFO]::PATTERNE ===> 0x" << std::hex << data << std::dec;
  Log(s.str(),1);
#endif
  v.push_back(data);
  status = CAENVME_ReadCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_PATTERNF_ADDRESS , &data ,CAEN_V1495PU_ADDRESSMODE,cvD32);
#ifdef CAEN_V1495PU_DEBUG
  s.str(""); s  << "[CAEN_V1495PU]::[INFO]::PATTERNF ===> 0x" << std::hex << data << std::dec;
  Log(s.str(),1);
#endif
  v.push_back(data);

  data=0x30000007; //EOE + eventSize
  v.push_back(data);

  if (status)
    {
      s.str(""); s  << "[CAEN_V1495PU]::[ERROR]::Error reading patterns for device @0x" <<std::hex <<  configuration_.baseAddress << std::dec <<  status;
      Log(s.str(),1);
      return ERR_READ;
    }

  //Clear pattern registers & busy, Re-enable the gate
  data=configuration_.ctrlRegWord | 0x100;
  status = CAENVME_WriteCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_CTRLREG_ADDRESS , &data, CAEN_V1495PU_ADDRESSMODE,cvD32);
  data = configuration_.ctrlRegWord | 0x1000; //Enable gate
  status |= CAENVME_WriteCycle(handle_,configuration_.baseAddress + CAEN_V1495_PATTERNUNIT_CTRLREG_ADDRESS, &data ,CAEN_V1495PU_ADDRESSMODE,cvD32);  

  if (status)
    {
      s.str(""); s  << "[CAEN_V1495PU]::[ERROR]::Error clearing and re-enabling gate for deveice @0x" <<std::hex <<  configuration_.baseAddress << std::dec <<  status;
      Log(s.str(),1);
      return ERR_READ;
    }

#ifdef CAEN_V1495PU_DEBUG
  s.str(""); s  << "--- V1495 END DEBUG ---" ;
  Log(s.str(),1);
#endif   


  return 0;
}


