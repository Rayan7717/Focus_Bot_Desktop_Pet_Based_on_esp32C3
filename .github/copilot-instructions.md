Project: Focus Bot (ESP32-C3 Emotional AI Desktop Pet)

Quick context
- Target: ESP32-C3 Supermini microcontroller
- Primary sketches: `EmotionalAI_DesktopPet_Logic_V1.ino` (main firmware), `Animation_Test.ino` (animation loader/test)
- Animations live under `Animation_Test_Compression Implemented/` and legacy frames in `OLD Ref frames/`

What the project expects from an AI coding agent
- Keep edits hardware-aware: pin defines (SDA_PIN=8, SCL_PIN=9, MOTOR_PIN=6, TOUCH pins in README) drive behavior across files.
- Preserve LittleFS/SPIFFS animation format and RLE compression semantics. Animation binaries are loaded using a 12-byte header (frame_count, fps, max_compressed_size) followed by a 2-byte-per-frame size table and fixed-size compressed slots.
- Avoid increasing RAM footprint unnecessarily (device is an ESP32-C3 with limited RAM). Prefer on-demand file streaming (as shown in `Animation_Test.ino`) over loading whole datasets into memory.

Important files & patterns (reference examples)
- `README.md` — project overview, hardware, and developer steps (use for board/partition guidance).
- `Animation_Test_Compression Implemented/Animation_Test.ino` — Reference for:
  - LittleFS mounting and format fallback
  - Animation file layout and reader (`loadAnimationMetadata`, `loadAnimation`, `displayFrameFromLittleFS`)
  - RLE decompression format (pairs of [count, value])
- `EmotionalAI_DesktopPet_Logic_V1.ino` — main AI/state machine (emotion transitions, personality stored in EEPROM). Use this for behavioral conventions and persistent-data expectations.
- `convert_to_spiffs.py` — used to create `.anim` binaries before uploading LittleFS; maintain compatibility with this tool when changing the animation layout.

Build / Deploy / Debug workflows
- Typical development uses the Arduino IDE with ESP32 board support (ESP32C3 Dev Module). Follow README steps for board URL and library installation.
- Animation workflow:
  1. Run `Animation_Test_Compression Implemented/convert_to_spiffs.py` to generate binary `.anim` files.
  2. Use Tools → ESP32 Sketch Data Upload (ESP32 filesystem uploader plugin) to upload the `data/` directory to LittleFS.
  3. Upload sketch (`Animation_Test.ino` or main firmware) via Arduino IDE.
- Partition note: if build size is large, adjust Tools → Partition Scheme (README suggests `Minimal SPIFFS` or 4MB scheme). Do not assume LittleFS exists — code formats on first run if missing.

Code conventions and constraints
- I2C devices (OLED, MPU6050) share the same bus (SDA_PIN=8, SCL_PIN=9) — changes to these pins must be propagated to all sketches.
- Animation files use fixed max compressed slot size (header field). When writing code that computes frame offsets, follow the same calculation used in `displayFrameFromLittleFS`:
  frameDataStart = 12 + (frameCount * 2)
  frameOffset = frameDataStart + (frameIndex * maxCompressedSize)
- RLE compression uses byte pairs: [count, value]. Decompression iterates pairs and emits `count` copies of `value`. Tests expect decompressed size = 128*64/8 = 1024 bytes.
- Persistent data (personality traits) is stored in EEPROM; keep struct layout stable to avoid migration issues.

Testing and performance checks
- Prefer microbenchmarks similar to `Animation_Test.ino` (use micros() to measure read/decompress/display times). Keep decompression <~1ms.
- When modifying animation code, verify: LittleFS file listing, header parsing (`loadAnimationMetadata`), and per-frame read success.

Safe change checklist for PRs
1. Confirm pin defines and I2C addresses are unchanged unless intentionally refactoring. Update README if pins change.
2. Run `convert_to_spiffs.py` and ensure produced `.anim` files match the expected header + frame table layout.
3. If increasing max compressed slot size, update `MAX_COMPRESSED_SIZE` and re-run tests to ensure malloc success on device.
4. Validate LittleFS mount behavior and that formatting fallback still works (the code intentionally formats on failure).

When you are unsure
- Ask for hardware details (actual board revision) if code touches low-level peripheral timing or memory sizes.
- For animation format changes, request a reference `.anim` produced by `convert_to_spiffs.py` to keep compatibility.

Contact the maintainer when necessary
- Owner: Rayan Singh (maintainer). For behavioral changes that alter EEPROM layout or core emotion-state transitions, request explicit sign-off.

Please review these instructions and tell me if you want additional examples, stricter safety rules (e.g., avoid EEPROM schema changes), or CI checks added.
