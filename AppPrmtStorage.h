
/******************************************************************************************
@copyright 2017-2018, 河南汉威电子股份有限公司
@file      AppPrmtStorage.h
@version   V1.0
@brief     Mini探测器应用程序,参数格式定义和统一存储管理
@warning:   
@remarks:  
 1.version V1.0    author 张晓男   date 2018.07.09
   modification 建立文档
*******************************************************************************************/

#ifndef _APP_PRMT_STORAGE__H_
#define _APP_PRMT_STORAGE__H_

#include "GlobalDefine.h"
#include "GasSensor.h"

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

typedef enum NetBandDef_
{
  _NetModeCtccB5   = 1U,      //电信B5   默认
  _NetModeCmccB8   = 2U,      //移动B8
  _NetModeCuccB3_8 = 4U,      //联通B3_8
}NetBandDef;

//IP地址和端口号
typedef struct NetIpPortDef_  //探测器搜索需要保存的信息
{
  u8 au8_Ip[30U];
  u8 au8_Port[5U];
}NetIpPortDef;

//APN
typedef struct NetApnDef_
{
  u8         au8_Apn[8U];
  u8         au8_ApnName[12U];
  u8         au8_ApnPassword[12U];
}NetApnDef;

typedef struct AppPrmtDef_        /*应用程序工作参数定义*/            
{
  u8            u8_SampleTime[2];      //采集周期
  u8            u8_UoloadTime[2];
  u8            u8_Key[8][16];//秘钥
  u32           u32_AutoUploadSec;   //自动上传周期(秒)
  NetIpPortDef  d_NetIpPort;         //IP地址
  NetApnDef     d_NetApn;            //APN
  NetBandDef    e_NetBand;           //运营商频段
  
  u32            u32_CumulTime;           /*工作时间累积,单位h*/
 
  f32            f32_AlarmL;              /*低报设置值*/
  f32            f32_AlarmH;              /*高报设置值*/
  
  u16            u16_InitFlag;          
  u16            u16_CommAddr;            /*通信地址*/
  
  u16            u16_ValveCheckDis;       /*置1禁用阀状态监测*/
  u16            u16_FuntionSwitch;       /*功能开关*/
  
//  u16            u16_SnrSentyAtten;       /*传感器灵敏度衰减*/       
//  u16            u16_SnrLifeTim;          /*传感器寿命*/
  
//  u16            u16_SnrGutePer;          /*传感器质保期*/
//  u16            u16_SnrModGutePer;       /*模组质保期*/         
//  
//  u32            u32_SnrManuDate;         /*传感器生产日期*/
//  
//  u16            u16_SnrManuFaty[6];      /*传感器生产厂家*/
//  u16            u16_SnrModManuFaty[6];   /*模组生产厂家*/
//  
  u32            u32_SnrModManuDate;      /*模组生产日期*/
//  
//  u16            u16_SnrModuleId[10];     /*模组序列号*/
//  u32            u32_SnrModuName;         /*模组名称*/
  
//  s32            s32_CalTempAd[6];        /*工厂标定多点温度AD值*/
//  f32            f32_CalTemp[6];          /*工厂标定多点温度值*/
  
  u16            u16_TempCalNum;          /*温度标定点数*/
  u16            u16_GasShildArryLen;     /*屏蔽数组的长度*/
  
//  f32            f32_GasShildArry[10];    /*气体屏蔽目标数组*/
  
  f32            f32_GasShildNegRange;    /*气体屏蔽负向范围*/
  f32            f32_GasShildPosRange;    /*气体屏蔽正向范围*/
  
//  u16            u16_Fill4;
//  u16            u16_AlarmDelayMs;        /*报警解除延时*/
  
  u16            u16_GasUnit;             /*气体单位*/
  u16            u16_GasName;             /*气体名称*/
  
  f32            f32_SnsrRange;           /*量程*/
  
  f32            f32_RatioLower;          /*AD/浓度比率下限  =0关闭检查,注意允许负值(负反应传感器)*/
  f32            f32_RatioUpper;          /*AD/浓度比率上限  =0关闭检查,注意允许负值(负反应传感器)*/
  
  u32            u32_AdLower;             /*故障AD下限*/
  u32            u32_AdUpper;             /*故障AD上限*/
  
//  u32            u32_CalDate;             /*标定日期*/
  
  f32            f32_SgCctrtCfft;         /*单点修正系数*/
  f32            f32_SgCctrt;             /*单点修正浓度*/
  
//  u32            u32_Fill3;        
//  u32            u32_Fill2; 
  
  u32            u32_ZeroAd;                            /*传感器零点AD值*/
  s32            s32_CalDeltaAd[GAS_SNSR_CAL_MAX_NUM];  /*工厂标定多点AD值*/
  f32            f32_CallCctrt[GAS_SNSR_CAL_MAX_NUM];   /*工厂标定多点浓度值*/
  
  u16            u16_CctrtCalNum;                       /*多浓度标定点数,范围1 - GAS_SNSR_CAL_MAX_NUM*/
  u16            u16_SnsrType;                          /*传感器类型*/    
  
//  u16            u16_Fill1;                       
//  u16            u16_Fill0;                       
  
  u16            u16_SoftVer;                           /*软件版本*/
  u16            u16_HardVer;                           /*硬件版本*/     
  u16            u16_ProtocolVer;                       /*协议版本号*/
  u16            u16_ProductType;                       /*产品类型*/                 
  
  u16            u16_DetectorId[10];                    /*产品唯一ID对应于产品上面的二维码*/
}AppPrmtDef;

/*
格式声明
====================================================================================================
外部函数
*/


/******************************************************************************************
@version V1.0      
@brief   获取整机参数内存地址,实际为可修改内存.
@param  
  void
@return   
  AppPrmtDef* 状态地址
@remarks      
 1.version V1.0    author 张晓男   date 2018.07.09
   modification    建立函数
*******************************************************************************************/
AppPrmtDef* x_GetAppPrmt(void);


/******************************************************************************************
@version V1.0      
@brief   初始化内存中的参数,从存储器中读出.参数使用之前必须先调用一次该函数
@param  
  void
@return   
  void
@remarks      
 1.version V1.0    author 张晓男   date 2018.07.09
   modification    建立函数
*******************************************************************************************/
void x_InitAppPrmt(void);


/******************************************************************************************
@version V1.0      
@brief   存储指定起始地址和长度的参数,存储时根据起始地址和内部的参数基地址计算存储器中的偏移量
@param  
  1.pu8I_Start  参数起始地址,必须以内存中参数为基地址
  2.u16I_Len    存储的长度
@return   
  void
@remarks      
 1.version V1.0    author 张晓男   date 2018.07.09
   modification    建立函数
*******************************************************************************************/
void x_SaveAppPrmt(u8* pu8I_Start, u16 u16I_Len);



#endif





