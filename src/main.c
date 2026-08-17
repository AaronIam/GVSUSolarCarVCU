#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

CAN_HandleTypeDef hcan1;
UART_HandleTypeDef huart3;

static void CAN1_Init(void);
static void CAN1_FilterConfig(void);
static void UART3_Init(void);

static HAL_StatusTypeDef UART3_SendString(const char *str);

void SysTick_Handler(void)
{
    HAL_IncTick();
}


/* =========================
 * CAN INITIALIZATION
 * ========================= */
static void CAN1_Init(void)
{
    hcan1.Instance = CAN1;

    hcan1.Init.Prescaler = 4;
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1 = CAN_BS1_16TQ;
    hcan1.Init.TimeSeg2 = CAN_BS2_7TQ;

    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = DISABLE;
    hcan1.Init.AutoWakeUp = DISABLE;
    hcan1.Init.AutoRetransmission = ENABLE;
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;

    if (HAL_CAN_Init(&hcan1) != HAL_OK)
    {
        while (1)
        {
        }
    }
}


/* =========================
 * CAN GPIO
 *
 * PD0 = CAN1_RX
 * PD1 = CAN1_TX
 * ========================= */
void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hcan->Instance == CAN1)
    {
        __HAL_RCC_CAN1_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;

        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    }
}


/* =========================
 * CAN FILTER
 *
 * Accept every message
 * ========================= */
static void CAN1_FilterConfig(void)
{
    CAN_FilterTypeDef filter = {0};

    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;

    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;

    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;

    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterActivation = ENABLE;

    filter.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(&hcan1, &filter) != HAL_OK)
    {
        while (1)
        {
        }
    }
}


/* =========================
 * UART
 * ========================= */
static HAL_StatusTypeDef UART3_SendString(const char *str)
{
    return HAL_UART_Transmit(
        &huart3,
        (uint8_t *)str,
        strlen(str),
        100
    );
}


static void UART3_Init(void)
{
    huart3.Instance = USART3;

    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart3) != HAL_OK)
    {
        while (1)
        {
        }
    }
}


void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART3)
    {
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;

        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    }
}


/* =========================
 * MAIN
 * ========================= */
int main(void)
{
    HAL_Init();

    UART3_Init();

    CAN1_Init();
    CAN1_FilterConfig();

    if (HAL_CAN_Start(&hcan1) != HAL_OK)
    {
        UART3_SendString("CAN START ERROR\r\n");

        while (1)
        {
        }
    }

    UART3_SendString("CAN RX BOARD STARTED\r\n");

    while (1)
    {
        /*
         * Check whether a CAN message
         * is waiting in FIFO0.
         */
        if (HAL_CAN_GetRxFifoFillLevel(
                &hcan1,
                CAN_RX_FIFO0) > 0)
        {
            CAN_RxHeaderTypeDef rxHeader;
            uint8_t rxData[8];

            if (HAL_CAN_GetRxMessage(
                    &hcan1,
                    CAN_RX_FIFO0,
                    &rxHeader,
                    rxData) == HAL_OK)
            {
                char msg[150];

                /*
                 * Print ID and DLC
                 */
                snprintf(
                    msg,
                    sizeof(msg),
                    "RX ID=0x%03lX DLC=%lu\r\n",
                    rxHeader.StdId,
                    rxHeader.DLC
                );

                UART3_SendString(msg);

                /*
                 * Print data
                 */
                snprintf(
                    msg,
                    sizeof(msg),
                    "DATA: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                    rxData[0],
                    rxData[1],
                    rxData[2],
                    rxData[3],
                    rxData[4],
                    rxData[5],
                    rxData[6],
                    rxData[7]
                );

                UART3_SendString(msg);
            }
        }
    }
}