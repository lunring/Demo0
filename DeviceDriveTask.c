/******************************************************************************************
@copyright 2017-2018, 汉威科技集团股份有限公司
@file      DeviceDriveTask.c
@version   V1.0
@brief     设备驱动
@warning:  None
@remarks:  None
1.version V1.0    author waf   date 2018.8.2
modification 设备驱动文件
*******************************************************************************************/
#include "fm33lc0xx_ll_gpio.h"
#include "fm33lc0xx_ll_rcc.h"
#include "fm33lc0xx_ll_bstim.h"

#include "GlobalFormatFm33Lc0xx.h"
#include "TimerHal.h"
#include "GpioHal.h"
#include "ClockHal.h"
#include "CoreHal.h"
#include "AdcHal.h"
#include "GpioHal.h"

#include "74HC595Drv.h"
#include "OutputRelay.h"
#include "SegLedGui.h"
#include "IrCommRecHal.h"
#include "SoftTimer.h"
#include "IrRecTask.h"
#include "AppCoreLogic.h"
#include "AppPrmtStorage.h"
#include "DataProtect.h"

#include "fm33lc0xx_ll_gptim.h"
#include "fm33lc0xx_ll_atim.h"

#include "HomeAlarmValve.h"
//#include "DeviceDriveTask.h"
#include "DetectorLed.h"
#include "GbExtCommTask.h"
#include "UploadCommTask.h"
#include "EC800NGpioDirver.h"
/*
文件引用
====================================================================================================
宏定义
*/

#define  SEG_LED_DISP_ALL   (0) 		//全显
#define  SEG_LED_DISP_OFF   (200)		//全灭
#define  SEG_LED_DISP_VER   (280)		//显示版本号
#define  SEG_LED_DISP_HI    (380)		//显示高报值
#define  SEG_LED_DISP_LO    (480)		//显示低报值
#define  SEG_LED_DISP_RANG  (580)		//显示量程
#define  SEG_LED_DISP_CLEAR (685)		//清屏

/*
宏定义
====================================================================================================
格式声明
*/

/*
格式声明
====================================================================================================
函数声明
*/

static void  n_SetRelay(u16 u16I_No, bl blI_Ctrl);
static void  n_SetLed(DetectorLedColorDef eI_Led, bl blI_Ctrl);
static u32*  n_GetLedMemory(u16 u16I_Byte);
static u32*  n_GetRelayMemory(u16 u16I_Byte);
static void n_InitRelayGpio(void);
static void n_InitLedGpio(void);
static void n_InitIrGpio(void);
static void  n_ConfigGpio(void);
static void n_Timer0BasCall(u16 u16I_T1, u16 u16I_T2, void* pxI_CallPrmt);
static void n_DetectorSelfDisp(void);
static void n_RunDeviceDriveTask(void);

/*
函数声明
====================================================================================================
外部变量
*/

extern u8 u8G_UploaderFlag;
/*
外部变量
====================================================================================================
变量定义
*/

static const OutputRelayObjectDef cdL_RelayObject = 
{
  .pf_GetMemory = n_GetRelayMemory,
  .pf_SetRelay  = n_SetRelay,
  .pf_Call      = NULL,
};

static const DetectorLedObjectDef cdL_LedObject = 
{
  .pf_GetMemory = n_GetLedMemory,
  .pf_SetLed    = n_SetLed,
  .pf_Call      = NULL,
};


static u16 au16S_AdArray[SNSR_AD_ARRAY_LEN];    

static u8  u8S_250UsCnt = 0U;        //250Us计数器

/*
变量定义
====================================================================================================
内部函数
*/


