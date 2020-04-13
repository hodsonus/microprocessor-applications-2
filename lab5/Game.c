/*
 * Game.c
 *
 *  Created on: Apr 3, 2020
 *      Author: John Hodson, Shida Yang
 */

#include <stdlib.h>
#include "Game.h"

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame()
{
    // Temp variables to prevent hold-and-wait condition
    GameState_t tempGameState;
    // Set initial SpecificPlayerInfo_t strict attributes (you can get the IP address by calling getLocalIP()
    SpecificPlayerInfo_t tempClientInfo = {
                CONFIG_IP,     // IP Address
                0,             // displacement
                TOP,           // playerNumber
                1,             // ready
                0,             // joined
                0              // acknowledge
    };

    // Red LED = No connection
    G8RTOS_WaitSemaphore(&LED_Mutex);
    LP3943_LedModeSet(RED, RED_LED);
    G8RTOS_SignalSemaphore(&LED_Mutex);

    // Empty client info packet
    G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
    clientInfo = tempClientInfo;
    G8RTOS_SignalSemaphore(&SpecificPlayerInfo_Mutex);

    // Send player info to the host
    G8RTOS_WaitSemaphore(&WiFi_Mutex);
    SendData((uint8_t*)(&clientInfo), HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t)/sizeof(uint8_t));
    G8RTOS_SignalSemaphore(&WiFi_Mutex);

    // Wait for server response
    G8RTOS_WaitSemaphore(&WiFi_Mutex);
    while( ReceiveData((uint8_t*)(&tempGameState), sizeof(GameState_t)/sizeof(uint8_t)) == NOTHING_RECEIVED );
    G8RTOS_SignalSemaphore(&WiFi_Mutex);

    // Empty the received packet
    G8RTOS_WaitSemaphore(&GameState_Mutex);
    gameState = tempGameState;
    G8RTOS_SignalSemaphore(&GameState_Mutex);

    // If you've joined the game, acknowledge you've joined to the host and show connection with an LED
    if (tempGameState.player.acknowledge)
    {
        // Update local client info
        G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
        clientInfo.acknowledge = true;
        clientInfo.joined = true;
        tempClientInfo = clientInfo;
        G8RTOS_SignalSemaphore(&SpecificPlayerInfo_Mutex);

        // Send acknowledgment
        G8RTOS_WaitSemaphore(&WiFi_Mutex);
        SendData((uint8_t*)(&tempClientInfo), HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t)/sizeof(uint8_t));
        G8RTOS_SignalSemaphore(&WiFi_Mutex);

        // Update LED to show connection
        // Blue LED = Connection Established
        G8RTOS_WaitSemaphore(&LED_Mutex);
        LP3943_LedModeSet(RED, 0);
        LP3943_LedModeSet(BLUE, BLUE_LED);
        G8RTOS_SignalSemaphore(&LED_Mutex);
    }
    else
    {
        G8RTOS_AddThread(&HostVsClient, MAX_PRIO, "host vs client");
        G8RTOS_KillSelf();
    }

    // Initialize the board state
    InitBoardState();

    // Add client and common threads
    AddClientGameThreads();
    AddCommonGameThreads();

    // Kill self
    G8RTOS_KillSelf();
}

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost()
{
   while (1)
   {
       // Continually receive data until a return value greater than zero is returned (meaning valid data has been read)
       // Note: Remember to release and take the semaphore again so you've still able to send data
       GameState_t tempGameState;
       _i32 retVal = NOTHING_RECEIVED;
       while(retVal != SUCCESS)
       {
           G8RTOS_WaitSemaphore(&WiFi_Mutex);
           retVal = ReceiveData((uint8_t*)(&tempGameState), sizeof(GameState_t)/sizeof(uint8_t));
           G8RTOS_SignalSemaphore(&WiFi_Mutex);

           // Sleeping here for 1ms would avoid a deadlock
           G8RTOS_Sleep(1);
       }

       // Empty the received packet
       G8RTOS_WaitSemaphore(&GameState_Mutex);
       gameState = tempGameState;
       G8RTOS_SignalSemaphore(&GameState_Mutex);

       // If the game is done, add EndOfGameClient thread with the highest priority
       if (tempGameState.gameDone) G8RTOS_AddThread(&EndOfGameClient, MAX_PRIO, "End Client");

       // Sleep for 5ms
       G8RTOS_Sleep(5);
   }
}

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost()
{
    while (1)
    {
        // Get player info
        G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
        SpecificPlayerInfo_t tempClientInfo = clientInfo;
        G8RTOS_SignalSemaphore(&SpecificPlayerInfo_Mutex);

        // Send player info
        G8RTOS_WaitSemaphore(&WiFi_Mutex);
        SendData((uint8_t*)(&tempClientInfo), HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t)/sizeof(uint8_t));
        G8RTOS_SignalSemaphore(&WiFi_Mutex);

        // Sleep for 2ms
        G8RTOS_Sleep(2);
    }
}

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient()
{
    s16 js_x_bias, js_y_bias, js_x_data, js_y_data;

    // Determine joystick bias (found experimentally) since every joystick is offset by some small amount displacement and noise
    GetJoystickCoordinates(&js_x_bias, &js_y_bias);

   while (1)
   {
       // Read joystick and add bias
       GetJoystickCoordinates(&js_x_data, &js_y_data);
       js_x_data -= js_x_bias;
       js_y_data -= js_y_bias;

       js_x_data *= -1;
       js_y_data *= -1;

       // Add Displacement to Self accordingly
       G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
       clientInfo.displacement = js_x_data;
       G8RTOS_SignalSemaphore(&SpecificPlayerInfo_Mutex);

       // Sleep 10ms
       G8RTOS_Sleep(10);
   }
}

