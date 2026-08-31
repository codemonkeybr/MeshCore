#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include <Arduino.h>

// Runtime health over the mesh (our only observability until serial logging is
// sorted): uptime, free heap, message/reply counters, cache size.
class StatsCommand : public Command {
public:
  const char* keyword() const override { return "stats"; }
  const char* help() const override { return "stats - uptime, heap, message counters"; }

  bool matches(const BotMsg& msg) const override {
    return matchesKeyword(msg, "stats") || matchesKeyword(msg, "status");
  }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    uint32_t up = mesh.uptimeSecs();
    const BotMesh::Stats& s = mesh.stats;
    char out[BOT_MAX_REPLY_LEN + 1];
    snprintf(out, sizeof(out),
             "up %lud%luh%lum | heap %uk | msg %lu match %lu reply %lu drop %lu/%lu fail %lu | nodes %d",
             (unsigned long)(up / 86400), (unsigned long)((up % 86400) / 3600),
             (unsigned long)((up % 3600) / 60),
             (unsigned)(ESP.getFreeHeap() / 1024),
             (unsigned long)s.msgs, (unsigned long)s.matched, (unsigned long)s.replies,
             (unsigned long)s.dropped_rate, (unsigned long)s.dropped_gap,
             (unsigned long)s.send_fail, mesh.store().count());
    mesh.botReply(msg, out);
  }
};
