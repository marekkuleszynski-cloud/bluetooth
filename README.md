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
* buffer incoming PCM data using a ring buffer

Physical audio output is not implemented yet.

## Current Progress

* [x] PlatformIO project
* [x] Git / GitHub
* [x] Modular project structure
* [x] Bluetooth A2DP receiver
* [x] Bluetooth pairing and connection
* [x] PCM stream reception
* [x] Audio callback
* [x] Ring buffer
* [x] Buffer wrap-around
* [x] Full-buffer protection
* [x] Shared `AudioBuffer`
* [ ] AudioManager consuming PCM data
* [ ] I2S
* [ ] DAC
* [ ] Physical audio output
* [ ] OLED
* [ ] AVRCP
* [ ] OTA

## Development Status

The current development focus is the audio data pipeline:

```text
Bluetooth reception
        ↓
    PCM callback
        ↓
    AudioBuffer
        ↓
   AudioManager
```

The next step is to implement the consumer side of the buffer and connect it to the audio output pipeline.

## Documentation

* [Architecture](ARCHITECTURE.md)
* [Roadmap](ROADMAP.md)

## Hardware

Target platform:

* ESP32
* Bluetooth A2DP
* External DAC
* External amplifier

The exact DAC and amplifier configuration will be defined during the I2S and audio output stages.

## Development Environment

* PlatformIO
* C++
* ESP32 Arduino framework
* ESP32-A2DP library

The project is developed incrementally, with each subsystem tested before integrating the next layer.