/*
 * End of game for the client
 */
void EndOfGameClient()
{
    // Wait for all semaphores to be released
    G8RTOS_WaitSemaphore(&LED_Mutex);
    G8RTOS_WaitSemaphore(&LCD_Mutex);
    G8RTOS_WaitSemaphore(&WiFi_Mutex);
    G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
    G8RTOS_WaitSemaphore(&GameState_Mutex);

    // Kill all other threads
    G8RTOS_KillAllOtherThreads();

    // Re-initialize semaphores
    G8RTOS_InitSemaphore(&LED_Mutex, 1);
    G8RTOS_InitSemaphore(&LCD_Mutex, 1);
    G8RTOS_InitSemaphore(&WiFi_Mutex, 1);
    G8RTOS_InitSemaphore(&SpecificPlayerInfo_Mutex, 1);
    G8RTOS_InitSemaphore(&GameState_Mutex, 1);

    // Clear screen with winner's color
    G8RTOS_WaitSemaphore(&LCD_Mutex);
    if (gameState.winner == TOP) LCD_Clear(gameState.players[TOP].color);
    else LCD_Clear(gameState.players[BOTTOM].color);
    G8RTOS_SignalSemaphore(&LCD_Mutex);

    // Wait for host to restart game
    while (gameState.gameDone)
    {
        // Wait for server response
        GameState_t tempGameState;
        G8RTOS_WaitSemaphore(&WiFi_Mutex);
        while( ReceiveData((uint8_t*)(&tempGameState), sizeof(GameState_t)/sizeof(uint8_t)) == NOTHING_RECEIVED);
        G8RTOS_SignalSemaphore(&WiFi_Mutex);

        // Empty the received packet
        G8RTOS_WaitSemaphore(&GameState_Mutex);
        gameState=tempGameState;
        G8RTOS_SignalSemaphore(&GameState_Mutex);
    }

    // Add all threads back and restart game variables

    // Initialize the board state
    InitBoardState();

    // Add client and common threads
    AddClientGameThreads();
    AddCommonGameThreads();

    // Kill Self
    G8RTOS_KillSelf();
}

/*********************************************** Client Threads *********************************************************************/


/*********************************************** Host Threads *********************************************************************/
/*
 * Thread for the host to create a game
 */
void CreateGame()
{
    // Temp variables to prevent hold-and-wait condition
    GameState_t tempGameState;
    SpecificPlayerInfo_t tempClientInfo;

    // Initializes the players
    G8RTOS_WaitSemaphore(&GameState_Mutex);
    // Host SpecificPlayerInfo
    gameState.player.IP_address = CONFIG_IP;
    gameState.player.playerNumber = BOTTOM;
    gameState.player.displacement = 0;
    gameState.player.ready = 1;
    gameState.player.joined = 0;
    gameState.player.acknowledge = 0;

    // Client: Top, blue
    gameState.players[TOP].position = TOP;
    gameState.players[TOP].color = PLAYER_BLUE;
    gameState.players[TOP].currentCenter = PADDLE_X_CENTER;
    rawClientCenter = (PADDLE_X_CENTER << PLAYER_CENTER_SHIFT_AMOUNT);

    // Host: bottom, red
    gameState.players[BOTTOM].position = BOTTOM;
    gameState.players[BOTTOM].color = PLAYER_RED;
    gameState.players[BOTTOM].currentCenter = PADDLE_X_CENTER;
    rawHostCenter = (PADDLE_X_CENTER << PLAYER_CENTER_SHIFT_AMOUNT);

    // Other variables
    gameState.numberOfBalls = 0;
    gameState.winner = 0;
    gameState.gameDone = 0;
    gameState.LEDScores[BOTTOM] = 0;
    gameState.LEDScores[TOP] = 0;
    gameState.overallScores[BOTTOM] = 0;
    gameState.overallScores[TOP] = 0;

    G8RTOS_SignalSemaphore(&GameState_Mutex);

    // Red LED = No connection
    G8RTOS_WaitSemaphore(&LED_Mutex);
    LP3943_LedModeSet(RED, RED_LED);
    G8RTOS_SignalSemaphore(&LED_Mutex);

    // Receive a packet from the client
    G8RTOS_WaitSemaphore(&WiFi_Mutex);
    while( ReceiveData((uint8_t*)(&tempClientInfo), sizeof(SpecificPlayerInfo_t)/sizeof(uint8_t)) == NOTHING_RECEIVED );
    G8RTOS_SignalSemaphore(&WiFi_Mutex);
    // Store the packet received
    G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
    clientInfo = tempClientInfo;
    G8RTOS_SignalSemaphore(&SpecificPlayerInfo_Mutex);

    // Update Host SpecificPlayerInfo
    G8RTOS_WaitSemaphore(&GameState_Mutex);
    gameState.player.joined = 1;
    gameState.player.acknowledge = 1;
    tempGameState = gameState;
    G8RTOS_SignalSemaphore(&GameState_Mutex);
    // Send new game state
    G8RTOS_WaitSemaphore(&WiFi_Mutex);
    SendData((uint8_t*)(&tempGameState), HOST_IP_ADDR, sizeof(GameState_t)/sizeof(uint8_t));
    G8RTOS_SignalSemaphore(&WiFi_Mutex);

    // Receive a packet from the client (waiting for acknowledgment)
    G8RTOS_WaitSemaphore(&WiFi_Mutex);
    while( ReceiveData((uint8_t*)(&tempClientInfo), sizeof(SpecificPlayerInfo_t)/sizeof(uint8_t)) == NOTHING_RECEIVED );
    G8RTOS_SignalSemaphore(&WiFi_Mutex);
    // Store the packet received
    G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
    clientInfo = tempClientInfo;
    G8RTOS_SignalSemaphore(&SpecificPlayerInfo_Mutex);

    if (tempClientInfo.acknowledge)
    {
        // Update LED to show connection
        // Blue LED = Connection Established
        G8RTOS_WaitSemaphore(&LED_Mutex);
        LP3943_LedModeSet(RED, 0);
        LP3943_LedModeSet(BLUE, BLUE_LED);
        G8RTOS_SignalSemaphore(&LED_Mutex);
    }
    else
    {
        G8RTOS_AddThread(&HostVsClient, MAX_PRIO, "host vs client");
        G8RTOS_KillSelf();
    }

    // Initialize the board (draw arena, players, and scores)
    InitBoardState();

    // Add host and common threads
    AddHostGameThreads();
    AddCommonGameThreads();

    // Kill self
    G8RTOS_KillSelf();
}

