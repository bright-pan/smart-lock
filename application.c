/*********************************************************************
 * Filename:      application.c
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-09 09:56:05
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include <board.h>
#include <rtthread.h>

#ifdef RT_USING_DFS
/* dfs init */
#include <dfs_init.h>
/* dfs filesystem:ELM filesystem init */
#include <dfs_elm.h>
/* dfs Filesystem APIs */
#include <dfs_fs.h>
#endif

#ifdef RT_USING_LWIP
#include <lwip/sys.h>
#include <lwip/api.h>
#include <netif/ethernetif.h>
#endif

#ifdef RT_USING_RTGUI
#include <rtgui/rtgui.h>
#include <rtgui/rtgui_server.h>
#include <rtgui/rtgui_system.h>
#include <rtgui/driver.h>
#endif


rt_device_t rtc_device;


void rt_init_thread_entry(void* parameter)
{
  /* Filesystem Initialization */
#ifdef RT_USING_DFS
  {
    /* init the device filesystem */
    dfs_init();

#ifdef RT_USING_DFS_ELMFAT
    /* init the elm chan FatFs filesystam*/
    elm_init();

    /* mount sd card fat partition 1 as root directory */
    if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)
    {
      rt_kprintf("File System initialized!\n");
    }
    else
      rt_kprintf("File System initialzation failed!\n");
#endif
  }
#endif

  /* LwIP Initialization */
#ifdef RT_USING_LWIP
  {
    extern void lwip_sys_init(void);

    /* register ethernetif device */
    eth_system_device_init();

#ifdef STM32F10X_CL
    rt_hw_stm32_eth_init();
#else
    /* STM32F103 */
#if STM32_ETH_IF == 0
    rt_hw_enc28j60_init();
#elif STM32_ETH_IF == 1
    rt_hw_dm9000_init();
#endif
#endif

    /* re-init device driver */
    rt_device_init_all();

    /* init lwip system */
    lwip_sys_init();
    rt_kprintf("TCP/IP initialized!\n");
  }
#endif

#ifdef RT_USING_RTGUI
  {
    extern void rtgui_system_server_init(void);
    extern void rt_hw_lcd_init();
    extern void rtgui_touch_hw_init(void);

    rt_device_t lcd;

    /* init lcd */
    rt_hw_lcd_init();

    /* init touch panel */
    rtgui_touch_hw_init();

    /* re-init device driver */
    rt_device_init_all();

    /* find lcd device */
    lcd = rt_device_find("lcd");

    /* set lcd device as rtgui graphic driver */
    rtgui_graphic_set_device(lcd);

    /* init rtgui system server */
    rtgui_system_server_init();
  }
#endif /* #ifdef RT_USING_RTGUI */
}

int rt_application_init()
{
  rt_thread_t init_thread;
  rt_thread_t alarm_mail_process_thread;
  rt_thread_t sms_mail_process_thread;
  rt_thread_t gprs_mail_process_thread;
  rt_thread_t local_mail_process_thread;


  /* alarm mail process thread */
  alarm_mail_process_thread = rt_thread_create("alarm",
                                  alarm_mail_process_thread_entry, RT_NULL,
                                  512, 20, 20);
  if (alarm_mail_process_thread != RT_NULL)
  {
    rt_thread_startup(alarm_mail_process_thread);
  }
  /* sms mail process thread */
  sms_mail_process_thread = rt_thread_create("sms",
                                  sms_mail_process_thread_entry, RT_NULL,
                                  1024, 22, 20);
  if (sms_mail_process_thread != RT_NULL)
  {
    rt_thread_startup(sms_mail_process_thread);
  }
  /* gprs mail process thread */
  gprs_mail_process_thread = rt_thread_create("sms",
                                  gprs_mail_process_thread_entry, RT_NULL,
                                  1024, 23, 20);
  if (gprs_mail_process_thread != RT_NULL)
  {
    rt_thread_startup(gprs_mail_process_thread);
  }
  /* local mail process thread */
  local_mail_process_thread = rt_thread_create("local",
                                  local_mail_process_thread_entry, RT_NULL,
                                  1024, 21, 20);
  if (local_mail_process_thread != RT_NULL)
  {
    rt_thread_startup(local_mail_process_thread);
  }
  
  /* init init_thread */
#if (RT_THREAD_PRIORITY_MAX == 32)
  init_thread = rt_thread_create("init",
                                 rt_init_thread_entry, RT_NULL,
                                 2048, 8, 20);
#else
  init_thread = rt_thread_create("init",
                                 rt_init_thread_entry, RT_NULL,
                                 2048, 80, 20);
#endif

  if (init_thread != RT_NULL)
    rt_thread_startup(init_thread);

  /* get rtc clock */
  rtc_device = rt_device_find("rtc");
  if (rtc_device == RT_NULL)
  {
    rt_kprintf("rtc_device is not exist!!!");
  }

  return 0;
}

/*@}*/
