# Marocco's BT Audio Receiver

A DIY **ESP32-WROOM Bluetooth stereo receiver** with a 5-band digital equalizer, stereo limiter, real-time spectrum analysis, and 17 visualizations on a **320 × 240 ST7789 TFT**. Written in modular C++ for **Arduino / PlatformIO**.

## Hardware

| Component | Connection / purpose |
| --- | --- |
| ESP32-WROOM (`esp32dev`) | Bluetooth Classic A2DP sink and main controller |
| PCM5102A DAC | I²S: BCLK **GPIO26**, LRCK/WS **GPIO25**, DATA **GPIO22** |
| ST7789 TFT | SPI: MOSI **GPIO23**, SCLK **GPIO18**, CS **GPIO5**, DC **GPIO27**, RST **GPIO4** |
| EC11 encoder | A **GPIO32**, B **GPIO33**, push **GPIO21** |
| KEY0 | **GPIO19**: DSP ON/OFF |

The TFT is configured in landscape orientation (`tft.setRotation(3)`, 320 × 240). Backlight is connected to 3.3 V. All modules must share ground.

## Features

- **Bluetooth A2DP** sink advertised as `Marocco's BT Audio Receiver`.
- **Stereo I²S output** to the PCM5102A (configured at 44.1 kHz / 16-bit).
- **5-band DSP equalizer**, adjustable **−12 to +12 dB**: 80 Hz low shelf, 250 Hz / 1 kHz / 4 kHz peaking, 10 kHz high shelf.
- **SMART BOOST**: no automatic fixed attenuation when boosting EQ bands; a shared **stereo peak limiter** reduces the risk of clipping while preserving L/R balance. Limiter ceiling: **0.98**; release: **160 ms**.
- **True PCM bypass** when DSP is disabled with KEY0.
- **Persistent EQ settings in ESP32 NVS**: all five band gains restore after restart. Changes are saved **3 seconds after the last adjustment**, not on each encoder detent. The implementation uses Arduino `Preferences` (`eq-settings` namespace, `gains-v1` key).
- **Visualization-only AGC** to reduce dependence on the source device's volume. It does **not** modify the signal sent to the DAC. Scope visualizations use per-frame normalization with shared L/R scaling. AGC cannot recover digital silence or very weak signals perfectly.
- **17 visualization modes**, including spectrum bars, dual analog gauges, Winamp-inspired themes and two PCM oscilloscopes.
- Neon **BT Audio Receiver** splash screen; the same artwork behind `WAITING FOR CONNECTION` and `WAITING FOR SIGNAL`. Status transitions are immediate (no text fade).
- Analog gauge scales **−15 to +6 dB** are visual reference scales, **not calibrated analog VU or absolute dBFS measurements**. Gauge rendering is capped at approximately 30 FPS.

### Visualization modes

1. Classic LED
2. Neon Dynamic
3. Center Bars
4. Mirror LED
5. Peak Hold
6. Rainbow LED
7. Retro Green
8. Vertical Rainbow
9. Color Wave
10. Pixel Matrix
11. Dual Gauge
12. Retro VU
13. Winamp Classic
14. Winamp Fire
15. Scope CRT
16. Scope Stereo
17. Equalizer

The actual order is controlled by `VisualMode.h` and must match `visualModeNames[]` in `DisplayManager.cpp` if reordered.

## Controls

| Control | Action |
| --- | --- |
| Rotate EC11 | Select visualization; adjust gain in EQ edit mode |
| Press EC11 on EQ screen | Enter EQ edit mode / cycle through EQ bands / exit |
| KEY0 | Toggle DSP ON/OFF |

## Audio and visualization data flow

```text
Phone / Bluetooth A2DP
         |
    SBC decode -> PCM stereo
         |
      AudioEngine
         |
   5-band EQ / SMART BOOST / stereo limiter (when DSP enabled)
         |
         +-----------------------> I²S -> PCM5102A -> amplifier
         |
         +--> visual levels / AGC -> Dual Gauge / Retro VU
         +--> PCM sample ring ----> Scope CRT / Scope Stereo
         +--> FFT, 16 bands -----> spectrum and Winamp visualizations
```

The visualizations are driven from audio data but are **not** in the time-critical I²S output path. Their gain and normalization do not change playback volume.

## Source layout

| File | Responsibility |
| --- | --- |
| `main.cpp` | Startup, splash screen, main service loop, NVS load/save service |
| `BluetoothManager.*` | A2DP connection state and PCM callback wiring |
| `AudioEngine.*` | Audio chunks, DSP/bypass, limiter, I²S output, visualization sample data |
| `Equalizer.*` | Biquad filters, EQ coefficients, NVS load and deferred save |
| `SpectrumAnalyzer.*` | FFT and 16-band spectral analysis |
| `DisplayManager.*` | ST7789 UI, status overlays, 17 visualization modes |
| `SplashScreen.h` | Flash-resident RGB565 splash artwork and drawing routine |
| `InputManager.*` | EC11 decoding and button inputs |
| `AppController.*` | UI actions; schedules EQ setting saves after gain changes |
| `AppState.*` | Shared UI/audio state and audio-detection thresholds |
| `VisualMode.h` | Visualization enum and `MODE_COUNT` |
| `HardwareConfig.h` | GPIO definitions |

## Build and flash

1. Open the project in **VS Code with PlatformIO** (`board = esp32dev`, Arduino framework).
2. Use the existing `platformio.ini` and project library versions (including **TFT_eSPI**, **ESP32-A2DP**, and **arduinoFFT**).
3. Configure TFT_eSPI for ST7789, 240 × 320, the listed SPI pins, and **BGR** order as required by the display setup.
4. The splash artwork increases firmware size. On a standard **4 MB ESP32** with USB flashing, this project uses the larger `no_ota` app partition:

   ```ini
   board_build.partitions = no_ota.csv
   ```

   **This partition layout removes the standard second OTA app slot.** Verify the actual flash capacity before applying it. Changing partitions can affect existing NVS data.
5. Optionally set faster serial upload (if supported by the USB-UART adapter):

   ```ini
   upload_speed = 921600
   monitor_speed = 115200
   ```

6. Build and upload (`pio run -t upload`). Connect a phone to **Marocco's BT Audio Receiver**.

## Verify saved EQ settings

1. Open the **Equalizer** visualization and change one or more band gains.
2. Leave the controls untouched for **at least 3 seconds**.
3. Serial Monitor (115200) should report: `[EQ NVS] Saved 5 bands`.
4. Restart or power-cycle the ESP32. The EQ values should be restored; Serial Monitor should report `[EQ NVS] Stored equalizer restored`.

NVS normally survives a routine firmware upload, **but not full flash erase** or some repartitioning operations.

## Development notes

- The latest tested EC11 decoder uses detent stabilization; changing to experimental FSM variants may cause missed or extra steps.
- Avoid heavy TFT/SPI operations in the Bluetooth audio callback. The gauges use bounded refresh to avoid earlier display freezes.
- Keep FFT-buffer and cross-task synchronization in mind when optimizing; a previous on-demand FFT experiment was rolled back because of instability.
- `VisualMode.h` controls display sequence; keep its enum, `MODE_COUNT`, and `visualModeNames[]` aligned.
- The RGB565 splash byte order is handled locally in `SplashScreen.h`; do not change global TFT color settings to compensate for the splash image.

*README updated 22 October 2026.*
