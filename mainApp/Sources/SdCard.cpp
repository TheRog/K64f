/*
 * SdCard.cpp
 *
 *  Created on: Oct 7, 2015
 *      Author: Schuck
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "SdCard.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_mpu_hal.h"

SdCard* staticInstance;

void SdCard::PORTC_IRQHandler()
{
   GPIO_DRV_ClearPinIntFlag (kGpioSW2); // Clear IRQ flag
   staticInstance->SwPressed();
}

void SdCard::SwPressed()
{
   PRINTF("SD_CARD: SW2 ISR\r\n");
   OSA_EventSet(&switchPressedEvent, 1);
}

void SdCard::Init(
      uint16_t main_task_priority,
      SdCardInterface* sd_interface
      )
{
   staticInstance = this;

   sdCardInterface = sd_interface;

   sdDiagPin.pinName = kSdDiagPin;
   sdDiagPin.config.outputLogic = 0;
   sdDiagPin.config.slewRate = kPortFastSlewRate;
   sdDiagPin.config.driveStrength = kPortHighDriveStrength;
   sdDiagPin.config.isOpenDrainEnabled = false;
   GPIO_DRV_OutputPinInit(&sdDiagPin);

   swInput.pinName = kGpioSW2;
   swInput.config.isPullEnable = true;
   swInput.config.pullSelect = kPortPullUp;
   swInput.config.isPassiveFilterEnabled = false;
   swInput.config.interrupt = kPortIntFallingEdge;
   GPIO_DRV_InputPinInit(&swInput); //SW2

   OSA_InstallIntHandler(PORTC_IRQn, &SdCard::PORTC_IRQHandler);

   OSA_EventCreate(&switchPressedEvent, kEventAutoClear);

   OSA_TaskCreate(
         &MainTaskWrapper, // Task Pointer
         (uint8_t*)"SD Main Task", // Name
         1024, // Stack Size
         NULL, // Stack Memory
         main_task_priority, // Priority
         this, // Parameters
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

   InitializeSdCard();

   event_flags_t flags;

   while(1)
   {
      PRINTF("SD_CARD: Press SW2 to open file\r\n");

      OSA_EventClear(&switchPressedEvent, 1);
      OSA_EventWait(&switchPressedEvent, 1, false, OSA_WAIT_FOREVER, &flags);

      PRINTF("SD_CARD: Opening text file\r\n");

      uint32_t bytes_written;
      off_t    new_line;

      char file_string[30];

      if(CreateIndexedFile((char*)"data.txt", file_string))
      {
         PRINTF("SD_CARD: Error opening text file\r\n");
         for(;;){OSA_TimeDelay(1000);}
      }

      PRINTF("SD_CARD: File created - %s \r\n", file_string);
      PRINTF("SD_CARD: Press SW2 to close file\r\n");

      OSA_EventClear(&switchPressedEvent, 1);
      while(1)
      {
         /* Write the constructed string to the new line in text file */
         GPIO_DRV_WritePinOutput(sdDiagPin.pinName, 1);
         if(sdCardInterface)
         {
            uint32_t length = sdCardInterface->UpdateText(text, TextLength);
            fr = f_write(&fil, (void*)text, length, &bytes_written);
         }
         GPIO_DRV_WritePinOutput(sdDiagPin.pinName, 0);

         //OSA_TimeDelay(5);

//         GPIO_DRV_WritePinOutput(sdDiagPin.pinName, 1);
//         f_sync(&fil);
//         GPIO_DRV_WritePinOutput(sdDiagPin.pinName, 0);
//
         OSA_TimeDelay(10);

         if(OSA_EventGetFlags(&switchPressedEvent))
         {
            PRINTF("SD_CARD: Closing File - %s \r\n", file_string);
            f_close(&fil);
            break;
         }
      }
   }
}

//Create a indexed file with an input of name.xxx
FRESULT SdCard::CreateIndexedFile(char* file_name, char* indexed_file_name)
{
   FRESULT fr;
   const uint8_t ExtensionLength = 4;
   //Get the number of characters without extension
   uint8_t name_length = strlen( file_name)-ExtensionLength;
   //Null terminated string to hold the indexed value
   char indexed_name[32];
   memset(indexed_name, 0, sizeof(indexed_name));
   //Set the number of extension digits
   const uint8_t nbr_of_index_digits = 4;
   //Set the maximum size of the indexed file_name - null[1] - extension[4]
   uint8_t max_name_length = sizeof(indexed_name)-nbr_of_index_digits-1-ExtensionLength;
   //Grab the extension
   char extension[ExtensionLength];
   memcpy(extension, &file_name[name_length], ExtensionLength);
   //Check desired length
   if( name_length > max_name_length)
   {
      //The desired file_name is too long
      return FR_INVALID_PARAMETER;
   }

   //Move the file_name to the indexed file_name, remove the extension
   memcpy(indexed_name,file_name,name_length);

   //Create the file
   for( int x=0; x<(int)pow(10.f,nbr_of_index_digits); x++)
   {
      uint8_t offset = snprintf( &indexed_name[name_length], nbr_of_index_digits+1, "%d", x);
      memcpy(&indexed_name[name_length+offset],extension,ExtensionLength);

      fr = f_open(&fil, indexed_name, FA_WRITE | FA_CREATE_NEW);
      if(fr == FR_OK)
      {
         break;
      }
   }

   //Set the filename
   if( indexed_file_name != NULL)
   {
      strcpy( indexed_file_name, indexed_name);
   }

   return fr;
}

void SdCard::InitializeSdCard()
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
