#include "irrad.h"
#include "thermo.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pinDefs.h"
#include "projdefs.h"
#include "printf.h"
#include "UART.h"
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

TSL25911FN_HandleTypeDef irrad_handle;
I2C_HandleTypeDef hi2c1;

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

        printf("White Light Irradiance %ld \r\n" , irrad_data.irrad_whitelight_q16);
        printf("Infared Light Irradiance %ld \r\n" , irrad_data.irrad_infrared_q16);

        uint32_t irrad_packed = (irrad_data.irrad_infrared_q16 << 16) + irrad_data.irrad_whitelight_q16;

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
        while (1){
                //maybe 80ms for reading idfk thou just a guess
                vTaskDelay(pdMS_TO_TICKS(200));
        }
}

void CANTask(void *argument){

        uint32_t irrad_recieved;
        while (1){
        
            if (uxQueueMessagesWaiting(IrradQueue) != 0){
                if (xQueueReceive(
                        IrradQueue,
                        &irrad_recieved,
                        200) == pdPASS)
                {
                    printf("Hi \r\n");
                    //can send
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


int main() {
    HAL_Init();
    SystemClock_Config();
    printfstart();

    IrradQueue = xQueueCreateStatic(
        QUEUE_LENGTH,
        sizeof(uint32_t),
        (uint8_t *)IrradQueueStorage,
        &IrradQueueBuffer
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

    xTaskCreateStatic(CANTask,
                        "CAN",
                        1024,
                        NULL,
                        tskIDLE_PRIORITY + 3,
                        CANTaskStack,
                        &CANTaskTCB);
  

    vTaskStartScheduler();

    while (1){}

}