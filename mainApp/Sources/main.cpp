/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_device_registers.h"
#include "fsl_os_abstraction.h"
#include "fsl_fxos8700_driver.h"
#include "diskio.h"
#include "board.h"
#include "ff.h"
#include "fapp.h"
#include <stdio.h>
#include <string.h>

#include "fsl_debug_console.h"
#include "fsl_mpu_hal.h"
#include "fsl_dspi_shared_function.h"
#include "fsl_dspi_master_driver.h"
#include "fsl_pit_driver.h"

#include "SdCard.h"
#include "DRV_MPU9250.h"

#include "FreeRTOS.h"
#include "task.h"

#define BOARD_PIT_INSTANCE  0

void log_accel_values(int16_t x, int16_t y, int16_t z);
void int16tostr (int16_t num, char* str_buff);

SdCard sdCard;
DRV_MPU9250 drv_Mpu9250;

int test_pin_name = GPIO_MAKE_PIN(GPIOB_IDX, 9U);
gpio_output_pin_user_config_t testPin;

static int idle_counts = 0;

class SdCardOutputStream : public SdCardInterface
{
   virtual uint32_t UpdateText(char* text, uint32_t max_length)
   {
      int bytes_written = 0;
      bytes_written += snprintf(text, max_length, "Timestamp = %d\r\n", OSA_TimeGetMsec());
      return bytes_written;
   }
};

SdCardOutputStream sdCardOutputStream;

static void FnetTask()
{
   // Delay the start of FNET
   OSA_TimeDelay(5000);

   /* Init UART. */
    fnet_cpu_serial_init(FNET_CFG_CPU_SERIAL_PORT_DEFAULT, 115200);

    /* Enable Interrupts.*/
   fnet_cpu_irq_enable(0);

   /* Run FNET application. - Function does not return */
   fapp_main();

   while(1){OSA_TimeDelay(1000);}
}

static void MainTask(void *arg)
{
   OSA_TimeDelay(500);

   drv_Mpu9250.Init();

   OSA_TimeDelay(500);

   PRINTF("\n****** uServer ******\r\n");

   LED1_EN; LED2_EN; LED3_EN;

   int counter = 0;
   while(1)
   {
      //drv_Mpu9250.Update();

      LED1_TOGGLE; OSA_TimeDelay(50);
      LED3_TOGGLE; OSA_TimeDelay(50);
      LED2_TOGGLE; OSA_TimeDelay(50);

      LED1_TOGGLE; OSA_TimeDelay(50);
      LED3_TOGGLE; OSA_TimeDelay(50);
      LED2_TOGGLE; OSA_TimeDelay(50);

      OSA_TimeDelay(700);
      counter++;
   }
}

int main (void)
{
   OSA_Init();

   /* Initialize clocks, debug console interface and configure required pins */
   hardware_init();

   /* Disable Memory Protection Unit */
   MPU_HAL_Disable(MPU);

	testPin.pinName = test_pin_name;
	testPin.config.outputLogic = 0;
	testPin.config.slewRate = kPortFastSlewRate;
	testPin.config.driveStrength = kPortHighDriveStrength;
	testPin.config.isOpenDrainEnabled = false;
	GPIO_DRV_OutputPinInit(&testPin);

	// Structure of initialize PIT channel No.0
   pit_user_config_t chn0Confg;
   chn0Confg.isInterruptEnabled = true;
   chn0Confg.periodUs = 1000000u;

   // Init pit module and enable run in debug
   PIT_DRV_Init(BOARD_PIT_INSTANCE, false);

   // Initialize PIT timer instance for channel 0 and 1
   PIT_DRV_InitChannel(BOARD_PIT_INSTANCE, 0, &chn0Confg);

   // Start channel 0
   PRINTF("\n\rStarting channel No.0 ...");
   PIT_DRV_StartTimer(BOARD_PIT_INSTANCE, 0);

//	drv_Mpu9250.Init();
	sdCard.Init(1, &sdCardOutputStream);

   OSA_TaskCreate((task_t)MainTask,   (uint8_t*)"Main Task",    4096, NULL, 2, NULL, true, NULL);
   //OSA_TaskCreate((task_t)FnetTask,   (uint8_t*)"FNET Task",    2048, NULL, 3, NULL, true, NULL);

   OSA_Start(); // This function will not return

   while(1);

   return(0);
}



///////////////////////////////////////////////////////////////////////////////
// Old Functions
///////////////////////////////////////////////////////////////////////////////

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : log_accel_values
* Comments       : Log accelerometer data values (x, y, z) in SD Card
*
* Parameters:	[X] - Accelerometer x axis value
* 		[Y] - Accelerometer y axis value
* 		[Z] - Accelerometer z axis value
*
*END*--------------------------------------------------------------------*/

