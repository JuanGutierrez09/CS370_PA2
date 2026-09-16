# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project status

This repository is an early-stage scaffold for a red-black tree implementation in C. As of the initial commit, all files are empty placeholders:

- `include/rbtree.h` — public header (empty)
- `src/rbtree.c` — implementation (empty)
- `tests/test_rbtree.c` — unit tests (empty)
- `tests/fuzz.c` — fuzz test harness (empty)
- `Makefile` — build configuration (empty)
- `PROMPTLOG.md` — presumably a log of prompts/decisions for this project (empty)

There is no build system, source code, or test suite yet. Do not assume any commands (make, gcc flags, test runners) or architecture beyond what is described above — verify against the actual file contents before relying on anything here, since this section will be outdated as soon as real code is added.

## Working in this repo

Given the layout, the project is intended to be a C library exposing a red-black tree via `include/rbtree.h`, implemented in `src/rbtree.c`, with correctness tests in `tests/test_rbtree.c` and a fuzzing harness in `tests/fuzz.c`, built via the `Makefile`. When adding the first real content, update this file with the actual build/test/run commands and any architectural decisions (e.g., node structure, whether it's intrusive or value-owning, thread-safety guarantees) once they exist in code.
