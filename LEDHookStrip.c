/**
 * Copyright (c) 2026 by 6Bolt
 *
 *
 */

//--------------------------------------------------------------------+
// Library
//--------------------------------------------------------------------+

// STD Libs
#include <stdio.h>
#include <stdlib.h>
#include <pico/stdio.h>
#include "pico/stdlib.h"

// For the PIO Stuff
#include "hardware/pio.h"
#include "hardware/clocks.h"
// PIO Program for WS2812, or Neopixel Interface
//#include "ws2812.pio.h"

// Multi-Core
#include "pico/multicore.h"
#include "pico/sync.h"

// For the USB Stuff
#include "bsp/board_api.h"
#include "tusb.h"

//For Reset
#include "hardware/watchdog.h"


// Global
#include "globals.h"

// Display Functions
#include "displayFunctions.h"

//--------------------------------------------------------------------+
// Globals Dec
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// Globals - PIO
//--------------------------------------------------------------------+

// For the PIO - There is 3 PIO Units, Each Unit having 4 PIO PSM Selected by the sm
// 4 pio0s, 4 pio1s, and 4 pio2s, to make 12
PIO *gp_pio;
// State Machine Picks What PIO PSM in the Init Above - 0 to 3
uint g_sm[LEDSTRINGSMAX] = {0, 1, 2, 3};
// PIO Offset
uint g_offset[LEDSTRINGSMAX];

// Pins for the LED Strings and the PIO
uint g_pioPins[LEDSTRINGSMAX] = {WS2812_PIN_0, WS2812_PIN_1, WS2812_PIN_2, WS2812_PIN_3};

// Does the LED String Have a White LED for 32 Writes, instead of just 24 Writes for just RGB
bool g_isRGBW = IS_RGBW;

// Success Bool for the PIO
bool g_success[LEDSTRINGSMAX] = {false, false, false, false};

// Number of Strings on the 2 Cores
uint8_t g_stringsCore0;
uint8_t g_stringsCore1;

//--------------------------------------------------------------------+
// Globals - Setting Up LED Strings Mutexs
//--------------------------------------------------------------------+

mutex_t g_mutexStrip[LEDSTRINGSMAX];
//mutex_t g_mutexDRData[LEDSTRINGSMAX];

//--------------------------------------------------------------------+
// Globals - Setting Up LED Strings
//--------------------------------------------------------------------+

// Setting Up the LED Strings
// To Know when The LED Strings Have Been Set-up, Run LEDSetUp Until it is True
bool g_isLEDStringsInited = false;
// To Know how Many LED Strings there Are
volatile uint8_t g_ledStringNumber;
// To Know How Many Eleemts are in Each String
uint16_t *gp_ledStringElements = NULL;
// Count To Keep Track of What LED String Info the Program is Recieving
uint8_t g_elementCount = 0;
// Is a Command Running on the LED String
volatile bool g_isRunning[LEDSTRINGSMAX] = {false, false, false, false};

//--------------------------------------------------------------------+
// Globals - PIO Tx FIFO Buffer Struct
//--------------------------------------------------------------------+

// Struct Used to Write to the Tx FIFO Buffer
TxDataWrite *gp_txData = NULL;

//--------------------------------------------------------------------+
// Globals - Setting Up Display Range
//--------------------------------------------------------------------+

// Display Range Infor and Set-up
// Stores All The Display Range Info, Per LED String(s)
DisplayRange *gp_displayRangeInfo = NULL;
// Count to Keep Track on Setting Up Display Range
uint8_t g_displayRangeCount = 0;
// When Entering Display Range Data
bool g_displayRangeActive = false;
// When Range Display is Running on a LED String
volatile bool g_isRDRunning[LEDSTRINGSMAX] = {false, false, false, false};


//--------------------------------------------------------------------+
// Globals - Setting Up Flash
//--------------------------------------------------------------------+

// Flash Data in the Struct
FlashData *gp_flashData = NULL;
// Count to Keep Track on Setting Flash
uint8_t g_flashCount = 0;
// When Entering Display Range Data
bool g_flashActive = false;

//--------------------------------------------------------------------+
// Globals - Setting Up Random Flash
//--------------------------------------------------------------------+

// Flash Data in the Struct
RndFlashData *gp_rndFlashData = NULL;
// Count to Keep Track on Setting Flash
uint8_t g_rndFlashCount = 0;
// When Entering Display Range Data
bool g_rndFlashActive = false;
// When 2nd Color is Enabled
bool g_rndFlash2ndColorActive = false;

//--------------------------------------------------------------------+
// Globals - Setting Up Sequential
//--------------------------------------------------------------------+

// Flash Data in the Struct
SeqData *gp_seqData = NULL;
// Count to Keep Track on Setting Flash
uint8_t g_seqCount = 0;
// When Entering Display Range Data
bool g_seqActive = false;

//--------------------------------------------------------------------+
// Globals - Command Buffer
//--------------------------------------------------------------------+

volatile uint8_t g_cmdBufferData[CMDBUFFERSIZE];
volatile uint8_t g_cmdBufferCount = 0;
volatile uint8_t g_cmdBufferEXE = 0;

volatile uint8_t g_core0CMD = 0;
volatile uint8_t g_core1CMD = 0;

//--------------------------------------------------------------------+
// Globals - Setting Up Color
//--------------------------------------------------------------------+

// Keeps Trak of the Color(s) When they are Recieved
uint8_t g_redCount = 0;
uint8_t g_greenCount = 0;
uint8_t g_blueCount = 0;

//--------------------------------------------------------------------+
// Globals - Setting Up Pattern when Not in Game
//--------------------------------------------------------------------+

// To Run a LED Strip Pattern whe Not in a Game
volatile bool g_ledPattern = false;

// Run the LED Pattern if Not In a Game
volatile bool g_runLEDPattern = false;

// To Know What LED Pattern to Run
volatile uint8_t g_ledPatternNumber = 0;

// Pattern T number
volatile int g_tNumber = 0;

