/**
******************************************************************************
* @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c
* @author  MCD Application Team
* @version V3.5.0
* @date    08-April-2011
* @brief   Main Interrupt Service Routines.
*          This file provides template for all exceptions handler and
*          peripherals interrupt service routine.
******************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include <board.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "gpio.h"

/** @addtogroup Template_Project
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief   This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void)
{
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void)
{
/* Go to infinite loop when Memory Manage exception occurs */
while (1)
{
}
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void)
{
/* Go to infinite loop when Bus Fault exception occurs */
while (1)
{
}
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void)
{
/* Go to infinite loop when Usage Fault exception occurs */
while (1)
{
}
}

/**
 * @brief  This function handles SVCall exception.
 * @param  None
 * @retval None
 */
void SVC_Handler(void)
{
}

/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void)
{
}

void SysTick_Handler(void)
{
extern void rt_hw_timer_handler(void);
rt_hw_timer_handler();
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/*******************************************************************************
 * Function Name  : DMA1_Channel2_IRQHandler
 * Description    : This function handles DMA1 Channel 2 interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void DMA1_Channel2_IRQHandler(void)
{
#ifdef RT_USING_USART3
extern struct rt_serial_device serial_device_usart3;
extern void rt_hw_serial_dma_tx_isr(struct rt_serial_device *serial);

/* enter interrupt */
rt_interrupt_enter();

if (DMA_GetITStatus(DMA1_IT_TC2))
{
/* transmission complete, invoke serial dma tx isr */
rt_hw_serial_dma_tx_isr(&serial_device_usart3);
}

/* clear DMA flag */
DMA_ClearFlag(DMA1_FLAG_TC2| DMA1_FLAG_TE2);

/* leave interrupt */
rt_interrupt_leave();
#endif
}
/*******************************************************************************
 * Function Name  : DMA1_Channel4_IRQHandler
 * Description    : This function handles DMA1 Channel 2 interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void DMA1_Channel4_IRQHandler(void)
{
#ifdef RT_USING_USART1
extern struct rt_serial_device serial_device_usart1;
extern void rt_hw_serial_dma_tx_isr(struct rt_serial_device *serial);

/* enter interrupt */
rt_interrupt_enter();

if (DMA_GetITStatus(DMA1_IT_TC4))
{
  /* transmission complete, invoke serial dma tx isr */
  rt_hw_serial_dma_tx_isr(&serial_device_usart1);
}

/* clear DMA flag */
DMA_ClearFlag(DMA1_FLAG_TC4| DMA1_FLAG_TE4);

/* leave interrupt */
rt_interrupt_leave();
#endif
}
/*******************************************************************************
 * Function Name  : DMA1_Channel7_IRQHandler
 * Description    : This function handles DMA1 Channel 2 interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void DMA1_Channel7_IRQHandler(void)
{
#ifdef RT_USING_USART2
  extern struct rt_serial_device serial_device_usart2;
  extern void rt_hw_serial_dma_tx_isr(struct rt_serial_device *serial);

  /* enter interrupt */
  rt_interrupt_enter();

  if (DMA_GetITStatus(DMA1_IT_TC7))
  {
    /* transmission complete, invoke serial dma tx isr */
    rt_hw_serial_dma_tx_isr(&serial_device_usart2);
  }

  /* clear DMA flag */
  DMA_ClearFlag(DMA1_FLAG_TC7| DMA1_FLAG_TE7);

  /* leave interrupt */
  rt_interrupt_leave();
#else
  extern struct rt_serial_device gsm_usart_device;
  extern void rt_hw_serial_dma_tx_isr(struct rt_serial_device *serial);

  /* enter interrupt */
  rt_interrupt_enter();

  if (DMA_GetITStatus(DMA1_IT_TC7))
  {
    /* transmission complete, invoke serial dma tx isr */
    rt_hw_serial_dma_tx_isr(&gsm_usart_device);
  }

  /* clear DMA flag */
  DMA_ClearFlag(DMA1_FLAG_TC7| DMA1_FLAG_TE7);

  /* leave interrupt */
  rt_interrupt_leave();  
#endif
}

