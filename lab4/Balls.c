/*
 * Balls.c
 *
 *  Created on: Mar 3, 2020
 *      Author: johnhodson
 */

#include <stdlib.h>
#include "Balls.h"
#include "BSP.h"
#include "G8RTOS/G8RTOS.h"

#define LO -25
#define HI 25

static ball balls[MAX_BALLS];
bool LCDTapped = false;
uint8_t NumberOfBalls = 0;
int16_t x_accel;
int16_t y_accel;

/* Aperiodic event - sets a flag when the screen is tapped. */
void LCDTapHandler(void)
{
    // If the interrupt flag for BIT0 is set
    if (P4->IFG & BIT0)
    {
        // Set the LCD flag
        LCDTapped = true;

        // And clear the flag
        P4->IFG &= ~BIT0;
    }
}

/* Background thread - deals with the event when the LCD is tapped. */
void LCDTappedWorker(void)
{
    while(1)
    {
        // If the LCD flag is true
        if (LCDTapped)
        {
            // Retrieve the location at which the screen was tapped
            G8RTOS_WaitSemaphore(&lcd_mutex);
            Point tap = TP_ReadXY();
            G8RTOS_SignalSemaphore(&lcd_mutex);

            // Determine if we are destroying a ball
            int ballToKill = -1;
            for (int i = 0; i < MAX_BALLS; ++i)
            {
                /* If the ith ball is dead, then we cannot possibly kill it,
                 * so continue */
                if (!balls[i].alive) continue;

                /* Calculate the delta between the tap and the ball's
                 * position */
                uint32_t delta_x = balls[i].x_position - tap.x;
                uint32_t delta_y = balls[i].y_position - tap.y;

                /* Compare the distance between the points squared to the
                 * threshold defined in the header. */
                uint32_t distance_squared = delta_x*delta_x + delta_y*delta_y;
                if (distance_squared < TAP_DISTANCE_THRESHOLD*TAP_DISTANCE_THRESHOLD)
                {
                    /* If the distance is within our threshold, assign
                     * ballToKill and break. */
                    ballToKill = i;
                    break;
                }
            }

            // If a ball to kill was NOT found in the above loop
            if (ballToKill == -1)
            {
                // Then create a ball at the location of the tap

                // Write the location of the tap to the shared FIFO
                G8RTOS_WriteFIFO(COORD_FIFO, tap.x << 16 | tap.y);

                // Create a new ball thread that will read from this FIFO
                G8RTOS_AddThread(&Ball, BALL_THREAD_PRIORITY, "ball thread");
            }
            else
            {
                // Else if we did find a ball to kill

                // Kill its respective thread
                G8RTOS_WaitSemaphore(&lcd_mutex);
                G8RTOS_Scheduler_Error error = G8RTOS_KillThread(balls[ballToKill].thread_id);
                G8RTOS_SignalSemaphore(&lcd_mutex);

                /* And remove the ball from our records if the thread removal
                 * was successful. */
                if (error == SCHEDULER_NO_ERROR)
                {
                    // Erase the old ball from the screen
                    draw_ball(BACKGROUND_COLOR,
                              balls[ballToKill].x_position,
                              balls[ballToKill].y_position);
                    // Mark it as dead
                    balls[ballToKill].alive = false;
                    // Decrement the number of active balls
                    --NumberOfBalls;
                }
            }

            /* Sleep to account for screen bouncing and then reset the flag so
             * we can later reenter. */
            G8RTOS_Sleep(500);
            LCDTapped = false;
        }
    }
}

/* Background thread - a thread that retrieves the accelerometer's current orientation. */
void ReadAccelerometer(void)
{
    /* The initial values read by the accelerometer will be used to zero future
     * readings */
    int16_t x_accel_zero, y_accel_zero, x_accel_temp, y_accel_temp;

    bmi160_read_accel_x(&x_accel_zero);
    bmi160_read_accel_y(&y_accel_zero);

    while(1)
    {
        // Read the most up to date value from the accelerometer
        bmi160_read_accel_x(&x_accel_temp);
        bmi160_read_accel_y(&y_accel_temp);

        /* Scale the acceleration back to a number we can work with and flip
         * it if necessary */
        x_accel_temp = (x_accel_temp - x_accel_zero)/ACCELERATION_SCALER;
        y_accel_temp = -(y_accel_temp - y_accel_zero)/ACCELERATION_SCALER;

        // Cap out the acceleration at +/- MAX_ACCELERATION for x and y
        if (x_accel_temp >= MAX_ACCELERATION) x_accel_temp = MAX_ACCELERATION;
        else if (x_accel_temp <= -MAX_ACCELERATION) x_accel_temp = -MAX_ACCELERATION;
        if (y_accel_temp >= MAX_ACCELERATION) y_accel_temp = MAX_ACCELERATION;
        else if (y_accel_temp <= -MAX_ACCELERATION) y_accel_temp = -MAX_ACCELERATION;

        // Set the global variables
        x_accel = x_accel_temp;
        y_accel = y_accel_temp;

        // Sleep for 30ms
        G8RTOS_Sleep(30);
    }
}

