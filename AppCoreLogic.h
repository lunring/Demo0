
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

#ifndef _APP_CORE_LOGIC__H_
#define _APP_CORE_LOGIC__H_

#include "GlobalDefine.h"
#include "GasSensorTask.h"
#include "Ir05Module.h"

/*
文件引用
====================================================================================================
宏定义
*/

/*
宏定义
====================================================================================================
格式声明
*/

extern u8 u8L_Alarm_flag;
extern u8 u8L_Error_flag;
extern u8 u8L_Loss_flag;
extern u8 u8L_Period_flag;
extern u8 u8L_Heart_flag;

typedef enum CoreAcceptMsgDef_  //应用程序核心接受的外部信息定义
{
  _CoreInfPrmtError = 0,   //参数错误,true产生,false解除,下同
  _CoreInfSnsrError = 1,   //传感器错误
  _CoreInfAlarmH    = 2,   //高报
  _CoreInfAlarmL    = 3,   //低报
  _CoreInfHiPrtct   = 4,   //高浓度保护
  _CoreInfOverLoad  = 5,   //超量程
  _CoreInfSecond    = 6,   //秒计时,true秒间隔到来, false要求结束预热
}CoreAcceptMsgDef;

typedef enum CoreCmdDef_   //应用程序核心接受的外部命令名定义
{
  _CoreCmdSetAlarmL  = _Ir05CmdSetAlarmL,  //设置低报,按照IR05枚举值定义
  _CoreCmdSetAlarmH  = _Ir05CmdSetAlarmH,  //设置高报
  _CoreCmdZeroMove   = _Ir05CmdZeroMove,   //0点平移
  _CoreCmdCalSingle  = _Ir05CmdCalSingle,  //单点校准
  _CoreCmdSetAddr    = _Ir05CmdSetAddr,    //设置通信地址,仅红外使用
  _CoreCmdCalCctrt0  = _Ir05CmdCalCctrt0,  //标定0点
  _CoreCmdCalCctrt1  = _Ir05CmdCalCctrt1,  //标定浓度1
  _CoreCmdCalCctrt2  = _Ir05CmdCalCctrt2,  //标定浓度2
  _CoreCmdCalCctrt3  = _Ir05CmdCalCctrt3,  //标定浓度3
  _CoreCmdCalCctrt4  = _Ir05CmdCalCctrt4,  //标定浓度4
  _CoreCmdCalCctrt5  = _Ir05CmdCalCctrt5,  //标定浓度5
  _CoreCmdCal4mA     = _Ir05CmdCal4mA,     //校准4mA
  _CoreCmdCal20mA    = _Ir05CmdCal20mA,    //校准20mA
  _CoreCmdStopWarmup = _Ir05CmdStopWarmup, //跳过预热,仅红外支持
}CoreCmdDef;

typedef enum ValveFlagDef_    //阀(机械手)状态标志
{
  _ValveIniting    = 0U,     //初始化中.   注意!该状态时设置关阀无效
  _ValveActing     = 1U,     //关阀动作中. 注意!该状态时设置关阀无效
  _ValveStop       = 2U,     //未动作且检测功能被禁用
  _ValveChecking   = 3U,     //状态检测中
  _ValveOpen       = 4U,     //检测到阀打开. 注意!初始化运行时,默认阀为此状态
  _ValveClose      = 5U,     //检测到阀关闭
  _ValveFaultJam   = 6U,     //检测到阀故障-关闭卡阻
  _ValveFaultLost  = 7U,     //检测到阀故障-未接阀
  _ValveFaultShort = 8U,     //检测到阀故障-阀短路
}ValveFlagDef;
  
typedef struct CoreAcceptCmdDef_ //应用程序核心接受的外部完整命令定义
{
  CoreCmdDef e_Cmd;     //命令
  union
  {
    f32   f32_Prmt;   //命令参数 f32  格式
    u16   u16_Prmt;   //命令参数 u32_8格式
  };
}CoreAcceptCmdDef;


