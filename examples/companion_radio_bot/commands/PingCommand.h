#pragma once

#include "Command.h"
#include "../BotMesh.h"

// Minimal round-trip test command. First real end-to-end path for the spike:
// mesh RX -> router -> reply back over the mesh.
class PingCommand : public Command {
public:
  const char* keyword() const override { return "ping"; }
  const char* help() const override { return "ping - reply with pong"; }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    mesh.botReply(msg, "pong");
  }
};
