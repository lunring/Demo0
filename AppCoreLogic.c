
/******************************************************************************************
@copyright 2017-2018, 河南汉威电子股份有限公司
@file      AppCoreLogic.h
@version   V1.0
@brief     Mini探测器应用程序,应用程序核心逻辑
@warning:   
@remarks:  
 1.version V1.0    author 张晓男   date 2018.07.09
   modification 建立文档
*******************************************************************************************/

#include "AppCoreLogic.h"
#include "DeviceDriveTask.h"
#include "AppPrmtStorage.h"
#include "CoreHal.h"
#include "UploadCommTask.h"

/*
文件引用
====================================================================================================
宏定义
*/

#define SNSR_WARMUP_FULL_TIME_S    (180U)  //完整预热时间
#define SNSR_WARMUP_PROTECT_TIME_S (30U)  //不可跳过基本预热时间
#define SYS_CHECKING_TIME_S        (1U)    //系统自检时间

//定制添加
u8 u8L_Alarm_flag=0u;
u8 u8L_Error_flag=0u;
u8 u8L_Loss_flag=0u;
u8 u8L_Period_flag=0u;
u8 u8L_Heart_flag=0u;
/*
宏定义
====================================================================================================
变量定义
*/

static CoreStateGroupDef dL_StateGroup = 
{
  .u8_CheckingCd          = 1U,
  .u8_WarmupCd            = SNSR_WARMUP_FULL_TIME_S,
  .d_Fault.u8_All         = 0U,
  .d_Alarm.u8_All         = 0U,
  .d_Abnormal.u8_All      = 0U,
  .u32_GasAd              = 0U,
  .f32_Cctrt              = 0U,
  .u8_DectorDispPrmtState = 0U,
};

/*
变量定义
====================================================================================================
内部函数
*/


/******************************************************************************************   
@brief   执行核心接受的外部命令,并返回执行是否正确. _True正确, _False错误, _Unknown未知命令
*******************************************************************************************/
static tn n_ExecCoreAcceptCmd(const CoreAcceptCmdDef* pcdI_Cmd)
{
  GasSnsrCmdDef    eT_SnsrCmd;
  tn               tnT_Rslt    = _False;
  AppPrmtDef*      pdT_AppPrmt = x_GetAppPrmt();
  
  switch (pcdI_Cmd->e_Cmd)
  {
  case _CoreCmdSetAlarmL:
    {
      tnT_Rslt = (tn)x_SnsrCmdSupport(_GasSnsrCmdAlarmLo, pcdI_Cmd->f32_Prmt);
    }break;
  case _CoreCmdSetAlarmH:
    {
      tnT_Rslt = (tn)x_SnsrCmdSupport(_GasSnsrCmdAlarmHi, pcdI_Cmd->f32_Prmt);
    }break;

  case _CoreCmdZeroMove:
    {
      tnT_Rslt = (tn)x_SnsrCmdSupport(_GasSnsrCmdZeroMove, pcdI_Cmd->f32_Prmt);
    }break;
    
  case _CoreCmdCalSingle:
    {
      tnT_Rslt = (tn)x_SnsrCmdSupport(_GasSnsrCmdSgCctrt, pcdI_Cmd->f32_Prmt);
    }break;
    
  case _CoreCmdSetAddr: //设置地址
    {
      tnT_Rslt                  = _True;
      pdT_AppPrmt->u16_CommAddr = pcdI_Cmd->u16_Prmt; //修改并存储
      x_SaveAppPrmt((u8*)&pdT_AppPrmt->u16_CommAddr, sizeof(pdT_AppPrmt->u16_CommAddr));
    }break;
    
  case _CoreCmdStopWarmup:
    {
      if (dL_StateGroup.u8_WarmupCd < (SNSR_WARMUP_FULL_TIME_S - SNSR_WARMUP_PROTECT_TIME_S)) //预热前20秒不允许跳过
      {
        tnT_Rslt = _True;
        dL_StateGroup.u8_WarmupCd = 0;
      }
    }break;
     
  default:
    {
      if (((u8)pcdI_Cmd->e_Cmd >= (u8)_CoreCmdCalCctrt0)&&((u8)pcdI_Cmd->e_Cmd <= (u8)_CoreCmdCalCctrt5)) //浓度标定
      {
        eT_SnsrCmd = (GasSnsrCmdDef)((u8)pcdI_Cmd->e_Cmd - (u8)_CoreCmdCalCctrt0);
        tnT_Rslt   = (tn)x_SnsrCmdSupport(eT_SnsrCmd, pcdI_Cmd->f32_Prmt);
      }
      else
      {
        tnT_Rslt = _Unknown;
      }
    }break;
  }
 
  return tnT_Rslt;
}

