AGENT PLAYBOOK FOR THIS REPO
-----------------------------
Goal: Give coding agents a concise, opinionated guide for building, flashing, testing, and coding style in this ESP-IDF + CycloneTCP project. Default to correctness and reproducibility.

PROJECT OVERVIEW
- Firmware: ESP32 using ESP-IDF and CycloneTCP; main sources under `main/` and `main/common/`.
- Build system: ESP-IDF CMake via `idf.py` (project root has `CMakeLists.txt`).
- Codebase language: C99/C11; strong embedded constraints (no dynamic alloc, strict type usage).
- Tests: No automated test targets are present; TDD ethos documented (Unity/CMock expected, but not yet wired here).
- Style authority: `.github/copilot-instructions.md` (embedded C, TDD, SOLID, Clean Architecture); no Cursor rules found.

BUILD / FLASH / CLEAN
- Full build: `idf.py build`
- Flash (requires connected board + correct `idf.py set-target` if needed): `idf.py flash`
- Build + flash in one: `idf.py flash -p <PORT> -b <BAUD>` (set `ESPPORT`/`ESPBAUD` envs or pass flags)
- Clean: `idf.py fullclean` (removes build dir and CMake cache)
- Select target (if not already configured): `idf.py set-target esp32`
- Build verbose: `idf.py build -v`
- Reconfigure only: `idf.py reconfigure`

RUNNING (AND LACK OF) TESTS
- There are currently no Unity/CMock or IDF unit-test components in the tree.
- If you add ESP-IDF unit tests, follow: `idf.py -T <test_name>` for a single test app, or `idf.py build` inside `components/<test>` if structured that way.
- Until tests exist, validate by building and, if possible, flashing and exercising the device manually. Document any ad-hoc steps in PRs.

LINT / STATIC ANALYSIS
- No dedicated lint target or config is present (no clang-format/clang-tidy configs found).
- Prefer compiler warnings as the first line of defense; keep builds warning-free.
- If adding linting, keep commands under `idf.py` custom targets or `tools/` scripts and document them here.

REPO STRUCTURE HINTS
- `main/` contains the ESP-IDF component with CycloneTCP sources and Wi-Fi manager logic.
- `main/common/` holds shared utilities: OS ports, debug macros, endian helpers, date/time, resource manager, path helpers, string utils.
- `main/cyclone_tcp/` mirrors CycloneTCP stack modules (core, protocols, drivers).
- `main/wifi_manager.*` exposes Wi-Fi AP/STA state machine and config plumbing.

IMPORT / INCLUDE ORDER
- Standard headers first (`<stdint.h>`, `<stdbool.h>`, `<stdio.h>`), then ESP-IDF headers (`esp_wifi.h`, `esp_event.h`, `driver/gpio.h`, etc.), then CycloneTCP headers (`core/net.h`, `drivers/wifi/esp32_wifi_driver.h`, protocol headers), then local module headers.
- Avoid unused includes; keep headers minimal to reduce build times and coupling.
- Header include guards are required (`#ifndef NAME_H` / `#define NAME_H`).

LANGUAGE AND TYPES (COPILOT INSTRUCTIONS APPLY)
- Use fixed-width integer types from `<stdint.h>` and booleans from `<stdbool.h>`; avoid plain `int/char/long` for logic unless mandated by APIs.
- Enforce const-correctness for read-only data (`const uint8_t *buf`).
- Prohibit dynamic allocation (`malloc/calloc/free`); prefer static or stack allocation and explicit buffer lengths.
- Pass buffer pointers with explicit length parameters; validate inputs at function entry, return `ERR_NULL_POINTER` when applicable.

NAMING CONVENTIONS
- Files: `snake_case` (e.g., `wifi_manager.c`, `i_serial.h`).
- Types/structs: `PascalCase_t` (e.g., `WifiManagerContext_t`).
- Interfaces: `I` + `PascalCase_t` (e.g., `ITimeSource_t`).
- Public functions: `Module_Action` (e.g., `WifiManager_Init`).
- Private/internal functions: `static` + `snake_case`.
- Locals: `snake_case`; static globals prefixed `s_`; macros/constants `UPPER_SNAKE_CASE`.

