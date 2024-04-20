#include "main.h"
#include "cmsis_os.h"
#include "string.h"

UART_HandleTypeDef huart1;
osThreadId LEDTaskHandle;
osThreadId SWTaskHandle;
osThreadId UARTTaskHandle;
osMessageQId UARTQueueHandle;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
void LED_Task(void const * argument);
void SW_Task(void const * argument);
void UART_Task(void const * argument);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();

  osThreadDef(LEDTask, LED_Task, osPriorityNormal, 0, 128);
  LEDTaskHandle = osThreadCreate(osThread(LEDTask), NULL);

  osThreadDef(SWTask, SW_Task, osPriorityNormal, 0, 128);
  SWTaskHandle = osThreadCreate(osThread(SWTask), NULL);

  osThreadDef(UARTTask, UART_Task, osPriorityNormal, 0, 128);
  UARTTaskHandle = osThreadCreate(osThread(UARTTask), NULL);

  osMessageQDef(UARTQueue, 10, uint8_t);
  UARTQueueHandle = osMessageCreate(osMessageQ(UARTQueue), NULL);

  osKernelStart();

  while (1)
  {
  }
}

void LED_Task(void const * argument)
{
  uint8_t led_state = GPIO_PIN_RESET;
  while (1)
  {
    uint8_t command;
    if (osMessagePeek(UARTQueueHandle, command, 0) == osEventMessage)
    {
      switch (command)
      {
        case 'O':
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
          break;
        case 'F':
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
          break;
        case 'T':
          led_state = (led_state == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET;
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, led_state);
          break;
        default:
          break;
      }
      osMessageGet(UARTQueueHandle, command, osWaitForever);
    }
  }
}

void SW_Task(void const * argument)
{
  while (1)
  {
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
    {
      osMessagePut(UARTQueueHandle, 'T', osWaitForever);
      osDelay(100); // Debounce delay
    }
  }
}

void UART_Task(void const * argument)
{
  uint8_t rxData;
  while (1)
  {
    if (HAL_UART_Receive(&huart1, &rxData, 1, 100) == HAL_OK)
    {
      osMessagePut(UARTQueueHandle, rxData, osWaitForever);
    }
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART1_UART_Init(void)
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
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
