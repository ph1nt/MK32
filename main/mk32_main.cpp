/*
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "driver/touch_pad.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
// OLED
#include "ssd1306.h"
// HID Ble functions
//#include "HID_kbdmousejoystick.h"
#include "hal_ble.h"

// MK32 functions
#include "battery_monitor.h"
#include "keyboard_config.h"
#include "keypress_handles.c"
#include "matrix.h"
#include "nvs_funcs.h"
#include "nvs_keymaps.h"
#include "r_encoder.h"

#define KEY_REPORT_TAG "KEY_REPORT"
#define SYSTEM_REPORT_TAG "KEY_REPORT"
#define TRUNC_SIZE 20
#define USEC_TO_SEC 1000000
#define SEC_TO_MIN 60
// plugin functions
#include "plugins.h"

static config_data_t config;
bool DEEP_SLEEP = true;  // flag to check if we need to go to deep sleep

TaskHandle_t xKeyreportTask;
TaskHandle_t xOledTask;

// handle battery reports over BLE
extern "C" void battery_reports(void *pvParameters) {
  // uint8_t past_battery_report[1] = { 0 };
  while (1) {
    uint32_t bat_level = get_battery_level();
    // if battery level is  100, we're charging
    if (bat_level == 100) {
      // if charging, do not enter deepsleep
      DEEP_SLEEP = false;
    }
    void *pReport = (void *)&bat_level;
    ESP_LOGI("Battery Monitor", "battery level %d", bat_level);
    if (BLE_EN == 1) {
      xQueueSend(battery_q, pReport, (TickType_t)0);
    }
    if (input_str_q != NULL) {
      xQueueSend(input_str_q, pReport, (TickType_t)0);
    }
    vTaskDelay(6 * 1000 / portTICK_PERIOD_MS);  // 6 seconds
  }
}

// Task for continually updating the OLED
extern "C" void oled_task(void *pvParameters) { update_oled(); }

// How to handle key reports
extern "C" void key_reports(void *pvParameters) {
  // Arrays for holding the report at various stages
  uint8_t past_report[REPORT_LEN] = {0};
  uint8_t report_state[REPORT_LEN];
  while (1) {
    memcpy(report_state, check_key_state(layouts[current_layout]),
           sizeof report_state);
    // Do not send anything if queues are uninitialized
    if (mouse_q == NULL || keyboard_q == NULL || joystick_q == NULL) {
      ESP_LOGE(KEY_REPORT_TAG, "queues not initialized");
      continue;
    }
    // Check if the report was modified, if so send it
    if (memcmp(past_report, report_state, sizeof past_report) != 0) {
      DEEP_SLEEP = false;
      void *pReport;
      memcpy(past_report, report_state, sizeof past_report);
#ifndef NKRO
      uint8_t trunc_report[REPORT_LEN] = {0};
      trunc_report[0] = report_state[0];
      trunc_report[1] = report_state[1];
      uint16_t cur_index = 2;
      // Phone's mtu size is usuaully limited to 20 bytes
      for (uint16_t i = 2; i < REPORT_LEN && cur_index < TRUNC_SIZE; ++i) {
        if (report_state[i] != 0) {
          trunc_report[cur_index] = report_state[i];
          ++cur_index;
        }
      }
      pReport = (void *)&trunc_report;
#else
      pReport = (void *)&report_state;
#endif
      if (BLE_EN == 1) {
        xQueueSend(keyboard_q, pReport, (TickType_t)0);
      }
      if (input_str_q != NULL) {
        xQueueSend(input_str_q, pReport, (TickType_t)0);
      }
    }
  }
}

// Handling rotary encoder
extern "C" void encoder_report(void *pvParameters) {
  uint8_t encoder_state = 0;
  uint8_t past_encoder_state = 0;
  while (1) {
    encoder_state = r_encoder_state();
    if (encoder_state != past_encoder_state) {
      DEEP_SLEEP = false;
      r_encoder_command(encoder_state, encoder_map[current_layout]);
      past_encoder_state = encoder_state;
    }
  }
}

// what to do after waking from deep sleep, doesn't seem to work after updating
// esp-idf extern "C" void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
//     rtc_matrix_deinit();;
//     SLEEP_WAKE=true;
// }

/*If no key press has been recieved in SLEEP_MINS amount of minutes, put device
 * into deep sleep wake up on touch on GPIO pin 2
 *  */
