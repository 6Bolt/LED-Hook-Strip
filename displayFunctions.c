#include "displayFunctions.h"

//--------------------------------------------------------------------+
// Functions - Setup LED Strings Beginning
//--------------------------------------------------------------------+

void NumberLEDStrings(uint8_t numberStrings)
{
    g_ledStringNumber = numberStrings;
    gp_ledStringElements = (uint16_t *)malloc(numberStrings * sizeof(uint16_t));

    if (numberStrings == 1)
    {
        g_stringsCore0 = 0;
        g_stringsCore1 = 1;
    }
    else if (numberStrings == 2)
    {
        g_stringsCore0 = 1;
        g_stringsCore1 = 1;
    }
    else if (numberStrings == 3)
    {
        g_stringsCore0 = 1;
        g_stringsCore1 = 2;
    }
    else if (numberStrings == 4)
    {
        g_stringsCore0 = 2;
        g_stringsCore1 = 2;
    }

    // Set Up the Display Functions Structs
    gp_displayRangeInfo = (DisplayRange *)malloc(numberStrings * sizeof(DisplayRange));

    if (gp_flashData == NULL)
        gp_flashData = (FlashData *)malloc(COMMANDMAX * sizeof(FlashData));

    if (gp_rndFlashData == NULL)
        gp_rndFlashData = (RndFlashData *)malloc(COMMANDMAX * sizeof(RndFlashData));

    if (gp_rndFlashData == NULL)
        gp_seqData = (SeqData *)malloc(COMMANDMAX * sizeof(SeqData));

    // Set Up PIO Tx FIFO Buffer Struct
    gp_txData = (TxDataWrite *)malloc(numberStrings * sizeof(TxDataWrite));

    for (uint8_t i = 0; i < numberStrings; i++)
    {
        gp_displayRangeInfo[i].isComplete = false;
        gp_displayRangeInfo[i].numLEDs = 1;
        gp_displayRangeInfo[i].dataChanged = false;
        gp_displayRangeInfo[i].displayNumber = 0;
        gp_displayRangeInfo[i].oldDisplayNumber = 0;
        gp_displayRangeInfo[i].enableReload = false;
        gp_displayRangeInfo[i].isLatched = false;
        gp_displayRangeInfo[i].lastColor = 0;

        gp_txData[i].drSeqReload = false;
        gp_txData[i].flash = false;
        gp_txData[i].rndFlash = false;
        gp_txData[i].seqiential = false;
        gp_txData[i].returnToFunction = false;
    }

    for (uint8_t i = 0; i < COMMANDMAX; i++)
    {
        gp_flashData[i].isComplete = false;
        gp_flashData[i].timesFlashed = 0;
        gp_rndFlashData[i].isComplete = false;
        gp_rndFlashData[i].timesFlashed = 0;
        gp_rndFlashData[i].enable2ndColor = false;
        gp_rndFlashData[i].use2ndColor = false;
        gp_seqData[i].isComplete = false;
        gp_seqData[i].seqLED = 1;
    }
}

void InitLEDStrings(void)
{
    for (uint8_t i = 0; i < g_ledStringNumber; i++)
    {
        if (!g_success[i])
        {
            g_success[i] = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &gp_pio[i], &g_sm[i], &g_offset[i], g_pioPins[i], 1, true);
            if (g_success[i])
            {
                hard_assert(g_success[i]);

                ws2812_program_init(gp_pio[i], g_sm[i], g_offset[i], g_pioPins[i], NEOPIXELCLOCK, g_isRGBW);

                InitLEDDisplay(i);
            }
        }
    }
}

void InitLEDDisplay(uint8_t stringNum)
{
    // Turn Off All LEDs in the String, for a Known State
    for (uint16_t i = 0; i < gp_ledStringElements[stringNum]; i++)
        pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], 0);
}

//--------------------------------------------------------------------+
// Functions - De-Init LED Strings
//--------------------------------------------------------------------+

void DeinitLEDStrings(void)
{
    for (uint8_t i = 0; i < g_ledStringNumber; i++)
        pio_remove_program_and_unclaim_sm(&ws2812_program, gp_pio[i], g_sm[i], g_offset[i]);
}


//--------------------------------------------------------------------+
// Functions - When In a Game - Display Range
//--------------------------------------------------------------------+

void UpdateDisplayNumber(uint8_t stringNum, uint16_t newDisplayNum)
{
    if (!gp_displayRangeInfo[stringNum].dataChanged)
    {
        gp_displayRangeInfo[stringNum].dataChanged = true;
        gp_displayRangeInfo[stringNum].oldDisplayNumber = gp_displayRangeInfo[stringNum].displayNumber;
        gp_displayRangeInfo[stringNum].lastColor = gp_displayRangeInfo[stringNum].color;
    }
    gp_displayRangeInfo[stringNum].displayNumber = newDisplayNum;

    DisplayDataOnLEDs(stringNum);
}

void UpdateJustDisplayNumber(uint8_t stringNum, uint16_t newDisplayNum)
{
    if (!gp_displayRangeInfo[stringNum].dataChanged)
    {
        gp_displayRangeInfo[stringNum].dataChanged = true;
        gp_displayRangeInfo[stringNum].oldDisplayNumber = gp_displayRangeInfo[stringNum].displayNumber;
        gp_displayRangeInfo[stringNum].lastColor = gp_displayRangeInfo[stringNum].color;
    }
    gp_displayRangeInfo[stringNum].displayNumber = newDisplayNum;
}