ARCHITECTURE PRINCIPLES (FROM COPILOT RULES)
- Apply Clean Architecture boundaries: `App/` and domain logic must not include hardware-specific headers like `stm32u5xx_hal.h` (analogy: keep ESP-IDF specifics out of domain layers; confine to adapters).
- Use dependency inversion: inject interfaces via init functions, avoid global lookups.
- Use v-table style interfaces for drivers/adapters when extension is needed.
- Keep ISRs minimal: copy data, signal via queue/semaphore, handle work in tasks; never block inside ISRs.

ERROR HANDLING
- Prefer `Result_t`-style enums with explicit values (`ERR_OK`, `ERR_ERROR`, `ERR_NULL_POINTER`, `ERR_TIMEOUT`, `ERR_BUSY`, `ERR_INVALID_PARAM`).
- Callers must check return codes; do not ignore results.
- Provide clear logs via debug macros; avoid silent failures.

LOGGING / TRACE (DEBUG MACROS)
- Trace levels defined in `main/common/debug.h`: `TRACE_LEVEL_*` with default `TRACE_LEVEL_DEBUG`.
- Use `TRACE_ERROR/TRACE_WARNING/TRACE_INFO/TRACE_DEBUG` macros; they suspend/resume tasks around `fprintf` to ensure thread safety.
- Avoid logging from within tight ISR contexts; prefer deferred logging in tasks.

MEMORY AND CONCURRENCY
- No heap allocation; design with deterministic stack/static use.
- When sharing buffers across tasks, guard with proper FreeRTOS primitives; avoid busy-waiting.
- Ensure ISR callbacks remain under ~50us; hand off via queues or semaphores.

FORMATTING / STRUCTURE
- Stick to C99/C11; keep line length reasonable (~100-120 chars) for readability in terminals.
- Braces on new lines for functions and control blocks; indent with spaces consistent with existing files (project uses spaces, width 3-4; follow local file).
- Group related declarations; initialize variables at point of use.
- Doxygen comments on public APIs (as in `wifi_manager.h`) with `@brief`, `@param`, `@return`.

IMPORTANCE OF TDD (COPILOT)
- Mandate: write a failing Unity/CMock test before production code; minimal code to pass; refactor with tests green.
- If HAL access blocks testability, refactor behind interfaces and inject fakes/mocks.
- Document any deviation when tests are absent; add TODOs with rationale and next steps to add tests.

ESP-IDF / CYCLONETCP SPECIFICS
- Wi-Fi: `WifiManager` targets dual AP+STA with backoff; preserve state machine semantics when modifying.
- TCP/IP: CycloneTCP stack is vendored; avoid style rewrites or broad refactors unless necessary; prefer targeted fixes.
- Configuration: SSID/PWD buffers sized via `WIFI_MANAGER_SSID_MAX_LEN`/`PASSWORD_MAX_LEN`; enforce bounds checks on writes.

ADDING NEW CODE
- Create headers for new modules; expose only necessary public API; keep private helpers `static`.
- Validate inputs early; return explicit error codes; log context on failure.
- Keep dependencies narrow: prefer passing interfaces/config structs rather than global access.
- For portability, avoid ESP-IDF-specific types in domain logic; wrap them in adapters.

DOCS AND COMMENTS
- Comment only when intent is non-obvious; prefer clear naming over redundant comments.
- Update this file and README when adding build/test steps or tools.

CHECKLIST BEFORE OPENING A PR
- Build: `idf.py build` must succeed and be warning-free.
- Tests: run any added Unity/IDF tests; if none, state that no automated tests exist.
- Static checks: if you added lint/format steps, run them and note versions.
- Logs: ensure debug output is sensible and gated by trace levels.

IF YOU INTRODUCE TESTS LATER
- Place Unity tests in an ESP-IDF test component (`components/<name>/test/` or `test/` app); expose a target callable via `idf.py -T`.
- Keep tests hardware-isolated; mock hardware via interfaces; avoid direct Wi-Fi/radio usage in unit tests.
- Document new commands here when added.

MISC
- No Cursor rules found; Copilot rules (above) are authoritative.
- Keep ASCII-only unless existing code/file already uses extended characters (some docs use Spanish; keep accents where present in docs, but code stays ASCII-friendly).

CONTACTS / OWNERSHIP
- Not specified in-repo; when in doubt, preserve existing behaviors and minimize changes to third-party CycloneTCP sources unless bug-fixing.

END OF PLAYBOOK