#ifdef SLEEP_MINS
extern "C" void deep_sleep(void *pvParameters) {
  uint64_t initial_time = esp_timer_get_time();  // notice that timer returns
                                                 // time passed in microseconds!
  uint64_t current_time_passed = 0;
  while (1) {
    current_time_passed = (esp_timer_get_time() - initial_time);

    if (DEEP_SLEEP == false) {
      current_time_passed = 0;
      initial_time = esp_timer_get_time();
      DEEP_SLEEP = true;
    }

    if (((double)current_time_passed / USEC_TO_SEC) >=
        (double)(SEC_TO_MIN * SLEEP_MINS)) {
      if (DEEP_SLEEP == true) {
        ESP_LOGE(SYSTEM_REPORT_TAG, "going to sleep!");
#ifdef OLED_ENABLE
        vTaskDelay(20 / portTICK_PERIOD_MS);
        vTaskSuspend(xOledTask);
        deinit_oled();
#endif
        // wake up esp32 using rtc gpio
        rtc_matrix_setup();
        esp_sleep_enable_touchpad_wakeup();
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
        esp_deep_sleep_start();
      }
      if (DEEP_SLEEP == false) {
        current_time_passed = 0;
        initial_time = esp_timer_get_time();
        DEEP_SLEEP = true;
      }
    }
  }
}
#endif

extern "C" void app_main() {
  ESP_LOGI("MMK", "start");
  // Reset the rtc GPIOS
  rtc_matrix_deinit();
  // Underclocking for better current draw (not really effective)
  //	esp_pm_config_esp32_t pm_config;
  //	pm_config.max_freq_mhz = 10;
  //	pm_config.min_freq_mhz = 10;
  //	esp_pm_configure(&pm_config);
  matrix_setup();
  esp_err_t ret;
  // Initialize NVS.
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  // Read config
  nvs_handle my_handle;
  ESP_LOGI("MAIN", "loading configuration from NVS");
  ret = nvs_open("config_c", NVS_READWRITE, &my_handle);
  if (ret != ESP_OK) ESP_LOGE("MAIN", "error opening NVS");
  size_t available_size = MAX_BT_DEVICENAME_LENGTH;
  strcpy(config.bt_device_name, GATTS_TAG);
  nvs_get_str(my_handle, "btname", config.bt_device_name, &available_size);
  if (ret != ESP_OK) {
    ESP_LOGE("MAIN", "error reading NVS - bt name, setting to default");
    strcpy(config.bt_device_name, GATTS_TAG);
  } else
    ESP_LOGI("MAIN", "bt device name is: %s", config.bt_device_name);
  esp_log_level_set("*", ESP_LOG_INFO);
  // Loading layouts from nvs (if found)
  nvs_load_layouts();
  // activate keyboard BT stack
  halBLEInit(1, 1, 1, 0);
  ESP_LOGI("HIDD", "MAIN finished...");
  // activate encoder functions
  r_encoder_setup();
  xTaskCreatePinnedToCore(encoder_report, "encoder report", 4096, NULL,
                          configMAX_PRIORITIES, NULL, 1);
  ESP_LOGI("Encoder", "initialized");
  // Start the keyboard Tasks
  // Create the key scanning task on core 1 (otherwise it will crash)
  BLE_EN = 1;
  xTaskCreatePinnedToCore(key_reports, "key report task", 8192, xKeyreportTask,
                          configMAX_PRIORITIES, NULL, 1);
  ESP_LOGI("Keyboard task", "initialized");
  // activate oled
  oled_setup();
  xTaskCreatePinnedToCore(key_reports, "key report task", 8192, xOledTask,
                          configMAX_PRIORITIES, NULL, 1);
  ESP_LOGI("OLED task", "initialized");
  // Start the battery Tasks
  init_batt_monitor();
  xTaskCreatePinnedToCore(battery_reports, "battery report", 4096, NULL,
                          configMAX_PRIORITIES, NULL, 1);
  ESP_LOGI("Battery monitor", "initialized");

#ifdef SLEEP_MINS
  xTaskCreatePinnedToCore(deep_sleep, "deep sleep task", 4096, NULL,
                          configMAX_PRIORITIES, NULL, 1);
  ESP_LOGI("Sleep", "initialized");
#endif
}
}