void DisplayDataOnLEDs(uint8_t stringNum)
{
    uint16_t displayNumber = gp_displayRangeInfo[stringNum].displayNumber;
    uint16_t oldDisplayNumber = gp_displayRangeInfo[stringNum].oldDisplayNumber;

    uint16_t displayCount = gp_displayRangeInfo[stringNum].p_lightsDisplay[displayNumber];
    uint16_t oldDisplayCount = gp_displayRangeInfo[stringNum].p_lightsDisplay[oldDisplayNumber];

    gp_displayRangeInfo[stringNum].dataChanged = false;

    if (displayCount != oldDisplayCount)
    {
        if (displayCount == 0)
        {
            gp_displayRangeInfo[stringNum].color = 0;

            if (oldDisplayCount > PIOTXMWRITE)
            {

                gp_txData[stringNum].color = 0;
                gp_txData[stringNum].count = PIOTXMWRITE;
                gp_txData[stringNum].countMax = oldDisplayCount;
                gp_txData[stringNum].stringNumber = stringNum;

                for (uint8_t i = 0; i < PIOTXMWRITE; i++)
                    pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], 0);

                add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[stringNum], &drSeqtimer[stringNum]);

                return;
            }
            else
            {
                // Done, Latch Strip and Release Strip
                if (oldDisplayCount != 0)
                {
                    for (uint16_t i = 0; i < oldDisplayCount; i++)
                        pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], 0);

                    // Latch Data In ALED Strip + Write Time
                    gp_txData[stringNum].stringNumber = stringNum;
                    uint16_t waitTime = (TIMEONELED * oldDisplayCount) + ALEDLATCHTIME;
                    // Wait Write and Latch Time
                    add_alarm_in_us(waitTime, LatchString, &gp_txData[stringNum], true);
                }

                return;
            }
        }

        uint8_t step;
        uint32_t color;

        // First Find the Step Placement
        if (gp_displayRangeInfo[stringNum].numberOfSteps > 1)
        {
            uint16_t lower = 0;

            for (uint8_t i = 0; i < gp_displayRangeInfo[stringNum].numberOfSteps; i++)
            {
                if (displayCount > lower && displayCount <= gp_displayRangeInfo[stringNum].p_lightsInStep[i])
                {
                    // Found Step
                    step = i;
                    break;
                }
                else
                    lower = gp_displayRangeInfo[stringNum].p_lightsInStep[i];
            }
        }
        else
            step = 0;

        color = SetColor(gp_displayRangeInfo[stringNum].p_redSteps[step], gp_displayRangeInfo[stringNum].p_greenSteps[step], gp_displayRangeInfo[stringNum].p_blueSteps[step], 0);

        gp_displayRangeInfo[stringNum].color = color;

        // Check for Sequential Reload
        if (displayCount - 1 > oldDisplayCount && gp_displayRangeInfo[stringNum].enableReload)
        {
            if (displayCount <= PIOTXMWRITE)
            {
                for (uint16_t i = 0; i < displayCount; i++)
                    pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], color);

                uint16_t waitTime = (displayCount * TIMEONELED) + ALEDLATCHTIME;
                gp_txData[stringNum].stringNumber = stringNum;

                // Wait Write and Latch Time
                add_alarm_in_us(waitTime, LatchString, &gp_txData[stringNum], true);

                return;
            }
            else
            {
                gp_displayRangeInfo[stringNum].reloadColor = color;
                gp_displayRangeInfo[stringNum].reloadCount = oldDisplayCount + 2;
                gp_displayRangeInfo[stringNum].reloadMax = displayCount;
                gp_displayRangeInfo[stringNum].isLatched = false;

                // add_alarm_in_us(gp_DRSeqData[stringNum].timeDelay, DODRSeq, &gp_txData[stringNum], false);
                DODRSeq((alarm_id_t)0, &gp_displayRangeInfo[stringNum]);
            }
        }
        else
        {
            if (displayCount < oldDisplayCount)
            {
                if (oldDisplayCount <= PIOTXMWRITE)
                {
                    for (uint16_t i = 0; i < displayCount; i++)
                        pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], color);

                    for (uint16_t i = displayCount; i < oldDisplayCount; i++)
                        pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], 0);

                    // Latch Data In ALED Strip + Write Time
                    gp_txData[stringNum].stringNumber = stringNum;
                    uint16_t waitTime = (TIMEONELED * oldDisplayCount) + ALEDLATCHTIME;
                    // Wait Write and Latch Time
                    add_alarm_in_us(waitTime, LatchString, &gp_txData[stringNum], true);
                }
                else if (displayCount <= PIOTXMWRITE)
                {
                    uint16_t delta = PIOTXMWRITE - displayCount;

                    gp_txData[stringNum].color = 0;
                    gp_txData[stringNum].count = displayCount + delta;
                    gp_txData[stringNum].countMax = oldDisplayCount;
                    gp_txData[stringNum].stringNumber = stringNum;

                    for (uint16_t i = 0; i < displayCount; i++)
                        pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], color);

                    if (delta != 0)
                    {
                        for (uint16_t i = 0; i < delta; i++)
                            pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], 0);
                    }

                    add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[stringNum], &drSeqtimer[stringNum]);
                }
                else
                {
                    gp_txData[stringNum].color = color;
                    gp_txData[stringNum].count = PIOTXMWRITE;
                    gp_txData[stringNum].countMax = displayCount;
                    gp_txData[stringNum].color2 = 0;
                    gp_txData[stringNum].count2 = displayCount;
                    gp_txData[stringNum].countMax2 = oldDisplayCount;
                    gp_txData[stringNum].stringNumber = stringNum;

                    for (uint16_t i = 0; i < PIOTXMWRITE; i++)
                        pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], color);

                    add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_2C_CB, &gp_txData[stringNum], &drSeqtimer[stringNum]);
                }
            }
            else
            {
                if (displayCount > PIOTXMWRITE)
                {
                    gp_txData[stringNum].color = color;
                    gp_txData[stringNum].count = PIOTXMWRITE;
                    gp_txData[stringNum].countMax = displayCount;
                    gp_txData[stringNum].stringNumber = stringNum;

                    for (uint16_t i = 0; i < PIOTXMWRITE; i++)
                        pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], color);

                    add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[stringNum], &drSeqtimer[stringNum]);
                }
                else
                {
                    for (uint16_t i = 0; i < displayCount; i++)
                        pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], color);

                    // Latch Data In ALED Strip + Write Time
                    gp_txData[stringNum].stringNumber = stringNum;
                    uint16_t waitTime = (TIMEONELED * displayCount) + ALEDLATCHTIME;
                    // Wait Write and Latch Time
                    add_alarm_in_us(waitTime, LatchString, &gp_txData[stringNum], true);
                }
            }
        }
    }
}

