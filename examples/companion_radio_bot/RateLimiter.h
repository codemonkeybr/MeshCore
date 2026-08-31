#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "BotConfig.h"

// Simple token bucket. Refills lazily on access.
struct TokenBucket {
  float tokens = 0.0f;
  float capacity = 1.0f;
  float refill_per_sec = 1.0f;
  unsigned long last_ms = 0;
  bool primed = false;

  void configure(float per_min, float burst) {
    capacity = burst;
    refill_per_sec = per_min / 60.0f;
    tokens = burst;
    primed = false;
  }
  void refill(unsigned long now) {
    if (!primed) { last_ms = now; primed = true; return; }
    float dt = (now - last_ms) / 1000.0f;
    last_ms = now;
    tokens += dt * refill_per_sec;
    if (tokens > capacity) tokens = capacity;
  }
  bool has(unsigned long now) { refill(now); return tokens >= 1.0f; }
  void take() { tokens -= 1.0f; }
};

// Global / per-channel / per-sender rate limiting for bot replies. A reply is
// permitted only when all three buckets currently hold a token; on success one
// token is consumed from each.
class RateLimiter {
public:
  void begin();

  // key: 4 bytes identifying the sender (pubkey prefix for DMs, name hash for
  // channel messages). channel_idx: 0xFF for a DM.
  bool allow(const uint8_t key[4], uint8_t channel_idx, unsigned long now);

  // Hash a sender name to a 4-byte key for channel-message rate limiting.
  static void keyFromName(const char* name, uint8_t out[4]);

private:
  struct SenderSlot {
    uint8_t key[4];
    TokenBucket bucket;
    unsigned long last_seen;
    bool used;
  };

  TokenBucket _global;
  TokenBucket _channel[BOT_RL_CHANNEL_SLOTS];
  SenderSlot _senders[BOT_RL_SENDER_SLOTS];

  TokenBucket* senderBucket(const uint8_t key[4], unsigned long now);
};
