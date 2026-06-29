# Changelog

All notable changes to this project are tracked here, following the spirit of
[Keep a Changelog](https://keepachangelog.com/). Dates use `YYYY-MM-DD`.

> **Note on how this file was started:** no git history was available at the time this changelog
> was first written (2026-06-29) — it was reconstructed from a full static code review (inline
> comments documenting prior fixes, and observed current behavior). Going forward, add entries
> here as part of each PR instead of reconstructing them after the fact.

## [Unreleased] — Debug Build

### Added
- Project documentation set: `README.md`, `eccles/tools/README.md`, `CONTRIBUTING.md`, this
  `CHANGELOG.md`, generated from a full-codebase review.

### Fixed (evidenced by inline code comments — confirm against real history if available)
- TTS packager argument parsing: a pre-increment bug previously made `-type dynamic` silently
  skip the next argument because `params[i++]` was compared against `"-type"` itself instead of
  peeking at the following argument (`eccles/tools/src/main.cpp`).
- `WebTransport::run()` now calls `udp.begin(PORT)` after the web server starts, so the UDP
  IP-broadcast actually listens/sends on the intended port (`src/Transport.cpp`).
- Filesystem driver conflict between LittleFS and SPIFFS under `espressif32@3.5.0` resolved by
  standardizing on SPIFFS (`board_build.filesystem = spiffs` in both PlatformIO environments).

### Known Issues — Not Yet Fixed

See the [README's Known Issues section](./README.md#known-issues--debug-notes) for the full,
maintained list. As of this writing it includes:
- C++17 not pinned in `platformio.ini` → possible linker errors building `Conversation.cpp`
- `StringCommand::parse` (~370 lines, duplicated monitor-packing logic) mid-refactor
- No upper bound on incoming command data size in `BinaryCommand::parse`
- `ConversationMode::AI` start path unimplemented (dead code path)
- Possible race condition in `Conversation::stop()`
- Tight stack margin in the audio record task
- UDP port 4210 reused for two unrelated protocols
- `LiveData::normalize()` clamps sensor overflow to 0% instead of 100%
- Global `delay` variable shadows Arduino's `delay()` in `Transport.cpp`
- TTS packager: manual (non-`-platformIO`) CLI invocation does nothing
- TTS packager: `extract()` unimplemented; dynamic model format parsed but unused

---

## How to add to this file

When you fix something, move it from "Known Issues" (here and in the README) into a new entry
under `[Unreleased] → Fixed`, with the file/function it lived in. When you cut an actual release,
rename `[Unreleased]` to a version number + date, and start a fresh `[Unreleased]` section above it.
