
/*
    RCntrl firmware v2 by Omadon
    
    Version 2.31

    Custom firmware for the StylesRallyIndustries Bluetooth Navigation / Digital Roadbook Controller 
    
    kresimir.petrovic@gmail.com

    https://www.instructables.com/Bluetooth-Navigation-Digital-Roadbook-Controller-4/
    https://github.com/StylesRallyIndustries/RallyController/releases/tag/v1.1

    The code was based on the BarButtons (firmware v1)
    https://jaxeadv.com/barbuttons-files/barbuttons.ino 
    
    Required Libraries:
      NimBLE-Arduino
      HijelHID_BLEKeyboard
      Keypad
    
    Tested on:
      NimBLE-Arduino 2.5.0
      HijelHID_BLEKeyboard 0.5.0
      Keypad 3.1.1
    Boards: esp32 3.3.8

    Myboard: ESP-WROOM-32 30pin USBC
      Board: ESP32 Dev Module
      CPU Frequency: 240 MHz
      Flash Frequency: 40 MHz
      Flash Mode: QIO
      Flash Size: 4MB
      Partition Scheme: Minimal with OTA


  Difference from the original firmware:
    - Uses the Keypad library (with event handler that simplifies button state handling).
    - void Keypad::scanKeys() was modifed and added delay(4) to compensate for longer wires
    - Uses HijelHID compatible BLE keyboard
    - Support for two controllers in daisy chain, connected to each other or to the controller box.
    - With keypad scanning we can daisy chain controllers using UTP 
      (5/6 wires for buttons + 2 wires for the LED).
    - Up to 8 different profiles:
        * Short press of buttons 1–8 → select profiles 1–8.
        * Long press of buttons 1–4 → select profiles 5–8.
    - Button mapping tables (normal + media) are split into two halves:
        * First half → short press mappings.
        * Second half → long press mappings.
        * On long press, firmware checks for a dedicated long-press mapping and uses it if defined,
          otherwise the short-press behavior repeats.
    - To change profiles, press sequence 1-2-3-4 and then the button number of the desired profile.
    - Profile change sequence can be modified as needed.
    - Selected profile is saved in the NVS, surviving controller reboot.
    - Profiles can send either normal keys (letters, numbers, arrows, etc.) or media keys 
      (volume/playback). Lookup order is always: normal → media.
    - Change BTLE advertisement based on the profile selected: 
      e.g. "RCntrl V2 P.1", "RCntrl V2 P.2", ...
        * iOS users: be aware of BTLE advertisement limitations.
    - Support for external or internal LED:
        * Blinking when not connected to BT.
        * Optional keepalive blinking (default 10h).
        * All blinking intervals can be tuned as needed.
        * Blink on each button press.
        * When changing profile, LED blinks to report the current profile; 
          after 1-2-3-4 it will blink the number of the selected profile.

    Set DEBUG to 0 for production use.

    This work is licensed under the Creative Commons Attribution-NonCommercial 4.0 International License. 
    To view a copy of this license, visit http://creativecommons.org/licenses/by-nc/4.0/ 
    or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
*/

// Debug flag, set to 0 in production. Adjust baud rate if required
const int DEBUG = 1;
const int BaudRate = 460800;

// Firmware version
const int firmware_version_major = 2;
const int firmware_version_minor = 31;

#include <HijelHID_BLEKeyboard.h>
#include <BLEHIDKeys.h>
#include <BLEHIDMediaKeys.h>
#include "keymappings.h"
#include <Keypad.h> // Keypad library to handle matrix keypad setup
#include <Preferences.h> // Used to save last used profile

Preferences preferences;

// Keypad library settings
// Support for 9 buttons, we will implement two RC controllers with 4 buttons each
// The idea is to daisy chain both controllers with UTP cable
// RC1 buttons 1-4, RC2 buttons 5-8
const byte ROWS = 3;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'}
};

// Connection diagram is very simple, follow the comments below or matrix above
// 1L, (button 1, left pin), 1R, (button 1, right pin)
// Make jumper connection from button to button, don't aggregate to the ESP32 pin
byte colPins[COLS] = {25, 26, 27}; // 25->(1L,4L,7L), 26->(2L,5L,8L), 27->(3L,6L,9L)
byte rowPins[ROWS] = {18, 19, 21}; // 18->(1R,2R,3R), 19->(4R,5R,6R), 21->(7R,8R,9R)