/******************************************************************** 
函 数 名：static void n_SetRelay(u16 u16I_No, bl blI_Ctrl)
功    能：OutputRelayNoDef继电器编号,MINI探测器仅支持高低报两个, bl输出控制 true继电器得电
说    明: 
入口参数：无
返 回 值：无
设    计：waf                    日    期：2021-01-21
修    改：                       日    期： 
*********************************************************************/
static void n_SetRelay(u16 u16I_No, bl blI_Ctrl)
{
  if (u16I_No == OUTPUT_RELAY_ALARM_H) //高报PA.1
  {
    if (blI_Ctrl == true)
    {
      X_SetGpioOutHiP0_A(GPIO_PIN_01);   //打开高报继电器
    }
    else
    {
      X_SetGpioOutLoP0_A(GPIO_PIN_01);   //关闭高报继电器
    }
  }
  else  //低报PA0
  {
    if (blI_Ctrl == true)
    {
      X_SetGpioOutHiP0_A(GPIO_PIN_00);   //打开低报继电器
    }
    else
    {
      X_SetGpioOutLoP0_A(GPIO_PIN_00);   //关闭低报继电器
    }
  }
}

/******************************************************************** 
函 数 名：static void n_SetLed(DetectorLedColorDef eI_Led, bl blI_Ctrl)
功    能：DetectorLedColorDef 指定颜色的灯, bl输出控制 true点亮
说    明: 
入口参数：无
返 回 值：无
设    计：waf                    日    期：2021-01-21
修    改：                       日    期： 
*********************************************************************/
static void n_SetLed(DetectorLedColorDef eI_Led, bl blI_Ctrl)
{
  if (eI_Led == _DetectorLedColorRed) //红灯PB12
  {
    if (blI_Ctrl == false)
    {
      X_SetGpioOutLoP1_B(GPIO_PIN_12);
    }
    else
    {
      X_SetGpioOutHiP1_B(GPIO_PIN_12);
    }
  }
  else if (eI_Led == _DetectorLedColorYellow) //黄灯PB14
  {
    if (blI_Ctrl == false)
    {
      X_SetGpioOutLoP1_B(GPIO_PIN_14);
    }
    else
    {
      X_SetGpioOutHiP1_B(GPIO_PIN_14);
    }
  }
  else if(eI_Led == _DetectorLedColorGreen)//绿灯PB13
  {
    if (blI_Ctrl == false)
    {
      X_SetGpioOutLoP1_B(GPIO_PIN_13);
    }
    else
    {
      X_SetGpioOutHiP1_B(GPIO_PIN_13);
    } 
  }
  else     //蜂鸣器控制PA2
  {
     if (blI_Ctrl == false)
    {
      X_SetGpioOutLoP0_A(GPIO_PIN_02);   //关闭蜂鸣器
    }
    else
    {
      X_SetGpioOutHiP0_A(GPIO_PIN_02);   //打开蜂鸣器
    } 
  }
}

/******************************************************************** 
函 数 名：static u32* n_GetLedMemory(u16 u16I_Byte)
功    能：获取状态和计时缓存,静态类型且初始化为0, u16获取内存的长度单位字节
说    明: 
入口参数：无
返 回 值：无
设    计：waf                    日    期：2021-01-21
修    改：                       日    期： 
*********************************************************************/
static u32* n_GetLedMemory(u16 u16I_Byte)
{
  static u32 u32S_LedMemory[DETECTOR_LED_MEMORY_BYTE / 4];
  return     u32S_LedMemory;
}

/******************************************************************** 
函 数 名：static u32* n_GetRelayMemory(u16 u16I_Byte)
功    能：获取状态和计时缓存,静态类型且初始化为0, u16获取内存的长度单位字节
说    明: 
入口参数：无
返 回 值：无
设    计：waf                    日    期：2021-01-21
修    改：                       日    期： 
*********************************************************************/
static u32* n_GetRelayMemory(u16 u16I_Byte)
{
  static u32 u32S_RelayMemory[OUTPUT_RELAY_MEMORY_BYTE / 4];
  return     u32S_RelayMemory;
}

