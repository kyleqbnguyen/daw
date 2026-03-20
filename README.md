# DAW

An open-source digital audio workstation built with [JUCE](https://juce.com/) and [Tracktion Engine](https://github.com/Tracktion/tracktion_engine).

## Status

Early development (v0.1.0). Not yet usable for production.

## Tech Stack

- **Language:** C++20
- **Audio Framework:** JUCE 8
- **DAW Engine:** Tracktion Engine 3
- **Build System:** CMake 3.22+
- **Plugin Formats:** VST3, Audio Units, CLAP (planned)
- **Platforms:** macOS, Windows, Linux

## Building

See [BUILDING.md](BUILDING.md) for detailed instructions.

### Quick Start

```bash
git clone <repo-url>
cd daw
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

The built application will be in `build/DAW_artifacts/Release/`.

## License

This project is licensed under the GNU General Public License v3.0 — see the [LICENSE](LICENSE) file for details.
