#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../CommandRouter.h"
#include <string.h>

// Space-separated list of available command keywords, prefixed "Avail. Commands:".
// The list is fixed at build time; if it will not fit one mesh text packet it is
// split into exactly two messages joined by a "..." continuation marker.
class CmdCommand : public Command {
public:
  const char* keyword() const override { return "cmd"; }
  const char* help() const override { return "cmd - list available commands"; }

  bool matches(const BotMsg& msg) const override {
    return matchesKeyword(msg, "cmd") || matchesKeyword(msg, "commands");
  }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    static const char PFX[]  = "Avail. Commands:";  // one space precedes each keyword
    static const char CONT[] = " ...";             // appended to message 1
    static const char HEAD[] = "...";              // starts message 2

    char out[BOT_MAX_REPLY_LEN + 1];
    const size_t LIMIT = sizeof(out) - 1;
    const CommandRouter& r = mesh.router();

    // Full length of "Avail. Commands: kw kw kw ..." for every command.
    size_t total = strlen(PFX);
    for (int i = 0; i < r.count(); i++) total += 1 + strlen(r.at(i)->keyword());

    size_t n = strlen(PFX);
    memcpy(out, PFX, n);
    int i = 0;

    if (total <= LIMIT) {                    // fits one message
      for (; i < r.count(); i++) {
        const char* kw = r.at(i)->keyword();
        out[n++] = ' ';
        memcpy(out + n, kw, strlen(kw));
        n += strlen(kw);
      }
      out[n] = '\0';
      mesh.botReply(msg, out);
      return;
    }

    // Message 1: keywords that fit while leaving room for CONT. Since total
    // exceeds LIMIT this loop always stops with keywords still to send.
    for (; i < r.count(); i++) {
      const char* kw = r.at(i)->keyword();
      if (n + 1 + strlen(kw) + strlen(CONT) > LIMIT) break;
      out[n++] = ' ';
      memcpy(out + n, kw, strlen(kw));
      n += strlen(kw);
    }
    memcpy(out + n, CONT, strlen(CONT));
    n += strlen(CONT);
    out[n] = '\0';
    mesh.botReply(msg, out);

    // Message 2: "..." + the remaining keywords. Two messages is by design
    // always enough for the built-in command list; any further overflow just
    // stops here rather than sending a third message.
    n = strlen(HEAD);
    memcpy(out, HEAD, n);
    for (; i < r.count(); i++) {
      const char* kw = r.at(i)->keyword();
      if (n + 1 + strlen(kw) > LIMIT) break;
      out[n++] = ' ';
      memcpy(out + n, kw, strlen(kw));
      n += strlen(kw);
    }
    out[n] = '\0';
    mesh.botReply(msg, out);
  }
};