/*
 * Thread that sends game state to client
 */
void SendDataToClient()
{
    while (1)
    {
        // Fill packet for client
        G8RTOS_WaitSemaphore(&GameState_Mutex);
        GameState_t tempGameState = gameState;
        G8RTOS_SignalSemaphore(&GameState_Mutex);

        // Send packet
        G8RTOS_WaitSemaphore(&WiFi_Mutex);
        SendData((uint8_t*)(&tempGameState), HOST_IP_ADDR, sizeof(GameState_t)/sizeof(uint8_t));
        G8RTOS_SignalSemaphore(&WiFi_Mutex);

        // If game is done, add EndOfGameHost thread with highest priority
        if (tempGameState.gameDone) G8RTOS_AddThread(&EndOfGameHost, MAX_PRIO, "EOG Host");

        // Sleep for 16ms (found experimentally to be a good amount of time for synchronization)
        // Was previously 5ms, but this led to large buffers and an unplayable game
        G8RTOS_Sleep(16);
    }
}

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient()
{
    while (1)
    {
        // Continually receive data until a return value greater than zero is returned (meaning valid data has been read)
        // Note: Remember to release and take the semaphore again so you've still able to send data (not so sure, but it seems to be this way)
        SpecificPlayerInfo_t tempClientInfo;
        _i32 retVal = NOTHING_RECEIVED;
        while (retVal != SUCCESS)
        {
            G8RTOS_WaitSemaphore(&WiFi_Mutex);
            retVal = ReceiveData((uint8_t*)(&tempClientInfo), sizeof(SpecificPlayerInfo_t)/sizeof(uint8_t));
            G8RTOS_SignalSemaphore(&WiFi_Mutex);

            // Sleeping here for 1ms would avoid a deadlock
            G8RTOS_Sleep(1);
        }

        G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
        // Empty client info packet
        clientInfo = tempClientInfo;
        rawClientCenter += clientInfo.displacement;
        if(rawClientCenter > MAX_RAW_PLAYER_CENTER)
        {
            rawClientCenter = MAX_RAW_PLAYER_CENTER;
        }
        else if(rawClientCenter < MIN_RAW_PLAYER_CENTER)
        {
            rawClientCenter = MIN_RAW_PLAYER_CENTER;
        }
        G8RTOS_WaitSemaphore(&GameState_Mutex);

        // Update the player's current center with the displacement received from the client
        UpdatePlayerDisplacement(&clientInfo);
        G8RTOS_SignalSemaphore(&GameState_Mutex);
        G8RTOS_SignalSemaphore(&SpecificPlayerInfo_Mutex);

        // Sleep for 1ms (again found experimentally)
        // Was originally 2ms, but 1ms was found to work better
        G8RTOS_Sleep(1);
    }
}

/*
 * Generate Ball thread
 */
void GenerateBall()
{
    uint16_t numBallsTemp;

    while (1)
    {
        G8RTOS_WaitSemaphore(&GameState_Mutex);
        numBallsTemp = gameState.numberOfBalls;
        G8RTOS_SignalSemaphore(&GameState_Mutex);

        // Adds another MoveBall thread if the number of balls is less than the max
        if (numBallsTemp < MAX_NUM_OF_BALLS) G8RTOS_AddThread(&MoveBall, MOVEBALL_PRIO, "move ball");

        // Sleeps proportional to the number of balls currently in play
        G8RTOS_Sleep(1000 * numBallsTemp + 1);
    }
}

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost()
{
    s16 js_x_bias, js_y_bias, js_x_data, js_y_data;

    // Determine joystick bias (found experimentally) since every joystick is offset by some small amount displacement and noise
    GetJoystickCoordinates(&js_x_bias, &js_y_bias);

    while (1)
    {
        // Read the joystick ADC values by calling GetJoystickCoordinates, applying previously determined bias
        GetJoystickCoordinates(&js_x_data, &js_y_data);
        js_x_data -= js_x_bias;
        js_y_data -= js_y_bias;

        js_x_data *= -1;
        js_y_data *= -1;

        // Change self.displacement accordingly (you can experiment with how much you want to scale the ADC value)
        G8RTOS_WaitSemaphore(&GameState_Mutex);
        gameState.player.displacement = js_x_data;
        G8RTOS_SignalSemaphore(&GameState_Mutex);

        rawHostCenter += js_x_data;
        if (rawHostCenter > MAX_RAW_PLAYER_CENTER)
        {
            rawHostCenter = MAX_RAW_PLAYER_CENTER;
        }
        else if (rawHostCenter < MIN_RAW_PLAYER_CENTER)
        {
            rawHostCenter = MIN_RAW_PLAYER_CENTER;
        }

        // Sleep for 10ms, by sleeping before updating the bottom player's position, it makes the game more fair between client and host
        G8RTOS_Sleep(10);

        /* Then add the displacement to the bottom player in the list of players (general list that sent to the
         * client and used for drawing) i.e. players[0].position += self.displacement */
        G8RTOS_WaitSemaphore(&GameState_Mutex);
        UpdatePlayerDisplacement(&gameState.player);
        G8RTOS_SignalSemaphore(&GameState_Mutex);
    }
}