/******************************************************************************************   
@brief   更新内核状态
*******************************************************************************************/
static void n_RefreshCoreStateGroup(const GasSnsrOutputDef* pcdI_SnsrOutput)
{
  static bl                 blS_Hi     = false;
  static bl                 blS_Lo     = false;
  bl                         blT_Hi     = false;
  bl                         blT_Lo     = false;
  static bl                 blS_Err     = false;
  bl                         blT_Err     = false;  
  
  static bl blS_ValveOpen       = false;
  static bl blS_ValveClose      = false;
  static bl blS_ValveFaultLost  = false;
  static bl blS_ValveFaultShort = false;
  
  bl         blT_ValveOpen       = false;
  bl         blT_ValveClose      = false;
  bl         blT_ValveFaultLost  = false;
  bl         blT_ValveFaultShort = false;
 
  if ((dL_StateGroup.u8_CheckingCd != 0) || (dL_StateGroup.u8_WarmupCd != 0U))    //自检中和预热状态不更新任何状态(除 s_Prmt)
  {
    return;
  }
  
  if(pcdI_SnsrOutput->d_State.s_Fault == 0U) //最新检查没有传感器故障
  {
    if (dL_StateGroup.d_Fault.s_Snsr != 0U)    /*原状态有传感器故障且新检查没有故障,表示故障解除*/
    {
       x_SnsrCmdSupport(_GasSnsrCmdWarmupSet,SNSR_WARMUP_PROTECT_TIME_S);   /*故障解除时,重置预热倒计时*/
    }
    else  /*没有传感器故障,更新传感器相关状态,否则和传感器相关的各个状态保持故障前的状态*/
    {
      dL_StateGroup.d_Alarm.s_Lo    = pcdI_SnsrOutput->d_State.s_AlarmLo;
      dL_StateGroup.d_Alarm.s_Hi    = pcdI_SnsrOutput->d_State.s_AlarmHi;
      dL_StateGroup.d_Abnormal.s_Hp = pcdI_SnsrOutput->d_State.s_HiPrtct;
      dL_StateGroup.d_Abnormal.s_Ol = pcdI_SnsrOutput->d_State.s_OverLoad;
      dL_StateGroup.f32_Cctrt       = pcdI_SnsrOutput->f32_Cctrt;
    }
  }
    dL_StateGroup.d_Fault.s_Snsr = pcdI_SnsrOutput->d_State.s_Fault;       /*更新传感器故障状态*/
    dL_StateGroup.u32_GasAd      = pcdI_SnsrOutput->u32_SnsrAd;            /*AD值不受预热状态和故障的影响*/
    dL_StateGroup.d_Fault.s_Cal  = pcdI_SnsrOutput->d_State.s_MulError;    /*标定数据错误*/
    
  /////////////////////////报警出发上传////////////////////////////////////////////////////////
  if(dL_StateGroup.d_Alarm.s_Lo != 0U)   //发生低报
    {
       blT_Lo = true;
    }
    
    if(dL_StateGroup.d_Alarm.s_Hi != 0U)   //发生高报
    {
      blT_Hi = true;
    }
  
    if(dL_StateGroup.d_Fault.u8_All != 0U)   //发生故障
    {
       blT_Err = true;
    }
  
   if (blS_Err != blT_Err)
    {
      dL_StateGroup.d_Upload.Normal_Flag = 1U;
      u8L_Error_flag = 1u;
      blS_Err = blT_Err;
    }
  
    if (blS_Lo != blT_Lo)
    {
      dL_StateGroup.d_Upload.Normal_Flag = 1U;
      u8L_Alarm_flag = 1u;
      blS_Lo = blT_Lo;
    }
    
    if(blS_Hi != blT_Hi)
    {
      dL_StateGroup.d_Upload.Normal_Flag = 1U;
      u8L_Alarm_flag = 1u;
      blS_Hi = blT_Hi;
    }
  
    if(_ValveOpen == dL_StateGroup.d_Valve)
    {
      blT_ValveOpen         = true;
    }
    if(_ValveClose == dL_StateGroup.d_Valve)
    {
      blT_ValveClose         = true;
    }
    if(_ValveFaultLost == dL_StateGroup.d_Valve)
    {
      blT_ValveFaultLost    = true;
    }
    if(_ValveFaultShort == dL_StateGroup.d_Valve)
    {
      blT_ValveFaultShort   = true;
    }
  
    if(blS_ValveOpen != blT_ValveOpen)
    {
      dL_StateGroup.d_Upload.PulseValve_Flag = 1U;
      blS_ValveOpen = blT_ValveOpen;
    }
    if(blS_ValveClose != blT_ValveClose)
    {
      dL_StateGroup.d_Upload.PulseValve_Flag = 1U;
      blS_ValveClose = blT_ValveClose;
    }
    if(blS_ValveFaultLost != blT_ValveFaultLost)
    {
      dL_StateGroup.d_Upload.PulseValve_Flag = 1U;
      blS_ValveFaultLost = blT_ValveFaultLost;
    }
    if(blS_ValveFaultShort != blT_ValveFaultShort)
    {
      dL_StateGroup.d_Upload.PulseValve_Flag = 1U;
      blS_ValveFaultShort = blT_ValveFaultShort;
    }
}

