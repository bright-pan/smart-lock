#ifndef __PHOTO_H__
#define	 __PHOTO_H__
#include "rtthread.h"
#include "rtdevice.h"



#define CM_BUFFER_LEN									2000
#define CM_SEM_BUF_WAIT_TIME					100
#define CM_OUTTIME_NUM								10
#define CM_CONTINUOUS_PHOTO_TIME			1000

/*		Camera Photograph Error type				*/
#define CM_RUN_DEAL_OK								0X00
#define CM_GET_LEN_ERROR							0X01
#define CM_DEAL_DATA_ERROR						0X02
#define CM_RECV_OUT_TIME							0X04



/*--------------Camera Data Type ------------*/


/*		 Camera Device Struct		 */
struct camera_dev
{
	rt_device_t device;
	rt_device_t glint_led;
	rt_device_t power;
	rt_uint32_t	surplus;
	rt_uint32_t	addr;
	rt_uint32_t	page;
	rt_base_t		size;
	rt_uint32_t	time;
	rt_uint8_t	error;
	rt_uint8_t	data[CM_BUFFER_LEN];
};
typedef struct camera_dev*	camera_dev_t;


/*		Strike Photograph Message Queuing 		*/
struct cm_recv_mq
{
	rt_uint32_t	time;
	const char*	name1;
	const char* name2;
};
typedef struct cm_recv_mq* cm_recv_mq_t;

/*		Photograph OK Message Queuing		*/
struct cm_send_mq
{
	const char *name1;
	const char *name2;
	rt_uint8_t	error;		
};	
typedef struct cm_send_mq cm_send_mq_t;



extern rt_mq_t			photo_ok_mq;			//photo finish

void photo_thread_init(void);

#endif
 