/*
 * Thread to move a single ball
 */
void MoveBall()
{
    // Go through array of balls and find one that's not alive
    int curr = -1;
    G8RTOS_WaitSemaphore(&GameState_Mutex);
    for (int i = 0; i < MAX_NUM_OF_BALLS; ++i)
    {
        if (!gameState.balls[i].alive)
        {
            curr = i;
            break;
        }
    }

    // Bug !
    if (curr == -1)
    {
        G8RTOS_SignalSemaphore(&GameState_Mutex);
        G8RTOS_KillSelf();
        while(1);
    }

    // Once found, initialize random position and X and Y velocities, as well as color and alive attributes
    gameState.balls[curr].alive = true;
    gameState.balls[curr].currentCenterX = ARENA_MIN_X + rand() / (RAND_MAX / (ARENA_MAX_X - ARENA_MIN_X + 1) + 1);
    gameState.balls[curr].currentCenterY = ARENA_MIN_Y + rand() / (RAND_MAX / (ARENA_MAX_Y - ARENA_MIN_Y + 1) + 1);
    gameState.balls[curr].velocityX = ((rand() % MAX_BALL_VELO) + 1);
    if (rand() & 1) gameState.balls[curr].velocityX *= -1;
    gameState.balls[curr].velocityY = ((rand() % MAX_BALL_VELO) + 1);
    if (rand() & 1) gameState.balls[curr].velocityY *= -1;
    gameState.balls[curr].color = INIT_BALL_COLOR;

    ++(gameState.numberOfBalls);

    G8RTOS_SignalSemaphore(&GameState_Mutex);

    while (1)
    {
        G8RTOS_WaitSemaphore(&GameState_Mutex);

        // Move the ball in its current direction according to its velocity
        gameState.balls[curr].currentCenterX += gameState.balls[curr].velocityX;
        gameState.balls[curr].currentCenterY += gameState.balls[curr].velocityY;

        // Check for 3 scenarios - (1) collision with a wall, (2) collision with paddle, or (3) past paddle

        // (1) If collision with wall occurs, flip x velocity and move ball in-bounds
        if (gameState.balls[curr].currentCenterX - BALL_SIZE_D2 < ARENA_MIN_X)
        {
            gameState.balls[curr].currentCenterX = ARENA_MIN_X + (BALL_SIZE_D2 * 2);
            gameState.balls[curr].velocityX *= -1;
        }
        else if (gameState.balls[curr].currentCenterX + BALL_SIZE_D2 > ARENA_MAX_X)
        {
            gameState.balls[curr].currentCenterX = ARENA_MAX_X - (BALL_SIZE_D2 * 2);
            gameState.balls[curr].velocityX *= -1;
        }

        // If there is an event with bottom paddle, occurs when the ball is below the bottom paddle's top edge (either scenario 2 or 3 has occurred)
        if (gameState.balls[curr].currentCenterY + BALL_SIZE_D2 > BOTTOM_PADDLE_EDGE - WIGGLE_ROOM)
        {
            // (2) If collision with paddle occurs, flip y velocity and move ball to edge of paddle
            // Collision with paddle occurs when ball center within the edges of the paddle
            if ( (gameState.players[BOTTOM].currentCenter - PADDLE_LEN_D2) < gameState.balls[curr].currentCenterX &&
                  gameState.balls[curr].currentCenterX < (gameState.players[BOTTOM].currentCenter + PADDLE_LEN_D2) )
            {
                gameState.balls[curr].currentCenterY = BOTTOM_PADDLE_EDGE - WIGGLE_ROOM - BALL_SIZE_D2;
                gameState.balls[curr].velocityY *= -1;
                gameState.balls[curr].color = gameState.players[BOTTOM].color;
            }
            // (3) If the ball passes the boundary edge, adjust score, account for the game possibly ending, and kill self
            // Passing the boundary edge occurs when the ball center is not within the edges of the paddle
            else
            {
                if (gameState.players[TOP].color == gameState.balls[curr].color)
                {
                    ++gameState.LEDScores[TOP];
                    if (gameState.LEDScores[TOP] > MAX_SCORE)
                    {
                        gameState.winner = TOP;
                        gameState.gameDone = true;
                    }
                }

                --(gameState.numberOfBalls);
                gameState.balls[curr].alive = false;

                G8RTOS_SignalSemaphore(&GameState_Mutex);
                G8RTOS_KillSelf();
                while(1);
            }
        }
        // Else if there is an event with top paddle, occurs when the ball is above the top paddle's bottom edge (either scenario 2 or 3 has occurred)
        else if (gameState.balls[curr].currentCenterY - BALL_SIZE_D2 < TOP_PADDLE_EDGE + WIGGLE_ROOM)
        {
            // (2) If collision with paddle occurs, flip y velocity and move ball to edge of paddle
            // Collision with paddle occurs when ball center within the edges of the paddle
            if ( (gameState.players[TOP].currentCenter - PADDLE_LEN_D2) < gameState.balls[curr].currentCenterX &&
                  gameState.balls[curr].currentCenterX < (gameState.players[TOP].currentCenter + PADDLE_LEN_D2) )
            {
                gameState.balls[curr].currentCenterY = TOP_PADDLE_EDGE + WIGGLE_ROOM + BALL_SIZE_D2;
                gameState.balls[curr].velocityY *= -1;
                gameState.balls[curr].color = gameState.players[TOP].color;
            }
            // (3) Else ball passes the boundary edge, adjust score, account for the game possibly ending, and kill self
            // Passing the boundary edge occurs when the ball center is not within the edges of the paddle
            else
            {
                if (gameState.players[BOTTOM].color == gameState.balls[curr].color)
                {
                    ++gameState.LEDScores[BOTTOM];
                    if (gameState.LEDScores[BOTTOM] > MAX_SCORE)
                    {
                        gameState.winner = BOTTOM;
                        gameState.gameDone = true;
                    }
                }

                --(gameState.numberOfBalls);
                gameState.balls[curr].alive = false;

                G8RTOS_SignalSemaphore(&GameState_Mutex);
                G8RTOS_KillSelf();
                while(1);
            }
        }

        G8RTOS_SignalSemaphore(&GameState_Mutex);

        // Sleep for 35ms
        G8RTOS_Sleep(35);
    }
}

