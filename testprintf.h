#ifndef	__TESTPRINTF_H__
#define		__TESTPRINTF_H__
#include "rtthread.h"
#include "photo.h"
#include "mms_dev.h"


#define USE_DEBUG_PRINTF

#ifdef	USE_DEBUG_PRINTF

#endif

void printf_write_to_file(void);
void printf_camera(camera_dev_t camera);
void mms_info(mms_dev_t mms);
void printf_loop_string(rt_uint8_t *buffer,rt_uint32_t size);
void printf_dev_data(rt_device_t dev,rt_uint8_t *buffer);
void printf_format_string(rt_uint8_t *addr,rt_uint8_t format,rt_uint8_t len);


#endif


