# ESP32 Bluetooth Audio Receiver

Bluetooth A2DP audio receiver built with ESP32 and PlatformIO.

The project is developed as a practical exercise in modern C++, embedded software architecture, ESP32 Bluetooth, buffering, and digital audio.

## Goals

* Learn PlatformIO
* Learn modern C++
* Learn ESP32 Bluetooth
* Learn audio buffering
* Learn I2S audio
* Learn embedded software architecture
* Build production-quality embedded software

## Status

🚧 Work in progress

The Bluetooth reception pipeline is already working.

The ESP32 can:

* advertise as an A2DP audio receiver
* connect to a phone
* receive PCM audio data
* pass PCM data through a callback
* buffer the data using a ring buffer

Physical audio output is not implemented yet.

## Architecture

```text
                         ESP32
                           │
                           ▼
                         main
                           │
            ┌──────────────┼──────────────┐
            │              │              │
            ▼              ▼              ▼
    BluetoothManager  AudioBuffer   AudioManager
            │              ▲              │
            │              │              │
            └─── write() ──┘              │
                           │               │
                           └── read() ─────┘
```

### Audio data flow

```text
Phone
  │
  │ Bluetooth A2DP
  ▼
BluetoothA2DPSink
  │
  │ PCM callback
  ▼
BluetoothManager
  │
  │ write()
  ▼
AudioBuffer
  │
  │ read()
  ▼
AudioManager
  │
  ▼
I2S
  │
  ▼
DAC
  │
  ▼
Amplifier
```

The I2S and physical audio output stages are planned but not implemented yet.

## Modules

### BluetoothManager

Responsible for Bluetooth A2DP.

* starts the Bluetooth receiver
* manages the Bluetooth connection
* receives PCM data
* writes PCM data to `AudioBuffer`

### AudioBuffer

Independent ring buffer responsible only for buffering PCM data.

Tested functionality:

* write
* read
* available data
* free space
* wrap-around
* full-buffer protection

Current test buffer size:

```text
8192 bytes
```

### AudioManager

Responsible for consuming buffered audio and managing audio-related functionality.

Planned functionality includes:

* PCM consumption
* audio output
* volume control
* connection tones
* searching tone
* disconnection tone

## Current State

### Working

* [x] ESP32 project
* [x] PlatformIO
* [x] Git/GitHub
* [x] Modular architecture
* [x] Bluetooth A2DP receiver
* [x] Phone pairing
* [x] PCM reception
* [x] PCM callback
* [x] Ring buffer
* [x] Buffer wrap-around
* [x] Full-buffer protection
* [x] Shared buffer between modules

### In Progress

* [ ] AudioManager consuming PCM data
* [ ] Producer/consumer synchronization
* [ ] I2S output
* [ ] DAC integration
* [ ] Physical audio output

### Planned

* [ ] OLED
* [ ] AVRCP
* [ ] OTA
* [ ] Equalizer
* [ ] Audio visualization

## Project Structure

```text
src/
├── main.cpp
├── BluetoothManager.cpp
├── AudioManager.cpp
└── AudioBuffer.cpp

include/
├── BluetoothManager.h
├── AudioManager.h
└── AudioBuffer.h
```

## Design Philosophy

The project follows a modular architecture.

Each module has one primary responsibility:

```text
BluetoothManager
    ↓
receives audio

AudioBuffer
    ↓
stores audio

AudioManager
    ↓
consumes/processes audio

I2S
    ↓
transports audio

DAC
    ↓
converts digital audio to analog

Amplifier
    ↓
drives the speaker
```

`main` acts as the system orchestrator and connects the modules together.

The modules do not unnecessarily own or copy each other's objects. Shared resources are passed explicitly, using references where a dependency is mandatory.

## Development

Built with:

* ESP32
* PlatformIO
* C++
* ESP32-A2DP library

The project is intentionally developed incrementally: each subsystem is tested independently before the next layer is introduced.