/******************************************************************** 
函 数 名：static void n_ConfigGpio(void)
功    能：继电器蜂鸣器控制引脚初始化
说    明: 
入口参数：无
返 回 值：无
设    计：waf                    日    期：2021-01-21
修    改：                       日    期： 
*********************************************************************/
static void n_InitRelayGpio(void)
{
    X_ConfigGpioOutP0_A(GPIO_PIN_00);    /*继电器1-低报继电器控制引脚*/
    X_ConfigGpioOutP0_A(GPIO_PIN_01);    /*继电器2-高报继电器控制引脚*/
    X_ConfigGpioOutP0_A(GPIO_PIN_04);    /*电磁阀控制*/
    X_SetGpioOutHiP0_A(GPIO_PIN_04);     /*电磁阀控制输出高电平*/
  
    Valve_Init_Out;                   //电磁阀控制引脚初始化
    Valve_False;                      //电磁阀失电
    Valve_CheckPin_Init;              //电磁阀检测引脚初始化
    Valve_DRCtrl_Init_Out;            //电磁阀电容控制引脚初始化 
    Valve_DRCtrl_False;               //电磁阀电容控制引脚失电
}
/******************************************************************** 
函 数 名：static void n_InitLedGpio(void)
功    能：LED指示灯控制引脚初始化
说    明: 
入口参数：无
返 回 值：无
设    计：waf                    日    期：2021-01-21
修    改：                       日    期： 
*********************************************************************/
static void n_InitLedGpio(void)
{
   /*运行指示灯---PB13*/
   X_ConfigGpioOutP1_B(GPIO_PIN_13);

   /*报警指示灯--PB12*/
   X_ConfigGpioOutP1_B(GPIO_PIN_12);
  
   /*故障指示灯--PB14*/
   X_ConfigGpioOutP1_B(GPIO_PIN_14);

   /*蜂鸣器控制--PA2*/
   X_ConfigGpioOutP0_A(GPIO_PIN_02);    /*蜂鸣器*/
}

/******************************************************************** 
函 数 名：static void n_InitIrGpio(void)
功    能：IR红外接收头引脚初始化
说    明: 
入口参数：无
返 回 值：无
设    计：waf                    日    期：2021-01-21
修    改：                       日    期： 
*********************************************************************/
static void n_InitIrGpio(void)
{
  /*红外接收头引脚*/
  X_ConfigGpioInPuP1_B(GPIO_PIN_04);   /*上拉输入*/
}

/******************************************************************** 
函 数 名：static void n_GpioCall(GpioPortNameDef eI_GpioPort, u32 u32I_Pin, void* pxI_CallPrmt)
功    能：根据硬件配置IO
说    明: 
入口参数：无
返 回 值：无
设    计：waf                    日    期：2021-01-21
修    改：                       日    期： 
*********************************************************************/
static void n_GpioCall(GpioPortNameDef eI_GpioPort, u32 u32I_Pin, void* pxI_CallPrmt)
{
   CoreStateGroupDef*  pdT_CoreState = x_GetCoreStateGroup();
   SegLedDispDataDef*  pdT_SegLedDat = cdG_SegLedDispAbst.pf_GetLedDispBuf();
   
   if(RESET == LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_1))    //IO口低电平掉电触发上传
   {
      pdT_CoreState->d_Upload.LossPowr_Flag = 1U;
      pdT_CoreState->u8_LossPowr = 1U;
      u8L_Loss_flag = 1u;
   }
   else
   {
     NVIC_SystemReset();
     //pdT_CoreState->u8_LossPowr = 0U;
   }
}

/******************************************************************** 
函 数 名：static void n_ConfigGpio(void)
功    能：根据硬件配置IO
说    明: 
入口参数：无
返 回 值：无
设    计：waf                    日    期：2021-01-21
修    改：                       日    期： 
*********************************************************************/
static void n_ConfigGpio(void)
{
    X_ConfigGpioOutP0_A(GPIO_PIN_03);  //传感器电源控制引脚初始化PA3
    x_InitSegGpio();                   //初始化四位一体数码管控制引脚
    n_InitIrGpio();                    //初始化红外接收头引脚
    n_InitLedGpio();                   //初始化LED指示灯控制引脚
    n_InitRelayGpio();                 //初始化继电器控制引脚
    X_SetGpioOutLoP0_A(GPIO_PIN_03);   //失能模组电源
    
    x_SetGpioCall(_GpioPort2_C, GPIO_PIN_01,n_GpioCall,NULL);   //配置掉电上传触发信号 GT-CXF不具备掉电上传功能
    X_ConfigGpioOutP0_A(GPIO_PIN_08);  //5V电源电容控制引脚
    X_SetGpioOutHiP0_A(GPIO_PIN_08);   //关闭5V电容充电
}

