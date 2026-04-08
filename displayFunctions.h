#ifndef DISPLAYFUNCTIONS_H
#define DISPLAYFUNCTIONS_H

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

// Multi-Core
#include "pico/multicore.h"
#include "pico/sync.h"

// For the USB Stuff - For tud_task
#include "bsp/board_api.h"
#include "tusb.h"

//Globals
#include "globals.h"


//static DisplayRangeSeq drSeqData[LEDSTRINGSMAX];
//static DisplayRangeSeq drSeqData[LEDSTRINGSMAX];

//static TxDataWrite txDataInfo[LEDSTRINGSMAX];

//TxDataWrite *gp_txData = NULL;

static struct repeating_timer drSeqtimer[LEDSTRINGSMAX];


//--------------------------------------------------------------------+
// Functions - Setup LED Strings Beginning
//--------------------------------------------------------------------+

// Process Data for Number of LED Strings
void NumberLEDStrings(uint8_t numberStrings);

// Init LED String with the PIO
void InitLEDStrings(void);

// Init LED String to Off - Know Color
void InitLEDDisplay(uint8_t stringNum);

//--------------------------------------------------------------------+
// Functions - De-Init LED Strings
//--------------------------------------------------------------------+

// De-Init LED String with the PIO
void DeinitLEDStrings(void);



//--------------------------------------------------------------------+
// Functions - When In a Game - Display Range
//--------------------------------------------------------------------+

// Update Display Number for a String
void UpdateDisplayNumber(uint8_t stringNum, uint16_t newDisplayNum);

// Update Just Data, When Doing other Display Functions
void UpdateJustDisplayNumber(uint8_t stringNum, uint16_t newDisplayNum);

// Display Data to the LEDs
void DisplayDataOnLEDs(uint8_t stringNum);

// After a Display Command Has Run, like Flash, then Put Back Display Range
void DisplayRangeData(uint8_t stringNum);

bool __not_in_flash_func(RT_WriteDataPIOTxBuffer_CB)(struct repeating_timer *t);

bool __not_in_flash_func(RT_WriteDataPIOTxBuffer_2C_CB)(struct repeating_timer *t);

int64_t __not_in_flash_func(LatchString)(alarm_id_t id, __unused void *user_data);

int64_t __not_in_flash_func(DODRSeq)(alarm_id_t id, __unused void *user_data);

//--------------------------------------------------------------------+
// Functions - When In a Game - Flash
//--------------------------------------------------------------------+

// Check if Range Display is Running
// Yes - Then Turn Off LEDs for Range Display Time
// No - Do a Flash on Certain Flash Struct
void DoFlash(uint8_t structNum);

// If Range Display is Running, Do Flash On
int64_t FlashOn(alarm_id_t id, __unused void *user_data);

// Time Callback Function to Turn Flash Off
int64_t FlashOff(alarm_id_t id, __unused void *user_data);

// Time Callback Function to Complete Flash
int64_t FlashComplete(alarm_id_t id, __unused void *user_data);


//--------------------------------------------------------------------+
// Functions - When In a Game - Random Flash
//--------------------------------------------------------------------+

// Check if Range Display is Running
// Yes - Then Turn Off LEDs for Range Display Time
// No - Do a Random Flash on Certain RndFlash Struct
void DoRndFlash(uint8_t structNum);

// If Range Display is Running, Do Flash On
int64_t RndFlashON(alarm_id_t id, __unused void *user_data);

// Time Callback Function to Turn Flash Off
int64_t RndFlashOff(alarm_id_t id, __unused void *user_data);

// Time Callback Function to Complete Flash
int64_t RndFlashComplete(alarm_id_t id, __unused void *user_data);


//--------------------------------------------------------------------+
// Functions - When In a Game - Sequential
//--------------------------------------------------------------------+

// Check if Range Display is Running
// Yes - Then Turn Off LEDs for Range Display Time
// No - Do a Sequential on Certain Seq Struct
void DoSeq(uint8_t structNum);

// If Range Display Running then First Sequential
int64_t SeqFirst(alarm_id_t id, __unused void *user_data);

// Time Callback Function for Next Sequential LED
int64_t SeqNext(alarm_id_t id, __unused void *user_data);

// Time Callback Function to Complete Sequential
int64_t SeqComplete(alarm_id_t id, __unused void *user_data);

#endif // DISPLAYFUNCTIONS_H