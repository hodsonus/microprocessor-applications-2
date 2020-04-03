/*
 * BSP.h
 *
 *  Created on: Dec 30, 2016
 *      Author: Raz Aloni
 */

#ifndef BSP_H_
#define BSP_H_

/* Includes */
#include <stdint.h>
#include "bme280_support.h"
#include "bmi160_support.h"
#include "opt3001.h"
#include "tmp007.h"
#include "BackChannelUart.h"
#include "ClockSys.h"
#include "Joystick.h"
#include "RGBLeds.h"
#include "LCDLib.h"
#include "cc3100_usage.h"

/********************************** Public Functions **************************************/

/* Initializes the entire board */
extern void BSP_InitBoard(bool LCD_usingTP, playerType wifi_hostOrClient);

/********************************** Public Functions **************************************/

#endif /* BSP_H_ */
