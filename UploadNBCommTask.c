/******************************************************************************************
@copyright 2017-2018, 汉威科技集团股份有限公司
@file      UploadNBCommTask.c
@version   V1.0
@brief     无线上传任务
@warning:  None
@remarks:  None
1.version V1.0    author waf   date 2018.8.2
modification 设备驱动文件
*******************************************************************************************/
#include "GlobalFormatFm33Lc0xx.h"
#include "TimerHal.h"
#include "GpioHal.h"
#include "ClockHal.h"
#include "CoreHal.h"
#include "AdcHal.h"
#include "GpioHal.h"
#include "UartHal.h"

#include "74HC595Drv.h"
#include "DetectorLed.h"
#include "OutputRelay.h"
#include "SegLedGui.h"
#include "IrCommRecHal.h"
#include "SoftTimer.h"
#include "IrRecTask.h"
#include "AppCoreLogic.h"
#include "AppPrmtStorage.h"
#include "DataProtect.h"

#include "UploadCommTask.h"
#include "StringOperate.h"
#include "BC260YNcGpioDirver.h"
#include "McuMath.h"
/*
文件引用
====================================================================================================
宏定义
*/

#define TIME_500MS   10
static u8 u8L_Aeskey1[16] = {0x86,0xCA,0xD7,0x27,0xDE,0xB5,0x42,0x63,0xB7,0x39,0x60,0xAA,0x79,0xC5,0xD9,0xB7};

//static u8 u8L_Data[146] = {0x83,0x40,0x15,0x01,0x29,0x15,0x21,0x01,0x02,0x03,0x04,0x01,0x08,0x80,0x00,0x00,0x85, \
//                        0x7E,0xCB,0xB6,0x73,0xD3,0x92,0x4D,0x12,0x94,0xD4,0x8C,0x70,0x36,0x8C,0x62,0x61,0xFC, \
//                        0x1F,0xCA,0x34,0x8A,0x5B,0xD5,0x99,0xCB,0x7F,0xD1,0x2D,0xFA,0x2C,0x4A,0xC7,0x3D,0x32, \
//                        0xDE,0x05,0x9C,0x80,0x26,0x08,0x36,0x22,0x30,0xEB,0xA4,0xBB,0xE0,0x9B,0x85,0x8B,0x52, \
//                        0x72,0xC3,0xB4,0xC2,0xCA,0x5D,0x77,0xED,0xE8,0x94,0x89,0x21,0x89,0x6F,0x9F,0x79,0x7A, \
//                        0xE4,0x9A,0xD2,0x56,0xCB,0x4C,0x94,0x5B,0xC7,0xF8,0x14,0x82,0xA0,0xD1,0xD7,0x9C,0xD3, \
//                        0x35,0x2D,0xCF,0x54,0x77,0x92,0xF0,0x27,0xBD,0xE0,0x20,0x8B,0xA2,0x87,0x53,0x09,0x23, \
//                        0xFD,0x66,0x20,0x4D,0xBE,0xE9,0x78,0x19,0x80,0x72,0x52,0x24,0x3B,0x15,0x31,0xF0,0xC9, \
//                        0xE6,0x17,0x3F,0x8D,0x36,0xB3,0xC2,0x92,0x6A,0xED};

/*
宏定义
====================================================================================================
格式声明
*/
typedef enum UpLoadStepDef_
{
	_Step0,
	_Step1,
	_Step2,
	_Step3,
	_Step4,
	_Step5,
	_StepSetUp,
	_StepExit,
  _StepRefresh,
}UpLoadStepDef;

typedef enum MsgStateDef_
{
	_Free, //空闲
	_Busy, //忙
	_Done, //完成
}MsgStateDef;

typedef enum TimeUnitDef_
{
	_Sec 	= 1U,		
	_Minute	= 2U,		
	_Hour 	= 3U,		
	_Day	= 4U,		
	_Month 	= 5U,		
	_Year 	= 6U,		
}TimeUnitDef;

typedef enum SendMsgTypeDef_
{
  _XaStateUpload=0x02,    //状态信息上报
  _XaSampleUpload=0x22,   //采集数据上报
  _XaAckEnd=0x06,         //终止帧上报
  _XaAckConfig=0x07,      //确认帧上报
  _XaAesDown=0x08,        //密钥下发
  _XaHeartUpload=0xCC,    //心跳包上报
  _XaGetIccid=0x09,      //获取CCID号
  _XaAnsError=0x0E,       //错误状态码应答
  _XaSetIpInfo=0x56,      //设置网络IP参数
  _XaSetDimainInfo=0x57,  //设置网络域名参数
  _XaSetCycTime=0x49,     //设置采集上报周期
  _XaReadCycTime=0x4A,    //读取采集上报周期
  _XaSetRtcTime=0x05,     //校对时钟
  _XaSetAlarmLevel=0x58,  //设置报警浓度值
  _XaGetAlarmLevel=0x6A,  //读取报警浓度值
  _XaLossNet=0x04,        //断网
  
}SendMsgTypeDef;

typedef struct UserMsgDef_
{
	MsgStateDef e_MsgState;
	u8 u8_FreeSecCnt; //发送完成后空闲的秒数,间隔太短上位机接收出错
	u16 u16_Len;
	u8 u8_Buf[350];
}UserMsgDef;

typedef struct UserMsgDef1_
{
	MsgStateDef e_MsgState;
	u8 u8_FreeSecCnt; //发送完成后空闲的秒数,间隔太短上位机接收出错
	u16 u16_Len;
	u8 u8_Buf[200];
}UserMsgDef1;

typedef struct XaMsgDef_
{
  u8  u8_KeyNo;//密钥号
  u8  u8_LossPower;//掉电
  u8  u8_UploadType;//上报类型
  SendMsgTypeDef MsgType;
  u8  u8_ErrorType;
}XaMsgDef;
/*
外部变量
====================================================================================================
变量定义
*/     

