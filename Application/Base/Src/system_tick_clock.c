/*
    FreeRTOS V8.2.0rc1 - Copyright (C) 2014 Real Time Engineers Ltd.
    All rights reserved
    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.
    This file is part of the FreeRTOS distribution.
    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html
    1 tab == 4 spaces!
    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?".  Have you defined configASSERT()?  *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************
    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************
    ***************************************************************************
     *                                                                       *
     *   Investing in training allows your team to be as productive as       *
     *   possible as early as possible, lowering your overall development    *
     *   cost, and enabling you to bring a more robust product to market     *
     *   earlier than would otherwise be possible.  Richard Barry is both    *
     *   the architect and key author of FreeRTOS, and so also the world's   *
     *   leading authority on what is the world's most popular real time     *
     *   kernel for deeply embedded MCU designs.  Obtaining your training    *
     *   from Richard ensures your team will gain directly from his in-depth *
     *   product knowledge and years of usage experience.  Contact Real Time *
     *   Engineers Ltd to enquire about the FreeRTOS Masterclass, presented  *
     *   by Richard Barry:  http://www.FreeRTOS.org/contact
     *                                                                       *
    ***************************************************************************
    ***************************************************************************
     *                                                                       *
     *    You are receiving this top quality software for free.  Please play *
     *    fair and reciprocate by reporting any suspected issues and         *
     *    participating in the community forum:                              *
     *    http://www.FreeRTOS.org/support                                    *
     *                                                                       *
     *    Thank you!                                                         *
     *                                                                       *
    ***************************************************************************
    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.
    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.
    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.
    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.
    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.
    1 tab == 4 spaces!
*/

/* Standard includes. */
#include <limits.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/* ST library functions. */
#include "stm32l1xx.h"
#include "stm32l1xx_hal_tim.h"
#include "stm32l1xx_hal_pwr.h"

/*
 * When configCREATE_LOW_POWER_DEMO is set to 1 then the tick interrupt
 * is generated by the TIM2 peripheral.  The TIM2 configuration and handling
 * functions are defined in this file.  Note the RTC is not used as there does
 * not appear to be a way to read back the RTC count value, and therefore the
 * only way of knowing exactly how long a sleep lasted is to use the very low
 * resolution calendar time.
 *
 * When configCREATE_LOW_POWER_DEMO is set to 0 the tick interrupt is
 * generated by the standard FreeRTOS Cortex-M port layer, which uses the
 * SysTick timer.
 */

/* The frequency at which TIM2 will run. */
#define lpCLOCK_INPUT_FREQUENCY     ( 1000UL )

/* STM32 register used to ensure the TIM2 clock stops when the MCU is in debug
mode. */
#define DBGMCU_APB1_FZ  ( * ( ( volatile unsigned long * ) 0xE0042008 ) )

/* Constants required to manipulate the core.  Registers first... */
#define portNVIC_SYSTICK_CTRL_REG           ( * ( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG  ( * ( ( volatile uint32_t * ) 0xe000e018 ) )
/* ...then bits in the registers. */
#define portNVIC_SYSTICK_ENABLE_BIT         ( 1UL << 0UL )

/*-----------------------------------------------------------*/

/*
 * The tick interrupt is generated by the TIM2 timer.
 */
void TIM2_IRQHandler( void );

/*-----------------------------------------------------------*/

/* Calculate how many clock increments make up a single tick period. */
static const uint32_t ulReloadValueForOneTick = ( ( lpCLOCK_INPUT_FREQUENCY / configTICK_RATE_HZ ) );

/* Holds the maximum number of ticks that can be suppressed - which is
basically how far into the future an interrupt can be generated. Set during
initialisation. */
static TickType_t xMaximumPossibleSuppressedTicks = 0;

/* Flag set from the tick interrupt to allow the sleep processing to know if
sleep mode was exited because of an tick interrupt or a different interrupt. */
static volatile uint32_t ulTickFlag = pdFALSE;


TIM_HandleTypeDef htim2;

/*-----------------------------------------------------------*/

/**
 * @brief: The tick interrupt handler.  This is always the same other than the part that
 * clears the interrupt, which is specific to the clock being used to generate the
 * tick.
 **/
void
TIM2_IRQHandler( void )
{
    /* Clear the interrupt. */
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);

    /* The next block of code is from the standard FreeRTOS tick interrupt
    handler.  The standard handler is not called directly in case future
    versions contain changes that make it no longer suitable for calling
    here. */
    osSystickHandler();

    /* In case this is the first tick since the MCU left a low power mode the
    reload value is reset to its default. */
    htim2.Instance->ARR = ( uint16_t ) ulReloadValueForOneTick;

    /* The CPU woke because of a tick. */
    ulTickFlag = pdTRUE;
}
/*-----------------------------------------------------------*/

