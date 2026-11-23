// EEPROM-backed stand-in for the ESP32 NVS "Preferences" API (ESP8266 port).
// Implements the small Preferences subset this project uses.
// Layout v2: magic/version + force flag + three 32-byte SSIDs and 64-byte passwords.
#pragma once
#include <Arduino.h>
#include <EEPROM.h>

class Preferences {
public:
  bool begin(const char* ns, bool readOnly = false) {
    (void)ns;
    _readOnly = readOnly;
    _force = false;
    for (int i = 0; i < 3; i++) { _ssid[i] = ""; _pass[i] = ""; }
    _user = ""; _webpass = "";
    EEPROM.begin(512);
    if (EEPROM.read(0) == 0xA6) {
      _force = EEPROM.read(1) != 0;
      char tmp[65];
      for (int i = 0; i < 3; i++) {
        int base = 2 + i * 98;
        readStr(base, tmp, 33); _ssid[i] = tmp;
        readStr(base + 33, tmp, 65); _pass[i] = tmp;
      }
      readStr(296, tmp, 33); _user = tmp;
      readStr(329, tmp, 65); _webpass = tmp;
    } else if (EEPROM.read(0) == 0xA5) { // migrate the old single-network layout
      char tmp[65];
      readStr(1, tmp, 33); _ssid[0] = tmp;
      readStr(34, tmp, 65); _pass[0] = tmp;
    }
    return true;
  }

  void end() { EEPROM.end(); }

  void clear() {
    if (_readOnly) return;
    _force = false;
    for (int i = 0; i < 3; i++) { _ssid[i] = ""; _pass[i] = ""; }
    _user = ""; _webpass = "";
    write();
  }

  String getString(const char* key, const char* def = "") {
    String k(key);
    if (k == "ssid" || k == "s0") return _ssid[0];
    if (k == "pass" || k == "p0") return _pass[0];
    if (k.length() == 2 && k[0] == 's' && k[1] >= '1' && k[1] <= '2') return _ssid[k[1] - '0'];
    if (k.length() == 2 && k[0] == 'p' && k[1] >= '1' && k[1] <= '2') return _pass[k[1] - '0'];
    if (k == "user") return _user.length() ? _user : String(def);
    if (k == "webpass") return _webpass.length() ? _webpass : String(def);
    return String(def);
  }

  void putString(const char* key, const String& value) {
    if (_readOnly) return;
    String k(key);
    if (k == "ssid" || k == "s0") _ssid[0] = value;
    else if (k == "pass" || k == "p0") _pass[0] = value;
    else if (k.length() == 2 && k[0] == 's' && k[1] >= '1' && k[1] <= '2') _ssid[k[1] - '0'] = value;
    else if (k.length() == 2 && k[0] == 'p' && k[1] >= '1' && k[1] <= '2') _pass[k[1] - '0'] = value;
    else if (k == "user") _user = value;
    else if (k == "webpass") _webpass = value;
    else return;
    write();
  }

  bool getBool(const char* key, bool def = false) { return String(key) == "force" ? _force : def; }
  void putBool(const char* key, bool value) { if (!_readOnly && String(key) == "force") { _force = value; write(); } }
  void remove(const char* key) {
    if (_readOnly) return;
    String k(key);
    if (k == "force") _force = false;
    else if (k == "ssid") _ssid[0] = "";
    else if (k == "pass") _pass[0] = "";
    write();
  }

private:
  bool _readOnly = false;
  bool _force = false;
  String _ssid[3], _pass[3], _user, _webpass;

  void readStr(int addr, char* out, size_t n) {
    for (size_t i = 0; i < n - 1; i++) {
      uint8_t c = EEPROM.read(addr + i);
      if (!c) { out[i] = 0; return; }
      out[i] = (char)c;
    }
    out[n - 1] = 0;
  }

  void writeStr(int addr, const String& s, size_t n) {
    for (size_t i = 0; i < n - 1; i++) EEPROM.write(addr + i, i < s.length() ? (uint8_t)s[i] : 0);
    EEPROM.write(addr + n - 1, 0);
  }

  void write() {
    EEPROM.write(0, 0xA6);
    EEPROM.write(1, _force ? 1 : 0);
    for (int i = 0; i < 3; i++) {
      int base = 2 + i * 98;
      writeStr(base, _ssid[i], 33);
      writeStr(base + 33, _pass[i], 65);
    }
    writeStr(296, _user, 33);
    writeStr(329, _webpass, 65);
    EEPROM.commit();
  }
};
