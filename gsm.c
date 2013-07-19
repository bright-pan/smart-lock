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
#include "gprs.h"
#include "aip.h"
#include "gpio_pin.h"
#include "gsm_usart.h"
#include "untils.h"
//#include "board.h"
#include <string.h>
#include <stdio.h>

#define GSM_MAIL_MAX_MSGS 20

rt_mq_t mq_gsm = RT_NULL;
rt_mutex_t mutex_gsm_mail_sequence;

char smsc[20] = {0,};
char phone_call[20] = {0,};

const char *at_command_map[70];

void at_command_map_init(void)
{
  at_command_map[AT] = "AT\r";
  at_command_map[AT_CNMI] = "AT+CNMI=2,1\r";
  at_command_map[AT_CSCA] = "AT+CSCA?\r";
  at_command_map[AT_CMGF] = "AT+CMGF=0\r";
  at_command_map[AT_CPIN] = "AT+CPIN?\r";
  at_command_map[AT_CSQ] = "AT+CSQ\r";
  at_command_map[AT_CGREG] = "AT+CGREG?\r";
  at_command_map[AT_CGATT] = "AT+CGATT?\r";
  at_command_map[AT_CIPMODE] = "AT+CIPMODE=1\r";
  at_command_map[AT_CSTT] = "AT+CSTT\r";
  at_command_map[AT_CIICR] = "AT+CIICR\r";
  at_command_map[AT_CIFSR] = "AT+CIFSR\r";
  at_command_map[AT_CIPSHUT] = "AT+CIPSHUT\r";
  at_command_map[AT_CIPSTATUS] = "AT+CIPSTATUS\r";
  at_command_map[AT_CIPSTART] = "AT+CIPSTART=\"TCP\",\"%s\",%d\r";
  at_command_map[AT_CMGS] = "AT+CMGS=%d\x0D";
  at_command_map[AT_CMGS_SUFFIX] = "";
  at_command_map[ATO] = "ATO\r";
  at_command_map[PLUS3] = "+++";
  at_command_map[AT_CMMSINIT] = "AT+CMMSINIT\r";
  at_command_map[AT_CMMSTERM] = "AT+CMMSTERM\r";
  at_command_map[AT_CMMSCURL] = "AT+CMMSCURL=\"mmsc.monternet.com\"\r";
  at_command_map[AT_CMMSCID] = "AT+CMMSCID=1\r";
  at_command_map[AT_CMMSPROTO] = "AT+CMMSPROTO=\"10.0.0.172\",80\r";
  at_command_map[AT_CMMSSENDCFG] = "AT+CMMSSENDCFG=6,3,0,0,2,4\r";
  at_command_map[AT_SAPBR_CONTYPE] = "AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r";
  at_command_map[AT_SAPBR_APN_CMWAP] = "AT+SAPBR=3,1,\"APN\",\"CMWAP\"\r";
  at_command_map[AT_SAPBR_APN_CMNET] = "AT+SAPBR=3,1,\"APN\",\"CMNET\"\r";
  at_command_map[AT_SAPBR_OPEN] = "AT+SAPBR=1,1\r";
  at_command_map[AT_SAPBR_CLOSE] = "AT+SAPBR=0,1\r";
  at_command_map[AT_SAPBR_REQUEST] = "AT+SAPBR=2,1\r";
  at_command_map[AT_CMMSEDIT_OPEN] = "AT+CMMSEDIT=1\r";
  at_command_map[AT_CMMSEDIT_CLOSE] = "AT+CMMSEDIT=0\r";
  at_command_map[AT_CMMSDOWN_PIC] = "AT+CMMSDOWN=\"PIC\",%d,50000\r";
  at_command_map[AT_CMMSDOWN_TITLE] = "AT+CMMSDOWN=\"TITLE\",%d,50000\r";
  at_command_map[AT_CMMSDOWN_TEXT] = "AT+CMMSDOWN=\"TEXT\",%d,50000\r";
  at_command_map[AT_CMMSRECP] = "AT+CMMSRECP=\"%s\"\r";
  at_command_map[AT_CMMSSEND] = "AT+CMMSSEND\r";
  at_command_map[AT_CLCC] = "AT+CLCC\r";
  at_command_map[ATA] = "ATA\r";
  at_command_map[ATH5] = "ATH5\r";
  at_command_map[AT_RING] = "";
  at_command_map[AT_RECV_SMS] = "";
  at_command_map[ATI] = "ATI\r";
  at_command_map[AT_GSV] = "AT+GSV\r";
  at_command_map[AT_V] = "AT&V\r";
  at_command_map[AT_D1] = "AT&D1\r";
  at_command_map[AT_W] = "AT&W\r";
  at_command_map[AT_HTTPINIT] = "AT+HTTPINIT\r";
  at_command_map[AT_HTTPTERM] = "AT+HTTPTERM\r";
  at_command_map[AT_HTTPPARA_CID] = "AT+HTTPPARA=\"CID\",1\r";
  at_command_map[AT_HTTPPARA_URL] = "AT+HTTPPARA=\"URL\",\"%s\"\r";
  at_command_map[AT_HTTPACTION_POST] = "AT+HTTPACTION=1\r";
  at_command_map[AT_HTTPACTION_GET] = "AT+HTTPACTION=0\r";
  at_command_map[AT_HTTPACTION_HEAD] = "AT+HTTPACTION=2\r";
  at_command_map[AT_HTTPREAD] = "AT+HTTPREAD=%d,%d\r";
  at_command_map[AT_HTTPPARA_BREAK] = "AT+HTTPPARA=\"BREAK\",%d\r";
  at_command_map[AT_HTTPPARA_BREAKEND] = "AT+HTTPPARA=\"BREAKEND\",%d\r";

}

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
  if (length)
  {
    rt_kprintf("\n");
  }
}