static UpLoadStepDef eL_Step = _Step0;
static u8 u8L_UpLoadSteps=0;
static u8 u8L_RevBufTemp[8];        //临时接收缓存
static UserMsgDef dL_UserMsgRev;   //用户接收
static UserMsgDef1 dL_UserMsgSend;  //用户发送
static tn n_ProtocolRevData(void);
static XaMsgDef dL_XaMsg;
static u16 n_GetUploadMessage(u8* buff,SendMsgTypeDef u8I_SendMsgType);
static void n_GetHeartUploadMessage(void);
/*
变量定义
====================================================================================================
内部函数
*/
/******************************************************************************************    
@brief   获取浓度
*******************************************************************************************/
static f32 n_GetGasPotency(void)
{
  AppPrmtDef* pdT_AppPrmt = NULL;
  CoreStateGroupDef* pdT_CoreState = NULL;

  pdT_AppPrmt   = x_GetAppPrmt();   //获取参数
  pdT_CoreState = x_GetCoreStateGroup();

  if(pdT_AppPrmt->f32_SnsrRange <= 9.0F)
  {
    pdT_CoreState->f32_Cctrt = pdT_CoreState->f32_Cctrt*1000U;
  }
  else if((pdT_AppPrmt->f32_SnsrRange >= 10.0F) && (pdT_AppPrmt->f32_SnsrRange <= 99.0F))
  {
    pdT_CoreState->f32_Cctrt = pdT_CoreState->f32_Cctrt*100U;
  }
  else if((pdT_AppPrmt->f32_SnsrRange >= 100.0F) && (pdT_AppPrmt->f32_SnsrRange <= 999.0F))
  {
    pdT_CoreState->f32_Cctrt = pdT_CoreState->f32_Cctrt*10U;
  }
  else if((pdT_AppPrmt->f32_SnsrRange >= 1000.0F) && (pdT_AppPrmt->f32_SnsrRange <= 9999.0F))
  {
    pdT_CoreState->f32_Cctrt = pdT_CoreState->f32_Cctrt;
  }
  else
  {
    pdT_CoreState->f32_Cctrt = pdT_CoreState->f32_Cctrt*10U;
  }
  
  return pdT_CoreState->f32_Cctrt;
}

/******************************************************************************************    
@brief   获取状态
*******************************************************************************************/
static u8 n_GetGasState(void)
{
  u8 eT_DeviceState;
  CoreStateGroupDef* pdT_CoreState = x_GetCoreStateGroup();
  
  if(pdT_CoreState->u8_WarmupCd != 0U)   //预热
  {
    eT_DeviceState = 0x09;
  }
  else if(pdT_CoreState->d_Fault.s_Snsr != 0U) //传感器故障
  {
    eT_DeviceState = 0x01;
  }
  else if(pdT_CoreState->d_Fault.s_Cal != 0U) ////标定错误
  {
    eT_DeviceState = 0x03;
  }
  else if(pdT_CoreState->d_Abnormal.s_Hp !=0U) //高浓度保护
  {
    eT_DeviceState = 0x0E;
  }
  else if(pdT_CoreState->d_Alarm.s_Hi != 0U)  //高报
  {
    eT_DeviceState = 0x0E;
  }
  else if(pdT_CoreState->d_Alarm.s_Lo != 0U)  //低报
  {
    eT_DeviceState = 0x0D;
  }
  else
  {
    eT_DeviceState = 0x0A;   //正常状态
  }
  
  return eT_DeviceState;
}

/*
内部函数
====================================================================================================
外部函数
*/

/*********************************************************************************************************
@brief 获取密钥情况
**********************************************************************************************************/
static void n_GetKetStatue(void)
{
  AppPrmtDef* pdT_AppPrmt = NULL;
  pdT_AppPrmt   = x_GetAppPrmt();   //获取参数	
     
 if((pdT_AppPrmt->u8_Key[0][0] == 0x00) 
 && (pdT_AppPrmt->u8_Key[1][0] == 0x00) 
   && (pdT_AppPrmt->u8_Key[2][0] == 0x00)
     && (pdT_AppPrmt->u8_Key[3][0] == 0x00) 
       && (pdT_AppPrmt->u8_Key[4][0] == 0x00) 
         && (pdT_AppPrmt->u8_Key[5][0] == 0x00) 
           && (pdT_AppPrmt->u8_Key[6][0] == 0x00) 
             && (pdT_AppPrmt->u8_Key[7][0] == 0x00))
 {
   dL_XaMsg.u8_KeyNo = 1u;
 }
 else
 {
   dL_XaMsg.u8_KeyNo = 2u;
 }
}



/*********************************************************************************************************
@brief 系统初始化函数
**********************************************************************************************************/
//u8I_Type 类型  0.上电仅发送一次的基本消息 1.周期发送的消息 2.发送正确应答 3.发送错误应答
static void n_SendMessage(SendMsgTypeDef u8I_SendMsgType)
{
    const UartAbstDef* pcdT_Uart;
    
    n_GetKetStatue();
	dL_UserMsgSend.e_MsgState = _Busy;
    dL_UserMsgSend.u16_Len = n_GetUploadMessage(dL_UserMsgSend.u8_Buf,u8I_SendMsgType);
	
    x_MemFill(dL_UserMsgRev.u8_Buf,0,sizeof(dL_UserMsgRev.u8_Buf));
    
	pcdT_Uart = x_GetNbiotBc26DrivePortAbst();
	pcdT_Uart->pf_SetTraData(dL_UserMsgSend.u8_Buf, dL_UserMsgSend.u16_Len);
}

//接收一帧数据
u8 rec_buf[350] = {0};
static void n_RevMessage(u8 u8I_RevData)
{
  static u8 u8S_Step = _Step0;
  u8 u8T_RevData;
  u8T_RevData = u8I_RevData;
  
  if(dL_UserMsgRev.e_MsgState == _Free)
  {
    u8S_Step = _Step0;
  }
  
  switch(u8S_Step)
  {
   case _Step0:
    if(u8T_RevData == '8')
    {
      dL_UserMsgRev.e_MsgState = _Busy;
      dL_UserMsgRev.u16_Len = 0;
      dL_UserMsgRev.u8_Buf[dL_UserMsgRev.u16_Len++] = u8T_RevData;
      u8S_Step = _Step1;
    }
    break;
  case _Step1:  
    if(u8T_RevData == '3')
    {
      dL_UserMsgRev.e_MsgState = _Busy;
      dL_UserMsgRev.u8_Buf[dL_UserMsgRev.u16_Len++] = u8T_RevData;
      u8S_Step = _Step2;      
    }
    break;
  case _Step2:
    
    if(dL_UserMsgRev.u16_Len < sizeof(dL_UserMsgRev.u8_Buf))
    {
      dL_UserMsgRev.u8_Buf[dL_UserMsgRev.u16_Len++] = u8T_RevData;
    }
    else
    {
      dL_UserMsgRev.e_MsgState = _Free;
    }
    if (u8T_RevData == '\r')
    {
      MathAsciiToHex(dL_UserMsgRev.u8_Buf,rec_buf,dL_UserMsgRev.u16_Len-1);
      dL_UserMsgRev.u16_Len = (dL_UserMsgRev.u16_Len-1)/2;
      x_MemCopy(dL_UserMsgRev.u8_Buf,rec_buf,dL_UserMsgRev.u16_Len);
      dL_UserMsgRev.e_MsgState = _Done;
      u8S_Step = _StepExit;
    }     
    break;

  case _StepExit:
    break;
  }
}

