#include "spicamera.h"

#define SPI1_BUS_NAME				("spi1")
#define SPI1_CS1_NAME				("cm_cs")
#define SPI1_DEVICE1_NAME		("camera")

#define SPI1_RCC						RCC_APB2Periph_SPI1
#define SPI1_PORT_RCC				RCC_APB2Periph_GPIOA
#define SPI1_GPIO_PORT			GPIOA
#define SPI1_CLK_PIN				GPIO_Pin_5
#define SPI1_MISO_PIN				GPIO_Pin_6
#define SPI1_MOSI_PIN				GPIO_Pin_7

#define SPI1_CS_PORT				GPIOA
#define SPI1_CS_PIN					GPIO_Pin_4
#define SPI1_CS_CLK					RCC_APB2Periph_GPIOA





struct camera_device 
{
	struct rt_device          parent;      /**< RT-Thread device struct */
	struct rt_spi_device *    spi_device;  /**< SPI interface */
	uint32_t                  max_clock;   /**< MAX SPI clock */
};
struct camera_device	camera_mode;



//static void delay(rt_uint32_t t)
//{
//	while(--t);
//}

void rt_hw_spi1_init(void)
{
	/*		initialization SPI Bus device		 */
	{		
		GPIO_InitTypeDef 							gpio_initstructure;

		RCC_APB2PeriphClockCmd(SPI1_RCC | SPI1_PORT_RCC |RCC_APB2Periph_AFIO,ENABLE);

		gpio_initstructure.GPIO_Mode = GPIO_Mode_AF_PP;
		gpio_initstructure.GPIO_Speed = GPIO_Speed_50MHz;
		gpio_initstructure.GPIO_Pin = SPI1_MISO_PIN | SPI1_MOSI_PIN | SPI1_CLK_PIN;
		GPIO_Init(SPI1_GPIO_PORT,&gpio_initstructure);
		/*		register spi bus device */
		stm32_spi_register(SPI1,&stm32_spi_bus_1,SPI1_BUS_NAME);
	}
	/*		initialization SPI CS device 		 */
	{
		GPIO_InitTypeDef								gpio_initstructure;
		static struct rt_spi_device 		spi_device; 			 
		static struct stm32_spi_cs			spi_cs; 
		
		/*		configure CS clock port pin		*/
		spi_cs.GPIOx = SPI1_CS_PORT;        
		spi_cs.GPIO_Pin = SPI1_CS_PIN;     
		RCC_APB2PeriphClockCmd(SPI1_CS_CLK, ENABLE);        
		
		gpio_initstructure.GPIO_Mode = GPIO_Mode_Out_PP;    
		gpio_initstructure.GPIO_Pin = spi_cs.GPIO_Pin;        
		gpio_initstructure.GPIO_Speed = GPIO_Speed_50MHz; 
		GPIO_SetBits(spi_cs.GPIOx, spi_cs.GPIO_Pin);     
		
		GPIO_Init(spi_cs.GPIOx, &gpio_initstructure);     
		/* 	add cs devie go to spi bus devie	*/
		rt_spi_bus_attach_device(&spi_device,SPI1_CS1_NAME, SPI1_BUS_NAME, (void*)&spi_cs);
	}
}








static u8 camera_read_write_byte(struct rt_spi_device* device,const u8 data)
{
	volatile u8 value;
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


static camera_read_buffer_data(struct rt_spi_device*  dev, rt_off_t pos, rt_uint8_t* buffer, rt_size_t size)
{
	
	while(size)
	{
		rt_spi_take(dev);
		*buffer = camera_read_write_byte(dev,0xff);
		rt_spi_release(dev);
		buffer++;
		size--;
	}

//	rt_spi_send_then_recv(dev,RT_NULL,0,buffer,size);
}

/**************************************** spi flash resiger function **********************************/

static rt_err_t rt_camera_init(rt_device_t dev)
{
	struct camera_device* camera = (struct camera_device *)dev;	

	 RT_ASSERT(dev != RT_NULL);
	 
	if(camera->spi_device->bus->owner != camera->spi_device)
	{
		camera->spi_device->bus->ops->configure(camera->spi_device,&camera->spi_device->config);
	}
	
	return RT_EOK;
}

static rt_err_t rt_camera_open(rt_device_t dev, rt_uint16_t oflag)
{
	return RT_EOK;
}
static rt_err_t rt_camera_close(rt_device_t dev)
{
	return RT_EOK;
}

static rt_size_t rt_camera_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
	struct camera_device *device = (struct camera_device *)dev;
	
	camera_read_buffer_data(device->spi_device,pos,buffer,size);
	
	return size;
}

static rt_size_t rt_camera_write(rt_device_t dev, rt_off_t pos,const void* buffer, rt_size_t size)
{
	return size;
}

static rt_err_t rt_camera_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
	RT_ASSERT(dev != RT_NULL);

    return RT_EOK;
}

rt_err_t rt_camera_register(const char* device_name,const char *cs_name)
{
	rt_err_t result = RT_EOK;
	struct rt_spi_device * spi_device;
	struct rt_spi_configuration spi1_configuer= 
	{
		RT_SPI_MODE_0 | RT_SPI_MSB,								//spi clock and data mode set
		8,																				//data width
		0,																				//reserved
		72000000/8																//MAX frequency 18MHz
	};

	spi_device = (struct rt_spi_device *)rt_device_find(cs_name);
	if(spi_device == RT_NULL)
	{

#ifdef RT_USING_FINSH
	rt_kprintf("spi device %s not found!\r\n", cs_name);
#endif

	return -RT_ENOSYS;
	}
	rt_memset(&camera_mode, 0, sizeof(camera_mode));
	camera_mode.spi_device = spi_device;
	camera_mode.max_clock = 9000000;
	camera_mode.spi_device->config = spi1_configuer;
	/* register flash device */
	camera_mode.parent.type    = RT_Device_Class_Char;



	camera_mode.parent.init    	= rt_camera_init;
	camera_mode.parent.open    	= rt_camera_open;
	camera_mode.parent.close   	= rt_camera_close;
	camera_mode.parent.read    	= rt_camera_read;
	camera_mode.parent.write   	= rt_camera_write;
	camera_mode.parent.control 	= rt_camera_control;

	/* no private, no callback */
	camera_mode.parent.user_data = RT_NULL;
	camera_mode.parent.rx_indicate = RT_NULL;
	camera_mode.parent.tx_complete = RT_NULL;

	result = rt_device_register(&camera_mode.parent, device_name,
	                            RT_DEVICE_FLAG_RDWR |RT_DEVICE_FLAG_STANDALONE );

	return result;	
}

void rt_hw_camera_init()
{
	rt_hw_spi1_init();
	rt_camera_register(SPI1_DEVICE1_NAME,SPI1_CS1_NAME);
}


