#ifndef GLOBALS_H
#define GLOBALS_H

// STD Libs
#include <stdio.h>
#include <stdlib.h>
#include <pico/stdio.h>
#include "pico/stdlib.h"

// For the PIO Stuff
#include "hardware/pio.h"
#include "hardware/clocks.h"
// PIO Program for WS2812, or Neopixel Interface
#include "ws2812.pio.h"

// For the USB Stuff
#include "bsp/board_api.h"
#include "tusb.h"

// Multi-Core
#include "pico/multicore.h"
#include "pico/sync.h"



//--------------------------------------------------------------------+
// Defines
//--------------------------------------------------------------------+

// Speed of the Neopixel Inteface, which is 800Khz, or 800,000Hz
#define NEOPIXELCLOCK       800000

// If LEDs have White LEDs or Not. If yes, then it is 32bits. If not, then 24bits
// 8bits per Primary Color, and 8bits for White (if there)
// Cannot Mix RGB and RGBW ALED Strings
#define IS_RGBW             false

// Number of LED Strings
#define LEDSTRINGSMAX       4

//Strings Per Core
#define STRINGSPERCORE      2

// GPIO Pin that Will be Used to Write Data to LEDs
// The 2 means GPIO2, which is the Physical Pin 4
#define WS2812_PIN_0        2
#define WS2812_PIN_1        3
#define WS2812_PIN_2        4
#define WS2812_PIN_3        5

// USB Serial Port Char Buffer Size
#define SPCHARBUFSIZE       64

// How Many LED Patterns to Display when Not in a Game
#define LEDPATTERNCNT       5

// LED pattern Count Max
#define LEDPATTERNMAX       1000

// ALED Latch Time in us
#define ALEDLATCHTIME       65

// Time to Wait to Try to get Mutex Again
#define MUTEXIRQWAIT        20 

// How Many Flash, Random Flash and Sequential Commands
#define COMMANDMAX          10

// For Repeating Timers Writing PIO Tx 64bit Buffer
#define PIOTXTIME           235
#define PIOTXMWRITE         8
#define TIMEONELED          30


// Wait Command Buffer Size
#define CMDBUFFERSIZE       10
#define UNASSIGN            69

//Commands for Core1
#define UPDATEDISPLAYRNUM   0
#define DOAFLASH            1
#define DOARNDFLASH         2
#define DOASEQUENTIAL       3

#define EXITOUTCMD          9
#define CLEARSTRIP          10
#define GAMESTARTED         11


// Time to Wait to Turn Off ALED String in ms
#define TIMETURNOFFSTRINGS  500

#define RESETSTATETIME      150



// Serial Port Commands

// End Termination
#define ENDCHAR             '|'

//Check if HOTR ALED Strip Controller
#define HOTRCHECK           '='

// Setting Up LED Strings
#define LEDSTRINGNUMBER     '!'
#define LEDSTRINGELEMENTS   '#'

// Go Back to LED String Set-up
#define BACKTOSETUP         '_'

// Setting Up the Display Range, for a LED String
#define DISPLAYRANGE        '$'
#define DISPLAYRANGEMAX     '?'
#define STEPNUMBERS         '%'
#define RELOADSEQ           ','
#define RELOADNUMLEDS       '.'

// Set Up the Color
#define REDCOLOR            '*'
#define GREENCOLOR          '('
#define BLUECOLOR           ')'

// Update A Display Range
#define UPDATEDISPLAYRANGE  ':'

// Setting Up the Flash, for a LED String
#define FLASHSTRING         ';'
#define FLASHTIMEON         '<'
#define FLASHTIMEOFF        '>'
#define FLASHTIMES          '^'

#define FLASH               'F'
#define FLASHWAIT           'W'

// Setting Up the Random Flash, for a LED String
#define RNDFLASHSTRING      '&'
#define RNDFLASHNUMLED      '['

#define RNDFLASH2NDCOLOR    '{'
#define RNDFLSH2NDNUMBER    '}'

