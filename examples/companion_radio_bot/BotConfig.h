#pragma once

// Compile-time bot configuration. Override any of these with -D in the build
// env (variants/heltec_v4/platformio.ini) or platformio.local.ini.

// --- identity -------------------------------------------------------------
// The advert name is set via -D ADVERT_NAME (consumed by MyMesh::begin).

#ifndef BOT_VERSION
#define BOT_VERSION "0.1.0-phase1"
#endif

// --- command parsing -----------------------------------------------------
#ifndef BOT_CMD_PREFIX
#define BOT_CMD_PREFIX ""        // optional prefix, e.g. "!" ; "" = bare keywords
#endif

// --- reply shaping -----------------------------------------------------
#ifndef BOT_MAX_REPLY_LEN
#define BOT_MAX_REPLY_LEN 140    // single mesh text packet for now (Phase 2 chunks)
#endif

// --- rate limiting (token buckets) --------------------------------------
// A reply is sent only if the global, per-channel AND per-sender buckets all
// have a token. Rates are tokens/second; capacity is the burst allowance.
#ifndef BOT_RL_GLOBAL_PER_MIN
#define BOT_RL_GLOBAL_PER_MIN  10      // sustained replies/min across everything
#endif
#ifndef BOT_RL_GLOBAL_BURST
#define BOT_RL_GLOBAL_BURST    3
#endif
#ifndef BOT_RL_CHANNEL_PER_MIN
#define BOT_RL_CHANNEL_PER_MIN 4       // per channel
#endif
#ifndef BOT_RL_CHANNEL_BURST
#define BOT_RL_CHANNEL_BURST   2
#endif
#ifndef BOT_RL_SENDER_PER_MIN
#define BOT_RL_SENDER_PER_MIN  2       // per sender (pubkey for DM, name-hash for channel)
#endif
#ifndef BOT_RL_SENDER_BURST
#define BOT_RL_SENDER_BURST    2
#endif
#ifndef BOT_RL_SENDER_SLOTS
#define BOT_RL_SENDER_SLOTS    16      // tracked senders (LRU)
#endif
#ifndef BOT_RL_CHANNEL_SLOTS
#define BOT_RL_CHANNEL_SLOTS   40      // must be >= MAX_GROUP_CHANNELS
#endif

// --- channel allow-list ------------------------------------------------
// Comma-separated channel names the bot will answer on. Empty = answer on any
// configured channel. Case-sensitive, exact match.
#ifndef BOT_CHANNEL_ALLOWLIST
#define BOT_CHANNEL_ALLOWLIST ""
#endif

// Whether the bot answers direct messages.
#ifndef BOT_ANSWER_DM
#define BOT_ANSWER_DM 1
#endif