void DisplayRangeData(uint8_t stringNum)
{
    // Display Range Data after a Display Command, Like Flash
    // ALEDs will be Off, so Got to Find Color, then do Up to Display Count
    uint16_t displayNumber = gp_displayRangeInfo[stringNum].displayNumber;
    uint16_t oldDisplayNumber = gp_displayRangeInfo[stringNum].oldDisplayNumber;

    // If Display is Zero, then do Nothing
    if (displayNumber == 0)
    {
        gp_displayRangeInfo[stringNum].dataChanged = false;

        // Turn Of Running a Display Command is Complete
        mutex_enter_blocking(&g_mutexStrip[stringNum]);
        g_isRunning[stringNum] = false;
        mutex_exit(&g_mutexStrip[stringNum]);
        return;
    }

    // Get Count
    uint16_t displayCount = gp_displayRangeInfo[stringNum].p_lightsDisplay[displayNumber];
    uint16_t oldDisplayCount = 0;

    if (gp_displayRangeInfo[stringNum].dataChanged)
        oldDisplayCount = gp_displayRangeInfo[stringNum].p_lightsDisplay[oldDisplayNumber];

    uint8_t step;
    uint32_t color;

    // First Find the Step Placement
    if (gp_displayRangeInfo[stringNum].numberOfSteps > 1)
    {
        uint16_t lower = 0;

        for (uint8_t i = 0; i < gp_displayRangeInfo[stringNum].numberOfSteps; i++)
        {
            if (displayCount > lower && displayCount <= gp_displayRangeInfo[stringNum].p_lightsInStep[i])
            {
                // Found Step
                step = i;
                break;
            }
            else
                lower = gp_displayRangeInfo[stringNum].p_lightsInStep[i];
        }
    }
    else
        step = 0;

    color = SetColor(gp_displayRangeInfo[stringNum].p_redSteps[step], gp_displayRangeInfo[stringNum].p_greenSteps[step], gp_displayRangeInfo[stringNum].p_blueSteps[step], 0);

    if (oldDisplayCount > displayCount)
    {
        if (oldDisplayCount <= PIOTXMWRITE)
        {
            for (uint16_t i = 0; i < displayCount; i++)
                pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], color);

            for (uint16_t i = displayCount; i < oldDisplayCount; i++)
                pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], 0);

            // Latch Data In ALED Strip + Write Time
            gp_txData[stringNum].stringNumber = stringNum;
            uint16_t waitTime = (TIMEONELED * oldDisplayCount) + ALEDLATCHTIME;
            // Wait Write and Latch Time
            add_alarm_in_us(waitTime, LatchString, &gp_txData[stringNum], true);
        }
        else if (displayCount <= PIOTXMWRITE)
        {
            uint16_t delta = PIOTXMWRITE - displayCount;

            gp_txData[stringNum].color = 0;
            gp_txData[stringNum].count = displayCount + delta;
            gp_txData[stringNum].countMax = oldDisplayCount;
            gp_txData[stringNum].stringNumber = stringNum;

            for (uint16_t i = 0; i < displayCount; i++)
                pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], color);

            if (delta != 0)
            {
                for (uint16_t i = 0; i < delta; i++)
                    pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], 0);
            }

            add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[stringNum], &drSeqtimer[stringNum]);
        }
        else
        {
            gp_txData[stringNum].color = color;
            gp_txData[stringNum].count = PIOTXMWRITE;
            gp_txData[stringNum].countMax = displayCount;
            gp_txData[stringNum].color2 = 0;
            gp_txData[stringNum].count2 = displayCount;
            gp_txData[stringNum].countMax2 = oldDisplayCount;
            gp_txData[stringNum].stringNumber = stringNum;

            for (uint16_t i = 0; i < PIOTXMWRITE; i++)
                pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], color);

            add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_2C_CB, &gp_txData[stringNum], &drSeqtimer[stringNum]);
        }
    }
    else
    {
        if (displayCount > PIOTXMWRITE)
        {
            gp_txData[stringNum].color = color;
            gp_txData[stringNum].count = PIOTXMWRITE;
            gp_txData[stringNum].countMax = displayCount;
            gp_txData[stringNum].stringNumber = stringNum;

            for (uint16_t i = 0; i < PIOTXMWRITE; i++)
                pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], color);

            add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[stringNum], &drSeqtimer[stringNum]);
        }
        else
        {
            for (uint16_t i = 0; i < displayCount; i++)
                pio_sm_put_blocking(gp_pio[stringNum], g_sm[stringNum], color);

            // Latch Data In ALED Strip + Write Time
            gp_txData[stringNum].stringNumber = stringNum;
            uint16_t waitTime = (TIMEONELED * displayCount) + ALEDLATCHTIME;
            // Wait Write and Latch Time
            add_alarm_in_us(waitTime, LatchString, &gp_txData[stringNum], true);
        }
    }

    gp_displayRangeInfo[stringNum].dataChanged = false;
}

