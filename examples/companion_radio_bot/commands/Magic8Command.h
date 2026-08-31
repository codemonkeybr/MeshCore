#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include <Arduino.h>

class Magic8Command : public Command {
public:
  const char* keyword() const override { return "magic8"; }
  const char* help() const override { return "magic8 <question> - shake the 8-ball"; }

  bool matches(const BotMsg& msg) const override {
    return matchesKeyword(msg, "magic8") || matchesKeyword(msg, "8ball");
  }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    static const char* answers[] = {
      "It is certain", "Without a doubt", "Yes definitely", "Most likely",
      "Signs point to yes", "Reply hazy, try again", "Ask again later",
      "Cannot predict now", "Don't count on it", "My reply is no",
      "Very doubtful", "Outlook not so good",
    };
    mesh.botReply(msg, answers[random(0, 12)]);
  }
};
