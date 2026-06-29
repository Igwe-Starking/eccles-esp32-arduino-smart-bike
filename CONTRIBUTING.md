# Contributing to Eccles Bike Control

First off — thank you for even considering this. This project is a working, real-hardware
prototype that's still actively being debugged, not a finished product, and **anyone is welcome
to contribute**: fixing a bug, improving a comment, writing a test, or just reporting that
something doesn't work on your setup all count as contributions.

## Before you start

1. Read the **[Known Issues](./README.md#known-issues--debug-notes)** section of the README —
   it's the current source of truth for what's broken and what's mid-refactor.
2. Open an issue (or comment on an existing one) saying what you're about to work on, so two
   people don't fix the same thing at the same time.
3. If you're fixing something not yet in Known Issues, that's fine too — just describe it in
   your issue/PR so reviewers have context.

## Getting set up

```bash
git clone <this repo>
cd EcclesBikeControl
pio run -e native       # builds + runs the TTS packager against eccles/resources/models/
pio run -e esp32dev      # builds the firmware (read the C++17 note in Known Issues #1 first)
```

You don't need real bike hardware to contribute meaningfully — most of the logic (command
parsing, audio DSP, the TTS packager, the conversation keyword matcher) can be reasoned about
and unit-tested without anything attached to a GPIO. Hardware-dependent changes (pin mappings,
timing-sensitive I2S/Bluetooth code) ideally get a note in the PR saying what you tested it on.

## Code style

Match what's already there rather than introducing a new style in your corner of the codebase:

- Types are the `e_` prefixed aliases from `EcclesTypes.h` (`e_uint8`, `e_string`, `e_boolean`,
  …), not raw `uint8_t`/`bool`/`char*`. This keeps a single place to retarget types if the
  platform changes.
- Code lives inside the `ECCLES_API { ... }` namespace wrapper.
- The firmware avoids dynamic allocation on hot paths where it reasonably can (command pooling in
  `Executor.cpp` is the canonical example) — if you're adding something that runs every loop
  iteration or every command, think about whether it needs `e_malloc`/`new` at all.
- Command/text parsing uses compile-time word hashing (`eccles_hashCT`/`eccles_hashRT`) rather
  than `strcmp` chains — follow that pattern for new keyword-driven logic instead of introducing
  string comparisons.
- Comment the *why*, not just the *what* — most of the existing comments explain a debugging
  story or a tradeoff, not just what the next line does. That history is genuinely useful to
  future contributors; please keep adding to it rather than stripping it out.

## Testing

`test/` is set up for [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html).
If you're touching parsing or DSP logic (`Executor.cpp`'s command parsers, `AudioConfig.cpp`'s
`Resampler`), please add or extend a test rather than relying on manual hardware testing alone —
those are exactly the kinds of pure-logic functions that are cheap to verify automatically and
expensive to debug only on real hardware.

## Submitting a change

1. Fork, branch off `main` with a descriptive name (`fix/binary-command-size-check`,
   `feature/conversation-ai-start`, etc.).
2. Keep PRs scoped to one issue/fix where possible — easier to review, easier to revert if wrong.
3. Update `CHANGELOG.md`: move the item from "Known Issues" into a new `[Unreleased] → Fixed`
   entry (or add a new entry if it wasn't already tracked).
4. Describe in the PR what you tested and how (unit test, native build, real hardware, etc.).
5. Be patient — this is a small project maintained around other commitments.

## Reporting bugs / suggesting ideas

Open a GitHub issue. Useful things to include:
- What you expected vs. what happened
- Which environment (`native` packager vs. `esp32dev` firmware) and, for firmware, what hardware
  revision/wiring you're using
- Relevant serial log output if `ECCLES_DEBUG` logging caught anything

## Code of conduct

Be respectful, be patient with people who are still learning (embedded C++ is hard, and a lot of
this project itself is a learning exercise), and assume good faith. Disagree about code, not
about people.

## License

By contributing, you agree your contributions are licensed under this project's
[MIT License](./LICENSE).
