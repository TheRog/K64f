/*
 * SdCard.cpp
 *
 *  Created on: Oct 7, 2015
 *      Author: Schuck
 */

#include <stdio.h>
#include <string.h>

#include "SdCard.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_os_abstraction.h"
#include "fsl_mpu_hal.h"

FATFS FatFs; /* FatFs system object */

int write_pin_name = GPIO_MAKE_PIN(GPIOB_IDX, 9U);
gpio_output_pin_user_config_t writePin;

const int length = 128;
char string[length];

void SdCard::Init( uint16_t main_task_priority)
{
   writePin.pinName = write_pin_name;
   writePin.config.outputLogic = 0;
   writePin.config.slewRate = kPortFastSlewRate;
   writePin.config.driveStrength = kPortHighDriveStrength;
   writePin.config.isOpenDrainEnabled = false;
   GPIO_DRV_OutputPinInit(&writePin);

   OSA_TaskCreate(
         &MainTaskWrapper, // Task Pointer
         (uint8_t*)"SD Main Task", // Name
         1024, // Stack Size
         NULL, // Stack Memory
         main_task_priority, // Priority
         NULL, // Parameters
         true, // Uses Floats
         NULL //Handler
         );
}

void SdCard::MainTaskWrapper(void* param)
{
   ((SdCard*)param)->MainTask();
}

void SdCard::MainTask()
{
   FRESULT fr;    /* FatFs return code */
   DRESULT ds;    /* Disk functions return code */

   OSA_TimeDelay(1000);

   PRINTF("SD_CARD: Starting task\r\n");

   /* Initialize SD Card detect input pin */
   GPIO_DRV_InputPinInit(&sdhcCdPin[0]);

   /* Initialize safe removal pin (SW3 = PTA4) */
   GPIO_DRV_InputPinInit(&switchPins[1]);

   if(!IsCardDetected())
   {
      PRINTF("SD_CARD: Please insert SD Card\r\n");
      /* Wait for SD Card insertion */
      while (!IsCardDetected()){OSA_TimeDelay(100);}
   }

   PRINTF("SD_CARD: SD Card Detected\r\n");

   PRINTF("SD_CARD: Initializing SD Card\r\n");

   /* Disable Memory Protection Unit */
   MPU_HAL_Disable(MPU);

   /* Initialize SDHC driver and SD Card */
   ds = (DRESULT)disk_initialize(SD);
   if(ds)
   {
      PRINTF("SD_CARD: Failed to initialize SD disk\r\n");
      for(;;){OSA_TimeDelay(1000);}
   }

   /* Select current logical device driver (0 = USB, 1 = SD) */
   fr = f_chdrive(SD);
   PRINTF("SD_CARD: Mounting file system to SD Card volume...\r\n");

   /* Mount file system to the SDCARD volume */
   fr = f_mount(SD, &FatFs);
   if(fr)
   {
      PRINTF("SD_CARD: Error mounting file system\r\n");
      for(;;){OSA_TimeDelay(1000);}
   }

   PRINTF("SD_CARD: Not safe to remove SD Card\r\n");

   OSA_TimeDelay(2000);

   FIL      fil;        /* File object */
   uint32_t bytes_written;
   off_t    new_line;

   /* Open a text file */
   fr = f_open(&fil, "rog_data.txt", FA_WRITE | FA_OPEN_ALWAYS);
   if(fr)
   {
      PRINTF("SD_CARD: Error opening text file\r\n");
      for(;;){OSA_TimeDelay(1000);}
   }

   new_line = f_size(&fil);
   /* Set file pointer to the start of new line in text file */
   fr = f_lseek(&fil, new_line);


   for(int i=0; i<length; i++)
   {
      string[i] = (char)i;
   }

   while(1)
   {
      /* Write the constructed string to the new line in text file */
      GPIO_DRV_WritePinOutput(writePin.pinName, 1);
      fr = f_write(&fil, (void*)string, length, &bytes_written);
      GPIO_DRV_WritePinOutput(writePin.pinName, 0);

      OSA_TimeDelay(5);

      GPIO_DRV_WritePinOutput(writePin.pinName, 1);
      f_sync(&fil);
      GPIO_DRV_WritePinOutput(writePin.pinName, 0);

      OSA_TimeDelay(100);
   }
}

bool SdCard::IsCardDetected(void)
{
   uint32_t value = 0;
   if (sdhcCdPin[0].config.pullSelect == kPortPullUp) /* pull up */
   {
      value = GPIO_DRV_ReadPinInput(sdhcCdPin[0].pinName);
      return (!value);
   }
   else /* pull down */
   {
      return (GPIO_DRV_ReadPinInput(sdhcCdPin[0].pinName));
   }
}
