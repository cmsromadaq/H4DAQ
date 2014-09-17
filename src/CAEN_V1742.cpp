#include "interface/CAEN_V1742.hpp"

#include <iostream>

int CAEN_V1742::Init()
{
  int ret=CAEN_DGTZ_Success;
  ERROR_CODES ErrCode= ERR_NONE;
  
  std::cout <<"**************************************************************" << std::endl;
  std::cout <<"                        Initialise V1742" << std::endl;

  if (bC_ == NULL ) {
    return ERR_CONF_NOT_FOUND;
  }

  ret = CAEN_DGTZ_GetInfo(digitizerHandle_, &boardInfo_);

  if (ret) {
    ErrCode = ERR_BOARD_INFO_READ;
    return ErrCode;
  }

  std::cout << "Connected to CAEN Digitizer Model "<< boardInfo_.ModelName << std::endl;
  std::cout << "ROC FPGA Release is " <<  boardInfo_.ROC_FirmwareRel;
  std::cout << "AMC FPGA Release is " <<boardInfo_.AMC_FirmwareRel;
  
  // Check firmware rivision (DPP firmwares cannot be used with WaveDump */
  int MajorNumber;
  sscanf(boardInfo_.AMC_FirmwareRel, "%d", &MajorNumber);
  if (MajorNumber >= 128) {
    printf("This digitizer has a DPP firmware\n");
    ErrCode = ERR_INVALID_BOARD_TYPE;
    return ErrCode;
  }
  
  // get num of channels, num of bit, num of group of the board */
  ret = (CAEN_DGTZ_ErrorCode) getMoreBoardInfo();
  if (ret) {
    ErrCode = ERR_INVALID_BOARD_TYPE;
    return ErrCode;
  }
  
  // if( digitizerConfiguration_.useCorrections == -1 ) { // only automatic corrections supported for the moment
  ret = CAEN_DGTZ_LoadDRS4CorrectionData(digitizerHandle_,CAEN_DGTZ_DRS4_5GHz);
  ret = CAEN_DGTZ_EnableDRS4Correction(digitizerHandle_);

  // mask the channels not available for this model
  if ((boardInfo_.FamilyCode != CAEN_DGTZ_XX740_FAMILY_CODE) && (boardInfo_.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE)){
    digitizerConfiguration_.EnableMask &= (1<<digitizerConfiguration_.Nch)-1;
  } else {
    digitizerConfiguration_.EnableMask &= (1<<(digitizerConfiguration_.Nch/8))-1;
  }
  if ((boardInfo_.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) && digitizerConfiguration_.DesMode) {
    digitizerConfiguration_.EnableMask &= 0xAA;
  }
  if ((boardInfo_.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE) && digitizerConfiguration_.DesMode) {
    digitizerConfiguration_.EnableMask &= 0x55;
  }
  
  /* *************************************************************************************** */
  /* program the digitizer                                                                   */
  /* *************************************************************************************** */

  ret = (CAEN_DGTZ_ErrorCode) programDigitizer();
  if (ret) {
    ErrCode = ERR_DGZ_PROGRAM;
    return ErrCode;
  }

  ret = CAEN_DGTZ_GetInfo(digitizerHandle_, &boardInfo_);
  if (ret) {
    ErrCode = ERR_BOARD_INFO_READ;
    return ErrCode;
  }
  ret = (CAEN_DGTZ_ErrorCode) getMoreBoardInfo();
  if (ret) {
    ErrCode = ERR_INVALID_BOARD_TYPE;
    return ErrCode;
  }
  
  CAEN_DGTZ_SWStartAcquisition(digitizerHandle_);
  std::cout << "**************************************************************" << std::endl;

  return 0;
};

int CAEN_V1742::Clear(){
  int i,j, ret = 0;
  
  /* reset the digitizer */
  ret |= CAEN_DGTZ_Reset(digitizerHandle_);

  if (ret != 0) {
    std::cout << "Error: Unable to reset digitizer.\nPlease reset digitizer manually then restart the program" << std::endl;
    return ERR_RESTART;
  }

  return 0;
};