/*
 * End of game for the host
 */
void EndOfGameHost()
{
    // Wait for all the semaphores to be released
    G8RTOS_WaitSemaphore(&LED_Mutex);
    G8RTOS_WaitSemaphore(&LCD_Mutex);
    G8RTOS_WaitSemaphore(&WiFi_Mutex);
    G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
    G8RTOS_WaitSemaphore(&GameState_Mutex);

    // Kill all other threads
    G8RTOS_KillAllOtherThreads();

    // Re-initialize semaphores
    G8RTOS_InitSemaphore(&LED_Mutex, 1);
    G8RTOS_InitSemaphore(&LCD_Mutex, 1);
    G8RTOS_InitSemaphore(&WiFi_Mutex, 1);
    G8RTOS_InitSemaphore(&SpecificPlayerInfo_Mutex, 1);
    G8RTOS_InitSemaphore(&GameState_Mutex, 1);

    // Clear screen with winner's color
    if (gameState.winner == TOP)
    {
        LCD_Clear(PLAYER_BLUE);
        ++gameState.overallScores[TOP];
    }
    else
    {
        LCD_Clear(PLAYER_RED);
        ++gameState.overallScores[BOTTOM];
    }

    // Print some message that waits for the host's action to start a new game
    LCD_Text(12, 100, "Press left to play again as the host.", LCD_WHITE);

    // Port should still be initialized from the original HostVsClient decision
    // Waits for the host's button press
    P5->IFG &= ~BIT5;
    while (!(P5->IFG & BIT5))
    {
        // Send EOG packet
        SendData((uint8_t*)(&gameState), HOST_IP_ADDR, sizeof(GameState_t)/sizeof(uint8_t));
        DelayMs(16);
    }
    P5->IFG &= ~BIT5;

    // Reset game variables
    clientInfo.displacement = 0;

    // Host SpecificPlayerInfo
    gameState.player.IP_address = CONFIG_IP;
    gameState.player.playerNumber = BOTTOM;
    gameState.player.displacement = 0;
    gameState.player.ready = 1;
    gameState.player.joined = 0;
    gameState.player.acknowledge = 0;

    // Client: Top, blue
    gameState.players[TOP].position = TOP;
    gameState.players[TOP].color = PLAYER_BLUE;
    gameState.players[TOP].currentCenter = PADDLE_X_CENTER;
    rawClientCenter=(PADDLE_X_CENTER<<PLAYER_CENTER_SHIFT_AMOUNT);

    // Host: bottom, red
    gameState.players[BOTTOM].position = BOTTOM;
    gameState.players[BOTTOM].color = PLAYER_RED;
    gameState.players[BOTTOM].currentCenter = PADDLE_X_CENTER;
    rawHostCenter=(PADDLE_X_CENTER<<PLAYER_CENTER_SHIFT_AMOUNT);

    // Other variables
    gameState.numberOfBalls = 0;
    for (int i = 0; i < MAX_NUM_OF_BALLS; ++i) gameState.balls[i].alive = false;
    gameState.winner = 0;
    gameState.gameDone = 0;
    gameState.LEDScores[BOTTOM] = 0;
    gameState.LEDScores[TOP] = 0;

    // Send notification to client, the client is just waiting on the host to start a new game
    SendData((uint8_t*)(&gameState), HOST_IP_ADDR, sizeof(GameState_t)/sizeof(uint8_t));

    // Reinitialize the game and objects
    InitBoardState();

    // Add back all the threads
    AddHostGameThreads();
    AddCommonGameThreads();

    // Kill self
    G8RTOS_KillSelf();
}

/*********************************************** Host Threads *********************************************************************/


/*********************************************** Common Threads *********************************************************************/
/*
 * Idle thread
 */
void IdleThread()
{
   while(1);
}

/*
 * Thread to draw all the objects in the game
 */
void DrawObjects()
{
    while (1)
    {
        GameState_t tempGameState;
        G8RTOS_WaitSemaphore(&GameState_Mutex);
        tempGameState = gameState;
        G8RTOS_SignalSemaphore(&GameState_Mutex);

        // Draw and/or update balls (you'll need a way to tell whether to draw a new ball, or update its position (i.e. if a new ball has just been created - hence the alive attribute in the Ball_t struct.
        for (int i = 0; i < MAX_NUM_OF_BALLS; i++)
        {
            if (!prevBalls[i].alive)
            {
                // Dead ball became alive, should draw new ball
                if (tempGameState.balls[i].alive)
                {
                    DrawBallOnScreen(&(prevBalls[i]), &(tempGameState.balls[i]));
                }
            }
            else
            {
                // alive ball is still alive, update ball
                if (tempGameState.balls[i].alive)
                {
                    UpdateBallOnScreen(&(prevBalls[i]), &(tempGameState.balls[i]));
                }
                // alive ball is dead, erase the ball
                else
                {
                    DeleteBallOnScreen(&(prevBalls[i]));
                }
            }

            prevBalls[i].alive = tempGameState.balls[i].alive;
        }

        // Update players
        for(int i = 0; i < MAX_NUM_OF_PLAYERS; ++i) UpdatePlayerOnScreen(&prevPlayers[i], &(tempGameState.players[i]));

        // Sleep for 20ms (reasonable refresh rate)
        G8RTOS_Sleep(20);
    }
}

