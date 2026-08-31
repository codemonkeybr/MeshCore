#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../CommandRouter.h"
#include <string.h>

// Compact comma-separated list of available command keywords.
class CmdCommand : public Command {
public:
  const char* keyword() const override { return "cmd"; }
  const char* help() const override { return "cmd - list available commands"; }

  bool matches(const BotMsg& msg) const override {
    return matchesKeyword(msg, "cmd") || matchesKeyword(msg, "commands");
  }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    char out[BOT_MAX_REPLY_LEN + 1];
    size_t n = 0;
    const CommandRouter& r = mesh.router();
    for (int i = 0; i < r.count(); i++) {
      const char* kw = r.at(i)->keyword();
      size_t need = strlen(kw) + (n ? 1 : 0);
      if (n + need >= sizeof(out) - 1) break;
      if (n) out[n++] = ' ';
      memcpy(out + n, kw, strlen(kw));
      n += strlen(kw);
    }
    out[n] = '\0';
    mesh.botReply(msg, out);
  }
};