/******************************************************************** 
函 数 名：static void n_Timer0BasCall(u16 u16I_T1, u16 u16I_T2, void* pxI_CallPrmt)
功    能：定时器0回调5ms定时刷新LED显示
说    明: 
入口参数：无
返 回 值：无
设    计：waf                    日    期：2021-01-21
修    改：                       日    期： 
*********************************************************************/
static void n_Timer0BasCall(u16 u16I_T1, u16 u16I_T2, void* pxI_CallPrmt)
{
  SegLedDispDataDef* pdT_SegLedDat;
  
  pdT_SegLedDat = cdG_SegLedDispAbst.pf_GetLedDispBuf();
  x_SegLedRefresh(pdT_SegLedDat->u8_LedDispBuf,0U);
}

/******************************************************************** 
函 数 名：static void n_Timer1BasCall(u16 u16I_T1, u16 u16I_T2, void* pxI_CallPrmt)
功    能：定时器1回调10ms定时系统节拍
说    明: 
入口参数：无
返 回 值：无
设    计：waf                    日    期：2021-01-21
修    改：                       日    期： 
*********************************************************************/
static void n_Timer1BasCall(u16 u16I_T1, u16 u16I_T2, void* pxI_CallPrmt)
{
  cdG_SoftTmrAbst.pf_SoftTmrCntDec();    //软件定时器减1
}

/******************************************************************** 
函 数 名：static void n_Timer2BasCall(u16 u16I_T1, u16 u16I_T2, void* pxI_CallPrmt)
功    能：定时器2回调250us定时器红外遥控器
说    明: 
入口参数：无
返 回 值：无
设    计：waf                    日    期：2021-01-21
修    改：                       日    期： 
*********************************************************************/
static void n_Timer2BasCall(u16 u16I_T1, u16 u16I_T2, void* pxI_CallPrmt)
{
  x_RunIrCommRecAbst0();     //250us调用红外驱动
  
  u8S_250UsCnt++;
  if(u8S_250UsCnt == 10U)
  {
    u8S_250UsCnt = 0U;
    
    x_RunHomeAlarmValve((void (*)(HmAlrmValveStateDef))x_SendValveStateCore); 
  }
  
  /*
  static CoreStateGroupDef *dL_StateGroup;
  static u8 u8_i = 0;
  dL_StateGroup = x_GetCoreStateGroup();
  
    if(dL_StateGroup->u8_CheckingCd == 0U)
    {
      if (dL_StateGroup->u8_WarmupCd != 0)    //自检结束后才会进行预热倒计时
      {
        if((dL_StateGroup->u8_WarmupCd < 160U) && (dL_StateGroup->u8_WarmupCd > 150U))  //自检时间+预热20秒后打开传感器电源
        {
          if( u8_i == 2)
          {
            X_SetGpioOutHiP0_A(GPIO_PIN_03);              //关闭传感器电源
            u8_i = 0;
          }
          else
          {
            X_SetGpioOutLoP0_A(GPIO_PIN_03);              //打开传感器电源   
          }
          u8_i++;
        }
        else if(dL_StateGroup->u8_WarmupCd < 150U)
        {
           X_SetGpioOutHiP0_A(GPIO_PIN_03);              //打开传感器电源
        }
      }
    }
*/
}

