#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../Almanac.h"
#include "../BotConfig.h"

// Sunrise / sunset / position for the bot's location (no network).
// Output mirrors the Python meshcore-bot's `sun` command.
class SunCommand : public Command {
public:
  const char* keyword() const override { return "sun"; }
  const char* help() const override { return "sun - sunrise/sunset times and sun position"; }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    uint32_t now = mesh.nowEpoch();
    if (now < 1000000000UL) { mesh.botReply(msg, "clock not set yet"); return; }

    double lat, lon;
    if (!mesh.selfLocation(lat, lon)) { mesh.botReply(msg, "my location isn't set"); return; }

    SunView s = computeSunView(now, lat, lon);
    if (s.always_up)   { mesh.botReply(msg, "\xE2\x98\x80\xEF\xB8\x8F Sun Info: up all day here"); return; }
    if (s.always_down) { mesh.botReply(msg, "\xE2\x98\x80\xEF\xB8\x8F Sun Info: polar night, sun stays down"); return; }

    const int tz = BOT_TZ_OFFSET_MIN;
    char a[16], b[16];
    char out[BOT_MAX_REPLY_LEN + 1];

    if (s.up_now) {
      almanacFormatEvent(s.sunset, tz, false, a, sizeof(a));
      almanacFormatEvent(s.sunrise, tz, false, b, sizeof(b));
      long day = (s.prev_sunrise && s.sunset) ? s.sunset - s.prev_sunrise : 0;
      long rem = s.sunset - (long)now;
      snprintf(out, sizeof(out),
               "\xE2\x98\x80\xEF\xB8\x8F Sun Info:\n"
               "SunSet: %s\nRise: %s\n"
               "Daylight: %ldh %ldm\nRemaining: %ldh %ldm\n"
               "Azimuth: %.2f\xC2\xB0\nAltitude: %.2f\xC2\xB0",
               a, b, day / 3600, (day % 3600) / 60, rem / 3600, (rem % 3600) / 60,
               s.az_deg, s.alt_deg);
    } else {
      almanacFormatEvent(s.sunrise, tz, false, a, sizeof(a));
      almanacFormatEvent(s.set_after_rise, tz, false, b, sizeof(b));
      long day = (s.set_after_rise && s.sunrise) ? s.set_after_rise - s.sunrise : 0;
      snprintf(out, sizeof(out),
               "\xE2\x98\x80\xEF\xB8\x8F Sun Info:\n"
               "SunRise: %s\nSet: %s\n"
               "Daylight: %ldh %ldm\nAzimuth: %.2f\xC2\xB0",
               a, b, day / 3600, (day % 3600) / 60, s.az_deg);
    }
    mesh.botReply(msg, out);
  }
};
