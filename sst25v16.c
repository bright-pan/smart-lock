/*********************************************************************
 * Filename:    sst25v16.h
 *
 * Description:  Tihs flie function spi flash block device
 *						drivie,chip type sst25vf016,user stm32
 *						hardware resources SPI2 ,
 *						
 * Author:        wangzw <wangzw@yuettak.com>
 * Created at:    2013-04-26 12:00:00
 *                
 * Modify:
 *
 * 
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "sst25v16.h"

/*-----------sst25v16 chip  command list --------------*/
#define CMD_RDSR                    	0x05  /* 读状态寄存器     */
#define CMD_WRSR                    	0x01  /* 写状态寄存器     */
#define CMD_EWSR                    	0x50  /* 使能写状态寄存器 */
#define CMD_WRDI                    	0x04  /* 关闭写使能       */
#define CMD_WREN                    	0x06  /* 打开写使能       */
#define CMD_READ                    	0x03  /* 读数据           */
#define CMD_FAST_READ                 0x0B  /* 快速读           */
#define CMD_BP                      	0x02  /* 字节编程         */
#define CMD_AAIP                    	0xAD  /* 自动地址增量编程 */
#define CMD_ERASE_4K               		0x20  /* 扇区擦除:4K      */
#define CMD_ERASE_32K               	0x52  /* 扇区擦除:32K     */
#define CMD_ERASE_64K               	0xD8  /* 扇区擦除:64K     */
#define CMD_ERASE_full              	0xC7  /* 全片擦除         */
#define CMD_JEDEC_ID                	0x9F  /* 读 JEDEC_ID      */
#define CMD_EBSY                    	0x70  /* 打开SO忙输出指示 */
#define CMD_DBSY                    	0x80  /* 关闭SO忙输出指示 */

/* use auxiliary data */
#define WIP_Flag                  		0x01  //Write In Progress (WIP) flag 
#define Dummy_Byte                		0xFF  //moke needful read clock




/* flash device */
struct flash_device 
{
	struct rt_device                	parent;      /**< RT-Thread device struct */
	struct rt_device_blk_geometry   	geometry;    /**< sector size, sector count */
	struct rt_spi_device *          	spi_device;  /**< SPI interface */
	u32 											max_clock;   /**< MAX SPI clock */
};

/*spi device*/
struct flash_device	sst25v16;



/*******************************************************************************
* Function Name  : rt_hw_spi_init
* Description    : register spi bus and register spi cs device
*                  
* Input				: None
* Output			: None
* Return         	: None
*******************************************************************************/
void rt_hw_spi_init(void)
{
	/*		initialization SPI Bus device		 */
	{		
		GPIO_InitTypeDef 							gpio_initstructure;

		RCC_APB1PeriphClockCmd(SPI2_CLOCK ,ENABLE);
		RCC_APB2PeriphClockCmd(GPIO_RCC,ENABLE);

		gpio_initstructure.GPIO_Mode = GPIO_Mode_AF_PP;
		gpio_initstructure.GPIO_Speed = GPIO_Speed_50MHz;
		gpio_initstructure.GPIO_Pin = SPI2_MISO_PIN | SPI2_MOSI_PIN | SPI2_SCK_PIN;
		GPIO_Init(SPI2_GPIO_PORT,&gpio_initstructure);
		/*		register spi bus device */
		stm32_spi_register(SPI2,&stm32_spi_bus_2,SPI2_BUS_NAME);
	}
	/*		initialization SPI CS device 		 */
	{
		static struct rt_spi_device 		spi_device;        
		static struct stm32_spi_cs  		spi_cs;	
		GPIO_InitTypeDef						gpio_initstructure;
   
		/*		configure CS clock port pin		*/
		spi_cs.GPIOx = SPI2_CS_PORT;        
		spi_cs.GPIO_Pin = SPI2_CS_PIN;     
		RCC_APB2PeriphClockCmd(SPI2_CS_CLOCK, ENABLE);        
		
		gpio_initstructure.GPIO_Mode = GPIO_Mode_Out_PP;    
		gpio_initstructure.GPIO_Pin = spi_cs.GPIO_Pin;        
		gpio_initstructure.GPIO_Speed = GPIO_Speed_50MHz; 
		GPIO_SetBits(spi_cs.GPIOx, spi_cs.GPIO_Pin);     
		
		GPIO_Init(spi_cs.GPIOx, &gpio_initstructure);     
		/* 	add cs devie go to spi bus devie	*/
		rt_spi_bus_attach_device(&spi_device,SPI2_CS_NAME1, SPI2_BUS_NAME, (void*)&spi_cs);
		
	}
}




/*-------------flash chip device drive function -------------*/
/*******************************************************************************
* Function Name  : spi_flash_write_read_byte
* Description    : write and read flash one byte
*                  
* Input				: device :spi device data :input flash of data
* Output			: read output data
* Return         	: None
*******************************************************************************/

