#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../HttpWorker.h"

// Minute-level "rain starting/easing in ~N min" (Open-Meteo 15-minutely).
class RainCommand : public Command {
public:
  const char* keyword() const override { return "rain"; }
  const char* help() const override { return "rain - short-term rain nowcast for my location"; }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    double lat, lon;
    if (!mesh.selfLocation(lat, lon)) { mesh.botReply(msg, "my location isn't set"); return; }
    HttpJob j;
    j.kind = HTTP_RAIN;
    j.target = mesh.targetFor(msg);
    j.lat = lat; j.lon = lon;
    if (!HttpWorker::instance().submit(j))
      mesh.botReply(msg, "busy fetching, try again shortly");
  }
};
