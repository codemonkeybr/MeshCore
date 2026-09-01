#pragma once

#include <Arduino.h>
#include <Mesh.h>
#include <helpers/ContactInfo.h>

// A normalised view of an inbound mesh message handed to the command router.
// Populated by BotMesh from the onChannelMessageRecv / onMessageRecv hooks.
//
// For channel messages the on-wire text is "<sender>: <body>"; BotMesh splits
// it so `text` is the body alone and `sender_name` is the "<sender>" part.
struct BotMsg {
  bool is_channel = false;               // true = group channel, false = direct message
  const char* text = nullptr;            // message body (sender prefix stripped for channels)
  const char* sender_name = nullptr;     // display name of the sender
  uint32_t sender_timestamp = 0;         // by the sender's clock

  // Direct-message context (is_channel == false)
  const ContactInfo* contact = nullptr;  // resolved sender contact, or null

  // Channel context (is_channel == true)
  const mesh::GroupChannel* channel = nullptr;
  uint8_t channel_idx = 0xFF;

  // Routing. `is_flood` false => heard direct (0 hops). When true, `path` points
  // at path_hops * path_hash_len bytes of hop hashes (hash = pubkey prefix).
  bool is_flood = false;
  uint8_t path_hash_len = 1;
  uint8_t path_hops = 0;
  const uint8_t* path = nullptr;

  float snr = 0.0f;
  float rssi = 0.0f;   // dBm, captured from the last raw RX (logRxRaw)
};
