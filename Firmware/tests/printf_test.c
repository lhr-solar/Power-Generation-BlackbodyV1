// A simple echo application to test input and output over serial
#include "UART.h"
#include "projdefs.h"
#include "stm32xx_hal.h"
#include "printf.h"

StaticTask_t txTaskBuffer;
StackType_t txTaskStack[configMINIMAL_STACK_SIZE];

void TxTask(void *argument){
    husart1->Instance = USART1;
    husart1->Init.BaudRate = 115200;
    husart1->Init.WordLength = UART_WORDLENGTH_8B;
    husart1->Init.StopBits = UART_STOPBITS_1;
    husart1->Init.Parity = UART_PARITY_NONE;
    husart1->Init.Mode = UART_MODE_TX_RX;
    husart1->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    husart1->Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(husart1);
    printf_init(husart1);

    while(1){
        printf("Hello World!\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();

    xTaskCreateStatic(TxTask,
                      "TX",
                      configMINIMAL_STACK_SIZE,
                      NULL,
                      tskIDLE_PRIORITY + 2,
                      txTaskStack,
                      &txTaskBuffer);

    vTaskStartScheduler();

    while (1) {
    }
}
