/*********************************************************************
 * Filename:      firm_update.c
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-07-18 18:06:56
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "firm_update.h"

typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;

#if defined (STM32F10X_HD) || defined (STM32F10X_HD_VL) || defined (STM32F10X_CL) || defined (STM32F10X_XL)
  #define FLASH_PAGE_SIZE    ((uint16_t)0x800)
#else
  #define FLASH_PAGE_SIZE    ((uint16_t)0x400)
#endif

#define BANK1_WRITE_START_ADDR  ((uint32_t)0x08000000)
#define BANK1_WRITE_END_ADDR    ((uint32_t)0x08060000)

void firm_update(void)
{
  uint32_t EraseCounter = 0x00, Address = 0x00, address_src;
  uint32_t Data = 0x3210ABCD;
  uint32_t NbrOfPage = 0x00;
  volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;
  volatile TestStatus MemoryProgramStatus = PASSED;

#ifdef STM32F10X_XL
  volatile TestStatus MemoryProgramStatus2 = PASSED;
#endif /* STM32F10X_XL */

  rt_hw_interrupt_disable();
  /* Unlock the Flash Bank1 Program Erase controller */
  FLASH_UnlockBank1();

  /* Define the number of page to be erased */
  NbrOfPage = (BANK1_WRITE_END_ADDR - BANK1_WRITE_START_ADDR) / FLASH_PAGE_SIZE;

  /* Clear All pending flags */
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

  /* Erase the FLASH pages */
  for(EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
  {
    FLASHStatus = FLASH_ErasePage(BANK1_WRITE_START_ADDR + (FLASH_PAGE_SIZE * EraseCounter));
  }
  
  /* Program Flash Bank1 */
  Address = BANK1_WRITE_START_ADDR;
  address_src = FLASH_START_WRITE_ADDR;

  while((Address < BANK1_WRITE_END_ADDR) && (FLASHStatus == FLASH_COMPLETE))
  {
    SST25_R_BLOCK(address_src, (u8 *)&Data, 4);
    FLASHStatus = FLASH_ProgramWord(Address, Data);
    Address = Address + 4;
    address_src = address_src + 4;
  }

  FLASH_LockBank1();

  NVIC_SystemReset();
  
  /* Check the correctness of written data 
  Address = BANK1_WRITE_START_ADDR;

  while((Address < BANK1_WRITE_END_ADDR) && (MemoryProgramStatus != FAILED))
  {
    if((*(__IO uint32_t*) Address) != Data)
    {
      MemoryProgramStatus = FAILED;
    }
    Address += 4;
  }
  */
}
