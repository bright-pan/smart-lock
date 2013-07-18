#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__
#include "stm32f10x.h"

#define Dummy_Byte 0

/* Select SPI FLASH: ChipSelect pin low  */
#define Select_Flash()     GPIO_ResetBits(GPIOB, GPIO_Pin_12)
/* Deselect SPI FLASH: ChipSelect pin high */
#define NotSelect_Flash()    GPIO_SetBits(GPIOB, GPIO_Pin_12)



void SST25_R_BLOCK(u32 addr, u8 *readbuff, u16 BlockSize);
void SST25_W_BLOCK(u32 addr, u8 *readbuff, u16 BlockSize);

void SPI_Flash_Init(void);

#endif


