#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../HttpWorker.h"

class JokeCommand : public Command {
public:
  const char* keyword() const override { return "joke"; }
  const char* help() const override { return "joke - a (safe-mode) joke"; }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    HttpJob j;
    j.kind = HTTP_JOKE;
    j.target = mesh.targetFor(msg);
    if (!HttpWorker::instance().submit(j))
      mesh.botReply(msg, "busy, try again shortly");
  }
};
