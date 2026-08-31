#pragma once

#include "../BotMsg.h"
#include <stddef.h>

class BotMesh;

// Base class for an on-device bot command. Concrete commands are registered
// (compile-time) in CommandRouter. Keep run() short and non-blocking: anything
// that needs the network must be handed to the HttpWorker (added in Phase 4),
// not run inline in the mesh RX callback.
class Command {
public:
  virtual ~Command() {}

  // Lowercase keyword with no command prefix, e.g. "ping".
  virtual const char* keyword() const = 0;

  // One-line usage string for `help`.
  virtual const char* help() const { return ""; }

  // Whether this command handles `msg`. Default: the first token of msg.text
  // (after an optional BOT_CMD_PREFIX) equals keyword(), case-insensitively.
  // Override to accept aliases via matchesKeyword().
  virtual bool matches(const BotMsg& msg) const { return matchesKeyword(msg, keyword()); }

  // Execute. Respond via mesh.botReply(msg, ...). Must not block.
  virtual void run(BotMesh& mesh, const BotMsg& msg) = 0;

protected:
  // True if the first whitespace-delimited token of msg.text (after an optional
  // BOT_CMD_PREFIX) case-insensitively equals kw.
  static bool matchesKeyword(const BotMsg& msg, const char* kw);

  // Copy the argument portion of msg.text (everything after the first token)
  // into out[0..out_sz). Returns the trimmed length. out is NUL-terminated.
  static size_t args(const BotMsg& msg, char* out, size_t out_sz);
};
