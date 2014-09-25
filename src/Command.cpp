#include "interface/Command.hpp"

string Command::name()
{
switch (cmd) {
	case WWE  : { return "WWE" ; }
	case WE   : { return "WE"  ; }
	case EE   : { return "EE"  ; }
	case WBE  : { return "WBE" ; }
	case BT   : { return "BT"  ; }
	case WBT  : { return "WBT" ; }
	case EBT  : { return "EBT" ; }
	case SEND : { return "SEND"; }
	case RECV : { return "RECV"; }
	case DATA : { return "DATA"; }
	default:
	case NOP  : { return "NOP"; }
	}
}
