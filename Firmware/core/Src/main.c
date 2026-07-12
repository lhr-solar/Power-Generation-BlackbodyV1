#include "irrad.h"
#include "thermo.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pinDefs.h"
#include "projdefs.h"
#include "printf.h"
#include "UART.h"
#include "CAN.h"
#include "stm32xx_hal.h"

#define QUEUE_LENGTH 50

StaticTask_t heartbeatTaskTCB;
StackType_t heartbeatTaskStack[256];

StaticTask_t irradTaskTCB;
StackType_t irradTaskStack[1024];

StaticTask_t thermoTaskTCB;
StackType_t thermoTaskStack[1024];

StaticTask_t CANTaskTCB;
StackType_t CANTaskStack[1024];

static QueueHandle_t IrradQueue;
static StaticQueue_t IrradQueueBuffer;
static uint32_t IrradQueueStorage[QUEUE_LENGTH];

static QueueHandle_t ThermoQueue;
static StaticQueue_t ThermoQueueBuffer;
static uint32_t ThermoQueueStorage[QUEUE_LENGTH];

TSL25911FN_HandleTypeDef irrad_handle;
MCP9600_HandleTypeDef thermo_handle;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;


void HeartbeatTask(void *argument)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef led_config = {0};
    led_config.Mode = GPIO_MODE_OUTPUT_PP;
    led_config.Pull = GPIO_NOPULL;
    led_config.Pin = PSOM_HEARTBEAT_LED_PIN;
    led_config.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PSOM_HEARTBEAT_LED_PORT, &led_config);
    HAL_GPIO_WritePin(PSOM_HEARTBEAT_LED_PORT, PSOM_HEARTBEAT_LED_PIN, GPIO_PIN_SET);

    (void)argument;

    while (1)
    {
      HAL_GPIO_TogglePin(PSOM_HEARTBEAT_LED_PORT, PSOM_HEARTBEAT_LED_PIN);
      vTaskDelay(pdMS_TO_TICKS(500));
    }

}

void IrradTask(void *argument){
    tsl25911fn_status_t irrad_status;
    TSL25911FN_data_t irrad_data = {0};

    uint32_t white_int;
    uint32_t white_frac;
    uint32_t infrared_int;
    uint32_t infrared_frac;


    irrad_status = tsl25911fn_init(&irrad_handle, &hi2c1);

    if(irrad_status != TSL25911FN_WRITE_FAIL){
        printf("status %d \n\r" , irrad_status);
    }


    irrad_status = tsl25911fn_power_on(&irrad_handle, 
                                pdMS_TO_TICKS(10));

    irrad_status = tsl25911fn_set_control(&irrad_handle,
                                            irrad_handle.control,
                                            pdMS_TO_TICKS(10));

    UNUSED(irrad_status);

    while (1){

        irrad_status = tsl25911fn_read_data(&irrad_handle,
                                            &irrad_data,
                                            pdMS_TO_TICKS(10)); 

        if (irrad_status == TSL25911FN_OK)
        {
            white_int =(uint32_t)(irrad_data.irrad_whitelight_q16 >> 16);

            white_frac = (uint32_t)(((irrad_data.irrad_whitelight_q16 & 0xFFFFULL) * 10000ULL) >> 16);

            infrared_int = (uint32_t)(irrad_data.irrad_infrared_q16 >> 16);

            infrared_frac = (uint32_t)(((irrad_data.irrad_infrared_q16 & 0xFFFFULL) * 10000ULL) >> 16);

            if (irrad_data.ch0 >= 36000 || irrad_data.ch1 >= 36000)
                {
                    printf("Sensor saturated\r\n");
                }

            // printf("CH0: %u CH1: %u\r\n",
            //        irrad_data.ch0,
            //        irrad_data.ch1);

            printf("White Light Irradiance: %lu.%04lu\r\n",
                   white_int,
                   white_frac);

            printf("Infrared Light Irradiance: %lu.%04lu\r\n",
                   infrared_int,
                   infrared_frac);
        }
        else
        {
            printf("Irradiance read failed\r\n");
        }

        uint32_t irrad_packed = ((uint16_t)irrad_data.ch1 << 16) | (uint16_t)irrad_data.ch0;

        if (uxQueueSpacesAvailable(IrradQueue) > 0){
            xQueueSend(
            IrradQueue,
            &irrad_packed,
            200);
        }

        //integration time for the sensor is 100ms on max gain
        vTaskDelay(pdMS_TO_TICKS(200)); 
        
    }
}


void ThermoTask(void *argument){
    mcp9600_status_t thermo_status;
    thermo_status = mcp9600_init(&thermo_handle,
                                &hi2c2,
                                MCP9600_7BIT_ADDR_6);
    UNUSED(thermo_status);
                    
    int32_t temp_int = 0;
    int32_t temp_frac = 0;
    uint16_t raw_temperature = 0;

        while (1){
            thermo_status = mcp9600_read_hot_junction(&thermo_handle,
                                            &temp_int,
                                            &temp_frac,
                                            &raw_temperature,
                                            pdMS_TO_TICKS(100));
            
            printf("Temperature: %ld.%04ld C\r\n", temp_int, temp_frac);
            //maybe 80ms for reading idfk thou just a guess

        uint16_t thermo_packed = raw_temperature;

        if (uxQueueSpacesAvailable(ThermoQueue) > 0){
            xQueueSend(
            ThermoQueue,
            &thermo_packed,
            200);
        }

            vTaskDelay(pdMS_TO_TICKS(200));
        }
}

