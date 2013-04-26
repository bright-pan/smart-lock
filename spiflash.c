/*********************************************************************
 * Filename:    spiflash.c
 *
 * Description:    Tihs flie realize spi flash device drive
 *						   hardware chip is w25q16,furnish filesystem
 *						   using.
 * Author:        wangzw <wangzw@yuettak.com>
 * Created at:    2013-04-26 12:00:00
 *                
 * Modify:
 *
 * 
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "spiflash.h"



/* hardware define ---------------------------------------------------------*/

#define SPI1_SCK_PIN											GPIO_Pin_5;
#define	 SPI1_MISO_PIN										GPIO_Pin_6
#define SPI1_MOSI_PIN										GPIO_Pin_7
#define SPI1_GPIO_PORT									GPIOA
#define SPI1_APB2_CLOCK								RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOA

#define SPI1_CS_PIN											GPIO_Pin_4
#define SPI1_CS_PORT										GPIOA
#define SPI1_CS_CLOCK									RCC_APB2Periph_GPIOA

#define SPI1_BUS_NAME									("spi1")
#define SPI1_CS_NAME										("flash")
#define FLASH_DEVICE_NAME							("w25")

#define SPI_FLASH_PageSize      					256
#define SPI_FLASH_PerWritePageSize      	256

/* Private define ------------------------------------------------------------*/
#define W25X_WriteEnable		     		 				0x06 
#define W25X_WriteDisable		      					0x04 
#define W25X_ReadStatusReg		    				0x05 
#define W25X_WriteStatusReg		    				0x01 
#define W25X_ReadData			        					0x03 
#define W25X_FastReadData		      					0x0B 
#define W25X_FastReadDual		     					0x3B 
#define W25X_PageProgram		      					0x02 
#define W25X_BlockErase			      					0xD8 
#define W25X_SectorErase		      					0x20 
#define W25X_ChipErase			      					0xC7 
#define W25X_PowerDown			      					0xB9 
#define W25X_ReleasePowerDown	   				0xAB 
#define W25X_DeviceID			        					0xAB 
#define W25X_ManufactDeviceID   					0x90 
#define W25X_JedecDeviceID		    				0x9F 

#define WIP_Flag                  							0x01  //Write In Progress (WIP) flag 
#define Dummy_Byte                						0xFF  //moke needful read clock




struct rt_spi_configuration spi1_configuer= 
{
	RT_SPI_MODE_3 | RT_SPI_MSB,
	8,
	0,
	18000000
};




void rt_hw_spi_init(void)
{
	{
		static struct stm32_spi_bus stm32_spi;				
		GPIO_InitTypeDef gpio_initstructure;

		RCC_APB2PeriphClockCmd(SPI1_APB2_CLOCK | SPI1_CS_CLOCK,ENABLE);

		gpio_initstructure.GPIO_Mode = GPIO_Mode_AF_PP;
		gpio_initstructure.GPIO_Speed = GPIO_Speed_50MHz;
		gpio_initstructure.GPIO_Pin = SPI1_MISO_PIN | SPI1_MOSI_PIN | SPI1_SCK_PIN;
		GPIO_Init(SPI1_GPIO_PORT,&gpio_initstructure);

		stm32_spi_register(SPI1,&stm32_spi,SPI1_BUS_NAME);
	}

	{
		static struct rt_spi_device spi_device;        
		static struct stm32_spi_cs  spi_cs;	

		GPIO_InitTypeDef	gpio_initstructure;

		gpio_initstructure.GPIO_Speed = GPIO_Speed_50MHz;        
		gpio_initstructure.GPIO_Mode = GPIO_Mode_Out_PP;        /* spi21: PG10 */        
		
		spi_cs.GPIOx = SPI1_CS_PORT;        
		spi_cs.GPIO_Pin = SPI1_CS_PIN;     
		
		RCC_APB2PeriphClockCmd(SPI1_CS_CLOCK, ENABLE);        
		
		gpio_initstructure.GPIO_Pin = spi_cs.GPIO_Pin;        
		
		GPIO_SetBits(spi_cs.GPIOx, spi_cs.GPIO_Pin);     
		
		GPIO_Init(spi_cs.GPIOx, &gpio_initstructure);     

		rt_spi_bus_attach_device(&spi_device,SPI1_CS_NAME, SPI1_BUS_NAME, (void*)&spi_cs);
		
	}
}













