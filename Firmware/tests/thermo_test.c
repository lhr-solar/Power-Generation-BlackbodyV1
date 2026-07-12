#include "thermo.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pinDefs.h"
#include "UART.h"
#include "projdefs.h"
#include "stm32xx_hal.h"

#define PRINTF_NVIC_PRIO configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 3

#include "printf.h"

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void HeartbeatTask(void *argument);
static void ThermoTask(void *argument);
static void Heartbeat_Clock_Init(void);

StaticTask_t thermoTaskTCB;
StackType_t thermoTaskStack[1024];

StaticTask_t heartbeatTaskTCB;
StackType_t heartbeatTaskStack[256];

MCP9600_HandleTypeDef thermo_handle;
volatile mcp9600_status_t thermo_status = MCP9600_OK;
I2C_HandleTypeDef hi2c2;

void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  if(hi2c->Instance==I2C2)
  {
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C2;
    PeriphClkInit.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    __HAL_RCC_I2C2_CLK_ENABLE();

    HAL_NVIC_SetPriority(I2C2_EV_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1, 0);
    HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);

    HAL_NVIC_SetPriority(I2C2_ER_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1, 0);
    HAL_NVIC_EnableIRQ(I2C2_ER_IRQn);
  }
}

void Heartbeat_Clock_Init(void)
{
  switch ((uint32_t)PSOM_HEARTBEAT_LED_PORT)
  {
    case (uint32_t)GPIOA:
      __HAL_RCC_GPIOA_CLK_ENABLE();
      break;
    case (uint32_t)GPIOB:
      __HAL_RCC_GPIOB_CLK_ENABLE();
      break;
    case (uint32_t)GPIOC:
      __HAL_RCC_GPIOC_CLK_ENABLE();
      break;
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
  if(hi2c->Instance==I2C2)
  {
    /* USER CODE BEGIN I2C2_MspDeInit 0 */

    /* USER CODE END I2C2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C2_CLK_DISABLE();

    /**I2C2 GPIO Configuration
    PB10     ------> I2C2_SCL
    PB14     ------> I2C2_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_14);

    /* I2C2 interrupt DeInit */
    HAL_NVIC_DisableIRQ(I2C2_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C2_ER_IRQn);
    /* USER CODE BEGIN I2C2_MspDeInit 1 */

    /* USER CODE END I2C2_MspDeInit 1 */
  }

}



static void HeartbeatTask(void *argument)
{
  (void)argument;

  while (1)
  {
    HAL_GPIO_TogglePin(PSOM_HEARTBEAT_LED_PORT, PSOM_HEARTBEAT_LED_PIN);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

static void ThermoTask(void *argument){
  husart1->Instance = USART1;
  husart1->Init.BaudRate = 115200;
  husart1->Init.WordLength = UART_WORDLENGTH_8B;
  husart1->Init.StopBits = UART_STOPBITS_1;
  husart1->Init.Parity = UART_PARITY_NONE;
  husart1->Init.Mode = UART_MODE_TX_RX;
  husart1->Init.HwFlowCtl = UART_HWCONTROL_NONE;
  husart1->Init.OverSampling = UART_OVERSAMPLING_16;

  printf_init(husart1);

  thermo_handle.device_addr = MCP9600_7BIT_ADDR_6;
  thermo_handle.hi2c = &hi2c2;
  thermo_handle.thermocouple_type = MCP9600_THERMOCOUPLE_TYPE_K;
  thermo_handle.filter = MCP9600_FILTER_0;
  thermo_handle.adc_resolution = MCP9600_ADC_RESOLUTION_18BIT;

  int32_t temp_int = 0;
  int32_t temp_frac = 0;

  thermo_status = mcp9600_init(&thermo_handle,
                               &hi2c2,
                               MCP9600_7BIT_ADDR_6);

  // if (thermo_status != MCP9600_OK)
  // {
  //   printf("MCP9600 init failed\r\n");
  //   while (1){}
  // }
  
  // printf("MCP9600 Ready\r\n");

  while (1)
  {
    thermo_status = mcp9600_read_hot_junction(&thermo_handle,
                                              &temp_int,
                                              &temp_frac,
                                              pdMS_TO_TICKS(100));

    // if (thermo_status != MCP9600_OK)
    // {
    //   printf("Thermo Read Failed\r\n");
    // }

    // printf("Lemperature: %ld.%04ld C\r\n", temp_int, temp_frac);

    vTaskDelay(pdMS_TO_TICKS(100));
  }

}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C2_Init();

  xTaskCreateStatic(HeartbeatTask,
                    "Heartbeat",
                    256,
                    NULL,
                    tskIDLE_PRIORITY + 1,
                    heartbeatTaskStack,
                    &heartbeatTaskTCB);

  xTaskCreateStatic(ThermoTask,
                    "Thermo",
                    1024,
                    NULL,
                    tskIDLE_PRIORITY + 2,
                    thermoTaskStack,
                    &thermoTaskTCB);

  vTaskStartScheduler();

  while (1)
  {
  }
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE; //PLL ON in weak

  // RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  // RCC_OscInitStruct.PLL.PLLM = 1;
  // RCC_OscInitStruct.PLL.PLLN = 40;
  // RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  // RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  // RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI; //RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0/* 4 in weak*/) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C2_Init(void)
{
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00100D14;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
}


static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef led_config = {0};

  Heartbeat_Clock_Init();

  led_config.Mode = GPIO_MODE_OUTPUT_PP;
  led_config.Pull = GPIO_NOPULL;
  led_config.Pin = PSOM_HEARTBEAT_LED_PIN;
  led_config.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(PSOM_HEARTBEAT_LED_PORT, &led_config);

  HAL_GPIO_WritePin(PSOM_HEARTBEAT_LED_PORT, PSOM_HEARTBEAT_LED_PIN, GPIO_PIN_RESET);

  __HAL_RCC_GPIOB_CLK_ENABLE();
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif