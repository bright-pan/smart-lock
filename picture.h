#ifndef __PHOTO_H__
#define	 __PHOTO_H__
#include "rtthread.h"
#include "rtdevice.h"

#define CAMERA_BUFFER_LEN							2000

#define CM_GET_LEN_ERROR							0X01
#define CM_DEAL_DATA_ERROR						0X02

/* Camera device struct */
struct _camera_
{
	rt_device_t device;
	rt_device_t glint_led;
	rt_device_t power;
	rt_uint32_t	surplus;
	rt_uint32_t	addr;
	rt_uint32_t	page;
	rt_base_t		size;
	rt_uint8_t		time;
	rt_uint8_t		error;
	rt_uint8_t	data[CAMERA_BUFFER_LEN];
};
typedef struct _camera_*	camera_t;


/* photograph Message Queuing */
struct _photo_mb_
{
	rt_uint32_t time;
	const char*	name1;
	const char* name2;
};
typedef struct _photo_mb_* pic_mb_t;

void photo_thread_init(void);

#endif