//--------------------------------------------------------------------+
// Globals - Setting Up In Game
//--------------------------------------------------------------------+

// To Know if a Supported Game is Being Played or Not
volatile bool g_inGame;

// To know when Data is Available on the CDC Interface
volatile bool g_DataOnCDC = false;



//--------------------------------------------------------------------+
// Functions
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// Random Patterns - When Not in a Game
//--------------------------------------------------------------------+

void pattern_snakes(PIO pio, uint sm, uint len, uint t)
{
    for (uint i = 0; i < len; ++i)
    {
        uint x = (i + (t >> 1)) % 64;
        if (x < 10)
            pio_sm_put_blocking(pio, sm, SetColor(0xff, 0, 0, 0));
        else if (x >= 15 && x < 25)
            pio_sm_put_blocking(pio, sm, SetColor(0, 0xff, 0, 0));
        else if (x >= 30 && x < 40)
            pio_sm_put_blocking(pio, sm, SetColor(0, 0, 0xff, 0));
        else
            pio_sm_put_blocking(pio, sm, 0);
    }
}

void pattern_random(PIO pio, uint sm, uint len, uint t)
{
    if (t % 8)
        return;
    for (uint i = 0; i < len; ++i)
        pio_sm_put_blocking(pio, sm, rand());
}

void pattern_sparkle(PIO pio, uint sm, uint len, uint t)
{
    if (t % 8)
        return;
    for (uint i = 0; i < len; ++i)
        pio_sm_put_blocking(pio, sm, rand() % 16 ? 0 : 0xffffffff);
}

void pattern_greys(PIO pio, uint sm, uint len, uint t)
{
    uint max = 100; // let's not draw too much current!
    t %= max;
    for (uint i = 0; i < len; ++i)
    {
        pio_sm_put_blocking(pio, sm, t * 0x10101);
        if (++t >= max)
            t = 0;
    }
}

typedef void (*pattern)(PIO pio, uint sm, uint len, uint t);
const struct
{
    pattern pat;
    const char *name;
} pattern_table[] = {
    {pattern_snakes, "Snakes!"},
    {pattern_random, "Random data"},
    {pattern_sparkle, "Sparkles"},
    {pattern_greys, "Greys"},
};

// Takes Char Number and Returns Tens (Number X 10)
uint8_t tensMult(char ten)
{
    uint8_t tens = (ten - '0');
    tens = (tens << 3) + (tens << 1);
    return tens;
}

// Takes Char Number and Returns Hundrers (Number X 100)
uint16_t hundredsMult(char hund)
{
    uint16_t hundreds = (hund - '0');
    hundreds = (hundreds << 6) + (hundreds << 5) + (hundreds << 2);
    return hundreds;
}

// Takes Char Number and Returns Hundrers (Number X 100 X 10)
uint16_t thousandsMult(char thou)
{
    uint16_t thousands = (thou - '0');
    thousands = (thousands << 6) + (thousands << 5) + (thousands << 2);
    thousands += (thousands << 3) + (thousands << 1);
    return thousands;
}

//--------------------------------------------------------------------+
// Add Struct Number to Command Buffer
//--------------------------------------------------------------------+
void AddCommandBuffer(uint8_t structN)
{
    for (uint8_t i = 0; i < CMDBUFFERSIZE; i++)
    {
        if (structN == g_cmdBufferData[i])
        {
            // Already in Command Buffer, Do Nothing
            return;
        }
    }

    g_cmdBufferData[g_cmdBufferCount] = structN;
    g_cmdBufferCount++;

    if (g_cmdBufferCount >= CMDBUFFERSIZE)
        g_cmdBufferCount = 0;
}










//--------------------------------------------------------------------+
// Functions - Setup LED Strings Beginning
//--------------------------------------------------------------------+

// Function to Set-up the LED Strings
void SetUpLEDStrings(char buf[], uint16_t count);

//--------------------------------------------------------------------+
// Functions - Set Display Data When Not In Game
//--------------------------------------------------------------------+

// Functions That Handles Collecting Data for Display Commands
void NoGame(char buf[], uint16_t count);

//--------------------------------------------------------------------+
// Functions - When In a Game
//--------------------------------------------------------------------+

// Function that Handles Display Commands
void GameRunning(char buf[], uint16_t count);

// When a Game has Ended and Free Up the Memory and Sets Globals to No Game State
void GameEnded(void);

// Checks if Data is Available on Serial Port
void cdc_task(void);




//--------------------------------------------------------------------+
// Core1 Main Loop
//--------------------------------------------------------------------+

// Core1 will drive LED Strip 0 and 2, which is the  first and third LED Strings
// It will use the Multi Core FIFO to communicate with the other Core
// 0 Will Be Update the LED String of 0 or 2
// 1 Will Do a Flash on a Struct 0-9
// 2 Will Do a Random Flash on a Struct 0-9
// 3 Will Do a Sequential on a Struct 0-9
// 10+ Will Do a Pattern (Num - 10), for not in a Game

