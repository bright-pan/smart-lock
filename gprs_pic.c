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

void gprs_flie_deal_entry(void *arg)
{
	while(1)
	{

	}
}


void gprs_file_deal_init(void)
{
	volatile rt_thread_t id = RT_NULL;

	id = rt_thread_create("GSPF",
												gprs_flie_deal_entry,
												RT_NULL,
												1024*1,
												120,
												20);

	if(id != RT_NULL)
	{
		rt_thread_startup(id);
	}
}