/**
 * @brief: Override the default definition of vPortSetupTimerInterrupt() that is weakly
 * defined in the FreeRTOS Cortex-M3 port layer with a version that configures TIM2
 * to generate the tick interrupt.
 **/
void
vPortSetupTimerInterrupt( void )
{

    /* Enable the TIM2 clock. */
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* Ensure clock stops in debug mode. */
    DBGMCU_APB1_FZ |= DBGMCU_APB1_FZ_DBG_TIM2_STOP;

    /**
     * Scale the clock so longer tickless periods can be achieved.  The SysTick
     * is not used as even when its frequency is divided by 8 the maximum tickless
     * period with a system clock of 16MHz is only 8.3 seconds.  Using a prescaled
     * clock on the 16-bit TIM2 allows a tickless period of nearly 66 seconds,
     * albeit at low resolution.
     * Make TIM2 to update at 100Hz.
     * SystemCoreClock = 32MHz
     * Prescaler = 32000 - 1
     * Period = 10 - 1
     *
     * TIM_Update = TIM clock / (X * Y) = 32MHz / (32000 * 10) = 100Hz
     *
     * 100Hz gives 10 reloads for 1 Tick, which gives max number = 6553 of suppresed ticks,
     * which is ~66 seconds
     */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = ( uint16_t ) (SystemCoreClock / 1000) - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = ( uint16_t ) ( lpCLOCK_INPUT_FREQUENCY / configTICK_RATE_HZ ) - 1;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim2) == HAL_OK)
    {
        /* Enable the TIM2 interrupt.  This must execute at the lowest interrupt
        priority. */
        HAL_NVIC_SetPriority(TIM2_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
        htim2.Instance->CNT = 0;
        /* Clear possible pending update interrupt */
        __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
        /* Start TIM2 */
        HAL_TIM_Base_Start_IT(&htim2);
    }

    /* See the comments where xMaximumPossibleSuppressedTicks is declared. */
    xMaximumPossibleSuppressedTicks = ( ( unsigned long ) USHRT_MAX ) / ulReloadValueForOneTick;
}
/*-----------------------------------------------------------*/

