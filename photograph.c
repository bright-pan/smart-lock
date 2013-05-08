#include "photograph.h"

rt_sem_t	graph_sem;//photograph sem



void com2_rx_indicate(rt_device_t device,rt_size_t len)
{
	RT_ASSERT(graph_sem != RT_NULL);
	if(len >= 6)
	rt_sem_release(graph_sem);
}

rt_uint8_t rec_data[10];
void photograph_thread_entry(void *arg)
{
	rt_device_t device;

	device = rt_device_find("usart2");
	if(device == RT_NULL)
	{
		rt_kprintf("device error\n");
	}
	rt_device_set_rx_indicate(device,com2_rx_indicate);
	while(1)
	{	
		rt_sem_take(graph_sem,RT_WAITING_FOREVER);
		rt_kprintf("\n thread graph");
		rt_device_read(device,0,rec_data,6);
	}
}

void photograph_thread_init(void)
{
	rt_thread_t	id;//threasd id
	rt_err_t	sem_result;

	
	id = rt_thread_create("graph",photograph_thread_entry,RT_NULL,1024*5,20,30);
	if(id == RT_NULL)
	{
		rt_kprintf("graph thread create fail\n");
		return ;
	}
	rt_thread_startup(id);

	graph_sem = rt_sem_create("sxt",0,RT_IPC_FLAG_FIFO);
	if(graph_sem == RT_NULL)
	{
		
	}
}














