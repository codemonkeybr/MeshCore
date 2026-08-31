#pragma once

// Lightweight logging for the on-device bot, gated on -D BOT_DEBUG=1.
// Mirrors the WIFI_DEBUG_PRINTLN / BLE_DEBUG_PRINTLN pattern used elsewhere.
#if defined(BOT_DEBUG) && BOT_DEBUG && defined(ARDUINO)
  #include <Arduino.h>
  #define BOT_DEBUG_PRINT(F, ...)   Serial.printf("Bot: " F, ##__VA_ARGS__)
  #define BOT_DEBUG_PRINTLN(F, ...) Serial.printf("Bot: " F "\n", ##__VA_ARGS__)
#else
  #define BOT_DEBUG_PRINT(...)   {}
  #define BOT_DEBUG_PRINTLN(...) {}
#endif
