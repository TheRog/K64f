/*
 * DRV_MPU9250.cpp
 *
 *  Created on: Oct 8, 2015
 *      Author: Schuck
 */

#include "DRV_MPU9250.h"

#include "board.h"
#include "fsl_clock_manager.h"
#include "fsl_os_abstraction.h"
#include "fsl_dspi_master_driver.h"
#include "fsl_debug_console.h"

#include "fsl_dspi_shared_function.h"
#include "fsl_device_registers.h"

#define DSPI_MASTER_INSTANCE        (0)                 /*! User change define to choose DSPI instance */
#define TRANSFER_SIZE               (32)                /*! Transfer size */
#define TRANSFER_BAUDRATE           (500000U)           /*! Transfer baudrate - 500k */
#define MASTER_TRANSFER_TIMEOUT     (5000U)             /*! Transfer timeout of master - 5s */

uint8_t receiveBuffer[TRANSFER_SIZE] = {0};
uint8_t sendBuffer[TRANSFER_SIZE] = {0};

int32_t loopCount = 0;

void SPI0_IRQHandler(void)
//void ROGER_SPI_CALLBACK(void)
{
   DSPI_DRV_MasterIRQHandler(SPI0_IDX);
   PRINTF("SPI: ISR Detected!!! \n\r");
}

int32_t DRV_MPU9250::Init()
{
   uint32_t calculatedBaudRate;

  // OSA_InstallIntHandler(42, SPI0_IRQHandler); //or 26
   OSA_InstallIntHandler(SPI0_IRQn, SPI0_IRQHandler);
   OSA_InstallIntHandler(SPI0_IRQn, SPI0_IRQHandler);

   for(int x=0; x<85; x++)
   {
      OSA_InstallIntHandler(x, SPI0_IRQHandler);
   }

   // Initialize the SPI peripheral
   dspi_status_t dspiResult;
   dspi_master_state_t masterState;
   dspi_device_t masterDevice;
   dspi_master_user_config_t masterUserConfig;
   masterUserConfig.isChipSelectContinuous     = false;
   masterUserConfig.isSckContinuous            = false;
   masterUserConfig.pcsPolarity                = kDspiPcs_ActiveLow;
   masterUserConfig.whichCtar                  = kDspiCtar0;
   masterUserConfig.whichPcs                   = kDspiPcs0;

   // Setup the configuration.
   masterDevice.dataBusConfig.bitsPerFrame = 8;
   masterDevice.dataBusConfig.clkPhase     = kDspiClockPhase_FirstEdge;
   masterDevice.dataBusConfig.clkPolarity  = kDspiClockPolarity_ActiveHigh;
   masterDevice.dataBusConfig.direction    = kDspiMsbFirst;

   PRINTF("SPI: Initializing SPI driver... \n\r");

   // Initialize master driver.
   dspiResult = DSPI_DRV_MasterInit(
                           DSPI_MASTER_INSTANCE,
                           &masterState,
                           &masterUserConfig);
   if (dspiResult != kStatus_DSPI_Success)
   {
     PRINTF("SPI: Can not initialize master driver \n\r");
     return -1;
   }

   // Configure baudrate.
   masterDevice.bitsPerSec = TRANSFER_BAUDRATE;
   dspiResult = DSPI_DRV_MasterConfigureBus(
                           DSPI_MASTER_INSTANCE,
                           &masterDevice,
                           &calculatedBaudRate);
   if (dspiResult != kStatus_DSPI_Success)
   {
     PRINTF("SPI: failure in configuration bus\n\r");
     return -1;
   }
   else
   {
     PRINTF("SPI: Transfer at baudrate %lu \r\n", calculatedBaudRate);
   }

}

void DRV_MPU9250::Update()
{
   dspi_status_t dspiResult;

   // Initialize the transmit buffer.
   for (int i = 0; i < TRANSFER_SIZE; i++)
   {
      sendBuffer[i] = i + loopCount;
   }

   // Print out transmit buffer.
   PRINTF("\r\n Master transmit:\n");
   for (int i = 0; i < TRANSFER_SIZE; i++)
   {
      // Print 16 numbers in a line.
      if ((i & 0x0F) == 0)
      {
          PRINTF("\r\n    ");
      }
      PRINTF(" %02X", sendBuffer[i]);
   }

   // Reset the receive buffer.
   for (int i = 0; i < TRANSFER_SIZE; i++)
   {
      receiveBuffer[i] = 0;
   }

   // Send the data.
   dspiResult = DSPI_DRV_MasterTransferBlocking(DSPI_MASTER_INSTANCE,
                                               NULL,
                                               sendBuffer,
                                               NULL,
                                               TRANSFER_SIZE,
                                               MASTER_TRANSFER_TIMEOUT);
   if (dspiResult != kStatus_DSPI_Success)
   {
      PRINTF("\r\nERROR: send data error \n\r");
      return;
   }
   // Wait until slave is ready to send.
//   OSA_TimeDelay(100);

//   // Receive the data.
//   dspiResult = DSPI_DRV_MasterTransferBlocking(DSPI_MASTER_INSTANCE,
//                                               NULL,
//                                               NULL,
//                                               receiveBuffer,
//                                               TRANSFER_SIZE,
//                                               MASTER_TRANSFER_TIMEOUT);
//   if (dspiResult != kStatus_DSPI_Success)
//   {
//      PRINTF("\r\nERROR: receive data error \n\r");
//      return;
//   }
//
//   // Print out receive buffer.
//   PRINTF("\r\n Master receive:");
//   for (int i = 0; i < TRANSFER_SIZE; i++)
//   {
//      // Print 16 numbers in a line.
//      if ((i & 0x0F) == 0)
//      {
//          PRINTF("\r\n    ");
//      }
//      PRINTF(" %02X", receiveBuffer[i]);
//   }
//
//   // Check receiveBuffer.
//   for (int i = 0; i < TRANSFER_SIZE; ++i)
//   {
//      if (receiveBuffer[i] != sendBuffer[i])
//      {
//          // Master received incorrect.
//          PRINTF("\r\n ERROR: master received incorrect \n\r");
//          return;
//      }
//   }
//
//   PRINTF("\r\n DSPI Master Sends/ Recevies Successfully");

   loopCount++;
}
