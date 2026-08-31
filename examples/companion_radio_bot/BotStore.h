#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "BotConfig.h"

// One cached node observation, kept for the `path` command's hop resolution and
// for `prefix`. Populated from every advert (via BotMesh::onDiscoveredContact),
// independent of the 350-entry companion contact table and its eviction.
struct NodeRec {
  uint8_t  pubkey[6];       // first 6 bytes of the public key (hop hash = pubkey[0])
  char     name[17];
  int32_t  lat_e6, lon_e6;  // 0,0 = unknown
  uint32_t last_advert;     // sender's clock
  uint8_t  type;            // ADV_TYPE_*
  uint8_t  used;

  bool hasLoc() const { return lat_e6 != 0 || lon_e6 != 0; }
  bool isRepeaterish() const { return type == 2 /*REPEATER*/ || type == 3 /*ROOM*/; }
};

class BotStore {
public:
  void begin();
  void loop(uint32_t now_epoch);   // periodic flush
  void flush();                    // persist the cache to SPIFFS now

  // Record / update a node from an advert.
  void noteNode(const uint8_t pubkey[], const char* name, uint8_t type,
                int32_t lat_e6, int32_t lon_e6, bool has_loc, uint32_t advert_ts);

  // Hop resolution: fill out[] with up to max_out records whose pubkey matches
  // the first hash_len bytes of `hash`. Returns the count.
  int findByHash(const uint8_t* hash, uint8_t hash_len,
                 const NodeRec** out, int max_out) const;

  const NodeRec* findByName(const char* name) const;
  int count() const { return _count; }

  // --- leaderboard (NVS-backed) ---
  void recordDistance(const char* who, float km, uint32_t now_epoch);
  float weekBest(char* who_out, size_t who_sz) const;
  float allTimeBest(char* who_out, size_t who_sz) const;

private:
  NodeRec* slotFor(const uint8_t pubkey[6]);   // find existing or allocate (LRU by last_advert)
  bool _dirty = false;
  unsigned long _next_flush_ms = 0;
  int _count = 0;
  NodeRec _recs[BOT_LOCCACHE_MAX];
};
