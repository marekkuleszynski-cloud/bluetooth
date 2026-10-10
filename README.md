# Marocco's BT Audio Receiver

An ESP32-based **Bluetooth stereo receiver** with real-time audio processing, a 5-band equalizer, and a 320 × 240 TFT spectrum/level display. Built as a modular Arduino/PlatformIO C++ project.

## Hardware

| Component | Connection |
| --- | --- |
| ESP32-WROOM | Main controller; Bluetooth Classic A2DP |
| PCM5102A DAC | I2S: BCLK **GPIO26**, LRCK **GPIO25**, DATA **GPIO22** |
| ST7789 TFT (240 × 320, landscape) | SPI: MOSI **GPIO23**, SCLK **GPIO18**, CS **GPIO5**, DC **GPIO27**, RST **GPIO4** |
| EC11 rotary encoder | A **GPIO32**, B **GPIO33**, push **GPIO21** |
| KEY0 | **GPIO19**, toggles audio processing |

TFT backlight is connected to 3.3 V. The DAC, display, and ESP32 require common ground. Display orientation: `tft.setRotation(3)` (320 × 240).

## Main features

- Bluetooth A2DP sink: **`Marocco's BT Audio Receiver`**.
- Stereo 44.1 kHz / 16-bit I2S output to PCM5102A.
- Five-band DSP EQ: **80 Hz** (low shelf), **250 Hz**, **1 kHz**, **4 kHz** (peaking), **10 kHz** (high shelf); adjustable **±12 dB**.
- KEY0 toggles DSP **ON/OFF**; OFF uses a direct PCM bypass.
- **12 display modes:** Classic LED, Neon Dynamic, Center Bars, Mirror LED, Peak Hold, Rainbow LED, Retro Green, Vertical Rainbow, Color Wave, Pixel Matrix, Equalizer, and Dual Gauge.
- Dual Gauge: independent left/right peak-driven analog needles, light metallic 3D faces, and an approximately **30 FPS** rendering cap.
- Consistent on-screen header showing the selected visualization and DSP state.
- Automatic Bluetooth waiting / audio-waiting screens, with startup fade effects.

## Controls

| Input | Action |
| --- | --- |
| Rotate EC11 | Switch visualization; in EQ edit mode, adjust the selected band's gain |
| Press EC11 on EQ screen | Enter EQ edit mode, advance through bands, then exit |
| KEY0 | Toggle DSP ON/OFF (EQ editing is disabled while DSP is OFF) |

## Code layout (`src/`)

| File | Responsibility |
| --- | --- |
| `main.cpp` | Startup and main loop |
| `AppState.*`, `AppController.*` | UI state, screen transitions, controls |
| `HardwareConfig.h` | GPIO assignments |
| `BluetoothManager.*` | Bluetooth A2DP connection and callbacks |
| `AudioEngine.*` | PCM processing, DSP/bypass, I2S, stereo level measurement |
| `Equalizer.*` | Five EQ filters and coefficients |
| `SpectrumAnalyzer.*` | FFT and frequency-band analysis |
| `DisplayManager.*` | TFT rendering, EQ screen, visualizations, dual gauges |
| `InputManager.*` | EC11 quadrature decoder and buttons |
| `VisualMode.h` | Visualization enum and `MODE_COUNT` |

## Useful settings

- **Audio silence timeout:** `AUDIO_SIGNAL_TIMEOUT_MS` in `AppState.h` (currently **500 ms**). Increase it to avoid switching to *Waiting for audio signal* during short pauses.
- **Silence threshold:** `AUDIO_SILENCE_THRESHOLD` in `AppState.h` (currently **0.0020**).
- **Gauge sensitivity:** `GAUGE_VISUAL_GAIN_DB` in `DisplayManager.cpp` (latest value **18.0f**, display only).
- **Gauge frame interval:** `GAUGE_REFRESH_MS` in `DisplayManager.cpp` (**33 ms**, ~30 FPS).
- **Visualization order:** `VisualMode.h`; keep `MODE_COUNT` last and maintain the matching `visualModeNames[]` order in `DisplayManager.cpp`.
- **Default visualization:** `visualMode` in `AppState.cpp` (`CLASSIC_LED` in the current source).

## Build and run

1. Open the project in **VS Code + PlatformIO** using the ESP32 `esp32dev` environment.
2. Ensure the project dependencies include **TFT_eSPI**, **ESP32-A2DP**, and **arduinoFFT**, matching the firmware's existing PlatformIO configuration.
3. Configure TFT_eSPI for **ST7789**, 240 × 320, BGR color order, and the pins above.
4. Build, flash, and connect a Bluetooth audio source to **`Marocco's BT Audio Receiver`**.
5. Open Serial Monitor at **115200 baud** when debugging.

## Development notes / known limitations

- The EC11 can occasionally miss a mechanical detent. Keep the previously tested quadrature decoder (`InputManager.cpp`) when updating visualizations; avoid replacing it with an older project snapshot.
- Dual Gauge previously caused freezes with more aggressive rendering. The current working approach limits drawing to ~30 FPS. Avoid long, uninterrupted SPI drawing transactions.
- The main loop still performs FFT processing even when some screens do not need its output. An attempted on-demand FFT optimization was rolled back after instability; revisit only with safe cross-core synchronization and testing.
- Drawing optimizations matter on this display: redraw changing elements rather than entire screens to reduce flicker.
- This README reflects the project state at the end of **10 October 2026**; verify behavior on physical ESP32 hardware after future changes.