struct flash_device 
{
	struct rt_device                			parent;      /**< RT-Thread device struct */
	struct rt_device_blk_geometry   	geometry;    /**< sector size, sector count */
	struct rt_spi_device *          		spi_device;  /**< SPI interface */
	uint32_t                        				max_clock;   /**< MAX SPI clock */
};


struct flash_device	w25q16_device;
#define 	RT_SPI_SEND_THEN_RECV(a,b)  rt_spi_send_then_recv(w25q16_device.spi_device,(a),(b),)

void spi_flash_write_enable(struct rt_spi_device *device )
{
  	rt_spi_take(device);

	rt_spi_sendrecv8(device,W25X_WriteEnable);

	rt_spi_release(device);
  
}
void spi_flash_wait_write_end(struct rt_spi_device *device)
{
	u8 flash_status;
	
	rt_spi_take(device);

	rt_spi_sendrecv8(device,W25X_ReadStatusReg);
	
  do
  {
    flash_status = rt_spi_sendrecv8(device,Dummy_Byte);	 
  }
  while ((flash_status& WIP_Flag) == SET); 

	rt_spi_release(device);
}
void spi_flash_sector_erase(struct rt_spi_device *device,u32 SectorAddr)
{
  spi_flash_write_enable(device);
  
  spi_flash_wait_write_end(device);			//test

  rt_spi_take(device);

  rt_spi_sendrecv8(device,W25X_SectorErase);

  rt_spi_sendrecv8(device,(SectorAddr & 0xFF0000) >> 16);

  rt_spi_sendrecv8(device,(SectorAddr & 0xFF00) >> 8);

  rt_spi_sendrecv8(device,SectorAddr & 0xFF);

  rt_spi_release(device);
  
  spi_flash_wait_write_end(device);
}

void spi_flash_chip_erase(struct rt_spi_device *device)
{
	spi_flash_write_enable(device);

	rt_spi_take(device);

	rt_spi_sendrecv8(device,W25X_ChipErase);

	rt_spi_release(device);

	spi_flash_wait_write_end(device);
}

void spi_flash_page_write(struct rt_spi_device *device,const u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
  spi_flash_write_enable(device);

  rt_spi_take(device);

  rt_spi_sendrecv8(device,W25X_PageProgram);

  rt_spi_sendrecv8(device,(WriteAddr & 0xFF0000) >> 16);

  rt_spi_sendrecv8(device,(WriteAddr & 0xFF00) >> 8);
  
  rt_spi_sendrecv8(device,WriteAddr & 0xFF);

  if(NumByteToWrite > SPI_FLASH_PerWritePageSize)
  {
     NumByteToWrite = SPI_FLASH_PerWritePageSize;
  }

  while (NumByteToWrite--)
  {
    rt_spi_sendrecv8(device,*pBuffer);
    pBuffer++;
  }
  rt_spi_release(device);

  spi_flash_wait_write_end(device);
}

