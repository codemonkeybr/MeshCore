#pragma once

#include <stdint.h>
#include <stddef.h>

// Local astronomy: sun and moon, computed from a UTC epoch + a lat/lon.
// No network. Low-precision Meeus-style body positions (good to ~1-2 min for
// rise/set at mid latitudes) + a synodic-month moon phase. The output shapes
// mirror the Python meshcore-bot (solar_conditions.get_sun / get_moon).

// ---- moon phase (kept: reused by computeMoonView) ----------------------
struct MoonInfo {
  float  phase = 0.0f;        // 0..1 lunation fraction (0 = new, 0.5 = full)
  float  illum = 0.0f;        // 0..1 illuminated fraction
  float  age_days = 0.0f;     // days since new moon
  const char* name = "";      // "new", "waxing crescent", ...
  int    next_new_days = 0;
  int    next_full_days = 0;
};
MoonInfo computeMoon(uint32_t utc_epoch);

// ---- sun --------------------------------------------------------------
struct SunView {
  bool   valid = false;
  bool   up_now = false;
  bool   always_up = false;
  bool   always_down = false;
  long   sunrise = 0;         // next rising, UTC epoch (0 = none in window)
  long   sunset = 0;          // next setting, UTC epoch
  long   prev_sunrise = 0;    // most recent rising before now (for "Daylight" when up)
  long   set_after_rise = 0;  // first setting after `sunrise` (for "Daylight" when down)
  double az_deg = 0.0;        // sun azimuth now, degrees from N clockwise
  double alt_deg = 0.0;       // sun altitude now, degrees
};
SunView computeSunView(uint32_t utc_epoch, double lat, double lon);

// ---- moon (rise/set + principal phases) ------------------------------
struct MoonView {
  bool   valid = false;
  float  illum_pct = 0.0f;    // 0..100
  const char* phase = "";     // "Waxing Gibbous" (no emoji)
  const char* emoji = "";     // "\U0001F314"
  long   moonrise = 0;        // next rising, UTC epoch (0 = none in window)
  long   moonset = 0;         // next setting, UTC epoch
  long   next_full = 0;       // UTC epoch
  long   next_new = 0;        // UTC epoch
};
MoonView computeMoonView(uint32_t utc_epoch, double lat, double lon);

// Format `epoch_utc` (shifted by tz_min) as the Python bot does:
//   with_month=false -> "Tue 02 07:34PM"   (sun events, moon rise/set)
//   with_month=true  -> "Sep 07 11:08AM"   (next full / new moon)
// Writes "--" for a zero/absent epoch.
void almanacFormatEvent(long epoch_utc, int tz_min, bool with_month, char* out, size_t n);
