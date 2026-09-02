#pragma once

#include <stdint.h>
#include "BotStore.h"

#ifndef BOT_PATH_MAX_HOPS
#define BOT_PATH_MAX_HOPS 16
#endif

struct PathHop {
  uint8_t hash[6];
  uint8_t hash_len = 1;
  const NodeRec* rep = nullptr;  // resolved node, or null
  bool guessed = false;          // chosen among colliding candidates
  int  candidates = 0;
  float score = 0.0f;            // winner's selection score (0..1), for the per-hop mark
};

struct PathInfo {
  int hops = 0;
  bool truncated = false;
  PathHop hop[BOT_PATH_MAX_HOPS];

  bool have_distance = false;
  float total_km = 0.0f;
  int missing_locs = 0;          // hops with no known location
  char confidence = 'L';         // 'H' / 'M' / 'L'
};

// Resolve each hop hash to a node and, when enough waypoints have coordinates,
// sum sender -> hops -> self as a great-circle distance.
void resolvePath(const uint8_t* path, uint8_t hash_len, uint8_t hops,
                 uint32_t now_epoch,
                 double sender_lat, double sender_lon, bool have_sender,
                 double self_lat, double self_lon, bool have_self,
                 const BotStore& store, PathInfo& out);
