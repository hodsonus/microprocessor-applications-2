/*
 * Threads.c
 *
 *  Created on: Jan 31, 2020
 *      Author: johnhodson
 */

#include <stdint.h>
#include <G8RTOS/G8RTOS.h>
#include <BSP.h>
#include "Threads.h"

void task0(void)
{

    int16_t accelerometerX;
    uint16_t ledData;

    while(1)
    {
        // Waits for the sensor I2C semaphore.
        G8RTOS_WaitSemaphore(&sensor);

        // Reads accelerometer’s x-axis and saves it.
        while(bmi160_read_accel_x(&accelerometerX));

        // Releases the sensor I2C semaphore.
        G8RTOS_SignalSemaphore(&sensor);

        // Construct data for LEDs.
        if      (accelerometerX >  14000) ledData = 0xFF00;
        else if (accelerometerX >  12000) ledData = 0x7F00;
        else if (accelerometerX >  10000) ledData = 0x3F00;
        else if (accelerometerX >   8000) ledData = 0x1F00;
        else if (accelerometerX >   6000) ledData = 0x0F00;
        else if (accelerometerX >   4000) ledData = 0x0700;
        else if (accelerometerX >   2000) ledData = 0x0300;
        else if (accelerometerX >      0) ledData = 0x0100;
        else if (accelerometerX >  -2000) ledData = 0x0010;
        else if (accelerometerX >  -4000) ledData = 0x0030;
        else if (accelerometerX >  -6000) ledData = 0x0070;
        else if (accelerometerX >  -8000) ledData = 0x00F0;
        else if (accelerometerX > -10000) ledData = 0x00F1;
        else if (accelerometerX > -12000) ledData = 0x00F3;
        else if (accelerometerX > -14000) ledData = 0x00F7;
        else                              ledData = 0x00FF;

        // Waits for the LED I2C semaphore.
        G8RTOS_WaitSemaphore(&led);

        // Output data to red LEDs.
        LP3943_LedModeSet(RED, ledData);

        // Releases the LED I2C semaphore.
        G8RTOS_SignalSemaphore(&led);
    }
}

void task1(void)
{

    uint16_t lightData;
    uint16_t ledData;

    while(1)
    {
        // Waits for the sensor I2C semaphore.
        G8RTOS_WaitSemaphore(&sensor);

        // Reads light sensor and saves it.
        while(!sensorOpt3001Read(&lightData));

        // Releases the sensor I2C semaphore.
        G8RTOS_SignalSemaphore(&sensor);

        // Construct data for LEDs.
        if      (lightData > 49000) ledData = 0xFFFF;
        else if (lightData > 45500) ledData = 0xFFFE;
        else if (lightData > 42000) ledData = 0xFFFC;
        else if (lightData > 38500) ledData = 0xFFF8;
        else if (lightData > 35500) ledData = 0xFFF0;
        else if (lightData > 31500) ledData = 0xFFE0;
        else if (lightData > 28000) ledData = 0xFFC0;
        else if (lightData > 26000) ledData = 0xFF80;
        else if (lightData > 24500) ledData = 0xFF00;
        else if (lightData > 21000) ledData = 0xFE00;
        else if (lightData > 17500) ledData = 0xFC00;
        else if (lightData > 14000) ledData = 0xF800;
        else if (lightData > 10500) ledData = 0xF000;
        else if (lightData >  7000) ledData = 0xE000;
        else if (lightData >  3500) ledData = 0xC000;
        else                        ledData = 0x8000;

        // Waits for the LED I2C semaphore.
        G8RTOS_WaitSemaphore(&led);

        // Output data to green LEDs.
        LP3943_LedModeSet(GREEN, ledData);

        // Releases the LED I2C semaphore.
        G8RTOS_SignalSemaphore(&led);
    }
}

void task2(void)
{

    int16_t gyroZ;
    uint16_t ledData;

    while(1)
    {
        // Waits for the sensor I2C semaphore.
        G8RTOS_WaitSemaphore(&sensor);

        // Reads z-axis of the gyro and saves it.
        while(bmi160_read_gyro_z(&gyroZ));

        // Releases the sensor I2C semaphore.
        G8RTOS_SignalSemaphore(&sensor);

        // Construct data for LEDs.
        if      (gyroZ >  7000) ledData = 0xFF00;
        else if (gyroZ >  6000) ledData = 0x7F00;
        else if (gyroZ >  5000) ledData = 0x3F00;
        else if (gyroZ >  4000) ledData = 0x1F00;
        else if (gyroZ >  3000) ledData = 0x0F00;
        else if (gyroZ >  2000) ledData = 0x0700;
        else if (gyroZ >  1000) ledData = 0x0300;
        else if (gyroZ >     0) ledData = 0x0100;
        else if (gyroZ > -1000) ledData = 0x0010;
        else if (gyroZ > -2000) ledData = 0x0030;
        else if (gyroZ > -3000) ledData = 0x0070;
        else if (gyroZ > -4000) ledData = 0x00F0;
        else if (gyroZ > -5000) ledData = 0x00F1;
        else if (gyroZ > -6000) ledData = 0x00F3;
        else if (gyroZ > -7000) ledData = 0x00F7;
        else                    ledData = 0x00FF;

        // Waits for the LED I2C semaphore.
        G8RTOS_WaitSemaphore(&led);

        // Output data to blue LEDs.
        LP3943_LedModeSet(BLUE, ledData);

        // Releases the LED I2C semaphore.
        G8RTOS_SignalSemaphore(&led);
    }
}
