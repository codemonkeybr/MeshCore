#pragma once

// Compile-time bot configuration. Override any of these with -D in the build
// env (variants/heltec_v4/platformio.ini) or platformio.local.ini.

// --- identity -------------------------------------------------------------
// The advert name is set via -D ADVERT_NAME (consumed by MyMesh::begin).

#ifndef BOT_VERSION
#define BOT_VERSION "0.3.0"
#endif

// Local time offset from UTC, in minutes, for almanac output (sun/moon).
// e.g. -480 = PST, -420 = PDT, 60 = CET. The device clock itself stays UTC.
#ifndef BOT_TZ_OFFSET_MIN
#define BOT_TZ_OFFSET_MIN 0
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
#define BOT_MIN_TX_GAP_MS 2500   // absolute floor between two bot transmits (airtime courtesy)
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
#define BOT_RL_GLOBAL_PER_MIN  6       // sustained replies/min across everything
#endif
#ifndef BOT_RL_GLOBAL_BURST
#define BOT_RL_GLOBAL_BURST    3
#endif
#ifndef BOT_RL_CHANNEL_PER_MIN
#define BOT_RL_CHANNEL_PER_MIN 3       // per channel
#endif
#ifndef BOT_RL_CHANNEL_BURST
#define BOT_RL_CHANNEL_BURST   2
#endif
#ifndef BOT_RL_SENDER_PER_MIN
#define BOT_RL_SENDER_PER_MIN  3       // per sender (pubkey for DM, name-hash for channel)
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

// --- internet commands (HttpWorker) ----------------------------------
#ifndef BOT_HTTP_INSECURE
#define BOT_HTTP_INSECURE 1        // skip TLS cert validation (public read-only APIs)
#endif
#ifndef BOT_HTTP_TIMEOUT_MS
#define BOT_HTTP_TIMEOUT_MS 9000
#endif
#ifndef BOT_HTTP_TASK_STACK
#define BOT_HTTP_TASK_STACK 16384
#endif
#ifndef BOT_WX_METRIC
#define BOT_WX_METRIC 1           // 1 = C/km/h, 0 = F/mph
#endif

// Compile-time fallback location (used by sun/moon/wx when the node prefs have
// no location). Set via [bot_secrets].lat / .lon.
#ifndef BOT_LAT
#define BOT_LAT 0.0
#endif
#ifndef BOT_LON
#define BOT_LON 0.0
#endif
