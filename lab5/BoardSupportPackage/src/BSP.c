/*
 * BSP.c
 *
 *  Created on: Dec 30, 2016
 *      Author: Raz Aloni
 */

#include <driverlib.h>
#include "BSP.h"
#include "i2c_driver.h"
#include "demo_sysctl.h"


/* Initializes the entire board */
void BSP_InitBoard(bool LCD_usingTP, playerType wifi_hostOrClient)
{
	/* Disable Watchdog */
	WDT_A_clearTimer();
	WDT_A_holdTimer();

	/* Initialize Clock */
	ClockSys_SetMaxFreq();

	/* Init i2c */
	initI2C();
	DelayMs(50);

	/* Init Opt3001 */
	sensorOpt3001Enable(true);
	DelayMs(50);

	/* Init Tmp007 */
	sensorTmp007Enable(true);
	DelayMs(50);

	/* Init Bmi160 */
    bmi160_initialize_sensor();
    DelayMs(50);

    /* Init joystick without interrupts */
	Joystick_Init_Without_Interrupt();
	DelayMs(50);

	/* Init Bme280 */
	bme280_initialize_sensor();
	DelayMs(50);

	/* Init BackChannel UART */
	BackChannelInit();
	DelayMs(50);

	/* Init RGB LEDs */
	init_RGBLEDS();
	DelayMs(50);

	/* Init LCD */
	LCD_Init(LCD_usingTP);
	DelayMs(50);

    /* Init CC3100 */
	initCC3100(wifi_hostOrClient);
    DelayMs(50);
}
