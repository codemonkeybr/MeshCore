#include "BotMesh.h"
#include "BotDebug.h"
#include <string.h>

#ifndef BOT_ENABLED
#define BOT_ENABLED 1
#endif

// Coarse floor on how often the bot will transmit a reply, regardless of source.
// A proper multi-class rate limiter (global / per-sender / per-channel) replaces
// this in Phase 1.
#ifndef BOT_MIN_REPLY_GAP_MS
#define BOT_MIN_REPLY_GAP_MS 4000
#endif

// Longest reply the bot will emit for now (single mesh text packet). Chunked
// multi-part replies come with the path command in Phase 2.
#ifndef BOT_MAX_REPLY_LEN
#define BOT_MAX_REPLY_LEN 140
#endif

void BotMesh::begin(bool has_display) {
  MyMesh::begin(has_display);
#if BOT_ENABLED
  _router.begin();
  BOT_DEBUG_PRINTLN("BotMesh ready as '%s'", getNodeName());
#else
  BOT_DEBUG_PRINTLN("BotMesh compiled but BOT_ENABLED=0");
#endif
}

void BotMesh::onChannelMessageRecv(const mesh::GroupChannel& channel, mesh::Packet* pkt,
                                   uint32_t timestamp, const char* text) {
  MyMesh::onChannelMessageRecv(channel, pkt, timestamp, text);
#if BOT_ENABLED
  BotMsg m;
  m.is_channel = true;
  m.text = text;
  m.sender_timestamp = timestamp;
  m.channel = &channel;
  m.channel_idx = (uint8_t)findChannelIdx(channel);
  m.snr = pkt->getSNR();
  if (pkt->isRouteFlood()) {
    m.path_len = pkt->path_len;
    m.path = pkt->path;
  }
  _router.dispatch(*this, m);
#endif
}

void BotMesh::onMessageRecv(const ContactInfo& from, mesh::Packet* pkt,
                            uint32_t sender_timestamp, const char* text) {
  MyMesh::onMessageRecv(from, pkt, sender_timestamp, text);
#if BOT_ENABLED
  BotMsg m;
  m.is_channel = false;
  m.text = text;
  m.sender_timestamp = sender_timestamp;
  m.contact = &from;
  m.snr = pkt->getSNR();
  if (pkt->isRouteFlood()) {
    m.path_len = pkt->path_len;
    m.path = pkt->path;
  }
  _router.dispatch(*this, m);
#endif
}

void BotMesh::botReply(const BotMsg& msg, const char* text) {
  if (!text || !*text) return;

  unsigned long now = millis();
  if (_last_reply_ms != 0 && (now - _last_reply_ms) < BOT_MIN_REPLY_GAP_MS) {
    BOT_DEBUG_PRINTLN("reply suppressed (rate gap): '%s'", text);
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
    _last_reply_ms = now;
    BOT_DEBUG_PRINTLN("reply sent (%s): '%s'", msg.is_channel ? "channel" : "dm", buf);
  } else {
    BOT_DEBUG_PRINTLN("reply FAILED (%s): '%s'", msg.is_channel ? "channel" : "dm", buf);
  }
}
