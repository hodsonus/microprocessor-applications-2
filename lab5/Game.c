/*
 * Game.c
 *
 *  Created on: Apr 3, 2020
 *      Author: John Hodson, Shida Yang
 */

#include "Game.h"

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame()
{
    // Red LED = No connection
    G8RTOS_WaitSemaphore(&LED_Mutex);
    LP3943_LedModeSet(RED, RED_LED);
    G8RTOS_SignalSemaphore(&LED_Mutex);

    // Set initial SpecificPlayerInfo_t strict attributes (you can get the IP address by calling getLocalIP()
    SpecificPlayerInfo_t tempClientInfo = {
                CONFIG_IP,     // IP Address
                0,             // displacement
                BOTTOM,        // playerNumber
                1,             // ready
                0,             // joined
                0              // acknowledge
    };

    // Empty client info packet
    G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
    clientInfo = tempClientInfo;
    G8RTOS_SignalSemaphore(&SpecificPlayerInfo_Mutex);

    // Send player info to the host
    G8RTOS_WaitSemaphore(&WiFi_Mutex);
    SendData((uint8_t*)(&clientInfo), HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t)/sizeof(uint8_t));
    G8RTOS_SignalSemaphore(&WiFi_Mutex);

    // Wait for server response
    GameState_t tempGameState;
    G8RTOS_WaitSemaphore(&WiFi_Mutex);
    while( ReceiveData((uint8_t*)(&tempGameState), sizeof(GameState_t)/sizeof(uint8_t)) == NOTHING_RECEIVED );
    G8RTOS_SignalSemaphore(&WiFi_Mutex);

    // Empty the received packet
    G8RTOS_WaitSemaphore(&GameState_Mutex);
    gameState = tempGameState;
    G8RTOS_SignalSemaphore(&GameState_Mutex);

    // If you've joined the game, acknowledge you've joined to the host and show connection with an LED
    // TODO - Shida, I don't understand this logic here to add the hsotvsclient thread if player.joined != 1
    if (tempGameState.player.acknowledge == 1)
    {
        // Update local client info
        G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
        clientInfo.acknowledge = 1;
        clientInfo.joined=1;
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
        // TODO - decide what to do here
        G8RTOS_AddThread(&HostVsClient, 0, "host vs client");
    }

    // Initialize the board state
    InitBoardState();

    // Add client and common threads
    // TODO - determine priorities
    G8RTOS_AddThread(&ReadJoystickClient, 0, "joystick");
    G8RTOS_AddThread(&SendDataToHost, 0, "send");
    G8RTOS_AddThread(&ReceiveDataFromHost, 0, "receive");
    G8RTOS_AddThread(&DrawObjects, 0, "draw");
    G8RTOS_AddThread(&MoveLEDs, 0, "leds");
    G8RTOS_AddThread(&IdleThread, 255, "idle");

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
       if (tempGameState.gameDone == 1) G8RTOS_AddThread(&EndOfGameClient, 0, "End Client");

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
    if (gameState.winner == TOP)
    {
        LCD_Clear(PLAYER_BLUE);
    }
    else
    {
        LCD_Clear(PLAYER_RED);
    }
    G8RTOS_SignalSemaphore(&LCD_Mutex);

    // Wait for host to restart game
    while (gameState.gameDone == 1)
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
    // TODO - determine priorities
    G8RTOS_AddThread(&ReadJoystickClient, 0, "joystick");
    G8RTOS_AddThread(&SendDataToHost, 0, "send");
    G8RTOS_AddThread(&ReceiveDataFromHost, 0, "receive");
    G8RTOS_AddThread(&DrawObjects, 0, "draw");
    G8RTOS_AddThread(&MoveLEDs, 0, "leds");
    G8RTOS_AddThread(&IdleThread, 255, "idle");

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
    // TODO - Initializes the players

    // TODO - Establish connection with client (use an LED on the Launchpad to indicate Wi-Fi connection)

        // TODO - Should be trying to receive a packet from the client

        // TODO - Should acknowledge client once client has joined

        // TODO - Wait for client acknowledgment

    // TODO - Initialize the board (draw arena, players, and scores)

    // Add host and common threads
    // TODO - determine priorities
    G8RTOS_AddThread(&GenerateBall, 0, "gen ball");
    G8RTOS_AddThread(&DrawObjects, 0, "draw");
    G8RTOS_AddThread(&ReadJoystickHost, 0, "joystick");
    G8RTOS_AddThread(&SendDataToClient, 0, "send data");
    G8RTOS_AddThread(&ReceiveDataFromClient, 0, "rcv data");
    G8RTOS_AddThread(&MoveLEDs, 0, "leds");
    G8RTOS_AddThread(&IdleThread, 255, "idle");

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
        // TODO - Fill packet for client

        // TODO - Send packet

        // TODO - Check if game is done

        // TODO - If done, Add EndOfGameHost thread with highest priority

        // Sleep for 5ms (found experimentally to be a good amount of time for synchronization)
        G8RTOS_Sleep(5);
    }
}

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient()
{
    while (1)
    {
        // TODO - Continually receive data until a return value greater than zero is returned (meaning valid data has been read)

            // Note: Remember to release and take the semaphore again so you've still able to send data

            // Sleeping here for 1ms would avoid a deadlock
            G8RTOS_Sleep(1);

        // TODO - Update the player's current center with the displacement received from the client

        // Sleep for 2ms (again found experimentally)
        G8RTOS_Sleep(2);
    }
}