void spi_flash_buffer_write(struct rt_spi_device *device,const u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
	u8 NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;
	
	WriteAddr &= ~0xfff;
	spi_flash_sector_erase(device,WriteAddr);
	Addr = WriteAddr % SPI_FLASH_PageSize;
	count = SPI_FLASH_PageSize - Addr;
	NumOfPage =  NumByteToWrite / SPI_FLASH_PageSize;
	NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;

  if (Addr == 0) /* WriteAddr is SPI_FLASH_PageSize aligned  */
  {
    if (NumOfPage == 0) /* NumByteToWrite < SPI_FLASH_PageSize */
    {   
      spi_flash_page_write(device,pBuffer, WriteAddr, NumByteToWrite);     
    }
    else /* NumByteToWrite > SPI_FLASH_PageSize */
    {
      while (NumOfPage--)
      {
        spi_flash_page_write(device,pBuffer, WriteAddr, SPI_FLASH_PageSize);
        WriteAddr +=  SPI_FLASH_PageSize;
        pBuffer += SPI_FLASH_PageSize;
      }
      spi_flash_page_write(device,pBuffer, WriteAddr, NumOfSingle);   
    }
  }
  else /* WriteAddr is not SPI_FLASH_PageSize aligned  */
  {
    if (NumOfPage == 0) /* NumByteToWrite < SPI_FLASH_PageSize */
    {
      if (NumOfSingle > count) /* (NumByteToWrite + WriteAddr) > SPI_FLASH_PageSize */
      {
        temp = NumOfSingle - count;
        spi_flash_page_write(device,pBuffer, WriteAddr, count);
        WriteAddr +=  count;
        pBuffer += count;
        spi_flash_page_write(device,pBuffer, WriteAddr, temp);
      }
      else
      {
        spi_flash_page_write(device,pBuffer, WriteAddr, NumByteToWrite);
      }
    }
    else /* NumByteToWrite > SPI_FLASH_PageSize */
    {
      NumByteToWrite -= count;
      NumOfPage =  NumByteToWrite / SPI_FLASH_PageSize;
      NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;

      spi_flash_page_write(device,pBuffer, WriteAddr, count);
      WriteAddr +=  count;
      pBuffer += count;

      while (NumOfPage--)
      {
        spi_flash_page_write(device,pBuffer, WriteAddr, SPI_FLASH_PageSize);
        WriteAddr +=  SPI_FLASH_PageSize;
        pBuffer += SPI_FLASH_PageSize;
      }

      if (NumOfSingle != 0)
      {
        spi_flash_page_write(device,pBuffer, WriteAddr, NumOfSingle);
      }
    }
  }
}

void spi_flash_buffer_read(struct rt_spi_device *device,u8* pBuffer, u32 ReadAddr, u16 NumByteToRead)
{
  rt_spi_take(device);

  rt_spi_send_then_recv(device,W25X_ReadData);

  rt_spi_sendrecv8(device,(ReadAddr & 0xFF0000) >> 16);

  rt_spi_sendrecv8(device,(ReadAddr& 0xFF00) >> 8);

  rt_spi_sendrecv8(device,ReadAddr & 0xFF);

  while (NumByteToRead--) 
  {
    *pBuffer = rt_spi_sendrecv8(device,Dummy_Byte);
    pBuffer++;
  }
  rt_spi_release(device);
}





static rt_err_t rt_flash_init(rt_device_t dev)
{
	struct flash_device* flash = (struct flash_device *)dev;	

	if(flash->spi_device != RT_NULL)
	{
		rt_spi_configure(flash->spi_device,&spi1_configuer);
	}
	
	return RT_EOK;
}

static rt_err_t rt_flash_open(rt_device_t dev, rt_uint16_t oflag)
{
	return RT_EOK;
}
static rt_err_t rt_flash_close(rt_device_t dev)
{
	return RT_EOK;
}

static rt_size_t rt_flash_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
	spi_flash_buffer_read(w25q16_device.spi_device,buffer,pos*4096,size*4096);
	return size;
}
static rt_size_t rt_flash_write(rt_device_t dev, rt_off_t pos,const void* buffer, rt_size_t size)
{
	spi_flash_buffer_write(w25q16_device.spi_device,buffer,pos*4096,size*4096);
	return size;
}

static rt_err_t rt_flash_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
    struct flash_device* msd = (struct flash_device*)dev;

    RT_ASSERT(dev != RT_NULL);

    if (cmd == RT_DEVICE_CTRL_BLK_GETGEOME)
    {
        struct rt_device_blk_geometry *geometry;

        geometry = (struct rt_device_blk_geometry *)args;
        if (geometry == RT_NULL) return -RT_ERROR;

        geometry->bytes_per_sector = msd->geometry.bytes_per_sector;
        geometry->block_size = msd->geometry.block_size;
        geometry->sector_count = msd->geometry.sector_count;
    }

    return RT_EOK;
}