static void n_SetTraCall(const u8* pcu8I_Buf, u16 u16I_Len, void* pxI_prmt) //设置发送完成回调,u8*缓存地址,u16处理长度.void*自定义参数
{
	dL_UserMsgSend.e_MsgState    = _Free;
	dL_UserMsgSend.u8_FreeSecCnt = 0U;
}
static void n_SetRecCall(const u8* pcu8I_Buf, u16 u16I_Len, void* pxI_prmt) //设置发送完成回调,u8*缓存地址,u16处理长度.void*自定义参数
{
	u8 u8T_RevData; 
	const UartAbstDef* pcdT_Uart;
	
	pcdT_Uart = x_GetNbiotBc26DrivePortAbst();
  
	u8T_RevData = u8L_RevBufTemp[0];
	
	n_RevMessage(u8T_RevData);
	
	pcdT_Uart->pf_SetRecData(u8L_RevBufTemp, 1);
}

//联网前先延时以达到错峰联网的效果
//返回值: 需延时的时间
u16 n_GetPreConnectTime(void)
{
	return 10;
}


/******************************************************************************************     
@brief   服务器链接回调函数
*******************************************************************************************/
static void n_SrvConnectCallback(NbiotBc26TaskInfoDef TaskInfo, void* pxI_Prmt)
{
  const NbiotBc26DeviceInfoDef* pcdT_DevInfo;
  NbiotBc26ConfigPrmtDef cdL_ConfigPrmt;
  
  switch(TaskInfo.e_CurrentTask)
  {
  case _NbiotBc26Task_StartUp:
    if (TaskInfo.e_TaskReport == _NbiotBc26Report_Right)
    {
      cdL_ConfigPrmt.pcu8_ServerIp   = x_GetAppPrmt()->d_NetIpPort.au8_Ip;
      cdL_ConfigPrmt.pcu8_ServerPort = x_GetAppPrmt()->d_NetIpPort.au8_Port;
      x_SetNbiotBc26ConnectTask(&cdL_ConfigPrmt);
    }
    else
    {
      *((tn*)pxI_Prmt) = _False;
    }
    break;
  case _NbiotBc26Task_Connect:
    if (TaskInfo.e_TaskReport == _NbiotBc26Report_Right)
    {
      pcdT_DevInfo                           = x_GetNbiotBc26DeviceInfo();
      x_SendUploadStateCore(_Unknown, pcdT_DevInfo->u8_MdImei, pcdT_DevInfo->u8_SimCcid);
      *((tn*)pxI_Prmt)                      = _True; 
    }
    else
    {
      x_SendUploadStateCore(_False, NULL, NULL); //传送连接失败信息
      *((tn*)pxI_Prmt) = _False;
    }
    break;	
  case _NbiotBc26Task_Wakeup:
    if (TaskInfo.e_TaskReport == _NbiotBc26Report_Right)
    {
      /* 设置刷新设备信息任务 */
      x_SetNbiotBc26RefreshInfoTask();
    }
    else
    {
      *((tn*)pxI_Prmt) = _False;
    }
    break;
  case _NbiotBc26Task_RefreshInfo:
    if (TaskInfo.e_TaskReport == _NbiotBc26Report_Right)
    {
      //pdT_DevInfo                      = x_GetNbiotBc26DeviceInfo();
      *((tn*)pxI_Prmt)                 = _True; 
    }
    else
    {
      *((tn*)pxI_Prmt) = _False;
    }
    break;
  }
}


/******************************************************************************************     
@brief   任务初始化.由 CoreLogic.c 调用
*******************************************************************************************/
void  x_UploadCommInit(void)
{ 
   x_SendUploadStateCore(_False, NULL, NULL);
   x_UploadCommGpioInit();   //配置NbiotBc26模组控制引脚
   x_SetNbiotBc26Object(&X_GetNbiotBc26Object(0));// 为NbiotBc26模组驱动配置一个操作实体
   eL_Step = _Step0;

//   x_MemCopy(dL_UserMsgRev.u8_Buf,u8L_Data,146);  
//   dL_UserMsgRev.u16_Len = 146;
//   n_ProtocolRevData();
}

