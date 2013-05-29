/*********************************************************************
 * Filename:      gsm.c
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-22 09:17:52
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "gsm.h"
#include "gpio_pin.h"
#include "gsm_usart.h"
#include "untils.h"
#include "board.h"
#include <string.h>
#include "slre.h"
#include <stdio.h>

#define GSM_SEND_MAX_MSGS 20
#define GSM_RECV_MAX_MSGS 20

#define RECV_BUF_SIZE 256

rt_mq_t gsm_send_mq;
rt_mq_t gsm_recv_mq;

rt_event_t event_gsm_mode;

const char *at_command[] =
{
  "AT\r",
  "AT+CNMI=2,1\r",
  "AT+CSCA?\r",
};

void gsm_put_char(const char *str, uint16_t length)
{
  uint16_t index = 0;
  while (index < length)
  {
    if (str[index] != '\r')
    {
      rt_kprintf("%c", str[index++]);
    }
    else
    {
      index++;
    }
  }
  rt_kprintf("\n");
}

void gsm_put_hex(const char *str, uint16_t length)
{
  uint16_t index = 0;
  while (index < length)
  {
    rt_kprintf("%02X ", str[index++]);
  }
  rt_kprintf("\n");
}

ATCommandStatus gsm_recv_setup(void)
{
  char *recv_buf;
  char *temp;
  rt_device_t device_gsm_usart = rt_device_find(DEVICE_NAME_GSM_USART);
  uint16_t recv_size;
  char match_error;
  
  if (device_gsm_usart != RT_NULL)
  {
    recv_buf = (char *)rt_malloc(RECV_BUF_SIZE);
    if (recv_buf == RT_NULL)
    {
      rt_kprintf("no more memory can use!!!\n");
      return AT_NO_RESPONSE;
    }
    memset(recv_buf, '\0', RECV_BUF_SIZE);
    recv_size = rt_device_read(device_gsm_usart, 0, recv_buf, RECV_BUF_SIZE);
    if (recv_size <= 0)
    {
      rt_free(recv_buf);
      rt_kprintf("gsm recv is not exist!\n");
      return AT_NO_RESPONSE;
    }
    else
    {
      rt_kprintf("gsm recv : ");
      gsm_put_char(recv_buf, recv_size);
      gsm_put_hex(recv_buf, recv_size);
      /* parse at response */
      temp = (char *)rt_malloc(RECV_BUF_SIZE);
      memset(temp, '\0', RECV_BUF_SIZE);
      match_error = sscanf(recv_buf, "%*[^+]+CPIN: %[^+]", temp);
      gsm_put_char(temp, strlen(temp));
      rt_free(temp);
      rt_free(recv_buf);
      if (match_error < 0)
      {
        rt_kprintf("Error parsing\n");
        return AT_ERROR;
      }
      else
      {
        rt_kprintf("Success parsing\n");
        return AT_OK;
      }
    }
  }
  else
  {
    rt_kprintf("\ngsm usart device is not exist !\n");
    return AT_NO_RESPONSE;
  }
}


/*
 * gsm power control, ENABLE/DISABLE
 */
void gsm_power(FunctionalState state)
{
  rt_int8_t dat = 0;
  rt_device_t device_gsm_power = rt_device_find(DEVICE_NAME_GSM_POWER);
  
  if (device_gsm_power == RT_NULL)
  {
    rt_kprintf("device %s is not exist!\n", DEVICE_NAME_GSM_POWER);
  }
  else
  {
    if(state == ENABLE)
    {
      dat = 1;
      rt_device_write(device_gsm_power, 0, &dat, 0);
    }
    else if(state == DISABLE)
    {
      dat = 0;
      rt_device_write(device_gsm_power, 0, &dat, 0);
    }
  }
}

GsmStatus gsm_setup(FunctionalState state)
{
  rt_int8_t dat = 0;
  rt_device_t device_gsm_status = rt_device_find(DEVICE_NAME_GSM_STATUS);

  if (device_gsm_status == RT_NULL)
  {
    rt_kprintf("\ndevice %s is not exist!\n", DEVICE_NAME_GSM_STATUS);
    if (state == ENABLE)
    {
      return GSM_SETUP_ENABLE_FAILURE;
    }
    else
    {
      return GSM_SETUP_DISABLE_FAILURE;
    }
  }
  else
  {
    if (state == ENABLE) // gsm setup
    {
      rt_device_read(device_gsm_status, 0, &dat, 0);
      if (dat == 0) // gsm status == 0
      {
        gsm_power(ENABLE);
        rt_thread_delay(100);
        gsm_power(DISABLE);
        rt_thread_delay(350);
        rt_device_read(device_gsm_status, 0, &dat, 0);
        if (dat != 0)
        {
          rt_kprintf("\nthe gsm device is setup!\n");
          gsm_recv_setup();
          return GSM_SETUP_ENABLE_SUCCESS;
        }
        else
        {
          rt_kprintf("\nthe gsm device can not setup! please try again.\n");
          return GSM_SETUP_ENABLE_FAILURE;
        }
      }
      else
      {
        // do nothing
        rt_kprintf("\nthe gsm device has been setup!\n");
        return GSM_SETUP_ENABLE_SUCCESS;
      }
    }
    else if(state == DISABLE) // gsm close
    {
      rt_device_read(device_gsm_status, 0, &dat, 0);
      if (dat != 0) // gsm status != 0
      {
        gsm_power(ENABLE);
        rt_thread_delay(100);
        gsm_power(DISABLE);
        rt_thread_delay(250);
        rt_device_read(device_gsm_status, 0, &dat, 0);
        if (dat == 0)
        {
          rt_kprintf("\nthe gsm device is close!\n");
          return GSM_SETUP_DISABLE_SUCCESS;
        }
        else
        {
          rt_kprintf("\nthe gsm device can not close! please try again.\n");
          return GSM_SETUP_DISABLE_FAILURE;
        }
        
      }
      else
      {
        // do nothing
        rt_kprintf("\nthe gsm device has been closed!\n");
        return GSM_SETUP_DISABLE_SUCCESS;
      }
    }
  }
  return GSM_SETUP_DISABLE_SUCCESS;
}