void gsm_put_hex(const uint8_t *str, uint16_t length)
{
  uint16_t index = 0;
  while (index < length)
  {
    rt_kprintf("%02X ", str[index++]);
  }
  if (length)
  {
    rt_kprintf("\n");
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

  memset(buf, 0, length);
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
  /*
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
  */
  return length;
}




int8_t at_response_process(AT_COMMAND_INDEX_TYPEDEF index, uint8_t *buf, GSM_MAIL_CMD_DATA *cmd_data)
{
  uint8_t counts = 0;
  uint8_t no_response_counts = 0;
  uint8_t delay_counts = 0;
  uint16_t recv_counts = 0;
  int8_t result = AT_RESPONSE_ERROR;
  uint8_t *process_buf = (uint8_t *)rt_malloc(512);
  int temp = 0;
  rt_device_t device_gsm_usart;

  device_gsm_usart = rt_device_find(DEVICE_NAME_GSM_USART);

  while (counts < 10)
  {
    memset(process_buf, 0, 512);
    recv_counts = gsm_recv_frame(process_buf);
    gsm_put_char(process_buf, strlen((char *)process_buf));
    gsm_put_hex(process_buf, strlen((char *)process_buf));
    if (recv_counts)
    {
      switch (index)
      {
        case AT :
        case AT_D1 :
        case AT_W :
        case AT_CNMI :
        case AT_CMGF :
        case AT_CPIN :
        case AT_CIPMODE:
        case AT_CSTT :
        case AT_CIPSHUT :
        case AT_CMMSINIT :
        case AT_CMMSTERM :
        case AT_HTTPINIT :
        case AT_HTTPTERM :
        case AT_HTTPPARA_CID :
        case AT_CMMSCID :
        case AT_CMMSCURL :
        case AT_CMMSPROTO :
        case AT_CMMSSENDCFG :
        case AT_SAPBR_CONTYPE :
        case AT_SAPBR_APN_CMWAP :
        case AT_SAPBR_APN_CMNET :
        case AT_SAPBR_CLOSE :
        case AT_CMMSEDIT_OPEN :
        case AT_CMMSEDIT_CLOSE :
        case ATA:
        case ATH5: {

          if (strstr((char *)process_buf, at_command_map[index]))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (strstr((char *)process_buf, "OK"))
              {
                result = AT_RESPONSE_OK;
              }
            }
            goto complete;
          }
          break;
        };
        case AT_CMMSRECP : {

          if (strstr((char *)process_buf, (char *)buf))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (strstr((char *)process_buf, "OK"))
              {
                result = AT_RESPONSE_OK;
              }
            }
            goto complete;
          }
          break;
        };
        case AT_HTTPPARA_BREAK:
        case AT_HTTPPARA_BREAKEND:
        case AT_HTTPPARA_URL : {

          if (strstr((char *)process_buf, (char *)buf))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (strstr((char *)process_buf, "OK"))
              {
                result = AT_RESPONSE_OK;
              }
            }
            goto complete;
          }
          break;
        };
        case AT_CIICR : {

          if (strstr((char *)process_buf, at_command_map[index]))
          {
            delay_counts = 0;
            while (delay_counts++ < 10)
            {
              rt_thread_delay(200);
              memset(process_buf, 0, 512);
              recv_counts = gsm_recv_frame(process_buf);
              if (recv_counts)
              {
                gsm_put_char(process_buf, strlen((char *)process_buf));
                gsm_put_hex(process_buf, strlen((char *)process_buf));
                if (strstr((char *)process_buf, "OK"))
                {
                  result = AT_RESPONSE_OK;
                }
                break;
              }
            }
            goto complete;
          }
          break;
        };
        case AT_SAPBR_OPEN : {

          if (strstr((char *)process_buf, at_command_map[index]))
          {
            delay_counts = 0;
            while (delay_counts++ < 10)
            {
              rt_thread_delay(200);
              memset(process_buf, 0, 512);
              recv_counts = gsm_recv_frame(process_buf);
              if (recv_counts)
              {
                gsm_put_char(process_buf, strlen((char *)process_buf));
                gsm_put_hex(process_buf, strlen((char *)process_buf));
                if (strstr((char *)process_buf, "OK"))
                {
                  result = AT_RESPONSE_OK;
                }
                break;
              }
            }
            goto complete;
          }
          break;
        };
        case AT_CMMSSEND : {

          if (strstr((char *)process_buf, at_command_map[index]))
          {
            delay_counts = 0;
            while (delay_counts++ < 100)
            {
              rt_thread_delay(200);
              memset(process_buf, 0, 512);
              recv_counts = gsm_recv_frame(process_buf);
              if (recv_counts)
              {
                gsm_put_char(process_buf, strlen((char *)process_buf));
                gsm_put_hex(process_buf, strlen((char *)process_buf));
                if (strstr((char *)process_buf, "OK"))
                {
                  result = AT_RESPONSE_OK;
                }
                break;
              }
            }
            goto complete;
          }
          break;
        };
        case AT_SAPBR_REQUEST : {

          if (strstr((char *)process_buf, at_command_map[index]))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (!strstr((char *)process_buf, "ERROR"))
              {
                memset(process_buf + 200, 0, 512-200);
                recv_counts = gsm_recv_frame(process_buf + 200);
                if (recv_counts)
                {
                  gsm_put_char(process_buf+200, strlen((char *)process_buf+200));
                  gsm_put_hex(process_buf+200, strlen((char *)process_buf+200));

                  if (strstr((char *)process_buf + 200, "OK"))
                  {
                    //sscanf((char *)process_buf, "%*[^\"]\"+%[^\"]", smsc);
                    result = AT_RESPONSE_OK;
                  }
                }
              }
            }
            goto complete;
          }
          break;
        }

        case AT_CSCA : {

          if (strstr((char *)process_buf, at_command_map[index]))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (!strstr((char *)process_buf, "ERROR"))
              {
                memset(process_buf + 200, 0, 512-200);
                recv_counts = gsm_recv_frame(process_buf + 200);
                if (recv_counts)
                {
                  gsm_put_char(process_buf+200, strlen((char *)process_buf+200));
                  gsm_put_hex(process_buf+200, strlen((char *)process_buf+200));

                  if (strstr((char *)process_buf + 200, "OK"))
                  {
                    memset(smsc, 0, sizeof(smsc));
                    sscanf((char *)process_buf, "%*[^\"]\"+%[^\"]", smsc);
                    result = AT_RESPONSE_OK;
                  }
                }
              }
            }
            goto complete;
          }
          break;
        }
        case AT_HTTPACTION_GET : {

          if (strstr((char *)process_buf, at_command_map[index]))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (strstr((char *)process_buf, "OK"))
              {
                delay_counts = 0;
                while (delay_counts++ < 100)
                {
                  rt_thread_delay(200);
                  memset(process_buf, 0, 512);
                  recv_counts = gsm_recv_frame(process_buf);
                  if (recv_counts)
                  {
                    gsm_put_char(process_buf, strlen((char *)process_buf));
                    gsm_put_hex(process_buf, strlen((char *)process_buf));

                    if (strstr((char *)process_buf, "+HTTPACTION:0"))
                    {
                      sscanf((char *)process_buf, "+HTTPACTION:0,%d,%d", &temp ,&aip_bin_size);
                      switch (temp)
                      {
                        case 200 : {
                          result = AT_RESPONSE_OK;
                          break;
                        }
                        case 206 : {
                          result = AT_RESPONSE_PARTIAL_CONTENT;
                          break;
                        }
                        case 400 : {
                          result = AT_RESPONSE_BAD_REQUEST;
                          break;
                        }
                        case 602 : {
                          result = AT_RESPONSE_NO_MEMORY;
                          break;
                        }
                        default : {
                          break;
                        }
                      }
                      break;
                    }
                    else
                    {
                      continue;
                    }
                  }
                }
              }
            }
            goto complete;
          }
          break;
        }
        case AT_HTTPREAD : {

          if (strstr((char *)process_buf, (char *)buf))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (strstr((char *)process_buf, "+HTTPREAD:"))
              {
                sscanf((char *)process_buf, "+HTTPREAD:%d", &temp);
                if (temp <= 512)
                {
                  recv_counts = rt_device_read(device_gsm_usart, 0,
                                               cmd_data->httpread.buf,
                                               temp);
                  if (recv_counts)
                  {
                    *(cmd_data->httpread.recv_counts) = recv_counts;
                    memset(process_buf, 0, 512);
                    recv_counts = gsm_recv_frame(process_buf);
                    if (recv_counts)
                    {

                      if (strstr((char *)process_buf, "OK"))
                      {
                        gsm_put_char(process_buf, strlen((char *)process_buf));
                        gsm_put_hex(process_buf, strlen((char *)process_buf));
                        result = AT_RESPONSE_OK;
                      }
                    }
                  }
                }
              }
            }
            goto complete;
          }
          break;
        }

        case AT_CIFSR : {

          if (strstr((char *)process_buf, at_command_map[index]))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              //sscanf((char *)process_buf, "%[^\"]", smsc);
              result = AT_RESPONSE_OK;
            }
            goto complete;
          }
          break;
        }
        case AT_CIPSTATUS : {

          if (strstr((char *)process_buf, at_command_map[index]))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (strstr((char *)process_buf, "OK"))
              {
                memset(process_buf, 0, 512);
                recv_counts = gsm_recv_frame(process_buf);
                if (recv_counts)
                {
                  gsm_put_char(process_buf, strlen((char *)process_buf));
                  gsm_put_hex(process_buf, strlen((char *)process_buf));
                  if (strstr((char *)process_buf, "CONNECT OK"))
                  {
                    result = AT_RESPONSE_CONNECT_OK;
                  }
                  else if (strstr((char *)process_buf, "TCP CLOSED"))
                  {
                    result = AT_RESPONSE_TCP_CLOSED;
                  }
                }
              }
            }
            goto complete;
          }
          
          break;
        };
        case AT_CIPSTART : {

          if (strstr((char *)process_buf, (char *)buf))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (strstr((char *)process_buf, "OK"))
              {
                delay_counts = 0;
                while (delay_counts++ < 10)
                {
                  rt_thread_delay(200);
                  memset(process_buf, 0, 512);
                  recv_counts = gsm_recv_frame(process_buf);
                  if (recv_counts)
                  {
                    gsm_put_char(process_buf, strlen((char *)process_buf));
                    gsm_put_hex(process_buf, strlen((char *)process_buf));
                    if (strstr((char *)process_buf, "CONNECT"))
                    {
                      result = AT_RESPONSE_CONNECT_OK;
                    }
                    break;
                  }
                }
              }
            }
            goto complete;
          }

          break;
        }
        case AT_CMGS : {

          if (strstr((char *)process_buf, (char *)buf))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (strstr((char *)process_buf, ">"))
              {
                result = AT_RESPONSE_OK;
              }
            }
            goto complete;
          }
          
          break;
        }
        case AT_CMGS_SUFFIX : {

          if (strstr((char *)process_buf, (char *)buf))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (strstr((char *)process_buf, "+CMGS:"))
              {
                memset(process_buf, 0, 512);
                recv_counts = gsm_recv_frame(process_buf);
                if (recv_counts)
                {
                  gsm_put_char(process_buf, strlen((char *)process_buf));
                  gsm_put_hex(process_buf, strlen((char *)process_buf));
                  if (strstr((char *)process_buf, "OK"))
                  {
                    result = AT_RESPONSE_OK;
                  }
                }
              }
            }
            goto complete;
          }

          break;
        };
        case ATO : {

          if (strstr((char *)process_buf, at_command_map[index]))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (strstr((char *)process_buf, "NO CARRIER"))
              {
                result = AT_RESPONSE_NO_CARRIER;
              }
              else if (strstr((char *)process_buf, "CONNECT"))
              {
                result = AT_RESPONSE_OK;
              }
            }
            goto complete;
          }
          break;
        };
        case PLUS3 : {

          if (strstr((char *)process_buf, "OK"))
          {
            result = AT_RESPONSE_OK;
          }
          goto complete;
          break;
        };
        case AT_CMMSDOWN_PIC :
        case AT_CMMSDOWN_TITLE :
        case AT_CMMSDOWN_TEXT : {

          if (strstr((char *)process_buf, (char *)buf))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (strstr((char *)process_buf, "CONNECT"))
              {
                result = AT_RESPONSE_CONNECT_OK;
              }
            }
            goto complete;
          }
          break;
        };

        case AT_CMMSDOWN_DATA : {

          if (strstr((char *)process_buf, "OK"))
          {
            result = AT_RESPONSE_OK;
            goto complete;
          }
          break;
        };
        case AT_RING : {

          if (strstr((char *)process_buf, "RING"))
          {
            result = AT_RESPONSE_OK;
            goto complete;
          }
          break;
        };
        case AT_CLCC : {
          if (strstr((char *)process_buf, at_command_map[index]))
          {
            memset(process_buf, 0, 512);
            recv_counts = gsm_recv_frame(process_buf);
            if (recv_counts)
            {
              gsm_put_char(process_buf, strlen((char *)process_buf));
              gsm_put_hex(process_buf, strlen((char *)process_buf));
              if (!strstr((char *)process_buf, "ERROR"))
              {
                memset(process_buf + 200, 0, 512-200);
                recv_counts = gsm_recv_frame(process_buf + 200);
                if (recv_counts)
                {
                  gsm_put_char(process_buf+200, strlen((char *)process_buf+200));
                  gsm_put_hex(process_buf+200, strlen((char *)process_buf+200));

                  if (strstr((char *)process_buf + 200, "OK"))
                  {
                    memset(phone_call, 0, sizeof(phone_call));
                    sscanf((char *)process_buf, "%*[^\"]\"%[^\"]", phone_call);
                    result = AT_RESPONSE_OK;
                  }
                }
              }
            }
            goto complete;
          }
          break;
        };
        default:{
          goto complete;
          break;
        };
      }
      
    }
    else
    {
      // no result process
      no_response_counts++;
      if (no_response_counts >= 10)
      {
        result = AT_NO_RESPONSE;
      }
    }
    counts++;
    rt_thread_delay(20);
  }