void core1_mainLoop()
{
    static uint16_t patCount1 = 0;
    static bool patCountTrip1 = true;
    static int dir1 = 1;
    static bool patRan = false;
    static bool inGameCore2 = false;

    while (1)
    {
        if (inGameCore2)
        {            
            if (multicore_fifo_rvalid())
            {
                uint8_t func = multicore_fifo_pop_blocking();

                if (func >= 0 && func < 2)
                {
                    if (func == UPDATEDISPLAYRNUM)
                    {
                        uint8_t string = multicore_fifo_pop_blocking();
                        uint16_t updateNum = multicore_fifo_pop_blocking();
                        UpdateDisplayNumber(string, updateNum);
                    }
                    else
                    {
                        uint8_t structNum = multicore_fifo_pop_blocking();
                        DoFlash(structNum);
                    }
                }
                else if (func > 1 && func < 4)
                {
                    if (func == DOARNDFLASH)
                    {
                        uint8_t structNum = multicore_fifo_pop_blocking();
                        DoRndFlash(structNum);
                    }
                    else
                    {
                        uint8_t structNum = multicore_fifo_pop_blocking();
                        DoSeq(structNum);
                    }
                }
                else if (func == EXITOUTCMD)
                {
                    InitLEDDisplay(0);
                    if (g_ledStringNumber > 2)
                        InitLEDDisplay(2);

                    inGameCore2 = false;
                }
                else if (func == CLEARSTRIP)
                {
                    InitLEDDisplay(0);
                    if (g_ledStringNumber > 2)
                        InitLEDDisplay(2);
                }
            }

            
            //Check if Data Changed on Display Range
            uint8_t indexS = 0;
            for(uint8_t i = 0; i < g_stringsCore1; i++)
            {
                if(gp_displayRangeInfo[indexS].dataChanged)
                {
                    if (!g_isRunning[indexS])
                    {
                        uint32_t mOwner;
                        if(mutex_try_enter(&g_mutexStrip[indexS], &mOwner))
                        {
                            g_isRunning[indexS] = true;
                            mutex_exit(&g_mutexStrip[indexS]);
                            DisplayDataOnLEDs(indexS);
                        }
                    }
                }
                indexS += 2;
            }
            
        }
        else
        {
            if (multicore_fifo_rvalid())
            {
                uint8_t func = multicore_fifo_pop_blocking();

                if (func == GAMESTARTED)
                {
                    inGameCore2 = true;
                    g_runLEDPattern = false;
                }
            }

            if (g_runLEDPattern)
            {
                patRan = true;
                if (patCountTrip1)
                {
                    dir1 = (rand() >> 30) & 1 ? 1 : -1;
                    patCountTrip1 = false;
                }
                
                for (int i = 0; i < 20; ++i)
                {
                    pattern_table[g_ledPatternNumber].pat(gp_pio[0], g_sm[0], gp_ledStringElements[0], g_tNumber);
                    if (g_ledStringNumber > 2)
                        pattern_table[g_ledPatternNumber].pat(gp_pio[2], g_sm[2], gp_ledStringElements[2], g_tNumber);
                    sleep_ms(10);
                    g_tNumber += dir1;
                    patCount1++;

                    if (patCount1 == LEDPATTERNMAX)
                        patCountTrip1 = true;
                }
            }
            else if (patRan)
            {
                InitLEDDisplay(0);
                if (g_ledStringNumber > 2)
                    InitLEDDisplay(2);

                patRan = false;
            }

            tud_task(); // tinyusb device task, to get CDC Data
        }
    }
}

//--------------------------------------------------------------------+
// Main
//--------------------------------------------------------------------+

int main()
{
    // Set up Globals
    g_ledStringNumber = 0;
    g_isLEDStringsInited = false;
    g_inGame = false;
    g_cmdBufferCount = 0;
    g_cmdBufferEXE = 0;

    // Clear Out Command Buffer
    for (uint8_t i = 0; i < CMDBUFFERSIZE; i++)
        g_cmdBufferData[i] = UNASSIGN;

    // Setup gp_pio for an Array, Old School C Way
    gp_pio = (PIO *)malloc(LEDSTRINGSMAX * sizeof(PIO));
    gp_pio[0] = pio0;
    gp_pio[1] = pio0;
    gp_pio[2] = pio0;
    gp_pio[3] = pio0;

    static uint16_t patCount = 0;
    static bool patCountTrip = true;
    static int dir = 1;

    // Initialize TinyUSB stack
    board_init();

    // init device stack on configured roothub port
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_HIGH
        //.speed = TUSB_SPEED_AUTO
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    stdio_init_all();

    // Initialize the mutex
    for (uint8_t i = 0; i < LEDSTRINGSMAX; i++)
    {
        mutex_init(&g_mutexStrip[i]);
        //mutex_init(&g_mutexDRData[i]);
    }

    // Setup Core1
    multicore_launch_core1(core1_mainLoop);

    // TinyUSB board init callback after init
    if (board_init_after_tusb)
    {
        board_init_after_tusb();
    }

    
    while (true)
    {
        tud_task(); // tinyusb device task, to get CDC Data
        cdc_task(); // Checks for Read Data on Serial Port

        if (g_runLEDPattern && g_ledStringNumber > 1)
        {
            if (patCountTrip)
            {
                dir = (rand() >> 30) & 1 ? 1 : -1;
                patCountTrip = false;
            }
            
            for (int j = 0; j < 5; ++j)
            {

                pattern_table[g_ledPatternNumber].pat(gp_pio[1], g_sm[1], gp_ledStringElements[1], g_tNumber);
                if (g_ledStringNumber > 3)
                    pattern_table[g_ledPatternNumber].pat(gp_pio[3], g_sm[3], gp_ledStringElements[3], g_tNumber);
                sleep_ms(10);
                g_tNumber += dir;
                patCount++;

                if (patCount == LEDPATTERNMAX)
                    patCountTrip = true;
            }
        }
    }

    // Take Back PIO State Machine
    DeinitLEDStrings();
}

//--------------------------------------------------------------------+
// USB CDC - the Virtual Serial Port
//--------------------------------------------------------------------+