/******************************************************************** 
函 数 名：static void n_DetectorSelfDisp(void)
功    能：显示初值
说    明: 自检显示界面
入口参数：
返 回 值：无
设    计：waf                日    期：2021-02-08
修    改：                       日    期： 
**********************************************************************/
static void n_DetectorSelfDisp(void)
{
  static u16 u16T_DetectorSelf = 0U;
  
  SegLedDispDataDef* pdT_SegLedDat = cdG_SegLedDispAbst.pf_GetLedDispBuf();
  
  AppPrmtDef* pdT_AppPrmt = x_GetAppPrmt();
  
  CoreStateGroupDef*  pdT_CoreState = x_GetCoreStateGroup();
  
  switch(u16T_DetectorSelf)
  {
       case SEG_LED_DISP_ALL:
             cdG_SegLedDispAbst.pf_LedDispCtrl(pdT_SegLedDat->u8_LedDispBuf,true);
       break;
     
      case SEG_LED_DISP_OFF:
        cdG_SegLedDispAbst.pf_LedDispCtrl(pdT_SegLedDat->u8_LedDispBuf,false);
        break;
        
      case SEG_LED_DISP_VER:
        cdG_SegLedDispAbst.pf_LedDispVer(pdT_SegLedDat->u8_LedDispBuf,pdT_AppPrmt->u16_SoftVer);
        break;
        
      case SEG_LED_DISP_HI:
        cdG_SegLedDispAbst.pf_LedDispNum(pdT_SegLedDat->u8_LedDispBuf,pdT_AppPrmt->f32_AlarmH,1U);
        break;
        
      case SEG_LED_DISP_LO:
        cdG_SegLedDispAbst.pf_LedDispNum(pdT_SegLedDat->u8_LedDispBuf,pdT_AppPrmt->f32_AlarmL,1U);
        break;
        
      case SEG_LED_DISP_RANG:
        cdG_SegLedDispAbst.pf_LedDispNum(pdT_SegLedDat->u8_LedDispBuf,pdT_AppPrmt->f32_SnsrRange,1U);
        break;
        
      case SEG_LED_DISP_CLEAR:
        pdT_CoreState->u8_CheckingCd = 0U;      //自检完成
        break;
     
      default:
        break;
  }
  u16T_DetectorSelf++;
}

/******************************************************************** 
函 数 名：static void n_RunDeviceDriveTask(void)
功    能：
说    明: 
入口参数：
返 回 值：无
设    计：waf                日    期：2021-02-08
修    改：                       日    期： 
**********************************************************************/
static void n_RunDeviceDriveTask(void)
{
  SegLedDispDataDef* pdT_SegLedDat =cdG_SegLedDispAbst.pf_GetLedDispBuf();    //获取显示缓存
  
  CoreStateGroupDef* pdT_CoreState = x_GetCoreStateGroup();  //获取核心状态集
  
  AppPrmtDef* pdT_AppPrmt = x_GetAppPrmt();
  
  if(pdT_CoreState->u8_CheckingCd == 1U)    //自检中
  {
    n_DetectorSelfDisp();
  }
  
  if(pdT_CoreState->u8_CheckingCd == 0U)
  {
   if(pdT_CoreState->u8_WarmupCd != 0U)  //预热中
  {
    cdG_SegLedDispAbst.pf_LedDispNum(pdT_SegLedDat->u8_LedDispBuf,(f32)pdT_CoreState->u8_WarmupCd,0U);
  }
  else if(pdT_CoreState->d_Fault.s_Prmt == 1U)
  {
    cdG_SegLedDispAbst.pf_LedDispErr(pdT_SegLedDat->u8_LedDispBuf,1U);
  }
  else if(pdT_CoreState->d_Fault.s_Snsr == 1U)
  {
    cdG_SegLedDispAbst.pf_LedDispErr(pdT_SegLedDat->u8_LedDispBuf,2U);
  }
  else if(pdT_CoreState->d_Fault.s_Cal == 1U)
  {
    cdG_SegLedDispAbst.pf_LedDispErr(pdT_SegLedDat->u8_LedDispBuf,6U);
  }
  else if(pdT_CoreState->u8_LossPowr == 1U)   //探测器掉电
  {
    cdG_SegLedDispAbst.pf_LedDispCtrl(pdT_SegLedDat->u8_LedDispBuf,false);
  }
  else if((pdT_CoreState->u8_DectorDispPrmtState > 0U) && (pdT_CoreState->u8_DectorDispPrmtState < 5U))
  {
    //显示设备地址
    if(pdT_CoreState->u8_DectorDispPrmtState == 1U)
    {
      cdG_SegLedDispAbst.pf_LedDispNum(pdT_SegLedDat->u8_LedDispBuf,pdT_AppPrmt->u16_CommAddr,0U);
    }
    else if(pdT_CoreState->u8_DectorDispPrmtState == 2U)
    {
      cdG_SegLedDispAbst.pf_LedDispNum(pdT_SegLedDat->u8_LedDispBuf,pdT_AppPrmt->f32_AlarmH,1U);
    }
    if(pdT_CoreState->u8_DectorDispPrmtState == 3U)
    {
      cdG_SegLedDispAbst.pf_LedDispNum(pdT_SegLedDat->u8_LedDispBuf,pdT_AppPrmt->f32_AlarmL,1U);
    }
    if(pdT_CoreState->u8_DectorDispPrmtState == 4U)
    {
      cdG_SegLedDispAbst.pf_LedDispNum(pdT_SegLedDat->u8_LedDispBuf,pdT_CoreState->u32_GasAd,0U);
    }
    else
    {
      return;           
    }
  }
  else
  {
    cdG_SegLedDispAbst.pf_LedDispNum(pdT_SegLedDat->u8_LedDispBuf,pdT_CoreState->f32_Cctrt,1U);
  }  
  }
}

