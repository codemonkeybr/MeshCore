#include "PathDistance.h"
#include "GeoUtil.h"
#include "BotConfig.h"
#include <string.h>
#include <math.h>

static const NodeRec* pickCandidate(const NodeRec* const* cands, int n,
                                    uint32_t now_epoch,
                                    bool have_prev, double prev_lat, double prev_lon,
                                    bool* out_guessed) {
  *out_guessed = (n > 1);
  const NodeRec* best = nullptr;
  float best_score = -1e9f;

  for (int i = 0; i < n; i++) {
    const NodeRec* c = cands[i];
    // recency: 1.0 now -> 0.0 at ~14 days old
    float age_days = now_epoch > c->last_advert ? (now_epoch - c->last_advert) / 86400.0f : 0.0f;
    float recency = 1.0f - (age_days / 14.0f);
    if (recency < 0.0f) recency = 0.0f;

    float geo = 0.3f;   // neutral when we can't check
    if (have_prev && c->hasLoc()) {
      float d = haversine_km(prev_lat, prev_lon, e6_to_deg(c->lat_e6), e6_to_deg(c->lon_e6));
      if (d > BOT_PATH_MAX_HOP_KM) geo = -1.0f;              // implausible hop
      else geo = 1.0f - (d / (float)BOT_PATH_MAX_HOP_KM);
    }

    float score = 0.45f * recency + 0.55f * geo;
    if (score > best_score) { best_score = score; best = c; }
  }
  return best;
}

void resolvePath(const uint8_t* path, uint8_t hash_len, uint8_t hops,
                 uint32_t now_epoch,
                 double sender_lat, double sender_lon, bool have_sender,
                 double self_lat, double self_lon, bool have_self,
                 const BotStore& store, PathInfo& out) {
  if (hash_len < 1) hash_len = 1;
  if (hash_len > 6) hash_len = 6;
  out.hops = hops;
  if (hops > BOT_PATH_MAX_HOPS) { out.hops = BOT_PATH_MAX_HOPS; out.truncated = true; }

  bool have_prev = have_sender;
  double prev_lat = sender_lat, prev_lon = sender_lon;

  // waypoints for the distance sum
  double wp_lat[BOT_PATH_MAX_HOPS + 2], wp_lon[BOT_PATH_MAX_HOPS + 2];
  int wp = 0;
  if (have_sender) { wp_lat[wp] = sender_lat; wp_lon[wp] = sender_lon; wp++; }

  int guesses = 0;
  for (int i = 0; i < out.hops; i++) {
    PathHop& h = out.hop[i];
    h.hash_len = hash_len;
    memcpy(h.hash, &path[i * hash_len], hash_len);

    const NodeRec* cands[8];
    int n = store.findByHash(h.hash, hash_len, cands, 8);
    h.candidates = n;

    if (n == 0) {
      h.rep = nullptr;
      out.missing_locs++;
      continue;
    }
    bool guessed = false;
    h.rep = pickCandidate(cands, n, now_epoch, have_prev, prev_lat, prev_lon, &guessed);
    h.guessed = guessed;
    if (guessed) guesses++;

    if (h.rep && h.rep->hasLoc()) {
      double la = e6_to_deg(h.rep->lat_e6), lo = e6_to_deg(h.rep->lon_e6);
      wp_lat[wp] = la; wp_lon[wp] = lo; wp++;
      prev_lat = la; prev_lon = lo; have_prev = true;
    } else {
      out.missing_locs++;
    }
  }

  if (have_self) { wp_lat[wp] = self_lat; wp_lon[wp] = self_lon; wp++; }

  if (BOT_PATH_SHOW_DISTANCE && wp >= 2 && out.missing_locs == 0) {
    float total = 0.0f;
    for (int i = 0; i + 1 < wp; i++)
      total += haversine_km(wp_lat[i], wp_lon[i], wp_lat[i + 1], wp_lon[i + 1]);
    out.total_km = total;
    out.have_distance = true;
  }

  if (out.missing_locs == 0 && guesses == 0) out.confidence = 'H';
  else if (out.missing_locs <= 1 && guesses <= 1) out.confidence = 'M';
  else out.confidence = 'L';
}
