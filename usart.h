/*
	文件作用:	实现串口驱动框架的硬件接口函数
 */

#ifndef __USART_H__
#define __USART_H__

#include <rthw.h>
#include <rtthread.h>

void rt_hw_serial1_register(void);
void rt_hw_serial2_register(void);

#endif
