#include "interface/CAEN_V1742.hpp"

#include <iostream>
#include <sstream>

//PG settings for the board

#define MAX_CH  64          /* max. number of channels */
#define MAX_SET 8           /* max. number of independent settings */
#define MAX_GW  1000        /* max. number of generic write commads */

//#define VME_INTERRUPT_LEVEL      1
//#define VME_INTERRUPT_STATUS_ID  0xAAAA
//#define INTERRUPT_MODE           CAEN_DGTZ_IRQ_MODE_ROAK
//#define INTERRUPT_TIMEOUT        200  // ms


using namespace std ;

int CAEN_V1742::Init ()
{
  int ret = CAEN_DGTZ_Success ;
  ERROR_CODES ErrCode = ERR_NONE ;
  
  cout << "[V1742]::[INFO]::++++++ CAEN V1742 INIT ++++++" << endl ;
  if (bC_ == NULL ) //PG FIXME what is bC_?
    {
      return ERR_CONF_NOT_FOUND ;
    }

  ret = CAEN_DGTZ_GetInfo (digitizerHandle_, &boardInfo_) ;

  if (ret) 
    {
      ErrCode = ERR_BOARD_INFO_READ ;
      return ErrCode ;
    }

  cout << "[V1742]::[INFO]::Connected to CAEN Digitizer Model "<< boardInfo_.ModelName       << endl ;
  cout << "[V1742]::[INFO]::ROC FPGA Release is "              << boardInfo_.ROC_FirmwareRel << endl ;
  cout << "[V1742]::[INFO]::AMC FPGA Release is "              << boardInfo_.AMC_FirmwareRel << endl ;
  
  // Check firmware rivision (DPP firmwares cannot be used with WaveDump */
  int MajorNumber ;
  sscanf (boardInfo_.AMC_FirmwareRel, "%d", &MajorNumber) ;
  if (MajorNumber >= 128) 
    {
      cerr << "[V1742]::[ERROR]::This digitizer has a DPP firmware" << endl ;
      ErrCode = ERR_INVALID_BOARD_TYPE ;
      return ErrCode ;
    }
  
  // get num of channels, num of bit, num of group of the board */
  ret = (CAEN_DGTZ_ErrorCode) getMoreBoardInfo () ;
  if (ret) 
    {
      ErrCode = ERR_INVALID_BOARD_TYPE ;
      return ErrCode ;
    }
  
  // if ( digitizerConfiguration_.useCorrections == -1 ) { // only automatic corrections supported for the moment
  ret = CAEN_DGTZ_LoadDRS4CorrectionData (digitizerHandle_,CAEN_DGTZ_DRS4_5GHz) ;
  ret = CAEN_DGTZ_EnableDRS4Correction (digitizerHandle_) ;

  // mask the channels not available for this model
  if ( (boardInfo_.FamilyCode != CAEN_DGTZ_XX740_FAMILY_CODE) && 
       (boardInfo_.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE))
    {
      digitizerConfiguration_.EnableMask &= (1<<digitizerConfiguration_.Nch)-1 ;
    } else {
      digitizerConfiguration_.EnableMask &= (1<< (digitizerConfiguration_.Nch/8))-1 ;
    }
  if ( (boardInfo_.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) && 
       digitizerConfiguration_.DesMode) 
    {
      digitizerConfiguration_.EnableMask &= 0xAA ;
    }
  if ( (boardInfo_.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE) && 
       digitizerConfiguration_.DesMode) 
    {
      digitizerConfiguration_.EnableMask &= 0x55 ;
    }
  
  /* *************************************************************************************** */
  /* program the digitizer                                                                   */
  /* *************************************************************************************** */

  ret = (CAEN_DGTZ_ErrorCode) programDigitizer () ;
  if (ret) 
    {
      ErrCode = ERR_DGZ_PROGRAM ;
      return ErrCode ;
    }

  ret = CAEN_DGTZ_GetInfo (digitizerHandle_, &boardInfo_) ;
  if (ret) 
    {
      ErrCode = ERR_BOARD_INFO_READ ;
      return ErrCode ;
    }

  ret = (CAEN_DGTZ_ErrorCode) getMoreBoardInfo () ;
  if (ret) 
    {
      ErrCode = ERR_INVALID_BOARD_TYPE ;
      return ErrCode ;
    }
  
  CAEN_DGTZ_SWStartAcquisition (digitizerHandle_) ;
  cout << "[V1742]::[INFO]::++++++ CAEN V1742 END INIT ++++++" << endl ;
  cout << "**************************************************************" << endl ;

  return 0 ;
} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::Clear (){
  int ret = 0 ;
  
  /* reset the digitizer */
  ret |= CAEN_DGTZ_Reset (digitizerHandle_) ;

  if (ret != 0) {
    cout << "[V1742]::[ERROR]::Unable to reset digitizer.\nPlease reset digitizer manually then restart the program" << endl ;
    return ERR_RESTART ;
  }

  return 0 ;
} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::BufferClear (){ return -1 ; } ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::Config (BoardConfig * bC)
{
  setDefaults () ;
  Board::Config (bC) ; //PG FIXME why do we call the base class as well?
  ParseConfiguration (bC) ;
  return 0 ;
} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::Read (vector<WORD> &v)
{
  CAEN_DGTZ_ErrorCode ret=CAEN_DGTZ_Success ;
  ERROR_CODES ErrCode= ERR_NONE ;
  int i ;
  // uint32_tint Nb=0,Ne=0  ;
  //, Nb=0, Ne=0 ;
  uint32_t BufferSize, NumEvents;
  CAEN_DGTZ_EventInfo_t       EventInfo ;

  char *v1742_buffer ;
  char *v1742_eventPtr ;
  
  v1742_buffer=NULL ;

  uint32_t AllocatedSize ;

  CAEN_DGTZ_X742_EVENT_t       *Event742=NULL ;  /* custom event struct */
  
  ret = CAEN_DGTZ_AllocateEvent (digitizerHandle_, (void**)&Event742) ;
  if (ret != CAEN_DGTZ_Success) {
    ErrCode = ERR_MALLOC ;
    return ErrCode ;
  }

  /* printf ("allocated event %d\n",ret) ; */
  ret = CAEN_DGTZ_MallocReadoutBuffer (digitizerHandle_, &v1742_buffer, &AllocatedSize) ; /* WARNING: This malloc must be done after the digitizer programming */
  /* printf ("malloc %d %d\n",AllocatedSize,ret) ; */
  if (ret) {
    ErrCode = ERR_MALLOC ;
    return ErrCode ;
  }

  BufferSize = 0 ;
  NumEvents = 0 ;

  while (NumEvents==0)
    {
      ret = CAEN_DGTZ_ReadData (digitizerHandle_, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, v1742_buffer, &BufferSize) ;
      
      if (ret) {
    ErrCode = ERR_READOUT ;
    return ErrCode ;
      }
      
      if (BufferSize != 0) {
    ret = CAEN_DGTZ_GetNumEvents (digitizerHandle_, v1742_buffer, BufferSize, &NumEvents) ;
    if (ret) {
      ErrCode = ERR_READOUT ;
      return ErrCode ;
    }
      }
    }
  
  //For the moment empty the buffers one by one
  if (NumEvents != 1)
    {
      ErrCode = ERR_MISMATCH_EVENTS ;
      return ErrCode ;
    }
  
  /* Analyze data */
  for (i = 0 ; i < (int)NumEvents ; i++) {
    /* Get one event from the readout buffer */
    ret = CAEN_DGTZ_GetEventInfo (digitizerHandle_, v1742_buffer, BufferSize, i, &EventInfo, &v1742_eventPtr) ;
    if (ret) {
      ErrCode = ERR_EVENT_BUILD ;
      return ErrCode ;
    }
    ret = CAEN_DGTZ_DecodeEvent (digitizerHandle_, v1742_eventPtr, (void**)&Event742) ;
    if (ret) {
      ErrCode = ERR_EVENT_BUILD ;
      return ErrCode ;
    }    
    // if (digitizerConfiguration_.useCorrections != -1) { // if manual corrections
    //     ApplyDataCorrection ( 0, digitizerConfiguration_.useCorrections, CAEN_DGTZ_DRS4_5GHz, & (Event742->DataGroup[0]), &Table_gr0) ;
    //     ApplyDataCorrection ( 1, digitizerConfiguration_.useCorrections, CAEN_DGTZ_DRS4_5GHz, & (Event742->DataGroup[1]), &Table_gr1) ;
    //       }
    ret = (CAEN_DGTZ_ErrorCode) writeEventToOutputBuffer (v,&EventInfo,Event742) ;
    if (ret) {
      ErrCode = ERR_EVENT_BUILD ;
      return ErrCode ;
    }    
  }
    

      //      printf ("%d %d\n",Nb,Ne) ;
      //      sleep (1) ;
  
  /* //Freeing V1742 memory  after read */
  free (v1742_buffer) ;
  //  free (v1742_eventPtr) ;
  delete (Event742) ;

  // Test what happens when enable this. Do we need to malloc again? To be checked
  /* ret = CAEN_DGTZ_FreeReadoutBuffer (&v1742_buffer) ; */
  /* if (ret) { */
  /*   ErrCode = ERR_FREE_BUFFER ; */
  /*   return ErrCode ; */
  /* } */
  
  return 0 ;

} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::getMoreBoardInfo ()
{
  CAEN_DGTZ_DRS4Frequency_t freq ;
  int ret ;
  switch (boardInfo_.FamilyCode) {
  case CAEN_DGTZ_XX724_FAMILY_CODE: digitizerConfiguration_.Nbit = 14 ; digitizerConfiguration_.Ts = 10.0 ; break ;
  case CAEN_DGTZ_XX720_FAMILY_CODE: digitizerConfiguration_.Nbit = 12 ; digitizerConfiguration_.Ts = 4.0 ;  break ;
  case CAEN_DGTZ_XX721_FAMILY_CODE: digitizerConfiguration_.Nbit =  8 ; digitizerConfiguration_.Ts = 2.0 ;  break ;
  case CAEN_DGTZ_XX731_FAMILY_CODE: digitizerConfiguration_.Nbit =  8 ; digitizerConfiguration_.Ts = 2.0 ;  break ;
  case CAEN_DGTZ_XX751_FAMILY_CODE: digitizerConfiguration_.Nbit = 10 ; digitizerConfiguration_.Ts = 1.0 ;  break ;
  case CAEN_DGTZ_XX761_FAMILY_CODE: digitizerConfiguration_.Nbit = 10 ; digitizerConfiguration_.Ts = 0.25 ;  break ;
  case CAEN_DGTZ_XX740_FAMILY_CODE: digitizerConfiguration_.Nbit = 12 ; digitizerConfiguration_.Ts = 16.0 ; break ;
  case CAEN_DGTZ_XX742_FAMILY_CODE: 
    digitizerConfiguration_.Nbit = 12 ; 
    if ( (ret = CAEN_DGTZ_GetDRS4SamplingFrequency (digitizerHandle_, &freq)) != CAEN_DGTZ_Success) return (int)CAEN_DGTZ_CommError ;
    switch (freq) {
    case CAEN_DGTZ_DRS4_1GHz:
      digitizerConfiguration_.Ts = 1.0 ;
      break ;
    case CAEN_DGTZ_DRS4_2_5GHz:
      digitizerConfiguration_.Ts = (float)0.4 ;
      break ;
    case CAEN_DGTZ_DRS4_5GHz:
      digitizerConfiguration_.Ts = (float)0.2 ;
      break ;
    }
    break ;
  default: return -1 ;
  }
  if ( ( (boardInfo_.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) ||
       (boardInfo_.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE) ) && digitizerConfiguration_.DesMode)
    digitizerConfiguration_.Ts /= 2 ;
    
  switch (boardInfo_.FamilyCode) {
  case CAEN_DGTZ_XX724_FAMILY_CODE:
  case CAEN_DGTZ_XX720_FAMILY_CODE:
  case CAEN_DGTZ_XX721_FAMILY_CODE:
  case CAEN_DGTZ_XX751_FAMILY_CODE:
  case CAEN_DGTZ_XX761_FAMILY_CODE:
  case CAEN_DGTZ_XX731_FAMILY_CODE:
    switch (boardInfo_.FormFactor) {
    case CAEN_DGTZ_VME64_FORM_FACTOR:
    case CAEN_DGTZ_VME64X_FORM_FACTOR:
      digitizerConfiguration_.Nch = 8 ;
      break ;
    case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
    case CAEN_DGTZ_NIM_FORM_FACTOR:
      digitizerConfiguration_.Nch = 4 ;
      break ;
    }
    break ;
  case CAEN_DGTZ_XX740_FAMILY_CODE:
    switch ( boardInfo_.FormFactor) {
    case CAEN_DGTZ_VME64_FORM_FACTOR:
    case CAEN_DGTZ_VME64X_FORM_FACTOR:
      digitizerConfiguration_.Nch = 64 ;
      break ;
    case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
    case CAEN_DGTZ_NIM_FORM_FACTOR:
      digitizerConfiguration_.Nch = 32 ;
      break ;
    }
    break ;
  case CAEN_DGTZ_XX742_FAMILY_CODE:
    switch ( boardInfo_.FormFactor) {
    case CAEN_DGTZ_VME64_FORM_FACTOR:
    case CAEN_DGTZ_VME64X_FORM_FACTOR:
      digitizerConfiguration_.Nch = 36 ;
      break ;
    case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
    case CAEN_DGTZ_NIM_FORM_FACTOR:
      digitizerConfiguration_.Nch = 16 ;
      break ;
    }
    break ;
  default:
    return -1 ;
  }
  return 0 ;
} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::programDigitizer ()
{
  int i,j, ret = 0 ;

  /* reset the digitizer */
  ret |= CAEN_DGTZ_Reset (digitizerHandle_) ;
  if (ret != 0) {
    cout << "Error: Unable to reset digitizer.\nPlease reset digitizer manually then restart the program" << endl ;
    return ERR_RESTART ;
  }
  /* execute generic write commands */
  for (i=0 ; i<digitizerConfiguration_.GWn ; i++)
    ret |= CAEN_DGTZ_WriteRegister (digitizerHandle_, digitizerConfiguration_.GWaddr[i], digitizerConfiguration_.GWdata[i]) ;

  // Set the waveform test bit for debugging
  if (digitizerConfiguration_.TestPattern)
    ret |= CAEN_DGTZ_WriteRegister (digitizerHandle_, CAEN_DGTZ_BROAD_CH_CONFIGBIT_SET_ADD, 1<<3) ;
  // custom setting for X742 boards
  if (boardInfo_.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
    ret |= CAEN_DGTZ_SetFastTriggerDigitizing (digitizerHandle_, (CAEN_DGTZ_EnaDis_t)digitizerConfiguration_.FastTriggerEnabled) ;
    ret |= CAEN_DGTZ_SetFastTriggerMode (digitizerHandle_, (CAEN_DGTZ_TriggerMode_t)digitizerConfiguration_.FastTriggerMode) ;
  }
  if ( (boardInfo_.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) || (boardInfo_.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE)) {
    ret |= CAEN_DGTZ_SetDESMode (digitizerHandle_, ( CAEN_DGTZ_EnaDis_t) digitizerConfiguration_.DesMode) ;
  }
  ret |= CAEN_DGTZ_SetRecordLength (digitizerHandle_, digitizerConfiguration_.RecordLength) ;
  ret |= CAEN_DGTZ_SetPostTriggerSize (digitizerHandle_, (uint32_t) digitizerConfiguration_.PostTrigger) ;
  if (boardInfo_.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE)
    ret |= CAEN_DGTZ_GetPostTriggerSize (digitizerHandle_, &digitizerConfiguration_.PostTrigger) ;
  ret |= CAEN_DGTZ_SetIOLevel (digitizerHandle_, (CAEN_DGTZ_IOLevel_t) digitizerConfiguration_.FPIOtype) ;
  if ( digitizerConfiguration_.InterruptNumEvents > 0) {
      // Interrupt handling
      if ( ret |= CAEN_DGTZ_SetInterruptConfig ( digitizerHandle_, CAEN_DGTZ_ENABLE,
                                               CAEN_V1742_VME_INTERRUPT_LEVEL, CAEN_V1742_VME_INTERRUPT_STATUS_ID,
                                               digitizerConfiguration_.InterruptNumEvents, CAEN_V1742_INTERRUPT_MODE)!= CAEN_DGTZ_Success) {
          printf ( "\nError configuring interrupts. Interrupts disabled\n\n") ;
          digitizerConfiguration_.InterruptNumEvents = 0 ;
      }
      printf ("Interrupt enabled\n") ;
  }
  ret |= CAEN_DGTZ_SetMaxNumEventsBLT (digitizerHandle_, digitizerConfiguration_.NumEvents) ;
  ret |= CAEN_DGTZ_SetAcquisitionMode (digitizerHandle_, CAEN_DGTZ_SW_CONTROLLED) ;
  ret |= CAEN_DGTZ_SetExtTriggerInputMode (digitizerHandle_, digitizerConfiguration_.ExtTriggerMode) ;

  if ( (boardInfo_.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE) || (boardInfo_.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE)){
    ret |= CAEN_DGTZ_SetGroupEnableMask (digitizerHandle_, digitizerConfiguration_.EnableMask) ;
    for (i=0 ; i< (digitizerConfiguration_.Nch/8) ; i++) {
      if (digitizerConfiguration_.EnableMask & (1<<i)) {
    if (boardInfo_.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
      for (j=0 ; j<8 ; j++) {
        if (digitizerConfiguration_.DCoffsetGrpCh[i][j] != -1)
          ret |= CAEN_DGTZ_SetChannelDCOffset (digitizerHandle_, (i*8)+j, digitizerConfiguration_.DCoffsetGrpCh[i][j]) ;
        else
          ret |= CAEN_DGTZ_SetChannelDCOffset (digitizerHandle_, (i*8)+j, digitizerConfiguration_.DCoffset[i]) ;
      }
    }
    else {
      ret |= CAEN_DGTZ_SetGroupDCOffset (digitizerHandle_, i, digitizerConfiguration_.DCoffset[i]) ;
      ret |= CAEN_DGTZ_SetGroupSelfTrigger (digitizerHandle_, digitizerConfiguration_.ChannelTriggerMode[i], (1<<i)) ;
      ret |= CAEN_DGTZ_SetGroupTriggerThreshold (digitizerHandle_, i, digitizerConfiguration_.Threshold[i]) ;
      ret |= CAEN_DGTZ_SetChannelGroupMask (digitizerHandle_, i, digitizerConfiguration_.GroupTrgEnableMask[i]) ;
    } 
    ret |= CAEN_DGTZ_SetTriggerPolarity (digitizerHandle_, i, (CAEN_DGTZ_TriggerPolarity_t) digitizerConfiguration_.TriggerEdge) ;
                
      }
    }
  } else {
    ret |= CAEN_DGTZ_SetChannelEnableMask (digitizerHandle_, digitizerConfiguration_.EnableMask) ;
    for (i=0 ; i<digitizerConfiguration_.Nch ; i++) {
      if (digitizerConfiguration_.EnableMask & (1<<i)) {
    ret |= CAEN_DGTZ_SetChannelDCOffset (digitizerHandle_, i, digitizerConfiguration_.DCoffset[i]) ;
    ret |= CAEN_DGTZ_SetChannelSelfTrigger (digitizerHandle_, digitizerConfiguration_.ChannelTriggerMode[i], (1<<i)) ;
    ret |= CAEN_DGTZ_SetChannelTriggerThreshold (digitizerHandle_, i, digitizerConfiguration_.Threshold[i]) ;
    ret |= CAEN_DGTZ_SetTriggerPolarity (digitizerHandle_, i, (CAEN_DGTZ_TriggerPolarity_t) digitizerConfiguration_.TriggerEdge) ;
      }
    }
  }
  if (boardInfo_.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
    for (i=0 ; i< (digitizerConfiguration_.Nch/8) ; i++) {
      ret |= CAEN_DGTZ_SetGroupFastTriggerDCOffset (digitizerHandle_,i,digitizerConfiguration_.FTDCoffset[i]) ;
      ret |= CAEN_DGTZ_SetGroupFastTriggerThreshold (digitizerHandle_,i,digitizerConfiguration_.FTThreshold[i]) ;
    }
  }
    
  if (ret)
    {
      printf ("Warning: errors found during the programming of the digitizer.\nSome settings may not be executed\n") ;
      return ERR_DGZ_PROGRAM ;
    }

  return 0 ;
} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::writeEventToOutputBuffer (vector<WORD>& CAEN_V1742_eventBuffer, CAEN_DGTZ_EventInfo_t* eventInfo, CAEN_DGTZ_X742_EVENT_t* event)
{
  int gr,ch ;
  
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

  //CAEN_V1742_eventBuffer.clear () ;
  CAEN_V1742_eventBuffer.resize (5) ;
  (CAEN_V1742_eventBuffer)[0]=0xA0000005 ; 
  (CAEN_V1742_eventBuffer)[1]= ( (eventInfo->BoardId)<<26)+eventInfo->Pattern ;
  (CAEN_V1742_eventBuffer)[2]=0 ;
  (CAEN_V1742_eventBuffer)[3]=eventInfo->EventCounter ;
  (CAEN_V1742_eventBuffer)[4]=eventInfo->TriggerTimeTag ;

  //  printf ("EVENT 1742 %d %d\n",eventInfo->EventCounter,eventInfo->TriggerTimeTag) ;
  for (gr=0 ;gr< (digitizerConfiguration_.Nch/8) ;gr++) {
    if (event->GrPresent[gr]) {
      for (ch=0 ; ch<9 ; ch++) {
    int Size = event->DataGroup[gr].ChSize[ch] ;
    if (Size <= 0) {
      continue ;
    }

    // Channel Header for this event
     uint32_t ChHeader[2] ;
    ChHeader[0] = (8<<28) + (2 + Size) ; //Number of words written for this channel
    ChHeader[1] = (gr<<16)+ch ;

    //Starting pointer
    int start_ptr=CAEN_V1742_eventBuffer.size () ;

    //Allocating necessary space for this channel
    CAEN_V1742_eventBuffer.resize (CAEN_V1742_eventBuffer.size () + 2 + Size) ;
    memcpy (& ( (CAEN_V1742_eventBuffer)[start_ptr]), &ChHeader[0], 2 * sizeof (unsigned int)) ;

    //Beware the datas are float (because they are corrected...) but copying them here bit by bit. Should remember this for reading them out
    memcpy (& ( (CAEN_V1742_eventBuffer)[start_ptr+2]), event->DataGroup[gr].DataChannel[ch], Size * sizeof (unsigned int)) ;

    //Update event size and #channels
    (CAEN_V1742_eventBuffer)[0]+= (Size+2) ;
    (CAEN_V1742_eventBuffer)[2]++ ;
      }
    }
  }
  
  return 0 ;

}


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


