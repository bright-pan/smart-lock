/*********************************************************************
 * Filename:    spiflash.c
 *
 * Description:    Used for file system test
 *						  
 *						   
 * Author:        wangzw <wangzw@yuettak.com>
 * Created at:    2013-05-2 9:00:00
 *                
 * Modify:
 *
 * 
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "filetest.h"
#include <dfs_init.h>
/* dfs filesystem:ELM filesystem init */
#include <dfs_elm.h>
/* dfs Filesystem APIs */
#include <dfs_fs.h>
#include "dfs_posix.h"
#define RING_BUFFER_LEN		20
const rt_uint8_t open_flie_cmd[] = "comopen";
const rt_uint8_t close_flie_cmd[] = "comclose";
const rt_uint8_t read_flie_cmd[] = "comread";
const rt_uint8_t remove_file_cmd[] = "commove";
struct ring_buffer
{
	rt_uint8_t dat[RING_BUFFER_LEN];
	rt_uint8_t head;
	rt_uint8_t tail;
};
struct ring_buffer com2_ring_buffer;
int file_id = 0;






rt_bool_t get_buffer_data(rt_uint8_t *data)
{
	if(com2_ring_buffer.head == com2_ring_buffer.tail)
	{
		return 0;
	}
	else
	{
		*data = com2_ring_buffer.dat[com2_ring_buffer.head++];
		com2_ring_buffer.head %= RING_BUFFER_LEN;
	}
  return 1;
}

rt_bool_t com_cmd_flie_deal(rt_uint8_t* rec_data,const rt_uint8_t cmd[],rt_uint8_t *cnt)
{
	rt_uint8_t* data = rec_data;
	
	if(*data == cmd[*cnt])
	{
		data++;
		(*cnt)++;
		if(cmd[*cnt] == '\0')
		{
			data += *cnt;
			*cnt = 0;
			return 1;
		}
	}
	else
	{
		*cnt = 0;
	}
	return 0;
}
void filesystem_test_thread(void *arg)
{
	int file_id;
	rt_uint8_t data[100];
	
	while(1)
	{
		rt_thread_delay(500);
		file_id = open("/photo/1.jpg",O_CREAT | O_RDWR,0);
		read(file_id,data,100);
		rt_kprintf("%s\n",data);
		close(file_id);
	}
}
void filesystem_test(void)
{
	rt_thread_t id;

	id = rt_thread_create("ftest",filesystem_test_thread,RT_NULL,1024*2,29,20);
	if(id != RT_NULL)
	{
		rt_thread_startup(id);
	}
}




#ifdef RT_USING_FINSH
#include <finsh.h>

FINSH_FUNCTION_EXPORT(unlink,unlink[path]);


void fileopen(const char *file, int flags)
{
	int flag = 0;
	if(flags & 0x01)
	{
		flag |= O_CREAT;
	}
	if(flags & 0x02)
	{
		flag |= O_RDWR;
	}
	if(flags & 0x04)
	{
		flag |= O_APPEND;
	}
	file_id = open(file,flag,0);
}

FINSH_FUNCTION_EXPORT(fileopen,fileopen[FileName ]);

void filecolse()
{
	close(file_id);
}

FINSH_FUNCTION_EXPORT(filecolse,filecolse[ ]);





#endif
















