---

# Custom Billy the Bass 🎣

This repository contains the code and setup instructions for converting a Billy the Bass into a programmable talking and singing fish using an ESP32 microcontroller.

## Features
- Control Billy the Bass movements and audio playback via ESP32.
- Audio output through DAC.
- SD card integration for storing audio files.
- Customizable wiring and configuration.

---

## Setup Instructions

### Prerequisites
- **Hardware**: ESP32-WROOM (or compatible ESP32 board), SD card module, DAC-compatible audio output, and amplifier (1–2W recommended).
- **Tools**: Soldering iron, glue gun, screwdriver, and basic tools for crafting/assembly.

---

### Code Configuration
1. If using an **ESP32-WROOM**, no changes are required.
2. For other ESP32 variants:
   - Verify the SD card wiring for compatibility and update the output pins in the code.
   - Ensure the audio output is connected to a **DAC pin** on the ESP32. Check your board’s documentation for pin configurations.
3. Upload the code to your ESP32 using the Arduino IDE or your preferred environment.

---

### Wiring Diagrams

#### SD Card Connection
Ensure the SD card is correctly wired as shown in the image below. Proper wiring ensures smooth playback of audio files.

![SD Card Wiring](https://alexlubbock.com/images/esp32-sd-card-wiring.jpg)

#### Control Circuit
The control circuit wiring, including motor can be referenced in the following diagram. Make adjustments as needed for your specific setup.

![Control Circuit](https://github.com/leonZtiger/custom-billy-the-bass/blob/main/Screenshot.png?raw=true)

**Note**: The amplifier circuit is not included in this guide. Purchase a standalone amplifier in the **1-2 watt range** for optimal audio quality.

---

### Assembly Instructions

1. **Component Placement**:
   - Connect all components according to the wiring diagrams.
   - Ensure all connections are secure before proceeding.
   - Insolate the wiring to make sure no circuit shorts can happen.

2. **Test Fit**:
   - Place the components on Billy the Bass’s backboard to confirm proper fit.
   - Trim or saw off parts of the backboard as necessary to accommodate the speaker or other components.

3. **Glue and Secure**:
   - After confirming everything fits, glue the components inside the board securely.
   - Screw the backboard together once all parts are in place.

4. **Power On**:
   - Power on the ESP32 and test the setup. Ensure audio plays correctly and all movements are functional.

---

## Troubleshooting
- **No Audio**: Verify that the audio output is connected to a DAC pin and the amplifier is functioning.
- **SD Card Issues**: Ensure the SD card is formatted correctly (FAT32) and files are in the correct directory.
- **Movement Errors**: Check motor and servo connections and adjust code if necessary.

---

## Additional Notes
- For best results, format your audio files correctly, 8-bit .wav file with a sample rate of 16k.
- I Experiencing clipping, lower the amplitude in the audio files.
- Experiment with code to customize Billy’s movements and behavior.

---

Happy fishing! 🛠️🐟

--- 
