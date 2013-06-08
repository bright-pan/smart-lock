/*********************************************************************
 * Filename:      gprs.h
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-06 11:12:03
 *
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#ifndef _GPRS_H_
#define _GPRS_H_

#include <rthw.h>
#include <rtthread.h>
#include "stm32f10x.h"

#include "alarm.h"

#define GPRS_MAIL_MAX_MSGS 20

typedef struct
{
  time_t time;
  ALARM_TYPEDEF alarm_type;
}GPRS_MAIL_TYPEDEF;

extern rt_mq_t gprs_mq;

void gprs_mail_process_thread_entry(void *parameter);
void send_gprs_mail(ALARM_TYPEDEF alarm_type, time_t time);



typedef union
{
  struct {
    uint32_t key;
    uint8_t time[6];
  }lock_open;
  struct {
    uint8_t power_status;
    uint8_t power;
    uint8_t time[6];
  }power_error;
  struct {
    uint16_t type;
    uint8_t status;
    uint8_t time[6];
  }work_alarm;
  struct {
    uint16_t type;
    uint8_t status;
    uint8_t time[6];
  }device_fault;
  struct {
    uint8_t type;
  }heart;
}GPRS_SEND_DATA_TYPEDEF;

typedef struct
{
  uint16_t length;
  uint8_t command;
  uint8_t counts;
  GPRS_SEND_DATA_TYPEDEF data;
}GPRS_SEND_FRAME_TYPEDEF;

#endif
