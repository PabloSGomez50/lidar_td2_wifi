
/* 
* This file is part of VL53L1 Platform 
* 
* Copyright (c) 2016, STMicroelectronics - All Rights Reserved 
* 
* License terms: BSD 3-clause "New" or "Revised" License. 
* 
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are met: 
* 
* 1. Redistributions of source code must retain the above copyright notice, this 
* list of conditions and the following disclaimer. 
* 
* 2. Redistributions in binary form must reproduce the above copyright notice, 
* this list of conditions and the following disclaimer in the documentation 
* and/or other materials provided with the distribution. 
* 
* 3. Neither the name of the copyright holder nor the names of its contributors 
* may be used to endorse or promote products derived from this software 
* without specific prior written permission. 
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
* 
*/

#include "vl53l1_platform.h"

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const TickType_t k_i2c_timeout_ticks = pdMS_TO_TICKS(1000);
static i2c_master_dev_handle_t s_dev_handle = NULL;

void vl53l1x_set_i2c_device(i2c_master_dev_handle_t handle)
{
    s_dev_handle = handle;
}

static int check_i2c_handle(void)
{
    return (s_dev_handle != NULL) ? 0 : -1;
}

int _I2CWrite(uint16_t Dev, uint8_t *pdata, uint32_t count) {
    if (check_i2c_handle() != 0) {
        return -1;
    }

    esp_err_t status = i2c_master_transmit(
        s_dev_handle,
        pdata,
        count,
        k_i2c_timeout_ticks);

    (void)Dev;
    return (status == ESP_OK) ? 0 : -1;
}

int _I2CRead(uint16_t Dev, uint8_t *pdata, uint32_t count) {
    if (check_i2c_handle() != 0) {
        return -1;
    }

    esp_err_t status = i2c_master_receive(
        s_dev_handle,
        pdata,
        count,
        k_i2c_timeout_ticks);

    (void)Dev;
    return (status == ESP_OK) ? 0 : -1;
}


VL53L1_Error VL53L1_WriteMulti( uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
    uint8_t _I2CBuffer[256];
    int status_int;
    VL53L1_Error Status = VL53L1_ERROR_NONE;

    if (count > sizeof(_I2CBuffer) - 2) {
        return VL53L1_ERROR_INVALID_PARAMS;
    }

    _I2CBuffer[0] = index >> 8;
    _I2CBuffer[1] = index & 0xFF;
    memcpy(&_I2CBuffer[2], pdata, count);

    status_int = _I2CWrite(dev, _I2CBuffer, count + 2);
    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
    }

    return Status;
}

VL53L1_Error VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count){
    uint8_t _I2CBuffer[2];
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    int status_int;

    _I2CBuffer[0] = index >> 8;
    _I2CBuffer[1] = index & 0xFF;

    status_int = _I2CWrite(dev, _I2CBuffer, 2);
    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
        return Status;
    }

    status_int = _I2CRead(dev, pdata, count);
    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
    }

    return Status;
}

VL53L1_Error VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data) {
    uint8_t _I2CBuffer[3];
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    int status_int;

    _I2CBuffer[0] = index >> 8;
    _I2CBuffer[1] = index & 0xFF;
    _I2CBuffer[2] = data;

    status_int = _I2CWrite(dev, _I2CBuffer, 3);
    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
    }

    return Status;
}

VL53L1_Error VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data) {
    uint8_t _I2CBuffer[4];
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    int status_int;

    _I2CBuffer[0] = index >> 8;
    _I2CBuffer[1] = index & 0xFF;
    _I2CBuffer[2] = data >> 8;
    _I2CBuffer[3] = data & 0xFF;

    status_int = _I2CWrite(dev, _I2CBuffer, 4);
    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
    }

    return Status;
}

VL53L1_Error VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data) {
    uint8_t _I2CBuffer[6];
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    int status_int;

    _I2CBuffer[0] = index >> 8;
    _I2CBuffer[1] = index & 0xFF;
    _I2CBuffer[2] = (data >> 24) & 0xFF;
    _I2CBuffer[3] = (data >> 16) & 0xFF;
    _I2CBuffer[4] = (data >> 8) & 0xFF;
    _I2CBuffer[5] = data & 0xFF;

    status_int = _I2CWrite(dev, _I2CBuffer, 6);
    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
    }

    return Status;
}

VL53L1_Error VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *data) {
    uint8_t _I2CBuffer[2];
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    int status_int;

    _I2CBuffer[0] = index >> 8;
    _I2CBuffer[1] = index & 0xFF;

    status_int = _I2CWrite(dev, _I2CBuffer, 2);
    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
        return Status;
    }

    status_int = _I2CRead(dev, data, 1);
    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
    }

    return Status;
}

VL53L1_Error VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *data) {
    uint8_t _I2CBuffer[2];
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    int status_int;

    _I2CBuffer[0] = index >> 8;
    _I2CBuffer[1] = index & 0xFF;

    status_int = _I2CWrite(dev, _I2CBuffer, 2);
    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
        return Status;
    }

    status_int = _I2CRead(dev, _I2CBuffer, 2);
    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
        return Status;
    }

    *data = ((uint16_t)_I2CBuffer[0] << 8) | _I2CBuffer[1];
    return Status;
}

VL53L1_Error VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data) {
    uint8_t _I2CBuffer[4];
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    int status_int;

    uint8_t index_buf[2];
    index_buf[0] = index >> 8;
    index_buf[1] = index & 0xFF;

    status_int = _I2CWrite(dev, index_buf, 2);
    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
        return Status;
    }

    status_int = _I2CRead(dev, _I2CBuffer, 4);
    if (status_int != 0) {
        Status = VL53L1_ERROR_CONTROL_INTERFACE;
        return Status;
    }

    *data = ((uint32_t)_I2CBuffer[0] << 24) | ((uint32_t)_I2CBuffer[1] << 16) |
            ((uint32_t)_I2CBuffer[2] << 8) | _I2CBuffer[3];
    return Status;
}

VL53L1_Error VL53L1_WaitMs(uint16_t dev, int32_t wait_ms){
    (void)dev;
    if (wait_ms <= 0) {
        return VL53L1_ERROR_NONE;
    }

    vTaskDelay(pdMS_TO_TICKS(wait_ms));
    return VL53L1_ERROR_NONE;
}
