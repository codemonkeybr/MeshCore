#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../Almanac.h"

// Moon phase / illumination / age (no network).
class MoonCommand : public Command {
public:
  const char* keyword() const override { return "moon"; }
  const char* help() const override { return "moon - phase, illumination, age"; }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    uint32_t now = mesh.nowEpoch();
    if (now < 1000000000UL) { mesh.botReply(msg, "clock not set yet"); return; }

    MoonInfo m = computeMoon(now);
    char out[BOT_MAX_REPLY_LEN + 1];
    snprintf(out, sizeof(out),
             "%s, %d%% lit, age %.1fd | full in %dd, new in %dd",
             m.name, (int)(m.illum * 100.0f + 0.5f), m.age_days,
             m.next_full_days, m.next_new_days);
    mesh.botReply(msg, out);
  }
};
