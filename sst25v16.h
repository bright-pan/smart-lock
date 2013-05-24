#ifndef	__SST25V16_H__
#define 	__SST25V16_H__
#include "rtdevice.h"
#include "rtthread.h"
#include "spibus.h"

/* device name  -------------------------*/
#define SPI2_CS1_NAME1									("sst25")
#define SPI2_BUS_NAME										("spi2bus")
#define FLASH1_DEVICE_NAME							("flash1")


/* hardware define pin port clock -------------*/
#define SPI2_SCK_PIN										GPIO_Pin_13;
#define	 SPI2_MISO_PIN									GPIO_Pin_14
#define SPI2_MOSI_PIN										GPIO_Pin_15
#define SPI2_GPIO_PORT									GPIOB
#define SPI2_CLOCK											RCC_APB1Periph_SPI2


#define SPI2_CS1_PIN										GPIO_Pin_12
#define SPI2_CS1_PORT										GPIOB
#define SPI2_CS1_CLOCK									RCC_APB2Periph_GPIOB

#define GPIO_RCC												RCC_APB2Periph_GPIOB

void rt_spi_flash_init(void);




#endif




