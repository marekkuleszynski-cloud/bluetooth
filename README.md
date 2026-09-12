# ESP32 Bluetooth Audio Receiver

Bluetooth A2DP audio receiver built with ESP32 and PlatformIO.

The project is developed as a practical exercise in modern C++, ESP32 Bluetooth, I2S digital audio, embedded software architecture, and hardware integration.

## Goals

- Learn PlatformIO
- Learn modern C++
- Learn ESP32 Bluetooth
- Learn I2S audio
- Learn embedded software architecture
- Understand real-time audio streaming on ESP32
- Build a stable Bluetooth audio receiver

## Status

🚧 Work in progress

The main Bluetooth audio pipeline is operational and stable.

The ESP32 can:

- advertise as an A2DP audio receiver
- pair and connect with a smartphone
- receive Bluetooth A2DP audio
- decode incoming audio to PCM
- stream PCM audio directly to the ESP32 I2S peripheral
- output audio through an external I2S DAC or amplifier

A previous custom buffering layer was removed after testing showed that the ESP32-A2DP / I2S stack already provides the buffering and streaming mechanisms required for stable playback.

Removing the duplicate buffering layer significantly improved audio stability.

## Current Progress

- [x] PlatformIO project
- [x] Git / GitHub
- [x] ESP32 Bluetooth A2DP receiver
- [x] Bluetooth pairing and connection
- [x] PCM audio reception
- [x] I2S output
- [x] PCM5102A DAC
- [x] MAX98357A I2S amplifier testing
- [x] Physical audio output
- [x] Stable direct A2DP → I2S streaming
- [x] Hardware signal integrity testing
- [ ] Final hardware assembly
- [ ] Volume control
- [ ] Connection / status handling

## Optional Features

- [ ] OLED
- [ ] AVRCP
- [ ] OTA
- [ ] Status tones

## Architecture

The current audio pipeline is intentionally simple:

```text
Smartphone
    ↓
Bluetooth A2DP
    ↓
ESP32-A2DP
    ↓
ESP32 I2S peripheral
    ↓
PCM5102A / MAX98357A
    ↓
Amplifier / speaker
```

The Bluetooth A2DP library and ESP32 audio stack handle the real-time audio buffering required for playback.

A custom ring buffer was previously implemented as a learning exercise, but was removed from the production audio path because it duplicated functionality already provided by the underlying stack.

## Development Findings

During development, a custom audio pipeline was tested:

```text
Bluetooth callback
    ↓
AudioBuffer
    ↓
AudioManager
    ↓
I2SManager
    ↓
DAC
```

Although the custom buffer worked in isolated tests, the additional producer / consumer layer introduced unnecessary complexity and audio instability.

A direct implementation:

```text
BluetoothA2DPSink
    ↓
I2SClass
```

proved significantly more stable.

This reinforced an important embedded design principle:

> Avoid duplicating buffering and real-time scheduling mechanisms already provided by the underlying framework unless the additional layer is required by the application.

## Hardware

Current development hardware:

- ESP32-WROOM-32 development board
- PCM5102A I2S DAC
- MAX98357A I2S DAC / Class-D amplifier
- External amplifier
- Speaker
- Smartphone as Bluetooth A2DP source

A Wemos D1 R32 ESP32 board was also tested, but showed less stable Bluetooth audio streaming than the standard ESP32-WROOM development board.

## I2S Wiring

```text
ESP32                 I2S DAC
--------------------------------
GPIO26   -----------> BCLK
GPIO25   -----------> LRCK / WS
GPIO22   -----------> DIN
GND      -----------> GND
```

Current I2S configuration:

```text
Sample rate:  44.1 kHz
Format:       16-bit PCM
Channels:     Stereo
BCLK:         GPIO26
LRCK / WS:    GPIO25
DATA OUT:     GPIO22
```

Short signal wires and solid ground connections are important for reliable I2S operation.

## PCM5102A

Typical control configuration used during testing:

```text
XMT  -> 3.3V
FMT  -> GND
SCK  -> GND
DMP  -> GND
FLT  -> GND
```

## MAX98357A

Typical test configuration:

```text
VIN  -> 5V
GND  -> GND
BCLK -> GPIO26
LRC  -> GPIO25
DIN  -> GPIO22
```

`SD` can be pulled high for deterministic operation.

`GAIN` may be left unconnected for the default gain configuration.

## Software

Main software components:

- ESP32 Arduino framework
- ESP32-A2DP
- ESP_I2S
- PlatformIO

The current implementation streams audio directly from `BluetoothA2DPSink` to `I2SClass`.

## Development Environment

- Ubuntu
- Visual Studio Code
- PlatformIO
- C++
- ESP32 Arduino framework
- ESP32-A2DP

Arduino-ESP32 `3.3.8` is currently used for reproducible audio testing.

## Documentation

- [Architecture](ARCHITECTURE.md)
- [Roadmap](ROADMAP.md)

## Lessons Learned

Several issues encountered during development were caused by both software architecture and physical prototype limitations.

Important findings included:

- shorter I2S wiring improves signal integrity
- soldered connections are more reliable than loose breadboard wiring
- twisted signal / ground pairs can reduce interference
- solid common ground is important
- unnecessary buffering layers can introduce timing and synchronization problems
- testing subsystems independently is an effective way to isolate problems

The project is developed incrementally, with each subsystem tested independently before integration.