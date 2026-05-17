/*
  MAPPING TABLE VERSION 7
   Mapping table layout (per profile):

   profiles_normal[NUM_PROFILES*2][NUM_KEYS]
   profiles_media [NUM_PROFILES*2][NUM_KEYS]

   ┌───────────────────────────┬───────────────────────────┐
   │   Short press mappings    │   Long press mappings     │
   │   (rows 0–7 = profiles)   │   (rows 8–15 = profiles)  │
   └───────────────────────────┴───────────────────────────┘

   Example (profile 1 shown):
   
   profiles_normal[0][keyX] → short press normal key
   profiles_media [0][keyX] → short press media key

   profiles_normal[8][keyX] → long press normal key
   profiles_media [8][keyX] → long press media key

   Lookup logic:
     1. On short press → check normal[profile], fallback to media[profile].
     2. On long press  → check normal[profile+NUM_PROFILES], 
                         if empty then check media[profile+NUM_PROFILES].
*/

#define NUM_PROFILES 8  // 8 profiles, enough for now, Bill Gates - "640K ought to be enough for anybody"
#define NUM_KEYS     8  // 8 buttons in each profile, with the single controller you will have only 4 buttons

// We maintain two separate lookup tables for key profiles:
//
// 1. normal_profiles:
//    - Stores standard keyboard keys (letters, numbers, symbols, arrows, etc.).
//    - These are represented as char values (e.g. 'a', '-', '1') or special
//      constants like KEY_RETURN, KEY_UP, etc.
//    - Use this table when you want the keypad to behave like a regular keyboard.
//
// 2. media_profiles:
//    - Stores media control keys (e.g. volume up/down, play/pause, next/prev track).
//    - These are defined in the BLE Keyboard library as `const uint16_t`
//      (HID usage codes for the consumer control report).
//    - Because media keys are defined differently than normal keys, they require
//      their own table.
//
// Table layout (for both normal_profiles and media_profiles):
//    - The first NUM_PROFILES rows are for short press mappings.
//    - The next NUM_PROFILES rows (offset by +NUM_PROFILES) are for long press mappings.
//      Example: normal_profiles[active_profile][key]         → short press mapping
//               normal_profiles[active_profile+NUM_PROFILES][key] → long press mapping
//
// Lookup order when a button is pressed:
//    - First, the program checks the active entry in normal_profiles (short or long depending on press).
//    - If the entry is valid (not empty / not 0), it sends the normal key.
//    - If the normal_profiles entry is empty (0), the program falls back to
//      the corresponding entry in media_profiles and sends that instead.
//
// This way, each button can either send a normal key or, if unused in the normal table,
// act as a media key — and both short and long press actions can be mapped independently

// Normal Keys
char profiles_normal[NUM_PROFILES*2][NUM_KEYS] = {
  // -----------------SHORT PRESS-------------------------
  // Profil 1 short press
  { '=', '-', 'r','c', KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_DOWN }, // Original RCntrl P.1 + arrow buttons for the second controller
  // Profil 2 short press
  { 0, 0, 0, 0, KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_DOWN }, // Original RCntrl P.2  + arrow buttons for the second controller
 // Profil 3 short press
  { 0, 0, 0, 0, KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_DOWN }, // Media profile with Volume/Play/Pause/Stop/Prev/Next + longpress for PLAY/PAUSE/STOP + arrow buttons for the second controller
  // Profil 4 short press
   { KEY_F6, KEY_F7, KEY_RETURN, KEY_ESCAPE, KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_DOWN },  // DMD2
  // Profil 5 short press
  { KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8 }, // generic profile with Function keys that can be mapped inside application
  // Profil 6 short press
  { '+', '-', 'D', 'C', KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_DOWN }, // Navigation
  // Profil 7 short press
  { KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8 }, //SpeedoX MyRide
  // Profil 8 short press
  { KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_DOWN, KEY_F6, KEY_F7, KEY_RETURN, KEY_ESCAPE },  // Inverted DMD2
  // -----------------LONG PRESS-------------------------
  // Profil 1 long press
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // Profil 2 long press
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // Profil 3 long press
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // Profil 4 long press
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // Profil 5 long press
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // Profil 6 long press
  { 0, 0, 'D', 0, 0, 0, 0, 0 }, // send D as a longpressa button
  // Profil 7 long press
  { KEY_F9, KEY_F10, KEY_F11, 0, KEY_F12, 0, 0, 0 },
  // Profil 8 long press
  { 0, 0, 0, 0, 0, 0, 0, 0 },
};

