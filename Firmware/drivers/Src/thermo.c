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
    uint8_t tempread[2];

    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read (handle->hi2c, 
                                            handle->device_addr << 1, 
                                            0x00, 
                                            1, 
                                            tempread, 
                                            2, 
                                            HAL_MAX_DELAY);

    UNUSED(ret);

    return MCP9600_READ_FAIL;
}