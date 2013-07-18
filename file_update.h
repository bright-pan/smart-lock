#ifndef	__FILE_UPDATE_H__
#define __FILE_UPDATE_H__
#include "SPI_Flash.h"
#include <rtthread.h>

#define FLASH_START_WRITE_ADDR			((512-128)*4096l)//flash update data start addr


rt_uint8_t file_write_flash_addr(const char *file_name,rt_uint32_t addr);//move file go to flash block
//void file_read_flash_addr(const char *file_name,rt_uint32_t addr,rt_uint32_t size);

#endif


