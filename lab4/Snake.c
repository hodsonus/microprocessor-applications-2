/*
 * Snake.c
 *
 *  Created on: Mar 27, 2020
 *      Author: johnhodson
 */

#include "Snake.h"
#include "BSP.h"

bool ScreenTapped = false;

void StartGame(void)
{
    // Kill all other threads upon arrival
    G8RTOS_KillAllOtherThreads();

    // Clear the screen
    LCD_Clear(BACKGROUND_COLOR);

    // Display prompt waiting for tap
    LCD_Text(MAX_SCREEN_X/2-50, MAX_SCREEN_Y/2-50, "Tap to start!", LCD_BLUE);

    while(!ScreenTapped);

    // TODO - disable touch interrupt and clear flag?

    // Clear the screen
    LCD_Clear(BACKGROUND_COLOR);

    // Initialize the semaphore protecting the LCD
    G8RTOS_InitSemaphore(&lcd_mutex, 1);

    // Initialize FIFO used to update snake position
    G8RTOS_InitFIFO(SNAKE_DIR_FIFO);

    G8RTOS_AddThread(&Snake, 25, "snake");
    G8RTOS_AddThread(&Joystick, 25, "joystick");
    G8RTOS_AddThread(&IdleSnake, 255, "idle");

    G8RTOS_KillSelf();
}

void AperiodicTap(void)
{
    // If the interrupt flag for BIT0 is set
    if (P4->IFG & BIT0)
    {
        // Set the LCD flag
        ScreenTapped = true;

        // And clear the interrupt flag
        P4->IFG &= ~BIT0;
    }
}

void Snake(void)
{
    direction_t proposed_dir;

    // Initialize snake attributes
    snake.head = 0;
    snake.tail = 0;
    snake.length = 1;
    snake.dir = RIGHT;
    snake.color = LCD_BLUE;
    int dir_fifo = SNAKE_DIR_FIFO;

    while (1)
    {
        /* Update the direction that we are moving.
         * Avoids blocking on a read if there is no new data. */
        if (!G8RTOS_FIFOIsEmpty(dir_fifo)) proposed_dir = (direction_t)G8RTOS_ReadFIFO(dir_fifo);

        // Only update the direction of the snake if it is not in opposition to our current direction
        if (proposed_dir == LEFT && snake.dir != RIGHT ||
            proposed_dir == RIGHT && snake.dir != LEFT ||
            proposed_dir == UP && snake.dir != DOWN ||
            proposed_dir == DOWN && snake.dir != UP) snake.dir = proposed_dir;

        // If we have outgrown the body
        if (snake.length == SNAKE_LENGTH)
        {
            // Erase the snake tail
            write_snake_block(snake.body[snake.tail], BACKGROUND_COLOR);

            // Update the location of the snake tail
            snake.tail = (snake.tail + 1) % SNAKE_LENGTH;
        }
        else ++snake.length;

        // Calculate the index of the new head
        int new_head = (snake.head + 1) % SNAKE_LENGTH;

        // Copy the old head's data to the new head
        snake.body[new_head] = snake.body[snake.head];

        // Update the index of the new head
        snake.head = new_head;

        // Adjust the x/y coordinate depending on which way we are moving
        if (snake.dir == LEFT) snake.body[snake.head].x -= BLOCK_SIZE;
        else if (snake.dir == RIGHT) snake.body[snake.head].x += BLOCK_SIZE;
        else if (snake.dir == DOWN) snake.body[snake.head].y -= BLOCK_SIZE;
        else if (snake.dir == UP) snake.body[snake.head].y += BLOCK_SIZE;

        // Wraparound (cases should be m.e.)
        if (snake.body[snake.head].x >= MAX_SCREEN_X - BLOCK_SIZE + 1) snake.body[snake.head].x = 0;
        else if (snake.body[snake.head].x < 0) snake.body[snake.head].x = MAX_SCREEN_X - BLOCK_SIZE;
        else if (snake.body[snake.head].y >= MAX_SCREEN_Y - BLOCK_SIZE + 1) snake.body[snake.head].y = 0;
        else if (snake.body[snake.head].y < 0) snake.body[snake.head].y = MAX_SCREEN_Y - BLOCK_SIZE;

        // Detect collision by reading the proposed head
//        G8RTOS_WaitSemaphore(&lcd_mutex);
//        int proposed_head_color = LCD_ReadPixelColor(snake.body[snake.head].x/BLOCK_SIZE*BLOCK_SIZE + SNAKE_BLOCK_SIZE/2, snake.body[snake.head].y/BLOCK_SIZE*BLOCK_SIZE + SNAKE_BLOCK_SIZE/2);
//        bool collision = proposed_head_color == snake.color;
//        G8RTOS_SignalSemaphore(&lcd_mutex);

        bool collision = detect_snake_collison();

        if (!collision)
        {
            // Draw the new head
            write_snake_block(snake.body[snake.head], snake.color);
        }
        else
        {
            // Restart the game
            ScreenTapped = false;
            P4->IFG &= ~BIT0;
            G8RTOS_AddThread(&StartGame, 0, "start");
        }

        G8RTOS_Sleep(SNAKE_SLEEP);
    }
}

void Joystick(void)
{
    int16_t x, y;
    int16_t x_zero, y_zero;

    // Get the first readings
    GetJoystickCoordinates(&x_zero, &y_zero);

    while (1)
    {
        // Retrieve the latest reading from the Joystick
        GetJoystickCoordinates(&x, &y);

        // Zero them according to the first reading
        x -= x_zero;
        y -= y_zero;

        // If either reading is above the threshold, pick a new direction for our snake
        if (abs(x) > JOYSTICK_THRESH || abs(y) > JOYSTICK_THRESH)
        {
            direction_t new_dir;

            /* If x is pushed farther than y, move in the x plane
             * Else, move in the y plane */
            if (abs(x) > abs(y))
            {
                if (x < 0) new_dir = RIGHT;
                else new_dir = LEFT;

            }
            else
            {
                if (y < 0) new_dir = DOWN;
                else new_dir = UP;
            }

            // Write the new information to the snake direction FIFO
            G8RTOS_WriteFIFO(SNAKE_DIR_FIFO, new_dir);
        }

        // Sleep for appropriate amount of time
        G8RTOS_Sleep(JOYSTICK_SLEEP);
    }
}

void IdleSnake(void)
{
    while(1);
}

bool detect_snake_collison(void)
{
    if (snake.length < 5) return false;
    point_t head = snake.body[0];
    for (int i = 1; i < snake.length; ++i)
    {
        if (snake.body[i].x/BLOCK_SIZE == head.x/BLOCK_SIZE && snake.body[i].y/BLOCK_SIZE == head.y/BLOCK_SIZE) return true;
    }

    return false;
}

void inline write_snake_block(point_t loc, uint16_t color)
{
    G8RTOS_WaitSemaphore(&lcd_mutex);

    // Bad logic here but fixes weird wraparound bug
    LCD_DrawRectangle(loc.x/BLOCK_SIZE*BLOCK_SIZE, loc.x/BLOCK_SIZE*BLOCK_SIZE+SNAKE_BLOCK_SIZE, loc.y/BLOCK_SIZE*BLOCK_SIZE, loc.y/BLOCK_SIZE*BLOCK_SIZE+SNAKE_BLOCK_SIZE, color);

    G8RTOS_SignalSemaphore(&lcd_mutex);
}
