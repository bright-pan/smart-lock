/*********************************************************************
 * Filename:      untils.h
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-22 09:26:03
 *
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#ifndef _UNTILS_H_
#define _UNTILS_H_

#include <rtthread.h>
#include <rthw.h>
#include <stm32f10x.h>

void delay_us(uint32_t time);

#define TELEPHONE_NUMBERS 10
#define TELEPHONE_ADDRESS_LENGTH 20

#define RFID_KEY_NUMBERS 10

#define TCP_DOMAIN_LENGTH 20

typedef struct {

  uint8_t flag;
  char address[TELEPHONE_ADDRESS_LENGTH];
}TELEPHONE_ADDRESS_TYPEDEF;

typedef struct {

  uint8_t flag;
  uint32_t key;
}RFID_KEY_TYPEDEF;

typedef struct {
  char domain[TCP_DOMAIN_LENGTH];
  int32_t port;
}TCP_DEMAIN_TYPEDEF;

typedef struct {

  TELEPHONE_ADDRESS_TYPEDEF alarm_telephone[TELEPHONE_NUMBERS];
  TELEPHONE_ADDRESS_TYPEDEF call_telephone[TELEPHONE_NUMBERS];
  RFID_KEY_TYPEDEF rfid_key[RFID_KEY_NUMBERS];
  TCP_DEMAIN_TYPEDEF tcp_domain;
  uint8_t lock_gate_alarm_time;
  uint8_t device_id[6];
  uint8_t key0[8];
  uint8_t key1[8];

}DEVICE_PARAMETERS_TYPEDEF;

extern DEVICE_PARAMETERS_TYPEDEF device_parameters;

#endif
