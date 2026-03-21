# Building DAW

## Prerequisites

### All Platforms

- **CMake 3.22+** — [cmake.org/download](https://cmake.org/download/)
- **Git** — for fetching dependencies
- **C++20 compatible compiler**

### macOS

- **Xcode 14+** (or Xcode Command Line Tools)

```bash
xcode-select --install
```

### Linux

Install the required development packages:

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libasound2-dev \
    libjack-jackd2-dev \
    libfreetype6-dev \
    libx11-dev \
    libxcomposite-dev \
    libxcursor-dev \
    libxext-dev \
    libxinerama-dev \
    libxrandr-dev \
    libxrender-dev \
    libwebkit2gtk-4.0-dev \
    libcurl4-openssl-dev \
    libgl1-mesa-dev
```

**Fedora:**
```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    git \
    alsa-lib-devel \
    jack-audio-connection-kit-devel \
    freetype-devel \
    libX11-devel \
    libXcomposite-devel \
    libXcursor-devel \
    libXext-devel \
    libXinerama-devel \
    libXrandr-devel \
    libXrender-devel \
    webkit2gtk3-devel \
    libcurl-devel \
    mesa-libGL-devel
```

### Windows

- **Visual Studio 2022** (with "Desktop development with C++" workload)
- Or **MSVC Build Tools 2022**

## Building

### Configure and Build

```bash
# Clone the repository
git clone <repo-url>
cd daw

# Configure (first run fetches JUCE, Tracktion Engine, and Catch2 — this takes a few minutes)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build (use --parallel for faster builds)
cmake --build build --config Release --parallel
```

### Build Output

The built application will be at:

| Platform | Path |
|----------|------|
| macOS | `build/DAW_artefacts/Release/DAW.app` |
| Linux | `build/DAW_artefacts/Release/DAW` |
| Windows | `build/DAW_artefacts/Release/DAW.exe` |

### Running Tests

```bash
ctest --test-dir build --build-config Release --output-on-failure
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `DAW_BUILD_TESTS` | `ON` | Build the test suite |
| `CMAKE_BUILD_TYPE` | `Debug` | Build type (Debug/Release/RelWithDebInfo) |

### IDE Setup

**Xcode (macOS):**
```bash
cmake -B build -G Xcode
open build/DAW.xcodeproj
```

**Visual Studio (Windows):**
```bash
cmake -B build
# Open build/DAW.sln in Visual Studio
```

**CLion / VS Code:** Open the project directory. CMake will be detected
automatically. Ensure `compile_commands.json` is generated (set
`CMAKE_EXPORT_COMPILE_COMMANDS=ON`, which is already the default).

## Troubleshooting

### First build is slow
The initial CMake configure fetches JUCE (~200MB), Tracktion Engine, and Catch2
via git. Subsequent builds use the cached dependencies in `build/_deps/`.

### macOS: "app is damaged" or Gatekeeper warning
The built app is unsigned. Right-click and select "Open" to bypass Gatekeeper, or:
```bash
xattr -cr build/DAW_artefacts/Release/DAW.app
```

### Linux: No audio output
Ensure ALSA or JACK is running. PipeWire with JACK compatibility also works.
