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

    // TODO - Set initial SpecificPlayerInfo_t strict attributes (you can get the IP address by calling getLocalIP()
    SpecificPlayerInfo_t tempClientInfo={
                CONFIG_IP,     //IP Address
                0,             //displacement
                BOTTOM,        //playerNumber
                1,             //ready
                0,             //joined
                0              //acknowledge
    };
    G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
    clientInfo=tempClientInfo;
    G8RTOS_SignalSemaphore(&SpecificPlayerInfo_Mutex);

    // TODO - Send player info to the host
    G8RTOS_WaitSemaphore(&WiFi_Mutex);
    SendData((uint8_t*)(&clientInfo), HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t)/sizeof(uint8_t));
    G8RTOS_SignalSemaphore(&WiFi_Mutex);

    // TODO - Wait for server response
    GameState_t tempGameState;
    G8RTOS_WaitSemaphore(&WiFi_Mutex);
    while(ReceiveData((uint8_t*)(&tempGameState), sizeof(GameState_t)/sizeof(uint8_t))==NOTHING_RECEIVED);
    G8RTOS_SignalSemaphore(&WiFi_Mutex);

    // TODO - Empty the received packet
    G8RTOS_WaitSemaphore(&GameState_Mutex);
    gameState=tempGameState;
    G8RTOS_SignalSemaphore(&GameState_Mutex);

    // TODO - If you've joined the game, acknowledge you've joined to the host and show connection with an LED
    if(gameState.player.joined==1){
        //update local client info
        G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
        clientInfo=gameState.player;
        clientInfo.acknowledge=1;
        G8RTOS_SignalSemaphore(&SpecificPlayerInfo_Mutex);
        //send acknowledgment
        G8RTOS_WaitSemaphore(&WiFi_Mutex);
        SendData((uint8_t*)(&clientInfo), HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t)/sizeof(uint8_t));
        G8RTOS_SignalSemaphore(&WiFi_Mutex);
        //Update LED to show connection
        // Blue LED = Connection Established
        G8RTOS_WaitSemaphore(&LED_Mutex);
        LP3943_LedModeSet(RED, 0);
        LP3943_LedModeSet(BLUE, BLUE_LED);
        G8RTOS_SignalSemaphore(&LED_Mutex);
    }
    else{
        G8RTOS_AddThread(&HostVsClient, 0, "host vs client");
    }

    // TODO - Initialize the board state


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
       // TODO - Continually receive data until a return value greater than zero is returned (meaning valid data has been read)
       // Note: Remember to release and take the semaphore again so you've still able to send data
       GameState_t tempGameState;
       while(1){
           G8RTOS_WaitSemaphore(&WiFi_Mutex);
           _i32 retVal=ReceiveData((uint8_t*)(&tempGameState), sizeof(GameState_t)/sizeof(uint8_t));
           G8RTOS_SignalSemaphore(&WiFi_Mutex);
           if(retVal==SUCCESS){
               break;
           }
           // Sleeping here for 1ms would avoid a deadlock
           G8RTOS_Sleep(1);
       }

       // TODO - Empty the received packet
       G8RTOS_WaitSemaphore(&GameState_Mutex);
       gameState=tempGameState;
       G8RTOS_SignalSemaphore(&GameState_Mutex);

       // TODO - If the game is done, add EndOfGameClient thread with the highest priority
       if(gameState.gameDone==1){
           G8RTOS_AddThread(&EndOfGameClient, 0, "End Client");
       }

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
        // TODO - Send player info
        // Get player info
        G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
        SpecificPlayerInfo_t tempClientInfo=clientInfo;
        G8RTOS_SignalSemaphore(&SpecificPlayerInfo_Mutex);

        // Send data
        G8RTOS_WaitSemaphore(&WiFi_Mutex);
        SendData((uint8_t*)(&clientInfo), HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t)/sizeof(uint8_t));
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
   // TODO - Determine joystick bias (found experimentally) since every joystick is offset by some small amount displacement and noise

   while (1)
   {
       // TODO - Read joystick and add bias
       s16 js_x_data, js_y_data;
       GetJoystickCoordinates(&js_x_data, &js_y_data);

       // TODO - Add Displacement to Self accordingly
       G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
       clientInfo.displacement=js_x_data+JOYSTICK_BIAS;
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
    // TODO - Wait for all semaphores to be released
    G8RTOS_WaitSemaphore(&LED_Mutex);
    G8RTOS_WaitSemaphore(&LCD_Mutex);
    G8RTOS_WaitSemaphore(&WiFi_Mutex);
    G8RTOS_WaitSemaphore(&SpecificPlayerInfo_Mutex);
    G8RTOS_WaitSemaphore(&GameState_Mutex);

    // Kill all other threads
    G8RTOS_KillAllOtherThreads();

    // TODO - Re-initialize semaphores
    G8RTOS_InitSemaphore(&LED_Mutex, 1);
    G8RTOS_InitSemaphore(&LCD_Mutex, 1);
    G8RTOS_InitSemaphore(&WiFi_Mutex, 1);
    G8RTOS_InitSemaphore(&SpecificPlayerInfo_Mutex, 1);
    G8RTOS_InitSemaphore(&GameState_Mutex, 1);

    // TODO - Clear screen with winner's color
    if(gameState.winner==TOP){
        LCD_Clear(PLAYER_BLUE);
    }
    else{
        LCD_Clear(PLAYER_RED);
    }

    // TODO - Wait for host to restart game
    while(gameState.gameDone==1){
        // TODO - Wait for server response
        GameState_t tempGameState;
        G8RTOS_WaitSemaphore(&WiFi_Mutex);
        while(ReceiveData((uint8_t*)(&tempGameState), sizeof(GameState_t)/sizeof(uint8_t))==NOTHING_RECEIVED);
        G8RTOS_SignalSemaphore(&WiFi_Mutex);
        // TODO - Empty the received packet
        G8RTOS_WaitSemaphore(&GameState_Mutex);
        gameState=tempGameState;
        G8RTOS_SignalSemaphore(&GameState_Mutex);
    }

    // TODO - Add all threads back and restart game variables
    // TODO - Initialize the board state


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
    // TODO - Determine joystick bias (found experimentally) since every joystick is offset by some small amount displacement and noise

    while (1)
    {
        // TODO - Read the joystick ADC values by calling GetJoystickCoordinates, applying previously determined bias

        // TODO - Change Self.displacement accordingly (you can experiment with how much you want to scale the ADC value)

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
    // TODO - Declare array of previous players and ball positions

    while (1)
    {
        // TODO - Draw and/or update balls (you'll need a way to tell whether to draw a new ball, or update its position (i.e. if a new ball has just been created - hence the alive attribute in the Ball_t struct.

        // TODO - Update players

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
        // TODO - Responsible for updating the LED array with current scores

        // TODO - Sleep for ???
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

    LCD_Clear(BACK_COLOR);

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
void DrawPlayer(GeneralPlayerInfo_t * player)
{
   // TODO
}

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer)
{
   // TODO
}

/*
 * Function updates ball position on screen
 */
void UpdateBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall, uint16_t outColor)
{
   // TODO
}

/*
 * Initializes and prints initial game state
 */
void InitBoardState()
{
   // TODO
}

/*********************************************** Public Functions *********************************************************************/
