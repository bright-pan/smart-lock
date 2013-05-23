/*********************************************************************
 * Filename:      testprintf.c
 *
 * Description:	test file test ok after delete
 *
 * Author:        wangzw <wangzw@yuettak.com>
 * Created at:    2013-05-17 14:22:03
 *                
 * Modify:
 *
 * 
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/
#include "testprintf.h"


void printf_write_to_file(void)
{
	static rt_uint8_t i = 0;
	
	i++;
	i %= 5;
	switch(i)
	{
		case 0:
		{
			rt_kprintf("\rWrite Data To File .    ");
			break;
		};
		case 1:
		{
			rt_kprintf("\rWrite Data To File ..   ");
			break;
		};
		case 2:
		{
			rt_kprintf("\rWrite Data To File ...  ");
			break;
		};
		case 3:
		{
			rt_kprintf("\rWrite Data To File ....");
			break;
		};
		case 4:
		{
			rt_kprintf("\rWrite Data To File .....");
			break;
		};
	}

}
void printf_camera(camera_dev_t camera)
{
	rt_kprintf("\ncamera->size    = %d\n",camera->size);
	rt_kprintf("camera->page    = %d\n",camera->page);
	rt_kprintf("camera->surplus = %d\n",camera->surplus);
	rt_kprintf("camera->addr    = %d\n",camera->addr);
	rt_kprintf("camera->error   = %d\n",camera->error);
	rt_kprintf("camera->time    = %d\n",camera->time);
	rt_kprintf("camera->device    = %s\n",camera->device->parent.name);
	rt_kprintf("camera->glint_led = %s\n",camera->glint_led->parent.name);
	rt_kprintf("camera->power     = %s\n",camera->power->parent.name);
	
}








#ifdef RT_USING_FINSH
#include <finsh.h>
/*  modification terminal display  */
void terminal()
{
	rt_kprintf("\033[1;40;32m");
}
FINSH_FUNCTION_EXPORT(terminal,terminal());


#endif

