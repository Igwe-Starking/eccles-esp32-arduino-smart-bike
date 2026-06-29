# Eccles Bike Control (ESP32 Smart Bike Control System)

> ## ⚠️ STATUS: ACTIVE DEBUG BUILD — NOT PRODUCTION READY
> This firmware is a **work in progress**. Core features run on real hardware, but several
> subsystems are incomplete, unguarded against malformed input, or have known race conditions.
> See **[Known Issues](#known-issues--debug-notes)** below before flashing anything you depend on.
> `EcclesConfig.h` currently ships with `#define ECCLES_DEBUG` enabled — expect verbose serial logging.

---

## Overview

Eccles Bike Control is an ESP32 firmware that turns a motorcycle/bike's electrical system into a
remotely controllable, voice-aware smart device. It exposes the bike's headlamp, horn, turn
signals, starter, ignition, and engine lock to:

- A **WebSocket + plain HTML web UI** served directly from the ESP32 (binary protocol)
- A **Serial console** (human-readable text commands)
- A **Bluetooth A2DP sink** (acts as a wireless speaker, with AVRCP transport controls)
- A **voice "Conversation" mode** — either bridging mic/speaker audio to a paired phone for
  real recognition (`REAL` mode), or answering simple canned questions on-device (`AI` mode)
- A custom **TTS packager** (`eccles/tools/`) that bundles `.wav` clips into firmware-embedded
  speech the bike can play back without any audio file on an SD card

## Architecture, in one paragraph

Everything funnels through a single `Executor` command pool (`Executor.h/.cpp`). Web, Serial,
and Bluetooth inputs are each parsed into a common `Command` struct (`BinaryCommand::parse` for
the websocket wire format, `StringCommand::parse` for serial text), pushed into a fixed-size pool
(no heap churn per command), and drained once per loop by whichever `Executor` subclass
(`DeviceExecutor`, `Bluetooth`, `Conversation`, `Configuration`, …) recognizes the command's
target device. Results flow back out through a `ResultHandler` matched to whichever transport
sent the command in.

## Hardware / Wiring

Pin assignments live in `include/Pins.h`. ESP32 GPIOs 32–39 are **input-only / ADC1**, which is
why every analog/feedback signal below is mapped to one of them.

| Function                | GPIO | Direction | Notes |
|--------------------------|------|-----------|-------|
| Ignition control         | 16   | Output    | Drives ignition relay/MOSFET |
| Horn control             | 17   | Output    | |
| Headlamp control         | 18   | Output    | |
| Left signal control      | 19   | Output    | |
| Right signal control     | 23   | Output    | |
| Starter control          | 5    | Output    | ⚠️ **Strapping pin** — avoid an external pull that's active at boot |
| Engine lock control      | 4    | Output    | |
| I2S bit clock (BCLK)     | 26   | Output    | Shared by speaker out + mic in |
| I2S word select (LRCLK)  | 22   | Output    | |
| I2S data                 | 25   | I/O       | Direction depends on active I2S mode |
| Ignition feedback / batt.| 32   | Analog in | Also used to derive battery voltage (see `LiveData::voltageLevel`) |
| Fuel gauge                | 35   | Analog in | Input-only pin |
| Temp gauge                | 33   | Analog in | Input-only pin |
| Microphone                | 36   | Analog in | Input-only pin |
| Horn feedback             | 14   | Digital in| |
| Headlamp feedback         | 27   | Digital in| |
| Starter feedback          | 34   | Digital in| Input-only pin |
| Left signal feedback      | 13   | Digital in| |
| Right signal feedback     | 21   | Digital in| |
| Shock sensor              | 39   | Digital in| Input-only pin |
| Engine lock feedback       | 2    | Digital in| ⚠️ **Strapping pin** — floating/driven state at boot can affect boot mode |

```
                         +-------------------------+
   Headlamp relay <----- |16 IGN  17 HORN  18 LAMP | <---- 14 Horn FB
   Horn relay      <----- |19 L-SIG 23 R-SIG  5 STRT| <---- 27 Lamp FB
   Left signal      <----- |4  ENG_LK               | <---- 34 Starter FB
   Right signal     <----- |                        | <---- 13 L-Sig FB
   Starter relay    <----- |        ESP32            | <---- 21 R-Sig FB
   Engine lock       <----- |                        | <---- 2  Eng Lock FB
                            |                         |
   Battery (via      ----> |32  ADC                  |
     divider, R0/R1)        |33  ADC <---- Temp sender
                            |35  ADC <---- Fuel sender
                            |36  ADC <---- Mic / preamp out
                            |39       <---- Shock sensor
                            |                         |
   I2S DAC/codec    <-----> |26 BCLK 22 LRCLK 25 DATA|
                            +-------------------------+
```

All output-driven loads (headlamp, horn, signals, starter, ignition, engine lock) are driven
through relays/MOSFETs, not directly from the GPIO — the bike's 12–14V rails are never tied
straight to the ESP32. The ignition feedback pin doubles as the battery-voltage sense input via
a resistor divider (`R0`/`R1` in `DeviceManager.h`), scaled down to the ESP32's 3.3V ADC range.

## Building & Flashing

Two PlatformIO environments are defined in `platformio.ini`:

| Env | Purpose |
|---|---|
| `[env:native]` | Builds the **TTS packager** for your PC, then a post-build script (`execute-tts-packager.py`) runs it against `eccles/resources/models/*.wav`, regenerates `data/StaticModel.eccles` + `include/StaticModel.h`, and uploads the result to the ESP32's LittleFS. |
| `[env:esp32dev]` | Builds and flashes the actual firmware. |

```bash
pio run -e native      # rebuild TTS models (only re-packages if the models folder changed)
pio run -e esp32dev -t upload
```

**Before building `esp32dev`, read [Known Issue #1](#known-issues--debug-notes)** — the project
does not currently pin a C++ standard, and recent `ConversationAI` code requires C++17 to link.

Dependencies (already in `platformio.ini`): `LittleFS_esp32`, `AsyncTCP`, `ESPAsyncWebServer-esphome`.
Filesystem is **SPIFFS** (not LittleFS, despite the LittleFS library dependency — see Known Issues).

## Using It

**Serial console** (115200 baud, newline-terminated, case-insensitive):
```
on headlamp
off horn
whenever fuel less_than 20
if engine on greater_than 0
on ignition for 5
```
Recognized action/target/modifier words are listed in the comment block at the top of
`StringCommand::parse` in `src/Executor.cpp`.

**Web UI** — connect to the ESP32's Wi-Fi AP/network, browse to its IP (broadcast over UDP on
port 4210 if mDNS resolution fails), and use the on-page buttons. The web client and the
binary WebSocket protocol both speak the 8-byte+ command frame documented in `BinaryCommand::parse`.

**Bluetooth** — the device advertises as an A2DP sink; pair a phone to it like any Bluetooth
speaker. Play/pause/next/prev are relayed back to the phone as AVRCP transport commands.

**Conversation** — `START_REAL` bridges live mic/speaker audio to a paired companion app over
UDP. `PLAY` (with text payload) speaks a reply via the on-device `ConversationAI` keyword
matcher, or speaks the text verbatim if `dataType == RAW_REPLY`. **`START_AI` does not currently
do anything — see Known Issues.**

## Known Issues / Debug Notes

This list reflects a full-codebase review (June 2026), not a formal issue tracker. File a real
GitHub issue for anything below you start working on, so two people don't fix it twice.

1. **C++17 required, but not pinned.** `ConversationAI` (in `Conversation.cpp`) has ~20
   `static constexpr` reply-bank arrays that get their address taken (passed as pointers). Before
   C++17, that requires an out-of-class definition or you get "undefined reference" linker
   errors. `platformio.ini`'s `[env:esp32dev]` doesn't set `-std=`, so it may fall back to the
   platform's default (`gnu++11` on older `espressif32` releases). **Fix:** add
   `build_flags = -std=gnu++17` and `build_unflags = -std=gnu++11` to `[env:esp32dev]`.
2. **`StringCommand::parse` is one ~370-line function** (`src/Executor.cpp`) with the same
   9-byte monitor-data packing logic copy-pasted in four places. A refactor extracting a shared
   `packMonitorByte()` helper is in progress on a separate branch.
3. **`BinaryCommand::parse` doesn't cap incoming data size** before `e_malloc(com->size)`.
   `com->size` is attacker/sender-controlled (2 bytes off the wire). Outgoing results are capped
   at 253 bytes (`D_MAX` in `Executor.cpp`); incoming has no equivalent ceiling — a malformed
   packet can request an arbitrarily large allocation.
4. **`Conversation::start()` never handles `ConversationMode::AI`.** The function only has a body
   for `ConversationMode::REAL`; calling `START_AI` silently does nothing. In practice, AI replies
   only ever happen through the stateless `PLAY` command path, independent of `active`/`instance`.
   This looks like unfinished cleanup after the design moved from a stateful AI session to
   stateless raw-command replies.
5. **`Conversation::stop()` race condition.** It flips `AudioInput::run = false` and immediately
   calls `cnUdp.stop()` from the caller's task, while the record task may still be mid-operation
   on the same `WiFiUDP` object on a different FreeRTOS task.
6. **Tight stack margin in the audio record task.** `ConversationHandler::onData()` runs on a task
   started with a 6000-byte stack (`Audio.cpp`) and stack-allocates a 2048-byte buffer on top of
   whatever `i2s_read`/`WiFiUDP` need underneath it.
7. **Port 4210 is reused for two unrelated UDP uses** — IP-broadcast discovery (`Transport.cpp`)
   and live conversation audio streaming (`Conversation.cpp`). Works today only because the
   broadcast listener is stopped once a WebSocket client connects.
8. **`LiveData::normalize()` clamps overflow to 0% instead of 100%** (`src/DeviceManager.cpp`):
   `if(p > 100) p = 0;` — an out-of-range sensor reading gets reported as "very low" instead of
   "very high," which is the more misleading failure mode for fuel/temp readouts.
9. **Global `delay` variable shadows Arduino's `delay()`** in `Transport.cpp`
   (`e_uint32 delay = 1000;`). No current call site is broken by it, but it's a landmine for the
   next person who adds a `delay(...)` call to that file.
10. **Packager manual CLI is currently non-functional.** `eccles_main()` (`eccles/tools/src/main.cpp`)
    parses `-type`/folder/`.txt` config arguments, but the dispatch at the bottom of the function
    only ever calls `staticModel::pack()` when `-platformIO` is passed; without that flag it
    parses your arguments and then unconditionally returns failure. Only the PlatformIO-driven
    build path currently packages anything.
11. **`staticModel::extract()` is a stub** (`return false;`) — extraction mode isn't implemented.
12. **No `EcclesConfig.txt` currently exists in this repo**, so the packager always builds with
    default settings (8000 Hz, 16-bit, mono, trailing-silence-removal on) until one is added.
13. **Dead/commented-out code retained for reference**: `DeviceManager::createFromPins()` and the
    `DAC` namespace in `Audio.cpp` are fully commented out but kept in the tree. Harmless, but a
    new contributor may not immediately realize they're inert.
14. **Stale inline comment** in `LiveData::monitor()`: `//5ms` next to a 500ms interval value.

## Roadmap

- [ ] Finish the `StringCommand::parse` refactor (shared monitor-byte packing helper)
- [ ] Wire up or remove the `ConversationMode::AI` start path
- [ ] Add an upper bound check to `BinaryCommand::parse`'s incoming data size
- [ ] Fix manual (non-`-platformIO`) CLI invocation of the TTS packager
- [ ] Implement `staticModel::extract()`
- [ ] Migrate fully to LittleFS (or fully to SPIFFS — pick one consistently)

## License

MIT License — see [`LICENSE`](./LICENSE).

## Credits

**Nwobodo Ecclesiastes Chidera**, A.K.A. **Igwe Starking** — design, firmware, and TTS tooling.