bool __not_in_flash_func(RT_WriteDataPIOTxBuffer_CB)(struct repeating_timer *t)
{
    // Stop Interrupts as this is timing critical
    uint32_t status = save_and_disable_interrupts();

    TxDataWrite *txData = (TxDataWrite *)t->user_data;
    uint8_t string = txData->stringNumber;

    if (txData->count < txData->countMax - PIOTXMWRITE)
    {
        txData->count += PIOTXMWRITE;

        for (uint8_t i = 0; i < PIOTXMWRITE; i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], txData->color);

        restore_interrupts(status);
        return true;
    }
    else if (txData->count < txData->countMax)
    {
        uint16_t delta = txData->countMax - txData->count;
        txData->count += delta;

        for (uint8_t i = 0; i < delta; i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], txData->color);

        restore_interrupts(status);
        return true;
    }
    else if (txData->returnToFunction)
    {
        if (txData->drSeqReload)
        {
            txData->returnToFunction = false;
            txData->drSeqReload = false;
            add_alarm_in_us(gp_displayRangeInfo[string].timeDelay, DODRSeq, &gp_displayRangeInfo[string], false);
        }
        else if (txData->flash)
        {
            txData->returnToFunction = false;
            txData->flash = false;
            if (txData->whatFunction == 0)
                add_alarm_in_ms(gp_displayRangeInfo[string].timeOff, FlashOn, &gp_flashData[txData->structNumber], false);
            else if (txData->whatFunction == 1)
                add_alarm_in_ms(gp_flashData[txData->structNumber].timeOn, FlashOff, &gp_flashData[txData->structNumber], false);
            else
                add_alarm_in_ms(gp_flashData[txData->structNumber].timeOff, FlashComplete, &gp_flashData[txData->structNumber], false);
        }
        else if (txData->rndFlash)
        {
            txData->returnToFunction = false;
            txData->rndFlash = false;
            if (txData->whatFunction == 0)
                add_alarm_in_ms(gp_displayRangeInfo[string].timeOff, RndFlashON, &gp_rndFlashData[txData->structNumber], false);
            else if (txData->whatFunction == 1)
                add_alarm_in_ms(gp_rndFlashData[txData->structNumber].timeOn, RndFlashOff, &gp_rndFlashData[txData->structNumber], false);
            else
                add_alarm_in_ms(gp_rndFlashData[txData->structNumber].timeOff, RndFlashComplete, &gp_rndFlashData[txData->structNumber], false);
        }
        else if (txData->seqiential)
        {
            txData->returnToFunction = false;
            txData->seqiential = false;
            if (txData->whatFunction == 0)
                add_alarm_in_ms(gp_displayRangeInfo[string].timeOff, SeqFirst, &gp_seqData[txData->structNumber], false);
            else if (txData->whatFunction == 1)
                add_alarm_in_us(gp_seqData[txData->structNumber].timeDelay + ALEDLATCHTIME, SeqNext, &gp_seqData[txData->structNumber], false);
            else if (txData->whatFunction == 2)
                add_alarm_in_us(gp_seqData[txData->structNumber].timeDelay, SeqComplete, &gp_seqData[txData->structNumber], false);
        }
        else
            txData->returnToFunction = false;

        restore_interrupts(status);
        return false;
    }
    else
    {
        add_alarm_in_us(ALEDLATCHTIME, LatchString, txData, true);
        restore_interrupts(status);
        return false;
    }

    // Something Got Fucked Up
    restore_interrupts(status);
    return false;
}

