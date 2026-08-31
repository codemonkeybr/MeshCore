#include "CommandRouter.h"
#include "BotDebug.h"
#include "commands/Command.h"
#include "commands/PingCommand.h"
#include "commands/VersionCommand.h"
#include "commands/DiceCommand.h"
#include "commands/RollCommand.h"
#include "commands/HelloCommand.h"
#include "commands/Magic8Command.h"
#include "commands/TestCommand.h"
#include "commands/PathCommand.h"
#include "commands/PrefixCommand.h"
#include "commands/LeaderboardCommand.h"
#include "commands/StatsCommand.h"
#include "commands/SunCommand.h"
#include "commands/MoonCommand.h"
#include "commands/WxCommand.h"
#include "commands/AqiCommand.h"
#include "commands/RainCommand.h"
#include "commands/SolarCommand.h"
#include "commands/JokeCommand.h"
#include "commands/DadjokeCommand.h"
#include "commands/CmdCommand.h"
#include "commands/HelpCommand.h"

// ---- Compile-time command table -------------------------------------------
// Order is match priority (first match wins). help/cmd introspect the table.
static PingCommand        s_ping;
static VersionCommand     s_version;
static TestCommand        s_test;
static PathCommand        s_path;
static PrefixCommand      s_prefix;
static LeaderboardCommand s_leaderboard;
static StatsCommand       s_stats;
static SunCommand         s_sun;
static MoonCommand        s_moon;
static WxCommand          s_wx;
static AqiCommand         s_aqi;
static RainCommand        s_rain;
static SolarCommand       s_solar;
static JokeCommand        s_joke;
static DadjokeCommand     s_dadjoke;
static HelloCommand       s_hello;
static DiceCommand        s_dice;
static RollCommand        s_roll;
static Magic8Command      s_magic8;
static CmdCommand         s_cmd;
static HelpCommand        s_help;

static Command* const kCommands[] = {
  &s_ping, &s_version, &s_test,
  &s_path, &s_prefix, &s_leaderboard, &s_stats,
  &s_sun, &s_moon,
  &s_wx, &s_aqi, &s_rain, &s_solar, &s_joke, &s_dadjoke,
  &s_hello, &s_dice, &s_roll, &s_magic8,
  &s_cmd, &s_help,
};
// --------------------------------------------------------------------------

void CommandRouter::begin() {
  _count = 0;
  for (Command* c : kCommands) {
    if (_count < MAX_COMMANDS) _cmds[_count++] = c;
  }
  BOT_DEBUG_PRINTLN("router: %d command(s) registered", _count);
}

Command* CommandRouter::match(const BotMsg& msg) const {
  for (int i = 0; i < _count; i++) {
    if (_cmds[i]->matches(msg)) return _cmds[i];
  }
  return nullptr;
}
