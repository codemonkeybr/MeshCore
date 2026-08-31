#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../HttpWorker.h"

// Space weather / HF propagation summary (NOAA SWPC).
class SolarCommand : public Command {
public:
  const char* keyword() const override { return "solar"; }
  const char* help() const override { return "solar - SFI / Kp / sunspots + HF outlook"; }

  bool matches(const BotMsg& msg) const override {
    return matchesKeyword(msg, "solar") || matchesKeyword(msg, "hfcond") ||
           matchesKeyword(msg, "hf");
  }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    HttpJob j;
    j.kind = HTTP_SOLAR;
    j.target = mesh.targetFor(msg);
    if (!HttpWorker::instance().submit(j))
      mesh.botReply(msg, "busy fetching, try again shortly");
  }
};