/* uart 4 dma tx */
void DMA2_Channel4_5_IRQHandler(void)
{
  extern struct rt_serial_device rfid_uart_device;
  extern void rt_hw_serial_dma_tx_isr(struct rt_serial_device *serial);

  /* enter interrupt */
  rt_interrupt_enter();

  if (DMA_GetITStatus(DMA2_IT_TC5))
  {
    /* transmission complete, invoke serial dma tx isr */
    rt_hw_serial_dma_tx_isr(&rfid_uart_device);
  }

  /* clear DMA flag */
  DMA_ClearFlag(DMA2_FLAG_TC5| DMA2_FLAG_TE5);

  /* leave interrupt */
  rt_interrupt_leave();
}

/*******************************************************************************
 * Function Name  : USART1_IRQHandler
 * Description    : This function handles USART1 global interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void USART1_IRQHandler(void)
{
#ifdef RT_USING_USART1
  extern struct rt_serial_device serial_device_usart1;
  extern void serial_device_usart_isr(struct rt_serial_device *serial);

  /* enter interrupt */
  rt_interrupt_enter();

  serial_device_usart_isr(&serial_device_usart1);
  
  /* leave interrupt */
  rt_interrupt_leave();
#endif
}

/*******************************************************************************
 * Function Name  : USART2_IRQHandler
 * Description    : This function handles USART2 global interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void USART2_IRQHandler(void)
{
#ifdef RT_USING_USART2
  extern struct rt_serial_device serial_device_usart2;
  extern void serial_device_usart_isr(struct rt_serial_device *serial);

  /* enter interrupt */
  rt_interrupt_enter();

  serial_device_usart_isr(&serial_device_usart2);
  
  /* leave interrupt */
  rt_interrupt_leave();

#else
  
  extern struct rt_serial_device gsm_usart_device;
  extern void gsm_usart_device_isr(struct rt_serial_device *serial);

  /* enter interrupt */
  rt_interrupt_enter();

  gsm_usart_device_isr(&gsm_usart_device);
  
  /* leave interrupt */
  rt_interrupt_leave();
#endif
}

/*******************************************************************************
 * Function Name  : USART3_IRQHandler
 * Description    : This function handles USART3 global interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void USART3_IRQHandler(void)
{
#ifdef RT_USING_USART3
  extern struct rt_serial_device serial_device_usart3;
  extern void serial_device_usart_isr(struct rt_serial_device *serial);

  /* enter interrupt */
  rt_interrupt_enter();

  serial_device_usart_isr(&serial_device_usart3);
  
  /* leave interrupt */
  rt_interrupt_leave();
#endif
}

void UART4_IRQHandler(void)
{
  extern struct rt_serial_device rfid_uart_device;
  extern void rfid_uart_device_isr(struct rt_serial_device *serial);

  /* enter interrupt */
  rt_interrupt_enter();

  rfid_uart_device_isr(&rfid_uart_device);
  
  /* leave interrupt */
  rt_interrupt_leave();
}

void UART5_IRQHandler(void)
{
  extern struct rt_serial_device camera_uart_device;
  extern void camera_uart_device_isr(struct rt_serial_device *serial);

  /* enter interrupt */
  rt_interrupt_enter();

  camera_uart_device_isr(&camera_uart_device);
  
  /* leave interrupt */
  rt_interrupt_leave();
}

#if defined(RT_USING_DFS) && STM32_USE_SDIO
/*******************************************************************************
 * Function Name  : SDIO_IRQHandler
 * Description    : This function handles SDIO global interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void SDIO_IRQHandler(void)
{
  extern int SD_ProcessIRQSrc(void);

  /* enter interrupt */
  rt_interrupt_enter();

  /* Process All SDIO Interrupt Sources */
  SD_ProcessIRQSrc();

  /* leave interrupt */
  rt_interrupt_leave();
}
#endif