#define RNDFLASH            'R'

// Setting Up the Sequential, for a LED String
#define SEQUENTIALSTRING    ']'

#define SEQUENTIAL          'S'


// When A Supported Game Has Started and All Display Data Has Been Updated
#define GAMESTART           '+'

// When A Supported Game Has Ended and Release All Data
#define GAMEENDED           '-'

// Select What Pattern to Display when Not In a Supported Game
#define SELECTPATTERN       '@'



//--------------------------------------------------------------------+
// Stucts
//--------------------------------------------------------------------+

// Struct for Display Range
typedef struct
{
    uint8_t stringNumber;      // LED String Number for Display Range Command (1-255)
    uint16_t maxRange;         // Max Range of Value to Calculate Lights Per Number
    uint8_t numberOfSteps;     // Number of Different Steps in the Display (1-255)
    float lightsPerUnit;       // Number of LEDs divided by Max Unit Value
    uint16_t *p_lightsDisplay; // Array of Max Unit Values, with How Many Lights to Display
    uint16_t *p_lightsInStep;  // Number of Lights in Each Step Array
    uint8_t *p_redSteps;       // Red Color for Each Step Array (8bit Color)
    uint8_t *p_greenSteps;     // Green Color for Each Step Array (8bit Color)
    uint8_t *p_blueSteps;      // Blue Color for Each Step Array (8bit Color)
    uint16_t displayNumber;    // Number to Display to the Light String
    uint16_t oldDisplayNumber; // Last Display Number
    uint16_t timeOff;          // Time Of Before Doing a Different Display, like a Flash
    bool enableReload;         // Enables Sequential Reload
    uint16_t timeDelay;        // Time Delay in us, using low level timers
    uint8_t numLEDs;           // Number of LEDs to Light Up for Seqential Reload
    uint32_t reloadColor;      // 32bit GRB Color of the reload
    uint16_t reloadCount;      // Count of the Sequential Reload
    uint16_t reloadMax;        // Where to Stop the Reload
    uint32_t color;            // Color Going onto ALED String
    uint32_t lastColor;        // Color Being Used on ALED String
    uint16_t count2;           // When there is Color and Turn Off Lights
    bool isLatched;            // Used to Latch Data
    bool isComplete;           // If All the Info is in the Struct
    bool dataChanged;          // Know When Data Has Changed, when Doing other Display
} DisplayRange;


// Struct for Flash
typedef struct
{
    uint8_t stringNumber;       // LED String Number for Display Range Command (1-255)
    uint16_t timeOn;            // Time On in ms (0-65,535)
    uint16_t timeOff;           // Time Off in ms (0-65,535)
    uint8_t numberOfFlashes;    // Number of Flashes to Do (1-255)
    uint8_t red;                // 8bit Red Color
    uint8_t green;              // 8bit Green Color
    uint8_t blue;               // 8bit Blue Color
    uint32_t color;             // 32bit GRB Color
    bool isComplete;            // Data Complete for Struct
    uint8_t structNumber;       // Array Position
    uint8_t timesFlashed;       // Number of Times Flashed   
} FlashData;


// Struct for Random Flash
typedef struct
{
    uint8_t stringNumber;       // LED String Number for Display Range Command (1-255)
    uint16_t timeOn;            // Time On in ms (0-65,535)
    uint16_t timeOff;           // Time Off in ms (0-65,535)
    uint8_t numberOfFlashes;    // Number of Flashes to Do (1-255)
    uint8_t red;                // 8bit Red Color
    uint8_t green;              // 8bit Green Color
    uint8_t blue;               // 8bit Blue Color
    uint32_t color;             // 32bit GRB Color
    bool isComplete;            // Data Complete for Struct
    uint8_t structNumber;       // Array Position
    uint8_t timesFlashed;       // Number of Times Flashed
    uint8_t ledsOn;             // Number of LEDs to Flash (1-10)
    uint16_t rndNumber;         // Random Number to Flash
    bool enable2ndColor;        // Enables 2nd Color on Random Flash
    uint8_t probability;        // Probability of the 2nd Color (2-12)
    bool use2ndColor;           // When to use the 2nd Color;
    uint8_t red2;               // 8bit Red Color
    uint8_t green2;             // 8bit Green Color
    uint8_t blue2;              // 8bit Blue Color
    uint32_t color2;            // 32bit GRB Color   
} RndFlashData;

