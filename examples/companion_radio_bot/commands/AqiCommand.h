#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../HttpWorker.h"

class AqiCommand : public Command {
public:
  const char* keyword() const override { return "aqi"; }
  const char* help() const override { return "aqi - air quality for my location"; }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    double lat, lon;
    if (!mesh.selfLocation(lat, lon)) { mesh.botReply(msg, "my location isn't set"); return; }
    HttpJob j;
    j.kind = HTTP_AQI;
    j.target = mesh.targetFor(msg);
    j.lat = lat; j.lon = lon;
    if (!HttpWorker::instance().submit(j))
      mesh.botReply(msg, "busy fetching, try again shortly");
  }
};