#ifdef RT_USING_LWIP
#ifdef STM32F10X_CL
/*******************************************************************************
 * Function Name  : ETH_IRQHandler
 * Description    : This function handles ETH interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void ETH_IRQHandler(void)
{
  extern void rt_hw_stm32_eth_isr(void);

  /* enter interrupt */
  rt_interrupt_enter();

  rt_hw_stm32_eth_isr();

  /* leave interrupt */
  rt_interrupt_leave();
}
#else
#if (STM32_ETH_IF == 0)
/*******************************************************************************
 * Function Name  : EXTI0_IRQHandler
 * Description    : This function handles External interrupt Line 0 request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void EXTI2_IRQHandler(void)
{
  extern void enc28j60_isr(void);

  /* enter interrupt */
  rt_interrupt_enter();

  enc28j60_isr();

  /* Clear the Key Button EXTI line pending bit */
  EXTI_ClearITPendingBit(EXTI_Line2);

  /* leave interrupt */
  rt_interrupt_leave();
}
#endif

#if (STM32_ETH_IF == 1)
/*******************************************************************************
 * Function Name  : EXTI4_IRQHandler
 * Description    : This function handles External lines 9 to 5 interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void EXTI4_IRQHandler(void)
{
  extern void rt_dm9000_isr(void);

  /* enter interrupt */
  rt_interrupt_enter();

  /* Clear the DM9000A EXTI line pending bit */
  EXTI_ClearITPendingBit(EXTI_Line4);

  rt_dm9000_isr();

  /* leave interrupt */
  rt_interrupt_leave();
}
#endif
#endif
#endif /* end of RT_USING_LWIP */

/**
 * @}
 */

void EXTI9_5_IRQHandler(void)
{
  extern void rt_hw_gpio_isr(gpio_device *gpio);
  
  /* enter interrupt */
  rt_interrupt_enter();
  /* lock_shell exti isr */
  if(EXTI_GetITStatus(EXTI_Line5) == SET)
  {
    extern gpio_device lock_shell_device;
		
    rt_hw_gpio_isr(&lock_shell_device);
    EXTI_ClearITPendingBit(EXTI_Line5);
  }
  /* motor status exti isr */
  if(EXTI_GetITStatus(EXTI_Line8) == SET)
  {
    extern gpio_device motor_status_device;
    rt_hw_gpio_isr(&motor_status_device);
    EXTI_ClearITPendingBit(EXTI_Line8);
  }
  /* battery switch exti isr */
  if(EXTI_GetITStatus(EXTI_Line9) == SET)
  {
    extern gpio_device battery_switch_device;
    rt_hw_gpio_isr(&battery_switch_device);
    EXTI_ClearITPendingBit(EXTI_Line9);
  }

  if(EXTI_GetITStatus(EXTI_Line6) == SET)
  {
    extern gpio_device key2_device;
    rt_hw_gpio_isr(&key2_device);
    EXTI_ClearITPendingBit(EXTI_Line6);
  }
  /* leave interrupt */
  rt_interrupt_leave();
}
/* camera photosensor exti isr */
void EXTI0_IRQHandler(void)
{
  extern void rt_hw_gpio_isr(gpio_device *gpio);
  
  /* enter interrupt */
  rt_interrupt_enter();
  if(EXTI_GetITStatus(EXTI_Line0) == SET)
  {
    extern gpio_device camera_photosensor_device;
		
    rt_hw_gpio_isr(&camera_photosensor_device);
    EXTI_ClearITPendingBit(EXTI_Line0);
  }
  /* leave interrupt */
  rt_interrupt_leave();
}
/* camera irda sensor exti isr */
void EXTI1_IRQHandler(void)
{
  extern void rt_hw_gpio_isr(gpio_device *gpio);
  
  /* enter interrupt */
  rt_interrupt_enter();
  if(EXTI_GetITStatus(EXTI_Line1) == SET)
  {
    extern gpio_device camera_irdasensor_device;
		
    rt_hw_gpio_isr(&camera_irdasensor_device);
    EXTI_ClearITPendingBit(EXTI_Line1);
  }
  /* leave interrupt */
  rt_interrupt_leave();
}
/* rfid key detect exti isr */
void EXTI15_10_IRQHandler(void)
{
  extern void rt_hw_gpio_isr(gpio_device *gpio);
  
  /* enter interrupt */
  rt_interrupt_enter();
  /* rfid_key_detect exti isr */
  if(EXTI_GetITStatus(EXTI_Line10) == SET)
  {
    extern gpio_device rfid_key_detect_device;
    rt_hw_gpio_isr(&rfid_key_detect_device);
    EXTI_ClearITPendingBit(EXTI_Line10);
  }
  if(EXTI_GetITStatus(EXTI_Line11) == SET)
  {
    extern gpio_device lock_shell_device;
    rt_hw_gpio_isr(&lock_shell_device);
    EXTI_ClearITPendingBit(EXTI_Line11);
  }
  if(EXTI_GetITStatus(EXTI_Line12) == SET)
  {
    extern gpio_device lock_gate_device;
    rt_hw_gpio_isr(&lock_gate_device);
    EXTI_ClearITPendingBit(EXTI_Line12);
  }
  if(EXTI_GetITStatus(EXTI_Line13) == SET)
  {
    extern gpio_device gsm_ring_device;
    rt_hw_gpio_isr(&gsm_ring_device);
    EXTI_ClearITPendingBit(EXTI_Line13);
  }
  if(EXTI_GetITStatus(EXTI_Line14) == SET)
  {
    extern gpio_device lock_temperature_device;
    rt_hw_gpio_isr(&lock_temperature_device);
    EXTI_ClearITPendingBit(EXTI_Line14);
  }
  if(EXTI_GetITStatus(EXTI_Line15) == SET)
  {
    extern gpio_device gate_temperature_device;
    rt_hw_gpio_isr(&gate_temperature_device);
    EXTI_ClearITPendingBit(EXTI_Line15);
  }
  /* leave interrupt */
  rt_interrupt_leave();
}

