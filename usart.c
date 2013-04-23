#include "stm32f10x.h"
#include "serial.h"
#include "rtthread.h"
#include <rtdevice.h>
/***********************************************************************************************************
@	pin config	USART1_REMAP = 0												
@____________________________________________________________________________*/

/* USART1_REMAP = 0 */
#define UART1_GPIO_TX						GPIO_Pin_9
#define UART1_GPIO_RX						GPIO_Pin_10
#define UART1_GPIO								GPIOA
#define RCC_APBPeriph_UART1		RCC_APB2Periph_USART1
#define USART1_TX_DMA					DMA1_Channel4
#define USART1_RX_DMA					DMA1_Channel5

#if defined(STM32F10X_LD) || defined(STM32F10X_MD) || defined(STM32F10X_CL)
#define UART2_GPIO_TX	    				GPIO_Pin_5
#define UART2_GPIO_RX	    				GPIO_Pin_6
#define UART2_GPIO	    						GPIOD
#define RCC_APBPeriph_UART2		RCC_APB1Periph_USART2
#else /* for STM32F10X_HD */
/* USART2_REMAP = 0 */
#define UART2_GPIO_TX						GPIO_Pin_2
#define UART2_GPIO_RX						GPIO_Pin_3
#define UART2_GPIO								GPIOA
#define RCC_APBPeriph_UART2		RCC_APB1Periph_USART2
#define UART2_TX_DMA						DMA1_Channel7
#define UART2_RX_DMA						DMA1_Channel6
#endif

/* USART3_REMAP[1:0] = 00 */
#define UART3_GPIO_RX						GPIO_Pin_11
#define UART3_GPIO_TX						GPIO_Pin_10
#define UART3_GPIO								GPIOB
#define RCC_APBPeriph_UART3		RCC_APB1Periph_USART3
#define UART3_TX_DMA						DMA1_Channel2
#define UART3_RX_DMA						DMA1_Channel3


#define USART1_DR_Base  0x40013804
#define USART2_DR_Base  0x40004404
#define USART3_DR_Base  0x40004804

/***********************************************************************************************************
@	Struct Definition	
@____________________________________________________________________________*/
struct serial_user_data
{
	USART_TypeDef* uart_device;
	const char name[RT_NAME_MAX];
	DMA_Channel_TypeDef *usart_tx_dma_channel;
	DMA_Channel_TypeDef *usart_rx_dma_channel;
};



/***********************************************************************************************************
@	Hardware clock configuration
@____________________________________________________________________________*/
static void RCC_Configuration(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	/* Enable USART1 and GPIOA clocks */
	RCC_APB2PeriphClockCmd(RCC_APBPeriph_UART1 | RCC_APB2Periph_GPIOA, ENABLE);

	/* Enable AFIO and GPIOD clock */
    RCC_APB1PeriphClockCmd(RCC_APBPeriph_UART2, ENABLE);

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
}

static void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Configure USART1 Rx (PA.10) as input floating */
	GPIO_InitStructure.GPIO_Pin = UART1_GPIO_RX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(UART1_GPIO, &GPIO_InitStructure);

	/* Configure USART1 Tx (PA.09) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = UART1_GPIO_TX;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(UART1_GPIO, &GPIO_InitStructure);

	/* Configure USART2 Rx as input floating */
	GPIO_InitStructure.GPIO_Pin = UART2_GPIO_RX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(UART2_GPIO, &GPIO_InitStructure);

	/* Configure USART2 Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = UART2_GPIO_TX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(UART2_GPIO, &GPIO_InitStructure);

}

static void NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Enable the USART1 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable the USART2 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable the DMA1 Channel2 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

}

static void DMA_Configuration(void)
{

	DMA_InitTypeDef DMA_InitStructure;

	/* fill init structure */
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

	/* DMA1 Channel5 (triggered by USART3 Tx event) Config */
	DMA_DeInit(USART1_TX_DMA);
	DMA_InitStructure.DMA_PeripheralBaseAddr = USART1_DR_Base;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	/* As we will set them before DMA actually enabled, the DMA_MemoryBaseAddr
	 * and DMA_BufferSize are meaningless. So just set them to proper values
	 * which could make DMA_Init happy.
	 */
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0;
	DMA_InitStructure.DMA_BufferSize = 1;
	DMA_Init(USART1_TX_DMA, &DMA_InitStructure);													//chang1
	DMA_ITConfig(USART1_TX_DMA, DMA_IT_TC | DMA_IT_TE, ENABLE);
	DMA_ClearFlag(DMA1_FLAG_TC4);																			//chang1

}




