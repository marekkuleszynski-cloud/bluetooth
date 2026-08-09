# Architecture

## Overview

The project is structured as a set of independent modules with clearly separated responsibilities.

The `main` module acts as the system orchestrator. It creates the components and connects their dependencies, but does not implement Bluetooth or audio processing logic itself.

```text
                         main
                          │
          ┌───────────────┼────────────────┐
          │               │                │
          ▼               ▼                ▼
 BluetoothManager    AudioBuffer      AudioManager
          │               ▲                │
          │               │                │
          └─── write() ──►│◄── read() ────┘
                          │
                     PCM audio data
```

## Data Flow

The current audio data flow is:

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
  │
  ▼
Audio Output
```

The actual audio output is not implemented yet.

## Modules

### main

`main` is responsible for system composition and orchestration.

Responsibilities:

* create system modules
* initialize modules
* connect module dependencies
* control the main execution flow

`main` should not contain Bluetooth implementation or audio processing logic.

Current objects:

```cpp
AudioBuffer audioBuffer;
BluetoothManager bluetooth(audioBuffer);
AudioManager audio(audioBuffer);
```

### BluetoothManager

Responsible for Bluetooth A2DP functionality.

Responsibilities:

* initialize Bluetooth A2DP
* start the Bluetooth sink
* manage Bluetooth connection
* receive PCM audio data
* forward received PCM data to `AudioBuffer`

The manager does not own the `AudioBuffer`.

It receives a reference to an existing buffer:

```cpp
BluetoothManager(AudioBuffer& buffer);
```

This keeps ownership separate from data production.

### AudioBuffer

`AudioBuffer` is an independent ring buffer used to decouple the Bluetooth producer from the audio consumer.

Responsibilities:

* store PCM data
* write incoming data
* read buffered data
* track available data
* track free space
* handle buffer wrap-around
* prevent writes when the buffer is full

The buffer has been tested for:

* normal write/read operation
* partial reads
* wrap-around
* full-buffer behavior

Current test capacity:

```text
8192 bytes
```

The buffer successfully handles the situation where Bluetooth continues producing data after the buffer becomes full:

```text
Received: 4096 | Written: 4096 | Buffer: 4096
Received: 4096 | Written: 4096 | Buffer: 8192
Received: 4096 | Written: 0    | Buffer: 8192
```

### AudioManager

`AudioManager` is responsible for audio consumption and audio-related functionality.

Planned responsibilities:

* consume PCM data from `AudioBuffer`
* control audio output
* control volume
* generate connection/search/disconnection tones
* later provide a suitable interface for additional audio processing

Current functionality:

* volume management
* buffer dependency
* initial consumer implementation

Not yet implemented:

* physical audio output
* I2S
* DAC integration
* tones

## Producer / Consumer Model

The architecture follows a producer/consumer model.

```text
             PRODUCER
                │
                │ PCM
                ▼
        ┌───────────────┐
        │  AudioBuffer  │
        └───────────────┘
                ▲
                │ PCM
                │
             CONSUMER
```

`BluetoothManager` produces PCM data.

`AudioManager` consumes PCM data.

`AudioBuffer` provides temporary storage between them.

This decouples the timing of Bluetooth reception from audio output processing.

## Dependency Ownership

The `AudioBuffer` is created by `main`.

Neither `BluetoothManager` nor `AudioManager` owns the buffer.

Both modules receive a reference:

```text
main
 │
 ├── owns AudioBuffer
 │
 ├── BluetoothManager ──► AudioBuffer
 │
 └── AudioManager ──────► AudioBuffer
```

References are used because both modules require an existing buffer and there is no meaningful "no buffer" state.

No copy of `AudioBuffer` is created.

## Bluetooth Callback

The ESP32-A2DP library provides PCM data through a callback:

```cpp
set_stream_reader(callback, false);
```

The callback receives:

```cpp
const uint8_t* data
uint32_t length
```

The current implementation forwards the data to the shared `AudioBuffer`.

Because the library expects a plain function pointer, the callback is implemented as a static member and routed to the active `BluetoothManager` instance.

## Design Principles

The project follows these principles:

1. Single responsibility per module.
2. `main` orchestrates but does not implement subsystem logic.
3. Modules communicate through explicit interfaces.
4. Ownership should be clear.
5. Avoid unnecessary copying of objects and data.
6. Bluetooth reception should be decoupled from audio output.
7. Real-time audio processing should not depend on blocking operations.
8. Additional functionality should be introduced incrementally and tested independently.

## Current Architecture Status

Implemented and tested:

* PlatformIO project
* Git/GitHub workflow
* modular project structure
* Bluetooth A2DP sink
* Bluetooth connection
* PCM reception
* PCM callback
* shared `AudioBuffer`
* ring buffer write/read
* wrap-around
* full-buffer handling
* producer/consumer architecture foundation

Not implemented yet:

* I2S
* DAC
* physical audio output
* audio tones
* AVRCP
* OLED
* OTA
* equalizer
* visualization
