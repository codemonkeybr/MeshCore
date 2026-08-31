#pragma once

#include "BotMsg.h"

class BotMesh;
class Command;

// Compile-time registry + matcher for bot commands. The command table is fixed
// at build time (see CommandRouter.cpp); there is no dynamic plugin loading.
class CommandRouter {
public:
  void begin();

  // Return the first command whose matches() accepts `msg`, or nullptr.
  // Does not execute — the caller applies rate limiting, then calls run().
  Command* match(const BotMsg& msg) const;

  int count() const { return _count; }
  Command* at(int i) const { return (i >= 0 && i < _count) ? _cmds[i] : nullptr; }

private:
  static const int MAX_COMMANDS = 48;
  Command* _cmds[MAX_COMMANDS];
  int _count = 0;
};
