/*
 * Threads.c
 *
 *  Created on: Feb 29, 2020
 *      Author: johnhodson
 */

#include <stdint.h>
#include <G8RTOS/G8RTOS.h>
#include <BSP.h>
#include "Threads.h"

/* TODO */
void thread0(void)
{
    // Read the BME280’s temperature sensor

    // Send data to temperature FIFO

    // Toggle an available GPIO pin

    // Sleep for 500 ms
    G8RTOS_Sleep(500);
}

/* TODO */
void thread1(void)
{
    // Read light sensor

    // Send data to light FIFO

    // Toggle an available GPIO pin

    // Sleep for 200 ms
    G8RTOS_Sleep(200);
}

/* TODO */
void thread2(void)
{
    // Read light FIFO

    // Calculate RMS value

    // If RMS < 5000

        // set global variable to true

    // else

        // set it false
}

/* TODO */
void thread3(void)
{
    // Read temperature FIFO

    // Output data to LEDs (as shown in Figure B of the Lab Manual)
}

/* TODO */
void thread4(void)
{
    // Read Joystick FIFO

    // Calculate decayed average for X-Coordinate

    // Output data to LEDs (as shown in Figure A of the Lab Manual)
}

void thread5(void)
{
    while(1);
}

/* TODO */
void pthread0(void)
{
    // Read X-coordinate from joystick

    // Write data to Joystick FIFO

    // Toggle an available GPIO pin
}

/* TODO */
void pthread1(void)
{
    // If global variable for light sensor is true

        // Print out the temperature (in degrees Fahrenheit) via UART

        // Print out decayed average value of the Joystick’s X-coordinate via UART
}

/* TODO */
static void RMS(void)
{
}
