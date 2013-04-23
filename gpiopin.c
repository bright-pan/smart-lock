#include "gpiopin.h"

struct gpio_user_data
{
	const char						name[RT_NAME_MAX];
	GPIO_TypeDef* 				gpiox;							//port
	rt_uint16_t						gpio_pinx;					//pin
	GPIOMode_TypeDef 		gpio_mode;				//mode
	GPIOSpeed_TypeDef 	gpio_speed;				//speed
	u32 									gpio_clock;				//clock
	u8 									gpio_port_source;
	u8 									gpio_pin_source;
	u32 									exti_line;     				//exti_line
	EXTIMode_TypeDef 		exti_mode;  				//exti_mode
	EXTITrigger_TypeDef		exti_trigger; 				//exti_trigger
	u8 						  			nvic_channel;
	u8  									nvic_preemption_priority;
	u8 									nvic_subpriority;
	
};




/*********************************************************************************************************************************
**															GPIO ops function interfaces 														**
**																																								**
**********************************************************************************************************************************/
/*
 * gpio ops configure
 */
rt_err_t stm32_gpio_configure(gpio_device *gpio)
{	
	struct gpio_user_data* user = (struct gpio_user_data *)gpio->parent.user_data;
	GPIO_InitTypeDef	gpio_initstructure;

	RCC_APB2PeriphClockCmd(user->gpio_clock,ENABLE);

	if(1 == gpio->isr_flag)
	{
		EXTI_InitTypeDef exti_initstructure;
		NVIC_InitTypeDef nvic_initstructure;

		nvic_initstructure.NVIC_IRQChannel = user->nvic_channel;
		nvic_initstructure.NVIC_IRQChannelPreemptionPriority = user->nvic_preemption_priority;
		nvic_initstructure.NVIC_IRQChannelSubPriority = user->nvic_subpriority;
		nvic_initstructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&nvic_initstructure);

		GPIO_EXTILineConfig(user->gpio_port_source,user->gpio_pin_source);

		exti_initstructure.EXTI_Line = user->exti_line;
		exti_initstructure.EXTI_Mode = user->exti_mode;
		exti_initstructure.EXTI_Trigger = user->exti_trigger;
		exti_initstructure.EXTI_LineCmd = ENABLE;
		EXTI_Init(&exti_initstructure);
	}

	gpio_initstructure.GPIO_Mode = user->gpio_mode;
	gpio_initstructure.GPIO_Pin = user->gpio_pinx;
	gpio_initstructure.GPIO_Speed = user->gpio_speed;
	GPIO_Init(user->gpiox,&gpio_initstructure);
	
	return RT_EOK;
}
rt_err_t stm32_gpio_control(gpio_device *gpio, int cmd, void *arg)
{
	struct gpio_user_data* user = (struct gpio_user_data *)gpio->parent.user_data;

	return RT_EOK;
}

void	stm32_gpio_out(gpio_device *gpio, rt_uint8_t c)
{
	struct gpio_user_data* user = (struct gpio_user_data *)gpio->parent.user_data;

	if(c)
	{
		GPIO_SetBits(user->gpiox,user->gpio_pinx);
	}
	else
	{
		GPIO_ResetBits(user->gpiox,user->gpio_pinx);
	}
}
rt_uint8_t stm32_gpio_intput(gpio_device *gpio)
{
	struct gpio_user_data* user = (struct gpio_user_data *)gpio->parent.user_data;
	rt_uint8_t data;

	
	data = GPIO_ReadInputDataBit(user->gpiox,user->gpio_pinx);

	return data;
}

struct rt_gpio_ops gpio_user_ops= 
{
	stm32_gpio_configure,
	stm32_gpio_control,
	stm32_gpio_out,
	stm32_gpio_intput
};






rt_err_t key2_rx_ind(rt_device_t dev, rt_size_t size)
{
	rt_kprintf("key2 ok\n");
	return RT_EOK;
}


/*********************************************************************************************************************************
**																	user device drive	struct														**
**																																								**
**********************************************************************************************************************************/


gpio_device key1_device;

struct gpio_user_data key1_user_data = 
{
	"key1",
	GPIOE,
	GPIO_Pin_5,
	GPIO_Mode_IPU,
	GPIO_Speed_50MHz,
	RCC_APB2Periph_GPIOE,
	GPIO_PortSourceGPIOE,
	GPIO_PinSource5,
	EXTI_Line5,
	EXTI_Mode_Interrupt,
	EXTI_Trigger_Falling,
	EXTI9_5_IRQn,
	1,
	3
};

void rt_hw_key1_register(void)
{
	rt_device_t key;
	
	key1_device.ops = &gpio_user_ops;
	key1_device.isr_flag = 1;

	rt_hw_gpio_register(&key1_device,key1_user_data.name,RT_DEVICE_FLAG_RDWR |RT_DEVICE_FLAG_INT_RX| RT_DEVICE_FLAG_STREAM,&key1_user_data);

	key = rt_device_find("key1");
	
	rt_device_init(key);
}

struct gpio_user_data key2_user_data = 
{
	"key2",
	GPIOE,
	GPIO_Pin_6,
	GPIO_Mode_IN_FLOATING,
	GPIO_Speed_50MHz,
	RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO,
	GPIO_PortSourceGPIOE,
	GPIO_PinSource6,
	EXTI_Line6,
	EXTI_Mode_Interrupt,
	EXTI_Trigger_Falling,
	EXTI9_5_IRQn,
	1,
	4
};

gpio_device key2_device;
void rt_hw_key2_register(void)
{
	rt_device_t key;
	key2_device.ops = &gpio_user_ops;
	key2_device.isr_flag = 1;

	rt_hw_gpio_register(&key2_device,key2_user_data.name,RT_DEVICE_FLAG_RDWR |RT_DEVICE_FLAG_INT_RX| RT_DEVICE_FLAG_STREAM,&key2_user_data);

	key = rt_device_find("key2");
	
	rt_device_init(key);
	
	rt_device_set_rx_indicate(key,key2_rx_ind);
	
}














#ifdef RT_USING_FINSH
#include <finsh.h>


void key(u8 str[])
{
	rt_device_t led;
	u8 dat = 0;

	led = rt_device_find(str);
	
	rt_device_read(led,0,&dat,0);
	rt_kprintf("key2 = 0x%x",dat);
}
FINSH_FUNCTION_EXPORT(key, key())
#endif





