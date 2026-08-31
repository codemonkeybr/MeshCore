#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../HttpWorker.h"

// Current conditions for the bot's location (Open-Meteo).
class WxCommand : public Command {
public:
  const char* keyword() const override { return "wx"; }
  const char* help() const override { return "wx - current weather for my location"; }

  bool matches(const BotMsg& msg) const override {
    return matchesKeyword(msg, "wx") || matchesKeyword(msg, "weather");
  }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    double lat, lon;
    if (!mesh.selfLocation(lat, lon)) { mesh.botReply(msg, "my location isn't set"); return; }
    HttpJob j;
    j.kind = HTTP_WX;
    j.target = mesh.targetFor(msg);
    j.lat = lat; j.lon = lon;
    if (!HttpWorker::instance().submit(j))
      mesh.botReply(msg, "busy fetching, try again shortly");
  }
};
