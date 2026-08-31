# companion_radio_bot

MeshCore **companion radio firmware with an on-device command engine** for the
Heltec V4 (ESP32-S3). It answers mesh commands (`ping`, `path`, …) directly on
the device — no host computer — while keeping the standard companion protocol
available over TCP/BLE/USB for a phone app.

This is a downstream project of [MeshCore](https://github.com/meshcore-dev/MeshCore).
It reuses `examples/companion_radio/*` **unchanged** and substitutes a `MyMesh`
subclass (`BotMesh`) for the global `the_mesh` via two build flags, so the delta
against upstream is tiny and rebases cleanly.

## Upstream delta

| File | Change |
|---|---|
| `examples/companion_radio/MyMesh.h` | `extern MyMesh the_mesh;` → guarded `MESH_CLASS` macro (defaults to `MyMesh`) |
| `examples/companion_radio/main.cpp` | optional `#include MESH_CLASS_HEADER`; `MyMesh the_mesh` → `MESH_CLASS the_mesh` |

Both are upstream-friendly and could be PR'd.

## Layout

```
BotMesh.{h,cpp}       MyMesh subclass: hooks onChannelMessageRecv / onMessageRecv,
                      calls base first, splits "<sender>: <body>" for channels,
                      loop guard (ignores own name), gate() = channel allow-list
                      + rate limiter, then runs the matched command. botReply()
                      sends over the mesh (channel or DM).
CommandRouter.{h,cpp} Compile-time command table + match() (no dynamic plugins).
RateLimiter.{h,cpp}   Token buckets: global / per-channel / per-sender. A reply
                      needs a token in all three; keyFromName() hashes channel
                      senders (DMs key on pubkey prefix).
BotConfig.h           All compile-time knobs (rates, allow-list, prefix, version).
BotMsg.h              Normalised inbound message (carries routing path).
BotDebug.h            `BOT_DEBUG_PRINTLN`, gated on -D BOT_DEBUG=1.
commands/
  Command.{h,cpp}     Base class: matchesKeyword() + args() parsing.
  PingCommand.h       ping -> pong
  VersionCommand.h    version -> bot/firmware version + uptime
  TestCommand.h       test / t -> SNR + hop count
  HelloCommand.h      hello / hi / hey
  DiceCommand.h       dice [NdM(+K)]
  RollCommand.h       roll [N]
  Magic8Command.h     magic8 / 8ball
  CmdCommand.h        cmd -> keyword list
  HelpCommand.h       help [cmd]
```

## Identity

The build sets `-D ADVERT_NAME='"D4rl3k"'`. On a device with an existing
identity file the name change applies but the **keypair is kept**. For a clean
throwaway identity, `esptool erase-flash` before the first upload — first boot
then generates a new keypair and adopts the name.

## Build

```
# set WIFI_SSID / WIFI_PWD in variants/heltec_v4/platformio.ini or platformio.local.ini
pio run -e heltec_v4_companion_radio_bot
pio run -e heltec_v4_companion_radio_bot -t upload
pio device monitor -b 115200      # "Bot: ..." debug lines
```

## Status

Phase 1 + 2:
- Command router, multi-class rate limiter, channel allow-list, loop guard.
- `BotStore`: persistent node/location cache (`onDiscoveredContact` -> RAM array,
  flushed to `/botcache.bin` on SPIFFS) that outlives the 350-contact companion
  table + eviction. Leaderboard records in NVS.
- `PathDistance`: hop-hash -> cached node, recency+geo disambiguation, haversine
  sum sender -> hops -> self.
- Commands: ping version test/t **path/p/decode/route** **prefix** **leaderboard/lb**
  **stats/status** hello dice roll magic8 cmd help.

Not yet: deferred/chunked TX queue, trace/tracer/multitest/neighbors, almanac,
internet commands, web config, serial-observability fix. See the project plan.
