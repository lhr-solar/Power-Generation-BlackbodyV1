#include "irrad.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pinDefs.h"
#include "UART.h"
#include "projdefs.h"
#include "stm32xx_hal.h"
#include "printf.h"
// #include <stdio.h>

#define PRINTF_NVIC_PRIO      configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 3

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void HeartbeatTask(void *argument);
static void IrradTask(void *argument);
static void Heartbeat_Clock_Init(void);

StaticTask_t irradTaskTCB;
StackType_t irradTaskStack[1024];

StaticTask_t heartbeatTaskTCB;
StackType_t heartbeatTaskStack[256];

/* Test globals */
TSL25911FN_HandleTypeDef irrad_handle;
volatile tsl25911fn_status_t irrad_status = TSL25911FN_OK;
volatile uint8_t reg_read = 0;
I2C_HandleTypeDef hi2c1;



/**
  * @brief I2C MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(hi2c->Instance==I2C1)
  {
    /* USER CODE BEGIN I2C1_MspInit 0 */

    /* USER CODE END I2C1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
    /* I2C1 interrupt Init */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
    /* USER CODE BEGIN I2C1_MspInit 1 */

    /* USER CODE END I2C1_MspInit 1 */

  }

}

void Heartbeat_Clock_Init() {
    switch ((uint32_t)PSOM_HEARTBEAT_LED_PORT) {
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

/**
  * @brief I2C MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
  if(hi2c->Instance==I2C1)
  {
    /* USER CODE BEGIN I2C1_MspDeInit 0 */

    /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);

    /* I2C1 interrupt DeInit */
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
    /* USER CODE BEGIN I2C1_MspDeInit 1 */

    /* USER CODE END I2C1_MspDeInit 1 */
  }

}


/**
  * @brief This function handles I2C1 event interrupt.
  */
void I2C1_EV_IRQHandler(void)
{
  HAL_I2C_EV_IRQHandler(&hi2c1);
}

/**
  * @brief This function handles I2C1 error interrupt.
  */
void I2C1_ER_IRQHandler(void)
{
  HAL_I2C_ER_IRQHandler(&hi2c1);
}

  void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == &hi2c1)
    {
        tsl_i2c_tx_done = 1;
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == &hi2c1)
    {
        tsl_i2c_error = 1;
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

void IrradTask(void *argument){
    husart1->Instance = USART1;
    husart1->Init.BaudRate = 115200;
    husart1->Init.WordLength = UART_WORDLENGTH_8B;
    husart1->Init.StopBits = UART_STOPBITS_1;
    husart1->Init.Parity = UART_PARITY_NONE;
    husart1->Init.Mode = UART_MODE_TX_RX;
    husart1->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    husart1->Init.OverSampling = UART_OVERSAMPLING_16;

    printf_init(husart1);

  TSL25911FN_data_t sensor_data = {0};

  irrad_handle.device_addr = TSL25911FN_7BIT_ADDRESS;
  irrad_handle.hi2c = &hi2c1;
  irrad_handle.gain = TSL25911FN_GAIN_LOW;
  irrad_handle.time = TSL25911FN_TIME_100MS;
  irrad_handle.control = irrad_handle.gain | irrad_handle.time;

  tsl_i2c_tx_done = 0;
  tsl_i2c_error = 0;

  irrad_status = tsl25911fn_power_on(&irrad_handle, 
                                    pdMS_TO_TICKS(10));

  if (irrad_status != TSL25911FN_OK)
  {
    printf("Power on failed\r\n");
    while (1){}
  }

  irrad_status = tsl25911fn_set_control(&irrad_handle,
                                        irrad_handle.control,
                                        pdMS_TO_TICKS(10));
  
  if (irrad_status != TSL25911FN_OK)
  {
    printf("Control set failed\r\n");
    while (1){}
  }

  vTaskDelay(pdMS_TO_TICKS(150));

  printf("TSL2591 Ready\r\n");

  while(1){
  irrad_status = tsl25911fn_read_data(&irrad_handle,
                                      &sensor_data,
                                      pdMS_TO_TICKS(10)); 

  if (irrad_status != TSL25911FN_OK)
  {
    printf("Read failed\r\n");
    while (1){}
  }
    {
      uint32_t white_int  = sensor_data.irrad_whitelight_q16 >> 16;
      uint32_t white_frac = ((sensor_data.irrad_whitelight_q16 & 0xFFFF) * 1000) >> 16;

      uint32_t ir_int  = sensor_data.irrad_infrared_q16 >> 16;
      uint32_t ir_frac = ((sensor_data.irrad_infrared_q16 & 0xFFFF) * 1000) >> 16;

      printf("CH0:%4u CH1:%4u | White:%lu.%03lu IR:%lu.%03lu \r\n",
             sensor_data.ch0,
             sensor_data.ch1,
             white_int, white_frac,
             ir_int, ir_frac);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}


int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();

  xTaskCreateStatic(HeartbeatTask,
                    "Heartbeat",
                    256,
                    NULL,
                    tskIDLE_PRIORITY + 1,
                    heartbeatTaskStack,
                    &heartbeatTaskTCB);

  xTaskCreateStatic(IrradTask,
                    "Irrad",
                    1024,
                    NULL,
                    tskIDLE_PRIORITY + 2,
                    irradTaskStack,
                    &irradTaskTCB);

  vTaskStartScheduler();

  while (1)
  {
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
// void SystemClock_Config(void)
// {
//   RCC_OscInitTypeDef RCC_OscInitStruct = {0};
//   RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

//   /** Configure the main internal regulator output voltage
//   */
//   if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
//   {
//     Error_Handler();
//   }

//   /** Initializes the RCC Oscillators according to the specified parameters
//   * in the RCC_OscInitTypeDef structure.
//   */
//   RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
//   RCC_OscInitStruct.MSIState = RCC_MSI_ON;
//   RCC_OscInitStruct.MSICalibrationValue = 0;
//   RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
//   RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
//   if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//   {
//     Error_Handler();
//   }

//   /** Initializes the CPU, AHB and APB buses clocks
//   */
//   RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
//                               |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
//   RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
//   RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
//   RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
//   RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

//   if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
//   {
//     Error_Handler();
//   }
// }

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00100D14;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
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


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */