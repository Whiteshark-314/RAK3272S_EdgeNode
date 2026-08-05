// Pico W <-> RAK3272S wiring:
//   Pico GPIO2            -> RAK3272S RST
//   Pico GPIO4 (UART1 TX) -> RAK3272S RX
//   Pico GPIO5 (UART1 RX) -> RAK3272S TX
//   Pico 3V3(OUT)         -> RAK3272S VDD
//   Pico GND              -> RAK3272S GND
//
// OTAA join on boot (Class C, unconfirmed uplinks), then one uplink every
// UPLINK_INTERVAL_MS carrying real MCU core temperature plus two clearly-
// labeled dummy values (see sendSensorUplink() below for the payload layout).
// Serial Monitor (USB, 115200) also stays live for manual AT commands.

#define RAK3272_RST 2

// OTAA_APPEUI / OTAA_APPKEY live in secrets.h, which is gitignored -- copy
// secrets.h.example to secrets.h and fill in your device's real credentials.
// DevEUI: left as the module's factory-programmed EUI (not set here).
// This network server requires AppEUI == DevEUI.
#include "secrets.h"
#define LORA_BAND     "3"   // 3 = IN865

#define UPLINK_PORT        2
#define UPLINK_INTERVAL_MS (5UL * 60UL * 1000UL)  // 5 minutes

String currentCommand = "";
bool awaitingResponse = false;
bool joined = false;
unsigned long lastUplinkMs = 0;
uint8_t dummyCounter = 0;

// Hardware-reset the RAK3272S via its RST pin and print whatever it sends back
// afterward (boot banner, garbage, or nothing) so wiring/baud issues are visible.
void resetModule() {
  Serial.println("Resetting RAK3272S...");
  while (Serial2.available()) Serial2.read(); // flush stale bytes

  digitalWrite(RAK3272_RST, LOW);
  delay(100);
  digitalWrite(RAK3272_RST, HIGH);

  String banner = "";
  unsigned long start = millis();
  while (millis() - start < 3000) {
    while (Serial2.available()) {
      banner += (char)Serial2.read();
      start = millis(); // keep listening as long as bytes keep arriving
    }
  }

  Serial.print("<< Boot output: ");
  Serial.println(banner.length() ? banner : "(nothing received)");
}

// Send an AT command to the RAK3272S and collect its response for a fixed window.
String sendAT(String cmd, unsigned long timeoutMs = 2000) {
  while (Serial2.available()) Serial2.read(); // flush stale bytes

  Serial.print(">> ");
  Serial.println(cmd);
  Serial2.print(cmd);
  Serial2.print("\r\n");

  String response = "";
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    while (Serial2.available()) {
      response += (char)Serial2.read();
    }
  }
  Serial.print("<< ");
  Serial.println(response);
  return response;
}

// Query AT+<name>=? and only send AT+<name>=<expected> if it isn't already set that
// way. RUI3 persists config to flash, so re-setting unchanged values on every boot
// just wastes flash writes (and AT+NWM can force a reboot even when unchanged).
// Returns true if a set command was actually sent (i.e. the value changed).
//
// The module echoes the query itself (e.g. "AT+CLASS=?") before answering with the
// real value line (e.g. "AT+CLASS=A"). A plain substring search for the expected
// value anywhere in that response is unsound -- e.g. expected="C" would falsely
// match the "C" inside the echoed "AT+CLASS=?"/"AT+CLASS" text. Instead, anchor on
// the LAST "AT+<name>=" occurrence (the real value line, since it comes after the
// echoed query) and compare only the token that follows it.
bool ensureSetting(const char* name, const String& expected, unsigned long timeoutMs = 2000) {
  String resp = sendAT(String("AT+") + name + "=?", timeoutMs);

  String marker = String("AT+") + name + "=";
  String respUpper = resp;
  respUpper.toUpperCase();
  String markerUpper = marker;
  markerUpper.toUpperCase();

  String currentValue = "";
  int idx = respUpper.lastIndexOf(markerUpper);
  if (idx != -1) {
    int valueStart = idx + markerUpper.length();
    int lineEnd = resp.indexOf('\r', valueStart);
    if (lineEnd == -1) lineEnd = resp.indexOf('\n', valueStart);
    if (lineEnd == -1) lineEnd = resp.length();
    currentValue = resp.substring(valueStart, lineEnd);
    currentValue.trim();
  }

  String expUpper = expected;
  expUpper.toUpperCase();
  String currentValueUpper = currentValue;
  currentValueUpper.toUpperCase();

  if (currentValueUpper == expUpper) {
    Serial.println("   (already " + expected + ", skipping set)");
    return false;
  }

  sendAT(String("AT+") + name + "=" + expected, timeoutMs);
  return true;
}

