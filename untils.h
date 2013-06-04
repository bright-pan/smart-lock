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

typedef struct {

  uint8_t flag;
  char address[TELEPHONE_ADDRESS_LENGTH];
}TELEPHONE_ADDRESS_TYPEDEF;

typedef struct {

  uint8_t flag;
  uint32_t key;
}RFID_KEY_TYPEDEF;

typedef struct {
  uint32_t device_id;
  TELEPHONE_ADDRESS_TYPEDEF alarm_telephone[TELEPHONE_NUMBERS];
  TELEPHONE_ADDRESS_TYPEDEF call_telephone[TELEPHONE_NUMBERS];
  RFID_KEY_TYPEDEF rfid_key[RFID_KEY_NUMBERS];
  
}DEVICE_PARAMETERS_TYPEDEF;

extern DEVICE_PARAMETERS_TYPEDEF device_parameters;

#endif
