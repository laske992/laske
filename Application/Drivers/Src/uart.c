/*
 * uart.c
 *
 *  Created on: Sep 1, 2018
 *      Author: Mislav
 */
#include <stdbool.h>
#include "uart.h"
#include "string.h"
#include "led.h"

UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim3;
bool uart_unsolicited_input;

static void UART_ByteReceivedHandler();
static void UART_TransmissionCompleteHandler();
static void UART_QueueReset();
static bool UART_ByteReceived();
static bool UART_ByteSent();
static bool UART_MessageSent();
static void UART_TxQueueFill(uint8_t *);
static void UART_RxDeQueue(char *);
static uint8_t UART_TxQueueGetByteFromISR(uint8_t *);
static uint8_t UART_TxQueueGetByte();
static void UART_TxQueuePush(uint8_t);
static void UART_RxQueuePush(uint8_t);
static uint8_t UART_RxQueuePop();
static void UART_PutByte(uint8_t);
static void UART_GetByte(uint8_t *);
static void UART_Timer35Start();
static void UART_Timer35Stop();
static ErrorType_t UART_busy(uint16_t);
static void UART_release();


xSemaphoreHandle txBinarySemaphore = NULL;
xSemaphoreHandle rxBinarySemaphore = NULL;
xSemaphoreHandle UARTBusySemaphore = NULL;

xQueueHandle uartRxQueue;
xQueueHandle uartTxQueue;

void
UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    // Init timer
    __HAL_RCC_TIM3_CLK_ENABLE();

    /**
     * Prescaler = X - 1
     * Period = Y - 1
     * update = 32MHz / (X * Y) = 32MHz / (320 * 10) = 100kHz
     * period is 100us
     **/
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = (uint32_t)(SystemCoreClock / 100000) - 1;   /* 32MHz / 10e5 - 1 = 320 - 1 */
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 10 - 1;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim3);

    HAL_NVIC_SetPriority(TIM3_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    // Init binary semaphores
    UARTBusySemaphore = xSemaphoreCreateBinary();
    rxBinarySemaphore = xSemaphoreCreateBinary();
    txBinarySemaphore = xSemaphoreCreateBinary();

    if ((UARTBusySemaphore == NULL) || (rxBinarySemaphore == NULL) || (txBinarySemaphore == NULL))
    {
        Error_Handler(FreeRTOS_Error);
    }
    uartRxQueue = xQueueCreate(SIM808_BUFFER_SIZE, sizeof(uint8_t));
    uartTxQueue = xQueueCreate(MAX_AT_CMD_LEN, sizeof(uint8_t));

    if ((uartRxQueue == NULL) || (uartTxQueue == NULL))
    {
        Error_Handler(FreeRTOS_Error);
    }
}

void
UART_DeInit(void) {
    HAL_UART_DeInit(&huart1);
}

void UART_Enable (uint8_t xRxEnable, uint8_t xTxEnable)
{
    uint32_t tmpreg = 0;

    tmpreg = huart1.Instance->CR1;
    if ( xRxEnable )
    {
        tmpreg |= (USART_CR1_RE);
        huart1.Instance->CR1 = (uint32_t)tmpreg;
        __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
    }
    else
    {
        tmpreg &= ~(USART_CR1_RE);
        huart1.Instance->CR1 = (uint32_t)tmpreg;
        __HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE);
    }

    tmpreg = huart1.Instance->CR1;
    if ( xTxEnable )
    {
        tmpreg |= (USART_CR1_TE);
        huart1.Instance->CR1 = (uint32_t)tmpreg;
        __HAL_UART_ENABLE_IT(&huart1, UART_IT_TC);
    }
    else
    {
        tmpreg &= ~(USART_CR1_TE);
        huart1.Instance->CR1 = (uint32_t)tmpreg;
        __HAL_UART_DISABLE_IT(&huart1, UART_IT_TC);
    }
}

/**
 * @brief: Send data over uart interface. Wait for response and parse it
 * if requested by caller function.
 * @param data: data to be transmitted over UART
 * @param tx_timeout: time to transmit whole data buffer
 * @param rx_at_struct: structure holding the instructions to parse AT response
 *                      (at request id, rx_timeout and response parse callback)
 * @return: Status of UART send and response parsing
 */
