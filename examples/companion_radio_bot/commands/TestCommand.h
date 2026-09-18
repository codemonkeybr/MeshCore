#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../BotConfig.h"
#include <stdio.h>
#include <string.h>

// Link check. Output matches the Python meshcore-bot's default `test` format:
//   ack @[<sender>]<phrase_part> | <path> | SNR: <snr> dB | RSSI: <rssi> dBm | Received at: HH:MM:SS
class TestCommand : public Command {
public:
  const char* keyword() const override { return "test"; }
  const char* help() const override { return "test (or t) [phrase] - link check: path, SNR, RSSI"; }

  bool matches(const BotMsg& msg) const override {
    return matchesKeyword(msg, "test") || matchesKeyword(msg, "t");
  }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    // path part. A ROUTE_TYPE_DIRECT packet's path is consumed hop-by-hop in
    // flight, so it's always empty on delivery - even after several repeaters
    // - which is indistinguishable here from a genuine 0-hop receipt without
    // falling back to the sender's last advert path (see PathCommand.h).
    char path[64];
    if (msg.is_flood && msg.path_hops > 0) {
      int k = snprintf(path, sizeof(path), "%u hop%s: ",
                       msg.path_hops, msg.path_hops == 1 ? "" : "s");
      for (int i = 0; i < msg.path_hops && k < (int)sizeof(path) - 4; i++) {
        k += snprintf(path + k, sizeof(path) - k, "%s%02x",
                      i ? "," : "", msg.path[i * msg.path_hash_len]);
      }
    } else {
      uint8_t cbuf[64], chops = 0, chash_len = 1;
      uint32_t age = 0;
      bool have_cache = !msg.is_flood &&
        mesh.lastKnownPath(msg, cbuf, sizeof(cbuf), chops, chash_len, age);

      if (have_cache && chops > 0) {
        int k = snprintf(path, sizeof(path), "~%u hop%s (cached): ",
                         chops, chops == 1 ? "" : "s");
        for (int i = 0; i < chops && k < (int)sizeof(path) - 4; i++) {
          k += snprintf(path + k, sizeof(path) - k, "%s%02x",
                        i ? "," : "", cbuf[i * chash_len]);
        }
      } else if (msg.is_flood || (have_cache && chops == 0)) {
        strcpy(path, "Direct");
      } else {
        strcpy(path, "Routed (hops unknown)");
      }
    }

    // phrase
    char phrase[40];
    args(msg, phrase, sizeof(phrase));

    // local timestamp HH:MM:SS
    char ts[9] = "--:--:--";
    uint32_t e = mesh.nowEpoch();
    if (e >= 1000000000UL) {
      long local = (long)e + (long)BOT_TZ_OFFSET_MIN * 60;
      long tod = ((local % 86400) + 86400) % 86400;
      snprintf(ts, sizeof(ts), "%02ld:%02ld:%02ld", tod / 3600, (tod % 3600) / 60, tod % 60);
    }

    char out[BOT_MAX_REPLY_LEN + 1];
    snprintf(out, sizeof(out),
             "ack @[%s]%s%s | %s | SNR: %.2f dB | RSSI: %.0f dBm | Received at: %s",
             msg.sender_name ? msg.sender_name : "?",
             phrase[0] ? ": " : "", phrase,
             path, msg.snr, msg.rssi, ts);
    mesh.botReply(msg, out);
  }
};
