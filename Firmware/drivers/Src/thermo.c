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

    hi2c->Instance = I2C2;
    hi2c->Init.Timing = 0x10909CEC; //0x00100D14
    hi2c->Init.OwnAddress1 = 0;
    hi2c->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c->Init.OwnAddress2 = 0;
    hi2c->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(hi2c) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_I2CEx_ConfigAnalogFilter(hi2c, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_I2CEx_ConfigDigitalFilter(hi2c, 0) != HAL_OK)
    {
        Error_Handler();
    }

    handle->hi2c = hi2c;
    handle->device_addr = addr;

    return MCP9600_OK;
}



mcp9600_status_t mcp9600_read_hot_junction(MCP9600_HandleTypeDef *handle,
                                            int32_t *temp_int,
                                            int32_t *temp_frac,
                                            uint16_t *raw_temperature,
                                            TickType_t delay)

{
    uint8_t tempread[2] = {0};
    int32_t temperature_x10000;

    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read (handle->hi2c, 
                                            handle->device_addr << 1, 
                                            0x00, 
                                            1, 
                                            tempread, 
                                            2, 
                                            HAL_MAX_DELAY);

    UNUSED(ret);

    *raw_temperature = (((uint16_t)tempread[0] << 8) | (uint16_t)tempread[1]);

    temperature_x10000 = (int32_t)*raw_temperature * 625;

    *temp_int = temperature_x10000 / 10000;
    *temp_frac = temperature_x10000 % 10000;

    if (*temp_frac < 0)
    {
        *temp_frac = -*temp_frac;
    }

    return MCP9600_READ_FAIL;
}