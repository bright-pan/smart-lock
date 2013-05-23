#include "sendphoto.h"



static void send_photo_thread_enter(void *arg)
{
	struct cm_send_mq recv_mq;
	while(1)
	{
		rt_mq_recv(photo_ok_mq,&recv_mq,sizeof(recv_mq),RT_WAITING_FOREVER);
		rt_kprintf("recv_mq.error %d\n",recv_mq.error);

		
	}
}

void send_photo_thread_init(void)
{
	rt_thread_t thread_id = RT_NULL;

	thread_id= rt_thread_create("dealpic",send_photo_thread_enter,RT_NULL,1024*4,29,10);
	if(RT_NULL == thread_id)
	{
		rt_kprintf("\"dealpic\" create fial\n");
		return ;
	}
	rt_thread_startup(thread_id);
}





