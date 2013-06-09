#include "boardinfo.h"


#define 									ADC1_DR_Address    ((u32)0x4001244C)
volatile u16 							ADC_ConvertedValue = 0;




static void rcc_config(void)
{
	/* Enable DMA clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	/* Enable ADC1 and GPIOC clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
}

static void dma_config(void)
{
	DMA_InitTypeDef DMA_InitStructure;

	/* DMA channel1 configuration */			 
	DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address; 					// 外设基地址
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&ADC_ConvertedValue; 				// AD转换值所存放的内存基地址 （就是给个地址）
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;												// 外设作为数据传输的来源 
	DMA_InitStructure.DMA_BufferSize = 1; 																		// 定义指定DMA通道 DMA缓存的大小
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;					// 外设地址寄存器不变
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;							// 内存地址寄存器不变
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // 数据宽度为16位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord; 				// HalfWord
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; 											//工作在循环模式下
	DMA_InitStructure.DMA_Priority = DMA_Priority_High; 										//高优先级
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;														//没有设置为内存到内存的传输
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);

	/* Enable DMA channel1 */
	DMA_Cmd(DMA1_Channel1, ENABLE); 										//ENABLE她

}
static void iadc_config(void)
{
	ADC_InitTypeDef ADC_InitStructure;
	/* ADC1 configuration */
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;												//独立工作模式
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;													//多通道
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;									//连续转换
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; 			//由软件触发启动
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;					//Right
	ADC_InitStructure.ADC_NbrOfChannel = 1; 													//仅一个通道转换
	ADC_Init(ADC1, &ADC_InitStructure);

		/*配置ADC时钟，为PCLK2的8分频，即9Hz*/
	RCC_ADCCLKConfig(RCC_PCLK2_Div8); 

	/* ADC1 regular channel16 configuration */ 
	//设置采样通道IN16, 设置采样时间
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5); 

	//使能温度传感器和内部参考电压	 
	ADC_TempSensorVrefintCmd(ENABLE); 																	 

	 /* Enable ADC1 DMA */		
	ADC_DMACmd(ADC1, ENABLE);

	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);

	/* Enable ADC1 reset calibaration register */ 	
	ADC_ResetCalibration(ADC1); 																	
	/* Check the end of ADC1 reset calibration register */
	while(ADC_GetResetCalibrationStatus(ADC1)); 												

	/* Start ADC1 calibaration */
	ADC_StartCalibration(ADC1); 															
	/* Check the end of ADC1 calibration */
	while(ADC_GetCalibrationStatus(ADC1));		
		 
	/* Start ADC1 Software Conversion */ 
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

}
static  rt_err_t  rt_gpio_init(rt_device_t dev)
{
	return RT_EOK;
}
static  rt_err_t  rt_gpio_open(rt_device_t dev, rt_uint16_t oflag)
{
	return RT_EOK;
}
static  rt_err_t  rt_gpio_close(rt_device_t dev)
{
	return RT_EOK;
}

static rt_size_t rt_gpio_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
	RT_ASSERT(dev != RT_NULL);

	*(u16*)buffer = ADC_ConvertedValue;

	return size;
}
rt_size_t rt_gpio_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
	return size;
}
rt_err_t  rt_gpio_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
	return RT_EOK;
}

static struct rt_device chip_temp_device;
void rt_hw_chip_register(void)
{
  rcc_config();
	dma_config();
	iadc_config();
	rt_kprintf("stm32f103ze temperature = %d ℃",ADC_ConvertedValue);

	chip_temp_device.type        = RT_Device_Class_Char;
  chip_temp_device.rx_indicate = RT_NULL;
  chip_temp_device.tx_complete = RT_NULL;
	
  chip_temp_device.init 		= rt_gpio_init;
  chip_temp_device.open   	= rt_gpio_open;
  chip_temp_device.close 		= rt_gpio_close;
  chip_temp_device.read   	= rt_gpio_read;
  chip_temp_device.write    = rt_gpio_write;
  chip_temp_device.control    = rt_gpio_control;
  chip_temp_device.user_data  = RT_NULL;
	
  /* register a character device */
	rt_device_register(&chip_temp_device,"IC_T", RT_DEVICE_OFLAG_RDONLY);
	
}


#ifdef RT_USING_FINSH
#include <finsh.h>

void ic_temp()
{
	rt_kprintf("stm32f103ze temperature = %03d ℃",(0x6EE-ADC_ConvertedValue)/0x05+25);
}
FINSH_FUNCTION_EXPORT(ic_temp,ic_temp() -- stm32f103ve chip internal temperature);



//rt_kprintf("stm32f103ze temperature = %3d ℃",(0x6EE-ADC_ConvertedValue)/0x05+25);
#endif





