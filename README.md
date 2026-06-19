# Zwissor Zero

**A portable mini hacking device capable of altering WiFi, SubGHz, BLE5, IR, and more — built for security research and wireless protocol testing.**

![Platform](https://img.shields.io/badge/Platform-ESP32--S3--Mini--Zero-blue)
![Version](https://img.shields.io/badge/Version-V4-green)
![License](https://img.shields.io/badge/License-Open%20Source-orange)

---

## 📖 Overview

**Zwisser Zero** is a powerful, portable multi-protocol wireless security research tool built on the ESP32-S3-Zero microcontroller. It combines WiFi, Bluetooth Low Energy (BLE5), and Infrared (IR) capabilities into a compact, handheld device with an OLED display and intuitive 5-way navigation.

Designed for security researchers, penetration testers, and wireless protocol enthusiasts, Zwisser Zero provides a comprehensive suite of tools for analyzing, testing, and experimenting with various wireless communication protocols.

---

## ✨ Features

### 📡 WiFi Attack Suite
- **Deauthentication Attacks** - Send deauth frames with customizable rate, channel, and reason codes
- **Beacon Flooding** - Spam fake APs with spoofed encryption (OPEN/WPA2/WPA3/WPA2-ENT)
- **Probe Request Sniffing** - Live capture and monitor probe requests from nearby devices
- **Channel Hopping** - Automatic channel cycling for comprehensive coverage
- **Persistent Station Mode** - Remembers and auto-connects to saved networks

### 🔵 BLE5 Attack & Spam Module
- **Apple AirTag Spam** - Broadcast fake AirTag advertisements
- **Samsung SmartTag Spam** - Emulate SmartTag devices
- **Microsoft Swift Pair Spam** - Trigger Windows pairing popups
- **Google Fast Pair Spam** - Flood Android devices with pairing requests
- **Apple iBeacon Spam** - Broadcast iBeacon frames
- **Google Eddystone URL Spam** - Push Eddystone-URL advertisements
- **BLE Pair Flood** - Random company ID spam
- **Name Randomizer** - Spoof device names from a list of 25+ real device names

### 🎮 IR Control Center
- **Universal IR Scanner** - Capture and decode IR signals
- **Raw IR Blaster** - Send custom IR codes
- **AC Control** - Control Gree, Coolix, Haier, Kelvinator AC units
- **TV-B-Gone** - Power-off code library for 10+ TV brands
- **CoolBeGone** - AC and fan power-off sequences (8 protocols)
- **IR Kill Switch** - Emergency IR shutdown
- **Custom Remotes Manager** - Create and persist remote control profiles
- **Signals Library** - Save, rename, search, and blast captured signals
- **Projector Control** - Power on/off for BenQ, Epson, Optoma, ViewSonic

### 🎨 Display & Interface
- **0.96" OLED Display** (128x64, landscape, 400KHz I2C)
- **5-Way Navigation** (UP/DOWN/LEFT/RIGHT/CENTER buttons)
- **8 Built-in Themes**: Matrix, Cyberpunk, Hacker, Deep Blue, Synthwave, Blood Dragon, Night Vision, Ghost
- **Screensaver Mode** - Auto-dims after 10s, turns off after 20s
- **Battery Monitor** - Real-time voltage and percentage display
- **USB-C Detection** - Shows charging status

### 🎵 Entertainment
- **Music Player** - 6 built-in melodies (Mario, Harry Potter, Imperial March, Tetris, Nokia, Fur Elise)
- **Tetris Game** - Full implementation with DAS, 7-bag randomizer, scoring
- **Super Mario** - Classic platformer stub
- **Morse Notebook** - Tap-to-type Morse code with buzzer feedback
- **Frequency Tester** - Buzzer tone generator (100-5000 Hz)

### 🌐 Web Interface
- **Responsive Web UI** - Control from any browser
- **Real-time Stats** - Temperature, CPU, RAM, PSRAM, WiFi RSSI
- **Live System Log** - Color-coded event log with auto-scroll
- **Theme Selector** - Change device theme from web
- **REST API** - Full JSON API for all functions

### 🔧 System Features
- **LittleFS Filesystem** - Persistent storage for remotes, signals, config
- **Preferences Storage** - Save WiFi creds, BT name, theme, AC brand
- **System Statistics** - Temp, CPU freq, RAM, PSRAM, uptime
- **Factory Reset** - Wipe all settings
- **OTA-ready Architecture** - Easy to extend with updates

---

## 🛠️ Hardware Requirements

| Component | Specification |
|-----------|---------------|
| **MCU** | ESP32-S3-Zero (or compatible ESP32-S3 board) |
| **Display** | 0.96" OLED (SSD1306, 128x64, I2C) |
| **IR Receiver** | TSOP38238 or similar (38kHz) |
| **IR Transmitter** | IR LED + driver transistor |
| **Buzzer** | Passive buzzer (GPIO 6) |
| **Buttons** | 5-way navigation switch (or 5 individual buttons) |
| **Power** | 3.7V LiPo battery (optional) + USB-C |
| **Battery Monitor** | Voltage divider (2x 100k resistors) on GPIO 1 |

### GPIO Pinout

| Function | GPIO | Notes |
|----------|------|-------|
| OLED SDA | GPIO 7 | I2C Data |
| OLED SCL | GPIO 8 | I2C Clock (400KHz) |
| IR Receiver | GPIO 4 | Input |
| IR Transmitter | GPIO 5 | Output |
| Button UP | GPIO 12 | INPUT_PULLUP |
| Button DOWN | GPIO 9 | INPUT_PULLUP |
| Button LEFT | GPIO 6 | INPUT_PULLUP |
| Button RIGHT | GPIO 11 | INPUT_PULLUP |
| Button CENTER | GPIO 10 | INPUT_PULLUP |
| Buzzer | GPIO 13 | Passive buzzer |
| Battery ADC | GPIO 1 | ADC1 (voltage divider) |

---

## 📦 Dependencies

Install these libraries via Arduino Library Manager or PlatformIO:

```
- WiFi (built-in)
- WebServer (built-in)
- NimBLE (ESP32 NimBLE Library)
- Preferences (built-in)
- IRremoteESP8266 (IRremote)
- IRsend (included with IRremote)
- IRrecv (included with IRremote)
- IRutils (included with IRremote)
- ir_Gree (included with IRremote)
- ir_Coolix (included with IRremote)
- ir_Haier (included with IRremote)
- ir_Kelvinator (included with IRremote)
- irac (included with IRremote)
- LittleFS (built-in)
- U8g2 (U8g2lib)
- ArduinoJson (ArduinoJson by Benoit Blanchon)
- Wire (built-in)
```

---

## 🚀 Installation

### Arduino IDE

1. **Install ESP32 Board Support**
   - Open Arduino IDE → File → Preferences
   - Add to "Additional Board Manager URLs":
     ```
     https://dl.espressif.com/dl/package_esp32s3_index.json
     ```
   - Board Manager → Install "ESP32S3 Dev Module"

2. **Select Board Settings**
   - Board: **ESP32S3 Dev Module**
   - Flash Frequency: **80MHz**
   - Flash Size: **16MB (128Mb)**
   - Partition Scheme: **Default 4MB with spiffs**
   - PSRAM: **OPI PSRAM** (if available)

3. **Install Libraries**
   - Open Library Manager (Ctrl+Shift+I)
   - Search and install:
     - **IRremote** by David Conran, Mark Szabo, et al.
     - **NimBLE** by h2zero
     - **U8g2** by oliver
     - **ArduinoJson** by Benoit Blanchon

4. **Upload Firmware**
   - Connect ESP32-S3-Zero via USB-C
   - Select correct COM port
   - Click **Upload**

### PlatformIO

```bash
# Clone repository
git clone https://github.com/ZalanLykos/Zwissor-Zero.git
cd zwisser-zero

# Install dependencies
pio lib install

# Build and upload
pio run --target upload

# Monitor serial
pio device monitor
```

---

## 🎯 Quick Start

### First Boot
1. Power on the device via USB-C or LiPo battery
2. The OLED will display the **ZWISSER ZERO** boot screen
3. The device creates a WiFi AP: `ZWISSER_ZERO_V4` (password: `zwissorzero`)
4. Connect to the AP and open the web interface at `http://192.168.4.1`

### Navigation
- **UP/DOWN/LEFT/RIGHT** - Navigate menus
- **CENTER** - Select / Confirm
- **LEFT (hold 1s)** - Go back
- **LEFT (hold 5s in games)** - Exit to menu

### Web Interface
Access the full control panel from any browser:
- **WiFi Control** - Scan, connect, configure AP
- **Bluetooth** - Scan, pair, manage devices
- **WiFi Attacks** - Deauth, beacon flood, probe sniff, channel hop
- **BLE Spam** - AirTag, SmartTag, Swift Pair, Fast Pair, iBeacon, Eddystone
- **IR Hub** - Scan, send, clone signals, control ACs
- **Themes** - 8 color schemes
- **Music** - Play buzzer melodies
- **System Stats** - Real-time monitoring

---

## 📚 API Reference

### WiFi Endpoints

```
GET  /api/wifi/status          - Get WiFi radio status
POST /api/wifi/toggle          - Toggle WiFi on/off
GET  /api/wifi/scan            - Scan nearby networks
POST /api/wifi/connect         - Connect to network
POST /api/wifi/config          - Save AP configuration
GET  /api/wifi/saved           - Get saved network
POST /api/wifi/forget          - Forget saved network
```

### Bluetooth Endpoints

```
GET  /api/bt/status            - Get BT status
POST /api/bt/toggle            - Toggle BLE on/off
GET  /api/bt/scan              - Scan BLE devices
POST /api/bt/name              - Set device name
POST /api/bt/pair              - Pair with device
GET  /api/bt/paired            - List paired devices
POST /api/bt/unpair            - Unpair device
```

### Attack Endpoints

```
POST /api/deauth/start         - Start deauth attack
POST /api/deauth/stop          - Stop deauth attack
POST /api/beacon/start         - Start beacon flood
POST /api/beacon/stop          - Stop beacon flood
POST /api/probe/start          - Start probe sniffer
POST /api/probe/stop           - Stop probe sniffer
GET  /api/probe/results        - Get captured probes
POST /api/hop/start            - Start channel hopping
POST /api/hop/stop             - Stop channel hopping
GET  /api/hop/status           - Get current channel
```

### BLE Spam Endpoints

```
POST /api/airtag/start         - Start AirTag spam
POST /api/airtag/stop          - Stop AirTag spam
POST /api/smarttag/start       - Start SmartTag spam
POST /api/smarttag/stop        - Stop SmartTag spam
POST /api/swiftpair/start      - Start Swift Pair spam
POST /api/swiftpair/stop       - Stop Swift Pair spam
POST /api/gfp/start            - Start Google Fast Pair spam
POST /api/gfp/stop             - Stop Google Fast Pair spam
POST /api/ibeacon/start        - Start iBeacon spam
POST /api/ibeacon/stop         - Stop iBeacon spam
POST /api/eddystone/start      - Start Eddystone spam
POST /api/eddystone/stop       - Stop Eddystone spam
POST /api/blespam/start        - Start BLE pair flood
POST /api/blespam/stop         - Stop BLE pair flood
POST /api/blespam/stopall      - Stop all BLE spam
POST /api/namerand/on          - Enable name randomizer
POST /api/namerand/off         - Disable name randomizer
```

### IR Endpoints

```
POST /api/ir/scan              - Toggle IR scanner
GET  /api/ir/last              - Get last captured signal
POST /api/ir/send              - Send IR signal
POST /api/ir/record            - Record IR signal (10s window)
POST /api/ir/gree              - Send Gree AC command
POST /api/ir/tvbgone/start     - Start TV-B-Gone
POST /api/ir/tvbgone/stop      - Stop TV-B-Gone
GET  /api/ir/state             - Get IR engine state
```

### System Endpoints

```
GET  /api/system/stats         - Get system statistics
POST /api/system/reboot        - Reboot device
POST /api/system/reset         - Factory reset
GET  /api/log                  - Get system log
POST /api/log/clear            - Clear system log
POST /api/theme                - Set theme
GET  /api/theme                - Get current theme
```

---

## 🎮 Games

### Tetris
- Full Tetris implementation with 7-bag randomizer
- DAS (Delayed Auto Shift) for smooth movement
- Wall kicks for rotation
- Lock delay and hard drop
- Score, level, and lines tracking
- Next piece preview

### Super Mario
- Classic platformer (stub implementation)
- World/level progression
- Score tracking
- Pause/resume functionality

---

## 🎨 Themes

| Theme | Description | Colors |
|-------|-------------|--------|
| **Matrix** | Classic hacker green on black | Green (#0F0) |
| **Cyberpunk** | Neon pink and cyan | Magenta/Cyan |
| **Hacker** | Amber terminal aesthetic | Orange (#FFB000) |
| **Deep Blue** | Oceanic blue tones | Blue (#00CCFF) |
| **Synthwave** | Retro 80s neon | Purple/Pink |
| **Blood Dragon** | Aggressive red theme | Red (#FF2200) |
| **Night Vision** | Military green night vision | Green (#33FF33) |
| **Ghost** | Minimalist gray/white | Gray (#CCCCCC) |

---

## 🔒 Legal & Ethical Use

### ⚠️ Disclaimer

**This tool is intended for security research, penetration testing, and educational purposes ONLY.**

- Only use this device on networks and systems you **own** or have **explicit written permission** to test.
- Unauthorized access to computer systems, networks, or wireless communications is **illegal** in most jurisdictions.
- The authors and contributors of this project are **not responsible** for any misuse or damage caused by this software.
- You are solely responsible for ensuring your use complies with all applicable laws and regulations.

### ✅ Acceptable Use Cases
- Security auditing of your own networks
- Penetration testing with written authorization
- Educational demonstrations in controlled environments
- Research on wireless protocol security
- Testing IoT device security

### ❌ Unacceptable Use Cases
- Attacking networks you don't own
- Interfering with emergency services
- Causing disruption to public networks
- Any illegal activity

---

## 🤝 Contributing

Contributions are welcome! Please follow these guidelines:

1. **Fork the repository**
2. **Create a feature branch** (`git checkout -b feature/AmazingFeature`)
3. **Commit your changes** (`git commit -m 'Add some AmazingFeature'`)
4. **Push to the branch** (`git push origin feature/AmazingFeature`)
5. **Open a Pull Request**

### Code Style
- Follow Arduino C++ conventions
- Comment your code clearly
- Test on actual hardware before submitting
- Update documentation for new features

---

## 📝 License

This project is released under the **MIT License**.

```
MIT License

Copyright (c) 2024 Zwissor Zero

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## 🙏 Credits

**Created by Zalan Lykos**

- **Website**: [zalanlykos.github.io](https://zalanlykos.github.io)
- **Contact**: 03295679004

### Special Thanks
- **ESP32 Community** - For excellent hardware and libraries
- **IRremote Library** - For comprehensive IR protocol support
- **NimBLE Team** - For efficient BLE implementation
- **U8g2 Library** - For OLED display support

---

## 📊 Project Stats

- **Language**: C++ (Arduino)
- **Platform**: ESP32-S3
- **Display**: 128x64 OLED
- **Connectivity**: WiFi 4, BLE 5.0, IR (38kHz)
- **Storage**: LittleFS (SPIFFS)
- **Size**: ~500KB firmware

---

## 🗺️ Roadmap

- [ ] Full Super Mario implementation
- [ ] Additional IR protocols (Samsung, LG, Sony, etc.)
- [ ] SubGHz support (433/315MHz)
- [ ] OTA (Over-The-Air) updates
- [ ] Bluetooth Classic support
- [ ] NFC support (if hardware available)
- [ ] GPS logging module
- [ ] SD card storage expansion
- [ ] More games (Snake, Pong, etc.)
- [ ] Custom theme creator

---

## 📸 Screenshots

*(Add screenshots of your device here)*

### Main Menu
![Main Menu](screenshots/main-menu.png)

### WiFi Attacks
![WiFi Attacks](screenshots/wifi-attacks.png)

### BLE Spam
![BLE Spam](screenshots/ble-spam.png)

### IR Control
![IR Control](screenshots/ir-control.png)

### Web Interface
![Web UI](screenshots/web-ui.png)

---

## 🐛 Known Issues

- Mario game is currently a stub (placeholder implementation)
- Some IR protocols may need tuning for specific devices
- Battery monitoring requires calibrated voltage divider
- BLE spam may interfere with nearby devices (use responsibly)

---

## 📞 Support

If you encounter issues or have questions:

1. **Check the documentation** - Review this README and code comments
2. **Search existing issues** - Look for similar problems on GitHub
3. **Open a new issue** - Provide detailed information about your problem
4. **Join discussions** - Share ideas and use cases

---

## ⭐ Star History

If you find this project useful, please consider giving it a star!

---

## 🔗 Related Projects

- [ESP32-S3-Zero](https://www.espressif.com/) - Main microcontroller
- [IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote) - IR library
- [NimBLE](https://github.com/h2zero/NimBLE) - BLE library
- [U8g2](https://github.com/olikraus/u8g2) - Display library

---

**Made with ❤️ by Zalan Lykos**