bool __not_in_flash_func(RT_WriteDataPIOTxBuffer_2C_CB)(struct repeating_timer *t)
{
    // Stop Interrupts as this is timing critical
    uint32_t status = save_and_disable_interrupts();

    TxDataWrite *txData = (TxDataWrite *)t->user_data;
    uint8_t string = txData->stringNumber;

    if (txData->count < txData->countMax - PIOTXMWRITE)
    {
        txData->count += PIOTXMWRITE;

        for (uint8_t i = 0; i < PIOTXMWRITE; i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], txData->color);

        restore_interrupts(status);
        return true;
    }
    else if (txData->count < txData->countMax)
    {
        uint16_t delta = txData->countMax - txData->count;
        uint16_t delta2, deltaO;

        txData->count += delta;

        deltaO = PIOTXMWRITE - delta;
        delta2 = txData->countMax2 - txData->count2;

        if (delta2 < deltaO)
            deltaO = delta2;

        txData->count2 += deltaO;

        for (uint8_t i = 0; i < delta; i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], txData->color);

        for (uint8_t i = 0; i < deltaO; i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], txData->color2);

        restore_interrupts(status);
        return true;
    }
    else if (txData->count2 < txData->countMax2 - PIOTXMWRITE)
    {
        txData->count2 += PIOTXMWRITE;

        for (uint8_t i = 0; i < PIOTXMWRITE; i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], txData->color2);

        restore_interrupts(status);
        return true;
    }
    else if (txData->count2 < txData->countMax2)
    {
        uint16_t delta = txData->countMax2 - txData->count2;

        if (delta > PIOTXMWRITE)
            delta = PIOTXMWRITE;

        txData->count2 += delta;

        for (uint8_t i = 0; i < delta; i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], txData->color2);

        restore_interrupts(status);
        return true;
    }
    else if (txData->returnToFunction)
    {
        if (txData->rndFlash)
        {
            txData->returnToFunction = false;
            txData->rndFlash = false;
            if (txData->whatFunction == 1)
                add_alarm_in_ms(gp_rndFlashData[txData->structNumber].timeOn, RndFlashOff, &gp_rndFlashData[txData->structNumber], false);
        }

        restore_interrupts(status);
        return false;
    }
    else
    {
        add_alarm_in_us(ALEDLATCHTIME, LatchString, txData, true);
        restore_interrupts(status);
        return false;
    }

    // Something Got Fucked Up
    restore_interrupts(status);
    return false;
}

int64_t __not_in_flash_func(LatchString)(alarm_id_t id, __unused void *user_data)
{
    TxDataWrite *txData = user_data;
    uint8_t string = txData->stringNumber;

    uint32_t mOwner;
    if (mutex_try_enter(&g_mutexStrip[string], &mOwner))
    {
        // Release the ALED String
        g_isRunning[string] = false;
        mutex_exit(&g_mutexStrip[string]);
    }
    else
        add_alarm_in_us(MUTEXIRQWAIT, LatchString, txData, true);

    return 0;
}

int64_t __not_in_flash_func(DODRSeq)(alarm_id_t id, __unused void *user_data)
{
    DisplayRange *drData = user_data;
    uint8_t string = drData->stringNumber;

    // Increase Sequential LED Count
    drData->reloadCount++;

    // Check Sequential Number and Turn on Next Light
    if (drData->reloadCount < drData->reloadMax)
    {
        if (drData->numLEDs > 1)
        {
            // Go -1, as Increamented by 1 above
            drData->reloadCount += drData->numLEDs - 1;

            // Check to Make Sure you didn't Go Over
            if (drData->reloadCount > drData->reloadMax)
                drData->reloadCount = drData->reloadMax;
        }

        if (drData->reloadCount > PIOTXMWRITE)
        {
            gp_txData[string].stringNumber = string;
            gp_txData[string].count = PIOTXMWRITE;
            gp_txData[string].countMax = drData->reloadCount;
            gp_txData[string].color = drData->reloadColor;
            gp_txData[string].returnToFunction = true;
            gp_txData[string].drSeqReload = true;

            for (uint8_t i = 0; i < PIOTXMWRITE; i++)
                pio_sm_put_blocking(gp_pio[string], g_sm[string], drData->reloadColor);

            add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[string], &drSeqtimer[string]);
        }
        else
        {
            for (uint8_t i = 0; i < drData->reloadCount; i++)
                pio_sm_put_blocking(gp_pio[string], g_sm[string], drData->reloadColor);

            add_alarm_in_us(drData->timeDelay, DODRSeq, drData, false);
        }
    }
    else if (!drData->isLatched)
    {
        drData->isLatched = true;
        // The Last Light is On and Wait for the Delay to End
        add_alarm_in_us(ALEDLATCHTIME, DODRSeq, drData, true);
    }
    else
    {
        // If Data Has Changed, then Update Display, If Not Then End
        if (gp_displayRangeInfo[string].dataChanged)
        {
            drData->isLatched = false;
            DisplayRangeData(string);
        }
        else
        {
            uint32_t mOwner;
            if (mutex_try_enter(&g_mutexStrip[string], &mOwner))
            {
                drData->isLatched = false;

                // Turn Of Running a Display Command is Complete
                g_isRunning[string] = false;
                mutex_exit(&g_mutexStrip[string]);
            }
            else
            {
                // Wait Some Time and Try Mutex Again. Make Time Func Small as Possible
                add_alarm_in_us(MUTEXIRQWAIT, DODRSeq, drData, true);
            }
        }
    }

    // Can return a value here in us to fire in the future
    return 0;
}

//--------------------------------------------------------------------+
// Functions - When In a Game - Flash
//--------------------------------------------------------------------+

void DoFlash(uint8_t structNum)
{
    uint8_t string = gp_flashData[structNum].stringNumber;
    int8_t rangeDisNum = -1;

    // Check if Range Display is Running
    if (g_isRDRunning[string] && gp_flashData[structNum].timesFlashed == 0)
        rangeDisNum = string;

    gp_txData[string].stringNumber = string;
    gp_txData[string].structNumber = structNum;
    gp_txData[string].count = PIOTXMWRITE;
    gp_txData[string].countMax = gp_ledStringElements[string];
    gp_txData[string].returnToFunction = true;
    gp_txData[string].flash = true;

    if (rangeDisNum == -1)
    {
        gp_txData[string].color = gp_flashData[structNum].color;
        gp_txData[string].whatFunction = 1;
    }
    else
    {
        gp_txData[string].color = 0;
        gp_txData[string].whatFunction = 0;
    }

    for (uint8_t i = 0; i < PIOTXMWRITE; i++)
        pio_sm_put_blocking(gp_pio[string], g_sm[string], gp_txData[string].color);

    add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[string], &drSeqtimer[string]);
}