GsmStatus gsm_reset(void)
{
  if (gsm_setup(DISABLE) == GSM_SETUP_DISABLE_SUCCESS)
  {
    if (gsm_setup(ENABLE) == GSM_SETUP_ENABLE_SUCCESS)
    {
      return GSM_RESET_SUCCESS;
    }
    else
    {
      return GSM_RESET_FAILURE;
    }
  }
  else
  {
    return GSM_RESET_FAILURE;
  }
}


ATCommandStatus gsm_send_at(void)
{
  char *recv_buf;
  char *temp;
  rt_device_t device_gsm_usart = rt_device_find(DEVICE_NAME_GSM_USART);
  uint8_t at_command_index = AT;
  uint16_t recv_size;
  char match_error;
  
  if (device_gsm_usart != RT_NULL)
  {
    recv_buf = (char *)rt_malloc(RECV_BUF_SIZE);
    if (recv_buf == RT_NULL)
    {
      rt_kprintf("no more memory can use!!!\n");
      return AT_NO_RESPONSE;
    }
    memset(recv_buf, '\0', RECV_BUF_SIZE);
    rt_kprintf("gsm send : ");
    gsm_put_char(at_command[at_command_index], strlen(at_command[at_command_index]));
    rt_device_write(device_gsm_usart, 0, \
                    at_command[at_command_index], \
                    strlen(at_command[at_command_index]));
    rt_thread_delay(5);
    recv_size = rt_device_read(device_gsm_usart, 0, recv_buf, RECV_BUF_SIZE);
    if (recv_size <= 0)
    {
      rt_free(recv_buf);
      rt_kprintf("gsm recv is not exist!\n");
      return AT_NO_RESPONSE;
    }
    else
    {
      rt_kprintf("gsm recv : ");
      gsm_put_char(recv_buf, recv_size);
      gsm_put_hex(recv_buf, recv_size);
      /* parse at response */
      temp = (char *)rt_malloc(RECV_BUF_SIZE);
      memset(temp, '\0', RECV_BUF_SIZE);
      match_error = sscanf(recv_buf, "%*[^AT]AT\r\n%s\r\n", temp);
      gsm_put_char(temp, strlen(temp));
      rt_free(temp);
      rt_free(recv_buf);
      
      if (match_error < 0)
      {
        rt_kprintf("Error parsing\n");
        return AT_ERROR;
      }
      else
      {
        rt_kprintf("Success parsing\n");
        return AT_OK;
      }
    }
  }
  else
  {
    rt_kprintf("gsm usart device is not exist !\n");
    return AT_NO_RESPONSE;
  }
}

ATCommandStatus gsm_send_at_cnmi(void)
{
  char *recv_buf;
  char *temp;
  rt_device_t device_gsm_usart = rt_device_find(DEVICE_NAME_GSM_USART);
  uint8_t at_command_index = AT_CNMI;
  uint16_t recv_size;
  char match_error;
  
  if (device_gsm_usart != RT_NULL)
  {
    recv_buf = (char *)rt_malloc(RECV_BUF_SIZE);
    if (recv_buf == RT_NULL)
    {
      rt_kprintf("no more memory can use!!!\n");
      return AT_NO_RESPONSE;
    }
    memset(recv_buf, '\0', RECV_BUF_SIZE);
    rt_kprintf("gsm send : ");
    gsm_put_char(at_command[at_command_index], strlen(at_command[at_command_index]));
    rt_device_write(device_gsm_usart, 0, \
                    at_command[at_command_index], \
                    strlen(at_command[at_command_index]));
    rt_thread_delay(5);
    recv_size = rt_device_read(device_gsm_usart, 0, recv_buf, RECV_BUF_SIZE);
    if (recv_size <= 0)
    {
      rt_free(recv_buf);
      rt_kprintf("gsm recv is not exist!\n");
      return AT_NO_RESPONSE;
    }
    else
    {
      rt_kprintf("gsm recv : ");
      gsm_put_char(recv_buf, recv_size);
      gsm_put_hex(recv_buf, recv_size);
      /* parse at response */
      temp = (char *)rt_malloc(RECV_BUF_SIZE);
      memset(temp, '\0', RECV_BUF_SIZE);
      match_error = sscanf(recv_buf, "%*[^O]%[OKERO]\r\n", temp);
      gsm_put_char(temp, RECV_BUF_SIZE);
      rt_free(temp);
      rt_free(recv_buf);

      if (match_error < 0)
      {
        rt_kprintf("Error parsing\n");
        return AT_ERROR;
      }
      else
      {
        rt_kprintf("Success parsing\n");
        return AT_OK;
      }
    }
  }
  else
  {
    rt_kprintf("gsm usart device is not exist !\n");
    return AT_NO_RESPONSE;
  }
}


