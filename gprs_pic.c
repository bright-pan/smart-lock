#include "gprs_pic.h"

struct gprs_pic
{
	rt_uint16_t 	length;
	rt_uint8_t 		cmd;
	rt_uint8_t 		order;
	rt_uint8_t		page;
	rt_bool_t			form;
	rt_bool_t 		DPI;
	rt_uint32_t		pic_size;
	rt_uint8_t*		per_data;
};