// Struct for Sequential
typedef struct
{
    uint8_t stringNumber;       // LED String Number for Display Range Command (1-255)
    uint16_t timeDelay;         // Time Delay in ms (0-65,535)
    uint8_t structNumber;       // Array Position
    uint16_t seqLED;            // Potition of the Sequential - LED Number
    uint16_t ledElements;       // Number of LEDs in the String
    uint8_t numLEDs;            // Number of LEDs to Light Up for Seqential
    uint8_t red;                // 8bit Red Color
    uint8_t green;              // 8bit Green Color
    uint8_t blue;               // 8bit Blue Color
    uint32_t color;             // 32bit GRB Color
    bool isComplete;            // Data Complete for Struct
} SeqData;

// Struct to Have Data Be Written to PIO Tx Buffer
typedef struct
{
    uint8_t stringNumber;      // LED String Number for Display Range Command (1-255)
    uint8_t structNumber;      // Array Position is using Struct
    uint32_t color;            // 32bit GRB Color
    uint16_t count;            // Count of the LED Data
    uint16_t countMax;         // Where to Stop
    uint32_t color2;           // 32bit GRB Color
    uint16_t count2;           // Count of the LED Data
    uint16_t countMax2;        // Where to Stop
    bool drSeqReload;          // If Display Range Sequential Reload
    bool flash;                // If Flash
    bool rndFlash;             // If Random Flash
    bool seqiential;           // If Seqiential
    bool returnToFunction;     // Return to a Display Function
    uint8_t whatFunction;      // What Function to Return to
} TxDataWrite; 



//--------------------------------------------------------------------+
// Globals Extern Def
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// Globals - PIO
//--------------------------------------------------------------------+

// For the PIO - There is 3 PIO Units, Each Unit having 4 PIO PSM SAelected by the sm
// 4 pio0s, 4 pio1s, and 4 pio2s, to make 12
extern PIO *gp_pio;
// State Machine Picks What PIO PSM in the Init Above - 0 to 3
extern uint g_sm[LEDSTRINGSMAX];
// PIO Offset
extern uint g_offset[LEDSTRINGSMAX];

// Pins for the LED Strings and the PIO
extern uint g_pioPins[LEDSTRINGSMAX];

// Does the LED String Have a White LED for 32 Writes, instead of just 24 Writes for just RGB
extern bool g_isRGBW;

// Success Bool for the PIO
extern bool g_success[LEDSTRINGSMAX];



//--------------------------------------------------------------------+
// Globals - Setting Up LED Strings
//--------------------------------------------------------------------+

// Setting Up the LED Strings
// To Know when The LED Strings Have Been Set-up, Run LEDSetUp Until it is True
extern bool g_isLEDStringsInited;
// To Know how Many LED Strings there Are
extern volatile uint8_t g_ledStringNumber;
// To Know How Many Eleemts are in Each String
extern uint16_t *gp_ledStringElements;
// Count To Keep Track of What LED String Info the Program is Recieving
extern uint8_t g_elementCount;
// Is a Command Running on the LED String
extern volatile bool g_isRunning[LEDSTRINGSMAX];

// Number of Strings on the 2 Cores
extern uint8_t g_stringsCore0;
extern uint8_t g_stringsCore1;


//--------------------------------------------------------------------+
// Globals - Display Range's Mutexs
//--------------------------------------------------------------------+

extern mutex_t g_mutexStrip[LEDSTRINGSMAX];
//extern mutex_t g_mutexDRData[LEDSTRINGSMAX];

