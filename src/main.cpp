// Copyright 2023 Blues Inc.  All rights reserved.
//
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.
//
// This example shows how a developer can efficiently send large binary payloads
// using COBS with the Notecard.

#include <Arduino.h>
#include <stdlib.h>
#include <Notecard.h>
#include "helper.h"

// Note that both of these definitions are optional; just prefix either line
// with `//` to remove it.
// - Remove txRxPinsSerial if you wired your Notecard using I2C SDA/SCL pins
//   instead of serial RX/TX
// - Remove usbSerial if you don't want the Notecard library to output debug
//   information

HardwareSerial stlinkSerial(PIN_VCP_RX, PIN_VCP_TX);
// #define txRxPinsSerial Serial1
#define usbSerial Serial

#include "example_img.h"
const uint8_t *img_map = example_img_map;
const size_t img_len = example_img_len;

#define CHUNK_SIZE 8192
uint8_t temp_buffer[CHUNK_SIZE + 128];

// This is the unique Notehub Product Identifier for your device
#define PRODUCT_UID "com.blues.rlauer:aaa"

#ifndef PRODUCT_UID
#define PRODUCT_UID "" // "com.my-company.my-name:my-project"
#pragma message "PRODUCT_UID is not defined in this example. Please ensure your Notecard has a product identifier set before running this example or define it in code here. More details at https://dev.blues.io/tools-and-sdks/samples/product-uid"
#endif

Notecard notecard;

void setup()
{
  // Set up for debug output (if available).
#ifdef usbSerial
  // If you open Arduino's serial terminal window, you'll be able to watch
  // JSON objects being transferred to and from the Notecard for each request.
  stlinkSerial.begin(115200);
  const size_t usb_timeout_ms = 3000;
  for (const size_t start_ms = millis(); !stlinkSerial && (millis() - start_ms) < usb_timeout_ms;)
    ;
  notecard.setDebugOutputStream(stlinkSerial);
#endif

  // Initialize the physical I/O channel to the Notecard
#ifdef txRxPinsSerial
  notecard.begin(txRxPinsSerial, 9600);
#else
  notecard.begin();
#endif

  // "notecard.newRequest()" uses the bundled "J" json package to allocate a
  //"req", which is a JSON object for the request to which we will then add
  // Request arguments.  The function allocates a "req" request structure
  // using malloc() and initializes its "req" field with the type of request.
  J *req = notecard.newRequest("hub.set");

  // This command (required) causes the data to be delivered to the Project on
  // notehub.io that has claimed this Product ID (see above).
  JAddStringToObject(req, "product", PRODUCT_UID);

  // This command determines how often the Notecard connects to the service.
  // "continous" mode is required for binary file transfers
  JAddStringToObject(req, "mode", "continuous");
  notecard.sendRequestWithRetry(req, 5); // 5 seconds

  // Reset the Notecard's binary storage area, so that we can start fresh
  NoteBinaryReset();

  // Send the binary image to Notecard
  binary_setup(CHUNK_SIZE, sizeof(temp_buffer));
  notecard.logDebugf("Sending image of size %u, in chunks of size: %u\n", img_len, CHUNK_SIZE);

  // send the binary image in fragments of size CHUNK_SIZE
  int i = 0;
  size_t bytes_left = img_len;
  while (bytes_left)
  {
    notecard.logDebugf("\nSending chunk %d, offset: %d...\n", i, i * CHUNK_SIZE);

    size_t bytes_to_send = bytes_left >= CHUNK_SIZE ? CHUNK_SIZE : bytes_left;
    memcpy(temp_buffer, img_map + i * CHUNK_SIZE, bytes_to_send);

    if (!binary_send_to_notecard(temp_buffer, bytes_to_send))
    {
      bytes_left -= bytes_to_send;
      i++;
    }
  }

  notecard.logDebug("File transfered to Notecard, now web.post to Notehub\n");
  binary_send_to_notehub();

  notecard.logDebug("Done!\n");
}

void loop()
{
}
