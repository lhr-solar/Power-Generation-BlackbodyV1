#include "thermo.h"

volatile uint8_t mcp_i2c_tx_done = 0;
volatile uint8_t mcp_i2c_rx_done = 0;
volatile uint8_t mcp_i2c_error = 0;

mcp9600_status_t mcp9600_init(MCP9600_HandleTypeDef *handle,
                              I2C_HandleTypeDef *hi2c,
                              uint8_t addr)
{
    if (handle == NULL || hi2c == NULL)
    {
        return MCP9600_INIT_FAIL;
    }

    handle->hi2c = hi2c;
    handle->device_addr = addr;

    return MCP9600_OK;
}



mcp9600_status_t mcp9600_read_hot_junction(MCP9600_HandleTypeDef *handle,
                                            int32_t *temp_int,
                                            int32_t *temp_frac,
                                            TickType_t delay)

{
    uint8_t final_temp_reg = MCP9600_FINAL_TEMP_REG;
    uint8_t tempread[2];
    TickType_t start_tick = xTaskGetTickCount();

    while (HAL_I2C_GetState(handle->hi2c) != HAL_I2C_STATE_READY)
    {
        if ((xTaskGetTickCount() - start_tick) >= delay)
        {
            uint32_t err = HAL_I2C_GetError(handle->hi2c);
            printf("I2C busy err=0x%08lx\r\n", err);
            return MCP9600_READ_FAIL;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }   
    
    start_tick = xTaskGetTickCount();
    mcp_i2c_tx_done = 0;
    mcp_i2c_rx_done = 0;
    mcp_i2c_error = 0;

    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit_IT(handle->hi2c,
                                                        handle->device_addr << 1,
                                                        &final_temp_reg,
                                                        1);

    if (ret != HAL_OK)
    {
        printf("ret=%d\n\r", ret);
        uint32_t err = HAL_I2C_GetError(handle->hi2c);

        printf("I2C ret=%d err=0x%08lx\r\n", ret, err);
        return MCP9600_READ_FAIL;
    }

    while ((mcp_i2c_rx_done == 0) && (mcp_i2c_error == 0))
    {
        if ((xTaskGetTickCount() - start_tick) >= delay)
        {
            printf("timeout state=0x%lx err=0x%08lx rx=%u error=%u\r\n",
                (unsigned long)HAL_I2C_GetState(handle->hi2c),
                (unsigned long)HAL_I2C_GetError(handle->hi2c),
                (unsigned int)mcp_i2c_rx_done,
                (unsigned int)mcp_i2c_error);

            HAL_I2C_Master_Abort_IT(handle->hi2c, handle->device_addr << 1);

            return MCP9600_READ_FAIL;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    if (mcp_i2c_error != 0)
    {
        uint32_t err = HAL_I2C_GetError(handle->hi2c);
        printf("I2C ret=%d err=0x%08lx\r\n", ret, err);
        return MCP9600_READ_FAIL;
    }

    start_tick = xTaskGetTickCount();

    while (HAL_I2C_GetState(handle->hi2c) != HAL_I2C_STATE_READY)
    {
        if ((xTaskGetTickCount() - start_tick) >= delay)
        {
            uint32_t err = HAL_I2C_GetError(handle->hi2c);
            printf("I2C busy after tx err=0x%08lx\r\n", (unsigned long)err);
            return MCP9600_READ_FAIL;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    start_tick = xTaskGetTickCount();
    mcp_i2c_rx_done = 0;
    mcp_i2c_error = 0;

    ret = HAL_I2C_Master_Receive_IT(handle->hi2c,
                                     handle->device_addr << 1,
                                    tempread,
                                    2);

    if (ret != HAL_OK)
    {
        printf("ret=%d\n\r", ret);
        uint32_t err = HAL_I2C_GetError(handle->hi2c);

        printf("I2C ret=%d err=0x%08lx\r\n", ret, err);
        return MCP9600_READ_FAIL;
    }
    return MCP9600_READ_FAIL;
}