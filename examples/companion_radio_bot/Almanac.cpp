#include "Almanac.h"
#include <math.h>
#include <string.h>
#include <time.h>

static const double D2R = M_PI / 180.0;
static const double R2D = 180.0 / M_PI;

static double jd_from_epoch(double epoch) { return epoch / 86400.0 + 2440587.5; }
static double norm360(double x) { x = fmod(x, 360.0); return x < 0 ? x + 360.0 : x; }

// Low-precision equatorial position (RA/Dec, degrees) of the sun or moon.
// Sun: Astronomical Almanac "low precision" formulae. Moon: Meeus ch. 47 abridged.
static void bodyRaDec(double epoch, bool moon, double& ra_deg, double& dec_deg) {
  double d = jd_from_epoch(epoch) - 2451545.0;
  double eps = 23.4393 - 3.563e-7 * d;
  double lambda, beta;

  if (!moon) {
    double L = norm360(280.460 + 0.9856474 * d);
    double g = norm360(357.528 + 0.9856003 * d) * D2R;
    lambda = L + 1.915 * sin(g) + 0.020 * sin(2 * g);
    beta = 0.0;
  } else {
    double L = norm360(218.316 + 13.176396 * d);   // mean longitude
    double M = norm360(134.963 + 13.064993 * d) * D2R;   // mean anomaly
    double F = norm360(93.272 + 13.229350 * d) * D2R;    // argument of latitude
    double D = norm360(297.850 + 12.190749 * d) * D2R;   // mean elongation
    double g = norm360(357.528 + 0.9856003 * d) * D2R;   // sun's mean anomaly
    lambda = L + 6.289 * sin(M) + 1.274 * sin(2 * D - M) + 0.658 * sin(2 * D)
               + 0.214 * sin(2 * M) - 0.186 * sin(g) - 0.114 * sin(2 * F);
    beta = 5.128 * sin(F) + 0.281 * sin(M + F) - 0.278 * sin(F - M);
  }

  double lr = lambda * D2R, br = beta * D2R, er = eps * D2R;
  double sl = sin(lr), cl = cos(lr), sb = sin(br), cb = cos(br);
  ra_deg = norm360(atan2(sl * cos(er) - (sb / cb) * sin(er), cl) * R2D);
  dec_deg = asin(sb * cos(er) + cb * sin(er) * sl) * R2D;
}

// Altitude (degrees) of the body now; azimuth (deg from N, clockwise) via az_out.
static double bodyAltitude(double epoch, bool moon, double lat, double lon,
                           double* az_out = nullptr) {
  double ra, dec;
  bodyRaDec(epoch, moon, ra, dec);
  double d = jd_from_epoch(epoch) - 2451545.0;
  double gmst = norm360(280.46061837 + 360.98564736629 * d);
  double H = norm360(gmst + lon - ra) * D2R;
  double latr = lat * D2R, decr = dec * D2R;
  double alt = asin(sin(latr) * sin(decr) + cos(latr) * cos(decr) * cos(H)) * R2D;
  if (az_out) {
    double az = atan2(sin(H), cos(H) * sin(latr) - tan(decr) * cos(latr)) * R2D;
    *az_out = norm360(az + 180.0);
  }
  return alt;
}

// First epoch after `from` where the body crosses `h0` in the requested
// direction (rising = up-crossing). 0 if none within `span` seconds.
static long nextCrossing(double from, bool moon, double lat, double lon,
                         double h0, bool rising, long span) {
  const long step = 300;   // 5 min
  double prev = bodyAltitude(from, moon, lat, lon) - h0;
  for (long t = step; t <= span; t += step) {
    double cur = bodyAltitude(from + t, moon, lat, lon) - h0;
    bool hit = rising ? (prev < 0.0 && cur >= 0.0) : (prev >= 0.0 && cur < 0.0);
    if (hit) {
      double frac = prev / (prev - cur);   // 0..1 within [t-step, t]
      return (long)llround(from + (t - step) + frac * step);
    }
    prev = cur;
  }
  return 0;
}

// ---- moon phase (synodic model) --------------------------------------

