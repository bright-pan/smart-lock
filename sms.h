/*********************************************************************
 * Filename:      sms.h
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-06 09:32:02
 *
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#ifndef _SMS_H_
#define _SMS_H_

#include <rthw.h>
#include <rtthread.h>
#include "stm32f10x.h"
#include "alarm.h"



/* PDU构造 */
#define	INTERNATIONAL_ADDRESS_TYPE		0x91
#define	LOCAL_ADDRESS_TYPE				0xA1

#define SMSC_LENGTH_DEFAULT 						0x08

#define FIRST_OCTET_DEFAULT				0x11
#define	TP_MR_DEFAULT						0x00
#define	TP_TYPE_DEFAULT					INTERNATIONAL_ADDRESS_TYPE//国际地址;
#define	TP_PID_DEFAULT					0x00//普通GSM 协议,点对点方式;
#define	TP_DCS_DEFAULT					0X08//UCS2编码方式;
#define	TP_VP_DEFAULT						0XC2//5分钟有效期限;

typedef struct {

  uint8_t SMSC_Length;//计算方式不同;
  uint8_t SMSC_Type_Of_Address;
  uint8_t SMSC_Address_Value[7];

}SMSC_TYPE;

typedef struct {

  uint8_t TP_OA_Length;//计算方式不同;
  uint8_t TP_OA_Type_Of_Address;
  uint8_t TP_OA_Address_Value[7];

}TP_OA_TYPE;

typedef struct {

  uint8_t TP_DA_Length;//计算方式不同;
  uint8_t TP_DA_Type_Of_Address;
  uint8_t TP_DA_Address_Value[7];

}TP_DA_TYPE;

typedef struct {

  uint8_t TP_SCTS_Year;//BCD;
  uint8_t TP_SCTS_Month;
  uint8_t TP_SCTS_Day;
  uint8_t TP_SCTS_Hour;
  uint8_t TP_SCTS_minute;
  uint8_t TP_SCTS_Second;
  uint8_t TP_SCTS_Time_Zone;

}TP_SCTS_TYPE;

typedef struct{

  uint8_t sms_head_length;
  uint8_t sms_laber_length;
  uint8_t sms_head_surplus_length;
  uint8_t sms_laber;
  uint8_t sms_numbers;
  uint8_t sms_index;

}SMS_HEAD_6;



typedef struct {

  uint8_t First_Octet;
  TP_OA_TYPE TP_OA;//9字节;
  uint8_t TP_PID;
  uint8_t TP_DCS;
  TP_SCTS_TYPE TP_SCTS;//7字节;
  uint8_t TP_UDL;//用户长度必须小于140 个字节;
  uint8_t TP_UD[140];

}SMS_RECEIVE_TPDU_TYPE;

typedef struct {

  uint8_t 	First_Octet;
  uint8_t	TP_MR;
  TP_DA_TYPE	TP_DA;//9字节;
  uint8_t 	TP_PID;
  uint8_t 	TP_DCS;
  uint8_t	TP_VP;// 1个字节;
  uint8_t	TP_UDL;//用户长度必须小于140个字节;
  uint8_t	TP_UD[420];

}SMS_SEND_TPDU_TYPE;

typedef struct {

  SMSC_TYPE SMSC;
  SMS_RECEIVE_TPDU_TYPE TPDU;

}SMS_RECEIVE_PDU_FRAME;

typedef struct {

  SMSC_TYPE SMSC;
  SMS_SEND_TPDU_TYPE TPDU;

}SMS_SEND_PDU_FRAME;




typedef struct
{
  time_t time;
  ALARM_TYPEDEF alarm_type;
}SMS_MAIL_TYPEDEF;

#define SMS_MAIL_MAX_MSGS 20

extern rt_mq_t sms_mq;

void sms_mail_process_thread_entry(void *parameter);

int8_t sms_pdu_ucs_send(char *dest_address, char *smsc_address, uint16_t *content, uint8_t length);

#endif