/******************************************************************************************     
@brief   运行任务.由 main.c 调用
*******************************************************************************************/
void  x_UploadCommTask(void)
{
	static u16 u16L_PreConnectTime;
	static u8 u8S_ReConnectCnt=3;
	static tn tnL_RunReport = _Unknown;   //模组驱动运行结果 _Unknown 正在运行; _True 成功退出; _False 失败退出
	static u32 u32S_SecCnt=0;
	static u16 u16S_CycleCnt=0xFFFF;
	static u16 u16S_TimeCnt = 0;
        
    CoreStateGroupDef* pdT_CoreState = x_GetCoreStateGroup();
        
	if (++u16S_CycleCnt >= 2*TIME_500MS) //秒到
	{
		u16S_CycleCnt = 0;
		u32S_SecCnt++;
	}
  
	switch (eL_Step)
	{
  case _Step0: //准备启动
      {   
          pdT_CoreState->d_Upload.Normal_Flag = 1;
          cdG_NbiotBc26Object0.pf_SetRst(_False);//20210825，WXY增加Step0硬件复位
          x_BlockingDelayMs(100);                //20210825，WXY增加Step0硬件复位
          cdG_NbiotBc26Object0.pf_SetRst(_True);//20210825，WXY增加Step0硬件复位
          u16L_PreConnectTime = n_GetPreConnectTime();
          eL_Step = _StepSetUp;
          break;
      }
  case _StepSetUp: //启动
      {
        if (u16L_PreConnectTime == 0)
            {
                u8L_UpLoadSteps = 0;
                x_SetNbiotBc26StartCtrlTask(true);
                tnL_RunReport = _Unknown;
                eL_Step = _Step1;
            }
            else
            {
                u16L_PreConnectTime--;
            }
            break;
      }
  case _Step1: //连网
      {
          //20201105：ymy add. 提前获取一些设备信息
          const NbiotBc26DeviceInfoDef* pcdT_DevInfo;                       
          pcdT_DevInfo  = x_GetNbiotBc26DeviceInfo();
          x_SendUploadStateCore(_Unknown, pcdT_DevInfo->u8_MdImei, pcdT_DevInfo->u8_SimCcid);  //此处只为了获取，状态后续可更新  20210712
          x_RunNbiotBc26DriveTask(n_SrvConnectCallback, &tnL_RunReport);  

          if (tnL_RunReport == _False) //联网失败
          {          
              if (u8S_ReConnectCnt > 0 )
              {
                  u8S_ReConnectCnt--;
                  eL_Step = _Step0; //重新开机联网
              }
              else
              {
                  u32S_SecCnt = 0;
                  eL_Step = _Step0; //关机
              }
          }
          else if (tnL_RunReport == _True) //连网成功
          {
              x_InitNbiotBc26DrivePortAbst(); //连网成功后初始化NB模块对接串口

              const UartAbstDef* pcdT_Uart;
              pcdT_Uart = x_GetNbiotBc26DrivePortAbst();
              pcdT_Uart->pf_SetTraCall(n_SetTraCall,NULL);
              pcdT_Uart->pf_SetRecCall(n_SetRecCall,NULL);
              pcdT_Uart->pf_SetRecData(u8L_RevBufTemp, 1);
              eL_Step = _Step2;
          }
         break;
      }
		
  case _Step2: //监控发送和接收
    {
      x_RunNbiotBc26DrivePortAbst((u8*)&tnL_RunReport);

      if(tnL_RunReport==_False)
      {
          eL_Step = _Step0; //重新开机联网
      }
      if (dL_UserMsgSend.e_MsgState == _Free) //发送空闲
      {
        if((pdT_CoreState->d_Upload.WarmUp_Flag == 1U) ||(pdT_CoreState->d_Upload.Normal_Flag == 1U) ||
           (pdT_CoreState->d_Upload.Period_Flag == 1U) ||(pdT_CoreState->d_Upload.LossPowr_Flag == 1U))
        {
          if(pdT_CoreState->d_Upload.LossPowr_Flag==1u)
          {
            dL_XaMsg.u8_LossPower = 1u;
          }
          pdT_CoreState->d_Upload.WarmUp_Flag = 0U;
          pdT_CoreState->d_Upload.Normal_Flag = 0U;
          pdT_CoreState->d_Upload.Period_Flag = 0U; 
          pdT_CoreState->d_Upload.LossPowr_Flag = 0U;
          pdT_CoreState->d_Upload.PulseValve_Flag = 0U;
          
          if (u8L_UpLoadSteps == 0)
          {
            u8L_UpLoadSteps = 1u;
            n_SendMessage(_XaStateUpload);
          }
          else
          {
            tnL_RunReport =_Unknown;
            
            x_SetNbiotBc26RefreshInfoTask();
            eL_Step = _StepRefresh;
            u16S_TimeCnt = 0u;
          }             
        }
        if(dL_UserMsgRev.e_MsgState == _Busy) //正在接收一帧数据
        {
          if(++u16S_TimeCnt > 40*TIME_500MS) //超时则清空并退出对该帧数据的接收
          {
            u16S_TimeCnt = 0;
            dL_UserMsgRev.e_MsgState = _Free;
          }
        }
        if(dL_UserMsgRev.e_MsgState == _Done) //一帧数据接收完成
        {
          u16S_TimeCnt = 0U; //20201125:ymy add.
          (void)n_ProtocolRevData(); //分析接收的数据
          dL_UserMsgRev.e_MsgState = _Free;
        }
      }
    break;
    }
  case _StepRefresh:  //周期上行数据发送前，更新当前的网络状态发送信息 by hzp
    {
      x_RunNbiotBc26DriveTask(n_SrvConnectCallback, &tnL_RunReport);
      if (tnL_RunReport == _True)
      {   
        x_InitNbiotBc26DrivePortAbst(); //连网成功后初始化NB模块对接串口
        
        const UartAbstDef* pcdT_Uart;
        pcdT_Uart = x_GetNbiotBc26DrivePortAbst();
        pcdT_Uart->pf_SetTraCall(n_SetTraCall,NULL);
        pcdT_Uart->pf_SetRecCall(n_SetRecCall,NULL);
        pcdT_Uart->pf_SetRecData(u8L_RevBufTemp, 1);                
        
        if(u8L_Period_flag==1u)//间隔上报
        {
          u8L_Period_flag = 0u;
          dL_XaMsg.u8_UploadType = 0x06;//间隔上报
          n_SendMessage(_XaSampleUpload);
          eL_Step = _Step2;
        }
        
        if(u8L_Error_flag==1u)//故障上传
        {
          u8L_Error_flag = 0u;
          dL_XaMsg.u8_UploadType = 0x07;//报警上报
          n_SendMessage(_XaSampleUpload);
          eL_Step = _Step2;
        }
        
        if(u8L_Alarm_flag==1u)//报警上传
        {
          u8L_Alarm_flag = 0u;
          u8 u8_Type = n_GetGasState();
          if((u8_Type!=0x0D)&&(u8_Type!=0x0E))//非低报非高报
          {
            dL_XaMsg.u8_UploadType = 0x08u;//报警恢复上报
          }
          else
          {
            dL_XaMsg.u8_UploadType = 0x07u;//报警上报
          }
          n_SendMessage(_XaSampleUpload);
          eL_Step = _Step2;
        }

        if(u8L_Loss_flag==1u)//掉电上传
        {
          u8L_Loss_flag = 0u;
          n_SendMessage(_XaStateUpload);
          eL_Step = _Step2;
        } 
        tnL_RunReport  = _Unknown;
        
      }
      else if (tnL_RunReport == _False)
      {//错误处理
        eL_Step = _Step0;
      }
      else
      {
        if(++u16S_TimeCnt > 80*TIME_500MS)
        {
          u16S_TimeCnt = 0u;
          eL_Step = _Step2;
        }
      }
    }
    break;
    case _Step3:  //周期上行数据发送前，更新当前的网络状态发送信息 by hzp
    {
      x_RunNbiotBc26DrivePortAbst((u8*)&tnL_RunReport);;
      if (tnL_RunReport == _True)
      {   
        x_InitNbiotBc26DrivePortAbst(); //连网成功后初始化NB模块对接串口
        
        const UartAbstDef* pcdT_Uart;
        pcdT_Uart = x_GetNbiotBc26DrivePortAbst();
        pcdT_Uart->pf_SetTraCall(n_SetTraCall,NULL);
        pcdT_Uart->pf_SetRecCall(n_SetRecCall,NULL);
        pcdT_Uart->pf_SetRecData(u8L_RevBufTemp, 1);
        eL_Step = _Step2;
        n_SendMessage(_XaAckEnd);
        tnL_RunReport = _Unknown;
      }
      else if (tnL_RunReport == _False)
      {//错误处理
        eL_Step = _Step0;
      }
      if(++u16S_TimeCnt > 40*TIME_500MS)
      {
          u16S_TimeCnt = 0u;
          eL_Step = _Step0;
      }
    }
    break;
  default:
    eL_Step = _StepExit;
    break;
	}
}

