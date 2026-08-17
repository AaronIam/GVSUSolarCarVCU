#include "stm32f4xx_hal.h"
#include <string.h>

CAN_HandleTypeDef hcan1;
UART_HandleTypeDef huart3;

static void CAN1_Init(void);
static void UART3_Init(void);
static void CAN1_FilterConfig(void);

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
 * ========================= */
static void CAN1_FilterConfig(void)
{
    CAN_FilterTypeDef filter = {0};

    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;

    /* Accept everything */
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
 * SEND CAN MESSAGE
 * ========================= */
static HAL_StatusTypeDef CAN1_Send(void)
{
    CAN_TxHeaderTypeDef txHeader;
    uint8_t txData[8];
    uint32_t txMailbox;

    txHeader.StdId = 0x13;
    txHeader.ExtId = 0;

    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;

    txHeader.DLC = 8;
    txHeader.TransmitGlobalTime = DISABLE;

    txData[0] = 0x11;
    txData[1] = 0x22;
    txData[2] = 0x33;
    txData[3] = 0x44;
    txData[4] = 0x55;
    txData[5] = 0x66;
    txData[6] = 0x77;
    txData[7] = 0x88;

    return HAL_CAN_AddTxMessage(
        &hcan1,
        &txHeader,
        txData,
        &txMailbox
    );
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

    UART3_SendString("CAN TX BOARD STARTED\r\n");

    while (1)
    {
        if (CAN1_Send() == HAL_OK)
        {
            UART3_SendString(
                "TX: ID=0x123 DATA=11 22 33 44 55 66 77 88\r\n"
            );
        }
        else
        {
            UART3_SendString("TX ERROR\r\n");
        }

        HAL_Delay(1000);
    }
}