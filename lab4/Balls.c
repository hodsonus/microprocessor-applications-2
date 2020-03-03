/*
 * Balls.c
 *
 *  Created on: Mar 3, 2020
 *      Author: johnhodson
 */

#include "Balls.h"
#include "BSP.h"
#include "G8RTOS/G8RTOS.h"

static ball balls[MAX_BALLS];
bool LCDTapped = false;
int16_t x_accel;
int16_t y_accel;

/* Aperiodic event - sets a flag when the screen is tapped. */
void LCDTapHandler(void)
{
    LCDTapped = true;

    if(P4->IFG & BIT0)
    {
        P4->IFG &= ~BIT0;
    }
}

/* Background thread - deals with the event when the LCD is tapped. */
void LCDTappedWorker(void)
{
    while(1)
    {
        if (LCDTapped)
        {
            // Retrieve the location at which the screen was tapped
            Point tap = TP_ReadXY();

            // Determine if we are creating or destroying a ball
            int ballToKill = -1;
            for (int i = 0; i < MAX_BALLS; ++i)
            {
                if (!balls[i].alive) continue;

                uint16_t delta_x = balls[i].x_position - tap.x;
                uint16_t delta_y = balls[i].y_position - tap.y;

                int distance = square_root(delta_x*delta_x + delta_y*delta_y);

                if (distance < TAP_DISTANCE_THRESHOLD)
                {
                    ballToKill = i;
                    break;
                }
            }

            if (ballToKill == -1)
            {
                // Create ball at location tap

                // Write the location of the tap to the shared FIFO
                G8RTOS_WriteFIFO(COORD_FIFO, tap.x << 16 | tap.y);

                // Create a new ball thread that will read from this FIFO
                G8RTOS_AddThread(&Ball, BALL_THREAD_PRIORITY, "ball thread");
            }
            else
            {
                // Erase the old ball from the screen
                draw_ball(BACKGROUND_COLOR,
                          balls[ballToKill].x_position,
                          balls[ballToKill].y_position);

                // Kill the ball thread
                G8RTOS_KillThread(balls[ballToKill].thread_id);
            }

            /* Sleep to account for screen bouncing and then reset the flag so
             * we can reenter. */
            G8RTOS_Sleep(500);
            LCDTapped = false;
        }
    }
}

/* Background thread - a thread that retrieves the accelerometer's current orientation. */
void ReadAccelerometer(void)
{
    while(1)
    {
        bmi160_read_accel_x(&x_accel);
        bmi160_read_accel_y(&y_accel);

        G8RTOS_Sleep(30);
    }
}

/* Background thread - dynamically killed and created by the LCDTappedWorker
 * and represents a ball. */
void Ball(void)
{
    // Read FIFO to remove the most recently inserted point
    int32_t point = G8RTOS_ReadFIFO(COORD_FIFO);

    // Finds a dead ball and makes it alive
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

    // Initialize coordinates accordingly from the point
    balls[ball_to_animate].x_position = point >> 16;
    balls[ball_to_animate].y_position = point & 0xFFFF;

    // TODO - initialize randomly
    balls[ball_to_animate].x_velocity = 0;
    balls[ball_to_animate].y_velocity = 0;

    balls[ball_to_animate].alive = true;

    balls[ball_to_animate].thread_id = G8RTOS_GetThreadId();

    balls[ball_to_animate].color = LCD_RED;

    while(1)
    {
        // Erase the old ball from the screen
        draw_ball(BACKGROUND_COLOR,
                  balls[ball_to_animate].x_position,
                  balls[ball_to_animate].y_position);

        int time_elapsed = 1;

        // Move position depending on velocity and acceleration
        balls[ball_to_animate].x_velocity = balls[ball_to_animate].x_velocity + x_accel / time_elapsed;
        balls[ball_to_animate].y_velocity = balls[ball_to_animate].y_velocity + y_accel / time_elapsed;

//        balls[ball_to_animate].x_position = balls[ball_to_animate].x_position + balls[ball_to_animate].x_velocity / time_elapsed;
//        balls[ball_to_animate].y_position = balls[ball_to_animate].y_position + balls[ball_to_animate].y_velocity / time_elapsed;

        balls[ball_to_animate].x_position = balls[ball_to_animate].x_position + 10;
        balls[ball_to_animate].y_position = balls[ball_to_animate].y_position + 10;

        if (balls[ball_to_animate].x_position >= MAX_SCREEN_X)
        {
            balls[ball_to_animate].x_position = 0;
        }

        if (balls[ball_to_animate].y_position >= MAX_SCREEN_Y)
        {
            balls[ball_to_animate].y_position = 0;
        }

        // Update ball on screen
        draw_ball(balls[ball_to_animate].color,
                  balls[ball_to_animate].x_position,
                  balls[ball_to_animate].y_position);

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
    LCD_DrawRectangle(x_start, x_start+BALL_SIZE, y_start, y_start+BALL_SIZE, color);
}

/* Helper function used to calculate the distance between a ball and a tap. */
int square_root(int n)
{
    int xk, xkp1 = n;

    do
    {
        xk = xkp1;
        xkp1 = (xk + (n / xk)) / 2;
    } while (abs(xkp1 - xk) >= 1);

    return xkp1;
}
