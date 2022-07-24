/******************************************************************************************
@copyright 2017-2018, 汉威科技集团股份有限公司
@file      AppPrmtStorage.c
@version   V1.0
@brief     参数存储管理
@warning:  None
@remarks:  None
1.version V1.0    author waf   date 2021.02.10
modification 建立文档
*******************************************************************************************/

#include "AppPrmtStorage.h"
#include "ParameterManage.h"
#include "AppCoreLogic.h"

/*
文件引用
====================================================================================================
宏定义
*/
//程序为新奥燃气定制程序
#define SOFTWARE_VERSION (0x0110) //当前软件版本号定义
#define HARDWARE_VERSION (0x0100) //适配硬件版本号定义
#define PROTOCOL_VERSION (0x0200) //适配硬件版本号定义

#define PRMT_INIT_KEY    (0x39C9) //参数初始化判断关键字

/*
宏定义 
====================================================================================================
格式声明
*/

static AppPrmtDef  dL_Prmt =   /*App参数内存定义*/
{
  .u8_SampleTime        = {0x1E,0},
  .u8_UoloadTime        = {0,0},
  .u32_AutoUploadSec    = 86400,
  .d_NetIpPort          = 
  {
//    "117.60.157.137",
//    "5683",
      "221.229.214.202",   //电信AEP平台LwM2M(UDP/CoAP非加密接入)
      "5683",
//    "tcp2.xiangyuniot.com",  //tcp
//    "50000",
  },
  .e_NetBand = _NetModeCtccB5,
  
  .u32_CumulTime        = 0U,                 /*工作时间累积,单位h*/
  
  .f32_AlarmL           = 20.0F,              /*低报设置值*/
  .f32_AlarmH           = 50.0F,              /*高报设置值*/
  
  .u16_InitFlag         = PRMT_INIT_KEY,
  .u16_CommAddr         = 1U,                 /*通信地址*/
 
  .u16_ValveCheckDis    = 1U,      
  .u16_FuntionSwitch    = 0x007FU,                           /*功能开关*/    
  
//  .u16_SnrSentyAtten    = 0U,                                /*传感器灵敏度衰减*/
//  .u16_SnrLifeTim       = 3U,                                /*传感器寿命*/
//  
//  .u16_SnrGutePer       = 1U,                                /*传感器质保期*/
//  .u16_SnrModGutePer    = 1U,                                /*模组质保期*/
//  
//  .u32_SnrManuDate      = 0U,                                /*传感器生产日期*/
  
//  .u16_SnrManuFaty      = {0U,0U,0U,0U,0U,0U},               /*传感器生产厂家*/
//  .u16_SnrModManuFaty   = {0U,0U,0U,0U,0U,0U},               /*模组生产厂家*/
// 
  .u32_SnrModManuDate   = 0U,                                /*模组生产日期*/
  
//  .u16_SnrModuleId      = {0U,0U,0U,0U,0U,0U,0U,0U,0U,0U},   /*模组序列号*/
//  .u32_SnrModuName      = 0U,                                /*模组名称*/
  
  //.s32_CalTempAd        = {0,0,0,0,0,0},                     /*工厂标定多点温度AD值*/
  //.f32_CalTemp          = {0.0F,0.0F,0.0F,0.0F,0.0F,0.0F},   /*工厂标定多点温度值*/
   
//  .u16_TempCalNum       = 0U,          /*温度标定点数*/
//  .u16_GasShildArryLen  = 0U,          /*屏蔽数组的长度*/
  
  //.f32_GasShildArry     = {0.0F,0.0F,0.0F,0.0F,0.0F,0.0F,0.0F,0.0F,0.0F,0.0F}, /*气体屏蔽目标数组*/
  
  .f32_GasShildNegRange = 0.0F,        /*气体屏蔽负向范围*/
  .f32_GasShildPosRange = 3.0F,        /*气体屏蔽正向范围*/
  
//  .u16_Fill4            = 0U,
//  .u16_AlarmDelayMs     = 0U,          /*报警解除延时*/
  
  .u16_GasUnit          = _GasUnitLel,  /*单位*/
  .u16_GasName          = 1U,          /*气体名称*/         
  
  .f32_SnsrRange        = 100.0F,      /*量程*/
  
  .f32_RatioLower       = 1.0F,     /*AD/浓度比率下限  =0关闭检查,注意允许负值(负反应传感器)*/
  .f32_RatioUpper       = 400.0F,    /*AD/浓度比率上限  =0关闭检查,注意允许负值(负反应传感器)*/
  
  .u32_AdLower          = 100U,        /*故障AD下限*/
  .u32_AdUpper          = 4095U,      /*故障AD上限*/

//  .u32_CalDate          = 0U,          /*标定日期*/
      
  .f32_SgCctrtCfft      = 1.0F,        /*单点修正系数*/
  .f32_SgCctrt          = 50.0F,       /*单点修正浓度*/
  
//  .u32_Fill3            = 0U,    
//  .u32_Fill2            = 0U, 
  
  .u32_ZeroAd           = 652U,  /*零点AD1918U*/ 
  
  .s32_CalDeltaAd       = {400U, 800U, 1200U, 1600U, 2000U},   /*工厂标定多点AD值*/
  .f32_CallCctrt        = {20.0F, 40.0F, 50.0F, 60.0F, 80.0F},    /*工厂标定多点浓度值*/
  
  .u16_CctrtCalNum      = 5U,                               /*多浓度标定点数,范围1 - GAS_SNSR_CAL_MAX_NUM*/
  .u16_SnsrType         = _GasSnsrTypeCc,                    /*传感器类型*/
  
//  .u16_Fill1            = 0U,            
//  .u16_Fill0            = 0U,            
  
  .u16_SoftVer          = SOFTWARE_VERSION,                 /*软件版本*/  
  .u16_HardVer          = HARDWARE_VERSION,                 /*硬件版本*/ 
  
  .u16_ProtocolVer      = PROTOCOL_VERSION,                 /*协议版本号*/
  .u16_ProductType      = 0U,                               /*产品类型*/                 
  
  .u16_DetectorId       = {0U,0U,0U,0U,0U,0U,0U,0U,0x0102U,0x0304U},  /*产品唯一ID号*/
};