// Media Keys
uint16_t profiles_media[NUM_PROFILES*2][NUM_KEYS] = {
 // -----------------SHORT PRESS-------------------------
 // Profil 1 short press
  { 0, 0, 0, 0, 0, 0, 0, 0 }, 
  // Profil 2 short press
  { MEDIA_PREV_TRACK, MEDIA_NEXT_TRACK, MEDIA_VOLUME_DOWN, MEDIA_VOLUME_UP, 0, 0, 0, 0 }, 
  // Profil 3 short press
  //{ 0, 0, 0, 0, 0, 0, 0, 0 },
  { MEDIA_NEXT_TRACK, MEDIA_PREV_TRACK, MEDIA_VOLUME_UP, MEDIA_VOLUME_DOWN, 0, 0, 0, 0 }, 
  // Profil 4 short press
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // Profil 5 short press
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // Profil 6 short press
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // Profil 7 short press
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // Profil 8 short press
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // If there is a mapping for long press, instead of sending repeating key, single key with new mapping is sent
  // -----------------LONG PRESS-------------------------  
  // Profil 1 long press 
  { 0, 0, 0, 0, 0, 0, 0, 0 },
    // Profil 2 long press 
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // Profil 3 long press 
  // if you use long press keys, make sure that you configure those keys as non-instant by modifying "key_actions" table
  // if you leave keys as instant keys, they will first send normal key and than long press key
  // Example for profile2: 
  //    first two buttons are non-instant, meaning key will be sent when you release the button (no repeat of course)
  //    button 1: * short press - > MEDIA_NEXT_TRACK       button 2: * short press - > MEDIA_PREV_TRACK
  //              * long press  - > MEDIA_PLAY_PAUSE                  * long press  - > MEDIA_STOP
  { MEDIA_PLAY_PAUSE, MEDIA_STOP, 0, 0, 0, 0, 0, 0 },
  // Profil 4 long press 
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // Profil 5 long press 
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // Profil 6 long press 
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // Profil 7 long press 
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  // Profil 8 long press 
  { 0, 0, 0, 0, 0, 0, 0, 0 }
};

// Status mapping table for keys in each profile
#define  RELEASE 0   // fire on key release
#define  INSTANT 1   // fire immediately on press
#define  DIRECT  2   // bypass normal handling, send keypress
int key_actions[NUM_PROFILES][NUM_KEYS] = {
  {INSTANT, INSTANT, INSTANT, INSTANT, INSTANT, INSTANT, INSTANT, INSTANT}, // profile 1
  {INSTANT, INSTANT, INSTANT, INSTANT, INSTANT, INSTANT, INSTANT, INSTANT}, // profile 2
  {RELEASE, RELEASE, INSTANT, INSTANT, INSTANT, INSTANT, INSTANT, INSTANT}, // profile 3
  {DIRECT,  DIRECT,  DIRECT,  DIRECT,  DIRECT,  DIRECT,  DIRECT,  DIRECT}, // profile 4
  {INSTANT, INSTANT, INSTANT, INSTANT, INSTANT, INSTANT, INSTANT, INSTANT}, // profile 5
  {INSTANT, INSTANT, RELEASE, INSTANT, INSTANT, INSTANT, INSTANT, INSTANT}, // profile 6
  {DIRECT,  DIRECT,  DIRECT,  DIRECT,  DIRECT,  DIRECT,  DIRECT,  DIRECT},  // profile 7
  {DIRECT,  DIRECT,  DIRECT,  DIRECT,  DIRECT,  DIRECT,  DIRECT,  DIRECT}  // profile 8
};