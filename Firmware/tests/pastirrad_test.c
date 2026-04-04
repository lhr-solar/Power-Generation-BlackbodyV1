#include "stm32xx_hal.h"
#include <stdio.h>
#include "UART.h"
#include "printf.h"

#define TSL25911FN_7BIT_ADDRESS (0x29)
#define TSL25911FN_8BIT_ADDRESS (0x29 << 1)

#define CMD (0xA0)
#define REG_ENABLE (0x00) //power on/off
#define REG_CONTROL (0x01)

#define REG_PackID (0x11)
#define REG_DevID (0x12)
#define REG_Status (0x13)

#define REG_C0DATAL (0x14)
#define REG_C0DATAH (0x15)
#define REG_C1DATAL (0x16)
#define REG_C1DATAH (0x17)

I2C_HandleTypeDef hi2c1;

/**
  * @brief I2C MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c1)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(hi2c1->Instance==I2C1)
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
    /* USER CODE BEGIN I2C1_MspInit 1 */

    /* USER CODE END I2C1_MspInit 1 */

  }

}

/**
  * @brief I2C MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c1)
{
  if(hi2c1->Instance==I2C1)
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

    /* USER CODE BEGIN I2C1_MspDeInit 1 */

    /* USER CODE END I2C1_MspDeInit 1 */
  }

}


/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);


int main(void)
{

  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();

/*Power on Chip*/
  uint8_t power_enable = 0x03;
  uint8_t powerread[1];
  uint8_t ENABLE = REG_ENABLE | 0xA0;

    HAL_I2C_Mem_Write (&hi2c1, TSL25911FN_8BIT_ADDRESS, ENABLE, 1, &power_enable, 1, 100);
    HAL_I2C_Mem_Read (&hi2c1, TSL25911FN_8BIT_ADDRESS, ENABLE, 1, powerread, 1, 100);

  HAL_Delay(200);

/*Control Register turns on ALS gain and intgeration time*/
  uint8_t control = 0x00;
  uint8_t controlread[1];
  uint8_t CONTROL = REG_CONTROL | 0xA0;

    HAL_I2C_Mem_Write (&hi2c1, TSL25911FN_8BIT_ADDRESS, CONTROL, 1, &control, 1, 100);
    HAL_I2C_Mem_Read (&hi2c1, TSL25911FN_8BIT_ADDRESS, CONTROL, 1, controlread, 1, 100);

  HAL_Delay(200);


  /* Infinite loop */
    while (1)
    {
        uint8_t databuffer[4];
        uint8_t DATA_START = REG_C0DATAL | CMD;


        HAL_I2C_Master_Transmit(&hi2c1, TSL25911FN_8BIT_ADDRESS, &DATA_START, 1, 100);

        HAL_Delay(50);

        HAL_I2C_Master_Receive(&hi2c1, TSL25911FN_8BIT_ADDRESS, databuffer, 4, 100);
            
          uint16_t ch0 = ((uint16_t)databuffer[1] << 8) | databuffer[0];
          uint16_t ch1 = ((uint16_t)databuffer[3] << 8) | databuffer[2];

        int32_t irrad_white;
        int32_t irrad_850;

        irrad_white = (int32_t)(((int64_t)ch0 * 9876 << 16) / 6024);
        irrad_850   = (int32_t)(((int64_t)ch1 * 9876 << 16) / 3474);

        int32_t white_int  = irrad_white >> 16;
        int32_t white_frac = ((irrad_white & 0xFFFF) * 1000) >> 16;

        int32_t ir_int  = irrad_850 >> 16;
        int32_t ir_frac = ((irrad_850 & 0xFFFF) * 1000) >> 16;

        printf("CH0: %u  CH1: %u\r\n", ch0, ch1);
        printf("White Irrad: %ld.%03ld uW/cm^2\r\n", white_int, white_frac);
        printf("IR Irrad: %ld.%03ld uW/cm^2\r\n\r\n", ir_int, ir_frac);

      HAL_Delay(500);
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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

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
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();

}

void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1){}
  
}