//#define LED_PIN 5    //   status led, PIN2 for internal LED
#define LED_PIN 13
int active_profile = 0; // Currently active profile

// Sequence for changing profile
#define PROFILE_SEQ_LENGTH 4
char profile_change_seq[PROFILE_SEQ_LENGTH] = { '1', '2', '3', '4' }; // Press 1 2 3 4 to change the profile
char profile_key_buffer[PROFILE_SEQ_LENGTH]; 
int profile_buf_index = 0;

bool profile_select_mode = false;

// Device name should match the current active profile "BarButtons"
#define BT_DEVICE_PROFILE 3
//BleKeyboard bleKeyboard(bt_device_profiles[BT_DEVICE_PROFILE].name, bt_device_profiles[BT_DEVICE_PROFILE].manufacturer, bt_device_profiles[BT_DEVICE_PROFILE].batteryLevel);
HijelHID_BLEKeyboard bleKeyboard(bt_device_profiles[BT_DEVICE_PROFILE].name, bt_device_profiles[BT_DEVICE_PROFILE].manufacturer, bt_device_profiles[BT_DEVICE_PROFILE].batteryLevel);
// For OTA updates, requires implementation, we will do this later

//#include <WiFi.h>
//#include <Update.h>
const char* SSID = "RCntrl";
const char* PSWD = "12345678";
String host = "213.147.117.10";
int port = 80;
String ota_bin_stable =  "/rcntrl-files/rcntrl-stable.bin";  // bin file name with a slash in front.
String ota_bin_preview = "/rcntrl-files/rcntrl-preview.bin"; // bin file name with a slash in front.

// Initialize keypad library
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

int last_keypad_state = IDLE; // Used to distinguish between button release from HOLD or PRESSED.

// Initialize variable that determines how long a key is pressed to the current time in milliseconds
unsigned long hold_time = millis();

// define time for a long press (ms)
const int long_press_time = 500; // was: 440
const int long_press_repeat_interval = 100;

// Possible application status'es
#define BT_DISCONNECTED 0 // 0: BT disconnected
#define CONFIG_MENU 1     // 1: BT connected (config menu)
#define MAIN_MENU 2       // 2: BT connected (main menu led off)
#define KEYMAP_STATUS 3   // 3: BT connected (config menu led keymap flash to indicate selected profile)

int app_status = BT_DISCONNECTED;

int led_mod_delays[4][2] = {  // holds the off/on pattern/times for the different status'es
  { 3000,     200  }, // BT_DISCONNECTED
  { 3000,     100 },  // CONFIG_MENU
  { 36000000, 100 },  // MAIN_MENU blink every 10h, almost never! Use { 5000, 100  } for normal blinking
  { 300,      300  }  // KEYMAP_STATUS
};

enum LedMode {
  LED_ON,
  LED_OFF,
  LED_STATUS,
  LED_FLASH,
  LED_PROFILE
};
LedMode ledMode = LED_STATUS;

struct LedController {
  bool state = LOW;
  unsigned long lastChange = 0;

  int repeat = 0;
  int completed = 0;

  int onTime = 100;
  int offTime = 100;
};

LedController led;

// Routine to send the keystrokes on a short press of the keypad
void send_short_press(KeypadEvent key, int key_action = INSTANT, int profile_offset = 0) {

  if (DEBUG) {Serial.print(F(" Sending short press key "));Serial.println(key);}

  // We're in the main menu
  if (app_status == MAIN_MENU || app_status == KEYMAP_STATUS || app_status == BT_DISCONNECTED) {
    switch (key) {
      case '1': send_single_key(active_profile + profile_offset, 0, key_action); break;
      case '2': send_single_key(active_profile + profile_offset, 1, key_action); break;
      case '3': send_single_key(active_profile + profile_offset, 2, key_action); break;
      case '4': send_single_key(active_profile + profile_offset, 3, key_action); break;
      case '5': send_single_key(active_profile + profile_offset, 4, key_action); break;
      case '6': send_single_key(active_profile + profile_offset, 5, key_action); break;
      case '7': send_single_key(active_profile + profile_offset, 6, key_action); break;
      case '8': send_single_key(active_profile + profile_offset, 7, key_action); break;
    }
  }      
}