void CANTask(void *argument){

        uint32_t irrad_recieved;
        uint16_t thermo_recieved;
        CAN_TxHeaderTypeDef tx_header = {0};   
        tx_header.StdId = 0x1;
        tx_header.RTR = CAN_RTR_DATA;
        tx_header.IDE = CAN_ID_STD;
        tx_header.DLC = 2;
        tx_header.TransmitGlobalTime = DISABLE;

        uint8_t tx_data[8] = {0};
        tx_data[0] = 0x01;
        tx_data[1] = 0x00;

        if (can_send(hcan1, &tx_header, tx_data, 200) != CAN_OK){

            // printf("can okay %ld\n\r", hcan1->ErrorCode);
            HAL_GPIO_WritePin(PSOM_LED3_PORT, PSOM_LED3_PIN, GPIO_PIN_SET);
        }
        
        while (1){
        
            if (uxQueueMessagesWaiting(IrradQueue) != 0){
                if (xQueueReceive(
                        IrradQueue,
                        &irrad_recieved,
                        200) == pdPASS)
                {
                    
                    printf("Irrad recieved \r\n");
                    
                }
            }

            if (uxQueueMessagesWaiting(ThermoQueue) != 0){
                if (xQueueReceive(
                        ThermoQueue,
                        &thermo_recieved,
                        200) == pdPASS)
                {
                    
                    printf("Thermo recieved \r\n");
                    
                }

            }
            //matched both tasks for data collection
            vTaskDelay(pdMS_TO_TICKS(200));
        }
}

void printfstart(void){
    husart1->Instance = USART1;
    husart1->Init.BaudRate = 115200;
    husart1->Init.WordLength = UART_WORDLENGTH_8B;
    husart1->Init.StopBits = UART_STOPBITS_1;
    husart1->Init.Parity = UART_PARITY_NONE;
    husart1->Init.Mode = UART_MODE_TX_RX;
    husart1->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    husart1->Init.OverSampling = UART_OVERSAMPLING_16;

    printf_init(husart1);
        
}

void canstart(void){
    CAN_FilterTypeDef  sFilterConfig;
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;

    // setup can1 init
    // Baud rate is 250 kbit/s
    hcan1->Init.Prescaler = 20;
    hcan1->Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1->Init.TimeSeg1 = CAN_BS1_13TQ;
    hcan1->Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan1->Init.Mode = CAN_MODE_LOOPBACK;
    hcan1->Init.TimeTriggeredMode = DISABLE;
    hcan1->Init.AutoBusOff = ENABLE;
    hcan1->Init.AutoWakeUp = DISABLE;
    hcan1->Init.AutoRetransmission = ENABLE;
    hcan1->Init.ReceiveFifoLocked = DISABLE;

    // If TransmitFifoPriority is disabled, the hardware selects the mailbox based on the message ID priority. 
    // If enabled, the hardware uses a FIFO mechanism to select the mailbox based on the order of transmission requests.
    hcan1->Init.TransmitFifoPriority = ENABLE;

    // initialize CAN1
    if (can_init(hcan1, &sFilterConfig) != CAN_OK) printf("can init doesn't work");
    if (can_start(hcan1) != CAN_OK) printf("can start doesn't work");
}


int main() {
    HAL_Init();
    SystemClock_Config();


    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef led_config = {0};
    led_config.Mode = GPIO_MODE_OUTPUT_PP;
    led_config.Pull = GPIO_NOPULL;
    led_config.Pin = PSOM_HEARTBEAT_LED_PIN;
    led_config.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PSOM_HEARTBEAT_LED_PORT, &led_config);
    HAL_GPIO_WritePin(PSOM_HEARTBEAT_LED_PORT, PSOM_HEARTBEAT_LED_PIN, GPIO_PIN_SET);

    printfstart();
    // canstart();

    HAL_GPIO_WritePin(PSOM_HEARTBEAT_LED_PORT, PSOM_HEARTBEAT_LED_PIN, 0);

    IrradQueue = xQueueCreateStatic(
        QUEUE_LENGTH,
        sizeof(uint32_t),
        (uint8_t *)IrradQueueStorage,
        &IrradQueueBuffer
    );

    ThermoQueue = xQueueCreateStatic(
        QUEUE_LENGTH,
        sizeof(uint16_t),
        (uint8_t *)ThermoQueueStorage,
        &ThermoQueueBuffer
    );

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

    xTaskCreateStatic(ThermoTask,
                        "Thermo",
                        1024,
                        NULL,
                        tskIDLE_PRIORITY + 2,
                        thermoTaskStack,
                        &thermoTaskTCB);

    // xTaskCreateStatic(CANTask,
    //                     "CAN",
    //                     1024,
    //                     NULL,
    //                     tskIDLE_PRIORITY + 3,
    //                     CANTaskStack,
    //                     &CANTaskTCB);

    vTaskStartScheduler();

    while (1){}

}


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
  }

    //MSP Init for Thermo Code
    GPIO_InitTypeDef GPIO_InitStruct2 = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit2 = {0};
    if(hi2c->Instance==I2C2)
    {
        PeriphClkInit2.PeriphClockSelection = RCC_PERIPHCLK_I2C2;
        PeriphClkInit2.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit2) != HAL_OK)
        {
        Error_Handler();
        }

        __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitStruct2.Pin = GPIO_PIN_10|GPIO_PIN_14;
        GPIO_InitStruct2.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct2.Pull = GPIO_NOPULL;
        GPIO_InitStruct2.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct2.Alternate = GPIO_AF4_I2C2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct2);

        __HAL_RCC_I2C2_CLK_ENABLE();
    }

}



