#include "sendphoto.h"



static void send_photo_thread_enter(void *arg)
{
	struct cm_send_mq recv_mq;
	int file_id;
	volatile int read_result = 1;
	while(1)
	{
//		rt_mq_recv(photo_ok_mq,&recv_mq,sizeof(recv_mq),RT_WAITING_FOREVER);
		rt_kprintf("recv_mq.error %d\n",recv_mq.error);

		file_id = open(recv_mq.name1,O_RDONLY,0);

		read_result = 1;
		while(read_result)
		{
			rt_uint8_t data;
			read_result = read(file_id,&data,1);
			rt_kprintf("%c",data);
		}
		close(file_id);
		
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
	//rt_thread_startup(thread_id);
}