// Function to Set-up the LED Strings
void SetUpLEDStrings(char buf[], uint16_t count)
{
    // read data
    uint16_t index = 0;
    uint16_t indexEnd = 2;

    while (indexEnd < count)
    {
        switch (buf[index])
        {
        case LEDSTRINGNUMBER:  // Number of ALED Strings
            if (buf[indexEnd] == ENDCHAR)
            {
                uint8_t numStrings = buf[index + 1] - '0';
                NumberLEDStrings(numStrings);
            }

            break;
        case LEDSTRINGELEMENTS:   // Number of LEDs in String
            uint16_t elementCount;
            if (buf[indexEnd] == ENDCHAR)
            {
                elementCount = buf[index + 1] - '0';
            }
            else if (buf[indexEnd + 1] == ENDCHAR)
            {
                uint8_t tens = tensMult(buf[index + 1]);
                tens += buf[index + 2] - '0';
                elementCount = tens;
                indexEnd++;
            }
            else if (buf[indexEnd + 2] == ENDCHAR)
            {
                uint16_t hundreds = hundredsMult(buf[index + 1]);
                hundreds += tensMult(buf[index + 2]);
                hundreds += buf[index + 3] - '0';
                elementCount = hundreds;
                indexEnd += 2;
            }
            gp_ledStringElements[g_elementCount] = elementCount;
            g_elementCount++;

            if (g_elementCount == g_ledStringNumber)
            {
                InitLEDStrings();
                g_isLEDStringsInited = true;
            }

            break;

        case HOTRCHECK:  // Check to Make Sure it is a HOTR ALED Strip Controller, and that String Set Up has not been Done

            if (buf[index + 1] == ENDCHAR && buf[indexEnd] == ENDCHAR)
            {
                // Checking if this is a HOTR ALED Strip Controller
                // tud_cdc_read
                const char *chkMsg = "69DE";
                tud_cdc_write(chkMsg, sizeof(chkMsg));
                tud_task();
                tud_cdc_write_flush();
            }

            break;
        }

        index = indexEnd + 1;
        indexEnd = index + 2;
    }
}

