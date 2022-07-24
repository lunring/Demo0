
#include "CoreHal.h"
#include "SegLedGui.h"
#include "DeviceDriveTask.h"
//#include "WdtHal.h"
#include "Fm33Lc0xxWdtHal.h"

void main (void)
{
  x_SetGlobalInt(false);
  x_InitDeviceDriveTask();
  x_ConfigWdt(IWDG_OVERFLOW_PERIOD_4000MS, NULL, NULL);    //看门狗溢出时间4S
  x_SetGlobalInt(true);
  while(true)
  {
    x_RstWdt();          //复位看门狗
    
    x_AppTask();
  }
}