void TIM1_BRK_IRQHandler(void)
{

}

/* voice data device isr */
void TIM1_UP_IRQHandler(void)
{
  extern struct gpio_pwm_user_data voice_data_user_data;
  /* enter interrupt */
  rt_interrupt_enter();
  /* tim1 update isr */
  if(TIM_GetITStatus(voice_data_user_data.timx, TIM_IT_Update) == SET)
  {
    TIM_ClearITPendingBit(voice_data_user_data.timx, TIM_IT_Update);
    if (voice_data_user_data.tim_pulse_counts == 0)
    {
      TIM_CCxCmd(voice_data_user_data.timx, voice_data_user_data.tim_oc_channel, TIM_CCx_Disable);
      TIM_Cmd(voice_data_user_data.timx, DISABLE);
    }    
  }  
  /* leave interrupt */
  rt_interrupt_leave();
}
void TIM1_TRG_COM_IRQHandler(void)
{

}
/* voice data device isr */
void TIM1_CC_IRQHandler(void)
{
  extern struct gpio_pwm_user_data voice_data_user_data;
  /* enter interrupt */
  rt_interrupt_enter();
  /* tim1 cc4 isr */
  if(TIM_GetITStatus(voice_data_user_data.timx, TIM_IT_CC4) == SET)
  {
    TIM_ClearITPendingBit(voice_data_user_data.timx, TIM_IT_CC4);
    if (voice_data_user_data.tim_pulse_counts > 0)
    {
      voice_data_user_data.tim_pulse_counts--;
    }
  }
  /* leave interrupt */
  rt_interrupt_leave();

}

