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
void gprs_heart_process_thread_entry(void *parameters);
void send_gprs_mail(ALARM_TYPEDEF alarm_type, time_t time);

typedef struct {

  uint8_t key[4];
  uint8_t time[6];

}GPRS_LOCK_OPEN;

typedef struct {

  uint8_t power_status;
  uint8_t power;
  uint8_t time[6];

}GPRS_POWER_FAULT;

typedef struct {

  uint16_t type;
  uint8_t status;
  uint8_t time[6];

}GPRS_WORK_ALARM;

typedef struct {

  uint16_t type;
  uint8_t status;
  uint8_t time[6];

}GPRS_DEVICE_FAULT;

typedef struct {

  uint8_t type;

}GPRS_HEART;

typedef struct {

  uint8_t device_id[6];
  uint8_t enc_data[16];

}GPRS_AUTH;

typedef struct
{

  uint16_t length;
  uint8_t cmd;
  uint8_t order;

  union {

    GPRS_LOCK_OPEN lock_open;
    GPRS_POWER_FAULT power_fault;
    GPRS_WORK_ALARM work_alarm;
    GPRS_DEVICE_FAULT device_fault;
    GPRS_HEART heart;
    GPRS_AUTH auth;

  };

}GPRS_SEND_FRAME_TYPEDEF;

typedef struct
{

  uint16_t length;
  uint8_t cmd;
  uint8_t order;

}GPRS_RECV_FRAME_TYPEDEF;

int8_t send_gprs_frame(ALARM_TYPEDEF alarm_type, time_t time);

#endif
