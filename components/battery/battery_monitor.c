/*
 * after after a voltage divider and checking the level on an analog pin
 * This battery monitoring code is based on reading the voltage
 * Based on the adc example from Espressif
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Copyright 2018 Gal Zaidenstein.
 */

#include "battery_monitor.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"
#include "esp_system.h"
#include "keyboard_config.h"

#define DEFAULT_VREF 1100  // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES 50   // Multisampling

static const adc_channel_t channel = BATT_PIN;
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;

uint32_t voltage = 0;

static esp_adc_cal_characteristics_t *adc_chars;
// check battery level

uint32_t get_battery_level(void) {
    uint32_t adc_reading = 0;
    // Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        adc_reading += adc1_get_raw((adc1_channel_t)channel);
    }
    adc_reading /= NO_OF_SAMPLES;
    // Convert adc_reading to voltage in mV
    voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    int32_t battery_percent = (voltage - Vout_min) * 100 / (Vout_max - Vout_min);
    battery_percent = (battery_percent < 100) ? battery_percent : 100;
    battery_percent = (battery_percent < 0) ? 0 : battery_percent;
    // ESP_LOGI("Batt", "Raw: %d\tVoltage: %dmV\tPercent: %d", adc_reading, voltage*2, battery_percent);
    return battery_percent;
}

// -K5px:VJFs5QUDY
// initialize battery monitor pin
void init_batt_monitor(void) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(BATT_PIN, atten);
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF,
                             adc_chars);
}
