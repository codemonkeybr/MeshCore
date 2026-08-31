#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../PathDistance.h"
#include <ctype.h>
#include <string.h>

// Decode the routing path of the request (or a hex path given as an argument)
// to repeater names, and sum sender -> hops -> bot as kilometres travelled.
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

    char arg[64];
    if (args(msg, arg, sizeof(arg)) > 0) {
      hops = parseHex(arg, buf, sizeof(buf));
      if (hops == 0) { mesh.botReply(msg, "no valid hex path"); return; }
      path = buf;
    } else if (msg.is_flood && msg.path_hops > 0) {
      path = msg.path;
      hops = msg.path_hops;
      hash_len = msg.path_hash_len;
    } else if (msg.is_flood) {
      mesh.botReply(msg, "reached me flood, 0 hops");
      return;
    } else {
      mesh.botReply(msg, "reached me direct (0 hops)");
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
    for (int i = 0; i < pi.hops && k < sizeof(out) - 24; i++) {
      if (i) k += snprintf(out + k, sizeof(out) - k, " > ");
      const NodeRec* r = pi.hop[i].rep;
      if (r && r->name[0]) {
        k += snprintf(out + k, sizeof(out) - k, "%s%s", r->name, pi.hop[i].guessed ? "?" : "");
      } else {
        k += snprintf(out + k, sizeof(out) - k, "%02x?", pi.hop[i].hash[0]);
      }
    }
    const char* mark = pi.confidence == 'H' ? " \xF0\x9F\x8E\xAF"
                     : pi.confidence == 'M' ? " \xF0\x9F\x93\x8D"
                                            : " \xE2\x9D\x93";
    if (pi.have_distance)
      k += snprintf(out + k, sizeof(out) - k, " (%d hop%s, ~%.0f km)%s",
                    pi.hops, pi.hops == 1 ? "" : "s", pi.total_km, mark);
    else
      k += snprintf(out + k, sizeof(out) - k, " (%d hop%s)%s",
                    pi.hops, pi.hops == 1 ? "" : "s", mark);

    mesh.botReply(msg, out);

    if (pi.have_distance && pi.confidence != 'L') {
      mesh.store().recordDistance(msg.sender_name ? msg.sender_name : "?",
                                  pi.total_km, mesh.nowEpoch());
    }
  }

private:
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
