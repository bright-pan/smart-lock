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

#define GSM_MAIL_MAX_MSGS 20

char smsc[20] = {0,};

const char *at_command_map[] =
{
  "AT\r",
  "AT+CNMI=2,1\r",
  "AT+CSCA?\r",
  "AT+CMGF=0\r",
  "AT+CPIN?\r",
  "AT+CSQ\r",
  "AT+CGREG?\r",
  "AT+CGATT?\r",
  "AT+CIPMODE=1\r",
  "AT+CSTT\r",
  "AT+CIICR\r",
  "AT+CIFSR\r",
  "AT+CIPSHUT\r",
  "AT+CIPSTATUS\r",
  "AT+CIPSTART=\"TCP\",\"%s\",%d\r",
  "ATO\r",
  "+++",
};

void gsm_put_char(const uint8_t *str, uint16_t length)
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

void gsm_put_hex(const uint8_t *str, uint16_t length)
{
  uint16_t index = 0;
  while (index < length)
  {
    rt_kprintf("%02X ", str[index++]);
  }
  rt_kprintf("\n");
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


typedef enum {

  GSM_MODE_CMD,
  GSM_MODE_GPRS,
  GSM_MODE_GPRS_CMD,

}GSM_MODE_TYPEDEF;

rt_mq_t mq_gsm;

typedef struct{

  GSM_MODE_TYPEDEF gsm_mode;
  uint8_t *request;
  uint8_t *response;

}GSM_MAIL_TYPEDEF;

uint16_t gsm_recv_frame(uint8_t *buf)
{
  uint8_t *buf_bk = buf;
  uint8_t temp = 0;
  uint16_t length = 0;
  uint16_t counts = 0;
  uint16_t result;
  rt_device_t device_gsm_usart;

  device_gsm_usart = rt_device_find(DEVICE_NAME_GSM_USART);

rescan_frame:

  buf_bk = buf;
  temp = 0;
  length = 0;
  counts = 0;

  while (counts < 512)
  {
    result = rt_device_read(device_gsm_usart, 0, &temp, 1);

    if (result == 1)
    {
      *buf_bk++ = temp;
      ++length;
      if (strstr((char *)buf,"\r\n"))
      {
        if (length == 2)
        {
          goto rescan_frame;
        }
        else
        {
          break;
        }
      }
    }
    ++counts;
  }
  if (strstr((char *)buf,"RING"))
  {
    // has phone call
    goto rescan_frame;
  }
  if (strstr((char *)buf,"+CMTI:"))
  {
    // has sms recv
    goto rescan_frame;
  }
  return length;
}

int8_t at_response_process(AT_COMMAND_INDEX_TYPEDEF index)
{
  uint8_t counts = 0;
  uint16_t recv_counts;
  int8_t result = AT_RESPONSE_ERROR;
  uint8_t *process_buf = (uint8_t *)rt_malloc(512);

  while (counts < 10)
  {
    memset(process_buf, 0, 512);
    recv_counts = gsm_recv_frame(process_buf);
    if (recv_counts > 2)
    {
      gsm_put_char(process_buf, strlen((char *)process_buf));
      gsm_put_hex(process_buf, strlen((char *)process_buf));
        
      if (strstr((char *)process_buf, at_command_map[index]))
      {
        if (index == PLUS3)
        {
          result = AT_RESPONSE_ERROR;
          break;
        }
        memset(process_buf, 0, 512);
        recv_counts = gsm_recv_frame(process_buf);
        if (recv_counts > 2)
        {
          gsm_put_char(process_buf, strlen((char *)process_buf));
          gsm_put_hex(process_buf, strlen((char *)process_buf));
          if (index == ATO)
          {
            if (strstr((char *)process_buf, "NO CARRIER"))
            {
              result = AT_RESPONSE_NO_CARRIER;
            }
            else if (strstr((char *)process_buf, "OK"))
            {
              result = AT_RESPONSE_OK;
            }
            break;

          }
          if (strstr((char *)process_buf, "OK"))
          {
            if (index == AT_CIPSTATUS)
            {
              memset(process_buf, 0, 512);
              gsm_recv_frame(process_buf);
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (strstr((char *)process_buf, "CONNECT OK"))
              {
                result = AT_RESPONSE_CONNECT_OK;
              }
              else if(strstr((char *)process_buf, "TCP CLOSED"))
              {
                result = AT_RESPONSE_TCP_CLOSED;
              }
              break;

            }
            if (index == AT_CIPSTART)
            {
              rt_thread_delay(200);
              memset(process_buf, 0, 512);
              recv_counts = gsm_recv_frame(process_buf);
              if (recv_counts > 2)
              {
                if (strstr((char *)process_buf, "CONNECT OK"))
                {
                  result = AT_RESPONSE_CONNECT_OK;
                }
                else if(strstr((char *)process_buf, "TCP CLOSED"))
                {
                  result = AT_RESPONSE_TCP_CLOSED;
                }
                else if(strstr((char *)process_buf, "PDP DEACT"))
                {
                  result = AT_RESPONSE_PDP_DEACT;
                }
              }
              else
              {
                rt_thread_delay(2000);
                memset(process_buf, 0, 512);
                recv_counts = gsm_recv_frame(process_buf);
                if (recv_counts > 2)
                {
                  if (strstr((char *)process_buf, "CONNECT OK"))
                  {
                    result = AT_RESPONSE_CONNECT_OK;
                  }
                  else if(strstr((char *)process_buf, "TCP_CLOSED"))
                  {
                    result = AT_RESPONSE_TCP_CLOSED;
                  }
                }
              }
              break;

            }

            result = AT_RESPONSE_OK;
            break;
          }
          else if (strstr((char *)process_buf, "ERROR"))
          {
            result = AT_RESPONSE_ERROR;
            break;
          }
          else
          {
            memset(process_buf + 200, 0, 512);
            recv_counts = gsm_recv_frame(process_buf + 200);
            if (recv_counts > 2)
            {
              gsm_put_char(process_buf+200, strlen((char *)process_buf+200));
              gsm_put_hex(process_buf+200, strlen((char *)process_buf+200));

              if (strstr((char *)process_buf + 200, "OK"))
              {
                if (index == AT_CSCA)
                {
                    sscanf((char *)process_buf, "%*[^\"]\"+%[^\"]", smsc);
                }
                result = AT_RESPONSE_OK;
                break;
              }
              else
              {
                result = AT_RESPONSE_ERROR;
                break;
              }
            }
            else
            {
              result = AT_RESPONSE_ERROR;
              break;
            }
          }

        }
        else 
        {
              
        }
      }
      else if (strstr((char *)process_buf, "OK"))
      {
        if (index == PLUS3)
        {
          result = AT_RESPONSE_OK;
          break;
        }
      }
    }
    else
    {
      // no result process
    }
    counts++;
    rt_thread_delay(20);
  }
  rt_free(process_buf);
  return result;
}

int8_t gsm_command(AT_COMMAND_INDEX_TYPEDEF index, uint16_t delay, uint8_t *buf)
{
  rt_device_t device_gsm_usart;
  int8_t result;
  uint8_t *process_buf = (uint8_t *)rt_malloc(512);
  device_gsm_usart = rt_device_find(DEVICE_NAME_GSM_USART);
  switch (index)
  {
    case AT_CIPSTART:{

      memset(process_buf, 0, 512);
      rt_sprintf((char *)process_buf,
                 at_command_map[index],
                 device_parameters.tcp_domain.domain,
                 device_parameters.tcp_domain.port);

      rt_device_write(device_gsm_usart, 0,
                      process_buf,
                      strlen((char *)process_buf));

      gsm_put_char(process_buf, strlen((char *)process_buf));

      break;
    };
    case PLUS3 : {
      rt_thread_delay(150);
      rt_device_write(device_gsm_usart, 0,
                      at_command_map[index],
                      strlen(at_command_map[index]));

      gsm_put_char((uint8_t *)at_command_map[index], strlen(at_command_map[index]));
      break;
    };
    default: {

      rt_device_write(device_gsm_usart, 0,
                      at_command_map[index],
                      strlen(at_command_map[index]));

      gsm_put_char((uint8_t *)at_command_map[index], strlen(at_command_map[index]));
      break;
    }
  }
  memcpy(process_buf, at_command_map[index], strlen(at_command_map[index]));

  rt_thread_delay(delay);
  result = at_response_process(index);

  rt_free(process_buf);
  return result;
}


int8_t gsm_mode_switch(GSM_MODE_TYPEDEF *gsm_mode, GSM_MAIL_TYPEDEF *buf)
{
  uint8_t *process_buf = (uint8_t *)rt_malloc(512);
  uint16_t result;
  
  switch (*gsm_mode)
  {
    case GSM_MODE_CMD : {
      if (buf->gsm_mode == GSM_MODE_GPRS)
      {
        //test cipstatus mode if test tcp is online send ATO\r or send cipstart
        // if ATO success , SET gsm mode GSM_MODE_GPRS,
      }
      break;
    };
    case GSM_MODE_GPRS : {
            
      break;
    };
  }
  rt_free(process_buf);
  return;
}
void gsm_process_thread_entry(void *parameters)
{

  rt_device_t device_gsm_status;
  rt_device_t device_gsm_usart;
  static GSM_MODE_TYPEDEF gsm_mode = GSM_MODE_CMD;
  GSM_MAIL_TYPEDEF *gsm_mail_buf;
  rt_err_t result;
  rt_uint8_t gsm_status;
  device_gsm_status = rt_device_find(DEVICE_NAME_GSM_STATUS);
  device_gsm_usart = rt_device_find(DEVICE_NAME_GSM_USART);

  mq_gsm = rt_mq_create("mq_gsm", sizeof(GSM_MAIL_TYPEDEF),
                        GSM_MAIL_MAX_MSGS,
                        RT_IPC_FLAG_FIFO);


  while (1)
  {
    if (gsm_reset() == GSM_RESET_FAILURE)
    {
      rt_thread_delay(100*300); // if reset failure delay 300s
    }
    else if (gsm_reset() == GSM_RESET_SUCCESS)
    {
      break;// success reset
    }
    else
    {

    }
  }
  rt_thread_delay(100*10); // delay 10S
  
  while (1)
  {
    rt_device_read(device_gsm_status, 0, &gsm_status, 0);
    if (gsm_status)
    {
      gsm_mail_buf = (GSM_MAIL_TYPEDEF *)rt_malloc(sizeof(GSM_MAIL_TYPEDEF));
      result = rt_mq_recv(mq_gsm, gsm_mail_buf,
                          sizeof(GSM_MAIL_TYPEDEF),
                          100);
      if (result == RT_EOK)
      {
        // process mail
      }
      else
      {
        // no mail
      }
      rt_free(gsm_mail_buf);
    }
    else
    {
      // gsm is not setup
      if (gsm_reset() == GSM_RESET_FAILURE)
      {
        rt_thread_delay(100*300); // if reset failure delay 300s
      }
      else if (gsm_reset() == GSM_RESET_SUCCESS)
      {

      }
      else
      {

      }
    }

  }
}


#ifdef RT_USING_FINSH
#include <finsh.h>
void gsm_ip_start(char *ip, int port)
{
  char *at_temp;
  rt_device_t device_gsm_usart = rt_device_find(DEVICE_NAME_GSM_USART);

  if (device_gsm_usart != RT_NULL)
  {
    at_temp = (char *)rt_malloc(512);
    memset(at_temp, '\0', 512);
    rt_sprintf(at_temp,"AT+CIPSTART=\"TCP\",\"%s\",%d\r", ip, port);
    gsm_put_char((uint8_t *)at_temp, strlen(at_temp));
    rt_device_write(device_gsm_usart, 0, at_temp, strlen(at_temp));
    rt_free(at_temp);
  }
  else
  {
    rt_kprintf("gsm usart device is not exist !\n");
  }
}
FINSH_FUNCTION_EXPORT(gsm_ip_start, gsm_ip_start[cmd parameters])

FINSH_FUNCTION_EXPORT(gsm_command, gsm_send_frame[at_index delay *buf])
#endif

