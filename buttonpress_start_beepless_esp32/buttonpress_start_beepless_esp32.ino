/*
 * Connects to the AMG Lab Commander timer over BLE (Nordic UART Service). On
 * each connection it mutes the beeper ("SET BEEP VOLUME 0"); each button press
 * then sends "COM START" so the timer starts silently.
 */

#include <Arduino.h>
#include "soc/soc_caps.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------
static const char *TARGET_TIMER_NAME = "AMG Lab COMM 1D8D";

static const uint8_t START_BUTTON_PIN = 12;
static const unsigned long BUTTON_DEBOUNCE_MS = 50;

static const uint8_t CONNECTED_LED_PIN = 13;

static BLEUUID serviceUUID   ("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static BLEUUID writeCharUUID ("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
static BLEUUID notifyCharUUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e");

static const char CMD_MUTE_BEEP[] = "SET BEEP VOLUME 0";
static const char CMD_START[]     = "COM START";

// Spec: SET commands need time to be processed before the next command; 100 ms
// is sufficient.
static const unsigned long COMMAND_SETTLE_MS = 100;

// -----------------------------------------------------------------------------
// Timer BLE client state
// -----------------------------------------------------------------------------
static BLEClient *pClient = nullptr;
static BLERemoteCharacteristic *pWriteChar  = nullptr;
static BLERemoteCharacteristic *pNotifyChar = nullptr;
static BLEAddress *targetDeviceAddress = nullptr;
static volatile bool connectedToTimer = false;
static volatile bool haveAddress      = false;

// -----------------------------------------------------------------------------
// BLE callbacks — flags only; no nested BLE operations
// -----------------------------------------------------------------------------
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *) override {
    Serial.println("[Client] Linked to timer.");
  }

  void onDisconnect(BLEClient *) override {
    connectedToTimer = false;
    pWriteChar = nullptr;
    pNotifyChar = nullptr;
    Serial.println("[Client] Link to timer lost.");
  }
};

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    if (!advertisedDevice.haveName()) return;
    if (advertisedDevice.getName() != TARGET_TIMER_NAME) return;

    Serial.printf("[Scan] Found target timer: %s (%s)\n",
                  advertisedDevice.getName().c_str(),
                  advertisedDevice.getAddress().toString().c_str());

    if (targetDeviceAddress != nullptr) {
      delete targetDeviceAddress;
      targetDeviceAddress = nullptr;
    }

    targetDeviceAddress = new BLEAddress(advertisedDevice.getAddress());
    haveAddress = true;
    advertisedDevice.getScan()->stop();
  }
};
static MyAdvertisedDeviceCallbacks scanCallbacks;

// Log everything the timer sends so its responses can be inspected.
static void notifyCallback(BLERemoteCharacteristic *, uint8_t *data,
                           size_t length, bool) {
  if (data == nullptr || length == 0) return;

  Serial.printf("[Timer @ %lums] len=%u HEX:", millis(), static_cast<unsigned>(length));
  for (size_t i = 0; i < length; ++i) Serial.printf(" %02X", data[i]);
  Serial.print(" | ASCII: ");
  for (size_t i = 0; i < length; ++i) {
    const uint8_t c = data[i];
    Serial.print((c >= 32 && c <= 126) ? static_cast<char>(c) : '.');
  }
  Serial.println();
}

// -----------------------------------------------------------------------------
// Connection helpers
// -----------------------------------------------------------------------------
static void setConnectedLed(bool on) {
  digitalWrite(CONNECTED_LED_PIN, on ? HIGH : LOW);
}

static void scanForTimer() {
  Serial.printf("[Scan] Searching for \"%s\"...\n", TARGET_TIMER_NAME);

  BLEScan *scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(&scanCallbacks);
  scan->setActiveScan(true);
  scan->start(5, false);
  scan->clearResults();
}

static bool connectToTimer() {
  if (targetDeviceAddress == nullptr) return false;

  Serial.printf("[Client] Connecting to %s...\n",
                targetDeviceAddress->toString().c_str());

  if (pClient == nullptr) {
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
  }

  if (!pClient->connect(*targetDeviceAddress)) return false;

  BLERemoteService *service = pClient->getService(serviceUUID);
  if (service == nullptr) {
    pClient->disconnect();
    return false;
  }

  pNotifyChar = service->getCharacteristic(notifyCharUUID);
  if (pNotifyChar != nullptr && pNotifyChar->canNotify()) {
    pNotifyChar->registerForNotify(notifyCallback);
  }

  pWriteChar = service->getCharacteristic(writeCharUUID);
  if (pWriteChar == nullptr) {
    pClient->disconnect();
    return false;
  }

  connectedToTimer = true;

  // Mute once per connection so a button press starts the timer immediately.
  // SET commands are only accepted while the timer is stopped; give it time to
  // process this before a start can be sent.
  sendCommand(CMD_MUTE_BEEP);
  delay(COMMAND_SETTLE_MS);

  setConnectedLed(true);
  Serial.println("[Client] Timer link active. Press the button to start.");
  return true;
}

static void sendCommand(const char *command) {
  if (!connectedToTimer || pWriteChar == nullptr) {
    Serial.printf("[Cmd] Not connected; \"%s\" not sent.\n", command);
    return;
  }

  Serial.printf("[Cmd] -> timer: %s\n", command);
  pWriteChar->writeValue((uint8_t *)command, strlen(command), false);
}

// -----------------------------------------------------------------------------
// Start button (debounced, fires once per press). Starts out assuming the
// button is held, so a press only counts after a release has been seen; a
// stuck-low or miswired pin can never start the timer on its own.
// -----------------------------------------------------------------------------
static bool startButtonPressed() {
  static int stableState = LOW;
  static int lastReading = LOW;
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
  delay(400);

  Serial.println();
  Serial.println("=== BOOT: buttonpress_start_beepless_esp32 ===");
  Serial.printf("Build: %s %s\n", __DATE__, __TIME__);

  pinMode(START_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CONNECTED_LED_PIN, OUTPUT);
  setConnectedLed(false);

  BLEDevice::init("current_starter");
}

void loop() {
  if (!connectedToTimer) {
    // The disconnect callback runs on the BLE task; update the LED here.
    setConnectedLed(false);

    if (!haveAddress) scanForTimer();

    if (haveAddress) {
      if (!connectToTimer()) {
        Serial.println("[Client] Connect failed; rescanning.");
        haveAddress = false;
        delay(1000);
      }
    } else {
      delay(500);
    }
    return;
  }

  if (startButtonPressed()) {
    sendCommand(CMD_START);
  }

  delay(5);
}