/***********************************************************************************************************
@	model driven architecture interface
@____________________________________________________________________________*/
rt_err_t stm32_serial_config(struct rt_serial_device *serial, struct serial_configure *cfg);

 int stm32_serial_put_char(struct rt_serial_device *serial, char c)
 {
 	struct serial_user_data* user = (struct serial_user_data *)(serial->parent.user_data);
 	
 	USART_ClearFlag(user->uart_device,USART_FLAG_TC);
	USART_SendData(user->uart_device,  c);
 	 while (USART_GetFlagStatus(user->uart_device, USART_FLAG_TC) == RESET);
 	 
 	 return c;
 }
 
int stm32_serial_get_char(struct rt_serial_device *serial)
 {	
	int  ch = -1;
 	struct serial_user_data* user = (struct serial_user_data *)(serial->parent.user_data);
 

	if(USART_GetITStatus(user->uart_device, USART_IT_RXNE) != RESET)
	{
		/* interrupt mode receive */
		RT_ASSERT(serial->parent.flag & RT_DEVICE_FLAG_INT_RX);

		ch = USART_ReceiveData(user->uart_device);
		
		/* clear interrupt */
		USART_ClearITPendingBit(user->uart_device, USART_IT_RXNE);
	}
	return ch;
 }

 rt_err_t stm32_serial_control(struct rt_serial_device *serial, int cmd, void *arg)
 {
	FunctionalState 	NewState;
	 
	struct serial_user_data *user = (struct serial_user_data *)serial->parent.user_data;

	switch(cmd)
	{
		case RT_DEVICE_CTRL_SET_INT:
		{
			NewState = ENABLE;
			break;
		}
		case RT_DEVICE_CTRL_CLR_INT:
		{
			NewState = DISABLE;
			break;
		}
		default:
		{
			break;
		}
	}

	if(*(rt_uint32_t *)arg & RT_SERIAL_RX_INT)
	{
		USART_ITConfig(user->uart_device, USART_IT_RXNE, NewState);

	}
	else if(*(rt_uint32_t *)arg & RT_SERIAL_TX_INT)
	{
		USART_ITConfig(user->uart_device, USART_IT_TC, NewState);
	}
	
	return RT_EOK;
 }

 rt_size_t stm32_serial_dma_transmit(struct rt_serial_device *serial, const char *buf, rt_size_t size)
 {
 	struct serial_user_data *user = (struct serial_user_data*)serial->parent.user_data;

	/* disable DMA */
	DMA_Cmd(user->usart_tx_dma_channel, DISABLE);

	/* set buffer address */
	user->usart_tx_dma_channel->CMAR = (u32)buf;
	/* set size */
	user->usart_tx_dma_channel->CNDTR = size;

	/* enable DMA */
	DMA_Cmd(user->usart_tx_dma_channel, ENABLE);
 	
	return size;
 }

