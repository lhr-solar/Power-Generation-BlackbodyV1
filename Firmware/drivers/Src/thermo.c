#include "thermo.h"

volatile uint8_t mcp_i2c_tx_done = 0;
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
    (void)delay;

    uint8_t tempread[2];

    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(handle->hi2c,
                                             MCP9600_7BIT_ADDR_6 << 1,
                                             0x00,
                                             I2C_MEMADD_SIZE_8BIT,
                                             tempread,
                                             2,
                                             delay);

    if (ret != HAL_OK)
    {
        uint32_t err = HAL_I2C_GetError(handle->hi2c);
        printf("I2C ret=%d err=0x%08lx\r\n", ret, err);
        return MCP9600_READ_FAIL;
    }

    int16_t temp_fixed = (int16_t)((tempread[0] << 8) | tempread[1]);

    *temp_int = temp_fixed / 16;
    *temp_frac = (temp_fixed % 16) * 625;

    if (*temp_frac < 0)
    {
        *temp_frac = -(*temp_frac);
    }

    return MCP9600_OK;
}