# Architecture

## Overview

The ESP32 Bluetooth Audio Receiver uses a direct Bluetooth A2DP to I2S audio pipeline.

The architecture deliberately avoids adding a custom PCM buffering layer because the ESP32-A2DP and ESP32 I2S stack already provide the buffering and real-time mechanisms required for continuous audio playback.

## Audio Pipeline

```text
Smartphone
    ↓
Bluetooth A2DP
    ↓
BluetoothA2DPSink
    ↓
I2SClass
    ↓
ESP32 I2S peripheral
    ↓
External I2S DAC / amplifier
    ↓
Analog audio / speaker
```

## Bluetooth Layer

Bluetooth audio reception is handled by the `ESP32-A2DP` library.

The ESP32 operates as an A2DP sink and receives audio from a smartphone or another Bluetooth audio source.

The library handles:

- Bluetooth discovery
- pairing
- A2DP connection
- SBC decoding
- PCM audio generation
- audio stream handling

## I2S Layer

Decoded PCM audio is passed directly to the ESP32 I2S subsystem using `I2SClass`.

Current configuration:

```text
Sample rate: 44.1 kHz
Bit depth:   16 bit
Channels:    Stereo
BCLK:        GPIO26
LRCK / WS:   GPIO25
DATA OUT:    GPIO22
```

## Hardware Output

Two I2S audio devices were tested during development.

### PCM5102A

Used as a stereo line-level DAC.

```text
ESP32 GPIO26 -> BCK
ESP32 GPIO25 -> LRCK
ESP32 GPIO22 -> DIN
```

The analog output is connected to an external amplifier.

Typical control configuration:

```text
XMT -> 3.3V
FMT -> GND
SCK -> GND
DMP -> GND
FLT -> GND
```

### MAX98357A

Used as an I2S DAC with an integrated Class-D speaker amplifier.

```text
ESP32 GPIO26 -> BCLK
ESP32 GPIO25 -> LRC
ESP32 GPIO22 -> DIN
```

## Removed Custom Buffer Layer

An earlier architecture used a custom producer / consumer pipeline:

```text
Bluetooth callback
    ↓
AudioBuffer
    ↓
AudioManager
    ↓
I2SManager
```

`AudioBuffer` implemented a custom ring buffer.

The design was useful for learning:

- circular buffers
- producer / consumer architecture
- PCM data flow
- buffer overflow protection
- modular C++ design
- unit testing

However, in the final audio path it duplicated buffering already provided by the Bluetooth and I2S stack.

It also introduced additional synchronization and timing concerns between the Bluetooth task and the audio consumer.

Testing showed that direct A2DP-to-I2S streaming was significantly more stable.

The custom buffer layer was therefore removed from the runtime architecture.

## Current Design Principle

The current design follows a simpler architecture:

```text
ESP32-A2DP library
        ↓
Real-time audio handling
        ↓
I2S peripheral
        ↓
External audio hardware
```

Custom software layers are added only when they provide functionality required by the application.

Possible future reasons to introduce an intermediate audio processing layer include:

- DSP
- equalizer
- software volume processing
- audio mixing
- sound effects
- generated notification tones

Until such functionality is required, the direct audio path is preferred.

## Hardware Design Considerations

I2S signal integrity proved important during development.

The prototype uses:

- short signal wires
- soldered connections
- solid common ground
- twisted signal / ground pairs where practical
- local power supply decoupling

The BCLK line is particularly sensitive to poor wiring because fast digital edges can cause ringing and reflections even when the nominal clock frequency is relatively low.

## Board Selection

A standard ESP32-WROOM development board provided stable Bluetooth A2DP and I2S operation.

A Wemos D1 R32 ESP32 board was also tested but showed intermittent audio interruptions in the same application.

The standard ESP32-WROOM board is therefore currently used as the reference development platform.

## Development Approach

The project is developed incrementally.

Each subsystem is tested independently before integration.

The debugging process follows this principle:

```text
Bluetooth
    ↓
verify independently

I2S
    ↓
verify independently

DAC / amplifier
    ↓
verify independently

Integrated system
```

This approach made it possible to distinguish software timing problems from hardware signal-integrity problems.