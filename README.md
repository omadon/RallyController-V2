
# RCntrl Firmware v2 by Omadon

**Version:** 2.20

Custom firmware for the StylesRallyIndustries Bluetooth Navigation / Digital Roadbook Controller.

**Author:** kresimir.petrovic@gmail.com

- [Instructables Guide](https://www.instructables.com/Bluetooth-Navigation-Digital-Roadbook-Controller-4/)
- [Original Firmware v1.1](https://github.com/StylesRallyIndustries/RallyController/releases/tag/v1.1)
- [BarButtons v1 Reference](https://jaxeadv.com/barbuttons-files/barbuttons.ino)

---

## Required Libraries

- [ESP32-BLE-Keyboard v0.3.2-beta](https://github.com/T-vK/ESP32-BLE-Keyboard/releases/tag/0.3.2-beta) – install via `.zip` file  
- Bounce2 – install via Arduino IDE (version 2.71)

---

## Differences from Original Firmware

- Uses the **Keypad library** with an event handler for simpler button state handling.
- Supports **two controllers in daisy chain**, connected to each other or to the controller box.
- Keypad scanning allows daisy chaining using **UTP cables** (5/6 wires for buttons + 2 wires for LED).
- **Up to 8 different profiles:**
  - Short press of buttons 1–8 → select profiles 1–8  
  - Long press of buttons 1–4 → select profiles 5–8
- **Button mapping tables** (normal + media) are split into two halves:
  - First half → short press mappings  
  - Second half → long press mappings  
  - Long press uses dedicated mapping if defined; otherwise short-press behavior repeats.
- Profile change sequence: press **1-2-3-4** then the button number of desired profile.  
- Selected profile is saved in **NVS**, surviving controller reboot.  
- Profiles can send **normal keys** (letters, numbers, arrows, etc.) or **media keys** (volume/playback).  
  Lookup order: **normal → media**.  
- **BTLE advertisement** changes based on selected profile (e.g., "RCntrl V2 P.1").  
  > iOS users: be aware of BTLE advertisement limitations.
- **LED support** (external or internal):
  - Blinking when not connected to Bluetooth  
  - Optional keepalive blinking (default 10h)  
  - Blink on each button press  
  - Profile change feedback: LED blinks number of selected profile after 1-2-3-4 sequence
- Set `DEBUG = 0` for production use.

---

## Mapping Table Layout (per profile)

```
profiles_normal[NUM_PROFILES*2][NUM_KEYS]
profiles_media [NUM_PROFILES*2][NUM_KEYS]

┌───────────────────────────┬───────────────────────────┐
│   Short press mappings    │   Long press mappings     │
│   (rows 0–7 = profiles)   │   (rows 8–15 = profiles)  │
└───────────────────────────┴───────────────────────────┘
```

**Example (Profile 1):**

- `profiles_normal[0][keyX]` → short press normal key  
- `profiles_media[0][keyX]` → short press media key  
- `profiles_normal[8][keyX]` → long press normal key  
- `profiles_media[8][keyX]` → long press media key  

**Lookup logic:**

1. On short press → check `normal[profile]`, fallback to `media[profile]`  
2. On long press → check `normal[profile+NUM_PROFILES]`, if empty then check `media[profile+NUM_PROFILES]`

---
## 🎛️ Profiles & Key Mapping Guide

The firmware supports **8 independent profiles**, each with:
- **8 button slots** (your controller may only use 4 buttons, but the code is scalable).  
- **Two actions per button**:  
  - **Short press**  
  - **Long press**

Each button can be mapped to:
1. **Normal keys (keyboard keys)**  
   → letters, numbers, arrows, function keys (`'a'`, `'1'`, `KEY_RETURN`, `KEY_UP_ARROW`, …).  
   → defined in `profiles_normal`.

2. **Media keys (consumer/media controls)**  
   → volume, play/pause, next/previous track (`KEY_MEDIA_PLAY_PAUSE`, `KEY_MEDIA_VOLUME_UP`, …).  
   → defined in `profiles_media`.

3. **Instant keys (timing control)**  
   → determines *when* a key event is sent:  
   - **Instant (1)** → key is sent immediately on button down.  
   - **Non-instant (0)** → key is sent on button release.  
   → defined in `instant_keys`.

---

## 🔑 How profiles are structured

- `profiles_normal`:  
  - First 8 rows → short press mappings (profiles 1–8).  
  - Next 8 rows → long press mappings (profiles 9–16).  

- `profiles_media`:  
  - Same structure as above (short first, then long).  

Example:
```cpp
// Profile 4, short press
{ KEY_F6, KEY_F7, KEY_RETURN, KEY_F5, KEY_UP_ARROW, KEY_LEFT_ARROW, KEY_RIGHT_ARROW, KEY_DOWN_ARROW },

// Profile 4, long press
{ 0, 0, 0, 0, 0, 0, 0, 0 },
```

---

## ✏️ Editing a profile

1. Open `keymapings.h`.
2. Find the profile you want to edit.
3. Update `profiles_normal` (for standard keyboard keys) or `profiles_media` (for media keys).  
   - Use `0` in one table if you want the key to come from the other table.  
4. Adjust `instant_keys` if you want to enable long press behavior.  
   - Keys set to `'0'` are delayed until button release (non-instant).  
   - Keys set to `'1'` are sent immediately (instant).

---

## 🛠 Example: Custom media profile

Let’s configure **Profile 2** like this:
- Button 1 → short press = Play/Pause, long press = Stop  
- Button 2 → short press = Next, long press = Previous  

Code changes:
```cpp
// profiles_normal (Profile 2 short press)
{ 0, 0, 0, 0, KEY_UP_ARROW, KEY_LEFT_ARROW, KEY_RIGHT_ARROW, KEY_DOWN_ARROW },

// profiles_media (Profile 2 short press)
{ KEY_MEDIA_PLAY_PAUSE, KEY_MEDIA_NEXT_TRACK, 0, 0, 0, 0, 0, 0 },

// profiles_media (Profile 2 long press)
{ KEY_MEDIA_STOP, KEY_MEDIA_PREVIOUS_TRACK, 0, 0, 0, 0, 0, 0 },

// instant_keys (Profile 2)
{'0', '0', '3', '4', '5', '6', '7', '8'},
```

Explanation:  
- `profiles_normal` is filled with zeros for Button 1 & 2 → they are resolved via `profiles_media`.  
- `instant_keys` marks Button 1 & 2 as **non-instant** (so long press detection works).  

---

## 📋 Default Profiles Overview

| Profile | Short Press (Normal) | Short Press (Media) | Long Press (Normal) | Long Press (Media) | Instant Keys |
|---------|----------------------|----------------------|----------------------|--------------------|--------------|
| 1 | `=, -, r, c, ↑, ←, →, ↓` | – | – | – | All instant |
| 2 | `–` | `Prev, Next, Vol-, Vol+` | – | – | All instant |
| 3 | `–` | `Next, Prev, Vol+, Vol-` | – | `Play/Pause, Stop` | Btn 1&2 non-instant |
| 4 | `F6, F7, Enter, F5, ↑, ←, →, ↓` | – | – | – | All instant |
| 5 | `F1–F8` | – | – | – | All instant |
| 6 | `=, -, N, C, ↑, ←, →, ↓` | – | `D` (Btn3) | – | Btn 3 non-instant |
| 7 | `F1–F8` | `Prev, Next, Play/Pause` | `F9–F12` | – | All non-instant |
| 8 | `↑, ←, →, ↓, F6, F7, Enter, F5` | – | – | – | All instant |

---

## 🚀 Workflow

1. Edit `keymapings.h`.  
2. Recompile the firmware.  
3. Flash the firmware to ESP32.  
4. Switch to the desired profile and test.  

---

## License

This work is licensed under the **Creative Commons Attribution-NonCommercial 4.0 International License**.  
To view a copy of this license, visit [CC BY-NC 4.0](http://creativecommons.org/licenses/by-nc/4.0/)  
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
