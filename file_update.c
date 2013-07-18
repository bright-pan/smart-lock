#include "file_update.h"
#include <dfs.h>
#include <dfs_posix.h>

#define FILE_WRITE_FLASH_BUFFER_SIZE	4096//sst25v16b min block is 4k


/*******************************************************************************
* Description:flash private function write sst25vf016 data 
*                  
* Input	:file name , start write addr
* Rturen:0 write file ok ;	1 write file fail
*******************************************************************************/
rt_uint8_t file_write_flash_addr(const char *file_name,
                                rt_uint32_t addr)
{
	int           FileId;
	rt_uint8_t		*buffer;
	rt_uint32_t   i;
	rt_uint32_t   page;
	rt_uint32_t   surplus;
	rt_uint32_t		WriteAddr;
	volatile  int    ReadSize = 0;
	struct  stat  FileInfo;

	FileId= open(file_name,O_RDONLY,0);
	if(FileId < 0)
	{
		rt_kprintf("File open fail ,possible file is none\n\n");
		
		return 1;
	}
	/* Check whether as legal address	*/
	if((addr < FLASH_START_WRITE_ADDR)||(addr % FILE_WRITE_FLASH_BUFFER_SIZE != 0))
	{
		rt_kprintf("Destination Address error \n");
		addr = FLASH_START_WRITE_ADDR;
	}
	WriteAddr = addr;
	stat(file_name,&FileInfo);
	rt_kprintf("FileInfo.st_size  %d\n",FileInfo.st_size);
	page = FileInfo.st_size / FILE_WRITE_FLASH_BUFFER_SIZE;
	rt_kprintf("page = %d",page);
	surplus = FileInfo.st_size % FILE_WRITE_FLASH_BUFFER_SIZE;
	/*  Move file data */
	buffer  = (rt_uint8_t*)rt_malloc(FILE_WRITE_FLASH_BUFFER_SIZE);
	if(buffer != RT_NULL)
	{
		rt_memset((rt_uint8_t*)buffer,0,FILE_WRITE_FLASH_BUFFER_SIZE);
	}
	else
	{	
		rt_kprintf("RAM too little file move fail");
		return 1;
	}
	for(i = 0;i < page ;i++)
	{
		ReadSize = read(FileId,buffer,FILE_WRITE_FLASH_BUFFER_SIZE);
		if(ReadSize < 0 || ReadSize > FILE_WRITE_FLASH_BUFFER_SIZE)
		{
			rt_kprintf("read file fail\n");
			break;
		}
		{
			rt_uint32_t i;

			for(i = 0;i < FILE_WRITE_FLASH_BUFFER_SIZE;i++)
			{
				rt_kprintf("%c",buffer[i]);
			}
		}
		SST25_W_BLOCK(WriteAddr,buffer,ReadSize);
		WriteAddr += ReadSize;
	}
	if(surplus != 0)
	{
		ReadSize = read(FileId,buffer,surplus);
		SST25_W_BLOCK(WriteAddr,buffer,ReadSize);
	}
	close(FileId);
	rt_free(buffer);
	
	return 0;
}


/*

void file_read_flash_addr(const char *file_name,
                          rt_uint32_t addr,
                          rt_uint32_t size)
{

}

*/

