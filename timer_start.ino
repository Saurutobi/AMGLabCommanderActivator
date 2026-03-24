#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEUtils.h>

// Hardcoded MAC address of the timer — change this to match your device
#define TIMER_MAC "AA:BB:CC:DD:EE:FF"

// Nordic UART Service UUIDs
static BLEUUID NUS_SERVICE_UUID ("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID NUS_RX_CHAR_UUID ("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"); // write here to send commands
static BLEUUID NUS_TX_CHAR_UUID ("6E400003-B5A3-F393-E0A9-E50E24DCCA9E"); // notifications come from here

static BLEClient*          pClient    = nullptr;
static BLERemoteCharacteristic* pRxChar = nullptr;
static bool connected = false;

// Notification callback — prints anything the timer sends back
void notifyCallback(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  Serial.print("Timer says: ");
  for (size_t i = 0; i < length; i++) {
    Serial.print((char)pData[i]);
  }
  Serial.println();
}

bool connectToTimer() {
  BLEAddress timerAddress(TIMER_MAC);

  pClient = BLEDevice::createClient();
  Serial.println("Connecting to timer...");

  if (!pClient->connect(timerAddress)) {
    Serial.println("Connection failed.");
    return false;
  }
  Serial.println("Connected.");

  BLERemoteService* pService = pClient->getService(NUS_SERVICE_UUID);
  if (!pService) {
    Serial.println("NUS service not found.");
    pClient->disconnect();
    return false;
  }

  // Subscribe to TX characteristic for incoming data
  BLERemoteCharacteristic* pTxChar = pService->getCharacteristic(NUS_TX_CHAR_UUID);
  if (pTxChar && pTxChar->canNotify()) {
    pTxChar->registerForNotify(notifyCallback);
  }

  // Get the RX characteristic to send commands
  pRxChar = pService->getCharacteristic(NUS_RX_CHAR_UUID);
  if (!pRxChar) {
    Serial.println("NUS RX characteristic not found.");
    pClient->disconnect();
    return false;
  }

  return true;
}

void sendCommand(const char* cmd) {
  if (!pRxChar) return;
  pRxChar->writeValue((uint8_t*)cmd, strlen(cmd), true);
  Serial.print("Sent: ");
  Serial.println(cmd);
}

void setup() {
  Serial.begin(115200);
  BLEDevice::init("ESP32-TimerClient");

  if (connectToTimer()) {
    connected = true;
    sendCommand("COM START");
  }
}

void loop() {
  if (connected && !pClient->isConnected()) {
    Serial.println("Disconnected from timer.");
    connected = false;
  }
  delay(1000);
}
