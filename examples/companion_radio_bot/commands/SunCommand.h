#pragma once

#include "Command.h"
#include "../BotMesh.h"
#include "../Almanac.h"
#include "../BotConfig.h"

// Sunrise / sunset / twilight for the bot's location (no network).
class SunCommand : public Command {
public:
  const char* keyword() const override { return "sun"; }
  const char* help() const override { return "sun - sunrise, sunset, twilight for my location"; }

  void run(BotMesh& mesh, const BotMsg& msg) override {
    uint32_t now = mesh.nowEpoch();
    if (now < 1000000000UL) { mesh.botReply(msg, "clock not set yet"); return; }

    double lat, lon;
    if (!mesh.selfLocation(lat, lon)) { mesh.botReply(msg, "my location isn't set"); return; }

    SunTimes s = computeSun(now, lat, lon, BOT_TZ_OFFSET_MIN);
    if (s.always_up)   { mesh.botReply(msg, "sun is up all day here"); return; }
    if (s.always_down) { mesh.botReply(msg, "polar night - sun stays down"); return; }

    char rise[6], set[6], noon[6], dawn[6], dusk[6];
    hhmm(s.sunrise_min, rise);
    hhmm(s.sunset_min, set);
    hhmm(s.noon_min, noon);
    hhmm(s.civil_dawn_min, dawn);
    hhmm(s.civil_dusk_min, dusk);

    char out[BOT_MAX_REPLY_LEN + 1];
    snprintf(out, sizeof(out),
             "rise %s, set %s (%dh%02dm day), noon %s, civil %s-%s",
             rise, set, s.day_length_min / 60, s.day_length_min % 60, noon, dawn, dusk);
    mesh.botReply(msg, out);
  }

private:
  static void hhmm(int m, char* out) {
    if (m < 0) { out[0] = '-'; out[1] = '-'; out[2] = '\0'; return; }
    snprintf(out, 6, "%02d:%02d", (m / 60) % 24, m % 60);
  }
};