void log_accel_values(int16_t x, int16_t y, int16_t z)
{
	FRESULT		fr;			/* FatFs functions return code */
	FIL			fil;			/* File object */
	uint32_t	bytes_written;
	char		str_number[10];
	char		aux_string[100];
	off_t		new_line;

	GPIO_DRV_WritePinOutput(testPin.pinName, 1);

	/* Open a text file */
	fr = f_open(&fil, "log_data.txt", FA_WRITE | FA_OPEN_ALWAYS);
	if(fr)
	{
		PRINTF("\nError opening text file\r\n");
		for(;;){}
	}

	//GPIO_DRV_TogglePinOutput(testPin.pinName);

	new_line = f_size(&fil);

	/* Set file pointer to the start of new line in text file */
	fr = f_lseek(&fil, new_line);

	//GPIO_DRV_TogglePinOutput(testPin.pinName);

	aux_string[0] = '\0';

	/* Append x value */
	int16tostr(x, str_number);
	strcat(aux_string, str_number);
	strcat(aux_string, "\t");

	/* Append y value */
	int16tostr(y, str_number);
	strcat(aux_string, str_number);
	strcat(aux_string, "\t");

	/* Append z value */
	int16tostr(z, str_number);
	strcat(aux_string, str_number);
	strcat(aux_string, "\r\n");

	GPIO_DRV_TogglePinOutput(testPin.pinName);

	/* Write the constructed string to the new line in text file */
	fr = f_write(&fil, aux_string, strlen(aux_string), &bytes_written);

	GPIO_DRV_TogglePinOutput(testPin.pinName);

	if(fr)
	{
	   PRINTF("\nError logging data\r\n");
		for(;;){}
	}

	/* Close the text file */
	fr = f_close(&fil);

	//GPIO_DRV_TogglePinOutput(testPin.pinName);

	GPIO_DRV_WritePinOutput(testPin.pinName, 0);
}

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : int16tostr
* Comments       : Converts a 16-bit integer number to a string
*
* Parameters:	[num] - 16-bit integer number
* 		[str_buff] - Pointer to the output string
*
*END*--------------------------------------------------------------------*/

void int16tostr (int16_t num, char* str_buff)
{
	uint8_t		i = 0, j = 0;
	char		temp[10];

	if(num < 0)
	{
		num = -num;
		str_buff[0] = '-';
		j = 1;
	}

	while(num > 0)
	{
		temp[i++] = (char)((num % 10) + 0x30);
		num /= 10;
	}

	for(; i >= 1; j++, i--)
	{
		str_buff[j] = temp[i-1];
	}
	str_buff[j] = '\0';
}



extern "C" {

void SPI0_IRQHandler(void)
{
   DSPI_DRV_MasterIRQHandler(SPI0_IDX);
   PRINTF("SPI: ISR Detected!!! \n\r");
}
void SPI1_IRQHandler(void)
{
   DSPI_DRV_MasterIRQHandler(SPI1_IDX);
   PRINTF("SPI: ISR Detected!!! \n\r");
}
void SPI2_IRQHandler(void)
{
   DSPI_DRV_MasterIRQHandler(SPI2_IDX);
   PRINTF("SPI: ISR Detected!!! \n\r");
}

const int buffer_length = 1024;
char buffer[buffer_length];
void PIT0_IRQHandler(void)
{
    /* Clear interrupt flag.*/
    PIT_HAL_ClearIntFlag(g_pitBase[0], 0U);
    //PRINTF("PITR ISR Detected!!! \n\r");
    //PRINTF(".");

    PRINTF("%d\r\n", idle_counts);
    idle_counts = 0;
}

void HardFault_Handler(){
   while(1);
}
void MemManage_Handler(){
   while(1);
}
void BusFault_Handler(){
   while(1);
}
void UsageFault_Handler(){
   while(1);
}

void vApplicationMallocFailedHook( void )
{
   taskDISABLE_INTERRUPTS();
   for( ;; );
}

void vApplicationIdleHook( void )
{
   /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
   task.  It is essential that code added to this hook function never attempts
   to block in any way (for example, call xQueueReceive() with a block time
   specified, or call vTaskDelay()).  If the application makes use of the
   vTaskDelete() API function (as this demo application does) then it is also
   important that vApplicationIdleHook() is permitted to return to its calling
   function, because it is the responsibility of the idle task to clean up
   memory allocated by the kernel to any task that has since been deleted. */

   idle_counts++;
}

}
