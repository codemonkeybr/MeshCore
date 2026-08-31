#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../HttpWorker.h"

class DadjokeCommand : public Command {
public:
  const char* keyword() const override { return "dadjoke"; }
  const char* help() const override { return "dadjoke - a dad joke"; }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    HttpJob j;
    j.kind = HTTP_DADJOKE;
    j.target = mesh.targetFor(msg);
    if (!HttpWorker::instance().submit(j))
      mesh.botReply(msg, "busy, try again shortly");
  }
};