/*
 * Thread to update LEDs based on score
 */
void MoveLEDs()
{
    while(1)
    {
        // Responsible for updating the LED array with current scores
        UpdateLEDScore();

        // 20ms should be enough, just keep the refresh rate the same as the screen
        G8RTOS_Sleep(20);
    }
}

/*
 * Thread to set up game as host or client, depending on user input
 */
void HostVsClient()
{
    // Initialize semaphores
    G8RTOS_InitSemaphore(&LED_Mutex, 1);
    G8RTOS_InitSemaphore(&LCD_Mutex, 1);
    G8RTOS_InitSemaphore(&WiFi_Mutex, 1);
    G8RTOS_InitSemaphore(&SpecificPlayerInfo_Mutex, 1);
    G8RTOS_InitSemaphore(&GameState_Mutex, 1);

    // Write message on screen assisting player choice of Host vs. Client
    LCD_Text(0, 100, "Press left for host and right for client", LCD_WHITE);

    playerType role = GetPlayerRole();
    if (role == Client) G8RTOS_AddThread(&JoinGame, MAX_PRIO, "join");
    else G8RTOS_AddThread(&CreateGame, MAX_PRIO, "create");

    G8RTOS_WaitSemaphore(&LCD_Mutex);
    LCD_Clear(BACK_COLOR);
    G8RTOS_SignalSemaphore(&LCD_Mutex);

    G8RTOS_KillSelf();
}

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/
/*
 * Returns either Host or Client depending on button press
 */
playerType GetPlayerRole()
{
    // Configure as I/O
    P4->SEL0 &= ~BIT5;
    P4->SEL1 &= ~BIT5;
    P5->SEL0 &= ~BIT5;
    P5->SEL1 &= ~BIT5;

    // Configure as input pin
    P4->DIR &= ~BIT5;
    P5->DIR &= ~BIT5;

    // Enable pull resistor on this pin
    P4->REN |= BIT5;
    P5->REN |= BIT5;

    // Pull up
    P4->OUT |= BIT5;
    P5->OUT |= BIT5;

    // Clear pending interrupt flags for this port
    P4->IFG &= ~BIT5;
    P5->IFG &= ~BIT5;

    // P5.5 is left, P4.5 is right
    while (1)
    {
        if (P5->IFG & BIT5)
        {
            P5->IFG &= ~BIT5;
            return Host;
        }
        if (P4->IFG & BIT5)
        {
            P4->IFG &= ~BIT5;
            return Client;
        }
    }
}

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t *player)
{
    G8RTOS_WaitSemaphore(&LCD_Mutex);
    // Bottom player
    if(player->position == BOTTOM)
    {
        LCD_DrawRectangle(player->currentCenter - PADDLE_LEN_D2,
                          player->currentCenter + PADDLE_LEN_D2,
                          BOTTOM_PADDLE_EDGE,
                          ARENA_MAX_Y,
                          player->color
        );
    }
    // Top player
    else
    {
        LCD_DrawRectangle(player->currentCenter - PADDLE_LEN_D2,
                          player->currentCenter + PADDLE_LEN_D2,
                          ARENA_MIN_Y,
                          TOP_PADDLE_EDGE,
                          player->color
        );
    }
    G8RTOS_SignalSemaphore(&LCD_Mutex);
}

/*
 * Updates player's paddle based on current and new center
 * (Only redraw the entire paddle when necessary)
 */
