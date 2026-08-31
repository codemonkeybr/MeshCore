#include "HttpWorker.h"
#include "BotDebug.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <math.h>
#include <stdlib.h>

static QueueHandle_t s_jobs = nullptr;
static QueueHandle_t s_results = nullptr;
static TaskHandle_t  s_task = nullptr;
static volatile bool s_busy = false;

HttpWorker& HttpWorker::instance() { static HttpWorker w; return w; }

void HttpWorker::begin() {
  if (s_task) return;
  s_jobs = xQueueCreate(2, sizeof(HttpJob));
  s_results = xQueueCreate(3, sizeof(HttpResult));
  xTaskCreatePinnedToCore(taskEntry, "bot_http", BOT_HTTP_TASK_STACK, this, 1, &s_task, 0);
  BOT_DEBUG_PRINTLN("http worker up");
}

bool HttpWorker::busy() const { return s_busy || (s_jobs && uxQueueMessagesWaiting(s_jobs) > 0); }

bool HttpWorker::submit(const HttpJob& job) {
  if (!s_jobs || busy()) return false;
  return xQueueSend(s_jobs, &job, 0) == pdTRUE;
}

bool HttpWorker::pollResult(HttpResult& out) {
  if (!s_results) return false;
  return xQueueReceive(s_results, &out, 0) == pdTRUE;
}

void HttpWorker::taskEntry(void* self) { static_cast<HttpWorker*>(self)->taskLoop(); }

// ---- helpers ----------------------------------------------------------

static const char* wmo_desc(int c) {
  switch (c) {
    case 0: return "clear"; case 1: return "mostly clear"; case 2: return "partly cloudy";
    case 3: return "overcast"; case 45: case 48: return "fog";
    case 51: return "light drizzle"; case 53: return "drizzle"; case 55: return "heavy drizzle";
    case 56: case 57: return "freezing drizzle";
    case 61: return "light rain"; case 63: return "rain"; case 65: return "heavy rain";
    case 66: case 67: return "freezing rain";
    case 71: return "light snow"; case 73: return "snow"; case 75: return "heavy snow";
    case 77: return "snow grains";
    case 80: return "light showers"; case 81: return "showers"; case 82: return "heavy showers";
    case 85: return "light snow showers"; case 86: return "snow showers";
    case 95: return "thunderstorm"; case 96: return "t-storm w/hail"; case 99: return "severe t-storm";
    default: return "?";
  }
}

static const char* compass(double deg) {
  static const char* d[] = {"N","NE","E","SE","S","SW","W","NW"};
  int i = (int)floor((fmod(deg, 360.0) + 22.5) / 45.0) & 7;
  return d[i];
}

