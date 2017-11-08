#include "uhal/uhal.hpp"

#include <vector>
#include <iostream>
#include <cstdlib>
#include <typeinfo>

#include <TApplication.h>
#include <TProfile.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1D.h>
#include <TTree.h>
#include <TFile.h>

#define GENE_TRIGGER 1
#define GENE_CALIB   2
#define GENE_100HZ   4
#define LED_ON       8

#define RESET                  (1<<0)
#define TRIGGER_MODE           (1<<1)
#define SOFT_TRIGGER           (1<<2)
#define SELF_TRIGGER           (1<<3)
#define CLOCK_PHASE            (1<<4)
#define CLOCK_RESET            (1<<7)
#define SELF_TRIGGER_THRESHOLD (1<<8)
#define SELF_TRIGGER_MASK      (1<<22)
#define BOARD_SN               (1<<28)

#define CAPTURE_START 1
#define CAPTURE_STOP  2
//#define SW_DAQ_DELAY 0x1800 // delay for laser with internal trigger
#define SW_DAQ_DELAY 0x0 // delay for laser with external trigger
#define HW_DAQ_DELAY 100 // Laser with external trigger
#define NSAMPLE_MAX 28672
#define MAX_PAYLOAD 1380
#define MAX_VFE     10
#define MAX_STEP    10000

#define DAC_WRITE       (1<<25)
#define ADC_WRITE       (1<<24)
#define DAC_FULL_RESET  (0xF<<16)
#define DAC_SOFT_RESET  (0x7<<16)
#define DAC_VAL_REG     (0x3<<16)
#define DAC_CTRL_REG    (0x4<<16)

#define ADC_OMODE_REG   (0x14<<16) // ADC register to define Output mode of the ADC
#define ADC_ISPAN_REG   (0x18<<16) // ADC register to define input span of the ADC from 1.383V to 2.087V

using namespace uhal;