void UpdatePlayerOnScreen(PrevPlayer_t *prevPlayerIn, GeneralPlayerInfo_t *outPlayer)
{
    int16_t displacement = outPlayer->currentCenter - prevPlayerIn->Center;

    G8RTOS_WaitSemaphore(&LCD_Mutex);
    // If the displacement is greater than the paddle length, we need to redraw the entire paddle
    if(abs(displacement) >= PADDLE_LEN)
    {
        // Bottom player
        if(outPlayer->position == BOTTOM)
        {
            LCD_DrawRectangle(prevPlayerIn->Center - PADDLE_LEN_D2,
                              prevPlayerIn->Center + PADDLE_LEN_D2,
                              BOTTOM_PADDLE_EDGE,
                              ARENA_MAX_Y,
                              BACK_COLOR
            );
            LCD_DrawRectangle(outPlayer->currentCenter - PADDLE_LEN_D2,
                              outPlayer->currentCenter + PADDLE_LEN_D2,
                              BOTTOM_PADDLE_EDGE,
                              ARENA_MAX_Y,
                              outPlayer->color
            );
        }
        // Top player
        else
        {
            LCD_DrawRectangle(prevPlayerIn->Center - PADDLE_LEN_D2,
                              prevPlayerIn->Center + PADDLE_LEN_D2,
                              ARENA_MIN_Y,
                              TOP_PADDLE_EDGE,
                              BACK_COLOR
            );
            LCD_DrawRectangle(outPlayer->currentCenter - PADDLE_LEN_D2,
                              outPlayer->currentCenter + PADDLE_LEN_D2,
                              ARENA_MIN_Y,
                              TOP_PADDLE_EDGE,
                              outPlayer->color
            );
        }
    }
    // Else, we only need to partially update the paddle
    else
    {
        // Calculate area need to modify
        int16_t deleteL, deleteR, drawL, drawR;
        // Move left
        if(displacement <= 0)
        {
            deleteR=prevPlayerIn->Center + PADDLE_LEN_D2;
            deleteL=deleteR + displacement;
            drawR=prevPlayerIn->Center - PADDLE_LEN_D2;
            drawL=drawR + displacement;
        }
        // Move right
        else
        {
            deleteL=prevPlayerIn->Center - PADDLE_LEN_D2;
            deleteR=deleteL + displacement;
            drawL=prevPlayerIn->Center + PADDLE_LEN_D2;
            drawR=drawL + displacement;
        }

        // Modify area
        // Bottom player
        if(outPlayer->position == BOTTOM)
        {
            LCD_DrawRectangle(deleteL,
                              deleteR,
                              BOTTOM_PADDLE_EDGE,
                              ARENA_MAX_Y,
                              BACK_COLOR
            );
            LCD_DrawRectangle(drawL,
                              drawR,
                              BOTTOM_PADDLE_EDGE,
                              ARENA_MAX_Y,
                              outPlayer->color
            );
        }
        // Top player
        else
        {
            LCD_DrawRectangle(deleteL,
                              deleteR,
                              ARENA_MIN_Y,
                              TOP_PADDLE_EDGE,
                              BACK_COLOR
            );
            LCD_DrawRectangle(drawL,
                              drawR,
                              ARENA_MIN_Y,
                              TOP_PADDLE_EDGE,
                              outPlayer->color
            );
        }
    }
    G8RTOS_SignalSemaphore(&LCD_Mutex);

    prevPlayerIn->Center = outPlayer->currentCenter;
}

/*
 * Draw a new ball on screen
 */
void DrawBallOnScreen(PrevBall_t *previousBall, Ball_t *currentBall)
{
    G8RTOS_WaitSemaphore(&LCD_Mutex);
    // Draw the new ball
    int16_t drawL, drawR, drawT, drawB;
    drawL=currentBall->currentCenterX - BALL_SIZE_D2;
    drawR=currentBall->currentCenterX + BALL_SIZE_D2;
    drawT=currentBall->currentCenterY - BALL_SIZE_D2;
    drawB=currentBall->currentCenterY + BALL_SIZE_D2;
    if(drawL<=ARENA_MIN_X){
        drawL=ARENA_MIN_X+1;
        drawR=drawL+BALL_SIZE;
    }
    if(drawR>=ARENA_MAX_X){
        drawR=ARENA_MAX_X-1;
        drawL=drawR-BALL_SIZE;
    }
    LCD_DrawRectangle(drawL,
                      drawR,
                      drawT,
                      drawB,
                      currentBall->color
    );
    G8RTOS_SignalSemaphore(&LCD_Mutex);

    previousBall->CenterX = currentBall->currentCenterX;
    previousBall->CenterY = currentBall->currentCenterY;
}

/*
 * Delete a dead ball on screen
 */
void DeleteBallOnScreen(PrevBall_t * previousBall)
{
    G8RTOS_WaitSemaphore(&LCD_Mutex);
    // Delete the old ball
    int16_t deleteL, deleteR, deleteT, deleteB;
    deleteL=previousBall->CenterX - BALL_SIZE_D2;
    deleteR=previousBall->CenterX + BALL_SIZE_D2;
    deleteT=previousBall->CenterY - BALL_SIZE_D2;
    deleteB=previousBall->CenterY + BALL_SIZE_D2;
    if(deleteL<=ARENA_MIN_X){
        deleteL=ARENA_MIN_X+1;
        deleteR=deleteL+BALL_SIZE;
    }
    if(deleteR>=ARENA_MAX_X){
        deleteR=ARENA_MAX_X-1;
        deleteL=deleteR-BALL_SIZE;
    }
    LCD_DrawRectangle(deleteL,
                      deleteR,
                      deleteT,
                      deleteB,
                      BACK_COLOR
    );
    G8RTOS_SignalSemaphore(&LCD_Mutex);
}

/*
 * Function updates ball position on screen
 */
// Note: I commented out the outColor variable because I could not find any use of it
void UpdateBallOnScreen(PrevBall_t *previousBall, Ball_t *currentBall/*, uint16_t outColor */)
{
    // Delete the old ball
    DeleteBallOnScreen(previousBall);

    G8RTOS_WaitSemaphore(&GameState_Mutex);
    Ball_t tempCurrentBall=*currentBall;
    G8RTOS_SignalSemaphore(&GameState_Mutex);
    // Draw the new ball
    DrawBallOnScreen(previousBall, &tempCurrentBall);
}

/*
 * Function updates overall scores
 */
