#include "picture.h"
#include <dfs_init.h>
#include <dfs_elm.h>
#include <dfs_fs.h>
#include "dfs_posix.h"

#define CAMERA_BUFFER_LEN							2000
//rt_uint8_t data_buffer[CAMERA_BUFFER_LEN];
struct _camera_
{
	rt_device_t device;
	rt_uint32_t	surplus;
	rt_uint32_t	addr;
	rt_uint32_t	page;
	rt_uint32_t	size;
	rt_uint8_t	data[CAMERA_BUFFER_LEN];
};
typedef struct _camera_*	camera_t;

struct _pic_mailbox_
{
	rt_uint8_t	time;
	rt_uint8_t*file_name;
};
typedef struct _pic_mailbox_* pic_mailbox;



const rt_uint8_t reset_camrea_cmd[] =	{0x56,0x00,0x26,0x00};
const rt_uint8_t switch_frame_cmd[] = {0x56,0x00,0x36,0x01,0x03};
const rt_uint8_t update_camrea_cmd[] = 	{0x56,0x00,0x36,0x01,0x02};
const rt_uint8_t stop_cur_frame_cmd[] =	{0x56,0x00,0x36,0x01,0x00};
const rt_uint8_t stop_next_frame_cmd[] = {0x56,0x00,0x36,0x01,0x01};
rt_uint8_t get_picture_len_cmd[] 	=	{0x56,0x00,0x34,0x01,0x00};
rt_uint8_t	recv_data_respond_cmd[] = {0x76,0x00,0x32,0x00,0x00};
rt_uint8_t get_picture_fbuf_cmd[16] = 		{0x56,0x00,0x32,0x0C,0x00,0x0a,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00};




rt_sem_t		com2_graph_sem = RT_NULL;//photograph sem
rt_sem_t		picture_sem = RT_NULL;
rt_mailbox_t	picture_mailbox = RT_NULL;
rt_mq_t			picture_mq = RT_NULL;


volatile rt_uint32_t release_com2_data = 0;
rt_timer_t		pic_timer = RT_NULL;
rt_uint32_t	pic_timer_value = 0;

//test
volatile rt_uint32_t	buffer_len_test;
volatile rt_bool_t		run = 0;


void printf_camera(camera_t camera)
{
	rt_kprintf("\ncamera->size = %d\n",camera->size);
	rt_kprintf("\ncamera->page = %d\n",camera->page);
	rt_kprintf("\ncamera->surplus = %d\n",camera->surplus);
	rt_kprintf("\ncamera->addr = %d\n",camera->addr);
	
}

static rt_err_t com2_picture_data_rx_indicate(rt_device_t device,rt_size_t len)
{
	RT_ASSERT(com2_graph_sem != RT_NULL);

	buffer_len_test = len;
	if(len == release_com2_data)
	{
		rt_sem_release(com2_graph_sem);
	}
	
	return RT_EOK;
}


static void uint32_to_array(rt_uint8_t *array,rt_uint32_t data)
{
	*array = (data>>24) & 0x000000ff;	
	array++;
	*array = (data>>16) & 0x000000ff;
	array++;
	*array = (data>>8) & 0x000000ff;
	array++;
	*array = (data>>0) & 0x000000ff;
}


void pic_timer_test(void *arg)
{
	pic_timer_value++;
}


void test_recv_data(rt_uint8_t data[],rt_uint32_t len)
{	
	 rt_uint32_t i = 0;
	
	while(i != len)
	{
		rt_kprintf("%x ",data[i]);
		i++;
	}
}

void test_run(void)
{
	
	while(run);run = 1;
}

void com2_release_buffer(camera_t camera)
{
	rt_uint8_t length;
	rt_err_t	result;
	
	release_com2_data = 1;
	while(1)
	{	
		result = rt_sem_take(com2_graph_sem,100);
		if(-RT_ETIMEOUT == result)
		{	
			length = rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);
				
			test_recv_data(camera->data,length);
			
			rt_kprintf("length = %d\n",length);
			break;
		}
	}
}