void NoGame(char buf[], uint16_t count)
{
    // read data
    uint16_t index = 0;
    uint16_t indexEnd = 2;

    while (indexEnd < count)
    {
        switch (buf[index])
        {
        case DISPLAYRANGE:  // Starts Display Range for Certain ALED String
            if (buf[indexEnd] == ENDCHAR)
            {
                gp_displayRangeInfo[g_displayRangeCount].stringNumber = buf[index + 1] - '0';
                g_displayRangeActive = true;
            }
            break;
        case DISPLAYRANGEMAX:   // Max Count Number for Display Range
            uint16_t maxCount;
            if (buf[indexEnd] == ENDCHAR)
            {
                maxCount = buf[index + 1] - '0';
            }
            else if (buf[indexEnd + 1] == ENDCHAR)
            {
                uint8_t tens = tensMult(buf[index + 1]);
                tens += buf[index + 2] - '0';
                maxCount = tens;
                indexEnd++;
            }
            else if (buf[indexEnd + 2] == ENDCHAR)
            {
                uint16_t hundreds = hundredsMult(buf[index + 1]);
                hundreds += tensMult(buf[index + 2]);
                hundreds += buf[index + 3] - '0';
                maxCount = hundreds;
                indexEnd += 2;
            }

            uint8_t stringNum = gp_displayRangeInfo[g_displayRangeCount].stringNumber;
            gp_displayRangeInfo[g_displayRangeCount].maxRange = maxCount;
            gp_displayRangeInfo[g_displayRangeCount].lightsPerUnit = (float)gp_ledStringElements[stringNum] / (float)maxCount;
            gp_displayRangeInfo[g_displayRangeCount].p_lightsDisplay = (uint16_t *)malloc((maxCount + 1) * sizeof(uint16_t));

            // Set the Beginning to 0 and End to LED String Elements
            gp_displayRangeInfo[g_displayRangeCount].p_lightsDisplay[0] = 0;
            gp_displayRangeInfo[g_displayRangeCount].p_lightsDisplay[maxCount] = gp_ledStringElements[stringNum];

            // Then Fill the In Between
            for (uint16_t i = 1; i < maxCount; i++)
                gp_displayRangeInfo[g_displayRangeCount].p_lightsDisplay[i] = gp_displayRangeInfo[g_displayRangeCount].lightsPerUnit * i;

            break;
        case STEPNUMBERS:   // Number of Steps in Display Range
            if (buf[indexEnd] == ENDCHAR)
            {
                uint16_t lightsStep;
                float lightsStepFloat;
                uint8_t steps = buf[index + 1] - '0';
                uint8_t stringNum = gp_displayRangeInfo[g_displayRangeCount].stringNumber;
                gp_displayRangeInfo[g_displayRangeCount].numberOfSteps = steps;

                if (steps == 0)
                    steps = 1;

                if (steps == 1)
                    lightsStep = gp_ledStringElements[stringNum];
                else
                {
                    lightsStep = gp_ledStringElements[stringNum] / steps;
                    lightsStepFloat = (float)gp_ledStringElements[stringNum] / (float)steps;
                }

                gp_displayRangeInfo[g_displayRangeCount].p_lightsInStep = (uint16_t *)malloc(steps * sizeof(uint16_t));
                gp_displayRangeInfo[g_displayRangeCount].p_redSteps = (uint8_t *)malloc(steps * sizeof(uint8_t));
                gp_displayRangeInfo[g_displayRangeCount].p_blueSteps = (uint8_t *)malloc(steps * sizeof(uint8_t));
                gp_displayRangeInfo[g_displayRangeCount].p_greenSteps = (uint8_t *)malloc(steps * sizeof(uint8_t));

                if (steps > 1)
                {
                    for (size_t i = 0; i < steps - 1; i++)
                        gp_displayRangeInfo[g_displayRangeCount].p_lightsInStep[i] = (uint16_t)(lightsStepFloat * (i + 1));
                }

                gp_displayRangeInfo[g_displayRangeCount].p_lightsInStep[steps - 1] = gp_ledStringElements[stringNum] + 1;
            }
            break;

        case RELOADSEQ:  // If Reload Sequence is Enable or Not
            if (buf[indexEnd] == ENDCHAR)
            {
                uint8_t enableRS = buf[index + 1] - '0';

                if (enableRS == 0)
                    gp_displayRangeInfo[g_displayRangeCount].enableReload = false;
                else
                    gp_displayRangeInfo[g_displayRangeCount].enableReload = true;
            }
            break;

        case RELOADNUMLEDS:   // Number of LEDs to light up for 1 Sequence of Reload
            if (buf[indexEnd] == ENDCHAR)
            {
                uint8_t numberLEDs = buf[index + 1] - '0';

                //Can only be 1, 2, 4, or 8
                if (numberLEDs == 1 || numberLEDs == 2 || numberLEDs == 4 || numberLEDs == 8)
                {
                    if(g_displayRangeActive)
                        gp_displayRangeInfo[g_displayRangeCount].numLEDs = numberLEDs;
                    else if(g_seqActive)
                        gp_seqData[g_seqCount].numLEDs = numberLEDs;
                }
                
            }
            break;


        // The 3 Primary Colors RGB
        case REDCOLOR:
        case GREENCOLOR:   // Colors for All the Display Functions
        case BLUECOLOR:    // Blue is the Last for All Display Functions

            uint8_t colorVal;
            if (buf[indexEnd] == ENDCHAR)
            {
                colorVal = buf[index + 1] - '0';
            }
            else if (buf[indexEnd + 1] == ENDCHAR)
            {
                uint8_t tens = tensMult(buf[index + 1]);
                tens += buf[index + 2] - '0';
                colorVal = tens;
                indexEnd++;
            }
            else if (buf[indexEnd + 2] == ENDCHAR)
            {
                uint16_t hundreds = hundredsMult(buf[index + 1]);
                hundreds += tensMult(buf[index + 2]);
                hundreds += buf[index + 3] - '0';
                colorVal = hundreds;
                indexEnd += 2;
            }

            if (buf[index] == REDCOLOR)
            {
                if(g_displayRangeActive)
                {
                    gp_displayRangeInfo[g_displayRangeCount].p_redSteps[g_redCount] = colorVal;

                    if ((g_redCount + 1) == gp_displayRangeInfo[g_displayRangeCount].numberOfSteps)
                        g_redCount = 0;
                    else
                        g_redCount++;
                }
                else if (g_flashActive)
                    gp_flashData[g_flashCount].red = colorVal;
                else if (g_rndFlashActive)
                {
                    if(g_rndFlash2ndColorActive)
                        gp_rndFlashData[g_rndFlashCount].red2 = colorVal;
                    else
                        gp_rndFlashData[g_rndFlashCount].red = colorVal;
                }
                else if (g_seqActive)
                    gp_seqData[g_seqCount].red = colorVal;
            }
            else if (buf[index] == GREENCOLOR)
            {
                if (g_displayRangeActive)
                {
                    gp_displayRangeInfo[g_displayRangeCount].p_greenSteps[g_greenCount] = colorVal;

                    if ((g_greenCount + 1) == gp_displayRangeInfo[g_displayRangeCount].numberOfSteps)
                        g_greenCount = 0;
                    else
                        g_greenCount++;
                }
                else if (g_flashActive)
                    gp_flashData[g_flashCount].green = colorVal;
                else if (g_rndFlashActive)
                {
                    if(g_rndFlash2ndColorActive)
                        gp_rndFlashData[g_rndFlashCount].green2 = colorVal;
                    else
                        gp_rndFlashData[g_rndFlashCount].green = colorVal;
                }
                else if (g_seqActive)
                    gp_seqData[g_seqCount].green = colorVal;
            }
            else if (buf[index] == BLUECOLOR)
            {

                if (g_displayRangeActive)
                {
                    gp_displayRangeInfo[g_displayRangeCount].p_blueSteps[g_blueCount] = colorVal;
                    if ((g_blueCount + 1) == gp_displayRangeInfo[g_displayRangeCount].numberOfSteps)
                    {
                        g_blueCount = 0;
                        gp_displayRangeInfo[g_displayRangeCount].isComplete = true;
                        g_isRDRunning[gp_displayRangeInfo[g_displayRangeCount].stringNumber] = true;
                        g_displayRangeCount++;
                        g_displayRangeActive = false;
                    }
                    else
                    {
                        g_blueCount++;
                    }
                }
                else if (g_flashActive)
                {
                    gp_flashData[g_flashCount].blue = colorVal;
                    gp_flashData[g_flashCount].color = SetColor(gp_flashData[g_flashCount].red, gp_flashData[g_flashCount].green, gp_flashData[g_flashCount].blue, 0);
                    gp_flashData[g_flashCount].isComplete = true;
                    g_flashCount++;
                    g_flashActive = false;
                }
                else if (g_rndFlashActive)
                {
                    if(g_rndFlash2ndColorActive)
                    {
                        gp_rndFlashData[g_rndFlashCount].blue2 = colorVal;
                        gp_rndFlashData[g_rndFlashCount].color2 = SetColor(gp_rndFlashData[g_rndFlashCount].red2, gp_rndFlashData[g_rndFlashCount].green2, gp_rndFlashData[g_rndFlashCount].blue2, 0);
                        g_rndFlash2ndColorActive = false;
                    }
                    else
                    {
                        gp_rndFlashData[g_rndFlashCount].blue = colorVal;
                        gp_rndFlashData[g_rndFlashCount].color = SetColor(gp_rndFlashData[g_rndFlashCount].red, gp_rndFlashData[g_rndFlashCount].green, gp_rndFlashData[g_rndFlashCount].blue, 0);
                        gp_rndFlashData[g_rndFlashCount].isComplete = true;
                        g_rndFlashCount++;
                        g_rndFlashActive = false;
                    }
                }
                else if (g_seqActive)
                {
                    gp_seqData[g_seqCount].blue = colorVal;
                    gp_seqData[g_seqCount].color = SetColor(gp_seqData[g_seqCount].red, gp_seqData[g_seqCount].green, gp_seqData[g_seqCount].blue, 0);
                    gp_seqData[g_seqCount].isComplete = true;
                    g_seqCount++;
                    g_seqActive = false;
                }
            }

            break;

        case FLASHSTRING:   // Sets Up Flash Structs 0-9
            if (buf[indexEnd] == ENDCHAR)
            {
                gp_flashData[g_flashCount].stringNumber = buf[index + 1] - '0';
                gp_flashData[g_flashCount].structNumber = g_flashCount;
                g_flashActive = true;
            }
            break;

        case FLASHTIMEON:   // Timing for On, Off, and Delay
        case FLASHTIMEOFF:

            uint16_t time;
            if (buf[indexEnd] == ENDCHAR)
            {
                time = buf[index + 1] - '0';
            }
            else if (buf[indexEnd + 1] == ENDCHAR)
            {
                uint8_t tens = tensMult(buf[index + 1]);
                tens += buf[index + 2] - '0';
                time = tens;
                indexEnd++;
            }
            else if (buf[indexEnd + 2] == ENDCHAR)
            {
                uint16_t hundreds = hundredsMult(buf[index + 1]);
                hundreds += tensMult(buf[index + 2]);
                hundreds += buf[index + 3] - '0';
                time = hundreds;
                indexEnd += 2;
            }
            else if (buf[indexEnd + 3] == ENDCHAR)
            {
                uint16_t thousands = thousandsMult(buf[index + 1]);
                thousands += hundredsMult(buf[index + 2]);
                thousands += tensMult(buf[index + 3]);
                thousands += buf[index + 4] - '0';
                time = thousands;
                indexEnd += 3;
            }

            if (g_displayRangeActive)
            {
                if (buf[index] == FLASHTIMEOFF)
                    gp_displayRangeInfo[g_displayRangeCount].timeOff = time;
                else
                    gp_displayRangeInfo[g_displayRangeCount].timeDelay = time;
            }
            else if (g_flashActive)
            {
                if (buf[index] == FLASHTIMEON)
                    gp_flashData[g_flashCount].timeOn = time;
                else
                    gp_flashData[g_flashCount].timeOff = time;
            }
            else if (g_rndFlashActive)
            {
                if (buf[index] == FLASHTIMEON)
                    gp_rndFlashData[g_rndFlashCount].timeOn = time;
                else
                    gp_rndFlashData[g_rndFlashCount].timeOff = time;
            }
            else if (g_seqActive)
                gp_seqData[g_seqCount].timeDelay = time;

            break;

        case FLASHTIMES:   //Number of Times to Flash
            if (g_flashActive)
            {
                if (buf[indexEnd] == ENDCHAR)
                    gp_flashData[g_flashCount].numberOfFlashes = buf[index + 1] - '0';
            }
            else if (g_rndFlashActive)
            {
                if (buf[indexEnd] == ENDCHAR)
                    gp_rndFlashData[g_rndFlashCount].numberOfFlashes = buf[index + 1] - '0';
            }

            break;

        case RNDFLASHSTRING:   // Set Up Random Flash
            if (buf[indexEnd] == ENDCHAR)
            {
                gp_rndFlashData[g_rndFlashCount].stringNumber = buf[index + 1] - '0';
                gp_rndFlashData[g_rndFlashCount].structNumber = g_rndFlashCount;
                g_rndFlashActive = true;
            }
            break;

        case RNDFLASHNUMLED:   // Number of LEDs in Random Flash 1-10
            if (buf[indexEnd] == ENDCHAR)
            {
                gp_rndFlashData[g_rndFlashCount].ledsOn = (buf[index + 1] - '0') + 1;
            }
            break;

        case RNDFLASH2NDCOLOR: // If 2nd Color is Enabled on Random Flash
            if (buf[indexEnd] == ENDCHAR)
            {
                uint8_t enable2ndColor = buf[index + 1] - '0';

                if(enable2ndColor > 0)
                {
                    gp_rndFlashData[g_rndFlashCount].enable2ndColor = true;
                    g_rndFlash2ndColorActive = true;
                }
                else
                {
                    gp_rndFlashData[g_rndFlashCount].enable2ndColor = false;
                    g_rndFlash2ndColorActive = false;
                }
            }
            break;

        case RNDFLSH2NDNUMBER:
            if (buf[indexEnd] == ENDCHAR)
            {
                uint8_t prob = buf[index + 1] - '0';
                prob += 2;

                if(prob > 1 && prob < 13 && g_rndFlash2ndColorActive)
                {
                    gp_rndFlashData[g_rndFlashCount].probability = prob;
                }
            }
            break;
            

        case SEQUENTIALSTRING:   // Set Up the Sequential
            if (buf[indexEnd] == ENDCHAR)
            {
                gp_seqData[g_seqCount].stringNumber = buf[index + 1] - '0';
                gp_seqData[g_seqCount].structNumber = g_seqCount;
                g_seqActive = true;
            }
            break;

        case SELECTPATTERN:   // Selects What Pattern to Display when Not in a Supported Game
            if (buf[indexEnd] == ENDCHAR)
            {
                uint8_t patternNum = buf[index + 1] - '0';
                if (patternNum > 0 && patternNum < LEDPATTERNCNT)
                {
                    g_ledPattern = true;
                    g_ledPatternNumber = patternNum - 1;
                    g_runLEDPattern = true;
                }
                else
                {
                    g_ledPattern = false;
                    g_runLEDPattern = false;
                    if (g_ledStringNumber > 3)
                    {
                        InitLEDDisplay(1);
                        InitLEDDisplay(3);
                    }
                    else if (g_ledStringNumber > 1)
                        InitLEDDisplay(1);
                }
            }
            break;

        case GAMESTART:  //  When a Supported Game Starts
            if (buf[index + 1] == ENDCHAR && buf[indexEnd] == ENDCHAR)
            {
                multicore_fifo_push_blocking(GAMESTARTED);

                g_runLEDPattern = false;
                sleep_ms(2);
                g_inGame = true;
                sleep_ms(2);
                multicore_fifo_push_blocking(CLEARSTRIP);

                if (g_ledPattern)
                {
                    if (g_ledStringNumber > 3)
                    {
                        InitLEDDisplay(1);
                        InitLEDDisplay(3);
                    }
                    else if (g_ledStringNumber > 1)
                        InitLEDDisplay(1);
                }
            }

            break;

        case BACKTOSETUP:  // Goes Back to ALED String Set up, When Editing in HOTR

            if (buf[index + 1] == ENDCHAR && buf[indexEnd] == ENDCHAR)
            {
                // Do a Reset
                watchdog_reboot(0, 0, 0);
            }

            break;

        case HOTRCHECK:  // Check, and Tells HOTR that String Set Up is Done

            if (buf[index + 1] == ENDCHAR && buf[indexEnd] == ENDCHAR)
            {
                // Checking if this is a HOTR ALED Strip Controller
                // tud_cdc_read
                const char *chkMsg = "DE69";
                tud_cdc_write(chkMsg, sizeof(chkMsg));
                tud_task();
                tud_cdc_write_flush();
            }

            break;
        }

        index = indexEnd + 1;
        indexEnd = index + 2;
    }
}

