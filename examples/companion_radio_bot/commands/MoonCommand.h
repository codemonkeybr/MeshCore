#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../Almanac.h"
#include "../BotConfig.h"

// Moon phase / illumination / rise-set / next principal phases (no network).
// Output mirrors the Python meshcore-bot's `moon` command.
class MoonCommand : public Command {
public:
  const char* keyword() const override { return "moon"; }
  const char* help() const override { return "moon - phase, illumination, rise/set, next full/new"; }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    uint32_t now = mesh.nowEpoch();
    if (now < 1000000000UL) { mesh.botReply(msg, "clock not set yet"); return; }

    double lat, lon;
    if (!mesh.selfLocation(lat, lon)) { mesh.botReply(msg, "my location isn't set"); return; }

    MoonView m = computeMoonView(now, lat, lon);
    const int tz = BOT_TZ_OFFSET_MIN;
    char rise[16], set[16], full[16], neww[16];
    almanacFormatEvent(m.moonrise, tz, false, rise, sizeof(rise));
    almanacFormatEvent(m.moonset,  tz, false, set,  sizeof(set));
    almanacFormatEvent(m.next_full, tz, true, full, sizeof(full));
    almanacFormatEvent(m.next_new,  tz, true, neww, sizeof(neww));

    char out[BOT_MAX_REPLY_LEN + 1];
    snprintf(out, sizeof(out),
             "\xF0\x9F\x8C\x99 %s%s %.2f%%\n"
             "Rise:%s Set:%s\n"
             "Full:%s New:%s",
             m.phase, m.emoji, m.illum_pct, rise, set, full, neww);
    mesh.botReply(msg, out);
  }
};