/******************************************************************************************   
@brief   更新定时输出: 核心定时状态, 4-20mA, 温湿度
*******************************************************************************************/
static void n_RefreshDelayOutput(void)
{
  static u8   u8S_SecondDiv = 1000000 / GAS_SENSOR_TASK_TICK_US; //依附于传感器任务,因此使用此节拍时间
  
  AppPrmtDef* pdT_AppPrmt    = x_GetAppPrmt();
  
  if (--u8S_SecondDiv == 0)    //秒
  {
    u8S_SecondDiv = 1000000 / GAS_SENSOR_TASK_TICK_US;
    
    if(dL_StateGroup.u8_CheckingCd == 0U)
    {
      if (dL_StateGroup.u8_WarmupCd != 0)    //自检结束后才会进行预热倒计时
      {
        dL_StateGroup.u8_WarmupCd--;
        
        if(dL_StateGroup.u8_WarmupCd == 178U)   //预热后初始化上传通讯模块
        {
          x_UploadCommInit();     
          dL_StateGroup.d_Upload.NetWork_Flag = 1U;
          dL_StateGroup.d_Upload.WarmUp_Flag = 1U;      //预热上传一次数据
          dL_StateGroup.u32_UpLoadSec = pdT_AppPrmt->u32_AutoUploadSec;   //更新上传周期
        }
          
        if(dL_StateGroup.u8_WarmupCd == 120U)  //打开5V电源电容控制引脚 GT-CXF探测器不具备掉电上传功能
        {
           X_SetGpioOutLoP0_A(GPIO_PIN_08);   //5V电容充电
        }

        if(dL_StateGroup.u8_WarmupCd == 0U)    //预热完成后上传一组数据
        {
          dL_StateGroup.d_Upload.Period_Flag = 1U;
          u8L_Period_flag = 1u;
        }
      }
    }
    
    if((dL_StateGroup.u8_WarmupCd == 0U)&&(dL_StateGroup.u32_UpLoadSec != 0U))  //预热完成后周期上传（30分钟上传一次）
    {
//      static u8 u8_HeartUploadSec = 180u;
      dL_StateGroup.u32_UpLoadSec--;
//      if(dL_StateGroup.u32_UpLoadSec!=0u)
//      {
//        u8_HeartUploadSec--;
//      }
      
      if(dL_StateGroup.u32_UpLoadSec == 0U)
      {
        dL_StateGroup.u32_UpLoadSec = pdT_AppPrmt->u32_AutoUploadSec;  //重新赋值
        dL_StateGroup.d_Upload.Period_Flag = 1U;                       //周期上传标志置位
        u8L_Period_flag = 1u;
      }
//      else
//      {
//        if(u8_HeartUploadSec==0u)
//        {
//          dL_StateGroup.d_Upload.Period_Flag = 1U; 
//          u8_HeartUploadSec = 180u;
//          u8L_Heart_flag = 1u;
//        }
//      }
    }
  }     
}
/******************************************************************************************   
@brief   更新实时输出: 继电器和指示灯
*******************************************************************************************/
static void n_RefreshRealTimeOutput(void)
{
  static bl                  blS_Hi      = false;
  static bl                  blS_Lo      = false;
  DetectorLedStateDef        eT_LedState  = _DetectorLedPowerOff;
  bl                         blT_Hi       = false;
  bl                         blT_Lo       = false;
  
  if (dL_StateGroup.u8_CheckingCd != 0)
  {
    eT_LedState = _DetectorLedSelfChecking;
  }
  else
  {
    if (dL_StateGroup.u8_WarmupCd != 0)
    {
      if (dL_StateGroup.d_Fault.u8_All != 0)
      {
        eT_LedState = _DetectorLedOnlyFault;
      }
      else
      {
        eT_LedState = _DetectorLedWarmup;
      }
    }
    else
    {
      if (dL_StateGroup.d_Alarm.s_Hi != 0)
      {
        blT_Hi = true;
        blT_Lo = true;
        
        if (dL_StateGroup.d_Fault.u8_All != 0)
        {
          eT_LedState = _DetectorLedFaultAlarmH;
        }
        else
        {
          eT_LedState = _DetectorLedOnlyAlarmH;
        }
      }
      else if (dL_StateGroup.d_Alarm.s_Lo != 0)
      {
        blT_Lo = true;
        
        if (dL_StateGroup.d_Fault.u8_All != 0)
        {
          eT_LedState = _DetectorLedFaultAlarmL;
        }
        else
        {
          eT_LedState = _DetectorLedOnlyAlarmL;
        }
      }
      else
      {
        if (dL_StateGroup.d_Fault.u8_All != 0)
        {
          eT_LedState = _DetectorLedOnlyFault;
        }
        else if (dL_StateGroup.u8_LossPowr != 0U)   //增加掉电状态20220412
        {
          eT_LedState = _DetectorLedPowerOff;      //三灯全灭
        }
        else
        {
          eT_LedState = _DetectorLedMonitoring;
        }
      }
    }
  }
  
  if (blS_Hi != blT_Hi)
  {
    x_SetOutputRelay(OUTPUT_RELAY_ALARM_H, blT_Hi); //设置高报继电器
    x_SetValveAct(true);                          //关闭电磁阀高报状态关闭脉冲阀（脉冲阀默认状态为常通）20211216
    blS_Hi = blT_Hi;
  }
  
  if (blS_Lo != blT_Lo)
  {
    x_SetOutputRelay(OUTPUT_RELAY_ALARM_L, blT_Lo); //设置低报继电器
    blS_Lo = blT_Lo;
  }
  
  x_SetDetectorLed(eT_LedState);                  //设置指示灯
}

