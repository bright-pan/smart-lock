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
#include <stdio.h>

#define GSM_SEND_MAX_MSGS 20
#define GSM_RECV_MAX_MSGS 20

#define RECV_BUF_SIZE 256

rt_event_t event_gsm_mode_request;
rt_event_t event_gsm_mode_response;

rt_mutex_t mutex_gsm_mode;

static volatile rt_uint32_t gsm_mode = 0;

char smsc[20] = {0,};

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


#define NO_INSERT_SIM "%*[^+]+CPIN: %[^+]"

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
      rt_kprintf("\ngsm recv is not exist!\n");
      return AT_NO_RESPONSE;
    }
    else
    {
      rt_kprintf("\ngsm recv : ");
      gsm_put_char(recv_buf, recv_size);
      gsm_put_hex(recv_buf, recv_size);
      /* parse at response */
      temp = (char *)rt_malloc(RECV_BUF_SIZE);
      memset(temp, '\0', RECV_BUF_SIZE);
      match_error = sscanf(recv_buf, "%*[^+]+CPIN: %[^+]", temp);
      
      if (match_error != 1)
      {
        match_error = sscanf(recv_buf, "%*[^+]+%*[^+]+CPIN: %[^\r]", temp);
        if (match_error != 1)
        {
          rt_kprintf("\nError parsing: GSM module setup error!\n");
          rt_free(temp);
          rt_free(recv_buf);
          return AT_ERROR;
        }
        else
        {
          rt_kprintf("\nSuccess parsing: ");
          gsm_put_char(temp, strlen(temp));
          rt_free(temp);
          rt_free(recv_buf);
          return AT_OK;
        }
      }
      else
      {
        rt_kprintf("\nSuccess parsing: ");
        gsm_put_char(temp, strlen(temp));
        rt_free(temp);
        rt_free(recv_buf);
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
        rt_thread_delay(150);
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
        rt_thread_delay(200);
        gsm_power(DISABLE);
        rt_thread_delay(400);
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
    rt_thread_delay(50);
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
      match_error = sscanf(recv_buf, "%*[^\r]\r\r\n%[^\r]\r\n", temp);

      if (match_error != 1)
      {
        rt_kprintf("Error parsing\n");
        rt_free(temp);
        rt_free(recv_buf);
        return AT_ERROR;
      }
      else
      {
        rt_kprintf("Success parsing\n");
        gsm_put_char(temp, strlen(temp));
        rt_free(temp);
        rt_free(recv_buf);
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
    rt_thread_delay(50);
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
      match_error = sscanf(recv_buf, "%*[^\r]\r\r\n%[^\r]\r\n", temp);

      if (match_error != 1)
      {
        rt_kprintf("Error parsing\n");
        rt_free(temp);
        rt_free(recv_buf);
        return AT_ERROR;
      }
      else
      {
        rt_kprintf("Success parsing\n");
        gsm_put_char(temp, RECV_BUF_SIZE);
        rt_free(temp);
        rt_free(recv_buf);
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
    rt_thread_delay(50);
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
      match_error = sscanf(recv_buf, "%*[^\"]\"+%[^\"]", temp);

      if (match_error != 1)
      {
        rt_kprintf("Error parsing\n");
        rt_free(temp);
        rt_free(recv_buf);
        return AT_ERROR;
      }
      else
      {
        rt_kprintf("Success parsing\n");
        gsm_put_char(temp, RECV_BUF_SIZE);
        /*        uint16_t data[] = {0x5DE5,0x4F5C,0x6109,0x5FEB,0xFF01};
        sms_pdu_ucs_send("8613316975697",temp,data, 5);
        */
        memcpy(smsc, temp, 20);
        gsm_put_char(smsc, 20);
        rt_free(temp);
        rt_free(recv_buf);
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


static rt_int8_t gsm_init(void)
{
  ATCommandStatus response;
  rt_int8_t no_response_counts;
  rt_int8_t counts = 0;

  while (1)
  {
    if (gsm_setup(ENABLE) == GSM_SETUP_ENABLE_SUCCESS)
    {
      rt_thread_delay(200);
      /* at */

      response = gsm_send_at();
      if (response == AT_OK)
      {
        no_response_counts = 0;
      }
      else if (response == AT_NO_RESPONSE)
      {
        if (no_response_counts++ > 20)
        {
          no_response_counts = 0;
          if (gsm_setup(DISABLE) == GSM_SETUP_DISABLE_SUCCESS)
          {
            rt_thread_delay(200);
          }
          else
          {
            rt_kprintf("gsm can`t disable!!!\n");
          }
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
        no_response_counts = 0;
      }
      else if (response == AT_NO_RESPONSE)
      {
        if (no_response_counts++ > 20)
        {
          no_response_counts = 0;          
          if (gsm_setup(DISABLE) == GSM_SETUP_DISABLE_SUCCESS)
          {
            rt_thread_delay(200);
          }
          else
          {
            rt_kprintf("gsm can`t disable!!!\n");
          }
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
        if (no_response_counts++ > 20)
        {
          no_response_counts = 0;          
          if (gsm_setup(DISABLE) == GSM_SETUP_DISABLE_SUCCESS)
          {
            rt_thread_delay(200);
          }
          else
          {
            rt_kprintf("gsm can`t disable!!!\n");
          }
        }
        rt_thread_delay(200);
        continue;
      }
      else
      {
        continue;
      }
      return 1;
    }
    else
    {
      if (counts++ > 20)
      {
        return -1;
      }
      rt_thread_delay(1000);
    }

  }
}

static void gsm_check(void)
{
  ATCommandStatus response;
  
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

      break;
    }
    else
    {
      rt_thread_delay(1000);
    }
  }
}

void gsm_process_thread_entry(void *parameters)
{
  rt_uint32_t request_event, response_event;
  rt_err_t result;


  event_gsm_mode_request = rt_event_create("evt_g_mrq", RT_IPC_FLAG_FIFO);
  event_gsm_mode_response = rt_event_create("evt_g_mrp", RT_IPC_FLAG_FIFO);  
  mutex_gsm_mode = rt_mutex_create("mut_g_m", RT_IPC_FLAG_FIFO);
  rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_CMD);
  gsm_mode_set(EVENT_GSM_MODE_CMD);
  rt_mutex_take(mutex_gsm_mode, RT_WAITING_FOREVER);
  gsm_reset();
  rt_mutex_release(mutex_gsm_mode);
  while (1)
  {
    result = rt_event_recv(event_gsm_mode_request,
                           EVENT_GSM_MODE_GPRS | EVENT_GSM_MODE_CMD | EVENT_GSM_MODE_GPRS_CMD,
                           RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 30000, &request_event);
    if (result == RT_EOK)
    {
      if (gsm_mode_get() & EVENT_GSM_MODE_CMD)
      {
        if (request_event & EVENT_GSM_MODE_GPRS)
        {
          // cmd -> gprs
          if (1) // success switch
          {
            rt_kprintf("\ngsm mode switch cmd -> gprs\n");
            gsm_mode_set(EVENT_GSM_MODE_GPRS);
            rt_event_send(event_gsm_mode_response, EVENT_GSM_MODE_GPRS);
          }
          else
          {

          }
        }
        else if (request_event & EVENT_GSM_MODE_GPRS_CMD)
        {
          //cmd -> gprs_cmd
          rt_kprintf("\ngsm mode switch cmd -> gprs_cmd\n");
          gsm_mode_set(EVENT_GSM_MODE_GPRS_CMD);
        }
        else if (request_event & EVENT_GSM_MODE_CMD)
        {
          rt_kprintf("\ngsm mode switch cmd -> cmd\n");
          //cmd -> cmd
          if (gsm_init() != -1)
          {
            rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &response_event);
            rt_event_send(event_gsm_mode_response, EVENT_GSM_MODE_SETUP);
            rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_GPRS);
          }
          else
          {
            rt_kprintf("\ngsm setup is fault, restart before 20s!\n");
            rt_thread_delay(2000);
            rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_CMD);
          }
        }
      }
      else if (gsm_mode_get() & EVENT_GSM_MODE_GPRS)
      {
        if (request_event & EVENT_GSM_MODE_GPRS)
        {
          // gprs -> gprs
          rt_kprintf("\ngsm mode switch gprs -> gprs\n");
          gsm_mode_set(EVENT_GSM_MODE_GPRS);
          rt_event_send(event_gsm_mode_response, EVENT_GSM_MODE_GPRS);
        }
        else if (request_event & EVENT_GSM_MODE_GPRS_CMD)
        {
          //gprs -> gprs_cmd
          rt_kprintf("\ngsm mode switch gprs -> gprs_cmd\n");
          gsm_mode_set(EVENT_GSM_MODE_GPRS_CMD);
          rt_event_send(event_gsm_mode_response, EVENT_GSM_MODE_GPRS_CMD);
        }
        else if (request_event & EVENT_GSM_MODE_CMD)
        {
          //gprs -> cmd
          rt_kprintf("\ngsm mode switch gprs -> cmd\n");
          gsm_mode_set(EVENT_GSM_MODE_CMD);
          rt_event_send(event_gsm_mode_response, EVENT_GSM_MODE_CMD);
        }
      }
      else if (gsm_mode_get() & EVENT_GSM_MODE_GPRS_CMD)
      {
        if (request_event & EVENT_GSM_MODE_GPRS)
        {
          // gprs_cmd -> gprs
          rt_kprintf("\ngsm mode switch gprs_cmd -> gprs\n");
          gsm_mode_set(EVENT_GSM_MODE_GPRS);
          rt_event_send(event_gsm_mode_response, EVENT_GSM_MODE_GPRS);
        }
        else if (request_event & EVENT_GSM_MODE_GPRS_CMD)
        {
          //gprs_cmd -> gprs_cmd
          rt_kprintf("\ngsm mode switch gprs_cmd -> gprs_cmd\n");
          gsm_mode_set(EVENT_GSM_MODE_GPRS);
          rt_event_send(event_gsm_mode_response, EVENT_GSM_MODE_GPRS);
        }
        else if (request_event & EVENT_GSM_MODE_CMD)
        {
          //gprs_cmd -> cmd
          rt_kprintf("\ngsm mode switch gprs_cmd -> cmd\n");
          gsm_mode_set(EVENT_GSM_MODE_CMD);
          rt_event_send(event_gsm_mode_response, EVENT_GSM_MODE_CMD);
        }
      }
    }
    else// gsm mode is maintain
    {
      if (gsm_mode_get() & EVENT_GSM_MODE_GPRS)
      {
        //send gprs heart;
        rt_kprintf("\ngsm mode is gprs\n");
        if (1)
        {
          // gprs heart success
        }
        else
        {
          //gprs heart failure
        }
      }
      else if (gsm_mode_get() & EVENT_GSM_MODE_GPRS_CMD)
      {
        //gprs_cmd
        //rt_kprintf("\ngsm mode is gprs_cmd\n");
      }
      else if (gsm_mode_get() & EVENT_GSM_MODE_CMD)
      {
        //cmd
        //rt_kprintf("\ngsm mode is cmd\n");
      }
    }//end
  }
}

rt_uint32_t gsm_mode_get(void)
{
  return gsm_mode;
}

void gsm_mode_set(rt_uint32_t mode)
{
  gsm_mode = mode;
}

