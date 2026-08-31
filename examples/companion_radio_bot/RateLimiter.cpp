#include "RateLimiter.h"
#include <string.h>

void RateLimiter::begin() {
  _global.configure(BOT_RL_GLOBAL_PER_MIN, BOT_RL_GLOBAL_BURST);
  for (auto& b : _channel) b.configure(BOT_RL_CHANNEL_PER_MIN, BOT_RL_CHANNEL_BURST);
  for (auto& s : _senders) {
    s.used = false;
    s.last_seen = 0;
    memset(s.key, 0, sizeof(s.key));
    s.bucket.configure(BOT_RL_SENDER_PER_MIN, BOT_RL_SENDER_BURST);
  }
}

void RateLimiter::keyFromName(const char* name, uint8_t out[4]) {
  // FNV-1a over the name, folded to 4 bytes.
  uint32_t h = 2166136261u;
  for (const char* p = name; p && *p; p++) {
    h ^= (uint8_t)*p;
    h *= 16777619u;
  }
  out[0] = h & 0xFF;
  out[1] = (h >> 8) & 0xFF;
  out[2] = (h >> 16) & 0xFF;
  out[3] = (h >> 24) & 0xFF;
}

TokenBucket* RateLimiter::senderBucket(const uint8_t key[4], unsigned long now) {
  int free_slot = -1, lru_slot = 0;
  unsigned long lru_age = 0;
  for (int i = 0; i < BOT_RL_SENDER_SLOTS; i++) {
    if (_senders[i].used) {
      if (memcmp(_senders[i].key, key, 4) == 0) {
        _senders[i].last_seen = now;
        return &_senders[i].bucket;
      }
      unsigned long age = now - _senders[i].last_seen;
      if (age >= lru_age) { lru_age = age; lru_slot = i; }
    } else if (free_slot < 0) {
      free_slot = i;
    }
  }
  int slot = (free_slot >= 0) ? free_slot : lru_slot;
  _senders[slot].used = true;
  memcpy(_senders[slot].key, key, 4);
  _senders[slot].last_seen = now;
  _senders[slot].bucket.configure(BOT_RL_SENDER_PER_MIN, BOT_RL_SENDER_BURST);
  return &_senders[slot].bucket;
}

bool RateLimiter::allow(const uint8_t key[4], uint8_t channel_idx, unsigned long now) {
  TokenBucket* chan = nullptr;
  if (channel_idx != 0xFF && channel_idx < BOT_RL_CHANNEL_SLOTS) {
    chan = &_channel[channel_idx];
  }
  TokenBucket* sender = senderBucket(key, now);

  if (!_global.has(now)) return false;
  if (chan && !chan->has(now)) return false;
  if (!sender->has(now)) return false;

  _global.take();
  if (chan) chan->take();
  sender->take();
  return true;
}
