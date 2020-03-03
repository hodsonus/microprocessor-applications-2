/*
 * Balls.h
 *
 *  Created on: Mar 3, 2020
 *      Author: johnhodson
 */

#ifndef BALLS_H_
#define BALLS_H_

#include "G8RTOS/G8RTOS_Structures.h"

#define MAX_BALLS 20
#define COORD_FIFO 0
#define TAP_DISTANCE_THRESHOLD 15
#define BACKGROUND_COLOR LCD_BLACK

#define BALL_THREAD_PRIORITY 50
#define ACCEL_THREAD_PRIORITY 50
#define WORKER_PRIORITY 50
#define BALL_SIZE 10

typedef struct ball {
    uint16_t x_position;
    uint16_t y_position;
    int32_t x_velocity;
    int32_t y_velocity;
    bool alive;
    threadId_t thread_id;
    uint16_t color;
} ball;

/* Aperiodic event - sets a flag when the screen is tapped. */
void LCDTapHandler(void);

/* Background thread - deals with the event when the LCD is tapped. */
void LCDTappedWorker(void);

/* Background thread - a thread that retrieves the accelerometer's current orientation. */
void ReadAccelerometer(void);

/* Background thread - dynamically killed and created by the LCDTappedWorker
 * and represents a ball. */
void Ball(void);

/* Background thread - an idle thread that runs when nothing else can run. */
void Idle(void);

/* Helper function used to draw and erase a ball from the screen. */
void draw_ball(uint16_t color, uint16_t x_start, uint16_t y_start);

/* Helper function used to calculate the distance between a ball and a tap. */
int square_root(int n);

#endif /* BALLS_H_ */
