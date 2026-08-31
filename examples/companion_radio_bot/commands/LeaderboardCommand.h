#pragma once

#include "Command.h"
#include "../BotMesh.h"

// Farthest `path`-measured distance this week and all-time.
class LeaderboardCommand : public Command {
public:
  const char* keyword() const override { return "leaderboard"; }
  const char* help() const override { return "leaderboard - farthest path distance (week / all-time)"; }

  bool matches(const BotMsg& msg) const override {
    return matchesKeyword(msg, "leaderboard") || matchesKeyword(msg, "lb");
  }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    char wkw[20], atw[20];
    float wk = mesh.store().weekBest(wkw, sizeof(wkw));
    float at = mesh.store().allTimeBest(atw, sizeof(atw));

    char out[BOT_MAX_REPLY_LEN + 1];
    if (at <= 0.0f) { mesh.botReply(msg, "no path distances recorded yet"); return; }
    snprintf(out, sizeof(out), "week: %.0f km (%s) | all-time: %.0f km (%s)",
             wk, wkw[0] ? wkw : "?", at, atw[0] ? atw : "?");
    mesh.botReply(msg, out);
  }
};
