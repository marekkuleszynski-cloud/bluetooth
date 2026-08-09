# Roadmap

## Sprint 0 — Project Foundation

* [x] PlatformIO
* [x] Git
* [x] GitHub
* [x] Project structure
* [x] Modular C++ architecture
* [x] `BluetoothManager`
* [x] `AudioBuffer`
* [x] `AudioManager` foundation
* [x] Shared buffer architecture
* [x] Ring buffer tests
* [x] Bluetooth PCM reception

## Sprint 1 — Bluetooth & Audio Data Pipeline

* [x] Bluetooth A2DP pairing
* [x] Bluetooth connection
* [x] PCM stream callback
* [x] PCM data reception
* [x] AudioBuffer integration
* [x] Producer/consumer architecture
* [x] AudioManager buffer consumption
* [x] Buffer underrun/overflow strategy
* [ ] Connection state handling
* [ ] Connection/disconnection events

## Sprint 2 — I2S

* [ ] Understand ESP32 I2S architecture
* [ ] Configure I2S
* [ ] Connect AudioManager to I2S
* [ ] Implement PCM → I2S data path
* [ ] Verify sample rate
* [ ] Verify channel configuration
* [ ] Verify bit depth
* [ ] Test continuous streaming

## Sprint 3 — Audio Output

* [ ] Connect DAC
* [ ] Connect amplifier
* [ ] Implement physical audio output
* [ ] Verify stable playback
* [ ] Tune audio buffering
* [ ] Handle buffer underruns
* [ ] Implement volume control
* [ ] Implement connected tone
* [ ] Implement disconnected tone
* [ ] Implement searching/standby tone

## Sprint 4 — OLED

* [ ] OLED driver/module
* [ ] Display Bluetooth state
* [ ] Display connection status
* [ ] Display volume
* [ ] Display playback state
* [ ] Display basic audio information
* [ ] Keep visualization separate from audio processing

## Sprint 5 — AVRCP

* [ ] AVRCP connection
* [ ] Play/pause
* [ ] Next track
* [ ] Previous track
* [ ] Remote volume control
* [ ] Track metadata
* [ ] Integrate AVRCP state with UI

## Sprint 6 — OTA

* [ ] OTA architecture
* [ ] Firmware update mechanism
* [ ] Validate update process
* [ ] Recovery strategy
* [ ] Document OTA procedure

## Future / Optional

These features are intentionally outside the current core scope:

* [ ] Equalizer
* [ ] Audio visualization
* [ ] Advanced DSP
* [ ] Additional codecs
* [ ] Advanced power management
* [ ] Web configuration
* [ ] Wi-Fi configuration