//Fill V1742 config struct from xmlNode
int CAEN_V1742::ParseConfiguration (BoardConfig * bC)
{

  int tr = -1 ;
  int ch = -1 ;
/* PG FIXME this still needs to be handled
    // Section (COMMON or individual channel) //PG FIXME still to be traduced, once I got what TR blah is
    if (str[0] == '[') 
        {
          if (strstr (str, "COMMON")) 
              {
                ch = -1 ;
                continue ; 
              }
          if (strstr (str, "TR")) 
            {
              sscanf (str+1, "TR%d", &val) ;
              if (val < 0 || val >= MAX_SET) 
                {
                  printf ("%s: Invalid channel number\n", str) ;
                } else {
                  tr = val ;
                }
            } else {
              sscanf (str+1, "%d", &val) ;
              if (val < 0 || val >= MAX_SET) 
                {
                  printf ("%s: Invalid channel number\n", str) ;
                } else {
                  ch = val ;
                }
            }
        }
*/
        
    // OPEN: read the details of physical path to the digitizer
    string content = bC->getElementContent ("OPEN") ;
    if (content.find ("NULL") != string::npos)
        {
          std::stringstream ststream (content) ; //PG FIXME se facessi ritornare un sstram direttamente?
          string linkType ;
          ststream >> linkType ;
          if      (linkType == "USB") digitizerConfiguration_.LinkType = CAEN_DGTZ_USB ;
          else if (linkType == "PCI") digitizerConfiguration_.LinkType = CAEN_DGTZ_PCI_OpticalLink ;
          else 
            {
              cerr << "[V1742]::[ERROR]::Invalid connection type: " << linkType << endl ;
              return -1 ; 
            }
          ststream >> digitizerConfiguration_.LinkNum ;
          if (digitizerConfiguration_.LinkType == CAEN_DGTZ_USB) digitizerConfiguration_.ConetNode = 0 ;
          else                                 ststream >> digitizerConfiguration_.ConetNode ;
          ststream >> digitizerConfiguration_.BaseAddress ;
        }
    else 
      {
        cerr << "[V1742]::[ERROR]:: Field OPEN not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    // Generic VME Write (address offset + data, both exadecimal)
    vector<pair<string, string> > contentList = bC->getNodeContentList ("WRITE_REGISTER") ;
    if (contentList.size () > 0 &&
        (digitizerConfiguration_.GWn < MAX_GW))
      {
        for (int i = 0 ; i < contentList.size () ; ++i)
          {
            if (digitizerConfiguration_.GWn >= MAX_GW) 
              {
                cout << "[V1742]::[WARNING]:: not all the WRITE_REGISTER fields have been acquired" << endl ;
                break ;
              }
            digitizerConfiguration_.GWaddr[digitizerConfiguration_.GWn] = stoi (contentList.at (i).first) ;
            digitizerConfiguration_.GWdata[digitizerConfiguration_.GWn] = stoi (contentList.at (i).second) ;
            digitizerConfiguration_.GWn++ ;
      }

    // Acquisition Record Length (number of samples)
    content = bC->getElementContent ("RECORD_LENGTH") ;
    if (content.find ("NULL") != string::npos)
        {
          std::stringstream ststream (content) ;
          ststream >> digitizerConfiguration_.RecordLength ;
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field RECORD_LENGTH not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    // Correction Level (mask)
    content = bC->getElementContent ("CORRECTION_LEVEL") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          string isAuto ;
          ststream >> isAuto ;
          if (isAuto == "AUTO") digitizerConfiguration_.useCorrections = -1 ;
          else                  digitizerConfiguration_.useCorrections = atoi (isAuto.c_str ()) ;
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field CORRECTION_LEVEL not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    // Test Pattern
    content = bC->getElementContent ("TEST_PATTERN") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          string dummy ;
          ststream >> dummy ;
          if (dummy == "YES") digitizerConfiguration_.TestPattern = 1 ;
          else if (dummy != "NO")
            {
               cout << "[V1742]::[WARNING]:: TEST_PATTERN " << dummy << " is an invalid option" << endl ;
               //PG FIXME abort run start?
            }
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field TEST_PATTERN not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    // Trigger Edge
    content = bC->getElementContent ("TRIGGER_EDGE") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          string dummy ;
          ststream >> dummy ;
          if (dummy == "FALLING") digitizerConfiguration_.TriggerEdge = 1 ;
          else if (dummy != "RISING")
            {
               cout << "[V1742]::[WARNING]:: TRIGGER_EDGE " << dummy << " is an invalid option" << endl ;
               //PG FIXME abort run start?
            }
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field TRIGGER_EDGE not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    // External Trigger (DISABLED, ACQUISITION_ONLY, ACQUISITION_AND_TRGOUT)
    content = bC->getElementContent ("EXTERNAL_TRIGGER") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          string dummy ;
          ststream >> dummy ;
          if (dummy == "DISABLED")                    digitizerConfiguration_.ExtTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED ;
          else if (dummy == "ACQUISITION_ONLY")       digitizerConfiguration_.ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY ;
          else if (dummy == "ACQUISITION_AND_TRGOUT") digitizerConfiguration_.ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT ;
          else
            {
               cout << "[V1742]::[WARNING]:: EXTERNAL_TRIGGER " << dummy << " is an invalid option" << endl ;
               //PG FIXME abort run start?
            }
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field EXTERNAL_TRIGGER not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    // Max. number of events for a block transfer (0 to 1023)
    content = bC->getElementContent ("MAX_NUM_EVENTS_BLT") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          ststream >> digitizerConfiguration_.NumEvents ;
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field MAX_NUM_EVENTS_BLT not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    //PG FIXME did not translate this yet
    /* // GNUplot path */
    /* if (strstr (str, "GNUPLOT_PATH")!=NULL) { */
    /*     read = fscanf (f_ini, "%s", digitizerConfiguration_.GnuPlotPath) ; */
    /*     continue ; */
    /* } */

    // Post Trigger (percent of the acquisition window)
    content = bC->getElementContent ("POST_TRIGGER") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          ststream >> digitizerConfiguration_.PostTrigger ;
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field POST_TRIGGER not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    // DesMode (Double sampling frequency for the Mod 731 and 751)
    content = bC->getElementContent ("ENABLE_DES_MODE") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          string dummy ;
          ststream >> dummy ;
          if (dummy == "YES") digitizerConfiguration_.DesMode = 1 ;
          else if (dummy != "NO")
            {
               cout << "[V1742]::[WARNING]:: ENABLE_DES_MODE " << dummy << " is an invalid option" << endl ;
               //PG FIXME abort run start?
            }
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field ENABLE_DES_MODE not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    //PG FIXME did not translate this yet
    /* // Output file format (BINARY or ASCII) */
    /* if (strstr (str, "OUTPUT_FILE_FORMAT")!=NULL) { */
    /*     read = fscanf (f_ini, "%s", str1) ; */
    /*     if (strcmp (str1, "BINARY")==0) */
    /*         digitizerConfiguration_.OutFileFlags|= OFF_BINARY ; */
    /*     else if (strcmp (str1, "ASCII")!=0) */
    /*         printf ("%s: invalid output file format\n", str1) ; */
    /*     continue ; */
    /* } */

    //PG FIXME did not translate this yet
    /* // Header into output file (YES or NO) */
    /* if (strstr (str, "OUTPUT_FILE_HEADER")!=NULL) { */
    /*     read = fscanf (f_ini, "%s", str1) ; */
    /*     if (strcmp (str1, "YES")==0) */
    /*         digitizerConfiguration_.OutFileFlags|= OFF_HEADER ; */
    /*     else if (strcmp (str1, "NO")!=0) */
    /*         printf ("%s: invalid option\n", str) ; */
    /*     continue ; */
    /* } */

    //    Interrupt settings (request interrupt when there are at least N events to read ; 0=disable interrupts (polling mode))
    content = bC->getElementContent ("USE_INTERRUPT") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          ststream >> digitizerConfiguration_.InterruptNumEvents ;
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field USE_INTERRUPT not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    content = bC->getElementContent ("FAST_TRIGGER") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          string dummy ;
          ststream >> dummy ;
          if (dummy == "DISABLED")         digitizerConfiguration_.FastTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED ;
          if (dummy == "ACQUISITION_ONLY") digitizerConfiguration_.FastTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY ;
          else if (dummy != "NO")
            {
               cout << "[V1742]::[WARNING]:: FAST_TRIGGER " << dummy << " is an invalid option" << endl ;
               //PG FIXME abort run start?
            }
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field FAST_TRIGGER not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    content = bC->getElementContent ("ENABLED_FAST_TRIGGER_DIGITIZING") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          string dummy ;
          ststream >> dummy ;
          if (dummy == "YES") digitizerConfiguration_.FastTriggerEnabled= 1 ;
          else if (dummy != "NO")
            {
               cout << "[V1742]::[WARNING]:: ENABLED_FAST_TRIGGER_DIGITIZING " << dummy << " is an invalid option" << endl ;
               //PG FIXME abort run start?
            }
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field ENABLED_FAST_TRIGGER_DIGITIZING not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    // DC offset (percent of the dynamic range, -50 to 50)
    content = bC->getElementContent ("DC_OFFSET") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          float dc ;
          ststream >> dc ;
          if (tr != -1) 
            {
              //                 digitizerConfiguration_.FTDCoffset[tr] = dc ;
              digitizerConfiguration_.FTDCoffset[tr*2] = (uint32_t)dc ;
              digitizerConfiguration_.FTDCoffset[tr*2+1] = (uint32_t)dc ;
            }
          else 
            {  
              int val = (int) ( (dc+50) * 65535 / 100) ;
              if (ch == -1) for (int i = 0 ; i < MAX_SET ; ++i) digitizerConfiguration_.DCoffset[i] = val ;
              else          digitizerConfiguration_.DCoffset[ch] = val ;
            }
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field DC_OFFSET not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    content = bC->getElementContent ("GRP_CH_DC_OFFSET") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          float dc[8] ;
          for (int i = 0 ; i < 8 ; ++i) ststream >> dc[i] ;
          for (int i = 0 ; i < MAX_SET ; ++i) 
              {
                int val = (int) ( (dc[i]+50) * 65535 / 100) ; 
                digitizerConfiguration_.DCoffsetGrpCh[ch][i] = val ;
              }
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field GRP_CH_DC_OFFSET not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    // Threshold
    content = bC->getElementContent ("TRIGGER_THRESHOLD") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          int val ;
          ststream >> val ;
          if (tr != -1) 
            {
              // digitizerConfiguration_.FTThreshold[tr] = val ;
              digitizerConfiguration_.FTThreshold[tr*2] = val ;
              digitizerConfiguration_.FTThreshold[tr*2+1] = val ;
            }
          else
            {    
              if (ch == -1) for (int i = 0 ; i < MAX_SET ; ++i) digitizerConfiguration_.Threshold[i] = val ;
              else          digitizerConfiguration_.Threshold[ch] = val ;
            }
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field TRIGGER_THRESHOLD not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }
        
   // Group Trigger Enable Mask (hex 8 bit)
    content = bC->getElementContent ("GROUP_TRG_ENABLE_MASK") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          int val ; //PG FIXME is this integer or hexadecimal?
          ststream >> val ;
          if (ch == -1) for (int i = 0 ; i < MAX_SET ; ++i) digitizerConfiguration_.GroupTrgEnableMask[i] = val & 0xFF ;
          else          digitizerConfiguration_.GroupTrgEnableMask[ch] = val & 0xFF ;
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field GROUP_TRG_ENABLE_MASK not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }
 
    // Channel Auto trigger (DISABLED, ACQUISITION_ONLY, ACQUISITION_AND_TRGOUT)
    content = bC->getElementContent ("CHANNEL_TRIGGER") ;
    if (content.find ("NULL") != string::npos)
        {
          int ok = 1 ;
          stringstream ststream (content) ;
          string dummy ;
          ststream >> dummy ;
          CAEN_DGTZ_TriggerMode_t tm ;
          if (dummy == "DISABLED")                    tm = CAEN_DGTZ_TRGMODE_DISABLED ;
          else if (dummy == "ACQUISITION_ONLY")       tm = CAEN_DGTZ_TRGMODE_ACQ_ONLY ;
          else if (dummy == "ACQUISITION_AND_TRGOUT") tm = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT ;
          else
            {  
               ok = 0 ;
               cout << "[V1742]::[WARNING]:: CHANNEL_TRIGGER " << dummy << " is an invalid option" << endl ;
               //PG FIXME abort run start?
            }
          if (ok)  
            {
              if (ch == -1) for (int i = 0; i < MAX_SET; ++i) digitizerConfiguration_.ChannelTriggerMode[i] = tm ;
              else          digitizerConfiguration_.ChannelTriggerMode[ch] = tm ; 
            }
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field CHANNEL_TRIGGER not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    // Front Panel LEMO I/O level (NIM, TTL)
    content = bC->getElementContent ("FPIO_LEVEL") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          string dummy ;
          ststream >> dummy ;
          if (dummy == "TTL") digitizerConfiguration_.FPIOtype = 1 ;
          else if (dummy != "NIM")
            {  
               cout << "[V1742]::[WARNING]:: FPIO_LEVEL " << dummy << " is an invalid option" << endl ;
               //PG FIXME abort run start?
            }
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field FPIO_LEVEL not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

    // Channel Enable (or Group enable for the V1740) (YES/NO)
    content = bC->getElementContent ("ENABLE_INPUT") ;
    if (content.find ("NULL") != string::npos)
        {
          stringstream ststream (content) ;
          string dummy ;
          ststream >> dummy ;
          if (dummy == "YES") 
              {
                if (ch == -1)  digitizerConfiguration_.EnableMask = 0xFF ;
                else digitizerConfiguration_.EnableMask |= (1 << ch) ;
              } 
          else if (dummy == "NO")
              {
                if (ch == -1)  digitizerConfiguration_.EnableMask = 0x00 ;
                else digitizerConfiguration_.EnableMask &= ~ (1 << ch) ;
              } 
          else 
            {  
               cout << "[V1742]::[WARNING]:: ENABLE_INPUT " << dummy << " is an invalid option" << endl ;
               //PG FIXME abort run start?
            }
        }
    else 
      {
        cout << "[V1742]::[WARNING]:: Field ENABLE_INPUT not found in board xml node config" << endl ;
        //PG FIXME abort run start?
      }

  }
  return 0 ;

} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int CAEN_V1742::setDefaults ()
{
//  char str[1000], str1[1000] ;
//  int i,j, ch=-1, val, Off=0, tr = -1 ;

  /* Default settings */
  digitizerConfiguration_.RecordLength = (1024*16) ;
  digitizerConfiguration_.PostTrigger = 80 ;
  digitizerConfiguration_.NumEvents = 1023 ;
  digitizerConfiguration_.EnableMask = 0xFF ;
  digitizerConfiguration_.GWn = 0 ;
  digitizerConfiguration_.ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY ;
  digitizerConfiguration_.InterruptNumEvents = 0 ;
  digitizerConfiguration_.TestPattern = 0 ;
  digitizerConfiguration_.TriggerEdge = 0 ;
  digitizerConfiguration_.DesMode = 0 ;
  digitizerConfiguration_.FastTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED ;
  digitizerConfiguration_.FastTriggerEnabled = 0 ; 
  digitizerConfiguration_.FPIOtype = 0 ;
  /* strcpy (digitizerConfiguration_.GnuPlotPath, GNUPLOT_DEFAULT_PATH) ; */
  for (int i = 0 ; i < MAX_SET ; ++i) 
    {
      digitizerConfiguration_.DCoffset[i] = 0 ;
      digitizerConfiguration_.Threshold[i] = 0 ;
      digitizerConfiguration_.ChannelTriggerMode[i] = CAEN_DGTZ_TRGMODE_DISABLED ;
      digitizerConfiguration_.GroupTrgEnableMask[i] = 0 ;
      for (int j = 0 ; j < MAX_SET ; ++j) digitizerConfiguration_.DCoffsetGrpCh[i][j] = -1 ;
      digitizerConfiguration_.FTThreshold[i] = 0 ;
      digitizerConfiguration_.FTDCoffset[i] = 0 ;
  }

  digitizerConfiguration_.useCorrections = -1 ;
  
  return 0 ;
}