/*
 * Generate Ball thread
 */
void GenerateBall()
{
    while (1)
    {
        // TODO - Adds another MoveBall thread if the number of balls is less than the max

        // TODO - Sleeps proportional to the number of balls currently in play
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

        // TODO - Change Self.displacement accordingly (you can experiment with how much you want to scale the ADC value)
        // Add Displacement to Self accordingly
        G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
        clientInfo.displacement = js_x_data;
        G8RTOS_SignalSemaphore(&SpecificPlayerInfo_Mutex);

        // Sleep for 10ms
        G8RTOS_Sleep(10);

        // TODO - Then add the displacement to the bottom player in the list of players (general list that鈥檚 sent to the client and used for drawing) i.e. players[0].position += self.displacement

        // TODO - By sleeping before updating the bottom player's position, it makes the game more fair between client and host
    }
}

/*
 * Thread to move a single ball
 */
void MoveBall()
{
    // TODO - Go through array of balls and find one that's not alive

    // TODO - Once found, initialize random position and X and Y velocities, as well as color and alive attributes

    while (1)
    {
        // TODO - Checking for collision given the current center and the velocity

        // TODO - If collision occurs, adjust velocity and color accordingly

        // TODO - If the ball passes the boundary edge, adjust score, account for the game possibly ending, and kill self

        // TODO - Otherwise, just move the ball in its current direction according to its velocity

        // Sleep for 35ms
        G8RTOS_Sleep(35);
    }
}

/*
 * End of game for the host
 */
void EndOfGameHost()
{
    // TODO - Wait for all the semaphores to be released

    // Kill all other threads
    G8RTOS_KillAllOtherThreads();

    // TODO - Re-initialize semaphores

    // TODO - Clear screen with the winner's color

    // TODO - Print some message that waits for the host's action to start a new game

    // TODO - Create an aperiodic thread that waits for the host's button press (the client will just be waiting on the host to start a new game)

    // TODO - Wait for button press event

    // TODO - Send notification to client

    // TODO - Reinitialize the game and objects and add back all the threads

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

            prevBalls[i].alive=tempGameState.balls[i].alive;
        }

        // Update players
        for(int i = 0; i < MAX_NUM_OF_PLAYERS; ++i)
        {
            UpdatePlayerOnScreen(&prevPlayers[i], &(tempGameState.players[i]));
        }

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

    // TODO - Write message on screen assisting player choice of Host vs. Client

    playerType role = GetPlayerRole();

    if (role == Client) G8RTOS_AddThread(&JoinGame, 0, "join");
    else G8RTOS_AddThread(&CreateGame, 0, "create");

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

    prevPlayerIn->Center=outPlayer->currentCenter;
}

/*
 * Draw a new ball on screen
 */
void DrawBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall)
{
    G8RTOS_WaitSemaphore(&LCD_Mutex);
    // Draw the new ball
    LCD_DrawRectangle(currentBall->currentCenterX - BALL_SIZE_D2,
                      currentBall->currentCenterX + BALL_SIZE_D2,
                      currentBall->currentCenterX - BALL_SIZE_D2,
                      currentBall->currentCenterX + BALL_SIZE_D2,
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
    LCD_DrawRectangle(previousBall->CenterX - BALL_SIZE_D2,
                      previousBall->CenterX + BALL_SIZE_D2,
                      previousBall->CenterY - BALL_SIZE_D2,
                      previousBall->CenterY + BALL_SIZE_D2,
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

    // Draw the new ball
    DrawBallOnScreen(previousBall, currentBall);
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
    for(int i = 0; i < gameState.LEDScores[0]; i++) player0LEDScore |= (1<<i);
    for(int i = 0; i < gameState.LEDScores[1]; i++) player1LEDScore |= (1<<i);

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
    for(int i=ARENA_MIN_Y; i<=ARENA_MAX_Y; i++){
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
/*********************************************** Public Functions *********************************************************************/