void GameRunning(char buf[], uint16_t count)
{
    // read data
    uint16_t index = 0;
    uint16_t indexEnd = 2;
    bool noMutex;

    while (indexEnd < count)
    {
        noMutex = false;

        if (buf[index] == UPDATEDISPLAYRANGE)
        {
            uint8_t stringN = buf[index + 1] - '0';
            uint16_t newNum;
            if (buf[indexEnd + 1] == ENDCHAR)
            {
                newNum = buf[index + 2] - '0';
                indexEnd++;
            }
            else if (buf[indexEnd + 2] == ENDCHAR)
            {
                uint8_t tens = tensMult(buf[index + 2]);
                tens += buf[index + 3] - '0';
                newNum = tens;
                indexEnd += 2;
            }
            else if (buf[indexEnd + 3] == ENDCHAR)
            {
                uint16_t hundreds = hundredsMult(buf[index + 2]);
                hundreds += tensMult(buf[index + 3]);
                hundreds += buf[index + 4] - '0';
                newNum = hundreds;
                indexEnd += 3;
            }
            
            if (!g_isRunning[stringN])
            {
                uint32_t mOwner;
                if(mutex_try_enter(&g_mutexStrip[stringN], &mOwner))
                {
                    g_isRunning[stringN] = true;
                    mutex_exit(&g_mutexStrip[stringN]);

                    if (stringN == 0 || stringN == 2)
                    {
                        multicore_fifo_push_blocking(UPDATEDISPLAYRNUM);
                        multicore_fifo_push_blocking(stringN);
                        multicore_fifo_push_blocking(newNum);
                    }
                    else
                        UpdateDisplayNumber(stringN, newNum);
                }
                else
                    noMutex = true;
            }
            else
            {
                UpdateJustDisplayNumber(stringN, newNum);
            }
            
        }
        else if (buf[index] == FLASH)
        {
            uint8_t structN = buf[index + 1] - '0';
            uint8_t string = gp_flashData[structN].stringNumber;
            uint8_t count = 0;

            if (!g_isRunning[string])
            {
                uint32_t mOwner;
                if(mutex_try_enter(&g_mutexStrip[string], &mOwner))
                {
                    g_isRunning[string] = true;
                    mutex_exit(&g_mutexStrip[string]);
                    if (string == 0 || string == 2)
                    {
                        multicore_fifo_push_blocking(DOAFLASH);
                        multicore_fifo_push_blocking(structN);
                    }
                    else
                        DoFlash(structN);
                }
                else
                    noMutex = true;
            }
            
        }
        else if (buf[index] == RNDFLASH)
        {
            uint8_t structN = buf[index + 1] - '0';
            uint8_t string = gp_rndFlashData[structN].stringNumber;
            uint8_t count = 0;

            if (!g_isRunning[string])
            {
                uint32_t mOwner;
                if(mutex_try_enter(&g_mutexStrip[string], &mOwner))
                {
                    g_isRunning[string] = true;
                    mutex_exit(&g_mutexStrip[string]);
                    if (string == 0 || string == 2)
                    {
                        multicore_fifo_push_blocking(DOARNDFLASH);
                        multicore_fifo_push_blocking(structN);
                    }
                    else
                        DoRndFlash(structN);
                }
                else
                    noMutex = true;
            }
            
        }
        else if (buf[index] == SEQUENTIAL)
        {
            uint8_t structN = buf[index + 1] - '0';
            uint8_t string = gp_seqData[structN].stringNumber;
            uint8_t count = 0;

            if (!g_isRunning[string])
            {
                uint32_t mOwner;
                if(mutex_try_enter(&g_mutexStrip[string], &mOwner))
                {
                    g_isRunning[string] = true;
                    mutex_exit(&g_mutexStrip[string]);
                    if (string == 0 || string == 2)
                    {
                        multicore_fifo_push_blocking(DOASEQUENTIAL);
                        multicore_fifo_push_blocking(structN);
                    }
                    else
                        DoSeq(structN);
                }
                else
                    noMutex = true;
            }
        }
        else if (buf[index] == FLASHWAIT)
        {
            uint8_t structN = buf[index + 1] - '0';
            uint8_t string = gp_flashData[structN].stringNumber;

            if (g_isRunning[string])
            {
                AddCommandBuffer(structN);
            }
            else
            {
                uint32_t mOwner;
                if(mutex_try_enter(&g_mutexStrip[string], &mOwner))
                {
                    g_isRunning[string] = true;
                    mutex_exit(&g_mutexStrip[string]);
                    if (string == 0 || string == 2)
                    {
                        multicore_fifo_push_blocking(DOAFLASH);
                        multicore_fifo_push_blocking(structN);
                    }
                    else
                        DoFlash(structN);
                }
                else
                    noMutex = true;
            }
        }
        else if (buf[index] == GAMEENDED)
        {
            if (buf[index + 1] == ENDCHAR && buf[indexEnd] == ENDCHAR)
            {
                GameEnded();
                g_inGame = false;
                // Tell other Core to Exit
                multicore_fifo_push_blocking(EXITOUTCMD);
            }
        }

        if(noMutex)
        {
            //Go Back Around to Try to Get Mutex Again
            indexEnd = 2;
        }
        else
        {
            // Go to the Next Command or End
            index = indexEnd + 1;
            indexEnd = index + 2;
        }
    }

    //Check if Data Changed on Display Range
    uint8_t indexS = 1;
    for(uint8_t i = 0; i < g_stringsCore0; i++)
    {
        if(gp_displayRangeInfo[indexS].dataChanged)
        {
            if (!g_isRunning[indexS])
            {
                uint32_t mOwner;
                if(mutex_try_enter(&g_mutexStrip[indexS], &mOwner))
                {
                    g_isRunning[indexS] = true;
                    mutex_exit(&g_mutexStrip[indexS]);
                    DisplayDataOnLEDs(indexS);
                }
            }
        }
        indexS += 2;
    }
}

