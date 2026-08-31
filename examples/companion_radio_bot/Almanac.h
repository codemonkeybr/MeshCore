#pragma once

#include <stdint.h>

// Local astronomy: sun and moon, computed from a UTC epoch + a lat/lon.
// No network. Algorithms: NOAA sunrise/sunset; simple synodic-month moon phase.

struct SunTimes {
  bool   valid = false;
  int    sunrise_min = -1;   // minutes past local midnight, -1 = none (polar day/night)
  int    sunset_min = -1;
  int    noon_min = -1;
  int    civil_dawn_min = -1;
  int    civil_dusk_min = -1;
  int    day_length_min = 0;
  bool   always_up = false;
  bool   always_down = false;
};

struct MoonInfo {
  float  phase = 0.0f;        // 0..1 (0 = new, 0.5 = full)
  float  illum = 0.0f;        // 0..1 illuminated fraction
  float  age_days = 0.0f;     // days since new moon
  const char* name = "";      // "new", "waxing crescent", ...
  int    next_new_days = 0;
  int    next_full_days = 0;
};

// tz_offset_min: local time offset from UTC (minutes). Times in SunTimes are
// minutes past LOCAL midnight for the local calendar day of `utc_epoch`.
SunTimes computeSun(uint32_t utc_epoch, double lat, double lon, int tz_offset_min);
MoonInfo computeMoon(uint32_t utc_epoch);
