#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"       // Contains esp_wifi_set_vendor_ie
#include "esp_wifi_types.h" // Likely contains wifi_vendor_ie_type_t, etc.
#include <sys/time.h>
#include "beaconData.h"
#include "customBeaconData.h" 


// --- Configuration ---
// ... (ssid, password, channel remain the same) ...
uint8_t oui[3] = {0x00, 0x11, 0x22}; // Example OUI

custom_beacon_data_t beaconData = {0,0,0};
uint8_t local_mac[6];

uint32_t syncStarted = 0;
bool timeSynced = false;

uint32_t lastNtpResync = 0;

String ntpServer = "pool.ntp.org";
long gmtOffset = 0;
int daylightOffset = 0;


void resync() {
    syncStarted = millis();
    configTime(gmtOffset, daylightOffset, ntpServer.c_str());
    timeSynced = false;
    lastNtpResync = millis();
}

// Define the structure expected by esp_wifi_set_vendor_ie
// Need enough space for element_id, length, and our custom payload
typedef struct {
    uint8_t element_id; // Should be 221
    uint8_t length;     // Length of the payload that follows (sizeof(custom_payload_t))
    base_beacon_data_t payload; // Contains OUI, type, counter
} __attribute__((packed)) vendor_ie_buffer_t;

vendor_ie_buffer_t vendor_ie_data;

// (Keep the includes, configurations, and structure definitions the same)

// Function to update the Vendor Specific IE using the older API (Disable/Enable method)
void update_vendor_ie( void *dataPayload, uint8_t dataPayloadSize) {
 

 
  // Fill the main vendor IE structure (same as before)
  // Ensure WIFI_VENDOR_IE_ELEMENT_ID is defined correctly (should be 221)
  // You might need to explicitly use 221 if the define isn't available
  vendor_ie_data.element_id = 221; // WIFI_VENDOR_IE_ELEMENT_ID;
  vendor_ie_data.length =   preparePayloadPacket(&vendor_ie_data.payload, (uint8_t*) dataPayload, dataPayloadSize );

  esp_err_t err_disable = ESP_FAIL;
  esp_err_t err_enable = ESP_FAIL;

  // 1. Try to disable the existing IE for this type and ID
  //    When disabling, the data pointer argument is often ignored (can be NULL).
  err_disable = esp_wifi_set_vendor_ie(
      false,                   // Disable IE
      WIFI_VND_IE_TYPE_BEACON, // Type of IE to disable
      WIFI_VND_IE_ID_0,        // ID of the IE slot to disable
      NULL                     // Data pointer (usually ignored/NULL when disabling)
  );

  // Check if disable was successful OR if it failed because it wasn't set yet (ESP_ERR_NOT_FOUND might be ok too)
  // We primarily care that it's not in an error state preventing us from enabling.
  // Let's proceed to enable even if disable returned an error, but log it.
  if (err_disable != ESP_OK) {
       Serial.printf("Note: Failed to disable Beacon IE before update (Error: %s). Attempting enable anyway...\n", esp_err_to_name(err_disable));
  }

  // 2. Enable the IE with the new data
  err_enable = esp_wifi_set_vendor_ie(
      true,                    // Enable IE
      WIFI_VND_IE_TYPE_BEACON, // Type of IE to enable
      WIFI_VND_IE_ID_0,        // ID of the IE slot to enable
      &vendor_ie_data          // Pointer to the *entire* IE structure (ID, Length, Payload)
  );


  // Report final status based on the enable call
  if (err_enable == ESP_OK) {
      Serial.println("Updated Beacon IE");
  } else {
      // Provide more context if disable failed previously
      if (err_disable != ESP_OK) {
          Serial.printf("Failed to update Beacon IE (Enable step failed: %s after Disable step failed: %s)\n", esp_err_to_name(err_enable), esp_err_to_name(err_disable));
      } else {
          Serial.printf("Failed to update Beacon IE (Enable step failed: %s)\n", esp_err_to_name(err_enable));
      }
  }
}

// --- setup() and loop() remain the same ---
// Remember setup() calls update_vendor_ie() once, and loop() calls it repeatedly.

void setup() {
  Serial.begin(115200);
  delay (2000);//usb insertion delay
  Serial.println("\nESP32 Custom Beacon Sender (using esp_wifi_set_vendor_ie - Disable/Enable)");

  WiFi.mode(WIFI_AP_STA);
  const char *sta_ssid     = "apples";
  const char *sta_password = "Py16Ghf7";
  const int sta_channel = 3;

  const char *ap_ssid     = "ESP32_CustomBeacon";
  const char *ap_password = "password123";
  const int ap_channel = 6;
  bool result = WiFi.softAP(ap_ssid, ap_password, ap_channel);

  if (result) {
      Serial.printf("AP Started. SSID: %s on channel %d\n",ap_ssid,ap_channel);
      Serial.printf("AP IP address: %s\n",WiFi.softAPIP().toString().c_str());
      result = WiFi.begin(sta_ssid, sta_password,sta_channel);

      if (result) {
        Serial.printf("Connecting to STA %s on channel %d:",sta_ssid, sta_channel);
        while (WiFi.status() != WL_CONNECTED) {
            Serial.printf(".");
            delay(100);
        } 
        Serial.printf("\nSTA IP address: %s\n",WiFi.localIP().toString().c_str());
        Serial.printf("STA Gateway IP address: %s\n",WiFi.gatewayIP().toString().c_str());
        resync();

        beaconData.counter = getTickBeacon();

        struct timeval now;
        gettimeofday(&now, nullptr);
        beaconData.epochDelta = ((((uint64_t) now.tv_sec) * 1000000) + now.tv_usec) -  esp_timer_get_time();
        beaconData.millis = millis();
        update_vendor_ie(&beaconData, sizeof beaconData); // First call
        beaconData.counter++;  
      } else {
          Serial.println("Failed to start STA mode!");
      }

    } else {
      Serial.println("Failed to start AP!");
      while(1);
  }
}

void loop() {
  delay(5000);
  beaconData.millis = millis();
  update_vendor_ie(&beaconData, sizeof beaconData); // Subsequent calls
  beaconData.counter++;
}