rt_err_t stm32_serial_config(struct rt_serial_device *serial, struct serial_configure *cfg)
{
	USART_InitTypeDef 				USART_InitStructure;
	USART_ClockInitTypeDef 		USART_ClockInitStructure;
	struct serial_user_data* 		user = (struct serial_user_data *)serial->parent.user_data;

	
	RCC_Configuration();
	
	GPIO_Configuration();
	
	NVIC_Configuration();
	
	DMA_Configuration();

	USART_InitStructure.USART_BaudRate 		= cfg->baud_rate;
	
	switch(cfg->data_bits)
	{
		case 8:
		{
			USART_InitStructure.USART_WordLength 	= USART_WordLength_8b;
			break;
		}
		case 9:
		{
			USART_InitStructure.USART_WordLength 	= USART_WordLength_9b;
			break;
		}
		default :
		{
#ifndef RT_USING_FINSH
			rt_kprintf("data bit set error\n");
#endif
			break;
		}
	}

	switch(cfg->stop_bits)
	{
		case 1:
		{
			USART_InitStructure.USART_StopBits 	= USART_StopBits_1;
			break;
		}
		case 2:
		{
			USART_InitStructure.USART_StopBits 	= USART_StopBits_2;
			break;
		}
		case 3:
		{
			USART_InitStructure.USART_StopBits 	= USART_StopBits_0_5;
			break;
		}
		case 4:
		{
			USART_InitStructure.USART_StopBits 	= USART_StopBits_1_5;
			break;
		}
		default :
		{
#ifndef RT_USING_FINSH
			rt_kprintf("stopbits bit set error\n");
#endif
			break;
		}
	}

	switch(cfg->parity)
	{
		case 0:
		{
			USART_InitStructure.USART_Parity 	= USART_Parity_No;
			break;
		}
		case 1:
		{
			USART_InitStructure.USART_Parity 	= USART_Parity_Odd;
			break;
		}
		case 2:
		{
			USART_InitStructure.USART_Parity 	= USART_Parity_Even;
			break;
		}
		default :
		{
#ifndef RT_USING_FINSH
			rt_kprintf("data bit set error\n");
#endif
			break;
		}
	}
	
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode 				= USART_Mode_Rx | USART_Mode_Tx;
	USART_ClockInitStructure.USART_Clock		= USART_Clock_Disable;
	USART_ClockInitStructure.USART_CPOL 		= USART_CPOL_Low;
	USART_ClockInitStructure.USART_CPHA 		= USART_CPHA_2Edge;
	USART_ClockInitStructure.USART_LastBit 	= USART_LastBit_Disable;

	USART_Init(user->uart_device, &USART_InitStructure);
	
	USART_ClockInit(user->uart_device, &USART_ClockInitStructure);	
	
	USART_Cmd(user->uart_device,ENABLE);
	
  if (serial->parent.flag & RT_DEVICE_FLAG_INT_RX)
  {
    USART_ITConfig(user->uart_device, USART_IT_RXNE, ENABLE);
  }
  
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    USART_DMACmd(user->uart_device, USART_DMAReq_Tx , ENABLE);
  }

	return RT_EOK;
}


/***********************************************************************************************************
@serial private function
@____________________________________________________________________________*/
struct rt_uart_ops  serial_ops = 
{
	stm32_serial_config,						//串口配置函数
	stm32_serial_control,						//串口控制函数
	stm32_serial_put_char,					//串口从硬件设备输出一个字符
	stm32_serial_get_char,					//串口从硬件设备获得一个字符
	stm32_serial_dma_transmit			//dma传送函数
};











/***********************************************************************************************************
@Struct declaration	
@____________________________________________________________________________*/
struct serial_configure   serial_config = 
{
	115200,				//USART_BaudRate
	8,							//USART_WordLength
	1,							//USART_StopBits
	0,							//USART_Parity
	0,							//
	0,							//
	0							//
};

struct serial_user_data usart2_device = 
{
	USART2,				//hardware device
	"uart2"				//device name
};
struct serial_ringbuffer serial_int_rx_buffer;
struct serial_ringbuffer serial_int_tx_buffer;
struct rt_serial_device	serial2_device;

void rt_hw_serial2_register(void)
{
	serial2_device.config = serial_config;
	serial2_device.int_rx = &serial_int_rx_buffer;
	serial2_device.int_tx = &serial_int_tx_buffer;
	serial2_device.ops = &serial_ops;

	rt_hw_serial_register(&serial2_device, usart2_device.name,
	RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
	&usart2_device);
}








struct serial_user_data usart1_device = 
{
	USART1,				//hardware device
	"uart1"		,				//device name
	USART1_TX_DMA
	
};
struct rt_serial_device	serial1_device;
struct serial_ringbuffer serial_int_rx_buffer1;
struct serial_ringbuffer serial_int_tx_buffer1;
void rt_hw_serial1_register(void)
{
	serial1_device.config = serial_config;
	serial1_device.int_rx = &serial_int_rx_buffer1;
	serial1_device.int_tx = &serial_int_tx_buffer1;
	serial1_device.ops = &serial_ops;

	rt_hw_serial_register(&serial1_device, usart1_device.name,
	RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
	&usart1_device);
}




















#ifdef RT_USING_FINSH
#include <finsh.h>
static rt_uint8_t led_inited = 0;
void usarts2(char *str)
{
	rt_device_t usart;

	usart = rt_device_find("uart2");
	rt_device_write(usart,0,str,20);
}
FINSH_FUNCTION_EXPORT(usarts2, set led[0 - 1] on[1] or off[0].)
#endif

