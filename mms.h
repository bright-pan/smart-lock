#ifndef	__MMS_H__
#define __MMS_H__
#include "rtthread.h"
#include "rtdevice.h"
#include <dfs_init.h>
#include <dfs_elm.h>
#include <dfs_fs.h>
#include "dfs_posix.h"

#define PER_MMC_PIC_MAXNUM							5
#define MOBILE_NO_NUM										10
#define MMS_USART_DEVICE_NAME						"g_usart"
#define MMS_ERROR_OK										0


struct mms_dev
{
	rt_device_t usart;														//mms device use usart
	rt_size_t		pic_size[PER_MMC_PIC_MAXNUM];			//one mms picture 1,picture 2,picture 3...of size
	const char	*pic_name[PER_MMC_PIC_MAXNUM];		//one mms picture 1,picture 2,picture 3...of name
	const char	*title;														//one mms of title
	const char	*text;														//one mms of text 
	rt_uint8_t	number;														//per mms picture sum
	rt_uint8_t  *mobile_no[MOBILE_NO_NUM];				//send address
	rt_uint8_t	error;														//mms error result
};
typedef struct mms_dev* mms_dev_t;


void rt_mms_thread_init(void);


#endif





