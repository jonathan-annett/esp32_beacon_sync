#pragma once
#include "Arduino.h"

uint8_t oui_custom[3] = {0x00, 0x11, 0x22}; // Example OUI


// WiFi Management Frame Header structure (simplified)
typedef struct {
    uint16_t frame_ctrl;
    uint16_t duration_id;
    uint8_t addr1[6]; /* receiver address */
    uint8_t addr2[6]; /* sender address */
    uint8_t addr3[6]; /* BSSID */
    uint16_t seq_ctrl;
} wifi_mgmt_header_t;

// Beacon Frame Body structure (fixed part)
typedef struct {
    uint64_t timestamp;
    uint16_t beacon_interval;
    uint16_t capability_info;
} beacon_frame_fixed_part_t;
// --- End Globals ---


typedef struct {
    wifi_mgmt_header_t         hdr1;
    beacon_frame_fixed_part_t  hdr2;
} beacon_frame_wrapper_t;

typedef int (*SniffedDataPacketCallback)(void *, size_t , const beacon_frame_fixed_part_t* );


typedef struct {
    uint8_t  oui[3];
    uint8_t  dataLen;
    uint16_t crc16;
} __attribute__((packed)) beacon_data_header_t;

typedef struct {
    beacon_data_header_t header;
    uint8_t data [256];
} __attribute__((packed)) base_beacon_data_t;

/**
 * @brief Calculates the CRC-16-MODBUS checksum for a given data block.
 *
 * This function implements the CRC-16 standard commonly known as MODBUS,
 * which uses the polynomial 0x8005 (represented as 0xA001 in the reversed,
 * reflected algorithm).
 *
 * Parameters:
 * - Polynomial: 0x8005 (Reversed: 0xA001)
 * - Initial Value: 0xFFFF
 * - Input Bytes Reflected: Yes (handled by the algorithm style)
 * - Output CRC Reflected: Yes (handled by the algorithm style)
 * - Final XOR: 0x0000 (None)
 *
 * @param data Pointer to the start of the data buffer.
 * @param length The number of bytes in the data buffer to process.
 * @return The calculated 16-bit CRC checksum.
 */
uint16_t crc16_modbus(const uint8_t *data, size_t length) {
    // Check for invalid input
    if (data == NULL && length > 0) {
        // Or handle error appropriately (e.g., return 0, assert)
        return 0x0000;
    }

    // Initial value required by CRC-16-MODBUS standard
    uint16_t crc = 0xFFFF;

    // Iterate through each byte of the data buffer
    for (size_t i = 0; i < length; ++i) {
        // XOR the current data byte into the lower byte of the CRC
        crc ^= (uint16_t)data[i];

        // Process all 8 bits of the current byte
        for (int j = 0; j < 8; ++j) {
            // Check if the LSB (least significant bit) of crc is 1
            if (crc & 0x0001) {
                // If LSB is 1, right-shift crc by 1 bit AND XOR with the polynomial
                crc = (crc >> 1) ^ 0xA001; // 0xA001 is the reversed polynomial 0x8005
            } else {
                // If LSB is 0, just right-shift crc by 1 bit
                crc >>= 1;
            }
        }
    }

    // The final CRC value (no final XOR required for MODBUS)
    return crc;
}

size_t preparePayloadPacket(base_beacon_data_t *buffer,  uint8_t *dataPayload, uint8_t dataPayloadSize) { 
    memmove(&buffer->header.oui,&oui_custom,sizeof(oui_custom)); 
    memmove(&buffer->data,dataPayload,dataPayloadSize);
    buffer->header.dataLen = dataPayloadSize;
    buffer->header.crc16 = crc16_modbus(dataPayload, dataPayloadSize);
    // returns the absolute size of the paylaod
    /* Serial.printf("header.dataLen = %d\nheader.crc16 = 0x%0.4x\ndataPayload = ",buffer->header.dataLen,buffer->header.crc16);
    for (int i = 0; i< dataPayloadSize; i ++) {
        Serial.printf(" 0x%0.2x",dataPayload[i]);
    }  
    Serial.printf("\ndataPayloadSize = %u\n",dataPayloadSize);*/
    return sizeof(buffer->header)+dataPayloadSize;
} 

bool detectPayloadHeader( const uint8_t * detectionWindow, size_t detectionWindowSize, const beacon_frame_fixed_part_t *fixedHdr, SniffedDataPacketCallback cb) {   
    // detectionWindow is adjusted by the called on each call, and extends to the end of the available underlying buffer. 
    // detectionWindowSize is as the name suggests
    if (memcmp(detectionWindow,&oui_custom,sizeof(oui_custom)) != 0) {
        // we did not find a valid oui at the start of the buffer
        return false;
    }

    base_beacon_data_t *payload = (base_beacon_data_t *) detectionWindow;

    if (  payload->header.dataLen > detectionWindowSize - sizeof(payload->header) ) {
        // implied length exceeds available data in source buffer.
        return false;
    }

    if (payload->header.dataLen == 0) {
        // there is no data, so return true if the crc matches empty data 
        return payload->header.crc16 == 0xFFFF;
    }
    /*
    Serial.printf("payload->header.dataLen = %u\n,payload->data = {",payload->header.dataLen);
    for (int i = 0; i< payload->header.dataLen; i ++) {
        Serial.printf(" 0x%0.2x",payload->data[i]);
    }  
    Serial.printf("}\n");
    */
    if ( crc16_modbus((const uint8_t*) &payload->data,payload->header.dataLen) != payload->header.crc16 )  {
        // packet data does not match crc in packet header
        return false;
    }

    if (cb) {
        cb (&payload->data,payload->header.dataLen,fixedHdr);
    }
    return true;
}