/******************************************************************************************     
@brief   任务关闭,关闭所有相关外设,进入低功耗或关机状态.由 CoreLogic.c 调用
*******************************************************************************************/
void  x_UploadCommClose(void)
{
	cdG_NbiotBc26Object0.pf_SetRst(_False);
	cdG_NbiotBc26Object0.pf_SetPsmEint(_False);
}

/******************************************************************************************    
@brief   获取心跳上传数据
*******************************************************************************************/
static void n_GetHeartUploadMessage(void)
{
	const UartAbstDef* pcdT_Uart;
	
	dL_UserMsgSend.e_MsgState = _Busy;
    dL_UserMsgSend.u16_Len = 0u;
	dL_UserMsgSend.u8_Buf[dL_UserMsgSend.u16_Len++] = 0x67u;
    dL_UserMsgSend.u8_Buf[dL_UserMsgSend.u16_Len++] = 0xCCu;
    dL_UserMsgSend.u8_Buf[dL_UserMsgSend.u16_Len++] = 0xEDu;
      
	pcdT_Uart = x_GetNbiotBc26DrivePortAbst();
	pcdT_Uart->pf_SetTraData(dL_UserMsgSend.u8_Buf, dL_UserMsgSend.u16_Len);  
}


/******************************************************************************************    
@brief   获取上传数据
*******************************************************************************************/
static u16 n_GetUploadMessage(u8* buff,SendMsgTypeDef u8I_SendMsgType)
{
  AppPrmtDef* pdT_AppPrmt = NULL;
  
  u16 u16_len = 0u;
  u16 u16_datalen;
  pdT_AppPrmt   = x_GetAppPrmt();   //获取参数
  const NbiotBc26DeviceInfoDef  *Bc26DeviceInfo;
  Bc26DeviceInfo = x_GetNbiotBc26DeviceInfo();
  
  buff[u16_len++] = 0x67;//包头
  buff[u16_len++] = 0x40;//协议版本
  buff[u16_len++] = 0x15;//设备类型
  
  //设备编号
  buff[u16_len++] = 0x01;//厂家编码
  buff[u16_len++] = 0x29; 
  buff[u16_len++] = 0x15;//设备类型ID
  buff[u16_len++] = 0x21;//设备型号ID
  buff[u16_len++] = (u8)(pdT_AppPrmt->u16_DetectorId[8U] >> 8U);//截取设备ID后四字节
  buff[u16_len++] = (u8)(pdT_AppPrmt->u16_DetectorId[8U]);
  buff[u16_len++] = (u8)(pdT_AppPrmt->u16_DetectorId[9U] >> 8U);
  buff[u16_len++] = (u8)(pdT_AppPrmt->u16_DetectorId[9U]);
  
  buff[u16_len++] = dL_XaMsg.u8_KeyNo;//密钥号
  buff[u16_len++] = (u8)u8I_SendMsgType;//命令码
  
  u16_datalen = u16_len;
  u16_len += 2u;
  
  u8* pu8T_Buff = &buff[u16_len];
  u8 u8_Length = 0u;
  
  switch(u8I_SendMsgType)
  {
    case _XaStateUpload://0x02   //状态信息上报
    {
       if(dL_XaMsg.u8_LossPower==1u)//掉电
       {
         pu8T_Buff[u8_Length++] = 0x02;//上报类型  掉电上报
       }
       else
       {
         pu8T_Buff[u8_Length++] = 0x01;//上报类型  上电上报
       }
       pu8T_Buff[u8_Length++] = Bc26DeviceInfo->u8_Time[1];
       pu8T_Buff[u8_Length++] = Bc26DeviceInfo->u8_Time[2];
       pu8T_Buff[u8_Length++] = Bc26DeviceInfo->u8_Time[3];
       pu8T_Buff[u8_Length++] = Bc26DeviceInfo->u8_Time[4];
       pu8T_Buff[u8_Length++] = Bc26DeviceInfo->u8_Time[5];
       pu8T_Buff[u8_Length++] = Bc26DeviceInfo->u8_Time[6];
       
       if(dL_XaMsg.u8_LossPower==1u)//掉电
       {
         pu8T_Buff[u8_Length++] = 0x02;//掉电
       }
       else
       {
         pu8T_Buff[u8_Length++] = 0x01;//正常供电
       }    
       dL_XaMsg.u8_LossPower = 0u;
       x_MemCopy(&pu8T_Buff[u8_Length], x_GetNbiotBc26DeviceInfo()->u8_SimCcid, 20U);  //CCID
       u8_Length += 20u;
       pu8T_Buff[u8_Length++] = 0x00;//数据补位
       pu8T_Buff[u8_Length++] = 0x00;
       pu8T_Buff[u8_Length++] = 0x00;
       pu8T_Buff[u8_Length++] = 0x00;       
    }
    break;
    case  _XaSampleUpload://0x22,   //采集数据上报
    {
      pu8T_Buff[u8_Length++] = 0x01;  //本包条数
      pu8T_Buff[u8_Length++] = dL_XaMsg.u8_UploadType;
      if(pdT_AppPrmt->u16_GasName==1u)
      {
        pu8T_Buff[u8_Length++] = 0x01;//甲烷
      }
      else if(pdT_AppPrmt->u16_GasName==0x0A)
      {
        pu8T_Buff[u8_Length++] = 0x03;//丙烷
      }
      else
      {
        pu8T_Buff[u8_Length++] = 0x01;//甲烷
      }
       pu8T_Buff[u8_Length++] = Bc26DeviceInfo->u8_Time[1];//时间
       pu8T_Buff[u8_Length++] = Bc26DeviceInfo->u8_Time[2];
       pu8T_Buff[u8_Length++] = Bc26DeviceInfo->u8_Time[3];
       pu8T_Buff[u8_Length++] = Bc26DeviceInfo->u8_Time[4];
       pu8T_Buff[u8_Length++] = Bc26DeviceInfo->u8_Time[5];
       pu8T_Buff[u8_Length++] = Bc26DeviceInfo->u8_Time[6];    
      
      pu8T_Buff[u8_Length++] = 0xFF;//电池电压
      pu8T_Buff[u8_Length++] = 0xFF;
      
      pu8T_Buff[u8_Length++] = 113-(x_GetNbiotBc26DeviceInfo()->u8_SigQuality*2)+2;//信号强度等级
      u16 Cctrt = (u16)n_GetGasPotency();
      Cctrt = Cctrt/10;
      
      Cctrt = Cctrt*100u;
      pu8T_Buff[u8_Length++] = (u8)Cctrt;   
      pu8T_Buff[u8_Length++] = (u8)(Cctrt>>8u);//探测器浓度     
      
      pu8T_Buff[u8_Length++] = n_GetGasState();//探测器状态
      pu8T_Buff[u8_Length++] = 0xFF;//无输出
      
      x_MemFill(&pu8T_Buff[u8_Length],0xFF,36u);      
      u8_Length += 36u;
        
      if((u8_Length % 16) != 0x00)//数据域长度不足16 需要补0
      {
        u8 u8_i = 16 - (u8_Length % 16);
        for(; u8_i > 0; u8_i--)
        {                                                             
          pu8T_Buff[u8_Length++] = 0x00;
        }
      }
    }
    break;
    case  _XaAesDown://0x08,        //密钥下发
    {
      pu8T_Buff[u8_Length++] = 0x00u;
      if((u8_Length % 16) != 0x00)//数据域长度不足16 需要补0
      {
        u8 u8_i = 16 - (u8_Length % 16);
        for(; u8_i > 0; u8_i--)
        {                                                             
          pu8T_Buff[u8_Length++] = 0x00;
        }
      }       
    }
    break;
    case  _XaGetIccid://0x09,      //获取CCID号
    {
       x_MemCopy(&pu8T_Buff[u8_Length], x_GetNbiotBc26DeviceInfo()->u8_SimCcid, 20U);  //CCID
       u8_Length += 20u;
      if((u8_Length % 16) != 0x00)//数据域长度不足16 需要补0
      {
        u8 u8_i = 16 - (u8_Length % 16);
        for(; u8_i > 0; u8_i--)
        {                                                             
          pu8T_Buff[u8_Length++] = 0x00;
        }
      }        
    }
    break;
    case  _XaAnsError://0x0E,       //错误状态码应答
    {
      pu8T_Buff[u8_Length++] = dL_XaMsg.u8_ErrorType;
      if((u8_Length % 16) != 0x00)//数据域长度不足16 需要补0
      {
        u8 u8_i = 16 - (u8_Length % 16);
        for(; u8_i > 0; u8_i--)
        {                                                             
          pu8T_Buff[u8_Length++] = 0x00;
        }
      }        
    }
    break;
    case  _XaSetIpInfo://0x56,      //设置网络IP参数
    {
      pu8T_Buff[u8_Length++] = 0x00u;
      if((u8_Length % 16) != 0x00)//数据域长度不足16 需要补0
      {
        u8 u8_i = 16 - (u8_Length % 16);
        for(; u8_i > 0; u8_i--)
        {                                                             
          pu8T_Buff[u8_Length++] = 0x00;
        }
      }       
    }      
    break;
    case  _XaSetDimainInfo://0x57,  //设置网络域名参数
    {
      pu8T_Buff[u8_Length++] = 0x00u;
      if((u8_Length % 16) != 0x00)//数据域长度不足16 需要补0
      {
        u8 u8_i = 16 - (u8_Length % 16);
        for(; u8_i > 0; u8_i--)
        {                                                             
          pu8T_Buff[u8_Length++] = 0x00;
        }
      }       
    }      
    break;
    case  _XaSetCycTime://0x49,     //设置采集上报周期
    {
      pu8T_Buff[u8_Length++] = 0x00u;
      if((u8_Length % 16) != 0x00)//数据域长度不足16 需要补0
      {
        u8 u8_i = 16 - (u8_Length % 16);
        for(; u8_i > 0; u8_i--)
        {                                                             
          pu8T_Buff[u8_Length++] = 0x00;
        }
      }       
    }       
    break;
    case  _XaReadCycTime://0x4A,    //读取采集上报周期
    {
      pu8T_Buff[u8_Length++] = 0x02;//只支持间隔上报
      
      x_MemFill(&pu8T_Buff[u8_Length], 0x00, 25);
      u8_Length             += 25;
      
      u16 u16_UploadTime = pdT_AppPrmt->u32_AutoUploadSec/60;
      pu8T_Buff[u8_Length++] = (u8)u16_UploadTime;//间隔时间
      pu8T_Buff[u8_Length++] = (u8)(u16_UploadTime>>8u);
      
      pu8T_Buff[u8_Length++] = pdT_AppPrmt->u8_UoloadTime[0];//首次上报时间
      pu8T_Buff[u8_Length++] = pdT_AppPrmt->u8_UoloadTime[1];
      
      pu8T_Buff[u8_Length++] = pdT_AppPrmt->u8_SampleTime[0];
      pu8T_Buff[u8_Length++] = pdT_AppPrmt->u8_SampleTime[1];
      if((u8_Length % 16) != 0x00)//数据域长度不足16 需要补0
      {
        u8 u8_i = 16 - (u8_Length % 16);
        for(; u8_i > 0; u8_i--)
        {                                                             
          pu8T_Buff[u8_Length++] = 0x00;
        }
      }
    }
    break;
    case  _XaSetRtcTime://0x05,     //校对时钟
    {
      pu8T_Buff[u8_Length++] = 0x00u;
      if((u8_Length % 16) != 0x00)//数据域长度不足16 需要补0
      {
        u8 u8_i = 16 - (u8_Length % 16);
        for(; u8_i > 0; u8_i--)
        {                                                             
          pu8T_Buff[u8_Length++] = 0x00;
        }
      }       
    }
    break;
    case  _XaSetAlarmLevel://0x58,  //设置报警浓度值
    {
      pu8T_Buff[u8_Length++] = 0x00u;
      if((u8_Length % 16) != 0x00)//数据域长度不足16 需要补0
      {
        u8 u8_i = 16 - (u8_Length % 16);
        for(; u8_i > 0; u8_i--)
        {                                                             
          pu8T_Buff[u8_Length++] = 0x00;
        }
      }       
    }
    break;
    case  _XaGetAlarmLevel://0x6A,  //读取报警浓度值
    {
      if(pdT_AppPrmt->u16_GasName==1u)
      {
        pu8T_Buff[u8_Length++] = 0x01;//甲烷
      }
      else if(pdT_AppPrmt->u16_GasName==0x0A)
      {
        pu8T_Buff[u8_Length++] = 0x03;//丙烷
      }
      else
      {
        pu8T_Buff[u8_Length++] = 0x01;//甲烷
      }
      
      pu8T_Buff[u8_Length++] = (u8)pdT_AppPrmt->f32_AlarmL ;
      pu8T_Buff[u8_Length++] = 0;
      pu8T_Buff[u8_Length++] = 0;
      pu8T_Buff[u8_Length++] = 0;
       
      pu8T_Buff[u8_Length++] = (u8)pdT_AppPrmt->f32_AlarmH;
      pu8T_Buff[u8_Length++] = 0;
      pu8T_Buff[u8_Length++] = 0;
      pu8T_Buff[u8_Length++] = 0;  
      
      if((u8_Length % 16) != 0x00)//数据域长度不足16 需要补0
      {
        u8 u8_i = 16 - (u8_Length % 16);
        for(; u8_i > 0; u8_i--)
        {                                                             
          pu8T_Buff[u8_Length++] = 0x00;
        }
      } 
    }
    break;
    case _XaAckEnd:
    {
      pu8T_Buff[u8_Length++] = 0x01u;
      if((u8_Length % 16) != 0x00)//数据域长度不足16 需要补0
      {
        u8 u8_i = 16 - (u8_Length % 16);
        for(; u8_i > 0; u8_i--)
        {                                                             
          pu8T_Buff[u8_Length++] = 0x00;
        }
      }  
    }
    break;
    
    default:
      break;
  }
    //填入数据长度
  u16 u16_AesLen = u8_Length;
  buff[u16_datalen] = (u8)u16_AesLen;//数据长度
  buff[u16_datalen+1] = (u8)(u16_AesLen >>8);
  
  if(dL_XaMsg.u8_KeyNo!=0u)//需要加密
  {
    //////////////////////////////////////////////////////////////////////////
    //	函数名：	MathAesEncryptECB
    //	描述：		加密数据，在该函数中重新计算各轮扩展密钥，并将密文结果赋值到明文中
    //	输入参数：	p_data	-- 待加密明文，即需加密的数据，其长度为nDataLen字节。
    //				data_len-- 数据长度，以字节为单位，必须为AES_KEY_LENGTH/8的整倍数。
    //				p_key	-- 密钥
    //				key_len	-- 密钥长度
    //	输出参数：	p_data	-- 输出密文，即由明文加密后的数据
    //	返回值：	无。
    ///////////////////////////////////////////////////////////////////////////
    if(dL_XaMsg.u8_KeyNo == 0x01)//用1号秘钥加密
    {
      //MathAesEncryptECB(pu8T_Buff, &u16_AesLen, u8L_Aeskey1, 16);
      u8 u8_n = (u8)(u16_AesLen / 16);
      u8 u8_AesBuff[16];
      for(u8 u8_m = 0; u8_m < u8_n; u8_m++)
      {
        aes_enc_dec(&pu8T_Buff[16 * u8_m], u8_AesBuff, u8L_Aeskey1, ENCRYPT);//加密
        x_MemCopy(&pu8T_Buff[16 * u8_m], u8_AesBuff, 16);
      }
    }
    else
    {
      u8 u8_n = (u8)(u16_AesLen / 16);
      u8 u8_AesBuff[16];
      for(u8 u8_m = 0; u8_m < u8_n; u8_m++)
      {
        aes_enc_dec(&pu8T_Buff[16 * u8_m], u8_AesBuff, pdT_AppPrmt->u8_Key[dL_XaMsg.u8_KeyNo-2], ENCRYPT);//加密
        x_MemCopy(&pu8T_Buff[16 * u8_m], u8_AesBuff, 16);
      }
    }
  }
  u16_len = u16_len+u8_Length+3u;
  u16 u16_Crc;
  tp_sec_f2_crc16(&buff[1], (u16_len -4), &u16_Crc);
  
  buff[u16_len - 3] = (u8)u16_Crc;
  buff[u16_len - 2] = (u8)(u16_Crc>>8); //填入校验值
  buff[u16_len - 1] = 0xED;//结束符  
  
  return u16_len;
}

