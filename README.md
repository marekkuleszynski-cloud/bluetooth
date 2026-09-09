# ESP32 Bluetooth Audio Receiver

Bluetooth A2DP audio receiver built with ESP32 and PlatformIO.

The project is developed as a practical exercise in modern C++, embedded software architecture, ESP32 Bluetooth, buffering, I2S, and digital audio.

## Goals

- Learn PlatformIO
- Learn modern C++
- Learn ESP32 Bluetooth
- Learn audio buffering
- Learn I2S audio
- Learn embedded software architecture
- Build production-quality embedded software

## Status

🚧 Work in progress

The complete Bluetooth-to-audio pipeline is now implemented.

The ESP32 can:

- advertise as an A2DP audio receiver
- pair and connect with a phone
- receive PCM audio data
- buffer incoming PCM data using a ring buffer
- consume buffered PCM data through `AudioManager`
- transmit PCM data through I2S
- output physical audio through an external DAC / I2S amplifier

The current development focus is audio output stability and hardware integration.

## Current Progress

- [x] PlatformIO project
- [x] Git / GitHub
- [x] Modular project structure
- [x] Bluetooth A2DP receiver
- [x] Bluetooth pairing and connection
- [x] PCM stream reception
- [x] Audio callback
- [x] Ring buffer
- [x] Buffer wrap-around
- [x] Full-buffer protection
- [x] Shared `AudioBuffer`
- [x] AudioManager consuming PCM data
- [x] I2S output
- [x] External DAC
- [x] Physical audio output
- [ ] Audio output stability testing
- [ ] Buffer underrun / overflow diagnostics
- [ ] Final hardware wiring

### Optional

- [ ] OLED
- [ ] AVRCP
- [ ] OTA

## Architecture

The current audio pipeline is:

```text
Phone / Spotify
      ↓
Bluetooth A2DP
      ↓
BluetoothManager
      ↓
PCM callback
      ↓
AudioBuffer
      ↓
AudioManager
      ↓
I2SManager
      ↓
ESP32 I2S
      ↓
External DAC / I2S amplifier
      ↓
Analog audio / speaker