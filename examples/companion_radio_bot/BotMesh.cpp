#include "BotMesh.h"
#include "BotConfig.h"
#include "BotDebug.h"
#include "commands/Command.h"
#include <string.h>

#ifndef BOT_ENABLED
#define BOT_ENABLED 1
#endif

// Absolute floor between two bot transmissions, independent of the token
// buckets. Guards against a single command calling botReply() in a tight loop.
#ifndef BOT_MIN_TX_GAP_MS
#define BOT_MIN_TX_GAP_MS 1200
#endif

static unsigned long s_last_tx_ms = 0;

void BotMesh::begin(bool has_display) {
  MyMesh::begin(has_display);
  _boot_ms = millis();
#if BOT_ENABLED
  _router.begin();
  _rl.begin();
  BOT_DEBUG_PRINTLN("BotMesh ready as '%s' (%d commands)", getNodeName(), _router.count());
#else
  BOT_DEBUG_PRINTLN("BotMesh compiled but BOT_ENABLED=0");
#endif
}

bool BotMesh::channelAllowed(uint8_t channel_idx) {
  const char* allow = BOT_CHANNEL_ALLOWLIST;
  if (!allow || !*allow) return true;   // empty list = any channel

  ChannelDetails ch;
  if (!getChannel(channel_idx, ch)) return false;

  // match ch.name against a comma-separated list
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
    if (!channelAllowed(msg.channel_idx)) {
      BOT_DEBUG_PRINTLN("gate: channel %u not allowed", msg.channel_idx);
      return false;
    }
  } else if (!BOT_ANSWER_DM) {
    BOT_DEBUG_PRINTLN("gate: DM answering disabled");
    return false;
  }

  unsigned long now = millis();
  if (!_rl.allow(rl_key, msg.is_channel ? msg.channel_idx : 0xFF, now)) {
    BOT_DEBUG_PRINTLN("gate: rate limited");
    return false;
  }
  return true;
}

void BotMesh::onChannelMessageRecv(const mesh::GroupChannel& channel, mesh::Packet* pkt,
                                   uint32_t timestamp, const char* text) {
  MyMesh::onChannelMessageRecv(channel, pkt, timestamp, text);
#if BOT_ENABLED
  _msg_count++;

  // Channel text is "<sender>: <body>". Split into _scratch.
  strncpy(_scratch, text ? text : "", sizeof(_scratch) - 1);
  _scratch[sizeof(_scratch) - 1] = '\0';
  const char* sender = "?";
  const char* body = _scratch;
  char* sep = strstr(_scratch, ": ");
  if (sep) {
    *sep = '\0';
    sender = _scratch;
    body = sep + 2;
  }

  // Ignore our own messages echoed back onto the channel (loop guard).
  if (strcmp(sender, getNodeName()) == 0) return;

  BotMsg m;
  m.is_channel = true;
  m.text = body;
  m.sender_name = sender;
  m.sender_timestamp = timestamp;
  m.channel = &channel;
  m.channel_idx = (uint8_t)findChannelIdx(channel);
  m.snr = pkt->getSNR();
  if (pkt->isRouteFlood()) { m.path_len = pkt->path_len; m.path = pkt->path; }

  Command* cmd = _router.match(m);
  if (!cmd) return;

  uint8_t key[4];
  RateLimiter::keyFromName(sender, key);
  if (!gate(m, key)) return;

  BOT_DEBUG_PRINTLN("run '%s' <- %s @ch%u", cmd->keyword(), sender, m.channel_idx);
  cmd->run(*this, m);
#endif
}

void BotMesh::onMessageRecv(const ContactInfo& from, mesh::Packet* pkt,
                            uint32_t sender_timestamp, const char* text) {
  MyMesh::onMessageRecv(from, pkt, sender_timestamp, text);
#if BOT_ENABLED
  _msg_count++;

  BotMsg m;
  m.is_channel = false;
  m.text = text;
  m.sender_name = from.name;
  m.sender_timestamp = sender_timestamp;
  m.contact = &from;
  m.snr = pkt->getSNR();
  if (pkt->isRouteFlood()) { m.path_len = pkt->path_len; m.path = pkt->path; }

  Command* cmd = _router.match(m);
  if (!cmd) return;

  uint8_t key[4];
  memcpy(key, from.id.pub_key, 4);
  if (!gate(m, key)) return;

  BOT_DEBUG_PRINTLN("run '%s' <- %s (dm)", cmd->keyword(), from.name);
  cmd->run(*this, m);
#endif
}

void BotMesh::botReply(const BotMsg& msg, const char* text) {
  if (!text || !*text) return;

  // Runs inside the mesh RX callback — never block here. If we're inside the
  // absolute min-gap, drop the reply rather than stall packet processing.
  // (Phase 2's chunked sender introduces a proper deferred TX queue.)
  unsigned long now = millis();
  if (s_last_tx_ms != 0 && (now - s_last_tx_ms) < BOT_MIN_TX_GAP_MS) {
    BOT_DEBUG_PRINTLN("reply dropped (min tx gap): '%s'", text);
    return;
  }

  char buf[BOT_MAX_REPLY_LEN + 1];
  strncpy(buf, text, BOT_MAX_REPLY_LEN);
  buf[BOT_MAX_REPLY_LEN] = '\0';

  bool sent = false;
  if (msg.is_channel) {
    ChannelDetails ch;
    if (getChannel(msg.channel_idx, ch)) {
      sent = sendGroupMessage(getRTCClock()->getCurrentTimeUnique(), ch.channel,
                              getNodeName(), buf, (int)strlen(buf));
    }
  } else if (msg.contact) {
    uint32_t expected_ack = 0, est_timeout = 0;
    int r = sendMessage(*msg.contact, getRTCClock()->getCurrentTimeUnique(), 0,
                        buf, expected_ack, est_timeout);
    sent = (r != MSG_SEND_FAILED);
  }

  if (sent) {
    s_last_tx_ms = now;
    _reply_count++;
    BOT_DEBUG_PRINTLN("reply (%s): '%s'", msg.is_channel ? "ch" : "dm", buf);
  } else {
    BOT_DEBUG_PRINTLN("reply FAILED (%s): '%s'", msg.is_channel ? "ch" : "dm", buf);
  }
}