void GameEnded(void)
{
    // Wait Until Things End
    bool stillRunning = true;
    uint8_t countString = 0;

    while(stillRunning)
    {
        for (uint8_t i = 0; i < g_ledStringNumber; i++)
        {
            if(!g_isRunning[i])
                countString++;
        }

        if(countString == g_ledStringNumber)
            stillRunning = false;
        else
            countString = 0;

        busy_wait_ms(10);
    }

    // Free Up the Memory Used
    for (uint8_t i = 0; i < g_ledStringNumber; i++)
    {
        // Go through the Display Range Data
        if (gp_displayRangeInfo[i].isComplete)
        {
            // Free Up the Memory
            free(gp_displayRangeInfo[i].p_lightsDisplay);
            free(gp_displayRangeInfo[i].p_lightsInStep);
            free(gp_displayRangeInfo[i].p_redSteps);
            free(gp_displayRangeInfo[i].p_greenSteps);
            free(gp_displayRangeInfo[i].p_blueSteps);
            // Set Complete to False for Display Range
            gp_displayRangeInfo[i].isComplete = false;
            gp_displayRangeInfo[i].isLatched = false;
            gp_displayRangeInfo[i].dataChanged = false;
            gp_displayRangeInfo[i].lastColor = 0;
            // And Display Numbers back to 0
            gp_displayRangeInfo[i].displayNumber = 0;
            gp_displayRangeInfo[i].oldDisplayNumber = 0;
            gp_displayRangeInfo[i].enableReload = false;
            gp_displayRangeInfo[i].numLEDs = 1;
        }
    }

    for (uint8_t i = 0; i < COMMANDMAX; i++)
    {
        if (gp_flashData[i].isComplete)
        {
            gp_flashData[i].isComplete = false;
            gp_flashData[i].timesFlashed = 0;
        }

        if (gp_rndFlashData[i].isComplete)
        {
            gp_rndFlashData[i].isComplete = false;
            gp_rndFlashData[i].timesFlashed = 0;
            gp_rndFlashData[i].enable2ndColor = false;
            gp_rndFlashData[i].use2ndColor = false;
        }

        if (gp_seqData[i].isComplete)
        {
            gp_seqData[i].isComplete = false;
            gp_seqData[i].seqLED = 1;
        }
    }

    // Set things back to 0
    // Display Functions
    g_displayRangeCount = 0;
    g_flashCount = 0;
    g_rndFlashCount = 0;
    g_seqCount = 0;
    // Color Counts
    g_redCount = 0;
    g_greenCount = 0;
    g_blueCount = 0;
    // Command is Active, to be Sure
    g_displayRangeActive = false;
    g_flashActive = false;
    g_rndFlashActive = false;
    g_seqActive = false;

    if (g_ledPattern)
        g_runLEDPattern = true;

    g_tNumber = 0;

    // Clear Out Command Buffer
    g_cmdBufferEXE = 0;
    g_cmdBufferCount = 0;

    // Clear Out Command Buffer
    for (uint8_t i = 0; i < CMDBUFFERSIZE; i++)
        g_cmdBufferData[i] = UNASSIGN;

    // Clear out the Running Bools
    for (uint8_t i = 0; i < LEDSTRINGSMAX; i++)
    {
        g_isRunning[i] = false;
        g_isRDRunning[i] = false;
    }
}

