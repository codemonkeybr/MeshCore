#pragma once

#include "Command.h"
#include "../BotMesh.h"

// Link check: echoes SNR and how the request reached the bot.
class TestCommand : public Command {
public:
  const char* keyword() const override { return "test"; }
  const char* help() const override { return "test (or t) - link check: SNR + hop count"; }

  bool matches(const BotMsg& msg) const override {
    return matchesKeyword(msg, "test") || matchesKeyword(msg, "t");
  }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    char out[BOT_MAX_REPLY_LEN + 1];
    if (!msg.is_flood) {
      snprintf(out, sizeof(out), "test ok! SNR %.1f, direct (0 hops)", msg.snr);
    } else {
      snprintf(out, sizeof(out), "test ok! SNR %.1f, %u hop%s",
               msg.snr, msg.path_hops, msg.path_hops == 1 ? "" : "s");
    }
    mesh.botReply(msg, out);
  }
};
