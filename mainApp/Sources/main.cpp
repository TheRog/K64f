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

#if (defined(TWR_K64F120M) || defined(FRDM_K64F) || defined(TWR_K60D100M) || \
	defined(TWR_K21F120M) || defined(TWR_K65F180M))
#include "fsl_mpu_hal.h"
#endif

bool sdhc_detect(void);
void log_accel_values(int16_t x, int16_t y, int16_t z);
void int16tostr (int16_t num, char* str_buff);

FATFS FatFs;	/* FatFs system object */

#ifdef __cplusplus
extern "C" {
#endif
void vApplicationMallocFailedHook( void )
{
	while(1);
}
#ifdef __cplusplus
}
#endif

static void SdCardTask(void *arg)
{
	FRESULT fr;		/* FatFs return code */
	DRESULT ds;		/* Disk functions return code */

	fxos_data_t sensorData;
	fxos_handler_t i2cModule;
	int16_t xData, yData, zData;

	OSA_TimeDelay(1000);

	/* Initialize the FXOS8700CQ */
	i2cModule.i2cInstance = BOARD_I2C_FXOS8700CQ_INSTANCE;
	FXOS_Init(&i2cModule, NULL);

	/* Initialize SD Card detect input pin */
	GPIO_DRV_InputPinInit(&sdhcCdPin[0]);

	/* Initialize safe removal pin (SW3 = PTA4) */
	GPIO_DRV_InputPinInit(&switchPins[1]);

	debug_printf("\n****** KSDK: FatFs + SD CARD demo ******\r\n");

	if(!sdhc_detect())
	{
	   debug_printf("\nPlease insert SD Card\r\n");

		/* Wait for SD Card insertion */
		while (!sdhc_detect());
	}

	debug_printf("\nSD Card inserted\r\n");

	debug_printf("\nInitializing SD Card...\r\n");

#if (defined(TWR_K64F120M) || defined(FRDM_K64F) || defined(TWR_K60D100M) || \
	defined(TWR_K21F120M) || defined(TWR_K65F180M))
	/* Disable Memory Protection Unit */
	MPU_HAL_Disable(MPU);
#endif

	/* Initialize SDHC driver and SD Card */
	ds = (DRESULT)disk_initialize(SD);
	if(ds)
	{
	   debug_printf("\nFailed to initialize SD disk\r\n");
		for(;;){}
	}

	/* Select current logical device driver (0 = USB, 1 = SD) */
	fr = f_chdrive(SD);
	debug_printf("\nMounting file system to SD Card volume...\r\n");

	/* Mount file system to the SDCARD volume */
	fr = f_mount(SD, &FatFs);
	if(fr)
	{
	   debug_printf("\nError mounting file system\r\n");
		for(;;){}
	}

	debug_printf("\nLogging accelerometer values... \r\n");
	debug_printf("\nNOTE: To safely remove SD Card, press and hold SW3\r\n");

	for (;;)
	{
		// Get new accelerometer data.
		FXOS_ReadData(&i2cModule, &sensorData);

		// Get the X and Y data from the sensor data structure.
		xData = (int16_t)((sensorData.accelXMSB << 8) | sensorData.accelXLSB);
		yData = (int16_t)((sensorData.accelYMSB << 8) | sensorData.accelYLSB);
		zData = (int16_t)((sensorData.accelZMSB << 8) | sensorData.accelZLSB);

		debug_printf("\nx = %d  y = %d  z = %d\r\n", xData, yData, zData);

		/* Check SW3 pin state */
		if(GPIO_DRV_ReadPinInput(switchPins[1].pinName) == 0)
		{
		   debug_printf("\nHold SW3 pressed and remove SD Card\r\n");
			while(GPIO_DRV_ReadPinInput(switchPins[1].pinName) == 0);
		}

		/* If SD card not present avoid logging */
		if(sdhc_detect())
		{
			log_accel_values(xData, yData, zData);
		}

		/* Wait 1 second to read and log new data */
		OSA_TimeDelay(1000);
	}
}

static void FnetTask()
{
   // Delay the start of FNET
   OSA_TimeDelay(5000);

   /* Init UART. */
    //fnet_cpu_serial_init(FNET_CFG_CPU_SERIAL_PORT_DEFAULT, 115200);
   fnet_cpu_serial_init(4, 115200);

    /* Enable Interrupts.*/
   fnet_cpu_irq_enable(0);

   /* Run FNET application. - Function does not return */
   fapp_main();

   while(1){OSA_TimeDelay(1000);}
}

static void MainTask(void *arg)
{
   OSA_TimeDelay(500);

   debug_printf("\n****** uServer ******\r\n");

   LED1_EN; LED2_EN; LED3_EN;

   int counter = 0;
   while(1)
   {
      counter++;

      LED1_TOGGLE; OSA_TimeDelay(50);
      LED3_TOGGLE; OSA_TimeDelay(50);
      LED2_TOGGLE; OSA_TimeDelay(50);

      LED1_TOGGLE; OSA_TimeDelay(50);
      LED3_TOGGLE; OSA_TimeDelay(50);
      LED2_TOGGLE; OSA_TimeDelay(50);

      OSA_TimeDelay(700);
   }
}

int main (void)
{
   OSA_Init();

   /* Initialize clocks, debug console interface and configure required pins */
   hardware_init();

   OSA_TaskCreate((task_t)SdCardTask, (uint8_t*)"SD Card Task", 1024, NULL, 1, NULL, true, NULL);
   OSA_TaskCreate((task_t)MainTask,   (uint8_t*)"Main Task",    1024, NULL, 2, NULL, true, NULL);
   //OSA_TaskCreate((task_t)FnetTask,   (uint8_t*)"FNET Task",    2048, NULL, 3, NULL, true, NULL);

   OSA_Start();

   while(1);

   return(0);
}

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

	/* Open a text file */
	fr = f_open(&fil, "log_data.txt", FA_WRITE | FA_OPEN_ALWAYS);
	if(fr)
	{
		printf("\nError opening text file\r\n");
		for(;;){}
	}

	new_line = f_size(&fil);

	/* Set file pointer to the start of new line in text file */
	fr = f_lseek(&fil, new_line);

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

	/* Write the constructed string to the new line in text file */
	fr = f_write(&fil, aux_string, strlen(aux_string), &bytes_written);

	if(fr)
	{
		printf("\nError logging data\r\n");
		for(;;){}
	}

	/* Close the text file */
	fr = f_close(&fil);
}

/*FUNCTION*----------------------------------------------------------------
* Function Name  : sdhc_detect
* Comments       : Detect if the SD Card is present or not with a GPIO pin
* Returns:	[0] - Card not available
* 		[1] - Card is present
*END*--------------------------------------------------------------------*/

bool sdhc_detect(void)
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

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
