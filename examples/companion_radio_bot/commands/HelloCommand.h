#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include <Arduino.h>

class HelloCommand : public Command {
public:
  const char* keyword() const override { return "hello"; }
  const char* help() const override { return "hello - say hi"; }

  bool matches(const BotMsg& msg) const override {
    return matchesKeyword(msg, "hello") || matchesKeyword(msg, "hi") ||
           matchesKeyword(msg, "hey");
  }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    static const char* greetings[] = {
      "beep boop. hello %s",
      "hi %s, %s online",
      "greetings %s",
      "%s: systems nominal, hello",
    };
    const char* g = greetings[random(0, 4)];
    char out[BOT_MAX_REPLY_LEN + 1];
    snprintf(out, sizeof(out), g,
             msg.sender_name ? msg.sender_name : "human", mesh.getNodeName());
    mesh.botReply(msg, out);
  }
};
