#pragma once

#include <Arduino.h>
#include "ReplyTarget.h"
#include "BotConfig.h"

#ifndef BOT_MAX_REPLY_LEN
#define BOT_MAX_REPLY_LEN 140
#endif

// One outbound HTTP request, submitted by a command. The worker runs it on its
// own task (never in the mesh RX / loop path), formats a reply, and posts a
// HttpResult back.
enum HttpKind : uint8_t {
  HTTP_WX = 1, HTTP_AQI, HTTP_RAIN, HTTP_SOLAR, HTTP_JOKE, HTTP_DADJOKE,
};

struct HttpJob {
  HttpKind    kind;
  ReplyTarget target;
  double      lat = 0, lon = 0;   // for weather kinds
};

struct HttpResult {
  ReplyTarget target;
  char        text[BOT_MAX_REPLY_LEN + 1];
};

// File-static singleton (kept out of the_mesh). One request in flight at a time.
class HttpWorker {
public:
  static HttpWorker& instance();

  void begin();                       // spawn the task + queues
  bool submit(const HttpJob& job);    // false if a request is already queued/running
  bool pollResult(HttpResult& out);   // BotMesh::loop drains this
  bool busy() const;

private:
  void taskLoop();
  static void taskEntry(void* self);
};
