#include "StandardIncludes.hpp"

int main(int argc, char** argv)
{

  int BHandle(0);
  int status(0);
  CVBoardTypes   VMEBoard;
  short          Link(0), Device(0);

  VMEBoard = cvV2718;

  for (int i = 1; i < argc; i++) {
    /* Check for a switch (leading "-"). */
    if (argv[i][0] == '-') {
      /* Use the next character to decide what to do. */
      switch (argv[i][1]) {
      case 't': VMEBoard = (CVBoardTypes) atoi(argv[++i]);
  	break;
      case 'l': Link = atoi(argv[++i]);
  	break;
      case 'd': Device = atoi(argv[++i]);
  	break;
      }
    }
  }

  printf("Opening VME Bridge %d %d %d\n",VMEBoard,Link,Device);

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

  status = CAENVME_SystemReset(BHandle);
  sleep(1);
  if(  status ) 
    {
      printf("\n\n Error opening VME BRIDGE %d %d %d\n",VMEBoard,Link,Device);
      return 1;
    }

  printf("VME Crate Initialized \n");

  
  return 0;

}
