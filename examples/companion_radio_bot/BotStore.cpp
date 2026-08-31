#include "BotStore.h"
#include "BotDebug.h"
#include <SPIFFS.h>
#include <Preferences.h>
#include <string.h>

#define CACHE_PATH "/botcache.bin"
#define CACHE_MAGIC 0x42430002u    // "BC" v2

void BotStore::begin() {
  _count = 0;
  memset(_recs, 0, sizeof(_recs));

  File f = SPIFFS.open(CACHE_PATH, "r");
  if (f) {
    uint32_t magic = 0;
    uint16_t n = 0;
    if (f.read((uint8_t*)&magic, 4) == 4 && magic == CACHE_MAGIC &&
        f.read((uint8_t*)&n, 2) == 2) {
      if (n > BOT_LOCCACHE_MAX) n = BOT_LOCCACHE_MAX;
      int got = f.read((uint8_t*)_recs, (size_t)n * sizeof(NodeRec));
      if (got == (int)((size_t)n * sizeof(NodeRec))) _count = n;
    }
    f.close();
  }
  BOT_DEBUG_PRINTLN("store: loaded %d cached nodes", _count);
  _next_flush_ms = millis() + (uint32_t)BOT_LOCCACHE_FLUSH_SEC * 1000;
}

void BotStore::flush() {
  if (!_dirty) return;
  File f = SPIFFS.open(CACHE_PATH, "w");
  if (!f) { BOT_DEBUG_PRINTLN("store: flush open failed"); return; }
  uint32_t magic = CACHE_MAGIC;
  uint16_t n = (uint16_t)_count;
  f.write((const uint8_t*)&magic, 4);
  f.write((const uint8_t*)&n, 2);
  f.write((const uint8_t*)_recs, (size_t)n * sizeof(NodeRec));
  f.close();
  _dirty = false;
  BOT_DEBUG_PRINTLN("store: flushed %d nodes", _count);
}

void BotStore::loop(uint32_t /*now_epoch*/) {
  if (_dirty && (long)(millis() - _next_flush_ms) >= 0) {
    flush();
    _next_flush_ms = millis() + (uint32_t)BOT_LOCCACHE_FLUSH_SEC * 1000;
  }
}

NodeRec* BotStore::slotFor(const uint8_t pubkey[6]) {
  int lru = -1;
  uint32_t lru_ts = 0xFFFFFFFFu;
  for (int i = 0; i < _count; i++) {
    if (_recs[i].used && memcmp(_recs[i].pubkey, pubkey, 6) == 0) return &_recs[i];
    if (_recs[i].used && _recs[i].last_advert < lru_ts) { lru_ts = _recs[i].last_advert; lru = i; }
  }
  if (_count < BOT_LOCCACHE_MAX) return &_recs[_count++];
  return (lru >= 0) ? &_recs[lru] : &_recs[0];   // evict oldest
}

void BotStore::noteNode(const uint8_t pubkey[], const char* name, uint8_t type,
                        int32_t lat_e6, int32_t lon_e6, bool has_loc, uint32_t advert_ts) {
  NodeRec* r = slotFor(pubkey);
  memcpy(r->pubkey, pubkey, 6);
  strncpy(r->name, name ? name : "", sizeof(r->name) - 1);
  r->name[sizeof(r->name) - 1] = '\0';
  r->type = type;
  if (has_loc && (lat_e6 != 0 || lon_e6 != 0)) { r->lat_e6 = lat_e6; r->lon_e6 = lon_e6; }
  if (advert_ts > r->last_advert) r->last_advert = advert_ts;
  r->used = 1;
  _dirty = true;
}

int BotStore::findByHash(const uint8_t* hash, uint8_t hash_len,
                         const NodeRec** out, int max_out) const {
  if (hash_len < 1) hash_len = 1;
  if (hash_len > 6) hash_len = 6;
  int n = 0;
  for (int i = 0; i < _count && n < max_out; i++) {
    if (_recs[i].used && memcmp(_recs[i].pubkey, hash, hash_len) == 0) out[n++] = &_recs[i];
  }
  return n;
}

const NodeRec* BotStore::findByName(const char* name) const {
  if (!name || !*name) return nullptr;
  const NodeRec* best = nullptr;
  for (int i = 0; i < _count; i++) {
    if (_recs[i].used && strcmp(_recs[i].name, name) == 0) {
      if (!best || _recs[i].last_advert > best->last_advert) best = &_recs[i];
    }
  }
  return best;
}

// ---- leaderboard ---------------------------------------------------------

static uint32_t week_of(uint32_t epoch) { return epoch / 604800u; }

void BotStore::recordDistance(const char* who, float km, uint32_t now_epoch) {
  Preferences p;
  if (!p.begin("botlb", false)) return;

  float at_km = p.getFloat("at_km", 0.0f);
  if (km > at_km) {
    p.putFloat("at_km", km);
    p.putString("at_who", who ? who : "?");
  }

  uint32_t wk = week_of(now_epoch);
  uint32_t stored_wk = p.getUInt("wk", 0);
  float wk_km = p.getFloat("wk_km", 0.0f);
  if (wk != stored_wk) { wk_km = 0.0f; p.putUInt("wk", wk); }
  if (km > wk_km) {
    p.putFloat("wk_km", km);
    p.putString("wk_who", who ? who : "?");
  }
  p.end();
}

float BotStore::weekBest(char* who_out, size_t who_sz) const {
  Preferences p;
  if (!p.begin("botlb", true)) { if (who_out && who_sz) who_out[0] = 0; return 0.0f; }
  float km = p.getFloat("wk_km", 0.0f);
  if (who_out && who_sz) { String s = p.getString("wk_who", "?"); strncpy(who_out, s.c_str(), who_sz - 1); who_out[who_sz - 1] = 0; }
  p.end();
  return km;
}

float BotStore::allTimeBest(char* who_out, size_t who_sz) const {
  Preferences p;
  if (!p.begin("botlb", true)) { if (who_out && who_sz) who_out[0] = 0; return 0.0f; }
  float km = p.getFloat("at_km", 0.0f);
  if (who_out && who_sz) { String s = p.getString("at_who", "?"); strncpy(who_out, s.c_str(), who_sz - 1); who_out[who_sz - 1] = 0; }
  p.end();
  return km;
}