int main ( int argc,char* argv[] )
{
  int ngood_event=0;
  int soft_reset, full_reset, command;
  ValWord<uint32_t> address;
  ValWord<uint32_t> data[3*NSAMPLE_MAX];
  //ValVector< uint32_t > mem[256];
  ValVector< uint32_t > block,mem;
  short int event[5][NSAMPLE_MAX];
  double fevent[5][NSAMPLE_MAX];
  double dv=1750./16384.; // 14 bits on 1.75V
  int debug=0;
// Define defaults for laser runs :
  int nevent=1000;
  int nsample=100;
  int trigger_type=0;   // pedestal by default
  int soft_trigger=0;   // external trigger by default 
  int self_trigger=0;   // No self trigger 
  int self_trigger_threshold=0;
  int self_trigger_mask=0x1F; // trig on all channels amplitude
  int n_calib=1;
  int calib_step=-128;
  int calib_level=32768;
  int calib_width=256;
  int calib_delay=0;
  int n_vfe=0;
  int vfe[MAX_VFE];
  char cdum;
  int negate_data=1; // Negate the ADC output by default (positive signal from 0 not to disturb people)
  int signed_data=0; // Don't use 2's complement. Use normal binary order (0...16383 full scale)
  int input_span=0; // Use default ADC input span (1.75V) can be tuned from 1.383 V to 2.087 V by steps of 0.022V
                    // this is 2's complemented on 5 bits. So from -16 to +15 -> 0x10..0x1f 0x00 0x01..0x0f -> 16..31 0 1..15
  TProfile *pshape[MAX_STEP][5];

  for(int k=1; k<argc; k++) 
  {    
    int i=strcmp( argv[k], "-dbg");
    if(i == 0)
    {    
      sscanf( argv[++k], "%d", &debug );
      continue;
    }    
    i=strcmp( argv[k], "-nevt");
    if(i == 0)
    {    
      sscanf( argv[++k], "%d", &nevent );
      continue;
    }    
    i=strcmp(argv[k],"-nsample");
    if(i == 0)
    {    
      sscanf( argv[++k], "%d", &nsample );
      continue;
    }    
    i=strcmp(argv[k],"-soft_trigger");
    if(i == 0)
    {    
// soft_trigger
// 0 : Use external trigger (GPIO)
// 1 : Generate trigger from software (1 written in FW register 
      sscanf( argv[++k], "%d", &soft_trigger );
      continue;
    }    
    i=strcmp(argv[k],"-trigger_type");
    if(i == 0)
    {    
// trigger_type
// 0 : pedestal
// 1 : calibration
// 2 : laser
      sscanf( argv[++k], "%d", &trigger_type );
      continue;
    }
    i=strcmp(argv[k],"-self_trigger");
    if(i == 0)
    {    
// self_trigger
// 0 : Don't generate trigger from data themselves
// 1 : Generate trigger if any data > self_trigger_threshold
      sscanf( argv[++k], "%d", &self_trigger );
      continue;
    }
    i=strcmp(argv[k],"-self_trigger_threshold");
    if(i == 0)
    {    
// self_trigger_threshold in ADC counts
      sscanf( argv[++k], "%d", &self_trigger_threshold );
      continue;
    }
    i=strcmp(argv[k],"-self_trigger_mask");
    if(i == 0)
    {    
// channel mask to generate self trigger lsb=ch0 ... msb=ch4
      sscanf( argv[++k], "%d", &self_trigger_mask );
      continue;
    }
    i=strcmp(argv[k],"-vfe");
    if(i == 0)
    {
      sscanf( argv[++k], "%d", &vfe[n_vfe++] );
      continue;
    }    
    i=strcmp(argv[k],"-calib_level");
    if(i == 0)
    {    
// DAC_value 0 ... 65532
      sscanf( argv[++k], "%d", &calib_level );
      continue;
    }    
    i=strcmp(argv[k],"-calib_width");
    if(i == 0)
    {    
// Calib trigger width 0 ... 65532
      sscanf( argv[++k], "%d", &calib_width );
      continue;
    }    
    i=strcmp(argv[k],"-calib_delay");
    if(i == 0)
    {    
// DAQ delay for calib triggers : 0..65532
      sscanf( argv[++k], "%d", &calib_delay );
      continue;
    }    
    i=strcmp(argv[k],"-n_calib");
    if(i == 0)
    {    
// Number of calibration step for linearity study
      sscanf( argv[++k], "%d", &n_calib );
      continue;
    }    
    i=strcmp(argv[k],"-calib_step");
    if(i == 0)
    {    
// DAC step for linearity study
      sscanf( argv[++k], "%d", &calib_step );
      continue;
    }    
    i=strcmp(argv[k],"-negate_data");
    if(i == 0)
    {    
// Put ADC in 2's complement
      sscanf( argv[++k], "%d", &negate_data );
      continue;
    }    
    i=strcmp(argv[k],"-signed_data");
    if(i == 0)
    {    
// Put ADC in 2's complement
      sscanf( argv[++k], "%d", &signed_data );
      continue;
    }    
    i=strcmp(argv[k],"-input_span");
    if(i == 0)
    {    
// Set ADC input SPAN from 0x1f - 0 - 0x0f
      sscanf( argv[++k], "%d", &input_span );
      continue;
    }    
    i=strcmp(argv[k],"-h");
    if(i == 0)
    {
      printf("Start DAQ with : \n");
      printf("-dpg dbg_level            : Set debug level for this session [0]\n");
      printf("-vfe n                    : Do DAQ on VFE n [1000]\n");
      printf("-nevt n                   : Number of events to record  [1000]\n");
      printf("-nsample n                : Number of sample per event (max=28670) [150]\n");
      printf("-trigger type n           : 0=pedestal, 1=calibration, 2=laser [0]\n");
      printf("-soft trigger n           : 0=externally triggered DAQ, 1=softwared triggered DAQ [0]\n");
      printf("-self trigger n           : 1=internal generated trigger if signal > threshold [0]\n");
      printf("-self_trigger_threshold n : minimal signal amplitude to generate self trigger [0]\n");
      printf("-self_trigger_mask n      : channel mask to generate self triggers, ch0=lsb    [0x1F]\n");
      printf("-calib_width n            : width of the calibration trigger sent to VFE [128]\n");
      printf("-calib_delay n            : delay between calibration trigger and DAQ start [0]\n");
      printf("-calib_level n            : DAC level to start linearity stud [32768]\n");
      printf("-n_calib n                : number of calibration steps for linearity study [1]\n");
      printf("-calib_step n             : DAC step for linearity study [128]\n");
      printf("-negate_data 0/1          : Negate (1) or not (0) the converted data in the ADC [0]\n");
      printf("-signed_data 0/1          : Set ADC in normal binary mode (0) or 2's complement (1) [0]\n");
      printf("-input_span n             : Set ADC input span (0x10 = 1.383V, 0x1f=1.727V, 0=1.75V, 0x01=1.772V, 0x0f=2.087V) [0]\n");
      exit(-1);
    }
  }
  if(n_vfe==0)
  {
    n_vfe=1;
    vfe[0]=4;
  }

  //if(trigger_type==0 || trigger_type==1)
  //{
  //  soft_trigger=1;
  //  self_trigger=0;
  //}
  //else if(trigger_type==2)
  //{
  //  soft_trigger=0;
  //  self_trigger=1;
  //  self_trigger_threshold=14000;
  //}
  printf("Start DAQ with %d cards : ",n_vfe);
  for(int i=0; i<n_vfe; i++)printf("%d, ",vfe[i]);
  printf("\n");
  printf("Parameters : \n");
  printf("  %d events \n",nevent);
  printf("  %d samples \n",nsample);
  printf("  trigger type  : %d (0=pedestal, 1=calibration, 2=laser)\n",trigger_type);
  printf("  soft trigger  : %d (0=externally triggered DAQ, 1=softwared triggered DAQ)\n",soft_trigger);
  printf("  self trigger  : %d (1=internal generated trigger if signal > threshold)\n",self_trigger);
  printf("  threshold     : %d (minimal signal amplitude to generate self trigger)\n",self_trigger_threshold);
  printf("  mask          : 0x%x (channel mask to generate self triggers)\n",self_trigger_mask);
  printf("  calib_width   : %d (width of the calibration trigger sent to VFE)\n",calib_width);
  printf("  calib_delay   : %d (delay between calibration trigger and DAQ start)\n",calib_delay);
  printf("  n_calib       : %d (number of calibration steps for linearity study)\n",n_calib);
  printf("  calib_step    : %d (DAC step for linearity study)\n",calib_step);
  if(n_calib<=0)n_calib=1;

  TApplication *Root_App=new TApplication("test", &argc, argv);
  TCanvas *c1=new TCanvas("c1","c1",800.,1000.);
  c1->Divide(2,3);
  TGraph *tg[5];
  TH1D *hmean[5], *hrms[5];
  double rms[5];
  char hname[8];
  for(int ich=0; ich<5; ich++)
  {
    tg[ich] = new TGraph();
    tg[ich]->SetMarkerStyle(20);
    tg[ich]->SetMarkerSize(0.5);
    sprintf(hname,"mean_ch%d",ich);
    hmean[ich]=new TH1D(hname,hname,100,150.,250.);
    sprintf(hname,"rms_ch%d",ich);
    hrms[ich]=new TH1D(hname,hname,200,0.,2.);
  }
  tg[0]->SetLineColor(kRed);
  tg[1]->SetLineColor(kYellow);
  tg[2]->SetLineColor(kBlue);
  tg[3]->SetLineColor(kMagenta);
  tg[4]->SetLineColor(kCyan);
  tg[0]->SetMarkerColor(kRed);
  tg[1]->SetMarkerColor(kYellow);
  tg[2]->SetMarkerColor(kBlue);
  tg[3]->SetMarkerColor(kMagenta);
  tg[4]->SetMarkerColor(kCyan);
  c1->Update();
  int dac_val=calib_level;

  for(int istep=0; istep<n_calib; istep++)
  {
    for(int ich=0; ich<5*n_vfe; ich++)
    {
      char pname[132];
      sprintf(pname,"ch_%d_step_%d_%d",ich,istep,dac_val);
      pshape[istep][ich]=new TProfile(pname,pname,nsample,0.,6.25*nsample);
    }
    dac_val+=calib_step;
  }

  ConnectionManager manager ( "file:///data/cms/ecal/fe/vice/connection_file.xml" );
  std::vector<uhal::HwInterface> devices;
  int i_vfe;
  for(i_vfe=0; i_vfe<n_vfe; i_vfe++)
  {
    char vice_str[80];
    sprintf(vice_str,"vice.udp.%d",vfe[i_vfe]);
    devices.push_back(manager.getDevice( vice_str ));
  }

  ValWord<uint32_t> free_mem;
  ValWord<uint32_t> trig_reg;
  ValWord<uint32_t> delays;
  int old_address[MAX_VFE];

// Reset board
//  for(int i=0; i<99999999999; i++)
//  {
  //while(1)
  //{
  for(auto & hw : devices)
  {
    hw.getNode("VICE_CTRL").write(RESET*1);
    //hw.getNode("VICE_CTRL").write(RESET*0);
    hw.dispatch();
  }
  usleep(500);
  //usleep(50);
  //}
  
// Try to program DAC
// First : Put DAC in powerup state
  full_reset=DAC_WRITE | (0xf<<16);
  soft_reset=DAC_WRITE | (0x7<<16);
// Set the control register
  command=DAC_WRITE | DAC_CTRL_REG |
          (0x1<<9) | // midscale upon clear*
          (0x0<<8) | // No overrange
          (0x0<<7) | // No bipolar range
          (0x0<<6) | // No thermal shutdown
          (0x1<<5) | // Use internal reference voltage
          (0x1<<3) | // midscale at power-up
          (0x3<<0);  // 0-5V range
  printf("Ctrl register loading command : 0x%x\n",command);
  for(auto & hw : devices)
  {
    printf("Do a full reset\n");
    hw.getNode("VFE_CTRL").write(full_reset);
    hw.dispatch();

    printf("Write in conrol reg\n");
    hw.getNode("VFE_CTRL").write(command); // First write to reconfigure output range
    hw.dispatch();

    printf("Do a full reset\n");
    hw.getNode("VFE_CTRL").write(full_reset); // Full reset after output range reconfig
    hw.dispatch();

    printf("Write in control reg\n");
    hw.getNode("VFE_CTRL").write(command); // Rewrite default values and switch on DAC output stage
    hw.dispatch();

    printf("do a soft reset\n");
    hw.getNode("VFE_CTRL").write(soft_reset); // soft reset
    hw.dispatch();

    command=DAC_WRITE | DAC_VAL_REG | (calib_level&0xffff);
    printf("Put %d in DAC register : 0x%x\n",calib_level,command);
    hw.getNode("VFE_CTRL").write(command);
    hw.dispatch();

// Put ADC in two's complement mode (if no pedestal bias) and invert de conversion result
    negate_data=(negate_data&1)<<2;
    signed_data&=3;
    command=ADC_WRITE | ADC_OMODE_REG | negate_data | signed_data;
    printf("Put ADC coding : 0x%x\n",command);
    hw.getNode("VFE_CTRL").write(command);
    hw.dispatch();

// Set ADC input range (default=1.75V)
    input_span&=0x1F;
    command=ADC_WRITE | ADC_ISPAN_REG | input_span;
    printf("Set ADC input span range : 0x%x\n",command);
    hw.getNode("VFE_CTRL").write(command);
    hw.dispatch();
  }

// Play with DAC values :
/*
  for(unsigned short int dac_val=0; dac_val<65535; dac_val+=128)
  {
    for(auto & hw : devices)
    {
      command=DAC_WRITE | DAC_VAL_REG | (dac_val&0xFFFF);
      printf("Write %u into DAC register : 0x%x\n",dac_val,command);
      hw.getNode("VFE_CTRL").write(command);
      hw.dispatch();
    }
    usleep(100000);
  }
*/

// Calibration trigger setting :
  for(auto & hw : devices)
  {
    command=(calib_delay<<16) | (calib_width&0xffff);
    printf("Calibration trigger with %d clocks width and %d clocks delay : %x\n",calib_width,calib_delay,command);
    hw.getNode("CALIB_CTRL").write(command);
    hw.dispatch();
  }

// Init stage :
  i_vfe=0;
  for(auto & hw : devices)
  {
  // Read FW version to check :
    ValWord<uint32_t> reg = hw.getNode("FW_VER").read();
  // Switch to triggered mode + external trigger :
    command = (SELF_TRIGGER_MASK     *(self_trigger_mask&0x1F))        |
              (SELF_TRIGGER_THRESHOLD*(self_trigger_threshold&0x3FFF)) |
               SELF_TRIGGER          *self_trigger                     |
               SOFT_TRIGGER          *soft_trigger                     |
               TRIGGER_MODE          *1                                | // Always DAQ on trigger
               RESET                 *0;
    hw.getNode("VICE_CTRL").write(command);
  // Stop DAQ and ask for NSAMPLE per frame (+timestamp) :
    command = ((nsample+1)<<16)+CAPTURE_STOP;
    hw.getNode("CAP_CTRL").write(command);
  // Add laser latency before catching data ~ 40 us
    hw.getNode("TRIG_DELAY").write((SW_DAQ_DELAY<<16)+HW_DAQ_DELAY);
  // Switch off FE-adapter LEDs
    command = LED_ON*0+GENE_100HZ*0+GENE_CALIB*0+GENE_TRIGGER*0;
    hw.getNode("FW_VER").write(command);
    hw.dispatch();

  // Reset the reading base address :
    hw.getNode("CAP_ADDRESS").write(0);
  // Start DAQ :
    command = ((nsample+1)<<16)+CAPTURE_START;
    hw.getNode("CAP_CTRL").write(command);
  // Read back delay values :
    delays=hw.getNode("TRIG_DELAY").read();
  // Read back the read/write base address
    address = hw.getNode("CAP_ADDRESS").read();
    free_mem = hw.getNode("CAP_FREE").read();
    trig_reg = hw.getNode("VICE_CTRL").read();
    hw.dispatch();

    printf("Firmware version      : %8.8x\n",reg.value());
    printf("Delays                : %8.8x\n",delays.value());
    printf("Initial R/W addresses : 0x%8.8x\n", address.value());
    printf("Free memory           : 0x%8.8x\n", free_mem.value());
    printf("Trigger mode          : 0x%8.8x\n", trig_reg.value());
    old_address[i_vfe]=address>>16;
    if(old_address[i_vfe]==0x6fff)old_address[i_vfe]=-1;
    i_vfe++;
  }

  int n_word=(nsample+1)*3; // 3*32 bits words per sample to get the 5 channels adta
  int n_transfer=n_word/(MAX_PAYLOAD/4); // Max ethernet packet = 1536 bytes, max user payload = 1500 bytes
  int n_last=n_word-n_transfer*(MAX_PAYLOAD/4);
  printf("Reading events by blocks of %dx32b-words, %d bits\n",n_word, n_word*32);
  printf("Using %d transfers of %d words + 1 transfer of %d words\n",n_transfer, MAX_PAYLOAD/4,n_last);
  if(n_transfer > 248)
  {
    printf("Event size too big ! Please reduce number of samples per frame.\n");
    printf("Max frame size : 28670\n");
  }

  unsigned long int timestamp=0;
  TTree *tdata=new TTree("data","data");
  tdata->Branch("timestamp",&timestamp,"timestamp/l");
  for(int ich=0; ich<5; ich++)
  {
    char bname[80], btype[80];
    sprintf(bname,"ch%d",ich);
    sprintf(btype,"ch%d[%d]/S",ich,nsample);
    tdata->Branch(bname,event[ich],btype);
  }

// Send triggers and wait between each trigger :
  for(int istep=0; istep<n_calib; istep++)
  {
// Program DAC for this step
    for(auto & hw : devices)
    {
      command=DAC_WRITE | DAC_VAL_REG | (calib_level&0xffff);
      printf("Put %d in DAC register : 0x%x\n",calib_level,command);
      hw.getNode("VFE_CTRL").write(command);
      hw.dispatch();
    }
    int ievt=0;
    while(ievt<nevent)
    {
      i_vfe=0;
      for(auto & hw : devices)
      {
        if(debug>0)
        {
          command = ((nsample+1)<<16)+CAPTURE_START;
          hw.getNode("CAP_CTRL").write(command);
          hw.dispatch();
        }
        if(soft_trigger == 1)
        {
          if(trigger_type == 0)
            command = LED_ON*0+GENE_100HZ*0+GENE_CALIB*0+GENE_TRIGGER*1;
          else if(trigger_type == 1)
            command = LED_ON*0+GENE_100HZ*0+GENE_CALIB*1+GENE_TRIGGER*0;

          free_mem = hw.getNode("CAP_FREE").read();
          hw.getNode("FW_VER").write(command);
          hw.dispatch();
          address = hw.getNode("CAP_ADDRESS").read();
          hw.dispatch();
          int new_address=address.value()>>16;
          int loc_address=new_address;
          int nretry=0;
          while(((loc_address-old_address[i_vfe])%28672) != nsample+1 && nretry<100)
          {
            address = hw.getNode("CAP_ADDRESS").read();
            hw.dispatch();
            new_address=address.value()>>16;
            loc_address=new_address;
            if(new_address<old_address[i_vfe])loc_address+=28672;
            nretry++;
            printf("ongoing R/W addresses    : old %d, new %d add 0x%8.8x\n", old_address[i_vfe], new_address,address.value());
          }
          if(nretry==100)
          {
            printf("Stop waiting for sample capture after %d retries\n",nretry);
            printf("R/W addresses    : old %8.8x, new %8.8x add 0x%8.8x\n", old_address[i_vfe], new_address, address.value());
          }
          old_address[i_vfe]=new_address;
          if(old_address[i_vfe]==0x6fff)old_address[i_vfe]=-1;
          //printf("R/W addresses    : old %8.8x, new %8.8x add 0x%8.8x\n", old_address[i_vfe], new_address, address.value());
        }
        else
        {
          free_mem = hw.getNode("CAP_FREE").read();
          address = hw.getNode("CAP_ADDRESS").read();
          hw.dispatch();
          printf("address : 0x%8.8x, Free memory : %d\n",address.value(),free_mem.value());
          if(debug>0)printf("address : 0x%8.8x, Free memory : %d\n",address.value(),free_mem.value());
          if(free_mem.value()==28671)continue;
        }
        ievt++;

// Read event samples from FPGA
        mem.clear();
        for(int itrans=0; itrans<n_transfer; itrans++)
        {
          block = hw.getNode ("CAP_DATA").readBlock(MAX_PAYLOAD/4);
          hw.dispatch();
          for(int is=0; is<MAX_PAYLOAD/4; is++)mem.push_back(block[is]);
        }
        block = hw.getNode ("CAP_DATA").readBlock(n_last);
        address = hw.getNode("CAP_ADDRESS").read();
        free_mem = hw.getNode("CAP_FREE").read();
        hw.dispatch();
        if(debug>0)printf("After reading address : 0x%8.8x, Free memory : %d\n",address.value(),free_mem.value());
        for(int is=0; is<n_last; is++)mem.push_back(block[is]);
        mem.valid(true);

        double ped[5], ave[5], rms[5];
        for(int ich=0; ich<5; ich++)
        {
          ped[ich]=0.;
          ave[ich]=0.;
          rms[ich]=0.;
        }
        double max=0.;
    // First sample should have bit 70 at 1
        if((mem[0]>>31) != 1) printf("Sample 0 not a header : %8.8x\n",mem[0]);
        unsigned long int t1= mem[0]     &0xFFFF;
        unsigned long int t2= mem[1]     &0xFFFF;
        unsigned long int t3=(mem[1]>>16)&0xFFFF;
        unsigned long int t4= mem[2]     &0xFFFF;
        unsigned long int t5=(mem[2]>>16)&0x00FF;
        timestamp=(t5<<56)+(t4<<42)+(t3<<28)+(t2<<14)+t1;
        
        if(debug>0)
        {
          printf("timestamp : %8.8x %8.8x %8.8x\n",mem[2],mem[1],mem[0]);
          printf("timestamp : %ld %4.4lx %4.4lx %4.4lx %4.4lx %4.4lx\n",timestamp,t5,t4,t3,t2,t1);
        }
        for(int isample=0; isample<nsample; isample++)
        {
          int j=(isample+1)*3;
// If data are coded in 2's complement, propagate sign bit (b13) to bit 14 and 15 of short int array 
// to keep signed data
          event[0][isample]= mem[j]       &0xFFFF;
          if(signed_data==1 && ((event[0][isample]>>13)&1)==1)event[0][isample]|=0xc000;
          event[1][isample]= mem[j+1]     &0xFFFF;
          if(signed_data==1 && ((event[1][isample]>>13)&1)==1)event[1][isample]|=0xc000;
          event[2][isample]=(mem[j+1]>>16)&0xFFFF;
          if(signed_data==1 && ((event[2][isample]>>13)&1)==1)event[2][isample]|=0xc000;
          event[3][isample]= mem[j+2]     &0xFFFF;
          if(signed_data==1 && ((event[3][isample]>>13)&1)==1)event[3][isample]|=0xc000;
          event[4][isample]=(mem[j+2]>>16)&0xFFFF;
          if(signed_data==1 && ((event[4][isample]>>13)&1)==1)event[4][isample]|=0xc000;
          if(signed_data==1)
          {
            //event[0][isample]-=8192;
            //event[1][isample]-=8192;
            //event[2][isample]-=8192;
            //event[3][isample]-=8192;
            //event[4][isample]-=8192;
          }
          //if(debug>0)printf("%8.8x %8.8x %8.8x\n",mem[j],mem[j+1],mem[j+2]);
// With Catia, signal is negative, based at ADC positive rail
// Converted to user friendly data in FPGA.
          fevent[0][isample]=double(event[0][isample]);
          fevent[1][isample]=(double)event[1][isample];
          fevent[2][isample]=(double)event[2][isample];
          fevent[3][isample]=(double)event[3][isample];
          fevent[4][isample]=(double)event[4][isample];

          if(dv*event[3][isample]>max)max=dv*event[3][isample];
        }

        for(int ich=0; ich<5; ich++)
        {
          for(int isample=0; isample<nsample; isample++)
          {
            //tg[ich]->SetPoint(isample,6.125*isample,dv*event[ich][isample]);
            tg[ich]->SetPoint(isample,6.125*isample,fevent[ich][isample]);
            ave[ich]+=dv*fevent[ich][isample];
            if(isample<30)ped[ich]+=dv*fevent[ich][isample];
            rms[ich]+=dv*fevent[ich][isample]*dv*fevent[ich][isample];
            pshape[istep][i_vfe*5+ich]->Fill(6.125*isample+1.,dv*fevent[ich][isample]);
          }
        }
        for(int ich=0; ich<5; ich++)
        {
          ave[ich]/=nsample;
          ped[ich]/=30.;
          rms[ich]/=nsample;
          rms[ich]=sqrt(rms[ich]-ave[ich]*ave[ich]);
          if(debug>0)printf("ich %d : ped=%f, ave=%f, rms=%f\n",ich,ped[ich],ave[ich],rms[ich]);
          hmean[ich]->Fill(ave[ich]);
          hrms[ich]->Fill(rms[ich]);
        }
        if(trigger_type==0 || max>0.)
        {
          tdata->Fill();
          if((ngood_event%200)==0)printf("%d events recorded\n",ngood_event);
          ngood_event++;
        }
        //if(debug>0 && max>0.)
        if(debug>0)
        {
          c1->cd(1);
          tg[0]->Draw("alp");
          c1->cd(2);
          tg[1]->Draw("alp");
          c1->cd(3);
          tg[2]->Draw("alp");
          c1->cd(4);
          tg[3]->Draw("alp");
          c1->cd(5);
          tg[4]->Draw("alp");
          c1->Update();
          system("stty raw");
          if(debug==1)cdum=getchar();
          if(debug==2)usleep(1000000);
          system("stty -raw");
          if(cdum=='q' || cdum=='Q') break;
        }
        if(debug>0)
        {
          command = ((nsample+1)<<16)+CAPTURE_STOP;
          hw.getNode("CAP_CTRL").write(command);
          hw.dispatch();
          old_address[i_vfe]=-1;
        }
        i_vfe++;
      }
      if(cdum=='q' || cdum=='Q') break;
    }
    calib_level+=calib_step;
  }

  for(auto & hw : devices)
  {
// Stop DAQ :
    command = ((nsample+1)<<16)+CAPTURE_STOP;
    hw.getNode("CAP_CTRL").write(command);
  // Switch on FE-adapter LEDs
    command = LED_ON*1+GENE_100HZ*0+GENE_TRIGGER*0;
    hw.getNode("FW_VER").write(command);
    hw.dispatch();
  }

  TCanvas *c2=new TCanvas("mean","mean",800.,1000.);
  c2->Divide(2,3);
  c2->Update();
  TCanvas *c3=new TCanvas("rms","rms",800.,1000.);
  c3->Divide(2,3);
  c3->Update();
  printf("RMS : ");
  for(int ich=0; ich<5; ich++)
  {
    c2->cd(ich+1);
    hmean[ich]->Draw();
    c2->Update();
    c3->cd(ich+1);
    hrms[ich]->Draw();
    rms[ich]=hrms[ich]->GetMean();
    printf("%e, ",rms[ich]);
    c3->Update();
  }
  printf("\n");
  TFile *fd=new TFile("/data/cms/ecal/fe/vfe_ADC160_asic_ETH/test/vice_data.tmp.root","recreate");
  tdata->Write();
  c1->Write();
  c2->Write();
  c3->Write();
  for(int istep=0; istep<n_calib; istep++)
  {
    for(i_vfe=0; i_vfe<n_vfe; i_vfe++)
    {
      for(int ich=0; ich<5; ich++)
      {
        pshape[istep][i_vfe*5+ich]->Write();
      }
    }
  }
  fd->Close();
  printf("Finished with %d events recorded\n",ngood_event);

}