ATCommandStatus gsm_send_at_csca(void)
{
  char *recv_buf;
  char *temp;
  rt_device_t device_gsm_usart = rt_device_find(DEVICE_NAME_GSM_USART);
  uint8_t at_command_index = AT_CSCA;
  uint16_t recv_size;
  char match_error;
  
  if (device_gsm_usart != RT_NULL)
  {
    recv_buf = (char *)rt_malloc(RECV_BUF_SIZE);
    if (recv_buf == RT_NULL)
    {
      rt_kprintf("no more memory can use!!!\n");
      return AT_NO_RESPONSE;
    }
    memset(recv_buf, '\0', RECV_BUF_SIZE);
    rt_kprintf("gsm send : ");
    gsm_put_char(at_command[at_command_index], strlen(at_command[at_command_index]));
    rt_device_write(device_gsm_usart, 0, \
                    at_command[at_command_index], \
                    strlen(at_command[at_command_index]));
    rt_thread_delay(5);
    recv_size = rt_device_read(device_gsm_usart, 0, recv_buf, RECV_BUF_SIZE);
    if (recv_size <= 0)
    {
      rt_free(recv_buf);
      rt_kprintf("gsm recv is not exist!\n");
      return AT_NO_RESPONSE;
    }
    else
    {
      rt_kprintf("gsm recv : ");
      gsm_put_char(recv_buf, recv_size);
      gsm_put_hex(recv_buf, recv_size);
      /* parse at response */
      temp = (char *)rt_malloc(RECV_BUF_SIZE);
      memset(temp, '\0', RECV_BUF_SIZE);
      match_error = sscanf(recv_buf, "%*[^86]86%[^\"]", temp);
      gsm_put_char(temp, RECV_BUF_SIZE);
      rt_free(temp);
      rt_free(recv_buf);

      if (match_error < 0)
      {
        rt_kprintf("Error parsing\n");
        return AT_ERROR;
      }
      else
      {
        rt_kprintf("Success parsing\n");
        return AT_OK;
      }
    }
  }
  else
  {
    rt_kprintf("gsm usart device is not exist !\n");
    return AT_NO_RESPONSE;
  }
}


void gsm_process_thread_entry(void *parameters)
{
  ATCommandStatus response;

  event_gsm_mode = rt_event_create("evt_gsm", RT_IPC_FLAG_FIFO);
  gsm_reset();
  while (1)
  {
    if (gsm_setup(ENABLE) == GSM_SETUP_ENABLE_SUCCESS)
    {
      rt_thread_delay(200);
      /* at */
      response = gsm_send_at();
      if (response == AT_OK)
      {

      }
      else if (response == AT_NO_RESPONSE)
      {
        if (gsm_setup(DISABLE) == GSM_SETUP_DISABLE_SUCCESS)
        {
          rt_thread_delay(200);
        }
        else
        {
          rt_kprintf("gsm can`t disable!!!\n");
        }
        rt_thread_delay(200);
        continue;
      }
      else
      {
        continue;
      }
      /* AT+CNMI */
      response = gsm_send_at_cnmi();
      if (response == AT_OK)
      {

      }
      else if (response == AT_NO_RESPONSE)
      {
        if (gsm_setup(DISABLE) == GSM_SETUP_DISABLE_SUCCESS)
        {
          rt_thread_delay(200);
        }
        else
        {
          rt_kprintf("gsm can`t disable!!!\n");
        }
        rt_thread_delay(200);
        continue;
      }
      else
      {
        continue;
      }
      /* AT+CSCA */
      response = gsm_send_at_csca();
      if (response == AT_OK)
      {

      }
      else if (response == AT_NO_RESPONSE)
      {
        if (gsm_setup(DISABLE) == GSM_SETUP_DISABLE_SUCCESS)
        {
          rt_thread_delay(200);
        }
        else
        {
          rt_kprintf("gsm can`t disable!!!\n");
        }
        rt_thread_delay(200);
        continue;
      }
      else
      {
        continue;
      }
    }
    else
    {
      rt_thread_delay(1000);
    }
  }
}



void gsm_sms_recv_process_thread_entry(void *parameters)
{
  rt_err_t result;
  rt_size_t recv_size;
  rt_device_t device_gsm_usart = rt_device_find(DEVICE_NAME_GSM_USART);
  char *recv_buf;
  char *match;
  
  while (1)
  {
    
  }
}