// Routine to send the keystrokes on a long press of the keypad
void send_long_press(KeypadEvent key) {

  if (DEBUG) {Serial.print(F(" Sending long press key for button "));Serial.println(key);}

  // We're in the main menu, or offline
  if (app_status == MAIN_MENU || app_status == KEYMAP_STATUS || app_status == BT_DISCONNECTED) {
    switch (key) {
      case '1': send_repeating_key(active_profile, 0); break;
      case '2': send_repeating_key(active_profile, 1); break;
      case '3': send_repeating_key(active_profile, 2); break;
      case '4': send_repeating_key(active_profile, 3); break;
      case '5': send_repeating_key(active_profile, 4); break;
      case '6': send_repeating_key(active_profile, 5); break;
      case '7': send_repeating_key(active_profile, 6); break;
      case '8': send_repeating_key(active_profile, 7); break;
    }
  }
}

// Set LED state and mark the time of the change
void setLed(bool state) {
  digitalWrite(LED_PIN, state);
  led.state = state;
  led.lastChange = millis();
}

// Flash the LED on given parameters
void ledFlash(int repeat, int onTime, int offTime) {
  ledMode = LED_FLASH;
  led.repeat = repeat;
  led.completed = 0;
  led.onTime = onTime;
  led.offTime = offTime;
  setLed(HIGH);
}
// Flash the LED to indicate current profile
void ledProfileBlink(int count) {
  ledMode = LED_PROFILE;
  led.repeat = count;
  led.completed = 0;
  led.onTime = led_mod_delays[KEYMAP_STATUS][1];
  led.offTime = led_mod_delays[KEYMAP_STATUS][0];
  setLed(HIGH);
}

void ledStatusMode() {
  ledMode = LED_STATUS;
}
void updateLed() {
  unsigned long now = millis();

  switch (ledMode) {

    case LED_FLASH:
    case LED_PROFILE:

      if (led.state && now - led.lastChange >= led.onTime) {
        setLed(LOW);
      }
      else if (!led.state && now - led.lastChange >= led.offTime) {
        led.completed++;
        if (DEBUG) {Serial.print(F(" *** LED FLASH Completed: "));Serial.print(led.completed);Serial.print(F("/"));Serial.println(led.repeat);}

        if (led.completed >= led.repeat) {
          ledMode = LED_STATUS;
          return;
        }
        setLed(HIGH);
      }
      break;

    case LED_STATUS:
      if (now - led.lastChange >= led_mod_delays[app_status][led.state]) {
        setLed(!led.state);
      }
      break;

    case LED_OFF:
      setLed(LOW);
      break;
  }
}

