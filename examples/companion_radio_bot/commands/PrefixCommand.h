#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../BotStore.h"
#include <ctype.h>

// Look up cached nodes whose public key starts with the given hex prefix.
class PrefixCommand : public Command {
public:
  const char* keyword() const override { return "prefix"; }
  const char* help() const override { return "prefix <hex> - nodes whose key starts with hex"; }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    char arg[16];
    if (args(msg, arg, sizeof(arg)) == 0) { mesh.botReply(msg, "usage: prefix <hex>"); return; }

    uint8_t want[6]; uint8_t wlen = 0; int hi = -1;
    for (const char* p = arg; *p && wlen < 6; p++) {
      int v;
      char c = *p;
      if (c >= '0' && c <= '9') v = c - '0';
      else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
      else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
      else continue;
      if (hi < 0) hi = v; else { want[wlen++] = (hi << 4) | v; hi = -1; }
    }
    if (wlen == 0) { mesh.botReply(msg, "need at least 2 hex chars"); return; }

    const NodeRec* hits[6];
    int n = mesh.store().findByHash(want, wlen, hits, 6);
    if (n == 0) { mesh.botReply(msg, "no match"); return; }

    char out[BOT_MAX_REPLY_LEN + 1];
    size_t k = 0;
    for (int i = 0; i < n && k < sizeof(out) - 32; i++) {
      k += snprintf(out + k, sizeof(out) - k, "%s%s", i ? ", " : "", hits[i]->name);
      if (hits[i]->hasLoc())
        k += snprintf(out + k, sizeof(out) - k, " (%.3f,%.3f)",
                      hits[i]->lat_e6 / 1e6, hits[i]->lon_e6 / 1e6);
    }
    mesh.botReply(msg, out);
  }
};