void cdc_task(void)
{
    // Check Flash Command Buffer, 1 Command at a Time
    if (g_cmdBufferEXE != g_cmdBufferCount)
    {
        uint8_t structNum = g_cmdBufferData[g_cmdBufferEXE];

        if (structNum != UNASSIGN)
        {
            uint8_t string = gp_flashData[structNum].stringNumber;

            if (!g_isRunning[string])
            {
                uint32_t mOwner;
                if(mutex_try_enter(&g_mutexStrip[string], &mOwner))
                {
                    g_isRunning[string] = true;
                    mutex_exit(&g_mutexStrip[string]);

                    // Now Doing Command, So Remove From Command Buffer
                    g_cmdBufferData[g_cmdBufferEXE] = UNASSIGN;

                    g_cmdBufferEXE++;
                    if (g_cmdBufferEXE >= CMDBUFFERSIZE)
                        g_cmdBufferEXE = 0;

                    if (string == 0 || string == 2)
                    {
                        multicore_fifo_push_blocking(DOAFLASH);
                        multicore_fifo_push_blocking(structNum);
                    }
                    else
                        DoFlash(structNum);
                }
            }
        }
    }

    // Get Data from Virtual Serial Port, and Send it to the Right PLace
    //if (tud_cdc_available())
    if(g_DataOnCDC)
    {
        char buffer[SPCHARBUFSIZE];
        uint16_t countRx = tud_cdc_read(buffer, sizeof(buffer));
        g_DataOnCDC = false;

        if (g_inGame)
            GameRunning(buffer, countRx);
        else if (g_isLEDStringsInited)
            NoGame(buffer, countRx);
        else
            SetUpLEDStrings(buffer, countRx);
    }
    
    
}





// Invoked when CDC interface received data from host - Polling is at 8ms, 7ms slower than using USB HID
void tud_cdc_rx_cb(uint8_t itf)
{
    (void)itf;

    g_DataOnCDC = true;

    // Do not want to read data here, as it is an interrupt, and will stop display functions
    // and Might Screw Up Writting to the ALED Strip
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void)itf;
    (void)rts;


    if (dtr)
    {
        // Terminal connected
    }
    else
    {
        // Terminal disconnected
    }
}

