#include "BotWebUI.h"
#include "BotMesh.h"
#include "BotConfig.h"
#include "BotDebug.h"
#include "BotStore.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#ifndef BOT_WEB_USER
#define BOT_WEB_USER "admin"
#endif
#ifndef BOT_WEB_PASS
#define BOT_WEB_PASS "changeme"
#endif

extern MESH_CLASS the_mesh;

static AsyncWebServer s_server(80);
static bool s_started = false;

static bool auth_ok(AsyncWebServerRequest* r) {
  if (r->authenticate(BOT_WEB_USER, BOT_WEB_PASS)) return true;
  r->requestAuthentication();
  return false;
}

static void status_json(String& j) {
  const BotMesh::Stats& s = the_mesh.stats;
  NodePrefs* p = the_mesh.getNodePrefs();
  const char* rst = BotMesh::lastResetReason();
  uint32_t up = the_mesh.uptimeSecs();

  j.reserve(640);
  j = "{";
  j += "\"name\":\"" + String(the_mesh.getNodeName()) + "\",";
  j += "\"version\":\"" BOT_VERSION " " __DATE__ " " __TIME__ "\",";
  j += "\"uptime_s\":" + String(up) + ",";
  j += "\"heap_free\":" + String(ESP.getFreeHeap()) + ",";
  j += "\"heap_min\":" + String(ESP.getMinFreeHeap()) + ",";
  j += "\"reset\":\"" + String(rst ? rst : "clean") + "\",";
  j += "\"clock\":" + String(the_mesh.nowEpoch()) + ",";
  j += "\"clock_ntp\":" + String(BotMesh::clockSynced() ? "true" : "false") + ",";
  j += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
  j += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  j += "\"cache_nodes\":" + String(the_mesh.store().count()) + ",";
  j += "\"msgs\":" + String(s.msgs) + ",";
  j += "\"matched\":" + String(s.matched) + ",";
  j += "\"replies\":" + String(s.replies) + ",";
  j += "\"drop_rate\":" + String(s.dropped_rate) + ",";
  j += "\"drop_gap\":" + String(s.dropped_gap) + ",";
  j += "\"drop_full\":" + String(s.dropped_full) + ",";
  j += "\"send_fail\":" + String(s.send_fail) + ",";
  if (p) {
    j += "\"lora\":\"" + String(p->freq, 3) + "MHz BW" + String(p->bw, 1) +
         " SF" + String(p->sf) + " CR" + String(p->cr) + "\",";
    j += "\"loc\":\"" + String(p->node_lat, 5) + "," + String(p->node_lon, 5) + "\"";
  } else {
    j += "\"lora\":\"?\",\"loc\":\"?\"";
  }
  j += "}";
}

// Full dump of the persistent node/repeater cache (BotStore), for debugging
// name-resolution issues (e.g. two nodes sharing a 1-byte hop hash prefix) -
// the same cache `path`/`prefix` resolve against, without needing a mesh
// command relayed back by hand.
static const char HEXD[] = "0123456789abcdef";

static void nodes_json(String& j) {
  static const NodeRec* recs[BOT_LOCCACHE_MAX];
  int n = the_mesh.store().listAll(recs, BOT_LOCCACHE_MAX);

  j.reserve((size_t)n * 96 + 16);
  j = "[";
  for (int i = 0; i < n; i++) {
    const NodeRec* r = recs[i];
    if (i) j += ",";
    j += "{\"hash\":\"";
    for (int b = 0; b < 6; b++) {
      j += HEXD[(r->pubkey[b] >> 4) & 0xF];
      j += HEXD[r->pubkey[b] & 0xF];
    }
    j += "\",\"name\":\"";
    for (const char* p = r->name; *p; p++) {
      if (*p == '"' || *p == '\\') j += '\\';
      j += *p;
    }
    j += "\",\"type\":" + String(r->type);
    j += ",\"has_loc\":" + String(r->hasLoc() ? "true" : "false");
    if (r->hasLoc()) {
      j += ",\"lat\":" + String(r->lat_e6 / 1e6, 6);
      j += ",\"lon\":" + String(r->lon_e6 / 1e6, 6);
    }
    j += ",\"last_advert\":" + String(r->last_advert);
    j += "}";
  }
  j += "]";
}

static const char PAGE[] = R"HTML(<!doctype html><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>D4rl3k</title>
<style>
body{font:14px/1.5 system-ui,sans-serif;margin:0;background:#0e1413;color:#dfe5e2}
.w{max-width:640px;margin:0 auto;padding:1.4rem}
h1{font-size:1.2rem;margin:0 0 1rem;color:#3fbaa3}
table{width:100%;border-collapse:collapse}
td{padding:.35rem .2rem;border-bottom:1px solid #263230}
td:first-child{color:#7d8a85;width:40%}
td:last-child{font-variant-numeric:tabular-nums}
a{color:#3fbaa3}
.bad{color:#e07a7a}
</style>
<div class=w>
<h1 id=name>D4rl3k</h1>
<table id=t></table>
<p><a href="/update">Firmware update (OTA)</a></p>
<p style=color:#7d8a85>auto-refresh 3s</p>
</div>
<script>
const rows=[["version","version",v=>v],
["uptime","uptime_s",v=>{let s=v|0,d=s/86400|0,h=s%86400/3600|0,m=s%3600/60|0;return `${d}d ${h}h ${m}m`}],
["free heap","heap_free",v=>(v/1024|0)+" k"],["min heap","heap_min",v=>(v/1024|0)+" k"],
["last reset","reset",v=>v],
["clock (UTC)","_clock",d=>new Date(d.clock*1000).toISOString().slice(0,19).replace("T"," ")+(d.clock_ntp?" (NTP)":" (no NTP)")],
["wifi","wifi_rssi",v=>v+" dBm"],["ip","ip",v=>v],
["cached nodes","cache_nodes",v=>v],["lora","lora",v=>v],["location","loc",v=>v],
["msgs seen","msgs",v=>v],["matched","matched",v=>v],["replies sent","replies",v=>v],
["drop rate/gap/full","_drop",d=>`${d.drop_rate}/${d.drop_gap}/${d.drop_full}`],
["send fail","send_fail",v=>v]];
async function tick(){
 try{
  let d=await (await fetch("/api/status")).json();
  document.getElementById("name").textContent=d.name;
  document.getElementById("t").innerHTML=rows.map(([k,f,fn])=>{
   let v=f[0]==="_"?fn(d):fn(d[f]);
   let cls=(f==="reset"&&d.reset!=="clean")||(f==="send_fail"&&d.send_fail>0)?" class=bad":"";
   return `<tr><td>${k}</td><td${cls}>${v}</td></tr>`}).join("");
 }catch(e){}
}
tick();setInterval(tick,3000);
</script>
)HTML";

void BotWebUI::begin() {
  if (s_started) return;

  s_server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth_ok(r)) return;
    r->send(200, "text/html", PAGE);
  });
  s_server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth_ok(r)) return;
    String j; status_json(j);
    r->send(200, "application/json", j);
  });
  s_server.on("/api/nodes", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!auth_ok(r)) return;
    String j; nodes_json(j);
    r->send(200, "application/json", j);
  });

  AsyncElegantOTA.begin(&s_server, BOT_WEB_USER, BOT_WEB_PASS);
  s_server.begin();
  s_started = true;
  BOT_DEBUG_PRINTLN("web UI on http://%s/", WiFi.localIP().toString().c_str());
}

bool BotWebUI::started() { return s_started; }
