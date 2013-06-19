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
static rt_timer_t logo_led_timer;

void rt_init_thread_entry(void* parameter)
{
  /* Filesystem Initialization */
#ifdef RT_USING_DFS
#define FILE_SYSTEM_DEVICE_NAME	"flash1"
#define FILE_SYSTEM_TYPE_NAME		"elm"
  {
    /* init the device filesystem */
    dfs_init();

#ifdef RT_USING_DFS_ELMFAT
    /* init the elm chan FatFs filesystam*/
    elm_init();

    /* mount sd card fat partition 1 as root directory */
    if (dfs_mount(FILE_SYSTEM_DEVICE_NAME, "/", FILE_SYSTEM_TYPE_NAME, 0, 0) == 0)
    {
      rt_kprintf("File System initialized ok!\n");
    }
    else
    {
      extern int dfs_mkfs(const char *fs_name, const char *device_name);
    	
      if(dfs_mkfs(FILE_SYSTEM_TYPE_NAME,FILE_SYSTEM_DEVICE_NAME) == 0)
      {
        if (dfs_mount("flash1", "/", "elm", 0, 0) == 0)
        {
          rt_kprintf("File System initialized ok!\n");
        }
      }
      else
      {
        rt_kprintf("File System initialzation failed!\n");
      }	
    }
     
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
  {
    extern void photo_thread_init(void);
    extern void filesystem_test(void);
    extern void send_photo_thread_init(void);
    extern void rt_mms_thread_init(void);
		
    //		extern void picture_thread_init(void);

    //		picture_thread_init();
    //		send_photo_thread_init();
    			photo_thread_init();
    //		filesystem_test();
    //		rt_mms_thread_init();
  }
#ifdef	USE_WILDFIRE_TEST
  rt_kprintf("User Wild Fire Hardware Piatform TESE\n");
#endif
	/* re-init device driver */
	rt_device_init_all();

}


static void logo_led_timeout(void *parameters)
{
  gpio_pin_output(DEVICE_NAME_LOGO_LED, 0);
  rt_timer_stop(logo_led_timer);
  rt_timer_delete(logo_led_timer);
}      

int rt_application_init()
{
  rt_thread_t init_thread;
  rt_thread_t alarm_mail_process_thread;
  rt_thread_t sms_mail_process_thread;
  rt_thread_t mms_mail_process_thread;
  rt_thread_t gprs_mail_process_thread;
  rt_thread_t local_mail_process_thread;
  rt_thread_t gsm_process_thread;
  rt_thread_t battery_check_process_thread;

  /* alarm mail process thread */
  gsm_process_thread = rt_thread_create("gsm",
                                        gsm_process_thread_entry, RT_NULL,
                                        2048, 100, 20);
  if (gsm_process_thread != RT_NULL)
  {
    rt_thread_startup(gsm_process_thread);
  }
  
  /* alarm mail process thread */
  alarm_mail_process_thread = rt_thread_create("alarm",
                                               alarm_mail_process_thread_entry, RT_NULL,
                                               512, 101, 20);
  if (alarm_mail_process_thread != RT_NULL)
  {
    rt_thread_startup(alarm_mail_process_thread);
  }
  
  /* sms mail process thread */
  sms_mail_process_thread = rt_thread_create("sms",
                                             sms_mail_process_thread_entry, RT_NULL,
                                             2048, 105, 20);
  if (sms_mail_process_thread != RT_NULL)
  {
    rt_thread_startup(sms_mail_process_thread);
  }
  
  /* mms mail process thread */
  mms_mail_process_thread = rt_thread_create("mms",
                                             mms_mail_process_thread_entry, RT_NULL,
                                             1024, 107, 20);
  if (mms_mail_process_thread != RT_NULL)
  {
    rt_thread_startup(mms_mail_process_thread);
  }
  
  /* gprs mail process thread */
  gprs_mail_process_thread = rt_thread_create("gprs",
                                              gprs_mail_process_thread_entry, RT_NULL,
                                              1024, 106, 20);
  if (gprs_mail_process_thread != RT_NULL)
  {
    rt_thread_startup(gprs_mail_process_thread);
  }
  
  /* local mail process thread */
  local_mail_process_thread = rt_thread_create("local",
                                               local_mail_process_thread_entry, RT_NULL,
                                               1024, 102, 20);
  if (local_mail_process_thread != RT_NULL)
  {
    rt_thread_startup(local_mail_process_thread);
  }
  
  /* battery check process thread */
  battery_check_process_thread = rt_thread_create("bt_check",
                                                  battery_check_process_thread_entry, RT_NULL,
                                                  1024, 120, 20);
  if (battery_check_process_thread != RT_NULL)
  {
    rt_thread_startup(battery_check_process_thread);
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
  {
		rt_thread_startup(init_thread);
  }
    
  /* get rtc clock */
  rtc_device = rt_device_find("rtc");
  if (rtc_device == RT_NULL)
  {
    rt_kprintf("rtc_device is not exist!!!");
  }

  // open logo led
  gpio_pin_output(DEVICE_NAME_LOGO_LED, 1);
  logo_led_timer = rt_timer_create("tr_led",
                                   logo_led_timeout,
                                   RT_NULL,
                                   1000,
                                   RT_TIMER_FLAG_ONE_SHOT);
//  rt_timer_start(logo_led_timer);
  // initialing status
//  lock_output(GATE_UNLOCK);//unlock
  return 0;
}

/*@}*/
