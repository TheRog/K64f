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

#include <stddef.h>
#include <stdio.h>

#include "fsl_device_registers.h"
#include "fsl_os_abstraction.h"
#include "board.h"

//#include "fapp.h"
//#include "FnetIpStack.h"
//FnetIpStack fnetIpStack;

//class TestClass
//{
//public:
//   void Init();
//};

static void MainTask(void *arg)
{
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

      OSA_TimeDelay(300);
   }
}

static void FnetTask()
{
//   /* Init UART. */
//    fnet_cpu_serial_init(FNET_CFG_CPU_SERIAL_PORT_DEFAULT, 115200);
//
//    /* Enable Interrupts.*/
//    fnet_cpu_irq_enable(0);
//
//   /* Run FNET application. - Function does not return */
//   fapp_main();

   while(1){OSA_TimeDelay(1000);}
}

/********************************************************************/
int main (void)
{
    OSA_Init();

    OSA_TaskCreate((task_t)MainTask, (uint8_t*)"Main Task", 1024, NULL, 1, NULL, true, NULL);
    //OSA_TaskCreate((task_t)FnetTask, (uint8_t*)"FNET Task", 4096, NULL, 2, NULL, true, NULL);

    OSA_Start();

    while(1);

    return(0);
}


