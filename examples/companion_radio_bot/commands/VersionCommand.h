#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../BotConfig.h"

class VersionCommand : public Command {
public:
  const char* keyword() const override { return "version"; }
  const char* help() const override { return "version - firmware version and uptime"; }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    uint32_t up = mesh.uptimeSecs();
    char out[BOT_MAX_REPLY_LEN + 1];
    snprintf(out, sizeof(out), "%s bot %s (MeshCore %s) up %lud %luh %lum",
             mesh.getNodeName(), BOT_VERSION, FIRMWARE_VERSION,
             (unsigned long)(up / 86400), (unsigned long)((up % 86400) / 3600),
             (unsigned long)((up % 3600) / 60));
    mesh.botReply(msg, out);
  }
};
