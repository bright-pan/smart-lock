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
  //device id
  0xAAAA,
  //alarm telephone
  {
    {
      1,
      "13316975697"
    },
    {
      0,
      "13316975697"
    },
  },
  //call telephone
  {
    {
      1,
      "13316975697"
    },
    {
      0,
      "13316975697"
    },
  },
  // rfid key
  {
    {
      1,
      "\xAA\xAA\xAA\xAA"
    },
  }
};