// Routine that waits while a key is held, returns true if the key is held
// for the entire time, otherwise false. Time is in milliseconds
bool wait_for_key_hold(int key_hold_time) {
  
  int wait_for_key_hold_start_time = millis();

  // Loop as long as both a button is held, and the time has not passed
  while (keypad.getState() == HOLD && millis() < (wait_for_key_hold_start_time + key_hold_time)) {
    delay(10); // Just wait a little
    keypad.getKey(); // update keypad event handler
  }
  
  // We are exiting the wait loop, is the button stil held?
  if (keypad.getState() == HOLD) {
      return true;
  } else {
      return false;
  }

}
// Send single key, first look in the normal profile table than in the media profile table
void send_single_key(int profile, int keyIndex, int key_action) {
  // Check if we are sending normal or media keys
  if (profiles_normal[profile][keyIndex] != 0) {
    uint8_t key = profiles_normal[profile][keyIndex];
    if (key_action == INSTANT) {
      bleKeyboard.write(key);
      if (DEBUG) { Serial.print(F(" Sending normal INSTANT key: "));  Serial.println(normalKeyToString(profiles_normal[profile][keyIndex])); }
    } else if (key_action == DIRECT) {
      bleKeyboard.press(key);
      if (DEBUG) { Serial.print(F(" Sending normal DIRECT key: "));  Serial.println(normalKeyToString(key)); }
    } 
  } 
  else if (profiles_media[profile][keyIndex] != 0) {
    /*
    if (key_action == INSTANT) {
      bleKeyboard.write(profiles_media[profile][keyIndex]);
      if (DEBUG) { Serial.print(F("Sending media INSTANT key: "));  Serial.println(mediaKeyToString(profiles_media[profile][keyIndex])); }
    } else if (key_action == DIRECT) {
      bleKeyboard.press(profiles_media[profile][keyIndex]);
      if (DEBUG) { Serial.print(F("Sending media DIRECT key: "));  Serial.println(mediaKeyToString(profiles_media[profile][keyIndex])); }
    }
    */
  }
  ledFlash(1, 150, 0); 
}
// Routine that sends a key repeatedly (for single char)
void send_repeating_key(int profile, int keyIndex) {
  //digitalWrite(LED_PIN, HIGH);
  bool ledState = false;
  while (keypad.getState() == HOLD) {
    // Check if we are sending normal or media keys
    if (profiles_normal[profile][keyIndex] != 0) {
      bleKeyboard.write(profiles_normal[profile][keyIndex]);
      if (DEBUG) { Serial.print(F("  Sending normal key: "));  Serial.println(profiles_normal[profile][keyIndex]); }

    } 
    else if (profiles_media[profile][keyIndex] != 0) {
      //ßbleKeyboard.write(profiles_media[profile][keyIndex]);
      //if (DEBUG) { Serial.print(F("Sending media key: "));  Serial.println(mediaKeyToString(profiles_media[profile][keyIndex])); }

    }
    delay(long_press_repeat_interval); // pause between presses
    keypad.getKey(); // update keypad event handler
    yield();
  }
  //digitalWrite(LED_PIN, LOW);
}
// Check if longpress mapping exists in the table
bool has_long_press_mapping(KeypadEvent key) {
    int idx = key - '1';  // tipke '1'..'8' -> indeks 0..7
    if (idx < 0 || idx >= NUM_KEYS) return false;

    int long_row = active_profile + NUM_PROFILES; // long part of the table

    // Provjeri media tablicu
    /*
    const uint8_t* media_key = profiles_media[long_row][idx];
    if (media_key != nullptr && media_key != 0) {
        // Ako media key postoji, provjeri normal tablicu
        char normal_key = profiles_normal[long_row][idx];
        if (normal_key == 0) {
            return true; // postoji long press mapping
        }
    }
    */
    return false; // nema mappinga
}

// Helper function: returns 0, 1, or 2 depending on the mapping
int get_key_mode(int key_index) {
  return key_actions[active_profile][key_index];
}

// Helper function to determine if a key is release"
int is_key_release(char key_pressed) {
  if (get_key_mode(key_pressed - '1') == 0) {
    if (DEBUG) { Serial.println(" Button type: RELEASE"); }
    return true;
  }
  return false;
}

// Helper function to determine if a key is supposed to be instant, or we should send a key on "key up"
int is_key_instant(char key_pressed) {
  if (get_key_mode(key_pressed - '1') == 1) {
    if (DEBUG) { Serial.println(" Button type: INSTANT"); }
    return true;
  }
  return false;
}

// Helper function to determine if a key is direct"
int is_key_direct(char key_pressed) {
  if (get_key_mode(key_pressed - '1') == 2) {
    if (DEBUG) { Serial.println(" Button type: DIRECT"); }
    return true;
  }
  return false;
}

// Function that checks the sequebnce required to change the profile
bool profile_check_sequence() {
  for (int i = 0; i < PROFILE_SEQ_LENGTH; i++) {
    if (profile_key_buffer[(profile_buf_index + i) % PROFILE_SEQ_LENGTH] != profile_change_seq[i]) {
      return false;
    }
  }
  return true;
}