/*
内部函数
====================================================================================================
外部函数
*/

/******************************************************************************************     
@brief   核心获取参数错误消息的钩子函数
*******************************************************************************************/
void x_CoreHookPrmtError(bl blI_Error)
{
  dL_StateGroup.d_Fault.s_Prmt  = (u8)blI_Error; //参数故障不受核心状态影响
}

/******************************************************************************************   
@brief   核心获取气体传感器浓度和监测结果的钩子函数
*******************************************************************************************/
void x_CoreHookGasSnsrRslt(const GasSnsrOutputDef* pcdI_SnsrOutput)
{
  n_RefreshCoreStateGroup(pcdI_SnsrOutput);                  //状态集更新
  n_RefreshDelayOutput();                                   //更新定时输出: 核心计时, 4-20mA, 温湿度
  n_RefreshRealTimeOutput();                                //更新实时输出: 继电器和指示灯
}

/******************************************************************************************   
@brief   设置核心接受的外部命令
*******************************************************************************************/
void x_SetCoreAcceptCmd(const CoreAcceptCmdDef* pcdI_Cmd, tn* ptnO_Rslt)
{
  tn tnT_Rslt;
  
  if (ptnO_Rslt != NULL) //通信命令,返回执行结果
  {
    if (pcdI_Cmd->e_Cmd == _CoreCmdStopWarmup) //对于通信命令,不支持跳过热
    {
      *ptnO_Rslt = _False;
    }
    else 
    {
      *ptnO_Rslt = n_ExecCoreAcceptCmd(pcdI_Cmd);
    }
  }
  else                  //红外命令,控制指示灯
  {
    if (dL_StateGroup.u8_CheckingCd != 0) //自检状态不支持任何命令,且不做指示灯响应
    {
      return;
    }
    
    if (dL_StateGroup.u8_WarmupCd == 0)  //非预热状态支持所有命令
    {
      tnT_Rslt = n_ExecCoreAcceptCmd(pcdI_Cmd);
    }
    else                                //预热状态只支持跳过预热和标定4,20mA
    {
      if ((pcdI_Cmd->e_Cmd == _CoreCmdStopWarmup)||(pcdI_Cmd->e_Cmd == _CoreCmdCal4mA)||(pcdI_Cmd->e_Cmd == _CoreCmdCal20mA))
      {
        tnT_Rslt = n_ExecCoreAcceptCmd(pcdI_Cmd);
      }
      else
      {
        tnT_Rslt = _False;
      }
    }
    
    if (tnT_Rslt == _True)
    {
      x_SetDetectorLed(_DetectorLedReplyRight);
    }
    else
    {
      x_SetDetectorLed(_DetectorLedReplyError);
    }
  }
}

