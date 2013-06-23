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
#include "untils.h"

#define GPRS_MAIL_MAX_MSGS 20

typedef struct
{

  time_t time;
  ALARM_TYPEDEF alarm_type;
  uint8_t order;
}GPRS_MAIL_TYPEDEF;

extern rt_mq_t gprs_mq;

void gprs_mail_process_thread_entry(void *parameter);
void gprs_heart_process_thread_entry(void *parameters);
void send_gprs_mail(ALARM_TYPEDEF alarm_type, time_t time, uint8_t order);

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
  uint8_t time[6];

}GPRS_FAULT_ALARM;

typedef struct {

  uint8_t type;

}GPRS_HEART;

typedef struct {

  uint8_t device_id[6];
  uint8_t enc_data[16];

}GPRS_AUTH;

typedef struct
{

  uint8_t result;
  uint8_t counts;
  uint8_t telephone[TELEPHONE_NUMBERS][11];

}GPRS_SEND_LIST_TELEPHONE;

typedef struct
{

  uint8_t result;

}GPRS_SEND_SET_TELEPHONE;

typedef struct
{

  uint8_t result;
  uint8_t counts;
  uint8_t key[RFID_KEY_NUMBERS][4];

}GPRS_SEND_LIST_RFID_KEY;

typedef struct
{

  uint8_t result;
  uint8_t parameters;

}GPRS_SEND_LIST_USER_PARAMETERS;

typedef struct
{

  uint8_t result;

}GPRS_SEND_SET_USER_PARAMETERS;

typedef struct
{

  uint8_t result;

}GPRS_SEND_SET_RFID_KEY;

typedef struct
{

  uint16_t length;
  uint8_t cmd;
  uint8_t order;

  union {

    GPRS_LOCK_OPEN lock_open;
    GPRS_POWER_FAULT power_fault;
    GPRS_WORK_ALARM work_alarm;
    GPRS_FAULT_ALARM fault_alarm;
    GPRS_HEART heart;
    GPRS_AUTH auth;
    GPRS_SEND_LIST_TELEPHONE list_telephone;
    GPRS_SEND_SET_TELEPHONE set_telephone;
    GPRS_SEND_LIST_RFID_KEY list_rfid_key;
    GPRS_SEND_SET_RFID_KEY set_rfid_key;
    GPRS_SEND_LIST_USER_PARAMETERS list_user_parameters;
    GPRS_SEND_SET_USER_PARAMETERS set_user_parameters;
  };

}GPRS_SEND_FRAME_TYPEDEF;

typedef struct
{

  uint8_t type;

}GPRS_LIST_TELEPHONE;

typedef struct
{
  uint8_t type;

}GPRS_LIST_RFID_KEY;

typedef struct
{
  uint8_t type;

}GPRS_LIST_USER_PARAMETERS;

typedef struct
{
  uint8_t parameters;

}GPRS_SET_USER_PARAMETERS;

typedef struct
{
  uint8_t counts;
  uint8_t telephone[TELEPHONE_NUMBERS][11];

}GPRS_SET_TELEPHONE;

typedef struct
{
  uint8_t counts;
  uint8_t key[RFID_KEY_NUMBERS][4];

}GPRS_SET_RFID_KEY;

typedef struct
{

  uint16_t length;
  uint8_t cmd;
  uint8_t order;
  union {

    GPRS_LIST_TELEPHONE list_telephone;
    GPRS_SET_TELEPHONE set_telephone;
    GPRS_LIST_RFID_KEY list_rfid_key;
    GPRS_SET_RFID_KEY set_rfid_key;
    GPRS_LIST_USER_PARAMETERS list_user_parameters;
    GPRS_SET_USER_PARAMETERS set_user_parameters;
  };

}GPRS_RECV_FRAME_TYPEDEF;

int8_t send_gprs_frame(ALARM_TYPEDEF alarm_type, time_t time, uint8_t order);

#endif