// Kick off OTAA join and wait (up to timeoutMs) for +EVT:JOINED / +EVT:JOIN_FAILED.
bool doOtaaJoin(unsigned long timeoutMs = 30000) {
  Serial.println("Starting OTAA join...");
  while (Serial2.available()) Serial2.read();
  Serial2.print("AT+JOIN=1:0:10:8\r\n");

  String resp = "";
  unsigned long start = millis();
  bool joinedOk = false;
  bool failed = false;

  while (millis() - start < timeoutMs) {
    while (Serial2.available()) {
      char c = Serial2.read();
      Serial.write(c);
      resp += c;
    }
    if (resp.indexOf("+EVT:JOINED") != -1) { joinedOk = true; break; }
    if (resp.indexOf("JOIN FAILED") != -1 || resp.indexOf("JOIN_FAILED") != -1) { failed = true; break; }
  }

  Serial.println();
  if (joinedOk) {
    Serial.println("*** OTAA JOIN SUCCESSFUL ***");
  } else if (failed) {
    Serial.println("*** OTAA JOIN FAILED ***");
  } else {
    Serial.println("*** OTAA JOIN TIMED OUT (no +EVT:JOINED seen) ***");
  }
  return joinedOk;
}

// Payload layout (5 bytes, big-endian), matching the device profile's decodeUplink():
//   [0..1] int16  mcu_temp_c_x100      -- REAL: RP2040 internal core temperature * 100
//   [2..3] int16  dummy_humidity_x100  -- DUMMY: simulated, sine wave so it visibly moves
//   [4]    uint8  dummy_counter        -- DUMMY: increments every send, wraps at 256
//
// mcu_temp_c is the chip's own die temperature (idles well above room temp), not an
// ambient/environmental reading -- named accordingly so it's never mistaken for one.
String toHex16(int16_t v) {
  char buf[5];
  snprintf(buf, sizeof(buf), "%04X", (uint16_t)v);
  return String(buf);
}

void sendSensorUplink() {
  float mcuTempC = analogReadTemp();
  float dummyHumidity = 50.0f + 15.0f * sinf(dummyCounter * 0.3f);

  int16_t tempScaled = (int16_t)lroundf(mcuTempC * 100);
  int16_t humScaled = (int16_t)lroundf(dummyHumidity * 100);

  char counterHex[3];
  snprintf(counterHex, sizeof(counterHex), "%02X", dummyCounter);
  String payload = toHex16(tempScaled) + toHex16(humScaled) + counterHex;

  Serial.printf("mcu_temp_c=%.2f  dummy_humidity=%.2f  dummy_counter=%u\n",
                mcuTempC, dummyHumidity, dummyCounter);

  sendAT(String("AT+SEND=") + UPLINK_PORT + ":" + payload, 3000);
  dummyCounter++;
}

void setup() {
  pinMode(RAK3272_RST, OUTPUT);
  digitalWrite(RAK3272_RST, LOW);

  Serial.begin(115200);
  delay(1000);
  while (!Serial) {
    ;
  }
  Serial.println("RAK3272S Edge Node - OTAA join + periodic dummy uplink");

  // Serial2 (Pico W UART1) for communication with RAK3272S UART2 (GPIO4 TX / GPIO5 RX)
  Serial2.setTX(4);
  Serial2.setRX(5);
  Serial2.setFIFOSize(2048);
  Serial2.setTimeout(1000);
  Serial2.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  resetModule();

  // --- Configure the module for OTAA on IN865 (only touches settings that are wrong) ---
  sendAT("AT");                                  // sanity check

  if (ensureSetting("NWM", "1")) {               // LoRaWAN mode (reboots module if it actually changes)
    delay(2000);
    while (Serial2.available()) Serial2.read(); // flush reboot banner
  }

  ensureSetting("NJM", "1");                     // OTAA
  ensureSetting("BAND", LORA_BAND);               // IN865
  ensureSetting("CLASS", "C");                   // production target: always-listening RX2
  ensureSetting("CFM", "0");                     // unconfirmed uplinks
  ensureSetting("APPEUI", OTAA_APPEUI);
  ensureSetting("APPKEY", OTAA_APPKEY);
  sendAT("AT+DEVEUI=?");                         // log the module's built-in DevEUI

  joined = doOtaaJoin();
  digitalWrite(LED_BUILTIN, joined ? HIGH : LOW);

  if (joined) {
    Serial.println("Sending first sensor uplink, then every 5 minutes.");
    sendSensorUplink();
    lastUplinkMs = millis();
  } else {
    Serial.println("Join failed -- not sending uplinks. You can still type AT commands manually below.");
  }
}

void loop() {
  if (joined && millis() - lastUplinkMs >= UPLINK_INTERVAL_MS) {
    sendSensorUplink();
    lastUplinkMs = millis();
  }

  // Manual AT passthrough: type a command + newline in the Serial Monitor.
  while (Serial.available()) {
    char c = Serial.read();
    currentCommand += c;
    Serial2.write(c);
    if (c == '\n') {
      Serial.print("\n>> Command: ");
      Serial.print(currentCommand);
      Serial2.flush();
      awaitingResponse = true;
      currentCommand = "";
    }
  }

  if (awaitingResponse && Serial2.available()) {
    Serial.print("<< Response: ");
    while (Serial2.available()) {
      Serial.write(Serial2.read());
      delay(1);
    }
    Serial.println();
    awaitingResponse = false;
  }
}
