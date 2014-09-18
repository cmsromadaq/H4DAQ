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

  ParseConfiguration();

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

int CAEN_V1742::Clear(){};
int CAEN_V1742::BufferClear(){};
int CAEN_V1742::Config(BoardConfig *bC)
{
  Board::Config(bC);
};
int CAEN_V1742::Read(vector<WORD> &v){};
int CAEN_V1742::SetHandle(vector<WORD> &v){};
int CAEN_V1742::getMoreBoardInfo(){};
int CAEN_V1742::programDigitizer(){};
int CAEN_V1742::writeEventToOutputBuffer(vector<WORD>& CAEN_V1742_eventBuffer, CAEN_DGTZ_EventInfo_t *EventInfo, CAEN_DGTZ_X742_EVENT_t *Event){};
int CAEN_V1742::ParseConfiguration(){};
