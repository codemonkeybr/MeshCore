#include "BotMesh.h"
#include "BotConfig.h"
#include "BotDebug.h"
#include "GeoUtil.h"
#include "commands/Command.h"
#include <string.h>
#include <esp_system.h>
#include <esp_task_wdt.h>

#ifndef BOT_ENABLED
#define BOT_ENABLED 1
#endif
#ifndef BOT_OUTQ_SIZE
#define BOT_OUTQ_SIZE 4
#endif

// Deferred reply queue — file-static so it does not sit inside the_mesh, and so
// sends never happen from within the mesh RX callback.
struct PendingReply {
  bool     used = false;
  bool     is_channel = false;
  uint8_t  channel_idx = 0xFF;
  uint8_t  to_pubkey[6] = {0};
  char     text[BOT_MAX_REPLY_LEN + 1] = {0};
};
static PendingReply s_outq[BOT_OUTQ_SIZE];
static unsigned long s_last_tx_ms = 0;

static const char* reset_reason_str(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_PANIC:    return "PANIC";
    case ESP_RST_INT_WDT:  return "INT_WDT";
    case ESP_RST_TASK_WDT: return "TASK_WDT";
    case ESP_RST_WDT:      return "WDT";
    case ESP_RST_BROWNOUT: return "BROWNOUT";
    default:               return nullptr;   // POWERON / SW / etc. -> not noteworthy
  }
}

static esp_reset_reason_t s_reset = ESP_RST_UNKNOWN;
static bool s_crash_announced = false;

void BotMesh::begin(bool has_display) {
  MyMesh::begin(has_display);
  _boot_ms = millis();
#if BOT_ENABLED
  s_reset = esp_reset_reason();
  s_crash_announced = (reset_reason_str(s_reset) == nullptr);
  _router.begin();
  _rl.begin();
  _store.begin();
  for (auto& e : s_outq) e.used = false;
  BOT_DEBUG_PRINTLN("BotMesh ready as '%s' (%d cmds, %d cached nodes, reset=%d)",
                    getNodeName(), _router.count(), _store.count(), (int)s_reset);
#else
  BOT_DEBUG_PRINTLN("BotMesh compiled but BOT_ENABLED=0");
#endif
}

const char* BotMesh::lastResetReason() { return reset_reason_str(s_reset); }

void BotMesh::loop() {
  MyMesh::loop();
#if BOT_ENABLED
  // Once the mesh has settled, announce a recovery from a crash so it's visible
  // on the mesh even without a serial console.
  if (!s_crash_announced && uptimeSecs() >= 15) {
    s_crash_announced = true;
    for (int i = 0; i < 40; i++) {
      ChannelDetails ch;
      if (getChannel(i, ch) && ch.name[0]) {
        for (auto& e : s_outq) {
          if (e.used) continue;
          e.is_channel = true; e.channel_idx = (uint8_t)i;
          snprintf(e.text, sizeof(e.text), "%s recovered (%s)", getNodeName(),
                   reset_reason_str(s_reset));
          e.used = true;
          break;
        }
        break;
      }
    }
  }
  drainOutQueue();
  _store.loop(nowEpoch());
#endif
}

// ---- location helpers ---------------------------------------------------

bool BotMesh::selfLocation(double& lat, double& lon) {
  NodePrefs* p = getNodePrefs();
  if (p && (p->node_lat != 0.0 || p->node_lon != 0.0)) {
    lat = p->node_lat; lon = p->node_lon; return true;
  }
  return false;
}

bool BotMesh::senderLocation(const BotMsg& msg, double& lat, double& lon) {
  if (!msg.is_channel && msg.contact && (msg.contact->gps_lat || msg.contact->gps_lon)) {
    lat = e6_to_deg(msg.contact->gps_lat); lon = e6_to_deg(msg.contact->gps_lon);
    return true;
  }
  const NodeRec* r = _store.findByName(msg.sender_name);
  if (r && r->hasLoc()) { lat = e6_to_deg(r->lat_e6); lon = e6_to_deg(r->lon_e6); return true; }
  return false;
}

// ---- advert capture ---------------------------------------------------

void BotMesh::onDiscoveredContact(ContactInfo& contact, bool is_new,
                                  uint8_t path_len, const uint8_t* path) {
  MyMesh::onDiscoveredContact(contact, is_new, path_len, path);
#if BOT_ENABLED
  bool has_loc = (contact.gps_lat != 0 || contact.gps_lon != 0);
  _store.noteNode(contact.id.pub_key, contact.name, contact.type,
                  contact.gps_lat, contact.gps_lon, has_loc,
                  contact.last_advert_timestamp);
#endif
}

// ---- gating ---------------------------------------------------------

bool BotMesh::channelAllowed(uint8_t channel_idx) {
  const char* allow = BOT_CHANNEL_ALLOWLIST;
  if (!allow || !*allow) return true;

  ChannelDetails ch;
  if (!getChannel(channel_idx, ch)) return false;

  const char* p = allow;
  size_t nlen = strlen(ch.name);
  while (*p) {
    const char* comma = strchr(p, ',');
    size_t seglen = comma ? (size_t)(comma - p) : strlen(p);
    if (seglen == nlen && strncmp(p, ch.name, seglen) == 0) return true;
    if (!comma) break;
    p = comma + 1;
  }
  return false;
}

