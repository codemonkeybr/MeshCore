#include "CommandRouter.h"
#include "BotDebug.h"
#include "commands/Command.h"
#include "commands/PingCommand.h"

// ---- Compile-time command table -------------------------------------------
// Add new commands here. Order is match priority (first match wins).
static PingCommand s_ping;

static Command* const kCommands[] = {
  &s_ping,
};
// --------------------------------------------------------------------------

void CommandRouter::begin() {
  _count = 0;
  for (Command* c : kCommands) {
    if (_count < MAX_COMMANDS) _cmds[_count++] = c;
  }
  BOT_DEBUG_PRINTLN("router: %d command(s) registered", _count);
}

bool CommandRouter::dispatch(BotMesh& mesh, const BotMsg& msg) {
  for (int i = 0; i < _count; i++) {
    if (_cmds[i]->matches(msg)) {
      BOT_DEBUG_PRINTLN("dispatch: '%s' (%s)", _cmds[i]->keyword(),
                        msg.is_channel ? "channel" : "dm");
      _cmds[i]->run(mesh, msg);
      return true;
    }
  }
  return false;
}