int CAEN_V1742::BufferClear(){};

int CAEN_V1742::Config(BoardConfig *bC)
{
  Board::Config(bC);
  ParseConfiguration();
};

int CAEN_V1742::Read(vector<WORD> &v){};

int CAEN_V1742::SetHandle(vector<WORD> &v){};

int CAEN_V1742::getMoreBoardInfo(){};

int CAEN_V1742::programDigitizer()
{
  int i,j, ret = 0;

  /* reset the digitizer */
  ret |= CAEN_DGTZ_Reset(digitizerHandle_);
  if (ret != 0) {
    std::cout << "Error: Unable to reset digitizer.\nPlease reset digitizer manually then restart the program" << std::endl;
    return ERR_RESTART;
  }
  /* execute generic write commands */
  for(i=0; i<digitizerConfiguration_.GWn; i++)
    ret |= CAEN_DGTZ_WriteRegister(digitizerHandle_, digitizerConfiguration_.GWaddr[i], digitizerConfiguration_.GWdata[i]);

  // Set the waveform test bit for debugging
  if (digitizerConfiguration_.TestPattern)
    ret |= CAEN_DGTZ_WriteRegister(digitizerHandle_, CAEN_DGTZ_BROAD_CH_CONFIGBIT_SET_ADD, 1<<3);
  // custom setting for X742 boards
  if (boardInfo_.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
    ret |= CAEN_DGTZ_SetFastTriggerDigitizing(digitizerHandle_,(CAEN_DGTZ_EnaDis_t)digitizerConfiguration_.FastTriggerEnabled);
    ret |= CAEN_DGTZ_SetFastTriggerMode(digitizerHandle_,(CAEN_DGTZ_TriggerMode_t)digitizerConfiguration_.FastTriggerMode);
  }
  if ((boardInfo_.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) || (boardInfo_.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE)) {
    ret |= CAEN_DGTZ_SetDESMode(digitizerHandle_, ( CAEN_DGTZ_EnaDis_t) digitizerConfiguration_.DesMode);
  }
  ret |= CAEN_DGTZ_SetRecordLength(digitizerHandle_, digitizerConfiguration_.RecordLength);
  ret |= CAEN_DGTZ_SetPostTriggerSize(digitizerHandle_, (uint32_t) digitizerConfiguration_.PostTrigger);
  if(boardInfo_.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE)
    ret |= CAEN_DGTZ_GetPostTriggerSize(digitizerHandle_, &digitizerConfiguration_.PostTrigger);
  ret |= CAEN_DGTZ_SetIOLevel(digitizerHandle_, (CAEN_DGTZ_IOLevel_t) digitizerConfiguration_.FPIOtype);
  if( digitizerConfiguration_.InterruptNumEvents > 0) {
      // Interrupt handling
      if( ret |= CAEN_DGTZ_SetInterruptConfig( digitizerHandle_, CAEN_DGTZ_ENABLE,
                                               VME_INTERRUPT_LEVEL, VME_INTERRUPT_STATUS_ID,
                                               digitizerConfiguration_.InterruptNumEvents, INTERRUPT_MODE)!= CAEN_DGTZ_Success) {
          printf( "\nError configuring interrupts. Interrupts disabled\n\n");
          digitizerConfiguration_.InterruptNumEvents = 0;
      }
      printf ("Interrupt enabled\n");
  }
  ret |= CAEN_DGTZ_SetMaxNumEventsBLT(digitizerHandle_, digitizerConfiguration_.NumEvents);
  ret |= CAEN_DGTZ_SetAcquisitionMode(digitizerHandle_, CAEN_DGTZ_SW_CONTROLLED);
  ret |= CAEN_DGTZ_SetExtTriggerInputMode(digitizerHandle_, digitizerConfiguration_.ExtTriggerMode);

  if ((boardInfo_.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE) || (boardInfo_.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE)){
    ret |= CAEN_DGTZ_SetGroupEnableMask(digitizerHandle_, digitizerConfiguration_.EnableMask);
    for(i=0; i<(digitizerConfiguration_.Nch/8); i++) {
      if (digitizerConfiguration_.EnableMask & (1<<i)) {
	if (boardInfo_.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
	  for(j=0; j<8; j++) {
	    if (digitizerConfiguration_.DCoffsetGrpCh[i][j] != -1)
	      ret |= CAEN_DGTZ_SetChannelDCOffset(digitizerHandle_,(i*8)+j, digitizerConfiguration_.DCoffsetGrpCh[i][j]);
	    else
	      ret |= CAEN_DGTZ_SetChannelDCOffset(digitizerHandle_,(i*8)+j, digitizerConfiguration_.DCoffset[i]);
	  }
	}
	else {
	  ret |= CAEN_DGTZ_SetGroupDCOffset(digitizerHandle_, i, digitizerConfiguration_.DCoffset[i]);
	  ret |= CAEN_DGTZ_SetGroupSelfTrigger(digitizerHandle_, digitizerConfiguration_.ChannelTriggerMode[i], (1<<i));
	  ret |= CAEN_DGTZ_SetGroupTriggerThreshold(digitizerHandle_, i, digitizerConfiguration_.Threshold[i]);
	  ret |= CAEN_DGTZ_SetChannelGroupMask(digitizerHandle_, i, digitizerConfiguration_.GroupTrgEnableMask[i]);
	} 
	ret |= CAEN_DGTZ_SetTriggerPolarity(digitizerHandle_, i, (CAEN_DGTZ_TriggerPolarity_t) digitizerConfiguration_.TriggerEdge);
                
      }
    }
  } else {
    ret |= CAEN_DGTZ_SetChannelEnableMask(digitizerHandle_, digitizerConfiguration_.EnableMask);
    for(i=0; i<digitizerConfiguration_.Nch; i++) {
      if (digitizerConfiguration_.EnableMask & (1<<i)) {
	ret |= CAEN_DGTZ_SetChannelDCOffset(digitizerHandle_, i, digitizerConfiguration_.DCoffset[i]);
	ret |= CAEN_DGTZ_SetChannelSelfTrigger(digitizerHandle_, digitizerConfiguration_.ChannelTriggerMode[i], (1<<i));
	ret |= CAEN_DGTZ_SetChannelTriggerThreshold(digitizerHandle_, i, digitizerConfiguration_.Threshold[i]);
	ret |= CAEN_DGTZ_SetTriggerPolarity(digitizerHandle_, i, (CAEN_DGTZ_TriggerPolarity_t) digitizerConfiguration_.TriggerEdge);
      }
    }
  }
  if (boardInfo_.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
    for(i=0; i<(digitizerConfiguration_.Nch/8); i++) {
      ret |= CAEN_DGTZ_SetGroupFastTriggerDCOffset(digitizerHandle_,i,digitizerConfiguration_.FTDCoffset[i]);
      ret |= CAEN_DGTZ_SetGroupFastTriggerThreshold(digitizerHandle_,i,digitizerConfiguration_.FTThreshold[i]);
    }
  }
    
  if (ret)
    {
      printf("Warning: errors found during the programming of the digitizer.\nSome settings may not be executed\n");
      return ERR_DGZ_PROGRAM;
    }

  return 0;
};

int CAEN_V1742::writeEventToOutputBuffer(vector<WORD>& CAEN_V1742_eventBuffer, CAEN_DGTZ_EventInfo_t *EventInfo, CAEN_DGTZ_X742_EVENT_t *Event){};

//Fill V1742 config struct from xmlNode
int CAEN_V1742::ParseConfiguration()
{
};
