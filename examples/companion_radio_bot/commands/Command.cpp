#include "Command.h"
#include "../BotConfig.h"
#include <string.h>
#include <ctype.h>

static const char* skip_ws(const char* p) {
  while (*p == ' ' || *p == '\t') p++;
  return p;
}

// Advance past an optional leading command prefix (BOT_CMD_PREFIX).
static const char* skip_prefix(const char* p) {
  const size_t plen = sizeof(BOT_CMD_PREFIX) - 1;
  if (plen && strncmp(p, BOT_CMD_PREFIX, plen) == 0) p += plen;
  return p;
}

bool Command::matchesKeyword(const BotMsg& msg, const char* kw) {
  if (!msg.text) return false;
  const char* p = skip_prefix(skip_ws(msg.text));

  const size_t kwlen = strlen(kw);
  for (size_t i = 0; i < kwlen; i++) {
    if (tolower((unsigned char)p[i]) != tolower((unsigned char)kw[i])) return false;
  }
  const char after = p[kwlen];
  return after == '\0' || after == ' ' || after == '\t' ||
         after == '\n' || after == '\r';
}

size_t Command::args(const BotMsg& msg, char* out, size_t out_sz) {
  out[0] = '\0';
  if (!msg.text || out_sz == 0) return 0;

  const char* p = skip_prefix(skip_ws(msg.text));
  while (*p && *p != ' ' && *p != '\t') p++;   // skip the keyword token
  p = skip_ws(p);

  size_t n = 0;
  while (*p && n < out_sz - 1) out[n++] = *p++;
  while (n && (out[n - 1] == ' ' || out[n - 1] == '\t' ||
               out[n - 1] == '\n' || out[n - 1] == '\r')) n--;
  out[n] = '\0';
  return n;
}