bool BotMesh::gate(const BotMsg& msg, const uint8_t rl_key[4]) {
  if (msg.is_channel) {
    if (!channelAllowed(msg.channel_idx)) { BOT_DEBUG_PRINTLN("gate: ch %u blocked", msg.channel_idx); return false; }
  } else if (!BOT_ANSWER_DM) {
    return false;
  }
  if (!_rl.allow(rl_key, msg.is_channel ? msg.channel_idx : 0xFF, millis())) {
    stats.dropped_rate++;
    BOT_DEBUG_PRINTLN("gate: rate limited");
    return false;
  }
  return true;
}

// ---- dispatch ---------------------------------------------------

void BotMesh::handle(const BotMsg& msg, const uint8_t rl_key[4]) {
  stats.msgs++;
  Command* cmd = _router.match(msg);
  if (!cmd) return;
  stats.matched++;
  if (!gate(msg, rl_key)) return;
  BOT_DEBUG_PRINTLN("run '%s' <- %s (%s)", cmd->keyword(),
                    msg.sender_name ? msg.sender_name : "?",
                    msg.is_channel ? "ch" : "dm");
  cmd->run(*this, msg);
}

void BotMesh::onChannelMessageRecv(const mesh::GroupChannel& channel, mesh::Packet* pkt,
                                   uint32_t timestamp, const char* text) {
  MyMesh::onChannelMessageRecv(channel, pkt, timestamp, text);
#if BOT_ENABLED
  strncpy(_scratch, text ? text : "", sizeof(_scratch) - 1);
  _scratch[sizeof(_scratch) - 1] = '\0';
  const char* sender = "?";
  const char* body = _scratch;
  char* sep = strstr(_scratch, ": ");
  if (sep) { *sep = '\0'; sender = _scratch; body = sep + 2; }

  if (strcmp(sender, getNodeName()) == 0) return;   // loop guard

  BotMsg m;
  m.is_channel = true;
  m.text = body;
  m.sender_name = sender;
  m.sender_timestamp = timestamp;
  m.channel = &channel;
  m.channel_idx = (uint8_t)findChannelIdx(channel);
  m.snr = pkt->getSNR();
  if (pkt->isRouteFlood()) {
    m.is_flood = true;
    m.path_hash_len = pkt->getPathHashSize();
    m.path_hops = pkt->getPathHashCount();
    m.path = pkt->path;
  }

  uint8_t key[4];
  RateLimiter::keyFromName(sender, key);
  handle(m, key);
#endif
}

void BotMesh::onMessageRecv(const ContactInfo& from, mesh::Packet* pkt,
                            uint32_t sender_timestamp, const char* text) {
  MyMesh::onMessageRecv(from, pkt, sender_timestamp, text);
#if BOT_ENABLED
  BotMsg m;
  m.is_channel = false;
  m.text = text;
  m.sender_name = from.name;
  m.sender_timestamp = sender_timestamp;
  m.contact = &from;
  m.snr = pkt->getSNR();
  if (pkt->isRouteFlood()) {
    m.is_flood = true;
    m.path_hash_len = pkt->getPathHashSize();
    m.path_hops = pkt->getPathHashCount();
    m.path = pkt->path;
  }

  uint8_t key[4];
  memcpy(key, from.id.pub_key, 4);
  handle(m, key);
#endif
}

// ---- reply: queue now, send from loop() -------------------------------

void BotMesh::botReply(const BotMsg& msg, const char* text) {
  if (!text || !*text) return;

  for (auto& e : s_outq) {
    if (e.used) continue;
    e.is_channel = msg.is_channel;
    e.channel_idx = msg.channel_idx;
    memset(e.to_pubkey, 0, sizeof(e.to_pubkey));
    if (!msg.is_channel && msg.contact) memcpy(e.to_pubkey, msg.contact->id.pub_key, 6);
    strncpy(e.text, text, BOT_MAX_REPLY_LEN);
    e.text[BOT_MAX_REPLY_LEN] = '\0';
    e.used = true;
    return;
  }
  stats.dropped_full++;
  BOT_DEBUG_PRINTLN("reply dropped (outq full): '%s'", text);
}

void BotMesh::drainOutQueue() {
  unsigned long now = millis();
  if (s_last_tx_ms != 0 && (now - s_last_tx_ms) < BOT_MIN_TX_GAP_MS) return;

  for (auto& e : s_outq) {
    if (!e.used) continue;

    bool sent = false;
    if (e.is_channel) {
      ChannelDetails ch;
      if (getChannel(e.channel_idx, ch)) {
        sent = sendGroupMessage(getRTCClock()->getCurrentTimeUnique(), ch.channel,
                                getNodeName(), e.text, (int)strlen(e.text));
      }
    } else {
      ContactInfo* c = lookupContactByPubKey(e.to_pubkey, 6);
      if (c) {
        uint32_t ack = 0, tmo = 0;
        int r = sendMessage(*c, getRTCClock()->getCurrentTimeUnique(), 0, e.text, ack, tmo);
        sent = (r != MSG_SEND_FAILED);
      }
    }

    if (sent) { stats.replies++; s_last_tx_ms = now; BOT_DEBUG_PRINTLN("tx (%s): '%s'", e.is_channel ? "ch" : "dm", e.text); }
    else      { stats.send_fail++; BOT_DEBUG_PRINTLN("tx FAILED (%s): '%s'", e.is_channel ? "ch" : "dm", e.text); }

    e.used = false;
    return;   // one send per loop pass
  }
}
