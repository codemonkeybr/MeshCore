#pragma once

#include <stdint.h>
#include <string.h>

// Where a (possibly deferred) reply should go. Built from a BotMsg by
// BotMesh::targetFor(); carried through the outbound queue and the HttpWorker.
struct ReplyTarget {
  bool     is_channel = false;
  uint8_t  channel_idx = 0xFF;
  uint8_t  to_pubkey[6] = {0};

  void clear() { is_channel = false; channel_idx = 0xFF; memset(to_pubkey, 0, 6); }
};
