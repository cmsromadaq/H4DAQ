#include "StandardIncludes.hpp"

int main(int argc, char** argv)
{

  int BHandle(0);

  // for (i = 1; i < argc; i++) {
  //   /* Check for a switch (leading "-"). */
  //   if (argv[i][0] == '-') {
  //     /* Use the next character to decide what to do. */
  //     switch (argv[i][1]) {
  //     case 'n': n_value = atoi(argv[++i]);
  // 	break;
  //     case 'p':p_value = atoi(argv[++i]);
  // 	break;
  //     case 'f':f_value = argv[++i];
  // 	break;
  //     case 'd':d_value = atoi(argv[++i]);
  // 	break;
  //     case 'b': beam_trigger = true;
  // 	break;
  //     case 'l': led_trigger = true;
  // 	break;
  //     case 'r': pedestal_freq = atof(argv[++i]);
  // 	break;
  //     }
  //   }
  // }


  int status;
  CVBoardTypes   VMEBoard;
  short          Link, Device;

  VMEBoard = cvV2718;
  Device = 0;
  Link = 0;
  
  status = CAENVME_Init(VMEBoard, Link, Device, &BHandle);
  if(  status ) 
    {
      printf("\n\n Error opening VME BRIDGE %d %d %d\n",VMEBoard,Link,Device);
      return 1;
    }
  else
  {
    printf("VME Crate Initialized \n");
  }

  CAENVME_SystemReset(BHandle);
  sleep(1);
  printf("VME crate has been reset\n");
  
  return 0;

}
