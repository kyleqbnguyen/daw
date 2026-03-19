# AGENTS.md

Guidelines for AI coding agents working in this repository.

## Project Overview

Open-source DAW (digital audio workstation) built with C++20, JUCE 8, and
Tracktion Engine 3. Licensed under GPLv3. Targets macOS, Windows, and Linux.

## Build Commands

First-time configure (fetches all dependencies — takes a few minutes):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -Wno-dev
```

Build:

```bash
cmake --build build --config Release --parallel
```

Run the application (macOS):

```bash
open build/DAW_artefacts/Release/DAW.app
```

## Test Commands

Run all tests:

```bash
ctest --test-dir build --build-config Release --output-on-failure
```

Run a single test by name:

```bash
ctest --test-dir build --build-config Release -R "test name pattern"
```

Run the test binary directly with Catch2 filters:

```bash
./build/DAWTests "test name pattern"
./build/DAWTests "[tag]"
```

## Project Structure

```
src/              Application source code
tests/            Catch2 test files
resources/        Assets (icons, fonts, etc.)
cmake/            CMake utility modules
.github/          CI workflows
```

Dependencies are fetched automatically via CMake FetchContent:
- **JUCE 8.0.6** — audio framework, UI, plugin hosting
- **Tracktion Engine v3.2.0** — DAW engine (transport, clips, mixing, plugins)
- **Catch2 v3.7.1** — test framework

## Code Style

Enforced by `.clang-format`. Run before committing:

```bash
clang-format -i src/**/*.cpp src/**/*.h
```

### Formatting Rules

- 2-space indentation, no tabs
- 80-column line limit
- Attached braces (K&R style, opening brace on same line)
- Pointers and references bind left: `int* ptr`, `const std::string& ref`
- Short single-expression methods may go on one line:
  `void shutdown() override { mainWindow.reset(); }`
- Template declarations always break before the template body

### Includes

Order (sorted case-insensitive within each group, separated by blank line):

1. Corresponding header (for `.cpp` files)
2. Project headers (`"Header.h"`)
3. Third-party/system headers (`<JuceHeader.h>`, `<tracktion_engine/...>`)

### Naming Conventions

- **Classes/Structs:** `PascalCase` — `MainComponent`, `AudioTrack`
- **Functions/Methods:** `camelCase` — `getApplicationName()`, `closeButtonPressed()`
- **Variables/Members:** `camelCase` — `mainWindow`, `engine`
- **Constants/Enums:** follow JUCE conventions
- **Files:** `PascalCase.cpp` / `PascalCase.h`, matching the primary class name

### Comments

Code should be self-documenting. Do not add comments that restate what the
code does. Only add comments when explaining *why* something non-obvious is
done a particular way. No section separator comment blocks.

Bad:
```cpp
// Set the window size
setSize(1280, 720);

// ============= Transport =============
```

Acceptable:
```cpp
// Only add the modules subdirectory to avoid conflicting with our JUCE
add_subdirectory(${tracktion_engine_SOURCE_DIR}/modules ...)
```

### License Headers

All source files must include the GPLv3 license header at the top:

```cpp
/*
    DAW - An open-source digital audio workstation
    Copyright (C) 2026 DAW Project Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
```

Test files use the shortened form (omit the "more details" paragraph).

### C++ Guidelines

- Use C++20 features where appropriate (concepts, ranges, `std::format`,
  structured bindings, `auto` with template lambdas)
- Prefer `std::unique_ptr` / `std::make_unique` for ownership
- Use JUCE's `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` macro on
  Component subclasses
- Use `#pragma once` for header guards
- Use `override` on all virtual method overrides
- Prefer `explicit` on single-argument constructors
- Use brace initialization for member defaults: `Engine engine{"DAW"};`

### CMakeLists.txt Style

- 2-space indentation
- No section separator comment blocks
- Only comment to explain non-obvious *why*, not *what*
- One argument per line for multi-argument CMake function calls

### Tests

- Framework: Catch2 v3
- Test files go in `tests/` and are named descriptively
- Use descriptive `TEST_CASE` names and `[tag]` annotations
- Tag conventions: `[smoke]` for build verification, feature tags as needed

## Key Architecture Notes

- `src/Main.cpp` — JUCE application entry point, owns `MainWindow`
- `MainComponent` — root UI component, owns the `tracktion::engine::Engine`
- Tracktion Engine is added as modules only (not the full repo) to avoid
  duplicate JUCE targets — see the `FetchContent_Declare` pattern in
  `CMakeLists.txt`
- Plugin hosting is enabled for VST3 and AU (macOS)