void TIM3_IRQHandler(void)
{
  extern struct gpio_pwm_user_data pwm1_user_data;
  /* enter interrupt */
  rt_interrupt_enter();
  /* pwm1 update isr */
  if(TIM_GetITStatus(pwm1_user_data.timx, TIM_IT_CC1) == SET)
  {
    //rt_hw_gpio_isr(&rfid_key_detect_device);
    TIM_ClearITPendingBit(pwm1_user_data.timx, TIM_IT_CC1);
    if (pwm1_user_data.tim_pulse_counts > 0)
    {
      pwm1_user_data.tim_pulse_counts--;
    }
    else
    {
      //TIM_Cmd(pwm1_user_data.timx, DISABLE);
      //TIM_CCxCmd(pwm1_user_data.timx, TIM_Channel_1, TIM_CCx_Disable);
    }
    
  }
  if(TIM_GetITStatus(pwm1_user_data.timx, TIM_IT_Update) == SET)
  {
    //rt_hw_gpio_isr(&rfid_key_detect_device);
    TIM_ClearITPendingBit(pwm1_user_data.timx, TIM_IT_Update);
    if (pwm1_user_data.tim_pulse_counts == 0)
    {
      //TIM_Cmd(pwm1_user_data.timx, DISABLE);
      //TIM_CCxCmd(pwm1_user_data.timx, TIM_Channel_1, TIM_CCx_Disable);
      TIM_CCxCmd(pwm1_user_data.timx, TIM_Channel_1, TIM_CCx_Disable);
      TIM_Cmd(pwm1_user_data.timx, DISABLE);
    }
    
  }  
  /* leave interrupt */
  rt_interrupt_leave();
}

/* motor pwm device irq */
void TIM8_UP_IRQHandler(void)
{
  extern struct gpio_pwm_user_data motor_a_pulse_user_data;
  extern struct gpio_pwm_user_data motor_b_pulse_user_data;
  /* enter interrupt */
  rt_interrupt_enter();
  /* tim8 update isr */
  if(TIM_GetITStatus(motor_a_pulse_user_data.timx, TIM_IT_Update) == SET)
  {
    TIM_ClearITPendingBit(motor_a_pulse_user_data.timx, TIM_IT_Update);
    if (motor_a_pulse_user_data.tim_pulse_counts == 0)
    {
      TIM_CCxCmd(motor_a_pulse_user_data.timx, motor_a_pulse_user_data.tim_oc_channel, TIM_CCx_Disable);
      if (motor_b_pulse_user_data.tim_pulse_counts == 0)
      {
        TIM_Cmd(motor_a_pulse_user_data.timx, DISABLE);
      }
    }    
    if (motor_b_pulse_user_data.tim_pulse_counts == 0)
    {
      TIM_CCxCmd(motor_b_pulse_user_data.timx, motor_b_pulse_user_data.tim_oc_channel, TIM_CCx_Disable);
      if (motor_a_pulse_user_data.tim_pulse_counts == 0)
      {
        TIM_Cmd(motor_b_pulse_user_data.timx, DISABLE);
      }
    }
  }  
  /*
  if(TIM_GetITStatus(motor_b_pulse_user_data.timx, TIM_IT_Update) == SET)
  {
    TIM_ClearITPendingBit(motor_b_pulse_user_data.timx, TIM_IT_Update);
    if (motor_b_pulse_user_data.tim_pulse_counts == 0)
    {
      TIM_CCxCmd(motor_b_pulse_user_data.timx, motor_b_pulse_user_data.tim_oc_chanel, TIM_CCx_Disable);
      TIM_Cmd(motor_b_pulse_user_data.timx, DISABLE);
    }
    
  }
  */
  /* leave interrupt */
  rt_interrupt_leave();
}
void TIM8_CC_IRQHandler(void)
{
  extern struct gpio_pwm_user_data motor_a_pulse_user_data;
  extern struct gpio_pwm_user_data motor_b_pulse_user_data;
  /* enter interrupt */
  rt_interrupt_enter();
  /* tim8 cc3 isr */
  if(TIM_GetITStatus(motor_a_pulse_user_data.timx, TIM_IT_CC3) == SET)
  {
    TIM_ClearITPendingBit(motor_a_pulse_user_data.timx, TIM_IT_CC3);
    if (motor_a_pulse_user_data.tim_pulse_counts > 0)
    {
      motor_a_pulse_user_data.tim_pulse_counts--;
    }
  }
  if(TIM_GetITStatus(motor_b_pulse_user_data.timx, TIM_IT_CC4) == SET)
  {
    TIM_ClearITPendingBit(motor_b_pulse_user_data.timx, TIM_IT_CC4);
    if (motor_b_pulse_user_data.tim_pulse_counts > 0)
    {
      motor_b_pulse_user_data.tim_pulse_counts--;
    }
  }
  /* leave interrupt */
  rt_interrupt_leave();
}

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