void picture_update_frame(camera_t camera)
{
 	volatile rt_size_t length = 0;
	
	rt_device_write(camera->device,0,update_camrea_cmd,sizeof(update_camrea_cmd));	
	
	rt_thread_delay(10);
	
	length = rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);
	
//	test_recv_data(camera->data,length);
//	test_run();
}
void picture_stop_cur_frame(camera_t camera)
{
	rt_uint8_t length = 0;
	
	rt_device_write(camera->device,0,stop_cur_frame_cmd,sizeof(stop_cur_frame_cmd)); 
	
	rt_thread_delay(10);
	
	length = rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);
//	test_recv_data(camera->data,length);
//	test_run();
}


void picture_stop_next_frame(camera_t camera)
{
	rt_uint8_t length = 0;

	rt_device_write(camera->device,0,stop_next_frame_cmd,sizeof(stop_next_frame_cmd)); 
	
	rt_thread_delay(10);
	
	length = rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);
	test_recv_data(camera->data,length);
}


void picture_switch_frame(camera_t camera)
{
	rt_uint8_t length = 0;

	rt_device_write(camera->device,0,switch_frame_cmd,sizeof(switch_frame_cmd)); 
	
	rt_thread_delay(10);
	
	length = rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);
}
void picture_get_size(camera_t camera,rt_uint8_t frame_flag)
{
	rt_size_t length;
	
	if(frame_flag == 0)
	{
		get_picture_len_cmd[4] = 0;
	}
	else
	{
		get_picture_len_cmd[4] = 1;
	}
	rt_device_write(camera->device,0,get_picture_len_cmd,sizeof(get_picture_len_cmd)); 
	
	rt_thread_delay(10);
	
	length = rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);

	if(length == 9)
	{
		rt_kprintf("\npicture_recv_len = %d\n",length);
		
		test_recv_data(camera->data,9);
	}
	else
	{
		rt_thread_delay(100);
		
		rt_kprintf("\npicture_recv_len = %d\n",length);
	}
	/*calculate receive size		*/
	camera->size = (camera->data[5] << 24) |(camera->data[6] << 16) |(camera->data[7] << 8) |(camera->data[8]); 
	camera->page = camera->size / CAMERA_BUFFER_LEN;
	camera->surplus = camera->size % CAMERA_BUFFER_LEN;
//	test_run();
	length = rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);

}


rt_bool_t picture_data_deal(camera_t camera,int file_id)
{
	volatile rt_err_t	sem_result = RT_EOK;
	rt_size_t	length;

	release_com2_data = CAMERA_BUFFER_LEN+10;
	camera->addr = 0;
	while(1)
	{ 
		static rt_uint32_t num = 0;

		/*get sem photograph*/
		sem_result = rt_sem_take(com2_graph_sem,30);//500ms no recevie data Data sent failure		
		if((-RT_ETIMEOUT) == sem_result)
		{
			rt_kprintf("\nsem result error: ");
			rt_kprintf("release_com2_data = %d",release_com2_data);
			rt_kprintf("buffer_len_test = %d",buffer_len_test);
			
			rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);	
			rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);	
			printf_camera(camera);
			
			rt_device_write(camera->device,0,get_picture_fbuf_cmd,sizeof(get_picture_fbuf_cmd));
			continue;
		}
		rt_device_read(camera->device,0,recv_data_respond_cmd,5);
		if(camera->page != 0)
		{
			
			length = rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);
			
			rt_device_read(camera->device,0,recv_data_respond_cmd,5);
			
			camera->addr += CAMERA_BUFFER_LEN;
			if(camera->page == 1)
			{
				release_com2_data = camera->surplus+10;
				uint32_to_array(&get_picture_fbuf_cmd[10],camera->surplus);
			}
			uint32_to_array(&get_picture_fbuf_cmd[6],camera->addr);

			rt_device_write(camera->device,0,get_picture_fbuf_cmd,sizeof(get_picture_fbuf_cmd));
		}
		else
		{
			length = rt_device_read(camera->device,0,camera->data,camera->surplus);
			
			rt_device_read(camera->device,0,recv_data_respond_cmd,5);
			camera->addr += camera->surplus;

		}
		{
			static	rt_uint16_t i = 0;
			write(file_id,camera->data,length); 
			rt_kprintf("file write %d\n",i++);
			
		}

		camera->page--;
		if(camera->addr >= camera->size)
		{
			close(file_id);
			
			rt_kprintf("\npicture_data_page = %d",camera->page);
			rt_kprintf("\npicture_recv_data_addr = %d",camera->addr);
			rt_kprintf("\npicture_recv_data_size = %d",camera->size);
			rt_kprintf("\n num = %d",num);		
			rt_kprintf("\npic_timer_value = %d",pic_timer_value);
			pic_timer_value = 0;
			return 0;
		}
	}
	return 1;
}

