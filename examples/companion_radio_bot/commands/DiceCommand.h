#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include <Arduino.h>
#include <stdlib.h>

// Tabletop dice: "dice", "dice 2d6", "dice d20", "roll 3d8+2".
class DiceCommand : public Command {
public:
  const char* keyword() const override { return "dice"; }
  const char* help() const override { return "dice [NdM(+K)] - roll dice, e.g. dice 2d6"; }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    char a[32];
    args(msg, a, sizeof(a));

    int count = 1, sides = 6, modifier = 0;
    if (a[0]) {
      const char* p = a;
      long n = strtol(p, (char**)&p, 10);
      if (*p == 'd' || *p == 'D') {
        p++;
        count = (n >= 1 && n <= 20) ? (int)n : 1;
        long s = strtol(p, (char**)&p, 10);
        sides = (s >= 2 && s <= 1000) ? (int)s : 6;
        if (*p == '+' || *p == '-') modifier = (int)strtol(p, nullptr, 10);
      } else if (n >= 2 && n <= 1000) {
        sides = (int)n;
      }
    }

    int total = modifier;
    char rolls[BOT_MAX_REPLY_LEN];
    size_t rl = 0;
    for (int i = 0; i < count; i++) {
      int r = (int)random(1, sides + 1);
      total += r;
      rl += snprintf(rolls + rl, sizeof(rolls) - rl, "%s%d", i ? "," : "", r);
      if (rl >= sizeof(rolls) - 8) break;
    }

    char mod[8] = "";
    if (modifier > 0) snprintf(mod, sizeof(mod), "+%d", modifier);
    else if (modifier < 0) snprintf(mod, sizeof(mod), "%d", modifier);

    char out[BOT_MAX_REPLY_LEN + 1];
    if (count == 1 && modifier == 0) {
      snprintf(out, sizeof(out), "d%d: %d", sides, total);
    } else {
      snprintf(out, sizeof(out), "%dd%d%s: [%s]%s = %d", count, sides, mod,
               rolls, mod, total);
    }
    mesh.botReply(msg, out);
  }
};
