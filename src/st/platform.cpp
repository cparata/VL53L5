/*
*  VL53L5CX ULD basic example    
*
*  Copyright (c) 2021 Seth Bonn, Simon D. Levy
*
*  MIT License
*/

#include "platform.h"
#include "Debugger.hpp"

#include <Arduino.h>
#include <Wire.h>


#ifndef DEFAULT_I2C_BUFFER_LEN
#ifdef BUFFER_LENGTH
#define DEFAULT_I2C_BUFFER_LEN  BUFFER_LENGTH
#else
#define DEFAULT_I2C_BUFFER_LEN  32
#endif
#endif

// Helper
static void start_transfer(uint16_t register_address)
{
    uint8_t buffer[2] {(uint8_t)(register_address >> 8),
                       (uint8_t)(register_address & 0xFF) }; 
    Wire.write(buffer, 2);
}

// All these functions return 0 on success, nonzero on error

uint8_t RdByte(
        VL53L5CX_Platform *p_platform,
        uint16_t RegisterAddress,
        uint8_t *p_value)
{

    uint8_t res = RdMulti(p_platform, RegisterAddress, p_value, 1);

    return res;
}

uint8_t RdMulti(
        VL53L5CX_Platform *p_platform,
        uint16_t RegisterAddress,
        uint8_t *p_values,
        uint32_t size)
{
    // Partially based on https://github.com/stm32duino/VL53L1 VL53L1_I2CRead()

    int status = 0;

    // Loop until the port is transmitted correctly
    do {
        Wire.beginTransmission((uint8_t)((p_platform->address >> 1) & 0x7F));

        start_transfer(RegisterAddress);

        status = Wire.endTransmission(false);

        // Fix for some STM32 boards
        // Reinitialize the i2c bus with the default parameters
#ifdef ARDUINO_ARCH_STM32
            if (status) {
                Wire.end();
                Wire.begin();
            }
#endif
        // End of fix

    } while (status != 0);

    uint32_t i = 0;
    if(size > DEFAULT_I2C_BUFFER_LEN)
    {
        while(i < size)
        {
            // If still more than DEFAULT_I2C_BUFFER_LEN bytes to go, DEFAULT_I2C_BUFFER_LEN,
            // else the remaining number of bytes
            uint8_t current_read_size = (size - i > DEFAULT_I2C_BUFFER_LEN ? DEFAULT_I2C_BUFFER_LEN : size - i);
            Wire.requestFrom(((uint8_t)((p_platform->address >> 1) & 0x7F)),
                    current_read_size);
            while (Wire.available()) {
                p_values[i] = Wire.read();
                i++;
            }
        }
    }
    else
    {
        Wire.requestFrom(((uint8_t)((p_platform->address >> 1) & 0x7F)), size);
        while (Wire.available()) {
            p_values[i] = Wire.read();
            i++;
        }
    }
    
    return i != size;
}

uint8_t WrByte(
        VL53L5CX_Platform *p_platform,
        uint16_t RegisterAddress,
        uint8_t value)
{
    // Just use WrMulti but 1 byte
    uint8_t res = WrMulti(p_platform, RegisterAddress, &value, 1); 
    return res;
}

uint8_t WrMulti(
        VL53L5CX_Platform *p_platform,
        uint16_t RegisterAddress,
        uint8_t *p_values,
        uint32_t size)
{
    uint32_t i = 0;

    while(i < size)
    {
        // If still more than DEFAULT_I2C_BUFFER_LEN bytes to go, DEFAULT_I2C_BUFFER_LEN,
        // else the remaining number of bytes
        size_t current_write_size = (size - i > DEFAULT_I2C_BUFFER_LEN ? DEFAULT_I2C_BUFFER_LEN : size - i);

        Wire.beginTransmission((uint8_t)((p_platform->address >> 1) & 0x7F));
        // Target register address for transfer
        start_transfer(RegisterAddress + i);
        if (Wire.write(p_values + i, current_write_size) == 0)
        {
            Debugger::reportForever(
                "WrMulti failed to send %d bytes to regsiter 0x%02X",
                size, RegisterAddress);
        }
        else
        {
            i += current_write_size;
            if (size - i)
            {
                // Flush buffer but do not send stop bit so we can keep going
                Wire.endTransmission(false);
            }
        }
    }

    return Wire.endTransmission(true);
}

uint8_t Reset_Sensor(uint8_t lpn_pin)
{
    // Set pin LPN to LOW
    // (Set pin AVDD to LOW)
    // (Set pin VDDIO  to LOW)
    pinMode(lpn_pin, OUTPUT);
    digitalWrite(lpn_pin, LOW);
    delay(100);

    // Set pin LPN of to HIGH
    // (Set pin AVDD of to HIGH)
    // (Set pin VDDIO of  to HIGH)
    digitalWrite(lpn_pin, HIGH);
    delay(100);

    return 0;
}

void SwapBuffer( uint8_t   *buffer, uint16_t     size) {

    // Example of possible implementation using <string.h>
    for(uint32_t i = 0; i < size; i = i + 4) {

        uint32_t tmp = (
                buffer[i]<<24)
            |(buffer[i+1]<<16)
            |(buffer[i+2]<<8)
            |(buffer[i+3]);

        memcpy(&(buffer[i]), &tmp, 4);
    }
} 

uint8_t WaitMs(
        VL53L5CX_Platform *p_platform,
        uint32_t TimeMs)
{
    (void)p_platform;
    delay(TimeMs);

    return 0;
}