/*
格式声明
====================================================================================================
函数声明
*/

static const FlashStorageAbstDef* n_GetStorage(void);  //函数形式声明.
static void n_SendResult(PrmtManageRsltDef eI_Rslt); //函数形式声明.PrmtManageResultDef, u16 u16I_Start, u16 u16I_Len
                   
static const PrmtManageObjectDef cdL_PrmtObject = 
{
  n_GetStorage,
  n_SendResult,
};
/*
函数声明
====================================================================================================
外部变量
*/


/*
外部变量
====================================================================================================
变量定义
*/


/*
变量定义
====================================================================================================
内部函数
*/

/******************************************************************** 
函 数 名：static const FlashStorageAbstDef* n_GetStorage(void)
功    能：获取物理存储器地址
说    明: 
入口参数：pdO_Data:传感器数据输出
返 回 值：无
设    计：waf                    日    期：2021-01-21
修    改：                       日    期： 
*********************************************************************/
static const FlashStorageAbstDef* n_GetStorage(void)
{
  return &cdG_FlashStorageAbst;
}

/******************************************************************** 
函 数 名：static void n_SendResult(PrmtManageRsltDef eI_Rslt)
功    能：发生错误后的处理
说    明: 
入口参数：pdO_Data:传感器数据输出
返 回 值：无
设    计：waf                    日    期：2021-01-21
修    改：                       日    期： 
*********************************************************************/
static void n_SendResult(PrmtManageRsltDef eI_Rslt)
{
  if ((eI_Rslt == _PrmtManageReadDone)||(eI_Rslt == _PrmtManageWriteDone))
  {
    x_CoreHookPrmtError(false); 
  }
  else
  {
    x_CoreHookPrmtError(true);
  }
}

/*
内部函数
====================================================================================================
外部函数
*/

/*********************************************************************************************************
@brief 获取整机参数地址
**********************************************************************************************************/
AppPrmtDef* x_GetAppPrmt(void)
{
  return &dL_Prmt;
}

/******************************************************************************************   
@brief   初始化内存中的参数,从存储器中读出
*******************************************************************************************/
void x_InitAppPrmt(void)
{
  const FlashStorageAbstDef* pcdT_FlashAbst = n_GetStorage();
  
  u16 u16T_ParameterLen = sizeof(dL_Prmt);
  
  pcdT_FlashAbst->pf_Config(_FlashStorageCfgReadWriteLine);   //配置FLASH
  
  x_PrmtManageRead((u8*)(&dL_Prmt.u16_InitFlag), (u8*)(&dL_Prmt.u16_InitFlag)-(u8*)(&dL_Prmt),2U, &cdL_PrmtObject);//读出初始化标志存储值

  if(dL_Prmt.u16_InitFlag == PRMT_INIT_KEY)
  {
    x_PrmtManageRead((u8*)&dL_Prmt,0U,u16T_ParameterLen,&cdL_PrmtObject);  //全部读出
  }
  else
  {
    dL_Prmt.u16_InitFlag = PRMT_INIT_KEY;
    
    x_PrmtManageWrite(0U, (u8*)&dL_Prmt,u16T_ParameterLen,&cdL_PrmtObject);  //全部写入
  }
}

/******************************************************************************************    
@brief   存储指定起始地址和长度的参数,存储时根据起始地址和内部的参数基地址计算存储器中的偏移量
*******************************************************************************************/
void x_SaveAppPrmt(u8* pu8I_Start, u16 u16I_Len)
{ 
  x_PrmtManageWrite(pu8I_Start - ((u8*)&dL_Prmt),pu8I_Start,u16I_Len,&cdL_PrmtObject);  
}