int64_t FlashOn(alarm_id_t id, __unused void *user_data)
{
    FlashData *flashData = user_data;
    uint8_t string = flashData->stringNumber;

    gp_txData[string].stringNumber = string;
    gp_txData[string].structNumber = flashData->structNumber;
    gp_txData[string].count = PIOTXMWRITE;
    gp_txData[string].countMax = gp_ledStringElements[string];
    gp_txData[string].returnToFunction = true;
    gp_txData[string].flash = true;
    gp_txData[string].color = flashData->color;
    gp_txData[string].whatFunction = 1;

    for (uint8_t i = 0; i < PIOTXMWRITE; i++)
        pio_sm_put_blocking(gp_pio[string], g_sm[string], flashData->color);

    add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[string], &drSeqtimer[string]);

    // Can return a value here in us to fire in the future
    return 0;
}

int64_t FlashOff(alarm_id_t id, __unused void *user_data)
{
    FlashData *flashData = user_data;
    uint8_t string = flashData->stringNumber;

    gp_txData[string].stringNumber = string;
    gp_txData[string].structNumber = flashData->structNumber;
    gp_txData[string].count = PIOTXMWRITE;
    gp_txData[string].countMax = gp_ledStringElements[string];
    gp_txData[string].returnToFunction = true;
    gp_txData[string].flash = true;
    gp_txData[string].color = 0;
    gp_txData[string].whatFunction = 2;

    for (uint8_t i = 0; i < PIOTXMWRITE; i++)
        pio_sm_put_blocking(gp_pio[string], g_sm[string], 0);

    add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[string], &drSeqtimer[string]);

    // Can return a value here in us to fire in the future
    return 0;
}

int64_t FlashComplete(alarm_id_t id, __unused void *user_data)
{
    FlashData *flashData = user_data;

    flashData->timesFlashed++;

    if (flashData->timesFlashed < flashData->numberOfFlashes)
        DoFlash(flashData->structNumber);
    else
    {
        flashData->timesFlashed = 0;

        if (g_isRDRunning[flashData->stringNumber])
            DisplayRangeData(flashData->stringNumber);
        else
        {
            mutex_enter_blocking(&g_mutexStrip[flashData->stringNumber]);
            g_isRunning[flashData->stringNumber] = false;
            mutex_exit(&g_mutexStrip[flashData->stringNumber]);
        }
    }

    // Can return a value here in us to fire in the future
    return 0;
}

//--------------------------------------------------------------------+
// Functions - When In a Game - Random Flash
//--------------------------------------------------------------------+

void DoRndFlash(uint8_t structNum)
{
    uint8_t string = gp_rndFlashData[structNum].stringNumber;
    int8_t rangeDisNum = -1;

    if (gp_rndFlashData[structNum].timesFlashed == 0)
    {
        gp_rndFlashData[structNum].rndNumber = rand() % (gp_ledStringElements[string] - gp_rndFlashData[structNum].ledsOn);

        if (gp_rndFlashData[structNum].enable2ndColor)
        {
            uint8_t rnd2ndColor = rand() % gp_rndFlashData[structNum].probability;

            if ((rnd2ndColor + 1) == gp_rndFlashData[structNum].probability)
                gp_rndFlashData[structNum].use2ndColor = true;
        }

        // Check if Range Display is Running
        if (g_isRDRunning[string])
            rangeDisNum = string;
    }

    gp_txData[string].stringNumber = string;
    gp_txData[string].structNumber = structNum;
    gp_txData[string].count = PIOTXMWRITE;
    gp_txData[string].returnToFunction = true;
    gp_txData[string].rndFlash = true;

    if (rangeDisNum == -1)
    {
        gp_txData[string].whatFunction = 1;

        if (gp_rndFlashData[structNum].rndNumber > PIOTXMWRITE)
        {
            gp_txData[string].countMax = gp_rndFlashData[structNum].rndNumber;
            gp_txData[string].color = 0;
            gp_txData[string].count2 = gp_rndFlashData[structNum].rndNumber;
            gp_txData[string].countMax2 = gp_rndFlashData[structNum].rndNumber + gp_rndFlashData[structNum].ledsOn;
            if (gp_rndFlashData[structNum].use2ndColor)
                gp_txData[string].color2 = gp_rndFlashData[structNum].color2;
            else
                gp_txData[string].color2 = gp_rndFlashData[structNum].color;

            for (uint8_t i = 0; i < PIOTXMWRITE; i++)
                pio_sm_put_blocking(gp_pio[string], g_sm[string], 0);

            add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_2C_CB, &gp_txData[string], &drSeqtimer[string]);
        }
        else
        {
            uint16_t delta = PIOTXMWRITE - gp_rndFlashData[structNum].rndNumber;
            uint16_t all = gp_rndFlashData[structNum].rndNumber + gp_rndFlashData[structNum].ledsOn;

            if (delta > gp_rndFlashData[structNum].ledsOn)
                delta = gp_rndFlashData[structNum].ledsOn;

            if (gp_rndFlashData[structNum].use2ndColor)
                gp_txData[string].color = gp_rndFlashData[structNum].color2;
            else
                gp_txData[string].color = gp_rndFlashData[structNum].color;
            gp_txData[string].countMax = gp_rndFlashData[structNum].ledsOn - delta;

            for (uint8_t i = 0; i < gp_rndFlashData[structNum].rndNumber; i++)
                pio_sm_put_blocking(gp_pio[string], g_sm[string], 0);

            if (delta != 0)
            {
                for (uint8_t i = 0; i < delta; i++)
                    pio_sm_put_blocking(gp_pio[string], g_sm[string], gp_txData[string].color);
            }

            if (all > PIOTXMWRITE)
                add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[string], &drSeqtimer[string]);
            else
                add_alarm_in_ms(gp_rndFlashData[structNum].timeOn, RndFlashOff, &gp_rndFlashData[structNum], false);
        }
    }
    else
    {
        gp_txData[string].whatFunction = 0;
        gp_txData[string].countMax = gp_ledStringElements[string];
        gp_txData[string].color = 0;

        for (uint8_t i = 0; i < PIOTXMWRITE; i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], 0);

        add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[string], &drSeqtimer[string]);
    }
}