void picture_set_per_read_size(camera_t camera,rt_uint8_t frame_type)
{
	rt_uint32_t length;
	
	camera->addr = 0;
	
	uint32_to_array(&get_picture_fbuf_cmd[10],CAMERA_BUFFER_LEN);
	
	uint32_to_array(&get_picture_fbuf_cmd[6],camera->addr);
	if(0 == frame_type)
	{
		get_picture_fbuf_cmd[4] = 0;
	}
	else
	{
		get_picture_fbuf_cmd[4] = 1;

	}
	rt_device_write(camera->device,0,get_picture_fbuf_cmd,sizeof(get_picture_fbuf_cmd));
	
	
}
void picture_create_file_two(camera_t camera,const char *pathname1,const char *pathname2)
{
	int file_id = 0;
	
	picture_update_frame(camera);
	
	picture_stop_cur_frame(camera);
	
	rt_thread_delay(100);

	picture_switch_frame(camera);

	picture_stop_next_frame(camera);

	/*deal picture one */
	picture_get_size(camera,0);
	
	picture_set_per_read_size(camera,0);

	unlink(pathname1);

	file_id = open(pathname1,O_CREAT | O_RDWR, 0);

	if(1 == picture_data_deal(camera,file_id))
	{
		rt_kprintf("\nphotograph fail\n");
	}


	/*deal picture two*/
	picture_get_size(camera,1);
	
	picture_set_per_read_size(camera,1);
	
	unlink(pathname2);
	
	file_id = open(pathname2,O_CREAT | O_RDWR, 0);
	
	if(1 == picture_data_deal(camera,file_id))
	{
		rt_kprintf("\nphotograph fail\n");
	}
}


void picture_create_file_one(camera_t camera,const char *pathname )
{
	int file_id;

	camera->addr = 0;
	
	picture_update_frame(camera);
	
	picture_stop_cur_frame(camera);
	
	picture_get_size(camera,0);

	printf_camera(camera);
	
	uint32_to_array(&get_picture_fbuf_cmd[10],CAMERA_BUFFER_LEN);
	
	uint32_to_array(&get_picture_fbuf_cmd[6],camera->addr);
	
	rt_device_write(camera->device,0,get_picture_fbuf_cmd,sizeof(get_picture_fbuf_cmd));
	
	unlink(pathname);
	
	file_id = open(pathname,O_CREAT | O_RDWR, 0);
	
	if(1 == picture_data_deal(camera,file_id))
	{
		rt_kprintf("\nphotograph fail\n");
	}

}

