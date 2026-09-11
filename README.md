# Balin

A minimal 2D raylib starter written in C.

The program opens a 1280x720 window and draws a white square in the center.

## Build and run

```sh
./run.sh
```

Or manually:

```sh
cmake -S . -B build
cmake --build build -j
./output/balin
```

## Layout

- `src/main.c`: the whole program.
- `vendor/raylib`: vendored raylib.
