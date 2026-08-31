#pragma once

#include <Arduino.h>
#include <Mesh.h>
#include <helpers/ContactInfo.h>

// A normalised view of an inbound mesh message handed to the command router.
// Populated by BotMesh from the onChannelMessageRecv / onMessageRecv hooks.
struct BotMsg {
  bool is_channel = false;               // true = group channel, false = direct message
  const char* text = nullptr;            // message body (NUL-terminated, owned by caller)
  uint32_t sender_timestamp = 0;         // by the sender's clock

  // Direct-message context (is_channel == false)
  const ContactInfo* contact = nullptr;  // resolved sender contact, or null

  // Channel context (is_channel == true)
  const mesh::GroupChannel* channel = nullptr;
  uint8_t channel_idx = 0xFF;

  // Routing: path_len == 0xFF means the packet arrived direct (not flood).
  // path points at up to path_len 1-byte hop hashes (valid only for flood).
  uint8_t path_len = 0xFF;
  const uint8_t* path = nullptr;

  float snr = 0.0f;
};
