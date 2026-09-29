/*
 * current_starter
 *
 * Connects to the AMG Lab Commander timer over BLE (Nordic UART Service) and
 * sends "COM START" whenever the start button is pressed.
 *
 * Board  : Arduino Nano 33 BLE Rev2 (Tools > Board > Arduino Mbed OS Nano
 *          Boards > Arduino Nano 33 BLE).
 * Library: ArduinoBLE.
 * Button : D2, active low with the internal pull-up. Wire a push button
 *          between D2 and GND.
 * LED    : the built-in LED is on while the timer link is active.
 */

#include <ArduinoBLE.h>

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------
static const char *TARGET_TIMER_NAME = "AMG Lab COMM 1D8D";

static const uint8_t START_BUTTON_PIN = 2;
static const unsigned long BUTTON_DEBOUNCE_MS = 50;

static const char *SERVICE_UUID     = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
static const char *WRITE_CHAR_UUID  = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
static const char *NOTIFY_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

static const char CMD_START[] = "COM START";

// -----------------------------------------------------------------------------
// Timer BLE central state
// -----------------------------------------------------------------------------
static BLEDevice timer;
static BLECharacteristic writeChar;
static BLECharacteristic notifyChar;
static bool connectedToTimer = false;
static bool scanning = false;

// Log everything the timer sends so its responses can be inspected.
static void logTimerPacket(const uint8_t *data, int length) {
  Serial.print("[Timer @ ");
  Serial.print(millis());
  Serial.print("ms] len=");
  Serial.print(length);
  Serial.print(" HEX:");
  for (int i = 0; i < length; ++i) {
    Serial.print(data[i] < 0x10 ? " 0" : " ");
    Serial.print(data[i], HEX);
  }
  Serial.print(" | ASCII: ");
  for (int i = 0; i < length; ++i) {
    const uint8_t c = data[i];
    Serial.print((c >= 32 && c <= 126) ? static_cast<char>(c) : '.');
  }
  Serial.println();
}

// -----------------------------------------------------------------------------
// Connection helpers
// -----------------------------------------------------------------------------
static void startScan() {
  if (scanning) return;
  Serial.print("[Scan] Searching for \"");
  Serial.print(TARGET_TIMER_NAME);
  Serial.println("\"...");
  BLE.scanForName(TARGET_TIMER_NAME);
  scanning = true;
}

static void dropTimer(const char *reason) {
  Serial.print("[Client] ");
  Serial.println(reason);
  if (timer.connected()) timer.disconnect();
  connectedToTimer = false;
  digitalWrite(LED_BUILTIN, LOW);
}

static bool connectToTimer(BLEDevice &device) {
  Serial.print("[Scan] Found target timer: ");
  Serial.print(device.localName());
  Serial.print(" (");
  Serial.print(device.address());
  Serial.println(")");

  BLE.stopScan();
  scanning = false;
  timer = device;

  Serial.println("[Client] Connecting...");
  if (!timer.connect()) {
    Serial.println("[Client] Connect failed; rescanning.");
    return false;
  }

  if (!timer.discoverService(SERVICE_UUID)) {
    dropTimer("UART service not found; disconnecting.");
    return false;
  }

  writeChar = timer.characteristic(WRITE_CHAR_UUID);
  if (!writeChar) {
    dropTimer("Write characteristic not found; disconnecting.");
    return false;
  }

  notifyChar = timer.characteristic(NOTIFY_CHAR_UUID);
  if (notifyChar && notifyChar.canSubscribe()) {
    if (!notifyChar.subscribe()) {
      Serial.println("[Client] Notify subscribe failed; timer replies will not be logged.");
    }
  }

  connectedToTimer = true;
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("[Client] Timer link active. Press the button to start.");
  return true;
}

static void sendCommand(const char *command) {
  if (!connectedToTimer || !writeChar) {
    Serial.print("[Cmd] Not connected; \"");
    Serial.print(command);
    Serial.println("\" not sent.");
    return;
  }

  Serial.print("[Cmd] -> timer: ");
  Serial.println(command);
  if (!writeChar.writeValue(reinterpret_cast<const uint8_t *>(command), strlen(command))) {
    Serial.println("[Cmd] Write failed.");
  }
}

// -----------------------------------------------------------------------------
// Start button (debounced, fires once per press)
// -----------------------------------------------------------------------------
static bool startButtonPressed() {
  static int stableState = HIGH;
  static int lastReading = HIGH;
  static unsigned long lastChangeAt = 0;

  const int reading = digitalRead(START_BUTTON_PIN);
  const unsigned long now = millis();

  if (reading != lastReading) {
    lastReading = reading;
    lastChangeAt = now;
  }

  if (now - lastChangeAt >= BUTTON_DEBOUNCE_MS && reading != stableState) {
    stableState = reading;
    return stableState == LOW;
  }
  return false;
}

// -----------------------------------------------------------------------------
// Arduino setup/loop
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  // Native USB: wait briefly for the Serial Monitor, but run standalone too.
  const unsigned long started = millis();
  while (!Serial && millis() - started < 3000) {}

  Serial.println();
  Serial.println("=== BOOT: current_starter ===");
  Serial.print("Build: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);

  pinMode(START_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  if (!BLE.begin()) {
    Serial.println("[Boot] FATAL: BLE init failed.");
    while (true) delay(1000);
  }
}

void loop() {
  BLE.poll();

  if (!connectedToTimer) {
    startScan();
    BLEDevice found = BLE.available();
    if (found) connectToTimer(found);
    return;
  }

  if (!timer.connected()) {
    dropTimer("Link to timer lost.");
    return;
  }

  if (notifyChar && notifyChar.valueUpdated()) {
    logTimerPacket(notifyChar.value(), notifyChar.valueLength());
  }

  if (startButtonPressed()) {
    sendCommand(CMD_START);
  }
}