ErrorType_t
UART_Send(uint8_t *data, uint16_t tx_timeout, struct at_response *at_r)
{
    ErrorType_t status = Ok;

    /* Take semaphore first */
    if (UART_busy(tx_timeout) != Ok)
    {
        status = UART_Error;
    }
    if (status == Ok)
    {
        /* Reset the queues */
        UART_QueueReset();

        /* Fill the TX queue */
        UART_TxQueueFill(data);

        /* Enable UART transmitter */
        UART_PutByte(UART_TxQueueGetByte());

        /* Delay till the whole frame is transmitted */
        if (xSemaphoreTake(txBinarySemaphore, (TickType_t)tx_timeout) == pdFALSE)
        {
            status = UART_Error;
        }
    }
    if (status == Ok)
    {
        if (at_r->wait)
        {
            /* Get AT response */
            UART_Get_rxData(at_r->response, at_r->rx_timeout);
            debug_printf("Response received: %s\r\n", at_r->response);
        }
    }
    UART_release();
    return status;
}

void
UART_Get_rxData(char *data, uint32_t timeout)
{
    /* Wait to get the answer from sim808 */
    if (xSemaphoreTake(rxBinarySemaphore, (TickType_t) timeout) == pdFALSE)
    {
        return;
    }
    /* Get the received message */
    UART_RxDeQueue(data);
}

void
UART_FlushQueues(void)
{
    if ((xSemaphoreTake(rxBinarySemaphore, 0) == pdTRUE))
    {
        UART_QueueReset();
    }
}

static void
UART_ByteReceivedHandler(void)
{
    uint8_t rxData;
    /* Clear Receive data register not empty flag */
    __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE);
    UART_Timer35Stop();
    UART_GetByte(&rxData);
    if ((rxData != '\r') && (rxData != '\n') && (rxData != '\0'))
    {
        UART_RxQueuePush(rxData);
    }
    UART_Timer35Start();
}

static void
UART_TransmissionCompleteHandler(void)
{
    uint8_t byteToSend;
    /* Clear Transmission Complete flag */
    __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_TC);
    if (UART_TxQueueGetByteFromISR(&byteToSend))
    {
        UART_PutByte(byteToSend);
    }
    else
    {
        /* Tx queue empty */
        BaseType_t xTaskWokenByReceive = pdFALSE;
        xSemaphoreGiveFromISR(txBinarySemaphore, &xTaskWokenByReceive);
        if (xTaskWokenByReceive != pdFALSE)
        {
            taskYIELD ();
        }
    }
}

static void
UART_QueueReset(void)
{
    /* Reset the queue */
    xQueueReset(uartTxQueue);
    xQueueReset(uartRxQueue);
}

/*
 * @brief: returns true if byte is received, otherwise false
 */
static bool
UART_ByteReceived(void)
{
    uint8_t rxne_flag;
    uint8_t rxne_it;
    rxne_flag = __HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE);
    rxne_it = __HAL_UART_GET_IT_SOURCE(&huart1, UART_IT_RXNE);
    return ((rxne_flag != RESET) && (rxne_it != RESET));
}

/*
 * @brief: returns true if byte is sent, otherwise false
 */
static bool
UART_ByteSent(void)
{
    uint8_t tc_flag;
    uint8_t tc_it;
    tc_flag = __HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC);
    tc_it = __HAL_UART_GET_IT_SOURCE(&huart1, UART_IT_TC);
    return ((tc_flag != RESET) && (tc_it != RESET));
}

/*
 * @brief: returns true if whole frame is sent, otherwise false
 */
static bool
UART_MessageSent(void)
{
    uint8_t txe_flag;
    uint8_t txe_it;
    txe_flag = __HAL_UART_GET_FLAG(&huart1, UART_FLAG_TXE);
    txe_it = __HAL_UART_GET_IT_SOURCE(&huart1, UART_IT_TXE);
    return ((txe_flag != RESET) && (txe_it != RESET));
}

