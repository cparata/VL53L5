/*
 * This example shows the possibility of VL53L5CX to program detection thresholds. It
 * initializes the VL53L5CX ULD, create 2 thresholds per zone for a 4x4 resolution,
 * and starts a ranging to capture 10 frames.

 * In this example, we also suppose that the number of target per zone is
 * set to 1 , and all output are enabled (see file platform.h).
 *
 *  Copyright (c) 2021 Simon D. Levy
 *
 *  MIT License
 */

#include <Wire.h>

#include "Debugger.hpp"
#include "VL53L5cx.h"

static const uint8_t INT_PIN = 8;
static const uint8_t LPN_PIN = 5;

static VL53L5cx sensor = VL53L5cx(LPN_PIN, 
                                  0x29,  // device address
                                  VL53L5cx::RESOLUTION_8X8, 
                                  VL53L5cx::TARGET_ORDER_CLOSEST,
                                  1);    // ranging frequency 


static volatile bool VL53L5_intFlag;
static void VL53L5_intHandler(void)
{
    VL53L5_intFlag = true;
}

void setup (void)
{
    // Start I^2C
    Wire.begin();

    delay(100);

    // Start serial debugging
    Serial.begin(115200);

    Serial.println("Initializing the sensor ...");

    // Fill the platform structure with customer's implementation. For this
    // example, only the I2C address is used.
    sensor._dev.platform.address = 0x29;

    // Reset the sensor by toggling the LPN pin
    Reset_Sensor(LPN_PIN);

    // Make sure there is a VL53L5CX sensor connected
    uint8_t isAlive = 0;
    vl53l5cx_is_alive(&sensor._dev, &isAlive);

    // Init VL53L5CX sensor
    vl53l5cx_init(&sensor._dev);

    /*********************************/
    /*  Program detection thresholds */
    /*********************************/

    // In this example, we want 2 thresholds per zone for a 4x4 resolution 
    // Create array of thresholds (size cannot be changed) 
    VL53L5CX_DetectionThresholds thresholds[VL53L5CX_NB_THRESHOLDS] = {};

    // Add thresholds for all zones (16 zones in resolution 4x4, or 64 in 8x8)
    for (uint8_t i = 0; i < 16; i++) {
        // The first wanted thresholds is GREATER_THAN mode. Please note that the
        // first one must always be set with a mathematic_operation
        // VL53L5CX_OPERATION_NONE.
        // For this example, the signal thresholds is set to 150 kcps/spads
        // (the format is automatically updated inside driver)
        //
        thresholds[2*i].zone_num = i;
        thresholds[2*i].measurement = VL53L5CX_SIGNAL_PER_SPAD_KCPS;
        thresholds[2*i].type = VL53L5CX_GREATER_THAN_MAX_CHECKER;
        thresholds[2*i].mathematic_operation = VL53L5CX_OPERATION_NONE;
        thresholds[2*i].param_low_thresh = 150;
        thresholds[2*i].param_high_thresh = 150;

        // The second wanted checker is IN_WINDOW mode. We will set a
        // mathematical thresholds VL53L5CX_OPERATION_OR, to add the previous
        // checker to this one.
        // For this example, distance thresholds are set between 200mm and
        // 400mm (the format is automatically updated inside driver).
        thresholds[2*i+1].zone_num = i;
        thresholds[2*i+1].measurement = VL53L5CX_DISTANCE_MM;
        thresholds[2*i+1].type = VL53L5CX_IN_WINDOW;
        thresholds[2*i+1].mathematic_operation = VL53L5CX_OPERATION_OR;
        thresholds[2*i+1].param_low_thresh = 200;
        thresholds[2*i+1].param_high_thresh = 400;
    }

    // The last thresholds must be clearly indicated. As we have 32
    // checkers (16 zones x 2), the last one is the 31
    thresholds[31].zone_num = VL53L5CX_LAST_THRESHOLD | thresholds[31].zone_num;

    // Send array of thresholds to the sensor 
    vl53l5cx_set_detection_thresholds(&sensor._dev, thresholds);

    // Enable detection thresholds
    vl53l5cx_set_detection_thresholds_enable(&sensor._dev, 1);

    vl53l5cx_set_ranging_frequency_hz(&sensor._dev, 10);

    // Set up interrupt
    pinMode(INT_PIN, INPUT); 
    attachInterrupt(INT_PIN, VL53L5_intHandler, FALLING);

    vl53l5cx_start_ranging(&sensor._dev);

    Serial.println("Put an object between 200mm and 400mm to catch an interrupt\n");
    delay(2000);

} // setup

void loop(void)
{
    static uint32_t loop_count;

    if (loop_count < 100) {

        if (VL53L5_intFlag) {

            VL53L5_intFlag = false;

            vl53l5cx_get_ranging_data(&sensor._dev, &sensor._results);

            // As the sensor is set in 4x4 mode by default, we have a total
            // of 16 zones to print. For this example, only the data of
            // first zone are print
            Debugger::printf("Print data no : %3u\n", sensor._dev.streamcount);
            for (uint8_t i = 0; i < 16; i++)
            {
                Debugger::printf("Zone : %3d, Status : %3u, Distance : %4d mm, Signal : %5lu kcps/SPADs\n",
                        i,
                        sensor._results.target_status[VL53L5CX_NB_TARGET_PER_ZONE*i],
                        sensor._results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*i],
                        sensor._results.signal_per_spad[VL53L5CX_NB_TARGET_PER_ZONE*i]);
            }
            Debugger::printf("\n");
            loop_count++;
        }

        // Wait a few ms to avoid too high polling (function in platform file, not in API) 
        delay(5);
    }

    else if (loop_count == 100) {
        vl53l5cx_stop_ranging(&sensor._dev);
        loop_count++;
    }

    else {
        Debugger::printf("End of ULD demo\n");
        delay(500);
    }

} // loop
