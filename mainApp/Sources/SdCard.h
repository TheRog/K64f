/*
 * SdCard.h
 *
 *  Created on: Oct 7, 2015
 *      Author: Schuck
 */

#ifndef SOURCES_SDCARD_H_
#define SOURCES_SDCARD_H_

#include <stdint.h>

#include "diskio.h"
#include "ff.h"
#include "fsl_gpio_driver.h"
#include "fsl_os_abstraction.h"

class SdCardInterface
{
public:
   virtual uint32_t UpdateText(char* text, uint32_t max_length) = 0;
};

class SdCard
{
public:
   SdCard(){}

   void Init(
         uint16_t main_task_priority,
         SdCardInterface* sd_interface
         );

   // Main Task
   static void MainTaskWrapper(void* param);
   void MainTask();

private:
   void InitializeSdCard();
   bool IsCardDetected();

   static void PORTC_IRQHandler(); // Static ISR
   void SwPressed(); // Called from the ISR

   // Auto increment and create file
   FRESULT CreateIndexedFile(char* file_name, char* indexed_file_name);

   SdCardInterface* sdCardInterface;

   FATFS   FatFs;    /* FatFs system object */
   FIL     fil;      /* File object */

   event_t switchPressedEvent;

   gpio_output_pin_user_config_t sdDiagPin;

   gpio_input_pin_user_config_t swInput;

   //Temporary write string
   static const int length = 128;
   char string[length];

   static const uint32_t TextLength = 1024;
   char text[TextLength];

};

#endif /* SOURCES_SDCARD_H_ */
