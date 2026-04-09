#include "WiFi.h"
#include <esp_wifi.h> 
#include <vector>

// physical pin mapping
int segmentPins[] = {12, 13, 33, 25, 26, 14, 27}; 
int buttonPin = 32; 

struct ScannedNetwork {
  String ssid;
  uint8_t bssid[6];
  String macStr;
  volatile int packetCount;
};
std::vector<ScannedNetwork> foundNetworks;

byte numbers[10][7] = {
  {1, 1, 1, 1, 1, 1, 0},
  {0, 1, 1, 0, 0, 0, 0}, 
  {1, 1, 0, 1, 1, 0, 1},
  {1, 1, 1, 1, 0, 0, 1}, 
  {0, 1, 1, 0, 0, 1, 1},
  {1, 0, 1, 1, 0, 1, 1},
  {1, 0, 1, 1, 1, 1, 1}, 
  {1, 1, 1, 0, 0, 0, 0}, 
  {1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 0, 1, 1}
};

// SNIFFER CALLBACK
void IRAM_ATTR sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA) return;
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  uint8_t *payload = pkt->payload;
  for (int i = 0; i < foundNetworks.size(); i++) {
    if (memcmp(foundNetworks[i].bssid, payload + 4, 6) == 0 ||
        memcmp(foundNetworks[i].bssid, payload + 10, 6) == 0 ||
        memcmp(foundNetworks[i].bssid, payload + 16, 6) == 0) {
      foundNetworks[i].packetCount++;
    }
  }
}

// STATE MANAGEMENT
enum AppState { IDLE, DISCOVERING, SNIFFING, RESULTS };
AppState currentState = IDLE;
int lastButtonState = HIGH;

// Timing variables
unsigned long stateTimer = 0;
unsigned long hopTimer = 0;
unsigned long displayTimer = 0;
int currentChannel = 1;
int displayStep = 0; 

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  for (int i = 0; i < 7; i++) pinMode(segmentPins[i], OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  
  showDash();
  Serial.println("\nSystem Ready. Press button to start.");
}

void loop() {
  handleButton();

  switch (currentState) {
    case IDLE:
      showDash();
      break;

    case DISCOVERING:
      runDiscovery();
      break;

    case SNIFFING:
      runSniffing();
      break;

    case RESULTS:
      updateDisplayResults(foundNetworks.size());
      break;
  }
}

// CORE FUNCTIONS

void handleButton() {
  int btn = digitalRead(buttonPin);
  if (btn == LOW && lastButtonState == HIGH) {
    delay(50); // Debounce
    if (currentState == IDLE || currentState == RESULTS) {
      startDiscovery();
    }
  }
  lastButtonState = btn;
}

void startDiscovery() {
  Serial.println("\nLooking for connections...");
  foundNetworks.clear();
  currentState = DISCOVERING;
  stateTimer = millis();
}

void runDiscovery() {
  // 10 second scan
  if (millis() - stateTimer < 10000) {
    animateCircle();
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i) {
      uint8_t* b = WiFi.BSSID(i);
      bool exists = false;
      for (auto &net : foundNetworks) { if (memcmp(net.bssid, b, 6) == 0) { exists = true; break; } }
      if (!exists) {
        ScannedNetwork sn; sn.ssid = WiFi.SSID(i); memcpy(sn.bssid, b, 6); sn.macStr = WiFi.BSSIDstr(i); sn.packetCount = 0;
        foundNetworks.push_back(sn);
        Serial.printf("[%d] %s | %s\n", foundNetworks.size(), sn.ssid == "" ? "[Hidden]" : sn.ssid.c_str(), sn.macStr.c_str());
      }
    }
    WiFi.scanDelete();
  } else {
    startSniffing();
  }
}

void startSniffing() {
  Serial.println("\nDiscovery complete. Found: " + String(foundNetworks.size()));
  Serial.println("Observing packets (20s)...");
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&sniffer_callback);
  currentState = SNIFFING;
  stateTimer = millis();
  hopTimer = millis();
}

void runSniffing() {
  if (millis() - stateTimer < 20000) {
    // Hop channels every 150ms (FAST!)
    if (millis() - hopTimer > 150) {
      currentChannel = (currentChannel % 13) + 1;
      esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
      hopTimer = millis();
    }
    // Update display simultaneously
    updateDisplayResults(foundNetworks.size());
  } else {
    finishAnalysis();
  }
}

void finishAnalysis() {
  esp_wifi_set_promiscuous(false);
  Serial.println("\n--- FINAL CONGESTION REPORT ---");
  for (auto &net : foundNetworks) {
    Serial.printf("%-27s | %d packets\n", net.ssid == "" ? "[Hidden]" : net.ssid.c_str(), net.packetCount);
  }
  currentState = RESULTS;
  Serial.println("\nFinished. Press button to restart.");
}

void updateDisplayResults(int num) {
  unsigned long now = millis();
  
  // single digit logic
  if (num <= 9) {
    if (displayStep == 0) { // show number
      displayNumber(num);
      if (now - displayTimer > 1000) { displayStep = 1; displayTimer = now; }
    } else { // Pause
      clearDisplay();
      if (now - displayTimer > 3000) { displayStep = 0; displayTimer = now; }
    }
  } 
  // double digit logic
  else {
    if (displayStep == 0) { // Tens
      displayNumber(num / 10);
      if (now - displayTimer > 1000) { displayStep = 1; displayTimer = now; }
    } else if (displayStep == 1) { // gap
      clearDisplay();
      if (now - displayTimer > 200) { displayStep = 2; displayTimer = now; }
    } else if (displayStep == 2) { // units
      displayNumber(num % 10);
      if (now - displayTimer > 1000) { displayStep = 3; displayTimer = now; }
    } else { // 3s pause
      clearDisplay();
      if (now - displayTimer > 3000) { displayStep = 0; displayTimer = now; }
    }
  }
}

void animateCircle() {
  int animPins[] = {12, 13, 33, 25, 26, 14};
  if (millis() - displayTimer > 80) {
    displayStep = (displayStep + 1) % 6;
    clearDisplay();
    digitalWrite(animPins[displayStep], HIGH);
    displayTimer = millis();
  }
}

void displayNumber(int num) { for (int i = 0; i < 7; i++) digitalWrite(segmentPins[i], numbers[num % 10][i]); }
void clearDisplay() { for (int i = 0; i < 7; i++) digitalWrite(segmentPins[i], LOW); }
void showDash() { clearDisplay(); digitalWrite(segmentPins[6], HIGH); }