int64_t RndFlashON(alarm_id_t id, __unused void *user_data)
{
    RndFlashData *rndFlashData = user_data;
    uint8_t string = rndFlashData->stringNumber;

    gp_txData[string].stringNumber = string;
    gp_txData[string].structNumber = rndFlashData->structNumber;
    gp_txData[string].count = PIOTXMWRITE;
    gp_txData[string].returnToFunction = true;
    gp_txData[string].rndFlash = true;
    gp_txData[string].whatFunction = 1;

    if (rndFlashData->rndNumber > PIOTXMWRITE)
    {
        gp_txData[string].countMax = rndFlashData->rndNumber;
        gp_txData[string].color = 0;
        gp_txData[string].count2 = rndFlashData->rndNumber;
        gp_txData[string].countMax2 = rndFlashData->rndNumber + rndFlashData->ledsOn;
        if (rndFlashData->use2ndColor)
            gp_txData[string].color = rndFlashData->color2;
        else
            gp_txData[string].color = rndFlashData->color;

        for (uint8_t i = 0; i < PIOTXMWRITE; i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], 0);

        add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_2C_CB, &gp_txData[string], &drSeqtimer[string]);
    }
    else
    {
        uint16_t delta = PIOTXMWRITE - rndFlashData->rndNumber;
        uint16_t all = rndFlashData->rndNumber + rndFlashData->ledsOn;

        if (delta > rndFlashData->ledsOn)
            delta = rndFlashData->ledsOn;

        if (rndFlashData->use2ndColor)
            gp_txData[string].color = rndFlashData->color2;
        else
            gp_txData[string].color = rndFlashData->color;
        gp_txData[string].countMax = rndFlashData->ledsOn - delta;

        for (uint8_t i = 0; i < rndFlashData->rndNumber; i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], 0);

        if (delta != 0)
        {
            for (uint8_t i = 0; i < delta; i++)
                pio_sm_put_blocking(gp_pio[string], g_sm[string], gp_txData[string].color);
        }

        if (all > PIOTXMWRITE)
            add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[string], &drSeqtimer[string]);
        else
            add_alarm_in_ms(rndFlashData->timeOn, RndFlashOff, rndFlashData, false);
    }

    // Can return a value here in us to fire in the future
    return 0;
}

int64_t RndFlashOff(alarm_id_t id, __unused void *user_data)
{
    RndFlashData *rndFlashData = user_data;
    uint8_t string = rndFlashData->stringNumber;

    if (rndFlashData->rndNumber + rndFlashData->ledsOn > PIOTXMWRITE)
    {
        gp_txData[string].stringNumber = string;
        gp_txData[string].structNumber = rndFlashData->structNumber;
        gp_txData[string].count = PIOTXMWRITE;
        gp_txData[string].returnToFunction = true;
        gp_txData[string].rndFlash = true;
        gp_txData[string].whatFunction = 2;
        gp_txData[string].countMax = rndFlashData->rndNumber + rndFlashData->ledsOn;
        gp_txData[string].color = 0;

        for (uint8_t i = 0; i < PIOTXMWRITE; i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], 0);

        add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[string], &drSeqtimer[string]);
    }
    else
    {
        // Turn String Off
        for (uint16_t i = 0; i < (rndFlashData->rndNumber + rndFlashData->ledsOn); i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], 0);

        // Timer to Turn Off Lights
        add_alarm_in_ms(rndFlashData->timeOff, RndFlashComplete, rndFlashData, false);
    }

    // Can return a value here in us to fire in the future
    return 0;
}

int64_t RndFlashComplete(alarm_id_t id, __unused void *user_data)
{
    RndFlashData *rndFlashData = user_data;

    rndFlashData->timesFlashed++;

    if (rndFlashData->timesFlashed < rndFlashData->numberOfFlashes)
        DoRndFlash(rndFlashData->structNumber);
    else
    {
        rndFlashData->timesFlashed = 0;
        rndFlashData->use2ndColor = false;

        if (g_isRDRunning[rndFlashData->stringNumber])
            DisplayRangeData(rndFlashData->stringNumber);
        else
        {
            mutex_enter_blocking(&g_mutexStrip[rndFlashData->stringNumber]);
            g_isRunning[rndFlashData->stringNumber] = false;
            mutex_exit(&g_mutexStrip[rndFlashData->stringNumber]);
        }
    }

    // Can return a value here in us to fire in the future
    return 0;
}

//--------------------------------------------------------------------+
// Functions - When In a Game - Sequential
//--------------------------------------------------------------------+

