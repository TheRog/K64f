/*
 * DRV_MPU9250.h
 *
 *  Created on: Oct 8, 2015
 *      Author: Schuck
 */

#ifndef SOURCES_DRV_MPU9250_H_
#define SOURCES_DRV_MPU9250_H_

#include <stdint.h>

class DRV_MPU9250
{
public:
   DRV_MPU9250(){}

   int32_t Init();

   void Update();

private:

};

#endif /* SOURCES_DRV_MPU9250_H_ */
