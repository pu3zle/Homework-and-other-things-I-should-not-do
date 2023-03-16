#include "globals.h"
#include "Proc_Info.h"

extern GLOBALS gData;

BOOL
InitGlobals()
{
	TpPreInitThreadPool(&gData.ThreadPool);
	LoadNtdllFunctions();
}