void DoSeq(uint8_t structNum)
{
    uint8_t string = gp_seqData[structNum].stringNumber;
    int8_t rangeDisNum = -1;

    // Check if Range Display is Running
    if (g_isRDRunning[string] && gp_seqData[structNum].seqLED == 1)
        rangeDisNum = string;

    gp_seqData[structNum].seqLED = gp_seqData[structNum].numLEDs;

    if (rangeDisNum == -1)
    {
        uint16_t waitTime = (gp_seqData[structNum].seqLED * TIMEONELED) + ALEDLATCHTIME;
        waitTime += gp_seqData[structNum].timeDelay;

        // Turn on First LED - Always 8 LEDs or Less
        for (uint16_t i = 0; i < gp_seqData[structNum].seqLED; i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], gp_seqData[structNum].color);

        // Timer to Turn On Next LED
        add_alarm_in_us(waitTime, SeqNext, &gp_seqData[structNum], false);
    }
    else
    {
        gp_txData[string].stringNumber = string;
        gp_txData[string].structNumber = structNum;
        gp_txData[string].count = PIOTXMWRITE;
        gp_txData[string].countMax = gp_ledStringElements[string];
        gp_txData[string].returnToFunction = true;
        gp_txData[string].seqiential = true;
        gp_txData[string].whatFunction = 0;
        gp_txData[string].color = 0;

        // Turn String Off
        for (uint16_t i = 0; i < PIOTXMWRITE; i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], 0);

        add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[string], &drSeqtimer[string]);
    }
}

int64_t SeqFirst(alarm_id_t id, __unused void *user_data)
{
    SeqData *seqData = user_data;
    uint8_t string = seqData->stringNumber;

    uint16_t waitTime = (seqData->seqLED * TIMEONELED) + ALEDLATCHTIME;
    waitTime += seqData->timeDelay;

    // Turn on First LED - Always 8 LEDs or Less
    for (uint16_t i = 0; i < seqData->seqLED; i++)
        pio_sm_put_blocking(gp_pio[string], g_sm[string], seqData->color);

    // Then Do Next Sequential LED after Delay
    add_alarm_in_us(waitTime, SeqNext, seqData, false);

    // Can return a value here in us to fire in the future
    return 0;
}

int64_t SeqNext(alarm_id_t id, __unused void *user_data)
{
    SeqData *seqData = user_data;
    uint8_t string = seqData->stringNumber;

    // Increase Sequential LED Count
    seqData->seqLED++;

    // Check Sequential Number and Turn on Next Light
    if (seqData->seqLED < gp_ledStringElements[string])
    {
        if (seqData->numLEDs > 1)
        {
            seqData->seqLED += seqData->numLEDs - 1;

            if (seqData->seqLED > gp_ledStringElements[string])
                seqData->seqLED = gp_ledStringElements[string];
        }

        if (seqData->seqLED > PIOTXMWRITE)
        {
            gp_txData[string].stringNumber = string;
            gp_txData[string].structNumber = seqData->structNumber;
            gp_txData[string].count = PIOTXMWRITE;
            gp_txData[string].countMax = seqData->seqLED;
            gp_txData[string].returnToFunction = true;
            gp_txData[string].seqiential = true;
            gp_txData[string].whatFunction = 1;
            gp_txData[string].color = seqData->color;

            for (uint16_t i = 0; i < PIOTXMWRITE; i++)
                pio_sm_put_blocking(gp_pio[string], g_sm[string], seqData->color);

            add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[string], &drSeqtimer[string]);
        }
        else
        {
            for (uint16_t i = 0; i < seqData->seqLED; i++)
                pio_sm_put_blocking(gp_pio[string], g_sm[string], seqData->color);

            // Timer to Turn On Next LED
            add_alarm_in_us(seqData->timeDelay + ALEDLATCHTIME, SeqNext, seqData, false);
        }
    }
    else
    {
        // The Last Light is On and Wait for the Delay to End
        add_alarm_in_us(seqData->timeDelay + ALEDLATCHTIME, SeqComplete, seqData, false);
    }

    // Can return a value here in us to fire in the future
    return 0;
}

int64_t SeqComplete(alarm_id_t id, __unused void *user_data)
{
    SeqData *seqData = user_data;
    uint8_t string = seqData->stringNumber;

    if (seqData->seqLED == 1)
    {
        if (g_isRDRunning[string])
            DisplayRangeData(string);
        else
        {
            // Turn Off Command is Running
            mutex_enter_blocking(&g_mutexStrip[string]);
            g_isRunning[string] = false;
            mutex_exit(&g_mutexStrip[string]);
        }
    }
    else
    {
        seqData->seqLED = 1;

        gp_txData[string].stringNumber = string;
        gp_txData[string].structNumber = seqData->structNumber;
        gp_txData[string].count = PIOTXMWRITE;
        gp_txData[string].countMax = gp_ledStringElements[string];
        gp_txData[string].color = 0;
        gp_txData[string].returnToFunction = true;
        gp_txData[string].seqiential = true;
        gp_txData[string].whatFunction = 2;

        // Turn Off LEDs
        for (uint16_t i = 0; i < PIOTXMWRITE; i++)
            pio_sm_put_blocking(gp_pio[string], g_sm[string], 0);

        add_repeating_timer_us(PIOTXTIME, RT_WriteDataPIOTxBuffer_CB, &gp_txData[string], &drSeqtimer[string]);
    }

    // Can return a value here in us to fire in the future
    return 0;
}