/*
内部函数
====================================================================================================
外部函数
*/

/*********************************************************************************************************
@brief 系统初始化函数
**********************************************************************************************************/
void x_InitDeviceDriveTask(void)
{
  static       u32   u32S_AdBuff;
  const static u8   cu8S_AdChn = 10U;
  AppPrmtDef*   pdT_AppPrmt     = NULL;
  
  //定时器BSTIM工作8MHZ 5ms定时用于扫描四位一体数码管
  static const TimerBasConfigDef cdS_Tim0BasConfig =
  {
    .e_BasMode            = _TimerClkCntIn,
    .s_ClkCnt.u16_Prescal = 2,
    .s_ClkCnt.u16_Val	    = 39999,
  };
  
  //定时器GPTIM0工作6MHZ 10ms定时用于系统时机
  static const TimerBasConfigDef cdS_Tim1BasConfig =
  {
    .e_BasMode            = _TimerClkCntIn,
    .s_ClkCnt.u16_Prescal = 3,
    .s_ClkCnt.u16_Val	  = 59999,
  };
  
  //定时器ATIM工作24MHZ 250us定时用于接收红外遥控器指令
  static const TimerBasConfigDef cdS_Tim2BasConfig =
  {
    .e_BasMode            = _TimerClkCntIn,
    .s_ClkCnt.u16_Prescal = 0,
    .s_ClkCnt.u16_Val	  = 5999,
  };
  
   /*PA15为D20模组输出电压*/
   static const AdcConfigDef cdS_GasSensorConfig =
  {
    .e_RefVolSrc   = _AdcRefVolSrcInter,
    .pu32_Buff     = &u32S_AdBuff,
    .pcu8_ChnArray = &cu8S_AdChn,
    .u8_ChnCnt     = 1U,
    .e_SampleCnt   = _AdcSampleCnt1,
  };
  
    x_InitClock();           /*初始化时钟*/
    n_ConfigGpio();          /*根据硬件电路配置IO状态*/
    x_InitAppPrmt();                                     /*存储数据*/
    cdG_SoftTmrAbst.pf_SoftTmrInit();       /*软件定时器初始化*/
    
    x_InitDetectorLed(&cdL_LedObject);                   //初始化LED
    x_InitOutputRelay(&cdL_RelayObject);              //初始化输出继电器
    
    x_InitIrRecTask();         //红外接收初始化
   
    pdT_AppPrmt = x_GetAppPrmt();
    
    if(pdT_AppPrmt->u16_ValveCheckDis == 0x01U)  /*阀检测开启*/
    {
      x_HomeAlarmValveInit(false);             /*开启电磁阀检测功能*/
    }
    else                                       /*阀检测关闭*/
    {
      x_HomeAlarmValveInit(true); 
    }
    
    x_SetValveDRPin(true);                   //脉冲电磁阀电容充电
      
    cdG_AdcAbst0.pf_Config(&cdS_GasSensorConfig);        /*配置ADC*/
    cdG_AdcAbst0.pf_StartCtrl(true);                     /*立即启动ADC*/
    
    x_InitGasSensorTask();                               /*初始化传感器任务*/
    x_GbExtCommInit();      //初始化上位机通讯应用 
    
    cdG_TimerAbst0.pf_ConfigBas(&cdS_Tim0BasConfig);   /*配置定时器0*/
    cdG_TimerAbst0.pf_SetBasCall(n_Timer0BasCall,NULL);
    
    cdG_TimerAbst1.pf_ConfigBas(&cdS_Tim1BasConfig);   /*配置定时器1*/
    cdG_TimerAbst1.pf_SetBasCall(n_Timer1BasCall,NULL);
    
    cdG_TimerAbst2.pf_ConfigBas(&cdS_Tim2BasConfig);   /*配置定时器2*/
    cdG_TimerAbst2.pf_SetBasCall(n_Timer2BasCall,NULL);
    
    
    cdG_SoftTmrAbst.pf_StartAutoSoftTmr(TMR_COMMUNICATION,1U);
    cdG_SoftTmrAbst.pf_StartAutoSoftTmr(TMR_GASSENSOR,5U); 
}