/**
* @brief: Override the default definition of vPortSuppressTicksAndSleep() that is
* weakly defined in the FreeRTOS Cortex-M3 port layer with a version that manages
* the TIM2 interrupt, as the tick is generated from TIM2 compare matches events.
* @param xExpectedIdleTime: expected time to sleep in ticks (1 tick = 10ms)
*/
void
vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
{
    uint32_t ulCounterValue = 0U;
    uint32_t ulCompleteTickPeriods = 0U;
    eSleepModeStatus eSleepAction = eAbortSleep;
    TickType_t xModifiableIdleTime = (TickType_t) 0U;

    /* THIS FUNCTION IS CALLED WITH THE SCHEDULER SUSPENDED. */

    /* Make sure the TIM2 reload value does not overflow the counter. */
    if( xExpectedIdleTime > xMaximumPossibleSuppressedTicks )
    {
        xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
    }

    /* Calculate the reload value required to wait xExpectedIdleTime tick
    periods. */
    ulCounterValue = ulReloadValueForOneTick * xExpectedIdleTime;

    /* Stop Systick */
    portNVIC_SYSTICK_CTRL_REG &= ~portNVIC_SYSTICK_ENABLE_BIT;

    /* Stop TIM2 momentarily.  The time TIM2 is stopped for is not accounted for
    in this implementation (as it is in the generic implementation) because the
    clock is so slow it is unlikely to be stopped for a complete count period
    anyway. */
    HAL_TIM_Base_Stop_IT(&htim2);

    /* Enter a critical section but don't use the taskENTER_CRITICAL() method as
    that will mask interrupts that should exit sleep mode. */
    __asm volatile ( "cpsid i" );
    __asm volatile ( "dsb" );
    __asm volatile ( "isb" );

    /* The tick flag is set to false before sleeping.  If it is true when sleep
    mode is exited then sleep mode was probably exited because the tick was
    suppressed for the entire xExpectedIdleTime period. */
    ulTickFlag = pdFALSE;

    /* If a context switch is pending then abandon the low power entry as
    the context switch might have been pended by an external interrupt that
    requires processing. */
    eSleepAction = eTaskConfirmSleepModeStatus();
    if( eSleepAction == eAbortSleep )
    {
        /* Restart tick. */
        HAL_TIM_Base_Start_IT(&htim2);

        /* Re-enable interrupts - see comments above the cpsid instruction()
        above. */
        __asm volatile ( "cpsie i" );
    }
    else
    {
        /* Trap underflow before the next calculation. */
        configASSERT( ulCounterValue >= htim2.Instance->CNT );

        /* Adjust the TIM2 value to take into account that the current time
        slice is already partially complete. */
        ulCounterValue -= ( uint32_t ) htim2.Instance->CNT;

        /* Trap overflow/underflow before the calculated value is written to
        TIM2. */
        configASSERT( ulCounterValue < ( uint32_t ) USHRT_MAX );
        configASSERT( ulCounterValue != 0 );

        /* Update to use the calculated overflow value. */
        __HAL_TIM_SET_AUTORELOAD( &htim2, ( uint16_t ) ulCounterValue );
        htim2.Instance->CNT = 0;

        /* Restart the TIM2. */
        HAL_TIM_Base_Start_IT(&htim2);

        /* Allow the application to define some pre-sleep processing.  This is
        the standard configPRE_SLEEP_PROCESSING() macro as described on the
        FreeRTOS.org website. */
        xModifiableIdleTime = xExpectedIdleTime;
        configPRE_SLEEP_PROCESSING( &xModifiableIdleTime );

        /* xExpectedIdleTime being set to 0 by configPRE_SLEEP_PROCESSING()
        means the application defined code has already executed the wait/sleep
        instruction. */
        if( xModifiableIdleTime > 0 )
        {
            /* Go sleep */
            HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);  /* PWR_LOWPOWERREGULATOR_ON */
        }

        /* Allow the application to define some post sleep processing.  This is
        the standard configPOST_SLEEP_PROCESSING() macro, as described on the
        FreeRTOS.org website. */
        configPOST_SLEEP_PROCESSING( &xModifiableIdleTime );

        /* Stop TIM2.  Again, the time the clock is stopped for in not accounted
        for here (as it would normally be) because the clock is so slow it is
        unlikely it will be stopped for a complete count period anyway. */
        HAL_TIM_Base_Stop_IT(&htim2);

        /* Re-enable interrupts - see comments above the cpsid instruction()
        above. */
        __asm volatile ( "cpsie i" );
        __asm volatile ( "dsb" );
        __asm volatile ( "isb" );

        if( ulTickFlag != pdFALSE )
        {
            /* Trap overflows before the next calculation. */
            configASSERT( ulReloadValueForOneTick >= ( uint32_t ) htim2.Instance->CNT );

            /* The tick interrupt has already executed, although because this
            function is called with the scheduler suspended the actual tick
            processing will not occur until after this function has exited.
            Reset the reload value with whatever remains of this tick period. */
            ulCounterValue = ulReloadValueForOneTick - ( uint32_t ) htim2.Instance->CNT;

            /* Trap under/overflows before the calculated value is used. */
            configASSERT( ulCounterValue <= ( uint32_t ) USHRT_MAX );
            configASSERT( ulCounterValue != 0 );

            /* Use the calculated reload value. */
            __HAL_TIM_SET_AUTORELOAD( &htim2, ( uint16_t ) ulCounterValue );
            htim2.Instance->CNT = 0;

            /* The tick interrupt handler will already have pended the tick
            processing in the kernel.  As the pending tick will be processed as
            soon as this function exits, the tick value maintained by the tick
            is stepped forward by one less than the time spent sleeping.  The
            actual stepping of the tick appears later in this function. */
            ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
        }
        else
        {
            /* Something other than the tick interrupt ended the sleep.  How
            many complete tick periods passed while the processor was
            sleeping? */
            ulCompleteTickPeriods = ( ( uint32_t ) htim2.Instance->CNT ) / ulReloadValueForOneTick;

            /* Check for over/under flows before the following calculation. */
            configASSERT( ( ( uint32_t ) htim2.Instance->CNT ) >= ( ulCompleteTickPeriods * ulReloadValueForOneTick ) );

            /* The reload value is set to whatever fraction of a single tick
            period remains. */
            ulCounterValue = ( ( uint32_t ) htim2.Instance->CNT ) - ( ulCompleteTickPeriods * ulReloadValueForOneTick );
            configASSERT( ulCounterValue <= ( uint32_t ) USHRT_MAX );
            if( ulCounterValue == 0 )
            {
                /* There is no fraction remaining. */
                ulCounterValue = ulReloadValueForOneTick;
                ulCompleteTickPeriods++;
            }
            __HAL_TIM_SET_AUTORELOAD( &htim2, ( uint16_t ) ulCounterValue );
            htim2.Instance->CNT = 0;
        }

        /* Restart TIM2 so it runs up to the reload value.  The reload value
        will get set to the value required to generate exactly one tick period
        the next time the TIM2 interrupt executes. */
        HAL_TIM_Base_Start_IT(&htim2);

        /* Clear the SysTick count flag and set the count value back to
        zero. */
        portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

        /* Restart SysTick. */
        portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

        /* Wind the tick forward by the number of tick periods that the CPU
        remained in a low power state. */
        vTaskStepTick( ulCompleteTickPeriods );
    }
}
