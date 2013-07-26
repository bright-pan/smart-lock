/*********************************************************************
 * Filename:      gsm.h
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-22 09:29:50
 *
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#ifndef _GSM_H_
#define _GSM_H_

#include <rtthread.h>
#include <rthw.h>
#include <stm32f10x.h>

typedef enum {
  GSM_SETUP_ENABLE_SUCCESS,
  GSM_SETUP_ENABLE_FAILURE,
  GSM_SETUP_DISABLE_SUCCESS,
  GSM_SETUP_DISABLE_FAILURE,
  GSM_RESET_SUCCESS,
  GSM_RESET_FAILURE,
  GSM_SMS_SEND_SUCCESS,
  GSM_SMS_SEND_FAILURE,
}GsmStatus;

typedef enum
{
  AT = 0,//index=0
  AT_CNMI,
  AT_CSCA,
  AT_CMGF,
  AT_CPIN,
  AT_CSQ,//index=5
  AT_CGREG,
  AT_CGATT,
  AT_CIPMODE,
  AT_CSTT,
  AT_CIICR,//index=10
  AT_CIFSR,
  AT_CIPSHUT,
  AT_CIPSTATUS,
  AT_CIPSTART,
  AT_CMGS,//index=15
  AT_CMGS_SUFFIX,
  ATO,
  PLUS3,
  AT_CMMSINIT,
  AT_CMMSTERM,//index=20
  AT_CMMSCURL,
  AT_CMMSCID,
  AT_CMMSPROTO,
  AT_CMMSSENDCFG,
  AT_SAPBR_CONTYPE,//index=25
  AT_SAPBR_APN_CMWAP,
  AT_SAPBR_OPEN,
  AT_SAPBR_CLOSE,
  AT_SAPBR_REQUEST,
  AT_CMMSEDIT_OPEN,//index=30
  AT_CMMSEDIT_CLOSE,
  AT_CMMSDOWN_PIC,
  AT_CMMSDOWN_TITLE,
  AT_CMMSDOWN_TEXT,
  AT_CMMSDOWN_DATA,//index=35
  AT_CMMSRECP,
  AT_CMMSSEND,
  AT_CLCC,
  ATA,
  ATH5,//index=40
  AT_RING,
  AT_RECV_SMS,
  ATI,
  AT_GSV,
  AT_V,//index=45
  AT_D1,
  AT_W,
  AT_HTTPINIT,
  AT_HTTPTERM,
  AT_HTTPPARA_CID,//index=50
  AT_HTTPPARA_URL,
  AT_HTTPACTION_POST,
  AT_HTTPACTION_GET,
  AT_HTTPACTION_HEAD,
  AT_HTTPREAD,//index=55
  AT_SAPBR_APN_CMNET,
  AT_HTTPPARA_BREAK,
  AT_HTTPPARA_BREAKEND,
  AT_CIPSEND,
  AT_CIPSEND_SUFFIX,//index=60
  AT_CIPMUX1,
  AT_SIDET,
  AT_CMIC,
  AT_CLVL,
  AT_CHFA,//index=65
  AT_ECHO,

}AT_COMMAND_INDEX_TYPEDEF;

typedef enum
{

  EVENT_GSM_MODE_GPRS = 0x01,// AT
  EVENT_GSM_MODE_CMD = 0x02,
  EVENT_GSM_MODE_GPRS_CMD = 0x04,
  EVENT_GSM_MODE_SETUP = 0x08,

}EVENT_GSM_MODE_TYPEDEF;


typedef enum {

  GSM_MODE_CMD,
  GSM_MODE_GPRS,

}GSM_MODE_TYPEDEF;



typedef struct
{
  uint8_t *request;
  uint8_t *response;
  uint16_t request_length;
  uint32_t *response_length;
  uint8_t has_response;
}GSM_MAIL_GPRS;

typedef struct
{
  uint16_t length;
  uint8_t *buf;
}GSM_MAIL_CMD_CMGS;

typedef struct
{
  uint16_t length;
  uint8_t *buf;
}GSM_MAIL_CMD_CIPSEND;

typedef struct
{
  uint8_t *buf;
}GSM_MAIL_CMD_CMMSRECP;

typedef struct
{
  uint32_t length;
}GSM_MAIL_CMD_CMMSDOWN_PIC;

typedef struct
{
  uint32_t length;
}GSM_MAIL_CMD_CMMSDOWN_TITLE;

typedef struct
{
  uint32_t length;
}GSM_MAIL_CMD_CMMSDOWN_TEXT;

typedef struct
{
  uint32_t length;
  uint8_t *buf;
  uint8_t has_complete;
}GSM_MAIL_CMD_CMMSDOWN_DATA;

typedef struct
{
  uint32_t length;
  uint8_t *buf;
}GSM_MAIL_CMD_HTTPPARA_URL;
typedef struct
{
  uint32_t start;
  uint32_t *recv_counts;
  uint8_t *buf;
  uint32_t size_of_process;
}GSM_MAIL_CMD_HTTPREAD;

typedef struct
{
  uint32_t start;
}GSM_MAIL_CMD_HTTPPARA_BREAK;

typedef struct
{
  uint32_t end;
}GSM_MAIL_CMD_HTTPPARA_BREAKEND;

typedef union
{
  GSM_MAIL_CMD_CMGS cmgs;
  GSM_MAIL_CMD_CIPSEND cipsend;
  GSM_MAIL_CMD_CMMSRECP cmmsrecp;
  GSM_MAIL_CMD_CMMSDOWN_PIC cmmsdown_pic;
  GSM_MAIL_CMD_CMMSDOWN_TITLE cmmsdown_title;
  GSM_MAIL_CMD_CMMSDOWN_TEXT cmmsdown_text;
  GSM_MAIL_CMD_CMMSDOWN_DATA cmmsdown_data;
  GSM_MAIL_CMD_HTTPPARA_URL httppara_url;
  GSM_MAIL_CMD_HTTPREAD httpread;
  GSM_MAIL_CMD_HTTPPARA_BREAK httppara_break;
  GSM_MAIL_CMD_HTTPPARA_BREAKEND httppara_breakend;
}GSM_MAIL_CMD_DATA;

typedef struct
{
  AT_COMMAND_INDEX_TYPEDEF index;
  uint16_t delay;
  GSM_MAIL_CMD_DATA cmd_data;
}GSM_MAIL_CMD;

typedef union
{
  GSM_MAIL_CMD cmd;
  GSM_MAIL_GPRS gprs;
}GSM_MAIL_DATA;

typedef struct{

  GSM_MODE_TYPEDEF send_mode;
  GSM_MAIL_DATA mail_data;
  int8_t *result;
  rt_sem_t result_sem;

}GSM_MAIL_TYPEDEF;


typedef enum {

  AT_NO_RESPONSE,
  AT_RESPONSE_OK,
  AT_RESPONSE_ERROR,
  AT_RESPONSE_NO_CARRIER,
  AT_RESPONSE_TCP_CLOSED,
  AT_RESPONSE_CONNECT_OK,
  AT_RESPONSE_PDP_DEACT,
  AT_RESPONSE_PARTIAL_CONTENT,
  AT_RESPONSE_BAD_REQUEST,
  AT_RESPONSE_NO_MEMORY,

}AT_RESPONSE_TYPEDEF;

//typedef struct GSM_MAIL_TYPEDEF

GsmStatus gsm_reset(void);
GsmStatus gsm_setup(FunctionalState state);
void gsm_power(FunctionalState state);

void gsm_process_thread_entry(void *parameters);

rt_uint32_t gsm_mode_get(void);
void gsm_mode_set(rt_uint32_t mode);
void gsm_put_char(const uint8_t *str, uint16_t length);
void gsm_put_hex(const uint8_t *str, uint16_t length);

AT_RESPONSE_TYPEDEF send_cmd_mail(AT_COMMAND_INDEX_TYPEDEF command_index, uint16_t delay, GSM_MAIL_CMD_DATA *cmd_data);

extern char smsc[20];
extern char phone_call[20];
extern rt_mq_t mq_gsm;
extern rt_mutex_t mutex_gsm_mail_sequence;

#endif
