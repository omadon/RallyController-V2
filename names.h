// We want real names in the debug output
struct MediaKeyName {
  const uint16_t code;
  const char* name;
};

MediaKeyName mediaKeyNames[] = {
  { MEDIA_NEXT_TRACK, "MEDIA_NEXT_TRACK" },
  { MEDIA_PREV_TRACK, "MEDIA_PREV_TRACK" },
  { MEDIA_PLAY_PAUSE, "MEDIA_PLAY_PAUSE" },
  { MEDIA_STOP, "MEDIA_STOP" },
  { MEDIA_MUTE, "MEDIA_MUTE" },
  { MEDIA_VOLUME_UP, "MEDIA_VOLUME_UP" },
  { MEDIA_VOLUME_DOWN, "MEDIA_VOLUME_DOWN" }
};

struct NormalKeyName {
  char code;
  const char* name;
};

NormalKeyName normalKeyNames[] = {
  { KEY_F1, "KEY_F1" },
  { KEY_F2, "KEY_F2" },
  { KEY_F3, "KEY_F3" },
  { KEY_F4, "KEY_F4" },
  { KEY_F5, "KEY_F5" },
  { KEY_F6, "KEY_F6" },
  { KEY_F7, "KEY_F7" },
  { KEY_F8, "KEY_F8" },
  { KEY_F9, "KEY_F9" },
  { KEY_F10, "KEY_F10" },
  { KEY_F11, "KEY_F11" },
  { KEY_F12, "KEY_F12" },
  { KEY_RETURN, "KEY_RETURN" },
  { KEY_UP, "KEY_UP" },
  { KEY_DOWN, "KEY_DOWN" },
  { KEY_LEFT, "KEY_LEFT" },
  { KEY_RIGHT, "KEY_RIGHT" },
  { KEY_TAB, "KEY_TAB" },
  { KEY_BACKSPACE, "KEY_BACKSPACE" },
  { KEY_DELETE, "KEY_DELETE" },
  { KEY_ESCAPE, "KEY_ESCAPE" }
};

// We want app status  names in the debug output
struct AppStatusName {
  const int code;
  const char* name;
};

AppStatusName AppStatusNames[] = {
  { 0, "BT_DISCONNECTED" },
  { 1, "CONFIG_MENU" },
  { 2, "MAIN_MENU" },
  { 3, "KEYMAP_STATUS" },
  { 4, "OTA" }
};

struct KeyStateName {
  const int code;
  const char* name;
};

KeyStateName KeyStateNames[] = {
  { 0, "IDLE" },
  { 1, "PRESSED" },
  { 2, "HOLD" },
  { 3, "RELEASED" }
};