/* Background thread - dynamically killed and created by the LCDTappedWorker
 * and represents a ball. */
void Ball(void)
{
    // Read FIFO to remove the most recently inserted point
    int32_t point = G8RTOS_ReadFIFO(COORD_FIFO);

    /* Finds a dead ball to animate by iterating over the list of balls and
     * picking the first one that is dead */
    int ball_to_animate = -1;
    for (int i = 0; i < MAX_BALLS; ++i)
    {
        if (!balls[i].alive)
        {
            ball_to_animate = i;
            break;
        }
    }

    // If we were unable to find a dead ball, then kill ourself
    if (ball_to_animate == -1)
    {
        G8RTOS_KillSelf();
    }

    // Initialize coordinate accordingly from the point retrieved from the FIFO
    balls[ball_to_animate].x_position = point >> 16;
    balls[ball_to_animate].y_position = point & 0xFFFF;

    // Initialize x and y velocity randomly (generate random number from LO to HI)
    balls[ball_to_animate].x_velocity = LO + rand() / (RAND_MAX / (HI - LO + 1) + 1);
    balls[ball_to_animate].y_velocity = LO + rand() / (RAND_MAX / (HI - LO + 1) + 1);

    // Initialize the ball as alive with the current thread id
    balls[ball_to_animate].alive = true;
    balls[ball_to_animate].thread_id = G8RTOS_GetThreadId();

    /* Set the color of the ball (rand() generates 15 bit numbers, colors
     * generated here need to be 16 bit or we will miss half of the possible
     * colors) */
    balls[ball_to_animate].color = (int16_t)(rand() << 1);

    ++NumberOfBalls;

    while(1)
    {
        // Erase the ball's previous frame from the screen
        draw_ball(BACKGROUND_COLOR,
                  balls[ball_to_animate].x_position,
                  balls[ball_to_animate].y_position);

        // Update x velocity, capping at +/- MAX_VELOCITY
        balls[ball_to_animate].x_velocity = balls[ball_to_animate].x_velocity + x_accel / TIME_ELAPSED;
        if (balls[ball_to_animate].x_velocity <= -MAX_VELOCITY) balls[ball_to_animate].x_velocity = -MAX_VELOCITY;
        else if (balls[ball_to_animate].x_velocity >= MAX_VELOCITY) balls[ball_to_animate].x_velocity = MAX_VELOCITY;

        // Update y velocity, capping at +/- MAX_VELOCITY
        balls[ball_to_animate].y_velocity = balls[ball_to_animate].y_velocity + y_accel / TIME_ELAPSED;
        if (balls[ball_to_animate].y_velocity <= -MAX_VELOCITY) balls[ball_to_animate].y_velocity = -MAX_VELOCITY;
        else if (balls[ball_to_animate].y_velocity >= MAX_VELOCITY) balls[ball_to_animate].y_velocity = MAX_VELOCITY;

        // Update the x position using the position and the velocity, wrapping around back to 0 if we are past the maximum.
        balls[ball_to_animate].x_position = balls[ball_to_animate].x_position + balls[ball_to_animate].x_velocity / TIME_ELAPSED;
        if (balls[ball_to_animate].x_position >= MAX_SCREEN_X) balls[ball_to_animate].x_position = 0;
        else if (balls[ball_to_animate].x_position <= 0) balls[ball_to_animate].x_position = MAX_SCREEN_X;

        // Update the y position using the position and the velocity, wrapping around back to 0 if we are past the maximum.
        balls[ball_to_animate].y_position = balls[ball_to_animate].y_position + balls[ball_to_animate].y_velocity / TIME_ELAPSED;
        if (balls[ball_to_animate].y_position >= MAX_SCREEN_Y) balls[ball_to_animate].y_position = 0;
        else if (balls[ball_to_animate].y_position <= 0) balls[ball_to_animate].y_position = MAX_SCREEN_Y;

        // Draw the ball's new frame on the screen
        draw_ball(balls[ball_to_animate].color,
                  balls[ball_to_animate].x_position,
                  balls[ball_to_animate].y_position);

        // Sleep for 30ms
        G8RTOS_Sleep(30);
    }
}

/* Background thread - an idle thread that runs when nothing else can run. */
void Idle(void)
{
    while(1);
}

/* Helper function used to draw and erase a ball from the screen. */
void draw_ball(uint16_t color, uint16_t x_start, uint16_t y_start)
{
    G8RTOS_WaitSemaphore(&lcd_mutex);
    LCD_DrawRectangle(x_start, x_start+BALL_SIZE, y_start, y_start+BALL_SIZE, color);
    G8RTOS_SignalSemaphore(&lcd_mutex);
}