static u8 spi_flash_write_read_byte(struct rt_spi_device *device,const u8 data)
{
	u8 value;
	struct rt_spi_message message;

	message.length = 1;
	message.recv_buf = &value;
	message.send_buf = &data;
	message.cs_release = 0;
	message.cs_take = 0;
	message.next = RT_NULL;
	rt_spi_transfer_message(device,&message);

	return value;
}
/*******************************************************************************
* Function Name  : spi_flash_wait_write_end
* Description    :flash private function busy check fun
*                  
* Input				: device :spi device 
* Output			: None
* Return         	: None
*******************************************************************************/
static void spi_flash_wait_write_end(struct rt_spi_device *device)
{
	u8 flash_status;
	
	rt_spi_take(device);
	
	spi_flash_write_read_byte(device,CMD_RDSR);
	 
  do
  {
    flash_status = spi_flash_write_read_byte(device,Dummy_Byte);	 
  }
  while ((flash_status& WIP_Flag) == SET); 
	
	rt_spi_release(device);
}
/*******************************************************************************
* Function Name  : spi_flash_write_status_enable
* Description    :flash private function enable write status register
*                  
* Input				: device :spi device 
* Output			: None
* Return         	: None
*******************************************************************************/
static void spi_flash_write_status_enable(struct rt_spi_device *device)
{
	rt_spi_take(device);

	spi_flash_write_read_byte(device,CMD_EWSR);

	rt_spi_release(device);

	rt_spi_take(device);

	spi_flash_write_read_byte(device,CMD_WRSR);
	
	spi_flash_write_read_byte(device , 0x00);

	rt_spi_release(device);

	spi_flash_wait_write_end(device);

}
/*******************************************************************************
* Function Name  : spi_flash_write_enable
* Description    :flash private function enable write data fun
*                  
* Input				: device :spi device 
* Output			: None
* Return         	: None
*******************************************************************************/
static void spi_flash_write_enable(struct rt_spi_device *device )
{
	spi_flash_write_status_enable(device);

	rt_spi_take(device);

	spi_flash_write_read_byte(device,CMD_WREN);

	rt_spi_release(device);

	spi_flash_wait_write_end(device);
}
/*******************************************************************************
* Function Name  : spi_flash_write_disable
* Description    :flash private function disable write data fun
*                  
* Input				: device :spi device 
* Output			: None
* Return         	: None
*******************************************************************************/
static void spi_flash_write_disable(struct rt_spi_device *device)
{
	spi_flash_write_status_enable(device);

	rt_spi_take(device);

	spi_flash_write_read_byte(device,CMD_WRDI);

	rt_spi_release(device);

	spi_flash_wait_write_end(device);
}
/*******************************************************************************
* Function Name  : spi_flash_sector_erase
* Description    :flash private function erase 4k sector
*                  
* Input				: device :spi device 
* Output			: None
* Return         	: None
*******************************************************************************/
void spi_flash_sector_erase(struct rt_spi_device *device,u32 SectorAddr)
{
	spi_flash_write_enable(device);

  rt_spi_take(device);

  spi_flash_write_read_byte(device,CMD_ERASE_4K);

  spi_flash_write_read_byte(device,(SectorAddr & 0xFF0000) >> 16);

  spi_flash_write_read_byte(device,(SectorAddr & 0xFF00) >> 8);

  spi_flash_write_read_byte(device,SectorAddr & 0xFF);

  rt_spi_release(device);
  
  spi_flash_wait_write_end(device);
}
/*******************************************************************************
* Function Name  : spi_flash_chip_erase
* Description    :flash private function erase Chip Integration plan
*                  
* Input				: device :spi device 
* Output			: None
* Return         	: None
*******************************************************************************/
static void spi_flash_chip_erase(struct rt_spi_device *device)
{
	spi_flash_write_enable(device);

	rt_spi_take(device);

	spi_flash_write_read_byte(device,CMD_ERASE_full);

	rt_spi_release(device);

	spi_flash_wait_write_end(device);
}
/*******************************************************************************
* Function Name  : spi_flash_buffer_write
* Description    :flash private function write sst25vf016 data 
*                  
* Input				: device :		spi device;
*						  pBuffer :		data buffer;
*						  WriteAddr:	statr write pos;
*						  NumByteToWrite: write data size
* Output			: None
* Return         	: None
*******************************************************************************/
static void spi_flash_buffer_write(struct rt_spi_device *device,const u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
	u32 index;


	WriteAddr &= ~0xFFF; // page size = 4096byte

	spi_flash_sector_erase(device,WriteAddr);

	spi_flash_write_enable(device);

	rt_spi_take(device);

	spi_flash_write_read_byte(device, CMD_AAIP );
	
	spi_flash_write_read_byte(device, WriteAddr>>16 );
	
	spi_flash_write_read_byte(device, WriteAddr>>8 );
	
	spi_flash_write_read_byte(device, WriteAddr );

	spi_flash_write_read_byte(device,*pBuffer++ );

	spi_flash_write_read_byte(device,*pBuffer++ );

	NumByteToWrite -= 2;

	rt_spi_release(device);

	spi_flash_wait_write_end(device);

	for(index=0; index < NumByteToWrite/2; index++)
	{
		rt_spi_take(device);
		spi_flash_write_read_byte(device, CMD_AAIP );
		spi_flash_write_read_byte(device, *pBuffer++ );
		spi_flash_write_read_byte(device, *pBuffer++ );
		rt_spi_release(device);
		spi_flash_wait_write_end(device);
	}

	spi_flash_write_disable(device);
}

