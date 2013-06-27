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

extern rt_mutex_t mutex_gsm_mode;
extern rt_event_t event_gsm_mode_request;
extern rt_event_t event_gsm_mode_response;


typedef enum
{
  AT = 0,// AT
  AT_CNMI,
  AT_CSCA,
  AT_CMGF,
  AT_CPIN,
  AT_CSQ,
  AT_CGREG,
  AT_CGATT,
  AT_CIPMODE,
  AT_CSTT,
  AT_CIICR,
  AT_CIFSR,
  AT_CIPSHUT,
  AT_CIPSTATUS,
  AT_CIPSTART,
  ATO,
  PLUS3,
}AT_COMMAND_INDEX_TYPEDEF;

typedef enum
{

  EVENT_GSM_MODE_GPRS = 0x01,// AT
  EVENT_GSM_MODE_CMD = 0x02,
  EVENT_GSM_MODE_GPRS_CMD = 0x04,
  EVENT_GSM_MODE_SETUP = 0x08,

}EVENT_GSM_MODE_TYPEDEF;


typedef enum {

  AT_NO_RESPONSE,
  AT_RESPONSE_OK,
  AT_RESPONSE_ERROR,
  AT_RESPONSE_NO_CARRIER,
  AT_RESPONSE_TCP_CLOSED,
  AT_RESPONSE_CONNECT_OK,
  AT_RESPONSE_PDP_DEACT,

}AT_RESPONSE_TYPEDEF;

GsmStatus gsm_reset(void);
GsmStatus gsm_setup(FunctionalState state);
void gsm_power(FunctionalState state);

void gsm_process_thread_entry(void *parameters);

rt_uint32_t gsm_mode_get(void);
void gsm_mode_set(rt_uint32_t mode);

extern char smsc[20];

#endif