//--------------------------------------------------------------------+
// Globals - PIO Tx FIFO Buffer Struct
//--------------------------------------------------------------------+

// Struct Used to Write to the Tx FIFO Buffer
extern TxDataWrite *gp_txData;

//--------------------------------------------------------------------+
// Globals - Setting Up Display Range
//--------------------------------------------------------------------+

// Display Range Info and Set-up
// Stores All The Display Range Info, Per LED String(s)
extern DisplayRange *gp_displayRangeInfo;
// Count to Keep Track on Setting Up Display Range
extern uint8_t g_displayRangeCount;
// When Entering Display Range Data
extern bool g_displayRangeActive;
// When Range Display is Running on a LED String
extern volatile bool g_isRDRunning[LEDSTRINGSMAX];


//--------------------------------------------------------------------+
// Globals - Setting Up Flash
//--------------------------------------------------------------------+

// Flash Data in the Struct
extern FlashData *gp_flashData;
// Count to Keep Track on Setting Flash
extern uint8_t g_flashCount;
// When Entering Display Range Data
extern bool g_flashActive;


//--------------------------------------------------------------------+
// Globals - Setting Up Random Flash
//--------------------------------------------------------------------+

// Flash Data in the Struct
extern RndFlashData *gp_rndFlashData;
// Count to Keep Track on Setting Flash
extern uint8_t g_rndFlashCount;
// When Entering Display Range Data
extern bool g_rndFlashActive;
// When 2nd Color is Enabled
extern bool g_rndFlash2ndColorActive;


//--------------------------------------------------------------------+
// Globals - Setting Up Sequential
//--------------------------------------------------------------------+

// Flash Data in the Struct
extern SeqData *gp_seqData;
// Count to Keep Track on Setting Flash
extern uint8_t g_seqCount;
// When Entering Display Range Data
extern bool g_seqActive;

//--------------------------------------------------------------------+
// Globals - Command Buffer
//--------------------------------------------------------------------+

extern volatile uint8_t g_cmdBufferData[CMDBUFFERSIZE];
extern volatile uint8_t g_cmdBufferCount;
extern volatile uint8_t g_cmdBufferEXE;

extern volatile uint8_t g_core0CMD;
extern volatile uint8_t g_core1CMD;



//--------------------------------------------------------------------+
// Globals - Setting Up Color
//--------------------------------------------------------------------+

// Keeps Trak of the Color(s) When they are Recieved
extern uint8_t g_redCount;
extern uint8_t g_greenCount;
extern uint8_t g_blueCount;



//--------------------------------------------------------------------+
// Globals - Setting Up Pattern when Not in Game
//--------------------------------------------------------------------+

// To Run a LED Strip Pattern when Not in a Game
extern volatile bool g_ledPattern;

// Run the LED Pattern if Not In a Game
extern volatile bool g_runLEDPattern;

// To Know What LED Pattern to Run
extern volatile uint8_t g_ledPatternNumber;

// Pattern T number
extern volatile int g_tNumber;


//--------------------------------------------------------------------+
// Globals - Setting Up In Game
//--------------------------------------------------------------------+

// To Know if a Supported Game is Being Played or Not
extern volatile bool g_inGame;

// To know when Data is Available on the CDC Interface
extern volatile bool g_DataOnCDC;



//--------------------------------------------------------------------+
// Small Functions
//--------------------------------------------------------------------+

// Sets Color to the GRB Format
// If RGBW is True, then Adds white into the Mix, at the Front
#if IS_RGBW == true
static inline uint32_t SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    return ((uint32_t)(r) << 8) |
           ((uint32_t)(g) << 16) |
           ((uint32_t)(w) << 24) |
           (uint32_t)(b);
}
#else
static inline uint32_t SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    (void) w;
    return ((uint32_t)(r) << 16) |
           ((uint32_t)(g) << 24) |
           ((uint32_t)(b) << 8);
}
#endif



#endif // GLOBALS_H