complete:
  rt_free(process_buf);
  return result;
}

int8_t gsm_command(AT_COMMAND_INDEX_TYPEDEF index, uint16_t delay, GSM_MAIL_CMD_DATA *cmd_data)
{
  rt_device_t device_gsm_usart;
  int8_t result = AT_RESPONSE_OK;
  uint8_t *process_buf = (uint8_t *)rt_malloc(512);;
  device_gsm_usart = rt_device_find(DEVICE_NAME_GSM_USART);
  switch (index)
  {
    case AT_HTTPPARA_BREAK:{

      memset(process_buf, 0, 512);
      rt_sprintf((char *)process_buf,
                 at_command_map[index],
                 cmd_data->httppara_break.start);

      rt_device_write(device_gsm_usart, 0,
                      process_buf,
                      strlen((char *)process_buf));

      gsm_put_char(process_buf, strlen((char *)process_buf));

      break;
    };
    case AT_HTTPPARA_BREAKEND:{

      memset(process_buf, 0, 512);
      rt_sprintf((char *)process_buf,
                 at_command_map[index],
                 cmd_data->httppara_breakend.end);

      rt_device_write(device_gsm_usart, 0,
                      process_buf,
                      strlen((char *)process_buf));

      gsm_put_char(process_buf, strlen((char *)process_buf));

      break;
    };
    case AT_HTTPPARA_URL:{

      memset(process_buf, 0, 512);
      rt_sprintf((char *)process_buf,
                 at_command_map[index],
                 cmd_data->httppara_url.buf);

      rt_device_write(device_gsm_usart, 0,
                      process_buf,
                      strlen((char *)process_buf));

      gsm_put_char(process_buf, strlen((char *)process_buf));

      break;
    };
    case AT_HTTPREAD:{

      memset(process_buf, 0, 512);
      rt_sprintf((char *)process_buf,
                 at_command_map[index],
                 cmd_data->httpread.start,
                 cmd_data->httpread.size_of_process);

      rt_device_write(device_gsm_usart, 0,
                      process_buf,
                      strlen((char *)process_buf));

      gsm_put_char(process_buf, strlen((char *)process_buf));

      break;
    };
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
    case AT_CMGS:{

      memset(process_buf, 0, 512);
      rt_sprintf((char *)process_buf,
                 at_command_map[index],
                 cmd_data->cmgs.length);

      rt_device_write(device_gsm_usart, 0,
                      process_buf,
                      strlen((char *)process_buf));

      gsm_put_char(process_buf, strlen((char *)process_buf));

      break;
    };
    case AT_CMMSRECP:{

      memset(process_buf, 0, 512);
      rt_sprintf((char *)process_buf,
                 at_command_map[index],
                 cmd_data->cmmsrecp.buf);

      rt_device_write(device_gsm_usart, 0,
                      process_buf,
                      strlen((char *)process_buf));

      gsm_put_char(process_buf, strlen((char *)process_buf));

      break;
    };

    case AT_CMMSDOWN_PIC:{

      memset(process_buf, 0, 512);
      rt_sprintf((char *)process_buf,
                 at_command_map[index],
                 cmd_data->cmmsdown_pic.length);

      rt_device_write(device_gsm_usart, 0,
                      process_buf,
                      strlen((char *)process_buf));

      gsm_put_char(process_buf, strlen((char *)process_buf));

      break;
    };
    case AT_CMMSDOWN_TITLE:{

      memset(process_buf, 0, 512);
      rt_sprintf((char *)process_buf,
                 at_command_map[index],
                 cmd_data->cmmsdown_title.length);

      rt_device_write(device_gsm_usart, 0,
                      process_buf,
                      strlen((char *)process_buf));

      gsm_put_char(process_buf, strlen((char *)process_buf));

      break;
    };
    case AT_CMMSDOWN_TEXT:{

      memset(process_buf, 0, 512);
      rt_sprintf((char *)process_buf,
                 at_command_map[index],
                 cmd_data->cmmsdown_text.length);

      rt_device_write(device_gsm_usart, 0,
                      process_buf,
                      strlen((char *)process_buf));

      gsm_put_char(process_buf, strlen((char *)process_buf));

      break;
    };
    case AT_CMMSDOWN_DATA:{

      memset(process_buf, 0, 512);
      memcpy(process_buf, cmd_data->cmmsdown_data.buf, cmd_data->cmmsdown_data.length);
      rt_device_write(device_gsm_usart, 0,
                      process_buf,
                      cmd_data->cmmsdown_data.length);

      gsm_put_char(process_buf, cmd_data->cmmsdown_data.length);

      if(!cmd_data->cmmsdown_data.has_complete)
      {
        goto at_process_complete;
      }

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
    case AT_RING :
    case AT_RECV_SMS :{

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
  rt_thread_delay(delay);
  result = at_response_process(index, process_buf, cmd_data);

  switch (index)
  {
    case AT_CMGS:{
      if (result == AT_RESPONSE_OK)
      {
        memset(process_buf, 0, 512);
        memcpy(process_buf, cmd_data->cmgs.buf, strlen((char *)cmd_data->cmgs.buf));
        rt_device_write(device_gsm_usart, 0,
                        process_buf,
                        strlen((char *)process_buf));
        rt_device_write(device_gsm_usart, 0, "\x1A", 1);
        gsm_put_char(process_buf, strlen((char *)process_buf));
        rt_thread_delay(600);
        result = at_response_process(AT_CMGS_SUFFIX, process_buf, cmd_data);
      }
      else
      {
        result = AT_RESPONSE_ERROR;
      }
      break;
    };
    default: {
      break;
    }
  }
at_process_complete:
  rt_free(process_buf);
  rt_kprintf("\n\nresult = %d\n\n", result);
  return result;
}


int8_t gsm_mode_switch(GSM_MODE_TYPEDEF *gsm_mode, GSM_MODE_TYPEDEF new_mode)
{
  int8_t result = 1;
  GSM_MAIL_CMD_DATA gsm_mail_cmd_data;

  switch (*gsm_mode)
  {
    case GSM_MODE_CMD : {
      if (new_mode == GSM_MODE_GPRS)
      {
        //test cipstatus mode if test tcp is online send ATO\r or send cipstart
        // if ATO success , SET gsm mode GSM_MODE_GPRS,
        if ((gsm_command(AT_CIPSTATUS, 100, &gsm_mail_cmd_data) == AT_RESPONSE_CONNECT_OK) &&
            (gsm_command(ATO, 100, &gsm_mail_cmd_data) == AT_RESPONSE_OK))
        {
          *gsm_mode = GSM_MODE_GPRS;
          result = 1;
        }
        else
        {
          if ((gsm_command(AT_CIPSHUT,50,&gsm_mail_cmd_data) == AT_RESPONSE_OK) &&
              (gsm_command(AT_CIPMODE,50,&gsm_mail_cmd_data) == AT_RESPONSE_OK) &&
              (gsm_command(AT_CSTT,50,&gsm_mail_cmd_data) == AT_RESPONSE_OK) &&
              (gsm_command(AT_CIICR,300,&gsm_mail_cmd_data) == AT_RESPONSE_OK) &&
              (gsm_command(AT_CIFSR,50,&gsm_mail_cmd_data) == AT_RESPONSE_OK) &&
              (gsm_command(AT_CIPSTART,300,&gsm_mail_cmd_data)== AT_RESPONSE_CONNECT_OK))
          {
            *gsm_mode = GSM_MODE_GPRS;
            result = 1;
            //send auth
            rt_thread_delay(100);
            send_gprs_auth_frame();
            rt_thread_delay(100);
            //send_gprs_mail(ALARM_TYPE_GPRS_AUTH, 0, 0);
          }
          else
          {
            result = 0;
          }
        }
      }
      break;
    };
    case GSM_MODE_GPRS : {
      if (new_mode == GSM_MODE_CMD)
      {
        // send plus3 to gsm, return !AT_NO_RESPONSE is success, or disable gsm.
        if (gsm_command(PLUS3, 100, &gsm_mail_cmd_data) == AT_NO_RESPONSE)
        {
          gsm_setup(DISABLE);
          *gsm_mode = GSM_MODE_CMD;
          result = -1;
        }
        else
        {
          *gsm_mode = GSM_MODE_CMD;
          result = 1;
        }
      }
      break;
    };
    default :{
      break;
    };
  }
  return result;
}

int8_t gsm_init_process(void)
{
  uint16_t counts = 0;
  int8_t result = -1;
  uint8_t *process_buf = (uint8_t *)rt_malloc(512);
  GSM_MAIL_CMD_DATA gsm_mail_cmd_data;
  while (counts < 100)
  {
    memset(process_buf,0,512);
    gsm_recv_frame(process_buf);
    if (strstr((char *)process_buf, "Call Ready"))
    {
      gsm_put_char(process_buf, strlen((char *)process_buf));
      gsm_put_hex(process_buf, strlen((char *)process_buf));
      if ((gsm_command(AT_D1, 50, &gsm_mail_cmd_data) == AT_RESPONSE_OK) &&
          (gsm_command(AT_W, 50, &gsm_mail_cmd_data) == AT_RESPONSE_OK) &&
          (gsm_command(AT_CNMI, 50, &gsm_mail_cmd_data) == AT_RESPONSE_OK) &&
          (gsm_command(AT_CSCA, 50, &gsm_mail_cmd_data) == AT_RESPONSE_OK) &&
          (gsm_command(AT_CMGF, 50, &gsm_mail_cmd_data) == AT_RESPONSE_OK))
      {
        result = 1;
        break;
      }
    }
    counts++;
    rt_thread_delay(50);
  }
  if (counts >= 100)
  {
    result = -1;
    rt_kprintf("gsm initial fault, please check sim insered!!!\n");
  }
  else
  {
    result = 1;
  }
  rt_free(process_buf);
  return result;
}

void gsm_process_thread_entry(void *parameters)
{

  rt_device_t device_gsm_status;
  rt_device_t device_gsm_usart;
  static GSM_MODE_TYPEDEF gsm_mode = GSM_MODE_CMD;
  GSM_MAIL_TYPEDEF gsm_mail_buf;
  rt_err_t result;
  rt_uint8_t gsm_status;
  uint16_t recv_counts = 0;
  int8_t send_counts = 0;
  uint8_t process_buf[512];
  GPRS_RECV_FRAME_TYPEDEF gprs_recv_frame;

  device_gsm_status = rt_device_find(DEVICE_NAME_GSM_STATUS);
  device_gsm_usart = rt_device_find(DEVICE_NAME_GSM_USART);

  at_command_map_init();
  mq_gsm = rt_mq_create("mq_gsm", sizeof(GSM_MAIL_TYPEDEF),
                        GSM_MAIL_MAX_MSGS,
                        RT_IPC_FLAG_FIFO);

  while (1)
  {
    if (gsm_reset() == GSM_RESET_SUCCESS ||
        gsm_reset() == GSM_RESET_SUCCESS)
    {
      if (gsm_init_process() == 1)
      {
        break;// success reset
      }
    }
    else
    {
      rt_thread_delay(100*300); // if reset failure delay 300s
    }
  }
  //rt_thread_delay(100*10); // delay 10S
  
  while (1)
  {
    rt_device_read(device_gsm_status, 0, &gsm_status, 0);
    if (gsm_status)
    {
      //gsm_mail_buf = (GSM_MAIL_TYPEDEF *)rt_malloc(sizeof(GSM_MAIL_TYPEDEF));
      result = rt_mq_recv(mq_gsm, &gsm_mail_buf,
                          sizeof(GSM_MAIL_TYPEDEF),
                          100);
      if (result == RT_EOK)
      {
        // process mail
        if (gsm_mail_buf.send_mode == GSM_MODE_CMD)
        {
          // cmd data
          if (gsm_mode_switch(&gsm_mode, gsm_mail_buf.send_mode) == 1)
          {
            
            *(gsm_mail_buf.result) = gsm_command(gsm_mail_buf.mail_data.cmd.index, gsm_mail_buf.mail_data.cmd.delay, &(gsm_mail_buf.mail_data.cmd.cmd_data));
          }
          else
          {
            *(gsm_mail_buf.result) = AT_RESPONSE_ERROR;
          }
        }
        else
        {
          //gprs data
          if (gsm_mode_switch(&gsm_mode, gsm_mail_buf.send_mode) == 1)
          {
            rt_device_write(device_gsm_usart, 0, gsm_mail_buf.mail_data.gprs.request, gsm_mail_buf.mail_data.gprs.request_length);
            if (gsm_mail_buf.mail_data.gprs.has_response == 1)
            {
              send_counts = 10;
              while (send_counts-- > 0)
              {
                rt_thread_delay(100);
                recv_counts = rt_device_read(device_gsm_usart, 0, gsm_mail_buf.mail_data.gprs.response, sizeof(GPRS_RECV_FRAME_TYPEDEF));
                *(gsm_mail_buf.mail_data.gprs.response_length) = recv_counts;
                if (recv_counts == 0)
                {
                  *(gsm_mail_buf.result) = -1;
                }
                else
                {
                  if (memmem(gsm_mail_buf.mail_data.gprs.request, gsm_mail_buf.mail_data.gprs.request_length, "\r\nCLOSED\r\n", strlen("\r\nCLOSED\r\n")) ||
                      memmem(gsm_mail_buf.mail_data.gprs.request, gsm_mail_buf.mail_data.gprs.request_length, gsm_mail_buf.mail_data.gprs.response, recv_counts))
                  {
                    gsm_mode_switch(&gsm_mode, GSM_MODE_CMD);
                    *(gsm_mail_buf.result) = -1;
                  }
                  else
                  {
                    *(gsm_mail_buf.result) = 1;
                  }
                  break;
                }
              }
            }
            else
            {
              *(gsm_mail_buf.result) = 1;
            }
          }
          else
          {
            *(gsm_mail_buf.result) = -1;
          }
        }
        rt_sem_release(gsm_mail_buf.result_sem);
      }
      else
      {
        if (gsm_mode == GSM_MODE_CMD)
        {
          //cmd data
          //memset(process_buf, 0, 512);
          //gsm_recv_frame(process_buf);
          //gsm_put_char(process_buf, strlen((char *)process_buf));
          //gsm_put_hex(process_buf, strlen((char *)process_buf));
        }
        else
        {
          //gprs data
          recv_counts = rt_device_read(device_gsm_usart, 0, &gprs_recv_frame, sizeof(GPRS_RECV_FRAME_TYPEDEF));
          if (recv_counts)
          {
            if (memmem(&gprs_recv_frame, sizeof(GPRS_RECV_FRAME_TYPEDEF), "\r\nCLOSED\r\n", strlen("\r\nCLOSED\r\n")))
            {
              gsm_mode_switch(&gsm_mode, GSM_MODE_CMD);
            }
            else
            {
              recv_gprs_frame(&gprs_recv_frame, recv_counts);
            }
          }
        }
      }
    }
    else
    {
      // gsm is not setup
      while (1)
      {
        if (gsm_reset() == GSM_RESET_SUCCESS ||
            gsm_reset() == GSM_RESET_SUCCESS)
        {
          if (gsm_init_process() == 1)
          {
            break;// success reset
          }
        }
        else
        {
          rt_thread_delay(100*300); // if reset failure delay 300s
        }
      }
    }
  }
}

AT_RESPONSE_TYPEDEF send_cmd_mail(AT_COMMAND_INDEX_TYPEDEF command_index, uint16_t delay, GSM_MAIL_CMD_DATA *cmd_data)
{
  GSM_MAIL_TYPEDEF gsm_mail_buf;
  int8_t send_result = AT_RESPONSE_ERROR;
  rt_err_t result;

  gsm_mail_buf.send_mode = GSM_MODE_CMD;
  gsm_mail_buf.result = &send_result;
  gsm_mail_buf.result_sem = rt_sem_create("m_ret", 0, RT_IPC_FLAG_FIFO);
  gsm_mail_buf.mail_data.cmd.index = command_index;
  gsm_mail_buf.mail_data.cmd.delay = delay;
  gsm_mail_buf.mail_data.cmd.cmd_data = *cmd_data;

  if (mq_gsm != NULL)
  {
    result = rt_mq_send(mq_gsm, &gsm_mail_buf, sizeof(GSM_MAIL_TYPEDEF));
    if (result == -RT_EFULL)
    {
      rt_kprintf("sms_mq is full!!!\n");
      send_result = AT_RESPONSE_ERROR;
    }
    else
    {
      rt_sem_take(gsm_mail_buf.result_sem, RT_WAITING_FOREVER);
    }
    rt_sem_delete(gsm_mail_buf.result_sem);
  }
  else
  {
    rt_kprintf("sms_mq is RT_NULL!!!\n");
    send_result = AT_RESPONSE_ERROR;
  }

  return send_result;
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

FINSH_FUNCTION_EXPORT(gsm_command, gsm_command[at_index delay *buf])
FINSH_FUNCTION_EXPORT(send_cmd_mail, gsm_command[at_index delay *buf])
#endif

