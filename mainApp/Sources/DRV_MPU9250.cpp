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

SPI_Type * dspiBaseAddr = (SPI_Type*)SPI0_BASE;
dspi_command_config_t commandConfig;

//extern "C" {
//void SPI0_IRQHandler(void)
//{
//   DSPI_DRV_MasterIRQHandler(SPI0_IDX);
//   PRINTF("SPI: ISR Detected!!! \n\r");
//}
//void SPI1_IRQHandler(void)
//{
//   DSPI_DRV_MasterIRQHandler(SPI0_IDX);
//   PRINTF("SPI: ISR Detected!!! \n\r");
//}
//void SPI2_IRQHandler(void)
//{
//   DSPI_DRV_MasterIRQHandler(SPI0_IDX);
//   PRINTF("SPI: ISR Detected!!! \n\r");
//}
//}

int32_t DRV_MPU9250::Init()
{
//   uint32_t calculatedBaudRate;

  // OSA_InstallIntHandler(42, SPI0_IRQHandler); //or 26
//   OSA_InstallIntHandler(SPI0_IRQn, SPI0_IRQHandler);
//   OSA_InstallIntHandler(SPI0_IRQn, SPI0_IRQHandler);
//
//   for(int x=0; x<85; x++)
//   {
//      OSA_InstallIntHandler(x, SPI0_IRQHandler);
//   }

   uint32_t i;
   uint32_t loopCount = 1;
   SPI_Type * dspiBaseAddr = (SPI_Type*)SPI0_BASE;
   uint32_t dspiSourceClock;
   uint32_t calculatedBaudRate;
   dspi_device_t masterDevice;

   commandConfig.isChipSelectContinuous = false,
   commandConfig.whichCtar              = kDspiCtar0;
   commandConfig.whichPcs               = kDspiPcs0;
   commandConfig.clearTransferCount     = true;
   commandConfig.isEndOfQueue           = false;

   // Print a note.
   PRINTF("\r\n DSPI board to board polling example");
   PRINTF("\r\n This example run on instance 0 ");
   PRINTF("\r\n Be sure DSPI0-DSPI0 are connected \n");

   // Enable DSPI clock.
   CLOCK_SYS_EnableSpiClock(DSPI_MASTER_INSTANCE);
   // Initialize the DSPI module registers to default value, which disables the module
   DSPI_HAL_Init(dspiBaseAddr);
   // Set to master mode.
   DSPI_HAL_SetMasterSlaveMode(dspiBaseAddr, kDspiMaster);
   // Configure for continuous SCK operation
   DSPI_HAL_SetContinuousSckCmd(dspiBaseAddr, false);
   // Configure for peripheral chip select polarity
   DSPI_HAL_SetPcsPolarityMode(dspiBaseAddr, kDspiPcs0, kDspiPcs_ActiveLow);
   // Disable FIFO operation.
   DSPI_HAL_SetFifoCmd(dspiBaseAddr, false, false);
   // Initialize the configurable delays: PCS-to-SCK, prescaler = 0, scaler = 1
   DSPI_HAL_SetDelay(dspiBaseAddr, kDspiCtar0, 0, 1, kDspiPcsToSck);
   // DSPI system enable
   DSPI_HAL_Enable(dspiBaseAddr);

   // Configure baudrate.
   masterDevice.dataBusConfig.bitsPerFrame = 8;
   masterDevice.dataBusConfig.clkPhase     = kDspiClockPhase_FirstEdge;
   masterDevice.dataBusConfig.clkPolarity  = kDspiClockPolarity_ActiveHigh;
   masterDevice.dataBusConfig.direction    = kDspiMsbFirst;

   DSPI_HAL_SetDataFormat(dspiBaseAddr, kDspiCtar0, &masterDevice.dataBusConfig);
   // Get DSPI source clock.
   dspiSourceClock = CLOCK_SYS_GetSpiFreq(DSPI_MASTER_INSTANCE);
   calculatedBaudRate = DSPI_HAL_SetBaudRate(dspiBaseAddr, kDspiCtar0, TRANSFER_BAUDRATE, dspiSourceClock);
   PRINTF("\r\n Transfer at baudrate %lu \r\n", calculatedBaudRate);

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
//   dspiResult = DSPI_DRV_MasterTransferBlocking(DSPI_MASTER_INSTANCE,
//                                               NULL,
//                                               sendBuffer,
//                                               NULL,
//                                               TRANSFER_SIZE,
//                                               MASTER_TRANSFER_TIMEOUT);

//   dspiResult = DSPI_DRV_MasterTransfer(DSPI_MASTER_INSTANCE,
//                                                  NULL,
//                                                  sendBuffer,
//                                                  NULL,
//                                                  TRANSFER_SIZE);

   // Restart the transfer by stop then start again, this will clear out the shift register
   DSPI_HAL_StopTransfer(dspiBaseAddr);
   // Flush the FIFOs
   DSPI_HAL_SetFlushFifoCmd(dspiBaseAddr, true, true);
   // Clear status flags that may have been set from previous transfers.
   DSPI_HAL_ClearStatusFlag(dspiBaseAddr, kDspiTxComplete);
   DSPI_HAL_ClearStatusFlag(dspiBaseAddr, kDspiEndOfQueue);
   DSPI_HAL_ClearStatusFlag(dspiBaseAddr, kDspiTxFifoUnderflow);
   DSPI_HAL_ClearStatusFlag(dspiBaseAddr, kDspiTxFifoFillRequest);
   DSPI_HAL_ClearStatusFlag(dspiBaseAddr, kDspiRxFifoOverflow);
   DSPI_HAL_ClearStatusFlag(dspiBaseAddr, kDspiRxFifoDrainRequest);
   // Clear the transfer count.
   DSPI_HAL_PresetTransferCount(dspiBaseAddr, 0);
   // Start the transfer process in the hardware
   DSPI_HAL_StartTransfer(dspiBaseAddr);
   // Send the data to slave.
   for (int i = 0; i < TRANSFER_SIZE; i++)
   {
      // Write data to PUSHR
      DSPI_HAL_WriteDataMastermodeBlocking(dspiBaseAddr, &commandConfig, sendBuffer[i]);
      // Delay to wait slave is ready.
      OSA_TimeDelay(1);
   }

//   if (dspiResult != kStatus_DSPI_Success)
//   {
//      PRINTF("\r\nERROR: send data error \n\r");
//      return;
//   }
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
