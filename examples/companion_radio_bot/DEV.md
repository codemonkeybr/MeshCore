# Dev notes — companion_radio_bot

## ⚠️ Always clean-build after changing a bot header

`the_mesh` is defined in `examples/companion_radio/main.cpp`, which pulls in
`BotMesh.h` through a **macro** (`#include MESH_CLASS_HEADER`). PlatformIO's
dependency scanner does not follow macro includes, so `main.cpp.o` is **not**
rebuilt when a bot header changes — and it keeps sizing the `the_mesh` global
off a stale `sizeof(BotMesh)`. If the real object is larger, writes to its last
members run past the allocation and corrupt whatever symbol follows (crash on
every inbound message, boot loops, TCB asserts).

Until a forced-dependency script is added: **`pio run -t clean` before every
build** while iterating on bot headers. An incremental build is only safe when
you touched `.cpp` files only.

## Flashing (Heltec V4, ESP32-S3 native USB)

Opening `/dev/ttyACM0` from a Linux host toggles DTR/RTS, which the S3
USB-Serial/JTAG interprets as a reset — the CPU stops and the OLED freezes on
its last frame ("Loading..."). So:

- **Flash** with the device on the PC, in download mode
  (hold BOOT, tap RST, release):

  ```
  # first flash / factory (wipes identity, region, channels):
  pio run -e heltec_v4_companion_radio_bot -t mergebin
  esptool --port /dev/ttyACM0 --before no-reset --after no-reset erase-flash
  esptool --port /dev/ttyACM0 --before no-reset --after hard-reset --baud 921600 \
          write-flash 0x0 .pio/build/heltec_v4_companion_radio_bot/firmware-merged.bin

  # iterative reflash (keeps identity / LoRa region / channels — SPIFFS untouched):
  pio run -e heltec_v4_companion_radio_bot
  esptool --port /dev/ttyACM0 --before no-reset --after hard-reset --baud 921600 \
          write-flash 0x10000 .pio/build/heltec_v4_companion_radio_bot/firmware.bin
  ```

- **Run** on external power (charger / power bank), *not* the PC.

### OTA (once P5a is on the device)

```
BIN=.pio/build/heltec_v4_companion_radio_bot/firmware.bin
curl -sS -H "Expect:" -m 180 -u admin:changeme \
  -F "MD5=$(md5sum $BIN | cut -d' ' -f1)" -F "firmware=@$BIN" \
  http://192.168.1.185/update
# curl exits 56 ("connection reset") on success - ElegantOTA restart()s
# before sending the final "OK". Verify via /api/status:
curl -s -u admin:changeme http://192.168.1.185/api/status   # check "version" + fresh "uptime_s"
```

No clean-build needed for a .cpp-only change; still `pio run -t clean` after header edits.

## Observability

- Over the mesh: the `stats` command (uptime, free heap, msg/reply/drop counters,
  cache size). This is the primary health signal while serial is impractical.
- `BOT_DEBUG=1` prints to USB `Serial`, but see below.

### Serial monitor without halting the chip

`ModemManager` probes new serial devices on plug-in and can grab / reset the S3.
Tell it (and brltty) to ignore Espressif devices:

```
sudo tee /etc/udev/rules.d/99-esp32s3-serial.rules >/dev/null <<'RULES'
# Keep ModemManager / brltty off ESP32-S3 USB-Serial/JTAG
ATTRS{idVendor}=="303a", ENV{ID_MM_DEVICE_IGNORE}="1"
SUBSYSTEM=="tty", ATTRS{idVendor}=="303a", ENV{ID_MM_DEVICE_IGNORE}="1", ENV{MM_DEVICE_IGNORE}="1"
ATTRS{idVendor}=="303a", ENV{BRLTTY_NO_HANDLING}="1"
RULES
sudo udevadm control --reload-rules && sudo udevadm trigger
```

Then read without toggling control lines:

```
stty -F /dev/ttyACM0 115200 -hupcl clocal -crtscts
cat /dev/ttyACM0
```

If it still resets, keep it on the charger and rely on `stats`.
