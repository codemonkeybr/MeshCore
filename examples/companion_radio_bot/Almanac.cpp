#include "Almanac.h"
#include <math.h>

static const double D2R = M_PI / 180.0;
static const double R2D = 180.0 / M_PI;

static double jd_from_epoch(uint32_t epoch) { return epoch / 86400.0 + 2440587.5; }

// Wikipedia "sunrise equation" (Meeus-based). altitude_deg: -0.833 for the disc
// edge (sunrise/set), -6 for civil twilight. Returns event times as UTC unix
// seconds via out_rise / out_set; returns false for polar day/night.
static bool solar_event(double jdate, double lat, double lon, double altitude_deg,
                        double& out_transit_unix, double& out_rise_unix, double& out_set_unix,
                        bool& always_up, bool& always_down) {
  const double lw = -lon;   // west longitude positive
  double n = round(jdate - 2451545.0 - 0.0009 + lw / 360.0);
  double Jstar = 2451545.0 + 0.0009 + (lw / 360.0) + n;
  double M = fmod(357.5291 + 0.98560028 * (Jstar - 2451545.0), 360.0);
  if (M < 0) M += 360.0;
  double Mr = M * D2R;
  double C = 1.9148 * sin(Mr) + 0.02 * sin(2 * Mr) + 0.0003 * sin(3 * Mr);
  double lambda = fmod(M + C + 180.0 + 102.9372, 360.0);
  double lr = lambda * D2R;
  double Jtransit = Jstar + 0.0053 * sin(Mr) - 0.0069 * sin(2 * lr);
  double decl = asin(sin(lr) * sin(23.4397 * D2R));

  double cos_w = (sin(altitude_deg * D2R) - sin(lat * D2R) * sin(decl)) /
                 (cos(lat * D2R) * cos(decl));

  out_transit_unix = (Jtransit - 2440587.5) * 86400.0;
  always_up = always_down = false;
  if (cos_w > 1.0)  { always_down = true; return false; }
  if (cos_w < -1.0) { always_up = true;   return false; }

  double w0 = acos(cos_w) * R2D;   // degrees
  double Jrise = Jtransit - w0 / 360.0;
  double Jset  = Jtransit + w0 / 360.0;
  out_rise_unix = (Jrise - 2440587.5) * 86400.0;
  out_set_unix  = (Jset  - 2440587.5) * 86400.0;
  return true;
}

static int mins_past_local_midnight(double event_unix, long local_midnight_unix, int tz_off_min) {
  double local = event_unix + tz_off_min * 60.0;
  long   m = (long)llround((local - local_midnight_unix) / 60.0);
  m %= 1440;
  if (m < 0) m += 1440;
  return (int)m;
}

SunTimes computeSun(uint32_t utc_epoch, double lat, double lon, int tz_offset_min) {
  SunTimes s;
  long tz_sec = (long)tz_offset_min * 60;
  long local_now = (long)utc_epoch + tz_sec;
  long local_midnight = (local_now / 86400) * 86400;                 // local-midnight, in "local unix"
  long local_midnight_utc = local_midnight - tz_sec;                 // same instant, UTC unix
  double jdate_noon = jd_from_epoch((uint32_t)(local_midnight_utc + 43200));

  double transit, rise, set;
  bool au, ad;
  bool ok = solar_event(jdate_noon, lat, lon, -0.833, transit, rise, set, au, ad);
  s.noon_min = mins_past_local_midnight(transit, local_midnight, tz_offset_min);
  if (!ok) {
    s.always_up = au; s.always_down = ad; s.valid = true;
    return s;
  }
  s.sunrise_min = mins_past_local_midnight(rise, local_midnight, tz_offset_min);
  s.sunset_min  = mins_past_local_midnight(set,  local_midnight, tz_offset_min);
  s.day_length_min = (int)llround((set - rise) / 60.0);

  double ct, cr, cs; bool cau, cad;
  if (solar_event(jdate_noon, lat, lon, -6.0, ct, cr, cs, cau, cad)) {
    s.civil_dawn_min = mins_past_local_midnight(cr, local_midnight, tz_offset_min);
    s.civil_dusk_min = mins_past_local_midnight(cs, local_midnight, tz_offset_min);
  }
  s.valid = true;
  return s;
}

MoonInfo computeMoon(uint32_t utc_epoch) {
  MoonInfo m;
  const double synodic = 29.530588861;
  double days_since_new = jd_from_epoch(utc_epoch) - 2451550.1;      // 2000-01-06 new moon
  double phase = fmod(days_since_new / synodic, 1.0);
  if (phase < 0) phase += 1.0;

  m.phase = (float)phase;
  m.age_days = (float)(phase * synodic);
  m.illum = (float)((1.0 - cos(2 * M_PI * phase)) / 2.0);
  m.next_new_days  = (int)ceil((1.0 - phase) * synodic);
  m.next_full_days = (int)ceil(((phase < 0.5 ? 0.5 : 1.5) - phase) * synodic);

  if (phase < 0.02 || phase >= 0.98)      m.name = "new";
  else if (phase < 0.24)                  m.name = "waxing crescent";
  else if (phase < 0.26)                  m.name = "first quarter";
  else if (phase < 0.49)                  m.name = "waxing gibbous";
  else if (phase < 0.51)                  m.name = "full";
  else if (phase < 0.74)                  m.name = "waning gibbous";
  else if (phase < 0.76)                  m.name = "last quarter";
  else                                    m.name = "waning crescent";
  return m;
}
