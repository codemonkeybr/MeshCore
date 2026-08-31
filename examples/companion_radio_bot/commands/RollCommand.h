#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include <Arduino.h>
#include <stdlib.h>

// "roll" -> 1..100 ; "roll 20" -> 1..20
class RollCommand : public Command {
public:
  const char* keyword() const override { return "roll"; }
  const char* help() const override { return "roll [N] - random 1..N (default 100)"; }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    char a[16];
    args(msg, a, sizeof(a));
    long n = a[0] ? strtol(a, nullptr, 10) : 100;
    if (n < 2) n = 100;
    if (n > 1000000) n = 1000000;

    char out[BOT_MAX_REPLY_LEN + 1];
    snprintf(out, sizeof(out), "%s rolls %ld (1-%ld)",
             msg.sender_name ? msg.sender_name : "you", (long)random(1, n + 1), n);
    mesh.botReply(msg, out);
  }
};
