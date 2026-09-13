# Balin

A minimal 2D raylib game written in C.

The program opens a 1280x720 window and draws a white square in the center.

## Build and run

Debug build with AddressSanitizer and UBSan:

```sh
./run.sh
```

Or manually:

```sh
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug -j
./output/balin
```

Release build:

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j
./output/balin
```

Both builds write `output/balin`, so the last build wins.

## Tests

```sh
ctest --test-dir build/debug --output-on-failure
```

`ctest` runs the assert-based test binary and the ADR-002 layer rule check.

## Bug bundles

Dev builds write a bug bundle when you press F9. Fatal errors write one
automatically in every build. A bundle lands in `output/bugs/<timestamp>/` and
contains `session.log`, `ring.log` (the last 2048 events at trace detail),
`state.txt`, and `env.txt`.

## Layout

- `src/main.c`: composition root. Owns the window, the loop, and wiring.
- `src/engine/`: engine modules. No includes from game or tools.
- `src/game/`: game modules. May include engine.
- `src/tools/`: dev tools, built when `BALIN_DEV` is on. May include engine and game.
- `tests/test_main.c`: assert-based tests.
- `tests/check_layers.cmake`: layer rule check, run by ctest.
- `vendor/raylib`: vendored raylib.