void picture_struct_init(camera_t camera,rt_device_t set_device)
{
	camera->addr = 0;
	camera->device = set_device;
	camera->page = 0;
	camera->size = 0;
	camera->surplus = 0;
//	camera->data = data_buffer;
}
void picture_thread_entry(void *arg)
{
	rt_device_t device = RT_NULL;
	camera_t	 picture = RT_NULL;
	struct _pic_mailbox_	recv_mb;

	(camera_t)picture = rt_malloc(sizeof(struct _camera_));
	
	pic_timer = rt_timer_create("pic",pic_timer_test,RT_NULL,10,RT_TIMER_FLAG_PERIODIC);
	
	rt_timer_start(pic_timer);
	
	device = rt_device_find("usart2");
	
	if(device == RT_NULL)
	{
		rt_kprintf("device error\n");
	}
	rt_device_set_rx_indicate(device,com2_picture_data_rx_indicate);

	picture_struct_init(picture,device);
	
	com2_release_buffer(picture);
	while(1)
	{
//			rt_mb_recv(picture_mailbox,(rt_uint32_t *)(&recv_mb),RT_WAITING_FOREVER);
//		rt_sem_take(picture_sem,RT_WAITING_FOREVER);
//		picture_create_file_one(picture,"/5.jpg");
//		test_run();
//		picture_create_file_two(picture,"/1.jpg","/2.jpg");

			rt_mq_recv(picture_mq,&recv_mb,sizeof(recv_mb),RT_WAITING_FOREVER);
		
			rt_kprintf("\n%d\n%s",recv_mb.time,recv_mb.file_name);
			picture_create_file_one(picture,recv_mb.file_name);
	}
	free((camera_t)picture);
}


void picture_thread_init(void)
{
	rt_thread_t	id;//threasd id

	
	id = rt_thread_create("graph",picture_thread_entry,RT_NULL,1024*6,20,30);
	if(RT_NULL == id )
	{
		rt_kprintf("graph thread create fail\n");
		return ;
	}
	rt_thread_startup(id);

	com2_graph_sem = rt_sem_create("sxt",0,RT_IPC_FLAG_FIFO);
	if(RT_NULL == com2_graph_sem)
	{
		rt_kprintf(" \"sxt\" sem create fail\n");
	}
	picture_sem	= rt_sem_create("z1x",0,RT_IPC_FLAG_FIFO);
	if(RT_NULL == picture_sem)
	{
		rt_kprintf(" \"z1x\" sem create fail \n");
	}
	
	picture_mailbox = rt_mb_create("pci_info",20,RT_IPC_FLAG_FIFO);
	if(RT_NULL == picture_mailbox)
	{
		rt_kprintf(" \"z1x\" mailbox create fial\n");
	}

	picture_mq = rt_mq_create("pic_mq",128,3,RT_IPC_FLAG_FIFO);
	if(RT_NULL == picture_mq)
	{
		rt_kprintf(" \"pic_mq\" mq create fial\n");
	}
}



#ifdef RT_USING_FINSH
#include <finsh.h>
void picture()
{
	rt_sem_release(picture_sem);
}

FINSH_FUNCTION_EXPORT(picture, picture());

void reset()
{
	rt_kprintf("%d",buffer_len_test);
	run = 0;
}

FINSH_FUNCTION_EXPORT(reset, reset());

void mailbox(rt_uint8_t time, rt_uint8_t *name)
{
	struct _pic_mailbox_ pic_mb = {0,"wfasdf"};
	
//	(pic_mailbox)pic_mb = rt_malloc(sizeof(struct _pic_mailbox_));
//	pic_mb.time = time;
//	pic_mb.file_name = name;
	rt_mb_send(picture_mailbox,(rt_uint32_t )(&pic_mb));
}
FINSH_FUNCTION_EXPORT(mailbox, mailbox(time,name));


void mq(rt_uint8_t time,rt_uint8_t *file_name)
{
	struct _pic_mailbox_ pic_mb;

	pic_mb.file_name = file_name;
	pic_mb.time = time;
	
	rt_mq_send(picture_mq,&pic_mb,sizeof(pic_mb));
}
FINSH_FUNCTION_EXPORT(mq, mq(time,name));

#endif