// function that changes the profile to specific number
void set_profile(int idx) {
  if (idx >= 0 && idx < NUM_PROFILES) {
    active_profile = idx;
    // Change BLE advertisment name to mach profile number
    //updateBleName(bt_device_profiles[active_profile].name);
    // Save current profile to survive rebooting
    preferences.putInt("profile", active_profile);
    if (DEBUG) { Serial.print(" >>> Profile set to: "); Serial.println(active_profile + 1);}
  }
  else if (DEBUG) { Serial.print(" >>> Profile out of range: "); Serial.println(active_profile + 1);}
}

// Select a profile by pressing a button from 1-8
// Long press a button 1-4 will select profile 5-8
int resolve_profile_from_key(char key, bool longPress) {
  int profile = key - '1';  // '1' → 0, '2' → 1 ... '8' → 7

  if (profile < 0 || profile > NUM_PROFILES - 1) {
    return -1; //  wrong profile
  }

  // If long press on buttons 1–4, use profile 5–8
  if (longPress && profile >= 0 && profile <= 3) {
    return profile + 4;
  }

  return profile;
}

void change_profile (char key, bool longPress) {
  int profile = resolve_profile_from_key(key, longPress);
  if (profile >= 0) {
    set_profile(profile);
    profile_select_mode = false;
    if (DEBUG) {
      if (longPress) {
        Serial.print(F(" >>> Long press profile selected: ")); 
      } else {
        Serial.print(F(" >>> Short press profile selected: "));
      }
      Serial.println(profile + 1);
      app_status = MAIN_MENU;
    }
  }
} 


// Function for display media buttons in debug
const char* mediaKeyToString(uint16_t key)
{
  for (auto &entry : mediaKeyNames) {
    if (entry.code == key) {
      return entry.name;
    }
  }
  return "UNKNOWN_MEDIA_KEY";
}
// Function for display normal buttons in debug
const char* normalKeyToString(char key) {
  for (auto &entry : normalKeyNames) {
    if (entry.code == key) {
      return entry.name;
    }
  }
  // If ASCII is printable
  if (key >= 32 && key <= 126) {
    static char buf[2];
    buf[0] = key;
    buf[1] = '\0';
    return buf;
  }
  return "UNKNOWN_KEY: add me to the table!";
}

// Function for display app status in debug
const char* AppStatusToString(int key) {
  for (auto &entry : AppStatusNames) {
    if (entry.code == key) {
      return entry.name;
    }
  }
  return "UNKNOWN_STATUS";
}

// Function for display KeuState names in debug
const char* KeyStateToString(int key) {
  for (auto &entry : KeyStateNames) {
    if (entry.code == key) {
      return entry.name;
    }
  }
  return "UNKNOWN";
}