//解析接收到的数据
static tn n_ProtocolRevData(void)
{
  tn tn_Result = _True;
  if((dL_UserMsgRev.u8_Buf[0] == 0x83) && (dL_UserMsgRev.u8_Buf[dL_UserMsgRev.u16_Len - 1] == 0xED))//检测起始符
  {
    if(dL_UserMsgRev.u8_Buf[1]==0xCC)
    {
      return _True;
    }
    //检查校验值
    u16 u16_Crc;
    tp_sec_f2_crc16(&dL_UserMsgRev.u8_Buf[1], (dL_UserMsgRev.u16_Len -4), &u16_Crc);
    
    if((((u16)dL_UserMsgRev.u8_Buf[dL_UserMsgRev.u16_Len - 2]<<8) | dL_UserMsgRev.u8_Buf[dL_UserMsgRev.u16_Len - 3]) == u16_Crc)//校验数据
    {     
        //获取秘钥号
        dL_XaMsg.u8_KeyNo = dL_UserMsgRev.u8_Buf[11];
        //获取命令码
        dL_XaMsg.MsgType = (SendMsgTypeDef)dL_UserMsgRev.u8_Buf[12];
        //获取数据长度
        u16 u16_DataLen = dL_UserMsgRev.u8_Buf[13];
        u8* pu8T_Buff = &dL_UserMsgRev.u8_Buf[15];//数据包起始地址

        AppPrmtDef* pdT_AppPrmt = NULL;
        pdT_AppPrmt   = x_GetAppPrmt();   //获取参数
        if(dL_XaMsg.u8_KeyNo != 0x00) //数据解密
        {
            //////////////////////////////////////////////////////////////////////////
            //	函数名：	MathAesDecryptECB
            //	描述：		解密数据
            //	输入参数：	p_data -- 待解密密文，即需解密的数据，其长度为nDataLen字节。
            //				data_len-- 数据长度，以字节为单位，必须为AES_KEY_LENGTH/8的整倍数。
            //				p_key	-- 密钥
            //				key_len	-- 密钥长度
            //	输出参数：	p_data  -- 输出明文，即由密文解密后的数据，可以与pCipherText相同。
            //	返回值：	无。
            //////////////////////////////////////////////////////////////////////////
            if(dL_XaMsg.u8_KeyNo == 0x01)
            {
              //用1号秘钥进行解密
              u8 u8_n = u16_DataLen / 16;
              u8 u8_AesBuff[16];
              for(u8 u8_m = 0; u8_m < u8_n; u8_m++)
              {
                aes_enc_dec(&pu8T_Buff[16 * u8_m], u8_AesBuff, u8L_Aeskey1, DECRYPT);
                x_MemCopy(&pu8T_Buff[16 * u8_m], u8_AesBuff, 16);
              }
              
            }
            else
            {
              u8 u8_n = (u8)(u16_DataLen / 16);
              u8 u8_AesBuff[16];
              for(u8 u8_m = 0; u8_m < u8_n; u8_m++)
              {
                aes_enc_dec(&pu8T_Buff[16 * u8_m], u8_AesBuff, pdT_AppPrmt->u8_Key[dL_XaMsg.u8_KeyNo-2], DECRYPT);
                x_MemCopy(&pu8T_Buff[16 * u8_m], u8_AesBuff, 16);
              }
            }      
        }
        switch(dL_XaMsg.MsgType)
        {
            case  _XaAesDown://0x08,        //密钥下发
            {      
              x_MemCopy(&pdT_AppPrmt->u8_Key[0][0],pu8T_Buff,128u);
              x_SaveAppPrmt((u8*)&pdT_AppPrmt->u8_Key,sizeof(pdT_AppPrmt->u8_Key));
              x_BlockingDelayMs(500); 
              n_SendMessage(_XaAesDown);
            }
            break;
            case  _XaSetCycTime://0x49,     //设置采集上报周期
            {           
             if(dL_UserMsgRev.u8_Buf[15]==0x02)
             {
                u16 u16_i = (((u16)dL_UserMsgRev.u8_Buf[42]<<8) | dL_UserMsgRev.u8_Buf[41]);
                if(u16_i==0u)  u16_i = 30u;
                pdT_AppPrmt->u32_AutoUploadSec = u16_i*60;//间隔上报时间
                x_SaveAppPrmt((u8*)&pdT_AppPrmt->u32_AutoUploadSec,sizeof(pdT_AppPrmt->u32_AutoUploadSec));
                pdT_AppPrmt->u8_UoloadTime[0] = dL_UserMsgRev.u8_Buf[43];//首次上报时间
                pdT_AppPrmt->u8_UoloadTime[1] = dL_UserMsgRev.u8_Buf[44]; 
                x_SaveAppPrmt((u8*)&pdT_AppPrmt->u8_UoloadTime,sizeof(pdT_AppPrmt->u8_UoloadTime));
                pdT_AppPrmt->u8_SampleTime[0] = dL_UserMsgRev.u8_Buf[45];//采集间隔时间
                pdT_AppPrmt->u8_SampleTime[1] = dL_UserMsgRev.u8_Buf[46];                 
                x_SaveAppPrmt((u8*)&pdT_AppPrmt->u8_SampleTime,sizeof(pdT_AppPrmt->u8_SampleTime));
                x_BlockingDelayMs(200); 
                n_SendMessage(_XaSetCycTime);
              } 
              else if(dL_UserMsgRev.u8_Buf[15] == 0x01)//定时上报  不作解析
              {
                dL_XaMsg.u8_ErrorType = 0x00u;
                n_SendMessage(_XaAnsError);
              }
            }       
            break;
            case  _XaSetRtcTime://0x05,     //校对时钟
            {      
              n_SendMessage(_XaSetRtcTime);
            }
            break;      
            break;
            case  _XaSetAlarmLevel://0x58,  //设置报警浓度值
            {      
              //u32 u32_Alarm_L = (u32)((dL_UserMsgRev.u8_Buf[16]<<24) | (dL_UserMsgRev.u8_Buf[17]<<16) | (dL_UserMsgRev.u8_Buf[18]<<8) | dL_UserMsgRev.u8_Buf[19]);
              //u32 u32_Alarm_H = (u32)((dL_UserMsgRev.u8_Buf[20]<<24) | (dL_UserMsgRev.u8_Buf[21]<<16) | (dL_UserMsgRev.u8_Buf[22]<<8) | dL_UserMsgRev.u8_Buf[23]);
              pdT_AppPrmt->f32_AlarmL = (f32)dL_UserMsgRev.u8_Buf[16];
              pdT_AppPrmt->f32_AlarmH = (f32)dL_UserMsgRev.u8_Buf[20];
              x_SaveAppPrmt((u8*)&pdT_AppPrmt->f32_AlarmL,sizeof(pdT_AppPrmt->f32_AlarmL));
              x_SaveAppPrmt((u8*)&pdT_AppPrmt->f32_AlarmH,sizeof(pdT_AppPrmt->f32_AlarmH));
              x_BlockingDelayMs(200); 
              n_SendMessage(_XaSetAlarmLevel);
            }
            break;             
            case  _XaGetIccid://=0x09,      //获取CCID号
            case  _XaReadCycTime://=0x4A,    //读取采集上报周期
            case  _XaGetAlarmLevel://=0x6A,  //读取报警浓度值
            {
              n_SendMessage(dL_XaMsg.MsgType);
            }
            break;
            case _XaSetIpInfo://=0x56,      //设置网络IP参数
            case _XaSetDimainInfo://=0x57,  //设置网络域名参数
            {
              dL_XaMsg.u8_ErrorType = 0x09u;//无效指令码
              n_SendMessage(_XaAnsError);
            }
            break;
            case _XaAckConfig:
            {
              n_SendMessage(_XaAckEnd);
            }
            break;
            default:
              break;
        }
    }
    else
    {
        dL_XaMsg.u8_ErrorType = 0x05u;//CRC错误
        n_SendMessage(_XaAnsError);
    }
  }
  else
  {
    if(dL_UserMsgRev.u8_Buf[0] == 0x83)
    {
      dL_XaMsg.u8_ErrorType = 0x02u;//帧尾错误
    }
    else
    {
      dL_XaMsg.u8_ErrorType = 0x01u;//帧头错误
    }
    n_SendMessage(_XaAnsError);
  }
  return tn_Result;
}
