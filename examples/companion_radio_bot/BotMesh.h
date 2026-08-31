#pragma once

#include "MyMesh.h"
#include "CommandRouter.h"
#include "BotMsg.h"

// BotMesh extends the stock companion-radio MyMesh with an on-device command
// engine. It is substituted for `the_mesh` via -D MESH_CLASS=BotMesh /
// -D MESH_CLASS_HEADER, so the base companion firmware (protocol over
// TCP/BLE/USB, offline queue, UI) is untouched and keeps working.
//
// The message hooks call their MyMesh base first (so a connected phone app
// still sees all traffic), then feed a normalised BotMsg to the CommandRouter.
class BotMesh : public MyMesh {
public:
  BotMesh(mesh::Radio& radio, mesh::RNG& rng, mesh::RTCClock& rtc,
          SimpleMeshTables& tables, DataStore& store, AbstractUITask* ui = nullptr)
    : MyMesh(radio, rng, rtc, tables, store, ui) {}

  // Hides MyMesh::begin (non-virtual). Called on the concrete BotMesh object
  // from main.cpp, so the correct overload is selected.
  void begin(bool has_display);

  // Send `text` back to the origin of `msg` (its channel, or the sender as a
  // DM). Applies a coarse minimum inter-reply gap. Safe to call from a hook.
  void botReply(const BotMsg& msg, const char* text);

  CommandRouter& router() { return _router; }

protected:
  void onChannelMessageRecv(const mesh::GroupChannel& channel, mesh::Packet* pkt,
                            uint32_t timestamp, const char* text) override;
  void onMessageRecv(const ContactInfo& from, mesh::Packet* pkt,
                     uint32_t sender_timestamp, const char* text) override;

private:
  CommandRouter _router;
  unsigned long _last_reply_ms = 0;
};
