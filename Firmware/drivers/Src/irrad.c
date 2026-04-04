#include "irrad.h"
#include <stdint.h>

volatile uint8_t tsl_i2c_tx_done = 0;
volatile uint8_t tsl_i2c_error = 0;

tsl25911fn_status_t tsl25911fn_write_reg(TSL25911FN_HandleTypeDef *handle,
                                         uint8_t reg,
                                         uint8_t value,
                                         TickType_t delay)
{
    uint8_t payload[2];
    uint8_t cmd_reg;

    if (handle == 0 || handle->hi2c == 0){
        return TSL25911FN_INIT_FAIL;
    }

    cmd_reg = TSL25911FN_REG_CMD | reg;

    payload[0]=cmd_reg;
    payload[1]=value;

    if (HAL_I2C_Master_Transmit_IT(handle->hi2c,
                            (handle->device_id << 1),
                            payload,
                            2) != HAL_OK)
    {
        return TSL25911FN_WRITE_FAIL;
    }

    if (delay > 0)
    {
        vTaskDelay(delay);
    }

    return TSL25911FN_OK;
}

tsl25911fn_status_t tsl25911fn_read_reg(TSL25911FN_HandleTypeDef *handle,
                                         uint8_t reg,
                                         uint8_t *value,
                                        TickType_t delay)
{
    uint8_t cmd_reg;

    if (handle == 0 || handle->hi2c == 0 || value == 0){
        return TSL25911FN_INIT_FAIL;
    }

    cmd_reg = TSL25911FN_REG_CMD | reg;

    if(HAL_I2C_Mem_Read(handle->hi2c,
                            (handle->device_id << 1),
                            cmd_reg,
                            I2C_MEMADD_SIZE_8BIT,
                            value,
                            1,
                            HAL_MAX_DELAY) != HAL_OK)
    
    {
        return TSL25911FN_READ_FAIL;
    }

    return TSL25911FN_OK;
}