typedef struct CoreStateGroupDef_ //应用程序核心状态集定义
{
  u8  u8_CheckingCd; //自检倒计时, =0结束
  u8  u8_WarmupCd;   //预热倒计时, =0结束
  
  union 
  {
    u8 u8_All;
    struct 
    {
      u8 s_Prmt :1;      //参数,  非0有效
      u8 s_Snsr :1;     //传感器
      u8 s_Cal  :1;     //标定故障
    };
  }d_Fault; //故障
  
  union 
  {
    u8 u8_All;
    struct 
    {
      u8 s_Hi :1; //高报
      u8 s_Lo :1; //低报
    };
  }d_Alarm;       //报警
  
  union 
  {
    u8 u8_All;
    struct 
    {
      u8 s_Hp :1; //高浓度保护
      u8 s_Ol :1; //超量程
    };
  }d_Abnormal;
  
  union                       //增加上传标志
  {
    u8 u8_All;
    struct 
    {
      u8 WarmUp_Flag :1;    //预热上传标志
      u8 Normal_Flag :1;    //正常上传标志
      u8 Period_Flag :1;   //周期上传标志 
      u8 NetWork_Flag :1;  //开始入网标志
      u8 LossPowr_Flag :1; //掉电上传标志
      u8 PulseValve_Flag:1; //脉冲阀上传标志
    };
  }d_Upload;
  
  u32               u32_GasAd; //浓度AD值
  f32               f32_Cctrt; //气体浓度
  u8                u8_DectorDispPrmtState; 
  u8                u8_LossPowr;  //掉电
  ValveFlagDef      d_Valve;            //电磁阀检测状态
  u8                au8_NetIMEI[15U];  //国际移动设备识别码
  u8                au8_SimICCID[20U]; //SIM卡识别码
  tn                tn_Upload;         //_True上传成功, _Unknown配网或连接中, _False断网或上传失败 
  u8                u8_SoundFlag;      //消音标志
  u32               u32_UpLoadSec;    //上传周期
}CoreStateGroupDef;

/*
格式声明
====================================================================================================
外部函数
*/

/******************************************************************************************
@version V1.0      
@brief   核心获取参数错误消息的钩子函数
@param  
  1.blI_Error       true=发生错误, false=错误解除
@return   
  void
@remarks      
 1.version V1.0    author 张晓男   date 2018.07.16
   modification    建立函数
*******************************************************************************************/
void x_CoreHookPrmtError(bl blI_Error);


/******************************************************************************************
@version V1.0      
@brief   核心获取气体传感器浓度和监测结果的钩子函数
@param  
  1.pcdI_ConvRslt    浓度相关输出结果
  2.pcdI_MonitorRslt 监测相关输出结果
@return   
  void
@remarks      
 1.version V1.0    author 张晓男   date 2018.07.16
   modification    建立函数
*******************************************************************************************/
void x_CoreHookGasSnsrRslt(const GasSnsrOutputDef* pcdI_SnsrOutput);


/******************************************************************************************
@version V1.0      
@brief   设置核心接受的外部命令,注意通信命令不支持跳过预热
@param  
  1.pcdI_Cmd     外部控制命令
  2.ptnO_Rslt    执行结果,=NULL为红外命令.!=NULL为通信命令,此时_True=成功,_Unknown=不支持的命令,_False=参数错误
@return   
  void
@remarks      
 1.version V1.0    author 张晓男   date 2018.07.16
   modification    建立函数
*******************************************************************************************/
void x_SetCoreAcceptCmd(const CoreAcceptCmdDef* pcdI_Cmd, tn* ptnO_Rslt);

/******************************************************************************************
@version V1.0      
@brief   获取应用程序核心状态集
@param  
  void
@return   
  const AppStateGroupDef*  应用程序核心状态集
@remarks      
 1.version V1.0    author 张晓男   date 2018.07.16
   modification    建立函数
*******************************************************************************************/
CoreStateGroupDef* x_GetCoreStateGroup(void);

/******************************************************************************************
@version V1.0      
@brief   向核心传送阀状态 由 DeviceDriveTask.c 调用
@param  
  void
@return   
  const AppStateGroupDef*  应用程序核心状态集
@remarks      
 1.version V1.0    author 张晓男   date 2018.07.16
   modification    建立函数
*******************************************************************************************/
void x_SendValveStateCore(ValveFlagDef eI_State);

/******************************************************************************************
@version V1.0      
@brief   向核心传送网络上传状态及相关设备信息 由 UploadCommTask.c 调用
@param  
  void
@return   
  const AppStateGroupDef*  应用程序核心状态集
@remarks      
 1.version V1.0    author 张晓男   date 2018.07.16
   modification    建立函数
*******************************************************************************************/
void x_SendUploadStateCore(tn tnI_State, const u8* pcu8I_NetIMEI, const u8* pcu8I_SimICCID);

#endif
