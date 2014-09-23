#include "interface/CAEN_V1742.hpp"

#include <iostream>

int CAEN_V1742::Init()
{
  int ret=CAEN_DGTZ_Success;
  ERROR_CODES ErrCode= ERR_NONE;
  
  std::cout << "[V1742]::[INFO]::++++++ CAEN V1742 INIT ++++++" << std::endl;
  if (bC_ == NULL ) {
    return ERR_CONF_NOT_FOUND;
  }

  ret = CAEN_DGTZ_GetInfo(digitizerHandle_, &boardInfo_);

  if (ret) {
    ErrCode = ERR_BOARD_INFO_READ;
    return ErrCode;
  }

  std::cout << "[V1742]::[INFO]::Connected to CAEN Digitizer Model "<< boardInfo_.ModelName << std::endl;
  std::cout << "[V1742]::[INFO]::ROC FPGA Release is " <<  boardInfo_.ROC_FirmwareRel;
  std::cout << "[V1742]::[INFO]::AMC FPGA Release is " <<boardInfo_.AMC_FirmwareRel;
  
  // Check firmware rivision (DPP firmwares cannot be used with WaveDump */
  int MajorNumber;
  sscanf(boardInfo_.AMC_FirmwareRel, "%d", &MajorNumber);
  if (MajorNumber >= 128) {
    std::cout << "[V1742]::[ERROR]::This digitizer has a DPP firmware" << std::endl;
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
  std::cout << "[V1742]::[INFO]::++++++ CAEN V1742 END INIT ++++++" << std::endl;
  std::cout << "**************************************************************" << std::endl;

  return 0;
};

int CAEN_V1742::Clear(){
  int i,j, ret = 0;
  
  /* reset the digitizer */
  ret |= CAEN_DGTZ_Reset(digitizerHandle_);

  if (ret != 0) {
    std::cout << "[V1742]::[ERROR]::Unable to reset digitizer.\nPlease reset digitizer manually then restart the program" << std::endl;
    return ERR_RESTART;
  }

  return 0;
};

int CAEN_V1742::BufferClear(){};

int CAEN_V1742::Config(BoardConfig *bC)
{
  Board::Config(bC); // configure common element from the Board
  ParseConfiguration();
};

int CAEN_V1742::Read(vector<WORD> &v)
{
  CAEN_DGTZ_ErrorCode ret=CAEN_DGTZ_Success;
  ERROR_CODES ErrCode= ERR_NONE;
  int i;
  //, Nb=0, Ne=0;
  uint32_t BufferSize, NumEvents,Nb=0,Ne=0;
  CAEN_DGTZ_EventInfo_t       EventInfo;

  char *v1742_buffer;
  char *v1742_eventPtr;
  
  v1742_buffer=NULL;

  uint32_t AllocatedSize;

  CAEN_DGTZ_X742_EVENT_t       *Event742=NULL;  /* custom event struct */
  
  ret = CAEN_DGTZ_AllocateEvent(digitizerHandle_, (void**)&Event742);
  if (ret != CAEN_DGTZ_Success) {
    ErrCode = ERR_MALLOC;
    return ErrCode;
  }

  /* printf("allocated event %d\n",ret); */
  ret = CAEN_DGTZ_MallocReadoutBuffer(digitizerHandle_, &v1742_buffer, &AllocatedSize); /* WARNING: This malloc must be done after the digitizer programming */
  /* printf("malloc %d %d\n",AllocatedSize,ret); */
  if (ret) {
    ErrCode = ERR_MALLOC;
    return ErrCode;
  }

  BufferSize = 0;
  NumEvents = 0;

  while (NumEvents==0)
    {
      ret = CAEN_DGTZ_ReadData(digitizerHandle_, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, v1742_buffer, &BufferSize);
      
      if (ret) {
	ErrCode = ERR_READOUT;
	return ErrCode;
      }
      
      if (BufferSize != 0) {
	ret = CAEN_DGTZ_GetNumEvents(digitizerHandle_, v1742_buffer, BufferSize, &NumEvents);
	if (ret) {
	  ErrCode = ERR_READOUT;
	  return ErrCode;
	}
      }
    }
  
  //For the moment empty the buffers one by one
  if (NumEvents != 1)
    {
      ErrCode = ERR_MISMATCH_EVENTS;
      return ErrCode;
    }
  
  /* Analyze data */
  for(i = 0; i < (int)NumEvents; i++) {
    /* Get one event from the readout buffer */
    ret = CAEN_DGTZ_GetEventInfo(digitizerHandle_, v1742_buffer, BufferSize, i, &EventInfo, &v1742_eventPtr);
    if (ret) {
      ErrCode = ERR_EVENT_BUILD;
      return ErrCode;
    }
    ret = CAEN_DGTZ_DecodeEvent(digitizerHandle_, v1742_eventPtr, (void**)&Event742);
    if (ret) {
      ErrCode = ERR_EVENT_BUILD;
      return ErrCode;
    }    
    // if(digitizerConfiguration_.useCorrections != -1) { // if manual corrections
    // 	ApplyDataCorrection( 0, digitizerConfiguration_.useCorrections, CAEN_DGTZ_DRS4_5GHz, &(Event742->DataGroup[0]), &Table_gr0);
    // 	ApplyDataCorrection( 1, digitizerConfiguration_.useCorrections, CAEN_DGTZ_DRS4_5GHz, &(Event742->DataGroup[1]), &Table_gr1);
    // 	  }
    ret = (CAEN_DGTZ_ErrorCode) writeEventToOutputBuffer(v,&EventInfo,Event742);
    if (ret) {
      ErrCode = ERR_EVENT_BUILD;
      return ErrCode;
    }    
  }
    

      //      printf("%d %d\n",Nb,Ne);
      //      sleep(1);
  
  /* //Freeing V1742 memory  after read */
  free(v1742_buffer);
  //  free(v1742_eventPtr);
  delete(Event742);

  // Test what happens when enable this. Do we need to malloc again? To be checked
  /* ret = CAEN_DGTZ_FreeReadoutBuffer(&v1742_buffer); */
  /* if (ret) { */
  /*   ErrCode = ERR_FREE_BUFFER; */
  /*   return ErrCode; */
  /* } */
  
  return 0;

};

int CAEN_V1742::getMoreBoardInfo()
{
  CAEN_DGTZ_DRS4Frequency_t freq;
  int ret;
  switch(boardInfo_.FamilyCode) {
  case CAEN_DGTZ_XX724_FAMILY_CODE: digitizerConfiguration_.Nbit = 14; digitizerConfiguration_.Ts = 10.0; break;
  case CAEN_DGTZ_XX720_FAMILY_CODE: digitizerConfiguration_.Nbit = 12; digitizerConfiguration_.Ts = 4.0;  break;
  case CAEN_DGTZ_XX721_FAMILY_CODE: digitizerConfiguration_.Nbit =  8; digitizerConfiguration_.Ts = 2.0;  break;
  case CAEN_DGTZ_XX731_FAMILY_CODE: digitizerConfiguration_.Nbit =  8; digitizerConfiguration_.Ts = 2.0;  break;
  case CAEN_DGTZ_XX751_FAMILY_CODE: digitizerConfiguration_.Nbit = 10; digitizerConfiguration_.Ts = 1.0;  break;
  case CAEN_DGTZ_XX761_FAMILY_CODE: digitizerConfiguration_.Nbit = 10; digitizerConfiguration_.Ts = 0.25;  break;
  case CAEN_DGTZ_XX740_FAMILY_CODE: digitizerConfiguration_.Nbit = 12; digitizerConfiguration_.Ts = 16.0; break;
  case CAEN_DGTZ_XX742_FAMILY_CODE: 
    digitizerConfiguration_.Nbit = 12; 
    if ((ret = CAEN_DGTZ_GetDRS4SamplingFrequency(digitizerHandle_, &freq)) != CAEN_DGTZ_Success) return (int)CAEN_DGTZ_CommError;
    switch (freq) {
    case CAEN_DGTZ_DRS4_1GHz:
      digitizerConfiguration_.Ts = 1.0;
      break;
    case CAEN_DGTZ_DRS4_2_5GHz:
      digitizerConfiguration_.Ts = (float)0.4;
      break;
    case CAEN_DGTZ_DRS4_5GHz:
      digitizerConfiguration_.Ts = (float)0.2;
      break;
    }
    break;
  default: return -1;
  }
  if (((boardInfo_.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) ||
       (boardInfo_.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE) ) && digitizerConfiguration_.DesMode)
    digitizerConfiguration_.Ts /= 2;
	
  switch(boardInfo_.FamilyCode) {
  case CAEN_DGTZ_XX724_FAMILY_CODE:
  case CAEN_DGTZ_XX720_FAMILY_CODE:
  case CAEN_DGTZ_XX721_FAMILY_CODE:
  case CAEN_DGTZ_XX751_FAMILY_CODE:
  case CAEN_DGTZ_XX761_FAMILY_CODE:
  case CAEN_DGTZ_XX731_FAMILY_CODE:
    switch(boardInfo_.FormFactor) {
    case CAEN_DGTZ_VME64_FORM_FACTOR:
    case CAEN_DGTZ_VME64X_FORM_FACTOR:
      digitizerConfiguration_.Nch = 8;
      break;
    case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
    case CAEN_DGTZ_NIM_FORM_FACTOR:
      digitizerConfiguration_.Nch = 4;
      break;
    }
    break;
  case CAEN_DGTZ_XX740_FAMILY_CODE:
    switch( boardInfo_.FormFactor) {
    case CAEN_DGTZ_VME64_FORM_FACTOR:
    case CAEN_DGTZ_VME64X_FORM_FACTOR:
      digitizerConfiguration_.Nch = 64;
      break;
    case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
    case CAEN_DGTZ_NIM_FORM_FACTOR:
      digitizerConfiguration_.Nch = 32;
      break;
    }
    break;
  case CAEN_DGTZ_XX742_FAMILY_CODE:
    switch( boardInfo_.FormFactor) {
    case CAEN_DGTZ_VME64_FORM_FACTOR:
    case CAEN_DGTZ_VME64X_FORM_FACTOR:
      digitizerConfiguration_.Nch = 36;
      break;
    case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
    case CAEN_DGTZ_NIM_FORM_FACTOR:
      digitizerConfiguration_.Nch = 16;
      break;
    }
    break;
  default:
    return -1;
  }
  return 0;
};

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
                                               CAEN_V1742_VME_INTERRUPT_LEVEL, CAEN_V1742_VME_INTERRUPT_STATUS_ID,
                                               digitizerConfiguration_.InterruptNumEvents, CAEN_V1742_INTERRUPT_MODE)!= CAEN_DGTZ_Success) {
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

int CAEN_V1742::writeEventToOutputBuffer(vector<WORD>& CAEN_V1742_eventBuffer, CAEN_DGTZ_EventInfo_t* eventInfo, CAEN_DGTZ_X742_EVENT_t* event)
{
  int gr,ch;
  
  //          ====================================================
  //          |           V1742 Raw Event Data Format            |
  //          ====================================================

  //                       31  -  28 27  -  16 15   -   0
  //            Word[0] = [ 1010  ] [Event Tot #Words  ] //Event Header (5 words)
  //            Word[1] = [     Board Id    ] [ Pattern]  
  //            Word[2] = [      #channels readout     ]
  //            Word[3] = [        Event counter       ]
  //            Word[4] = [      Trigger Time Tag      ]
  //            Word[5] = [ 1000  ] [    Ch0   #Words  ] // Ch0 Data (2 + #samples words)
  //            Word[6] = [    Ch0  #Gr    ] [ Ch0 #Ch ] 
  //            Word[7] = [ Ch0 Corr. samples  (float) ]
  //                ..  = [ Ch0 Corr. samples  (float) ]
  // Word[5+Ch0 #Words] = [ 1000  ] [    Ch1   #Words  ] // Ch1 Data (2 + #samples words)
  // Word[6+Ch0 #Words] = [    Ch1  #Gr    ] [ Ch1 #Ch ]
  // Word[7+Ch0 #Words] = [ Ch1 Corr. samples  (float) ]
  //               ...   = [          .....             ]

  //CAEN_V1742_eventBuffer.clear();
  CAEN_V1742_eventBuffer.resize(5);
  (CAEN_V1742_eventBuffer)[0]=0xA0000005; 
  (CAEN_V1742_eventBuffer)[1]=((eventInfo->BoardId)<<26)+eventInfo->Pattern;
  (CAEN_V1742_eventBuffer)[2]=0;
  (CAEN_V1742_eventBuffer)[3]=eventInfo->EventCounter;
  (CAEN_V1742_eventBuffer)[4]=eventInfo->TriggerTimeTag;

  //  printf("EVENT 1742 %d %d\n",eventInfo->EventCounter,eventInfo->TriggerTimeTag);
  for (gr=0;gr<(digitizerConfiguration_.Nch/8);gr++) {
    if (event->GrPresent[gr]) {
      for(ch=0; ch<9; ch++) {
	int Size = event->DataGroup[gr].ChSize[ch];
	if (Size <= 0) {
	  continue;
	}

	// Channel Header for this event
 	uint32_t ChHeader[2];
	ChHeader[0] = (8<<28) + (2 + Size); //Number of words written for this channel
	ChHeader[1] = (gr<<16)+ch;

	//Starting pointer
	int start_ptr=CAEN_V1742_eventBuffer.size();

	//Allocating necessary space for this channel
	CAEN_V1742_eventBuffer.resize(CAEN_V1742_eventBuffer.size() + 2 + Size);
	memcpy(&((CAEN_V1742_eventBuffer)[start_ptr]), &ChHeader[0], 2 * sizeof(unsigned int));

	//Beware the datas are float (because they are corrected...) but copying them here bit by bit. Should remember this for reading them out
	memcpy(&((CAEN_V1742_eventBuffer)[start_ptr+2]), event->DataGroup[gr].DataChannel[ch], Size * sizeof(unsigned int));

	//Update event size and #channels
	(CAEN_V1742_eventBuffer)[0]+=(Size+2);
	(CAEN_V1742_eventBuffer)[2]++;
      }
    }
  }
  
  return 0;

}

//Fill V1742 config struct from xmlNode
int CAEN_V1742::ParseConfiguration()
{
};
