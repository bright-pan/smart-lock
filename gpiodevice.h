#ifndef	__GPIODEVICE_H__
#define 	__GPIODEVICE_H__
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>

#define RT_DEVICE_CTRL_CONFIG           0x03    /* configure device */
#define RT_DEVICE_CTRL_SET_INT          0x10    /* enable receive irq */
#define RT_DEVICE_CTRL_CLR_INT          0x11    /* disable receive irq */

/*		
*	gpio_device data stuct
*/
typedef struct gpio_device
{
	struct rt_device 							parent;
	const struct rt_gpio_ops*			ops;
	rt_uint8_t										isr_flag	;	
}gpio_device;

/*		
*	gpio user data
*/
struct rt_gpio_ops
{
	rt_err_t (*configure)(gpio_device *gpio);
	rt_err_t (*control)(gpio_device *gpio, int cmd, void *arg);

	void  (*out)(gpio_device *gpio, rt_uint8_t c);
	rt_uint8_t (*intput)(gpio_device *gpio);
};









rt_err_t rt_hw_gpio_register(gpio_device *gpio,
                               const char            	 *name,
                               rt_uint32_t              flag,
                               void                    *data);




void rt_hw_gpio_isr(gpio_device *gpio);




#endif






