#pragma once

#include "MyMesh.h"
#include "CommandRouter.h"
#include "RateLimiter.h"
#include "BotStore.h"
#include "BotMsg.h"
#include "ReplyTarget.h"

// BotMesh extends the stock companion-radio MyMesh with an on-device command
// engine. It is substituted for `the_mesh` via -D MESH_CLASS=BotMesh /
// -D MESH_CLASS_HEADER, so the base companion firmware (protocol over
// TCP/BLE/USB, offline queue, UI) is untouched and keeps working.
//
// The message hooks call their MyMesh base first (so a connected phone app
// still sees all traffic), then feed a normalised BotMsg to the CommandRouter.
// Replies are queued and actually transmitted from loop(), never from inside
// the RX callback.
class BotMesh : public MyMesh {
public:
  BotMesh(mesh::Radio& radio, mesh::RNG& rng, mesh::RTCClock& rtc,
          SimpleMeshTables& tables, DataStore& store, AbstractUITask* ui = nullptr)
    : MyMesh(radio, rng, rtc, tables, store, ui) {}

  void begin(bool has_display);     // hides MyMesh::begin (non-virtual)
  void loop();                      // hides MyMesh::loop

  // Queue `text` to be sent back to the origin of `msg` from the next loop().
  void botReply(const BotMsg& msg, const char* text);

  // Reply routing for a (possibly deferred, e.g. HTTP) response.
  ReplyTarget targetFor(const BotMsg& msg) const;
  void enqueueReply(const ReplyTarget& t, const char* text);

  CommandRouter& router() { return _router; }
  BotStore& store() { return _store; }

  uint32_t uptimeSecs() const { return (millis() - _boot_ms) / 1000; }
  uint32_t nowEpoch() { return getRTCClock()->getCurrentTime(); }
  static const char* lastResetReason();   // nullptr if a clean boot
  static bool clockSynced();               // true once NTP has set the clock
  float lastRssi() const { return _last_rssi; }
  bool selfLocation(double& lat, double& lon);
  bool senderLocation(const BotMsg& msg, double& lat, double& lon);

  // Best-known route from a DM contact to us, from their most recently heard
  // advert (not necessarily the route this specific message took). `out_path`
  // must be at least `max_path_bytes` bytes; false if there's nothing cached.
  bool lastKnownPath(const BotMsg& msg, uint8_t* out_path, uint8_t max_path_bytes,
                     uint8_t& hops, uint8_t& hash_len, uint32_t& age_secs);

  struct Stats { uint32_t msgs, matched, replies, dropped_rate, dropped_gap, dropped_full, send_fail; } stats{};

protected:
  void onChannelMessageRecv(const mesh::GroupChannel& channel, mesh::Packet* pkt,
                            uint32_t timestamp, const char* text) override;
  void onMessageRecv(const ContactInfo& from, mesh::Packet* pkt,
                     uint32_t sender_timestamp, const char* text) override;
  void onDiscoveredContact(ContactInfo& contact, bool is_new,
                           uint8_t path_len, const uint8_t* path) override;
  void logRxRaw(float snr, float rssi, const uint8_t raw[], int len) override;

private:
  bool gate(const BotMsg& msg, const uint8_t rl_key[4]);
  bool channelAllowed(uint8_t channel_idx);
  void handle(const BotMsg& msg, const uint8_t rl_key[4]);
  void drainOutQueue();

  CommandRouter _router;
  RateLimiter _rl;
  BotStore _store;
  unsigned long _boot_ms = 0;
  float _last_rssi = 0.0f;
  float _last_snr = 0.0f;

  char _scratch[192];   // splits "<sender>: <body>" for channel messages
};