// This function handles events from the keypad.h library. Each event is only called once
void keypad_handler(KeypadEvent key) {

  if (DEBUG) { Serial.println(F("\nEntering keypad handler")); }

  // State changes for the buttons
  switch (keypad.getState()) {

    case PRESSED: // At the 'key down' event of a button
      if (DEBUG) { Serial.println(F(" keypad.getState = PRESSED"));}
      if (DEBUG) { Serial.print(F(" Aoo FSM: ")); Serial.println(AppStatusToString(app_status)); }
      if (is_key_instant(key) && app_status != CONFIG_MENU && app_status != KEYMAP_STATUS) { send_short_press(key, INSTANT);}
      else if (is_key_direct(key) && app_status != CONFIG_MENU && app_status != KEYMAP_STATUS) { send_short_press(key, DIRECT);}
      last_keypad_state = PRESSED;
      break;

    case HOLD: // When a button is held beyond the long_press_time value
      last_keypad_state = HOLD;  
      if (DEBUG) { Serial.println(F(" keypad.getState = HOLD")); }
      
      // Check if we are in the profile selected mode
      if (profile_select_mode) {
        change_profile(key, true);
      } 
      // If we have a direct key, we already sent bleKeyboard.press no need for repeat
      // Direct keys have no mappings or repeat requirements.
      else if (is_key_direct(key)) {
      }
      // Release key with long mapping triggers sending single key
      else  if (is_key_release(key) && has_long_press_mapping(key)) {
        // Use the long press mapping table that has an offset equals to NUM_PROFILES   
        send_short_press(key, INSTANT, NUM_PROFILES);
      }
      else {
        send_long_press(key);
      }   
      break;    

    case RELEASED: // When a button is released
      if (DEBUG) {
        Serial.println(F(" keypad.getState = RELEASED"));
        Serial.print(F(" Previous keypad.getState = "));
        Serial.println(KeyStateToString(last_keypad_state));
        Serial.print(F(" App FSM: "));
        Serial.println(AppStatusToString(app_status));
      }

      // Check if we are in the profile selected mode
      if (profile_select_mode) {
        //app_status = KEYMAP_STATUS;
        change_profile(key, false);
      } 
      else {
      // Store prevoius pressed buttons so we can change the profile if proper key sequence is typed
        profile_key_buffer[profile_buf_index] = key;
        profile_buf_index = (profile_buf_index + 1) % PROFILE_SEQ_LENGTH;

        // Check if profile change sequence has been entered
        if (profile_check_sequence()) {
          profile_select_mode = true;
          //delay(500);
          ledProfileBlink(active_profile + 1);
          if (DEBUG) {Serial.println(F(" >>> Enter profile selection mode: press 1-8 to select profile")); }
          app_status = CONFIG_MENU;
        }
      }

      // Release key, we are sending a key on "key up"
      if (last_keypad_state == PRESSED) {
        //if (!(is_key_instant(key) && app_status != CONFIG_MENU)) {
          if (is_key_release(key) && app_status != CONFIG_MENU) {
            send_short_press(key, INSTANT);
        }
      }

      // Release all keys (write() should have done this, but keys that are press()-ed should be released)
      bleKeyboard.releaseAll();
      last_keypad_state = RELEASED;
      break;
    
    case IDLE: // not sure why this generates a callback
      if (DEBUG) {
        Serial.println(F(" keypad.getState = IDLE"));
        Serial.print(F(" App FSM: ")); Serial.println(AppStatusToString(app_status));
        Serial.print(F(" Profile: ")); 
        Serial.println(active_profile + 1);
        }

      // Release all keys (write() should have done this, but keys that are press()-ed should be released)
      bleKeyboard.releaseAll();
      last_keypad_state = IDLE;
      break;
  }
}

// Arduino built-in setup loop
void setup() {
  
  Serial.begin(BaudRate);
  Serial.println("\nRCntrl_V2: BOOT OK");

  // Start bluetooth keyboard
  delay(200); 
  bleKeyboard.begin();
  Serial.println("RCntrl_V2: HijelHID BLEKeyboard OK");
  
  // Handle all keypad events through this listener
  keypad.addEventListener(keypad_handler); // Add an event listener for this keypad

  // set HoldTime
  keypad.setHoldTime(long_press_time); 
  Serial.println("RCntrl_V2: Keypad EventListener OK");
  // Enable the led to indicate we're switched on
  pinMode(LED_PIN, OUTPUT);  
  Serial.print("RCntrl_V2: LED ");
  Serial.print(LED_PIN);
  Serial.println(" OK");

  Serial.println("\nRCntrl_V2: Starting RCntrl V2 keyboard");
  String fw_version = String(firmware_version_major) + "." + String(firmware_version_minor);
  Serial.println("RCntrl_V2: Firmware version: " + fw_version);
  
  // load profile from "settings" namespace
  Serial.println("RCntrl_V2: Loading saved profile from EPROM");
  preferences.begin("settings", false);
  active_profile = preferences.getInt("profile", 0);
  set_profile(active_profile);

  Serial.println("Cntrl_V2: Setup DONE");
  delay(1000);
  // End of setup()
}

void loop() {
  
  static unsigned long lastPrint = 0;

  // Need to call this function constantly to make the keypad library work
  keypad.getKey();
  // Update LED state if required
  updateLed();

  // Blink based on BT connectivity
  //
  if (app_status == BT_DISCONNECTED && bleKeyboard.isConnected())  {
    Serial.println("\nRCntrl_V2: Keyboard connected...");
    app_status = MAIN_MENU;
  }
  if (app_status != BT_DISCONNECTED && app_status != CONFIG_MENU && !bleKeyboard.isConnected()) {
    app_status = BT_DISCONNECTED;

  }

  delay(1);yield();
}