/******************************************************************************************
@brief   获取气体传感器实时Ad值
*******************************************************************************************/
u32  x_GetGasSnsrSignalAd(void)
{  
  static u8 u8S_AdIndex = 0U;
  
  au16S_AdArray[u8S_AdIndex] = *cdG_AdcAbst0.pf_GetState()->pu32_Buff; //取出转换值
  u8S_AdIndex++;
  
  if(u8S_AdIndex > 19U)
  {
    u8S_AdIndex = 0U;
  }
  
  u32 u32T_Ad = (u32)x_MedianAvgFilter(au16S_AdArray,20U);
  
  u32T_Ad = x_KalmanFilter(u32T_Ad);
  
  cdG_AdcAbst0.pf_StartCtrl(true);                      //同时内部启动一轮AD转换
  
  return u32T_Ad;
}

/******************************************************************************************
@brief   输出继电器应用层控制
*******************************************************************************************/
void  x_SetOutputRelay(u16 u16I_No, bl blI_Ctrl)
{
  x_SetOutputRelayHold(u16I_No, blI_Ctrl, &cdL_RelayObject);
}

/******************************************************************************************
@brief   输出指示灯应用层控制
*******************************************************************************************/
void  x_SetDetectorLed(DetectorLedStateDef eI_State)
{
  x_SetDetectorLedState(eI_State, &cdL_LedObject);
}

/*********************************************************************************************************
@brief 系统初始化函数
**********************************************************************************************************/
void x_AppTask(void)
{
  CoreStateGroupDef* pdT_CoreStateGroup = x_GetCoreStateGroup();

  if(cdG_SoftTmrAbst.pf_CheckSoftTmr(TMR_COMMUNICATION) == 1U) //10ms
  {
    x_RunDetectorLed(&cdL_LedObject);   //驱动LED
    x_RunOutputRelay(&cdL_RelayObject); //驱动输出继电器
    x_RunAdcAbst0();                    //驱动ADC
    n_RunDeviceDriveTask();
    x_GbExtCommTask();                  //每10ms运行一次上位机通信任务
  }
  
  if(cdG_SoftTmrAbst.pf_CheckSoftTmr(TMR_GASSENSOR) == 1U)
  {
    x_RunGasSensorTask();         //每50ms运行一次传感器任务
    
    if(pdT_CoreStateGroup->d_Upload.NetWork_Flag == 1U)
    {
      x_UploadCommTask();           //每50ms运行一次上传通信任务
    }
  }
}

/******************************************************************************************
@brief   控制电磁阀引脚
*******************************************************************************************/
void x_SetValvePin(bl blI_Act)
{  
  if (blI_Act == true)  
  {
    Valve_True;   //得电
  }
  else
  {
    Valve_False;  //失电
  }
}

/******************************************************************************************
@brief   电磁阀电容控制引脚
*******************************************************************************************/
void x_SetValveDRPin(bl blI_Act)
{
  if(true == blI_Act)
  {
    Valve_DRCtrl_True;   //打开电容控制引脚为电容充电
  }
  else
  {
    Valve_DRCtrl_False;  //关闭电容控制引脚
  }
}

/******************************************************************************************
@brief   阀控制. 注意!控制优先级高于自检动作.由 CoreLogic.c 调用
*******************************************************************************************/
void x_SetValveAct(bl blI_Act)
{
  if (blI_Act == true)
  {
    x_SetHomeAlarmValveAct();  
  }
}