static void spi_flash_buffer_read(struct rt_spi_device *device,u8* pBuffer, u32 ReadAddr, u16 NumByteToRead)
{
	spi_flash_write_disable(device);
	
	rt_spi_take(device);

	spi_flash_write_read_byte(device,CMD_FAST_READ);

	spi_flash_write_read_byte(device,(ReadAddr & 0x00FF0000) >> 16);

	spi_flash_write_read_byte(device,(ReadAddr & 0x0000FF00) >> 8);

	spi_flash_write_read_byte(device, ReadAddr & 0x000000FF);

	spi_flash_write_read_byte(device,0);

	while (NumByteToRead--) 
	{
		*pBuffer = spi_flash_write_read_byte(device,Dummy_Byte);
		pBuffer++;
	}
	
	rt_spi_release(device);
}






























/**************************************** spi flash resiger function **********************************/

static rt_err_t rt_flash_init(rt_device_t dev)
{
	struct flash_device* flash = (struct flash_device *)dev;	

	if(flash->spi_device->bus->owner != flash->spi_device)
	{
		/* 	current configuration spi bus */		
		flash->spi_device->bus->ops->configure(flash->spi_device,&flash->spi_device->config);
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
	struct flash_device* flash = (struct flash_device *)dev;	
	
	spi_flash_buffer_read(flash->spi_device,buffer,pos*4096,size*4096);
	
	return size;
}


static rt_size_t rt_flash_write(rt_device_t dev, rt_off_t pos,const void* buffer, rt_size_t size)
{
	struct flash_device* flash = (struct flash_device *)dev;	
	
	spi_flash_buffer_write(flash->spi_device,buffer,pos*4096,size*4096);
	
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
	struct rt_spi_configuration spi2_configuer= 
	{
		RT_SPI_MODE_MASK,										//spi clock and data mode set
		8,																			//data width
		0,																			//reserved
		36000000/2														//MAX frequency 18MHz
	};

    spi_device = (struct rt_spi_device *)rt_device_find(spi_device_name);
    if(spi_device == RT_NULL)
    {
    
#ifdef RT_USING_FINSH
		rt_kprintf("spi device %s not found!\r\n", spi_device_name);
#endif

		return -RT_ENOSYS;
    }
    rt_memset(&sst25v16, 0, sizeof(sst25v16));
    sst25v16.spi_device = spi_device;
	sst25v16.max_clock = 18000000;
	sst25v16.spi_device->config = spi2_configuer;
    /* register flash device */
    sst25v16.parent.type    = RT_Device_Class_Block;

    sst25v16.geometry.bytes_per_sector = 4096;
    sst25v16.geometry.sector_count = 512;
    sst25v16.geometry.block_size = 4096;

   

    sst25v16.parent.init    		= rt_flash_init;
    sst25v16.parent.open    	= rt_flash_open;
    sst25v16.parent.close   	= rt_flash_close;
    sst25v16.parent.read    	= rt_flash_read;
    sst25v16.parent.write   	= rt_flash_write;
    sst25v16.parent.control 	= rt_flash_control;

    /* no private, no callback */
    sst25v16.parent.user_data = RT_NULL;
    sst25v16.parent.rx_indicate = RT_NULL;
    sst25v16.parent.tx_complete = RT_NULL;

    result = rt_device_register(&sst25v16.parent, flash_device_name,
                                RT_DEVICE_FLAG_RDWR  | RT_DEVICE_FLAG_STANDALONE | RT_DEVICE_FLAG_DMA_RX |RT_DEVICE_FLAG_DMA_TX);

    return result;	
}



void rt_spi_flash_init(void)
{
	rt_hw_spi_init();
	rt_flash_register(FLASH_DEVICE_NAME,SPI2_CS_NAME1);
}






























#ifdef RT_USING_FINSH
#include <finsh.h>


void flashread(u32 addr,u32 size)
{
	u32  i = addr;
	u8 dat;

	for(i = addr;i < size;i++)
	{
		spi_flash_buffer_read(sst25v16.spi_device,&dat,i,1);
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
		spi_flash_buffer_read(sst25v16.spi_device,&dat,i,1);
		rt_kprintf("%c ",dat);
	}
}
FINSH_FUNCTION_EXPORT(flashreadc,flashreadc(start_addr, end_addr)-- Read_String);




void flashsectore(u32 size)
{
	spi_flash_sector_erase(sst25v16.spi_device,size);
}
FINSH_FUNCTION_EXPORT(flashsectore,flashsectore(Sectore_Addr)--Sectore_Erase);

void flashwrite(u32 addr,u8 *data, u32 size)
{
	spi_flash_buffer_write(sst25v16.spi_device,data,addr,size);
	
}
FINSH_FUNCTION_EXPORT(flashwrite,flashwrite(addr,data,size)--Sectore_Erase);

void sreset()
{
	NVIC_SystemReset();
}
FINSH_FUNCTION_EXPORT(sreset,sreset()--system reset);




#endif