void UpdateOverallScore()
{
    // Convert score to string
    char player0ScoreStr[3], player1ScoreStr[3];
    snprintf(player0ScoreStr, 3, "%02d", gameState.overallScores[0]);
    snprintf(player1ScoreStr, 3, "%02d", gameState.overallScores[1]);

    G8RTOS_WaitSemaphore(&LCD_Mutex);

    // Clear score region
    LCD_DrawRectangle(TOP_SCORE_MIN_X, TOP_SCORE_MAX_X, TOP_SCORE_MIN_Y, TOP_SCORE_MAX_Y, BACK_COLOR);
    LCD_DrawRectangle(BOTTOM_SCORE_MIN_X, BOTTOM_SCORE_MAX_X, BOTTOM_SCORE_MIN_Y, BOTTOM_SCORE_MAX_Y, BACK_COLOR);

    // if player0 is BOTTOM, player1 is TOP
    if (gameState.players[0].position == BOTTOM)
    {
        LCD_Text(BOTTOM_SCORE_MIN_X, BOTTOM_SCORE_MIN_Y, player0ScoreStr, gameState.players[0].color);
        LCD_Text(TOP_SCORE_MIN_X, TOP_SCORE_MIN_Y, player1ScoreStr, gameState.players[1].color);
    }
    // player0 is TOP, player1 is BUTTOM
    else
    {
        LCD_Text(TOP_SCORE_MIN_X, TOP_SCORE_MIN_Y, player0ScoreStr, gameState.players[0].color);
        LCD_Text(BOTTOM_SCORE_MIN_X, BOTTOM_SCORE_MIN_Y, player1ScoreStr, gameState.players[1].color);
    }

    G8RTOS_SignalSemaphore(&LCD_Mutex);
}

/*
 * Function updates LED scores
 */
void UpdateLEDScore()
{
    uint16_t player0LEDScore = 0;
    uint16_t player1LEDScore = 0;
    for (int i = 0; i < gameState.LEDScores[0]; i++) player0LEDScore |= (BIT0<<i);
    for (int i = 0; i < gameState.LEDScores[1]; i++) player1LEDScore |= (BITF>>i);

    G8RTOS_WaitSemaphore(&LED_Mutex);
    // Clear LED scores
    LP3943_LedModeSet(BLUE, 0);
    LP3943_LedModeSet(RED, 0);
    // If player0 is RED, player1 is BLUE
    if(gameState.players[0].color == PLAYER_RED)
    {
        LP3943_LedModeSet(RED, player0LEDScore);
        LP3943_LedModeSet(BLUE, player1LEDScore);
    }
    // Else, player0 is BLUE, player1 is RED
    else
    {
        LP3943_LedModeSet(BLUE, player0LEDScore);
        LP3943_LedModeSet(RED, player1LEDScore);
    }
    G8RTOS_SignalSemaphore(&LED_Mutex);
}

/*
 * Initializes and prints initial game state
 */
void InitBoardState()
{
    G8RTOS_WaitSemaphore(&LCD_Mutex);
    // Clear background
    LCD_Clear(BACK_COLOR);
    // Draw two vertical lines
    for(int i = ARENA_MIN_Y; i <= ARENA_MAX_Y; i++)
    {
        LCD_SetPoint(ARENA_MIN_X, i, LCD_WHITE);
        LCD_SetPoint(ARENA_MAX_X, i, LCD_WHITE);
    }
    G8RTOS_SignalSemaphore(&LCD_Mutex);

    // Draw player paddles
    DrawPlayer(&(gameState.players[0]));
    DrawPlayer(&(gameState.players[1]));

    // Draw scores
    UpdateOverallScore();
    UpdateLEDScore();
}

/*
 * Adds the client's game threads - abstraction used to clean up the initialization of a new game.
 */
void AddClientGameThreads()
{
    G8RTOS_AddThread(&ReadJoystickClient, JOYSTICK_PRIO, "joystick");
    G8RTOS_AddThread(&SendDataToHost, SENDDATA_PRIO, "send");
    G8RTOS_AddThread(&ReceiveDataFromHost, RECEIVEDATA_PRIO, "receive");
}

/*
 * Adds the host's game threads - abstraction used to clean up the initialization of a new game.
 */
void AddHostGameThreads()
{
    G8RTOS_AddThread(&GenerateBall, GENBALL_PRIO, "gen ball");
    G8RTOS_AddThread(&ReadJoystickHost, JOYSTICK_PRIO, "joystick");
    G8RTOS_AddThread(&SendDataToClient, SENDDATA_PRIO, "send data");
    G8RTOS_AddThread(&ReceiveDataFromClient, RECEIVEDATA_PRIO, "rcv data");
}

/*
 * Adds the common game threads - abstraction used to clean up the initialization of a new game.
 */
void AddCommonGameThreads()
{
    G8RTOS_AddThread(&DrawObjects, DRAWOBJ_PRIO, "draw");
    G8RTOS_AddThread(&MoveLEDs, MOVELED_PRIO, "leds");
    G8RTOS_AddThread(&IdleThread, MIN_PRIO, "idle");
}

/*
 * Updates a particular player's displacement, given it's SpecificPlayerInfo_t struct.
 * NOTE - MUST BE HOLDING THE GameState MUTEX AND/OR THE CORRESPONDING SpecificPlayerInfo MUTEX WHEN CALLING THIS FUNCTION
 */
void UpdatePlayerDisplacement(SpecificPlayerInfo_t *player)
{
    // Client
    if((player->playerNumber)==TOP)
    {
        gameState.players[player->playerNumber].currentCenter=(int16_t)((rawClientCenter>>PLAYER_CENTER_SHIFT_AMOUNT)&0xFFFF);
    }
    // Host
    else
    {
        gameState.players[player->playerNumber].currentCenter=(int16_t)((rawHostCenter>>PLAYER_CENTER_SHIFT_AMOUNT)&0xFFFF);
    }

    // Wraparound logic
    if (gameState.players[player->playerNumber].currentCenter > HORIZ_CENTER_MAX_PL)
    {
        gameState.players[player->playerNumber].currentCenter = HORIZ_CENTER_MAX_PL;
    }
    else if (gameState.players[player->playerNumber].currentCenter < HORIZ_CENTER_MIN_PL)
    {
        gameState.players[player->playerNumber].currentCenter = HORIZ_CENTER_MIN_PL;
    }
}
/*********************************************** Public Functions *********************************************************************/
