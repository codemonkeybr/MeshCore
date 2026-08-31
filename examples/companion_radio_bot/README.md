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
                      calls base then feeds the CommandRouter; botReply() sender.
CommandRouter.{h,cpp} Compile-time command table + dispatcher (no dynamic plugins).
BotMsg.h              Normalised inbound message handed to commands.
BotDebug.h            `BOT_DEBUG_PRINTLN`, gated on -D BOT_DEBUG=1.
commands/
  Command.{h,cpp}     Base class + keyword matching / arg parsing.
  PingCommand.h       Round-trip test command.
```

## Build

```
# set WIFI_SSID / WIFI_PWD in variants/heltec_v4/platformio.ini or platformio.local.ini
pio run -e heltec_v4_companion_radio_bot
pio run -e heltec_v4_companion_radio_bot -t upload
pio device monitor -b 115200      # "Bot: ..." debug lines
```

## Status

Phase 1 spike. `ping` round-trips; coarse single inter-reply gap only. Not yet:
multi-class rate limiter, channel allow-list, persistent repeater/location cache,
`path` with distance. See the project plan for the phase breakdown.
