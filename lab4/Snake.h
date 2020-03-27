/*
 * Snake.h
 *
 *  Created on: Mar 27, 2020
 *      Author: johnhodson
 */

#ifndef SNAKE_H_
#define SNAKE_H_

#include "G8RTOS/G8RTOS.h"

/* Constants */
#define SNAKE_LENGTH 256

#define JOYSTICK_SLEEP 30
#define SNAKE_SLEEP 30

#define JOYSTICK_THRESH 500

#define BLOCK_SIZE 6
#define SNAKE_BLOCK_SIZE 4

#define BACKGROUND_COLOR LCD_BLACK


/* Typedefs */
typedef struct point_t {
    int32_t x;
    int32_t y;
} point_t;

typedef enum direction_t {
    UP,
    DOWN,
    LEFT,
    RIGHT
} direction_t;

typedef struct snake_t {
    point_t body[SNAKE_LENGTH];
    int head;
    int tail;
    int length;
    direction_t dir;
    uint16_t color;
} snake_t;


/* Global variables */
semaphore_t lcd_mutex;
snake_t snake;
#define SNAKE_DIR_FIFO 0


/* Threads */

/* Aperiodic event - sets a flag when the screen is tapped. */
void AperiodicTap(void);

/* Sets up the game and waits for a tap. */
void StartGame(void);

/* Snake worker thread */
void Snake(void);

/* Continuously reads from joystick. */
void Joystick(void);

/* Idle thread */
void IdleSnake(void);


/* Helper Functions */
bool detect_snake_collison(void);
void inline write_snake_block(point_t loc, uint16_t color);


#endif /* SNAKE_H_ */
