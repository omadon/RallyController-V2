
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

## License

This work is licensed under the **Creative Commons Attribution-NonCommercial 4.0 International License**.  
To view a copy of this license, visit [CC BY-NC 4.0](http://creativecommons.org/licenses/by-nc/4.0/)  
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
