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

class SdCard
{
public:
   SdCard(){}

   void Init(uint16_t main_task_priority);

   static void MainTaskWrapper(void* param);
   void MainTask();

private:
   void InitializeSdCard();
   bool IsCardDetected();
   FRESULT CreateIndexedFile(char* file_name, char* indexed_file_name);

   //FIL      fil;        /* File object */

};

#endif /* SOURCES_SDCARD_H_ */
