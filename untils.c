/*********************************************************************
 * Filename:      untils.c
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-22 09:25:39
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "untils.h"

void delay_us(uint32_t time)
{
  uint8_t nCount;
  while(time--)
  {
    for(nCount = 6 ; nCount != 0; nCount--);
  }
}


DEVICE_PARAMETERS_TYPEDEF device_parameters = {
  //alarm telephone
  {
    {
      1,
      "8613316975697"
    },
    {
      1,
      "8613544033975"
    },
  },
  //call telephone
  {
    {
      1,
      "8613316975697"
    },
    {
      0,
      "8613316975697"
    },
  },
  // rfid key
  {
    {
      1,
      0xAAAAAA
    },
  },
  {
    "iyuet.com",
    6800
  },
  //lock gate alarm time
  30,
  //device id
  {0x00,},
  //key0
  {0x00,},
  //key1
  {0x00,},
};