rt_err_t rt_flash_register(const char * flash_device_name, const char * spi_device_name)
{
    rt_err_t result = RT_EOK;
    struct rt_spi_device * spi_device;

    spi_device = (struct rt_spi_device *)rt_device_find(spi_device_name);
    if(spi_device == RT_NULL)
    {
    
#ifdef RT_USING_FINSH
		rt_kprintf("spi device %s not found!\r\n", spi_device_name);
#endif

		return -RT_ENOSYS;
    }
    rt_memset(&w25q16_device, 0, sizeof(w25q16_device));
    w25q16_device.spi_device = spi_device;

    /* register sdcard device */
    w25q16_device.parent.type    = RT_Device_Class_Block;

    w25q16_device.geometry.bytes_per_sector = 4096;
    w25q16_device.geometry.sector_count = 512;
    w25q16_device.geometry.block_size = 4096;

    w25q16_device.parent.init    		= rt_flash_init;
    w25q16_device.parent.open    	= rt_flash_open;
    w25q16_device.parent.close   	= rt_flash_close;
    w25q16_device.parent.read    	= rt_flash_read;
    w25q16_device.parent.write   	= rt_flash_write;
    w25q16_device.parent.control 	= rt_flash_control;

    /* no private, no callback */
    w25q16_device.parent.user_data = RT_NULL;
    w25q16_device.parent.rx_indicate = RT_NULL;
    w25q16_device.parent.tx_complete = RT_NULL;

    result = rt_device_register(&w25q16_device.parent, flash_device_name,
                                RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_REMOVABLE | RT_DEVICE_FLAG_STANDALONE);

    return result;	
}



void rt_spi_flash_init(void)
{
	rt_hw_spi_init();
	rt_flash_register(FLASH_DEVICE_NAME,SPI1_CS_NAME);
}




















#ifdef RT_USING_FINSH
#include <finsh.h>


void flashread(u32 addr,u32 size)
{
	u32  i = addr;
	u8 dat;

	for(i = addr;i < size;i++)
	{
		spi_flash_buffer_read(w25q16_device.spi_device,&dat,i,1);
		rt_kprintf("%d  = %x 	 ",i,dat);
		if(i % 10 == 0)
		{
			rt_kprintf("\n");
		}
	}
}
FINSH_FUNCTION_EXPORT(flashread,flashread(start_addr, end_addr) --Read_Hex);

void flashreadc(u32 addr,u32 size)
{
	u32  i = addr;
	u8 dat;

	for(i = addr;i < size;i++)
	{
		spi_flash_buffer_read(w25q16_device.spi_device,&dat,i,1);
		rt_kprintf("%c ",dat);
	}
}
FINSH_FUNCTION_EXPORT(flashreadc,flashreadc(start_addr, end_addr)-- Read_String);




void flashsectore(u32 size)
{
	spi_flash_sector_erase(w25q16_device.spi_device,size);
}
FINSH_FUNCTION_EXPORT(flashsectore,flashsectore(Sectore_Addr)--Sectore_Erase);



/*******************************************************************************
* Function Name  : SPI_Flash_Write_NoCheck
* Description    : Not check write address to flash write 256byte
*                  
*                   
* Input         	 : pBuffer:data buffer WriteAddr:write address NumByteToWrite:byte num < 256
* Output         : None
* Return         : None
*******************************************************************************/
void SPI_Flash_Write_NoCheck(const u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite)   
{ 			 		 
	u16 pageremain;	   
	pageremain=256-WriteAddr%256; //单页剩余的字节数		 	    
	if(NumByteToWrite<=pageremain)pageremain=NumByteToWrite;//不大于256个字节
	while(1)
	{	   
		spi_flash_page_write(w25q16_device.spi_device,pBuffer,WriteAddr,pageremain);
		if(NumByteToWrite==pageremain)break;//写入结束了
	 	else //NumByteToWrite>pageremain
		{
			pBuffer+=pageremain;
			WriteAddr+=pageremain;	

			NumByteToWrite-=pageremain;			  //减去已经写入了的字节数
			if(NumByteToWrite>256)pageremain=256; //一次可以写入256个字节
			else pageremain=NumByteToWrite; 	  //不够256个字节了
		}
	};	    
} 

#endif

