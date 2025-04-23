#pragma once
#include "beaconData.h"
#include "esp_timer.h"
 

typedef struct {
   uint64_t counter;     // message serial number 
   uint32_t millis;      // milliseconds since server boot.
   int32_t  epochDelta;  // 
   uint16_t versionMajor;
   uint16_t versionMinor;
   uint16_t versionBuild;
} __attribute__((packed)) custom_beacon_data_t;

#include "version.h"

uint64_t getTickBeacon() {
   if (versionData.epochDelta==0) {
         struct timeval now;
         gettimeofday(&now, nullptr);
         return static_cast<uint64_t>(now.tv_sec) * 1000 + now.tv_usec / 1000;     
   }
   return  (  esp_timer_get_time() + versionData.epochDelta ) / 1000;
}