static void
UART_TxQueueFill(uint8_t *data)
{
    uint16_t i;
    uint16_t txSize = strlen((char *)data);
    for (i = 0; i < txSize; i++)
    {
        UART_TxQueuePush(data[i]);
    }
}

static void
UART_RxDeQueue(char *data)
{
    while (uxQueueMessagesWaiting(uartRxQueue) > 0)
    {
        PUT_BYTE(data, UART_RxQueuePop());
    }
}

static uint8_t
UART_TxQueueGetByteFromISR(uint8_t *data)
{
    BaseType_t xTaskWokenByReceive = pdFALSE;

    if (xQueueReceiveFromISR(uartTxQueue, data, &xTaskWokenByReceive) == pdFALSE)
    {
        return 0;
    }
    if (xTaskWokenByReceive != pdFALSE)
    {
        taskYIELD();
    }
    return 1;
}

static uint8_t
UART_TxQueueGetByte(void)
{
    uint8_t data;
    xQueueReceive(uartTxQueue, &data, 10);
    return data;
}

static void
UART_TxQueuePush(uint8_t data)
{
    /* Do not block if queue is already full */
    xQueueSend(uartTxQueue, &data, 0);
}

static void
UART_RxQueuePush(uint8_t data)
{
    BaseType_t xTaskWokenByReceive = pdFALSE;
    xQueueSendFromISR(uartRxQueue, &data, &xTaskWokenByReceive);
    if (xTaskWokenByReceive != pdFALSE)
    {
        taskYIELD();
    }
}

static uint8_t
UART_RxQueuePop(void)
{
    uint8_t data;
    xQueueReceive(uartRxQueue, &data, 10);
    return data;
}

static void
UART_PutByte(uint8_t ucByte)
{
    huart1.Instance->DR = ucByte;
}

static void
UART_GetByte(uint8_t *pucByte)
{
    *pucByte = huart1.Instance->DR;
}

/*
 * @brief: Start UART timer to be able to identify end of received message.
 */
static void
UART_Timer35Start(void)
{
    htim3.Instance->CNT = 0;
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(&htim3);
}

/*
 * @brief: Stop UART timer.
 */
static void
UART_Timer35Stop(void)
{
    HAL_TIM_Base_Stop_IT(&htim3);
}

static ErrorType_t
UART_busy(uint16_t timeout)
{
    ErrorType_t ret = Ok;
    uart_unsolicited_input = false;
    if (xSemaphoreTake(UARTBusySemaphore, (TickType_t) timeout) == pdFALSE)
    {
        return UART_Error;
    }
    return ret;
}

static void
UART_release(void)
{
    uart_unsolicited_input = true;
    xSemaphoreGive(UARTBusySemaphore);
}

void USART1_IRQHandler(void)
{
    /* Handle received byte */
    if (UART_ByteReceived())
    {
        UART_ByteReceivedHandler();
    }
    /* Handle byte transmission */
    if (UART_ByteSent())
    {
        UART_TransmissionCompleteHandler();
    }
    /* Handle end of transmission */
    if (UART_MessageSent())
    {
        /* TXE flag is cleared only by a write to the USART_DR register.
         * (Transmit data register empty flag)
         * */
    }
}

/*
 * Triggers when Timer expired from last received byte.
 * End of received frame.
 */
void
TIM3_IRQHandler(void)
{
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
    __HAL_TIM_CLEAR_IT(&htim3, TIM_IT_UPDATE);
    HAL_NVIC_ClearPendingIRQ(TIM3_IRQn);

    HAL_TIM_Base_Stop_IT(&htim3);

    BaseType_t xTaskWokenByReceive = pdFALSE;

    /* Give semaphore */
    xSemaphoreGiveFromISR(rxBinarySemaphore, &xTaskWokenByReceive);
    if (uart_unsolicited_input)
    {
        _setSignal(SIM808UnsolicitedTask, SIGNAL_UNSOLICITED);
    }
    if (xTaskWokenByReceive != pdFALSE)
    {
        taskYIELD();
    }
}
