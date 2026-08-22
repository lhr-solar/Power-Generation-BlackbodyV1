#include "irrad.h"
#include <stdint.h>

volatile uint8_t tsl_i2c_tx_done = 0;
volatile uint8_t tsl_i2c_error = 0;

uint32_t gain_ch0;
uint32_t gain_ch1;


tsl25911fn_status_t tsl25911fn_init(TSL25911FN_HandleTypeDef *handle, I2C_HandleTypeDef *hi2c){

    hi2c->Instance = I2C1;
    hi2c->Init.Timing = 0x00100D14;
    hi2c->Init.OwnAddress1 = 0;
    hi2c->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c->Init.OwnAddress2 = 0;
    hi2c->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    handle->hi2c = hi2c;

    if (HAL_I2C_Init(hi2c) != HAL_OK)
    {
        return TSL25911FN_INIT_FAIL;
    }

    if (HAL_I2CEx_ConfigAnalogFilter(hi2c, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
        return TSL25911FN_WRITE_FAIL;
    }

    if (HAL_I2CEx_ConfigDigitalFilter(hi2c, 0) != HAL_OK)
    {
        return TSL25911FN_WRITE_FAIL;
    }

    handle->device_addr = TSL25911FN_7BIT_ADDRESS;
    handle->gain = TSL25911FN_GAIN_LOW;
    handle->time = TSL25911FN_TIME_100MS;
    handle->control = handle->gain | handle->time;

    switch(handle->gain){
        case TSL25911FN_GAIN_LOW:
            gain_ch0 = TSL25911FN_GAIN_LOW_MULTI_CH0;
            gain_ch1 = TSL25911FN_GAIN_LOW_MULTI_CH1;
            break;
        case TSL25911FN_GAIN_MED:
            gain_ch0 = TSL25911FN_GAIN_MED_MULTI_CH0;
            gain_ch1 = TSL25911FN_GAIN_MED_MULTI_CH1;
            break;
        case TSL25911FN_GAIN_HIGH: 
            gain_ch0 = TSL25911FN_GAIN_HIGH_MULTI_CH0;
            gain_ch1 = TSL25911FN_GAIN_HIGH_MULTI_CH1;
            break;
        case TSL25911FN_GAIN_MAX:
            gain_ch0 = TSL25911FN_GAIN_MAX_MULTI_CH0;
            gain_ch1 = TSL25911FN_GAIN_MAX_MULTI_CH1;
            break;

    }

    return TSL25911FN_OK;
}


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

    tsl_i2c_tx_done = 0;
    tsl_i2c_error = 0;

    cmd_reg = TSL25911FN_REG_CMD | reg;

    payload[0]=cmd_reg;
    payload[1]=value;
    TickType_t start;



    if (HAL_I2C_Master_Transmit(handle->hi2c,
                            (handle->device_addr << 1),
                            payload,
                            2,
                            100) != HAL_OK)
    {
        return TSL25911FN_WRITE_FAIL;
    }

    start = xTaskGetTickCount();

    while(!tsl_i2c_tx_done && !tsl_i2c_error){
        if((xTaskGetTickCount() - start) >=delay){
            return TSL25911FN_WRITE_FAIL;
        }
    }

    if (tsl_i2c_error)
    {
        return TSL25911FN_WRITE_FAIL;
    }

    return TSL25911FN_OK;
}

tsl25911fn_status_t tsl25911fn_read_reg(TSL25911FN_HandleTypeDef *handle,
                                        uint8_t reg,
                                        volatile uint8_t *value,
                                        TickType_t delay)
{
    uint8_t cmd_reg;

    if (handle == 0 || handle->hi2c == 0 || value == 0){
        return TSL25911FN_INIT_FAIL;
    }

    cmd_reg = TSL25911FN_REG_CMD | reg;

    if(HAL_I2C_Mem_Read(handle->hi2c,
                            (handle->device_addr << 1),
                            cmd_reg,
                            I2C_MEMADD_SIZE_8BIT,
                            (uint8_t *)value,
                            1,
                            HAL_MAX_DELAY) != HAL_OK)
    
    {
        return TSL25911FN_READ_FAIL;
    }

    return TSL25911FN_OK;
}