MoonInfo computeMoon(uint32_t utc_epoch) {
  MoonInfo m;
  const double synodic = 29.530588861;
  double days_since_new = jd_from_epoch(utc_epoch) - 2451550.1;   // 2000-01-06 new moon
  double phase = fmod(days_since_new / synodic, 1.0);
  if (phase < 0) phase += 1.0;

  m.phase = (float)phase;
  m.age_days = (float)(phase * synodic);
  m.illum = (float)((1.0 - cos(2 * M_PI * phase)) / 2.0);
  m.next_new_days = (int)ceil((1.0 - phase) * synodic);
  m.next_full_days = (int)ceil(((phase < 0.5 ? 0.5 : 1.5) - phase) * synodic);

  if (phase < 0.02 || phase >= 0.98)     m.name = "new";
  else if (phase < 0.24)                 m.name = "waxing crescent";
  else if (phase < 0.26)                 m.name = "first quarter";
  else if (phase < 0.49)                 m.name = "waxing gibbous";
  else if (phase < 0.51)                 m.name = "full";
  else if (phase < 0.74)                 m.name = "waning gibbous";
  else if (phase < 0.76)                 m.name = "last quarter";
  else                                   m.name = "waning crescent";
  return m;
}

// Phase name + emoji, keyed on lunation fraction (matches the Python bot's
// _MOON_PHASES boundaries).
static void moonPhaseNameEmoji(double frac, const char** name, const char** emoji) {
  struct P { double ub; const char* n; const char* e; };
  static const P t[] = {
    { 0.0625, "New Moon",        "\xF0\x9F\x8C\x91" },
    { 0.1875, "Waxing Crescent", "\xF0\x9F\x8C\x92" },
    { 0.3125, "First Quarter",   "\xF0\x9F\x8C\x93" },
    { 0.4375, "Waxing Gibbous",  "\xF0\x9F\x8C\x94" },
    { 0.5625, "Full Moon",       "\xF0\x9F\x8C\x95" },
    { 0.6875, "Waning Gibbous",  "\xF0\x9F\x8C\x96" },
    { 0.8125, "Last Quarter",    "\xF0\x9F\x8C\x97" },
    { 0.9375, "Waning Crescent", "\xF0\x9F\x8C\x98" },
  };
  for (const P& p : t) {
    if (frac < p.ub) { *name = p.n; *emoji = p.e; return; }
  }
  *name = "New Moon";
  *emoji = "\xF0\x9F\x8C\x91";
}

// ---- views ----------------------------------------------------------

SunView computeSunView(uint32_t utc_epoch, double lat, double lon) {
  SunView v;
  const double H0 = -0.833;   // disc edge + refraction
  v.alt_deg = bodyAltitude(utc_epoch, false, lat, lon, &v.az_deg);
  v.up_now = v.alt_deg >= 0.0;

  v.sunrise = nextCrossing(utc_epoch, false, lat, lon, H0, true, 108000);
  v.sunset  = nextCrossing(utc_epoch, false, lat, lon, H0, false, 108000);

  if (v.sunrise == 0 && v.sunset == 0) {   // polar day / night
    v.always_up = v.up_now;
    v.always_down = !v.up_now;
    v.valid = true;
    return v;
  }

  if (v.up_now) {
    v.prev_sunrise = nextCrossing((double)utc_epoch - 100000.0, false, lat, lon,
                                  H0, true, 100000);
  } else if (v.sunrise) {
    v.set_after_rise = nextCrossing((double)v.sunrise, false, lat, lon,
                                    H0, false, 108000);
  }
  v.valid = true;
  return v;
}

MoonView computeMoonView(uint32_t utc_epoch, double lat, double lon) {
  MoonView v;
  MoonInfo mi = computeMoon(utc_epoch);
  v.illum_pct = mi.illum * 100.0f;
  moonPhaseNameEmoji(mi.phase, &v.phase, &v.emoji);

  const double H0 = 0.125;   // mean parallax - semidiameter - refraction
  v.moonrise = nextCrossing(utc_epoch, true, lat, lon, H0, true, 108000);
  v.moonset  = nextCrossing(utc_epoch, true, lat, lon, H0, false, 108000);

  const double synodic = 29.530588861;
  double phase = mi.phase;
  v.next_new = (long)utc_epoch + (long)((1.0 - phase) * synodic * 86400.0);
  double to_full = ((phase < 0.5 ? 0.5 : 1.5) - phase) * synodic * 86400.0;
  v.next_full = (long)utc_epoch + (long)to_full;

  v.valid = true;
  return v;
}

void almanacFormatEvent(long epoch_utc, int tz_min, bool with_month, char* out, size_t n) {
  if (n == 0) return;
  if (epoch_utc <= 0) { strncpy(out, "--", n); out[n - 1] = 0; return; }
  time_t local = (time_t)epoch_utc + (time_t)tz_min * 60;
  struct tm tmv;
  gmtime_r(&local, &tmv);
  strftime(out, n, with_month ? "%b %d %I:%M%p" : "%a %d %I:%M%p", &tmv);
}