/******************************************************************************************    
@brief   获取应用程序核心状态集
*******************************************************************************************/
CoreStateGroupDef* x_GetCoreStateGroup(void)
{
  return &dL_StateGroup;
}

/******************************************************************************************     
@brief   向核心传送阀状态 由 DeviceDriveTask.c 调用
*******************************************************************************************/
void x_SendValveStateCore(ValveFlagDef eI_State)
{
  if((_ValveOpen == eI_State)||(_ValveClose == eI_State)||
     (_ValveFaultLost == eI_State)||(_ValveFaultShort == eI_State))
  {
    dL_StateGroup.d_Valve = eI_State;//更新阀状态
  }
}
/******************************************************************************************     
@brief   向核心传送网络上传状态及相关设备信息 由 UploadCommTask.c 调用
*******************************************************************************************/
void x_SendUploadStateCore(tn tnI_State, const u8* pcu8I_NetIMEI, const u8* pcu8I_SimICCID)
{
  u8 u8T_NumCnt;
  
  dL_StateGroup.tn_Upload = tnI_State;
  
  if(pcu8I_NetIMEI != NULL)
  {
    for(u8T_NumCnt = 0; u8T_NumCnt < 15U; u8T_NumCnt++)
    {
       dL_StateGroup.au8_NetIMEI[u8T_NumCnt] = *(pcu8I_NetIMEI + u8T_NumCnt);
    }
  }
  if(pcu8I_SimICCID != NULL)
  {
    for(u8T_NumCnt = 0; u8T_NumCnt < 20U; u8T_NumCnt++)
    {
       dL_StateGroup.au8_SimICCID[u8T_NumCnt] = *(pcu8I_SimICCID + u8T_NumCnt);
    }
  }
}

