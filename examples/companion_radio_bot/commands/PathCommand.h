#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../PathDistance.h"
#include <ctype.h>
#include <string.h>

// Decode the routing path of the request (or a hex path given as an argument)
// to repeater names, and sum sender -> hops -> bot as kilometres travelled.
//
// Output (one line per hop, matching the Python meshcore-bot's layout):
//   <xx>: <name>            resolved, unique prefix
//   <xx>: <name> <mark>     resolved from a prefix collision (🎯 / 📍 / ❓ by score)
//   <xx>?                   no node in the cache for that prefix
//   📏 ~<km> KM · SNR <n> dB  footer; km part omitted when it can't be computed,
//                            SNR part omitted for a manual `path <hex>` decode
// Direct (0-hop) replies collapse to "📡 Direct connection · SNR <n> dB".
class PathCommand : public Command {
public:
  const char* keyword() const override { return "path"; }
  const char* help() const override { return "path [hex] - repeaters in the route + km travelled"; }

  bool matches(const BotMsg& msg) const override {
    return matchesKeyword(msg, "path") || matchesKeyword(msg, "p") ||
           matchesKeyword(msg, "decode") || matchesKeyword(msg, "route");
  }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    uint8_t buf[BOT_PATH_MAX_HOPS];
    uint8_t hops = 0, hash_len = 1;
    const uint8_t* path = nullptr;

    bool manual = false;   // path given as a hex argument => no live-packet SNR
    char arg[64];
    if (args(msg, arg, sizeof(arg)) > 0) {
      manual = true;
      hops = parseHex(arg, buf, sizeof(buf));
      if (hops == 0) { mesh.botReply(msg, "no valid hex path"); return; }
      path = buf;
    } else if (msg.is_flood && msg.path_hops > 0) {
      path = msg.path;
      hops = msg.path_hops;
      hash_len = msg.path_hash_len;
    } else {
      char d[64];
      snprintf(d, sizeof(d),
               "\xF0\x9F\x93\xA1 Direct connection \xC2\xB7 SNR %.2f dB", msg.snr);
      mesh.botReply(msg, d);
      return;
    }

    double slat, slon, mlat, mlon;
    bool have_s = mesh.senderLocation(msg, slat, slon);
    bool have_m = mesh.selfLocation(mlat, mlon);

    PathInfo pi;
    resolvePath(path, hash_len, hops, mesh.nowEpoch(),
                slat, slon, have_s, mlat, mlon, have_m, mesh.store(), pi);

    char out[BOT_MAX_REPLY_LEN + 1];
    size_t k = 0;
    for (int i = 0; i < pi.hops && k < sizeof(out) - 48; i++) {
      if (i) k += snprintf(out + k, sizeof(out) - k, "\n");
      const PathHop& h = pi.hop[i];
      const NodeRec* r = h.rep;
      if (r && r->name[0]) {
        char nm[32];
        stripRepeater(r->name, nm, sizeof(nm));
        const char* sym = "";
        if (h.guessed)
          sym = h.score >= 0.75f ? " \xF0\x9F\x8E\xAF"    // 🎯
              : h.score >= 0.50f ? " \xF0\x9F\x93\x8D"    // 📍
                                 : " \xE2\x9D\x93";        // ❓
        k += snprintf(out + k, sizeof(out) - k, "%02x: %s%s", h.hash[0], nm, sym);
      } else {
        k += snprintf(out + k, sizeof(out) - k, "%02x?", h.hash[0]);
      }
    }

    // Footer: "📏 ~<km> KM · SNR <snr> dB". The km part is dropped when the
    // distance can't be computed; the SNR part is dropped for a manual decode.
    char foot[48];
    size_t fk = 0;
    if (pi.have_distance)
      fk += snprintf(foot + fk, sizeof(foot) - fk,
                     "\xF0\x9F\x93\x8F ~%.0f KM", pi.total_km);
    if (!manual) {
      if (fk) fk += snprintf(foot + fk, sizeof(foot) - fk, " \xC2\xB7 ");
      fk += snprintf(foot + fk, sizeof(foot) - fk, "SNR %.2f dB", msg.snr);
    }
    if (fk && k < sizeof(out) - 2)
      k += snprintf(out + k, sizeof(out) - k, "\n%s", foot);

    mesh.botReply(msg, out);

    if (pi.have_distance && pi.confidence != 'L') {
      mesh.store().recordDistance(msg.sender_name ? msg.sender_name : "?",
                                  pi.total_km, mesh.nowEpoch());
    }
  }

private:
  // Copy `in` to `out`, dropping the standalone word "repeater" (any case) and
  // tidying the separators it leaves behind. Falls back to `in` if the strip
  // would leave nothing.
  static void stripRepeater(const char* in, char* out, size_t n) {
    char tmp[32];
    size_t t = 0;
    for (const char* p = in; *p && t < sizeof(tmp) - 1; ) {
      bool at_boundary = (t == 0) || !isalnum((unsigned char)tmp[t - 1]);
      if (at_boundary) {
        static const char w[] = "repeater";
        int j = 0;
        while (j < 8 && tolower((unsigned char)p[j]) == w[j]) j++;
        if (j == 8 && !isalnum((unsigned char)p[8])) { p += 8; continue; }
      }
      tmp[t++] = *p++;
    }
    tmp[t] = 0;

    size_t o = 0;
    bool prev_sep = true;   // swallow leading separators
    for (size_t i = 0; i < t && o < n - 1; i++) {
      char c = tmp[i];
      if (c == ' ' || c == '_' || c == '-') {
        if (!prev_sep) { out[o++] = ' '; prev_sep = true; }
      } else {
        out[o++] = c;
        prev_sep = false;
      }
    }
    while (o > 0 && out[o - 1] == ' ') o--;
    out[o] = 0;

    if (o == 0) { strncpy(out, in, n - 1); out[n - 1] = 0; }
  }

  static uint8_t parseHex(const char* s, uint8_t* out, uint8_t max) {
    uint8_t n = 0;
    int hi = -1;
    for (const char* p = s; *p && n < max; p++) {
      char c = *p;
      int v;
      if (c >= '0' && c <= '9') v = c - '0';
      else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
      else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
      else { hi = -1; continue; }        // separator resets the nibble pairing
      if (hi < 0) hi = v;
      else { out[n++] = (uint8_t)((hi << 4) | v); hi = -1; }
    }
    return n;
  }
};