// GET url into a JsonDocument. Returns HTTP status (or a negative error).
// If `filter` is non-null, only keys present in *filter are kept.
static int fetch_json(const char* url, JsonDocument& doc, bool accept_json = true,
                      JsonDocument* filter = nullptr) {
  WiFiClientSecure client;
#if BOT_HTTP_INSECURE
  client.setInsecure();
#endif

  HTTPClient http;
  http.setConnectTimeout(BOT_HTTP_TIMEOUT_MS);
  http.setTimeout(BOT_HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setReuse(false);
  if (!http.begin(client, url)) return -1;
  http.addHeader("User-Agent", "meshcore-heltec-bot/0.4 (+github.com/codemonkeybr)");
  if (accept_json) http.addHeader("Accept", "application/json");

  int code = http.GET();
  if (code == 200) {
    int len = http.getSize();
    if (len > 24000) { http.end(); BOT_DEBUG_PRINTLN("resp too big: %d", len); return -3; }
    // getString() de-chunks (getStream() leaves chunk framing in the bytes).
    String body = http.getString();
    DeserializationError err = filter
        ? deserializeJson(doc, body, DeserializationOption::Filter(*filter))
        : deserializeJson(doc, body);
    if (err) {
      BOT_DEBUG_PRINTLN("json err: %s (body %u b)", err.c_str(), (unsigned)body.length());
      code = -2;
    }
  }
  http.end();
  return code;
}

// ---- per-kind ---------------------------------------------------------

static void do_wx(const HttpJob& j, char* out, size_t n) {
  char url[240];
  snprintf(url, sizeof(url),
    "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
    "&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,"
    "wind_speed_10m,wind_direction_10m,precipitation"
    "&temperature_unit=%s&wind_speed_unit=%s",
    j.lat, j.lon,
    BOT_WX_METRIC ? "celsius" : "fahrenheit",
    BOT_WX_METRIC ? "kmh" : "mph");

  JsonDocument doc;
  int code = fetch_json(url, doc, false, nullptr);
  if (code != 200) { snprintf(out, n, "weather unavailable (%d)", code); return; }

  JsonObject c = doc["current"];
  double t = c["temperature_2m"] | 0.0;
  double feels = c["apparent_temperature"] | t;
  int rh = c["relative_humidity_2m"] | 0;
  int wc = c["weather_code"] | -1;
  double ws = c["wind_speed_10m"] | 0.0;
  double wd = c["wind_direction_10m"] | 0.0;
  const char* tu = BOT_WX_METRIC ? "C" : "F";
  const char* su = BOT_WX_METRIC ? "km/h" : "mph";
  snprintf(out, n, "%.0f%s (feels %.0f), %s, %d%% RH, wind %.0f %s %s",
           t, tu, feels, wmo_desc(wc), rh, ws, su, compass(wd));
}

static void do_aqi(const HttpJob& j, char* out, size_t n) {
  char url[220];
  snprintf(url, sizeof(url),
    "https://air-quality-api.open-meteo.com/v1/air-quality?latitude=%.4f&longitude=%.4f"
    "&current=us_aqi,pm2_5,pm10,ozone", j.lat, j.lon);
  JsonDocument doc;
  int code = fetch_json(url, doc, false, nullptr);
  if (code != 200) { snprintf(out, n, "AQI unavailable (%d)", code); return; }

  JsonObject c = doc["current"];
  int aqi = c["us_aqi"] | -1;
  double pm25 = c["pm2_5"] | 0.0;
  double pm10 = c["pm10"] | 0.0;
  const char* cat = aqi < 0 ? "?" : aqi <= 50 ? "good" : aqi <= 100 ? "moderate" :
                    aqi <= 150 ? "unhealthy (sensitive)" : aqi <= 200 ? "unhealthy" :
                    aqi <= 300 ? "very unhealthy" : "hazardous";
  snprintf(out, n, "AQI %d (%s), PM2.5 %.1f, PM10 %.1f", aqi, cat, pm25, pm10);
}

static void do_rain(const HttpJob& j, char* out, size_t n) {
  char url[220];
  snprintf(url, sizeof(url),
    "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
    "&minutely_15=precipitation&forecast_minutely_15=8", j.lat, j.lon);
  JsonDocument doc;
  int code = fetch_json(url, doc, false, nullptr);
  if (code != 200) { snprintf(out, n, "rain nowcast unavailable (%d)", code); return; }

  JsonArray p = doc["minutely_15"]["precipitation"];
  if (p.isNull() || p.size() == 0) { snprintf(out, n, "no nowcast data"); return; }

  bool now_wet = p[0].as<double>() > 0.05;
  int change_idx = -1;
  for (size_t i = 1; i < p.size(); i++) {
    bool wet = p[i].as<double>() > 0.05;
    if (wet != now_wet) { change_idx = (int)i; break; }
  }
  if (change_idx < 0) {
    snprintf(out, n, now_wet ? "raining, no letup in the next 2h" : "dry, no rain in the next 2h");
  } else {
    int mins = change_idx * 15;
    snprintf(out, n, now_wet ? "rain easing in ~%d min" : "rain starting in ~%d min", mins);
  }
}

static bool fetch_noaa_val(const char* url, const char* const* keys, char* val, size_t vn) {
  JsonDocument doc;
  if (fetch_json(url, doc, false) != 200) return false;
  for (int i = 0; keys[i]; i++) {
    JsonVariant v = doc[keys[i]];
    if (!v.isNull()) {
      if (v.is<const char*>()) strlcpy(val, v.as<const char*>(), vn);
      else snprintf(val, vn, "%g", v.as<double>());
      return true;
    }
  }
  return false;
}

static void do_solar(const HttpJob&, char* out, size_t n) {
  char flux[16] = "?", kp[16] = "?", ssn[16] = "?";
  const char* flux_keys[] = {"Flux", "flux", nullptr};
  const char* kp_keys[]   = {"kp", "kp_index", "k_index", nullptr};
  const char* ssn_keys[]  = {"SSN", "ssn", "sunspot_number", nullptr};
  fetch_noaa_val("https://services.swpc.noaa.gov/products/summary/10cm-flux.json", flux_keys, flux, sizeof(flux));
  fetch_noaa_val("https://services.swpc.noaa.gov/products/summary/planetary-k-index.json", kp_keys, kp, sizeof(kp));
  fetch_noaa_val("https://services.swpc.noaa.gov/products/summary/sunspot-number.json", ssn_keys, ssn, sizeof(ssn));

  int sfi = atoi(flux);
  double k = atof(kp);
  const char* hf = (sfi >= 150 && k < 3) ? "HF good on 20-10m"
                 : (sfi >= 110 && k < 4) ? "HF fair, 40-20m best"
                 : (k >= 5)              ? "HF poor - geomag storm"
                                         : "HF marginal";
  snprintf(out, n, "SFI %s, Kp %s, SSN %s - %s", flux, kp, ssn, hf);
}

static void do_joke(const HttpJob&, char* out, size_t n) {
  JsonDocument filter;
  filter["joke"] = true;
  JsonDocument doc;
  int code = fetch_json("https://v2.jokeapi.dev/joke/Any?safe-mode&type=single&format=json",
                        doc, true, &filter);
  if (code != 200 || doc["joke"].isNull()) { snprintf(out, n, "no joke right now (%d)", code); return; }
  strlcpy(out, doc["joke"] | "?", n);
}

static void do_dadjoke(const HttpJob&, char* out, size_t n) {
  JsonDocument filter; filter["joke"] = true;
  JsonDocument doc;
  int code = fetch_json("https://icanhazdadjoke.com/", doc, true, &filter);
  if (code != 200 || doc["joke"].isNull()) { snprintf(out, n, "no dad joke right now (%d)", code); return; }
  strlcpy(out, doc["joke"] | "?", n);
}

// ---- task loop ------------------------------------------------------

void HttpWorker::taskLoop() {
  HttpJob job;
  for (;;) {
    if (xQueueReceive(s_jobs, &job, portMAX_DELAY) != pdTRUE) continue;
    s_busy = true;

    HttpResult res;
    res.target = job.target;
    res.text[0] = '\0';

    if (WiFi.status() != WL_CONNECTED) {
      strlcpy(res.text, "offline - no WiFi", sizeof(res.text));
    } else {
      switch (job.kind) {
        case HTTP_WX:      do_wx(job, res.text, sizeof(res.text)); break;
        case HTTP_AQI:     do_aqi(job, res.text, sizeof(res.text)); break;
        case HTTP_RAIN:    do_rain(job, res.text, sizeof(res.text)); break;
        case HTTP_SOLAR:   do_solar(job, res.text, sizeof(res.text)); break;
        case HTTP_JOKE:    do_joke(job, res.text, sizeof(res.text)); break;
        case HTTP_DADJOKE: do_dadjoke(job, res.text, sizeof(res.text)); break;
        default:           strlcpy(res.text, "?", sizeof(res.text)); break;
      }
    }

    xQueueSend(s_results, &res, pdMS_TO_TICKS(1000));
    s_busy = false;
  }
}