tsl25911fn_status_t tsl25911fn_power_on(TSL25911FN_HandleTypeDef *handle, 
                                        TickType_t delay)
{
    
    if (handle == 0 || handle->hi2c == 0){
        return TSL25911FN_INIT_FAIL;
    }

    return tsl25911fn_write_reg(handle, 
                                TSL25911FN_REG_ENABLE, 
                                TSL25911FN_ENABLE_POWER_ON, 
                                delay);


}

tsl25911fn_status_t tsl25911fn_power_off(TSL25911FN_HandleTypeDef *handle, 
                                        TickType_t delay)
{
    
    if (handle == 0 || handle->hi2c == 0){
        return TSL25911FN_INIT_FAIL;
    }

    return tsl25911fn_write_reg(handle, 
                                TSL25911FN_REG_ENABLE, 
                                TSL25911FN_ENABLE_POWER_OFF, 
                                delay);


}


tsl25911fn_status_t tsl25911fn_set_control(TSL25911FN_HandleTypeDef *handle, 
                                        uint8_t control, 
                                        TickType_t delay)
{
    if (handle == 0 || handle->hi2c == 0){
        return TSL25911FN_INIT_FAIL;
    }

    handle->control = control;

    return tsl25911fn_write_reg(handle, 
                                TSL25911FN_REG_CONTROL, 
                                handle->control, 
                                delay);
}

tsl25911fn_status_t tsl25911fn_read_channels(TSL25911FN_HandleTypeDef *handle, 
                                uint16_t *ch0, 
                                uint16_t *ch1, 
                                TickType_t delay)
{
    uint8_t c0l, c0h, c1l, c1h;
    tsl25911fn_status_t status;

    if (handle == 0 || handle->hi2c == 0 || ch0 == 0 || ch1 == 0) {
        return TSL25911FN_INIT_FAIL;
    }

    status = tsl25911fn_read_reg(handle, TSL25911FN_REG_C0DATAL, &c0l, delay);
    if (status != TSL25911FN_OK) return status;

    status = tsl25911fn_read_reg(handle, TSL25911FN_REG_C0DATAH, &c0h, delay);
    if (status != TSL25911FN_OK) return status;

    status = tsl25911fn_read_reg(handle, TSL25911FN_REG_C1DATAL, &c1l, delay);
    if (status != TSL25911FN_OK) return status;

    status = tsl25911fn_read_reg(handle, TSL25911FN_REG_C1DATAH, &c1h, delay);
    if (status != TSL25911FN_OK) return status;

    *ch0 = ((uint16_t)c0h << 8) | c0l;
    *ch1 = ((uint16_t)c1h << 8) | c1l;

    return TSL25911FN_OK;
}

tsl25911fn_status_t tsl25911fn_read_data(TSL25911FN_HandleTypeDef *handle, 
                                        TSL25911FN_data_t *data, 
                                        TickType_t delay)
{
    tsl25911fn_status_t status;

    if (handle == 0 || handle->hi2c == 0 || data == 0) {
        return TSL25911FN_INIT_FAIL;
    }

    status = tsl25911fn_read_channels(handle, &data->ch0, &data->ch1, delay);

    if (status != TSL25911FN_OK) {
        return status;
    }

        data->irrad_whitelight_q16 = (((uint64_t)data->ch0 * 4000000ULL * 10ULL) << 16) / (2641000ULL * gain_ch0); //math for low

        data->irrad_infrared_q16 = (((uint64_t)data->ch1 * 4000000ULL * 10ULL) << 16) / (1541000ULL * gain_ch1); //math for low

    return TSL25911FN_OK;
}