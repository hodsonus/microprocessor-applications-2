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

#define NUM_READINGS 10

int32_t joystick_decayed_avg;
int32_t temperature_farenheit;
bool light_flag;

void thread0(void)
{
    int32_t temperature;
    while (1)
    {
        G8RTOS_WaitSemaphore(&sensor_mutex);
        // Read the BME280’s temperature sensor to get the uncompensated value
        bme280_read_uncomp_temperature(&temperature);
        // Use the provided API to go from uncompensated temperature -> 0.01 degree celsius
        temperature = bme280_compensate_temperature_int32(temperature);
        G8RTOS_SignalSemaphore(&sensor_mutex);

        // convert from 0.01 degree celsius -> degree celsius
        temperature /= 100;
        // convert from celsius -> farenheit
        temperature = (temperature * 9)/5 + 32;

        // Send data to temperature FIFO
        G8RTOS_WriteFIFO(TEMP_FIFO, temperature);

        // Toggle an available GPIO pin (P4.5)
        BITBAND_PERI(P4->OUT, 5) = ~BITBAND_PERI(P4->OUT, 5);

        // Sleep for 500 ms
        G8RTOS_Sleep(500);
    }
}

void thread1(void)
{
    uint16_t light_data;
    while (1)
    {
        // Read light sensor
        G8RTOS_WaitSemaphore(&sensor_mutex);
        sensorOpt3001Read(&light_data);
        G8RTOS_SignalSemaphore(&sensor_mutex);

        // Send data to light FIFO
        G8RTOS_WriteFIFO(LIGHT_FIFO, light_data);

        // Toggle an available GPIO pin (P4.7)
        BITBAND_PERI(P4->OUT, 7) = ~BITBAND_PERI(P4->OUT, 7);

        // Sleep for 200 ms
        G8RTOS_Sleep(200);
    }
}

static int sum(int arr[], int n)
{
    int sum = 0;
    for (int i = 0; i < n; ++i) sum += arr[i];
    return sum;
}

static int square_root(int n)
{
    int xk, xkp1 = n;

    do
    {
        xk = xkp1;
        xkp1 = (xk + (n / xk)) / 2;
    } while (abs(xkp1 - xk) >= 1);

    return xkp1;
}

void thread2(void)
{
    /* The idea here is to keep track of the N most recent readings from
     * the light sensor. Since the N+1st reading has to fall off each time
     * that we read, we must keep track of every value in a queue. */

    int32_t new_light_data;
    int i = 0;
    int light_readings[NUM_READINGS];
    int RMS;

    while (1)
    {
        // read light FIFO
        new_light_data = G8RTOS_ReadFIFO(LIGHT_FIFO);

        // calculate xi^2/NUM_READINGS and place in our queue
        light_readings[i] = (new_light_data * new_light_data) / NUM_READINGS;

        // update insertion index, wrapping around if necessary
        i = (i+1) % NUM_READINGS;

        // calculate the sum and take the square root of it to find the RMS
        RMS = square_root( sum(light_readings, NUM_READINGS) );

        // set or clear the light_flag according to Lab Manual spec
        light_flag = RMS < 5000;
    }
}

void thread3(void)
{
    while (1)
    {
        // Read temperature FIFO
        temperature_farenheit = G8RTOS_ReadFIFO(TEMP_FIFO);

        // Construct data for LEDs (as shown in Figure B of the Lab Manual)
        uint16_t red_bitmask, blue_bitmask;
        if (temperature_farenheit > 84)
        {
            red_bitmask = 0x00FF;
            blue_bitmask = 0x0000;
        }
        else if (temperature_farenheit > 81)
        {
            red_bitmask = 0x007F;
            blue_bitmask = 0x0080;
        }
        else if (temperature_farenheit > 78){
            red_bitmask = 0x003F;
            blue_bitmask = 0x00C0;
        }
        else if (temperature_farenheit > 75)
        {
            red_bitmask = 0x001F;
            blue_bitmask = 0x00E0;
        }
        else if (temperature_farenheit > 72)
        {
            red_bitmask = 0x000F;
            blue_bitmask = 0x00F0;
        }
        else if (temperature_farenheit > 69)
        {
            red_bitmask = 0x0007;
            blue_bitmask = 0x00F8;
        }
        else if (temperature_farenheit > 66)
        {
            red_bitmask = 0x0003;
            blue_bitmask = 0x00FC;
        }
        else if (temperature_farenheit > 63)
        {
            red_bitmask = 0x0001;
            blue_bitmask = 0x00FE;
        }
        else // temperature_farenheit < 63
        {
            red_bitmask = 0x0000;
            blue_bitmask = 0x00FF;
        }

        // Send data to LEDs
        G8RTOS_WaitSemaphore(&led_mutex);
        LP3943_LedModeSet(BLUE, blue_bitmask);
        LP3943_LedModeSet(RED, red_bitmask);
        G8RTOS_SignalSemaphore(&led_mutex);
    }
}

void thread4(void)
{
    int32_t joystick_data;
    while (1)
    {
        // Read Joystick FIFO
        joystick_data = G8RTOS_ReadFIFO(JOYSTICK_FIFO);

        // Calculate decayed average for X-Coordinate
        joystick_decayed_avg = (joystick_decayed_avg + joystick_data) >> 1;

        // Construct data for LEDs (as shown in Figure A of the Lab Manual)
        uint16_t green_bitmask;
        if (joystick_decayed_avg > 6000)
        {
            green_bitmask = 0xF000;
        }
        else if (joystick_decayed_avg > 4000)
        {
            green_bitmask = 0x7000;
        }
        else if (joystick_decayed_avg > 2000)
        {
            green_bitmask = 0x3000;
        }
        else if (joystick_decayed_avg > 500)
        {
            green_bitmask = 0x1000;
        }
        else if (joystick_decayed_avg > -500)
        {
            green_bitmask = 0x0000;
        }
        else if (joystick_decayed_avg > -2000)
        {
            green_bitmask = 0x0800;
        }
        else if (joystick_decayed_avg > -4000)
        {
            green_bitmask = 0x0C00;
        }
        else if (joystick_decayed_avg > -6000)
        {
            green_bitmask = 0x0E00;
        }
        else // joystick_decayed_avg > -8000
        {
            green_bitmask = 0x0F00;
        }

        // Send data to LEDs
        G8RTOS_WaitSemaphore(&led_mutex);
        LP3943_LedModeSet(GREEN, green_bitmask);
        G8RTOS_SignalSemaphore(&led_mutex);
    }
}

void thread5(void)
{
    while(1);
}

void pthread0(void)
{
    int16_t x_coord, y_coord;

    // Read data from joystick
    GetJoystickCoordinates(&x_coord, &y_coord);

    // Write X-coordinate to Joystick FIFO
    G8RTOS_WriteFIFO(JOYSTICK_FIFO, x_coord);

    // Toggle an available GPIO pin (P5.4)
    BITBAND_PERI(P5->OUT, 4) = ~BITBAND_PERI(P5->OUT, 4);
}

void pthread1(void)
{
    if (light_flag)
    {
        // Print out the temperature (in degrees Fahrenheit) via UART
        BackChannelPrintIntVariable("temperature_farenheit", temperature_farenheit);

        // Print out decayed average value of the Joystick’s X-coordinate via UART
        BackChannelPrintIntVariable("joystick_decayed_avg", joystick_decayed_avg);
    }
}
