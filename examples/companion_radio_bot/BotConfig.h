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
#ifndef BOT_MIN_TX_GAP_MS
#define BOT_MIN_TX_GAP_MS 900    // absolute floor between two bot transmits
#endif

// --- location cache (BotStore) ---------------------------------------
#ifndef BOT_LOCCACHE_MAX
#define BOT_LOCCACHE_MAX 400     // ~36 B/entry in RAM; persisted to SPIFFS
#endif
#ifndef BOT_LOCCACHE_FLUSH_SEC
#define BOT_LOCCACHE_FLUSH_SEC 120
#endif
#ifndef BOT_PATH_MAX_HOP_KM
#define BOT_PATH_MAX_HOP_KM 180  // geo-plausibility limit for disambiguating a hop
#endif
#ifndef BOT_PATH_SHOW_DISTANCE
#define BOT_PATH_SHOW_DISTANCE 1
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
#define BOT_RL_SENDER_PER_MIN  4       // per sender (pubkey for DM, name-hash for channel)
#endif
#ifndef BOT_RL_SENDER_BURST
#define BOT_RL_SENDER_BURST    3
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
