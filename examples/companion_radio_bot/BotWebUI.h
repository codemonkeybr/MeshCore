#pragma once

// Minimal status + OTA web UI, served on port 80 over WiFi. Read-only status
// page + /api/status JSON + ElegantOTA firmware update. Basic-auth protected
// (BOT_WEB_USER / BOT_WEB_PASS).
namespace BotWebUI {
  void begin();       // idempotent; call once WiFi is connected
  bool started();
}
