#pragma once

#include "BotMsg.h"

class BotMesh;
class Command;

// Compile-time registry + dispatcher for bot commands. The command table is
// fixed at build time (see CommandRouter.cpp); there is no dynamic plugin
// loading on-device.
class CommandRouter {
public:
  void begin();

  // Try to handle `msg`. Returns true if a command matched and ran.
  bool dispatch(BotMesh& mesh, const BotMsg& msg);

  int count() const { return _count; }
  Command* at(int i) const { return (i >= 0 && i < _count) ? _cmds[i] : nullptr; }

private:
  static const int MAX_COMMANDS = 48;
  Command* _cmds[MAX_COMMANDS];
  int _count = 0;
};
