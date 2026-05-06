#ifndef SYSTEM_OLED_SHOW_H
#define SYSTEM_OLED_SHOW_H

#include "GetBatteryLevel.h"
#include "OLED_driver.h"
#include "RTC_time.h"
#include "OLED_Data.h"
#include "WIFI_manager.h"
#include "MAX30102.h"
#include "MPU6050.h"

void OLEDShowTask(void *pvParameters);

#endif
