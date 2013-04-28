/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */

/**
 * @addtogroup STM32
 */
/*@{*/

#include <board.h>
#include <rtthread.h>

#ifdef RT_USING_DFS
/* dfs init */
#include <dfs_init.h>
/* dfs filesystem:ELM filesystem init */
#include <dfs_elm.h>
/* dfs Filesystem APIs */
#include <dfs_fs.h>

#include "spiflash.h"
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

#include "led.h"

ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t led_stack[ 512 ];
static struct rt_thread led_0_thread;
static void led_0_thread_entry(void* parameter)
{
  unsigned int count=0;
  rt_hw_led_init();
  while (1)
  {
    /* led1 on */
#ifndef RT_USING_FINSH
    rt_kprintf("led on, count : %d\r\n",count);
#endif
    count++;
    rt_hw_led_on(0);
    rt_thread_delay( RT_TICK_PER_SECOND/2 ); /* sleep 0.5 second and switch to other thread */
    /* led1 off */
#ifndef RT_USING_FINSH
    rt_kprintf("led off\r\n");
#endif
    rt_hw_led_off(0);
    rt_thread_delay( RT_TICK_PER_SECOND/2 );
  }
}



static void led_1_thread_entry(void* parameter)
{
  unsigned int count=0;

  rt_hw_led_init();
  
  while (1)
  {
    /* led1 on */
#ifndef RT_USING_FINSH
    rt_kprintf("led on, count : %d\r\n",count);
#endif
    count++;
    
    rt_hw_led_on(1);
    rt_thread_delay( RT_TICK_PER_SECOND/2 ); /* sleep 0.5 second and switch to other thread */
    /* led1 off */
#ifndef RT_USING_FINSH
    rt_kprintf("led off\r\n");
#endif
    rt_hw_led_off(1);
    rt_thread_delay( RT_TICK_PER_SECOND/2 );
  }
}

#define MAX_SEM 5

rt_uint32_t array[MAX_SEM];

struct rt_semaphore sem_lock;
struct rt_semaphore sem_empty, sem_full;

rt_uint32_t set, get;


void rt_producer_thread_entry(void* parameter) {
  
  rt_int32_t cnt = 0;

  while (cnt < 100) {
    rt_sem_take(&sem_empty, RT_WAITING_FOREVER);
    rt_sem_take(&sem_lock, RT_WAITING_FOREVER);
    array[set%MAX_SEM] = cnt + 1;
#ifndef RT_USING_FINSH
    rt_kprintf("the producer generates a number: %d\n", array[set%MAX_SEM]);
#endif
    set++;
    rt_sem_release(&sem_lock);
    rt_sem_release(&sem_full);
    cnt++;
               
    rt_thread_delay(200);
  }
#ifndef RT_USING_FINSH
  rt_kprintf("the producer exit!\n");
#endif
}

void rt_consumer_thread_entry(void* parameter) {
  
  rt_uint32_t sum;

  while (1) {
    rt_sem_take(&sem_full, RT_WAITING_FOREVER);

    rt_sem_take(&sem_lock, RT_WAITING_FOREVER);
    sum += array[get%MAX_SEM];
#ifndef RT_USING_FINSH
    rt_kprintf("the consumer [%d] get a number: %d\n", no, array[set%MAX_SEM]);
#endif
    get++;
    rt_sem_release(&sem_lock);
          
    rt_sem_release(&sem_empty);
    if (get >= 100)
      break;
    rt_thread_delay(10);
  }
#ifndef RT_USING_FINSH  
  rt_kprintf("the consumer[%d] sum is %d \n", no, sum);
  rt_kprintf("the producer exit!\n");
#endif
}


void rt_init_thread_entry(void* parameter)
{
  /* Filesystem Initialization */
#ifdef RT_USING_DFS
  {
  	static int result;
  	
    /* init the device filesystem */
    dfs_init();

#ifdef RT_USING_DFS_ELMFAT
    /* init the elm chan FatFs filesystam*/
	elm_init();
	rt_spi_flash_init();
	result = dfs_mount("w25", "/", "elm", 0, 0);
	/* mount sd card fat partition 1 as root directory */
	rt_kprintf("%d\n",result);
	if (result == 0)
	{
	  rt_kprintf("File System initialized!\n");
	}
	else
	{
	rt_kprintf("File System initialzation failed!\n");

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
}

int rt_application_init()
{
  rt_thread_t init_thread;
  rt_thread_t led_1_thread;
  rt_thread_t producer_thread;
  rt_thread_t consumer_thread;
     
  rt_err_t result;

  /*
  result = rt_thread_init(&led_0_thread,
                          "led_0",
                          led_0_thread_entry, RT_NULL,
                          (rt_uint8_t*)&led_stack[0], sizeof(led_stack), 20, 5);
  if (result == RT_EOK) {
    rt_thread_startup(&led_0_thread);
  }


  led_1_thread = rt_thread_create("led_1",
                                  led_1_thread_entry, RT_NULL,
                                  512, 20, 5);
  if (led_1_thread != RT_NULL) {
    rt_thread_startup(led_1_thread);
  }

  rt_sem_init(&sem_lock, "lock", 1, RT_IPC_FLAG_FIFO);
  rt_sem_init(&sem_empty, "empty", MAX_SEM, RT_IPC_FLAG_FIFO);
  rt_sem_init(&sem_full, "full", 0, RT_IPC_FLAG_FIFO);

     
  producer_thread = rt_thread_create("p", rt_producer_thread_entry, RT_NULL, 512, 15, 5);
  if (producer_thread != RT_NULL) {
    rt_thread_startup(producer_thread);
  }
  
  consumer_thread = rt_thread_create("c1", rt_consumer_thread_entry, (void *)1, 512, 18, 5);
  if (consumer_thread != RT_NULL) {
    rt_thread_startup(consumer_thread);
  }
     
  consumer_thread = rt_thread_create("c2", rt_consumer_thread_entry, (void *)2, 512, 20, 5);
  if (consumer_thread != RT_NULL) {
    rt_thread_startup(consumer_thread);
  }
     
  consumer_thread = rt_thread_create("c3", rt_consumer_thread_entry, (void *)3, 512, 22, 5);
  if (consumer_thread != RT_NULL) {
    rt_thread_startup(consumer_thread);
  }
  */
     
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

  return 0;
}

/*@}*/
