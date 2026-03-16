# Repository Guidelines

This repository hosts MMIO++ (a C++ memory-mapped I/O access framework). The project is in an early scaffolding state, so several conventions below are forward-looking and should be updated as code is added.

## Project Structure & Module Organization

- Root files: `README.md` and `LICENSE` are the only tracked files today.
- Source code: not yet present. When introduced, keep library headers under `include/` and implementation under `src/` to keep the public API clear.
- Tests: not yet present. Add unit tests under `tests/` (or `test/`) and mirror the library namespace in the directory layout.
- Assets/config: keep hardware register maps, docs, or fixtures in a dedicated `assets/` or `docs/` folder to avoid mixing with code.

## Build, Test, and Development Commands

No build or test system is defined yet. If you add one, document the exact commands here. Example (if CMake is adopted):

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Coding Style & Naming Conventions

- Language: C++.
- Formatting/linting: no tooling configured yet. If you add clang-format or clang-tidy, pin versions and include config files at the repo root.
- Naming: keep public types and functions consistent and descriptive (e.g., `RegisterBank`, `ReadRegister`, `write_register`). Document any namespace conventions when added.
- Indentation: default to 2 or 4 spaces and keep it consistent; update this guide once the codebase establishes a preference.

## Testing Guidelines

- Framework: not selected yet. If you introduce one (e.g., Catch2, GoogleTest), add it to the build and document how to run it.
- Coverage: prioritize register access paths, error handling, and boundary conditions.
- Naming: align test files with the class or module name (e.g., `register_bank_tests.cpp`).

## Commit & Pull Request Guidelines

- Commit messages in history are short, imperative, and capitalized (e.g., "Update README.md", "Initial commit"). Follow this style for consistency.
- PRs should include a concise summary, rationale, and the tests run (or note if not run). Link related issues when applicable and include API/behavior changes in the description.
